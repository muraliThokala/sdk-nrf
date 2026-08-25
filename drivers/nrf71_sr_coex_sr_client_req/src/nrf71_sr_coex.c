/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF71 Wi-Fi / Short-Range coexistence driver core (SR SW client variant).
 *
 * Same post/wait CM model as the base driver, plus CD2CM_SR_SW_CLIENT_REQUEST /
 * CM2CD_SR_SW_CLIENT_STATUS_EVENT from the ROM 1.0 SR SW client firmware patch.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
#include <nrf71_coex_if.h>
#include <nrf71_cd_sr_if.h>
#include <nrf71_sr_coex_api.h>

#include "nrf71_sr_coex_internal.h"

LOG_MODULE_REGISTER(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

#define CD_CM_EVENT_WAIT_MS 500U

static const struct coex_wifi_priority_range_t default_wifi_range = {
	.sw_request_priority_range = {10, 5, 1},
	.client0_ccconf_pti_range = {20, 15, 1},
	.client1_ccconf_pti_range = {25, 20, 1},
	.client2_ccconf_pti_range = {140, 120, 3},
	.client3_ccconf_pti_range = {160, 145, 3},
	.hw_client_priority_level = 5,
};

static const struct coex_sr_priority_range_t default_sr_range = {
	.sr_rx_client_ccconf_pti_range = {30, 25, 1},
	.sr_tx_client_ccconf_pti_range = {40, 30, 2},
	.sr_rx_client_critical_ccconf_pti_range = {20, 15, 1},
	.sr_tx_client_critical_ccconf_pti_range = {25, 20, 1},
	.client_priority_level = 5,
};

static const struct coex_user_params_t default_user_params = {
	.message_id = CD2CM_UPDATE_COEX_USER_PARAMS,
	.listen2inactive_sr_rx_prot_prob_ps = 100,
	.inactive2listen_sr_rx_prot_prob_ps = 100,
	.inactive2listen_sr_rx_prot_prob_calib = 100,
	.listen2inactive_sr_rx_prot_prob_calib = 100,
	.wifi_scan_puncture_info = {.wifi_scan_prot_prob = 100},
	.wifi_beacon_prot_prob = 100,
	.wifi_conn_prot_prob = 100,
	.wifi_calib_prot_prob = 100,
	.shared_ant_control = ANT_ALLOC_STATIC_WIFI,
};

static struct {
	struct k_mutex lock;
	struct k_mutex cmd_lock;
	struct k_sem cm_event_sem;

	bool wifi_up;
	bool sr_up;
	bool sr_coex_enabled;
	bool transition_active;
	bool coexc_configured;

	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;
	struct coex_user_params_t user_params;

	struct cm2cd_event_status_name_t last_event;
	struct cm_stats_t last_stats;
	struct cm_fsm_patch_stats_sr_t last_patch_stats;
	bool stats_valid;
	bool patch_stats_valid;
} cd;

__weak unsigned int coex_sr_enable(unsigned int enable_coex)
{
	ARG_UNUSED(enable_coex);
	return 1U;
}

__weak unsigned int coex_sr_set_client_priority(
	const struct coex_sr_priority_range_t *sr_priority_range)
{
	ARG_UNUSED(sr_priority_range);
	return 1U;
}

static int cd_wait_for_cm_event(enum cm_event_to_host_t event_name, k_timeout_t timeout)
{
	enum cm_event_to_host_t received;

	while (k_sem_take(&cd.cm_event_sem, timeout) == 0) {
		k_mutex_lock(&cd.lock, K_FOREVER);
		received = (enum cm_event_to_host_t)cd.last_event.event_name;
		k_mutex_unlock(&cd.lock);

		if (received == event_name) {
			return 0;
		}

		LOG_DBG("CM2CD event %u (status=%u) while waiting for %u",
			(unsigned int)received, cd.last_event.command_status,
			(unsigned int)event_name);
	}

	return -ETIMEDOUT;
}

static int cd_validate_last_cm_event(enum cm_event_to_host_t event_name)
{
	k_mutex_lock(&cd.lock, K_FOREVER);

	switch (event_name) {
	case CM2CD_WIFI_SW_CLIENT_STATUS_EVENT:
	case CM2CD_SR_SW_CLIENT_STATUS_EVENT:
		LOG_DBG("CM2CD SW client status event %u status=%u",
			(unsigned int)event_name, cd.last_event.command_status);
		k_mutex_unlock(&cd.lock);
		return 0;
	case CM2CD_STATISTICS_EVENT:
		if (!cd.stats_valid) {
			LOG_ERR("CM2CD statistics event without stats payload");
			k_mutex_unlock(&cd.lock);
			return -EIO;
		}

		if (cd.last_event.command_status != COMMAND_PROCESSING_SUCCESS) {
			LOG_ERR("CM2CD statistics command processing failed (%u)",
				cd.last_event.command_status);
			k_mutex_unlock(&cd.lock);
			return -EIO;
		}

		k_mutex_unlock(&cd.lock);
		return 0;
	default:
		if (cd.last_event.command_status != COMMAND_PROCESSING_SUCCESS) {
			LOG_ERR("CM2CD event %u command processing FAIL (status=%u)",
				(unsigned int)event_name, cd.last_event.command_status);
			k_mutex_unlock(&cd.lock);
			return -EIO;
		}

		LOG_DBG("CM2CD event %u command processing SUCCESS",
			(unsigned int)event_name);
		k_mutex_unlock(&cd.lock);
		return 0;
	}
}

static void coex_event_handler(void *ctx, const void *event, size_t len)
{
	const struct cm2cd_event_status_name_t *hdr;
	enum cm_event_to_host_t event_name;
	size_t stats_offset;

	ARG_UNUSED(ctx);

	if ((event == NULL) || (len < sizeof(*hdr))) {
		return;
	}

	hdr = event;
	event_name = (enum cm_event_to_host_t)hdr->event_name;
	stats_offset = sizeof(*hdr);

	k_mutex_lock(&cd.lock, K_FOREVER);
	cd.last_event = *hdr;

	switch (event_name) {
	case CM2CD_STATISTICS_EVENT:
		cd.stats_valid = false;
		cd.patch_stats_valid = false;

		if (len >= (stats_offset + sizeof(struct cm_stats_t))) {
			memcpy(&cd.last_stats, (const uint8_t *)event + stats_offset,
			       sizeof(cd.last_stats));
			cd.stats_valid = true;

			stats_offset += sizeof(cd.last_stats);
			if (len >= (stats_offset + sizeof(cd.last_patch_stats))) {
				memcpy(&cd.last_patch_stats,
				       (const uint8_t *)event + stats_offset,
				       sizeof(cd.last_patch_stats));
				cd.patch_stats_valid = true;
			}
		} else {
			LOG_WRN("Short statistics event (%zu bytes)", len);
		}
		break;
	case CM2CD_WIFI_SW_CLIENT_STATUS_EVENT:
	case CM2CD_SR_SW_CLIENT_STATUS_EVENT:
		LOG_DBG("CM2CD SW client event %u grant/status=%u", (unsigned int)event_name,
			hdr->command_status);
		break;
	default:
		if (hdr->command_status != COMMAND_PROCESSING_SUCCESS) {
			LOG_WRN("CM2CD event %u command processing FAIL (status=%u)",
				(unsigned int)event_name, hdr->command_status);
		} else {
			LOG_DBG("CM2CD event %u command processing SUCCESS",
				(unsigned int)event_name);
		}
		break;
	}

	k_mutex_unlock(&cd.lock);
	k_sem_give(&cd.cm_event_sem);
}

int coex_cd_cm_send_and_wait(const void *cmd, size_t len, enum cm_event_to_host_t expected_event)
{
	int ret;

	if ((cmd == NULL) || (len == 0U)) {
		return -EINVAL;
	}

	k_mutex_lock(&cd.cmd_lock, K_FOREVER);

	ret = coex_cm_send(cmd, len);
	if (ret != 0) {
		k_mutex_unlock(&cd.cmd_lock);
		return ret;
	}

	ret = cd_wait_for_cm_event(expected_event, K_MSEC(CD_CM_EVENT_WAIT_MS));
	if (ret != 0) {
		LOG_ERR("Timeout waiting for CM2CD event %u", (unsigned int)expected_event);
		k_mutex_unlock(&cd.cmd_lock);
		return ret;
	}

	ret = cd_validate_last_cm_event(expected_event);
	k_mutex_unlock(&cd.cmd_lock);
	return ret;
}

static int cd_ensure_coexc_configured(void)
{
	uint32_t ccmallow;
	int ret;

	k_mutex_lock(&cd.lock, K_FOREVER);
	if (cd.coexc_configured) {
		k_mutex_unlock(&cd.lock);
		return 0;
	}
	k_mutex_unlock(&cd.lock);

	if (!nrf71_wifi_coex_is_ready()) {
		return -EACCES;
	}

	ret = cd_coexc_verify_configured(COEX_SHARED_ANT_CFG, &ccmallow);
	if (ret == 0) {
		k_mutex_lock(&cd.lock, K_FOREVER);
		cd.coexc_configured = true;
		k_mutex_unlock(&cd.lock);
		return 0;
	}

	ret = cd_coexc_configure_and_enable(COEX_SHARED_ANT_CFG);
	if (ret != 0) {
		LOG_ERR("COEXC configure/enable failed (%d)", ret);
		return ret;
	}

	k_mutex_lock(&cd.lock, K_FOREVER);
	cd.coexc_configured = true;
	k_mutex_unlock(&cd.lock);

	return 0;
}

static int cd_apply_cm_config(void)
{
	int ret;

	ret = cd_ensure_coexc_configured();
	if (ret != 0) {
		return ret;
	}

	ret = coex_cm_set_priority_ranges(&cd.wifi_range, &cd.sr_range);
	if (ret) {
		return ret;
	}

	if (coex_sr_set_client_priority(&cd.sr_range) == 0U) {
		LOG_WRN("SR driver rejected priority ranges");
	}

	ret = coex_cm_update_user_params(&cd.user_params);
	if (ret) {
		return ret;
	}

	ret = coex_cm_update_coex_params();
	if (ret) {
		return ret;
	}

	return coex_cm_enable(true);
}

/** Post CD2CM_ENABLE_COEXISTENCE and wait for CM2CD_ENABLE_COEXISTENCE_EVENT. */
int coex_cd_enable(bool enable)
{
	return coex_cm_enable(enable);
}

/** Post CD2CM_SET_PRIORITY_RANGES and wait for CM2CD_SET_PRIORITY_RANGES_EVENT. */
int coex_cd_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range)
{
	return coex_cm_set_priority_ranges(wifi_range, sr_range);
}

/** Post CD2CM_UPDATE_COEX_USER_PARAMS and wait for CM2CD_UPDATE_COEX_USER_PARAMS_EVENT. */
int coex_cd_update_user_params(const struct coex_user_params_t *user_params)
{
	return coex_cm_update_user_params(user_params);
}

/** Post CD2CM_UPDATE_COEX_PARAMS and wait for CM2CD_UPDATE_COEX_PARAMS_EVENT. */
int coex_cd_update_coex_params(void)
{
	return coex_cm_update_coex_params();
}

/** Post CD2CM_UPDATE_COEX_PARAMS and wait for CM2CD_UPDATE_COEX_PARAMS_EVENT. */
int coex_cd_update_coex_params_blob(const uint8_t *blob, size_t blob_len)
{
	return coex_cm_update_coex_params_blob(blob, blob_len);
}

/** Post CD2CM_ALLOCATE_PPW and wait for CM2CD_ALLOCATE_PPW_EVENT. */
int coex_cd_allocate_ppw(const struct coex_ppw_parameters_t *ppw_params)
{
	return coex_cm_allocate_ppw(ppw_params);
}

/** Post CD2CM_WIFI_SW_CLIENT_REQUEST and wait for CM2CD_WIFI_SW_CLIENT_STATUS_EVENT. */
int coex_cd_wifi_sw_client_request(const struct coex_sw_client_params_t *params)
{
	return coex_cm_wifi_sw_client_request(params);
}

/** Post CD2CM_GET_STATS and wait for CM2CD_STATISTICS_EVENT. */
int coex_cd_get_stats(void)
{
	return coex_cm_get_stats();
}

/** Return cm_stats_t retained from the last CM2CD_STATISTICS_EVENT, or NULL. */
const struct cm_stats_t *coex_cd_get_last_stats(void)
{
	const struct cm_stats_t *stats;

	k_mutex_lock(&cd.lock, K_FOREVER);
	stats = cd.stats_valid ? &cd.last_stats : NULL;
	k_mutex_unlock(&cd.lock);

	return stats;
}

/** Return patch stats retained from the last CM2CD_STATISTICS_EVENT, or NULL. */
const struct cm_fsm_patch_stats_sr_t *coex_cd_get_last_patch_stats(void)
{
	const struct cm_fsm_patch_stats_sr_t *patch_stats;

	k_mutex_lock(&cd.lock, K_FOREVER);
	patch_stats = cd.patch_stats_valid ? &cd.last_patch_stats : NULL;
	k_mutex_unlock(&cd.lock);

	return patch_stats;
}

int coex_cd_coexc_configure_and_enable(enum coex_antenna_cfg_type antenna_cfg_type)
{
	return cd_coexc_configure_and_enable(antenna_cfg_type);
}

int coex_cd_coexc_verify_configured(enum coex_antenna_cfg_type antenna_cfg_type,
				    uint32_t *ccmallow_client0_mode0)
{
	return cd_coexc_verify_configured(antenna_cfg_type, ccmallow_client0_mode0);
}

/**
 * Post CD2CM_SR_SW_CLIENT_REQUEST and wait for CM2CD_SR_SW_CLIENT_STATUS_EVENT.
 *
 * Returns the grant/deny result in grant_status from the CM2CD event payload.
 */
int coex_cd_sr_software_client_request(const struct coex_sr_sw_client_params_t *client_params,
				       enum coex_sr_sw_client_req_status_t *grant_status)
{
	int ret;

	if ((client_params == NULL) || (grant_status == NULL)) {
		return -EINVAL;
	}

	k_mutex_lock(&cd.lock, K_FOREVER);

	if (!cd.sr_coex_enabled) {
		k_mutex_unlock(&cd.lock);
		return -EACCES;
	}

	k_mutex_unlock(&cd.lock);

	ret = coex_cm_sr_sw_client_request(client_params);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&cd.lock, K_FOREVER);
	*grant_status = (enum coex_sr_sw_client_req_status_t)cd.last_event.command_status;
	k_mutex_unlock(&cd.lock);

	return 0;
}

/**
 * Post CD2CM_ALLOCATE_PPW on behalf of the SR driver and wait for
 * CM2CD_ALLOCATE_PPW_EVENT after validating coex state.
 */
int coex_cd_update_short_range_activity_info(
	const struct short_range_activity_info_t *activity_info)
{
	struct coex_ppw_parameters_t ppw;

	if (activity_info == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&cd.lock, K_FOREVER);

	if (!cd.sr_coex_enabled) {
		k_mutex_unlock(&cd.lock);
		return -EACCES;
	}

	k_mutex_unlock(&cd.lock);

	if (activity_info->sr_activity_action == SR_ACTIVITY_START) {
		ppw = (struct coex_ppw_parameters_t){
			.start_or_stop_ppw = START_ALLOC_WINDOWS,
			.first_window_to_wifi_or_sr = SR_RADIO,
			.wifi_pti_window_duration = activity_info->activity_duration,
			.sr_pti_window_duration = activity_info->activity_interval,
			.ppws_timeout = activity_info->activity_timeout,
		};
	} else {
		ppw = (struct coex_ppw_parameters_t){
			.start_or_stop_ppw = STOP_ALLOC_WINDOWS,
		};
	}

	return coex_cm_allocate_ppw(&ppw);
}

int coex_cd_sr_power_notify(enum coex_sr_power_event_t event)
{
	switch (event) {
	case COEX_SR_PREPARE_POWER_DOWN:
		k_mutex_lock(&cd.lock, K_FOREVER);
		if (cd.transition_active) {
			k_mutex_unlock(&cd.lock);
			return -EBUSY;
		}
		cd.transition_active = true;
		cd.sr_up = false;
		cd.sr_coex_enabled = false;
		cd.transition_active = false;
		k_mutex_unlock(&cd.lock);

		(void)coex_sr_enable(0U);
		return 0;

	case COEX_SR_POWERED_UP_READY:
		k_mutex_lock(&cd.lock, K_FOREVER);
		if (cd.transition_active) {
			k_mutex_unlock(&cd.lock);
			return -EBUSY;
		}
		cd.transition_active = true;
		k_mutex_unlock(&cd.lock);

		if (coex_sr_set_client_priority(&cd.sr_range) == 0U) {
			LOG_WRN("SR driver rejected priority ranges on power-up");
		}
		(void)coex_sr_enable(1U);

		k_mutex_lock(&cd.lock, K_FOREVER);
		cd.sr_up = true;
		cd.sr_coex_enabled = cd.wifi_up && nrf71_wifi_coex_is_ready();
		cd.transition_active = false;
		k_mutex_unlock(&cd.lock);
		return 0;

	default:
		return -EINVAL;
	}
}

int coex_cd_wifi_power_notify(enum coex_wifi_power_event_t event)
{
	int ret;

	switch (event) {
	case COEX_WIFI_PREPARE_POWER_DOWN:
		k_mutex_lock(&cd.lock, K_FOREVER);
		if (cd.transition_active) {
			k_mutex_unlock(&cd.lock);
			return -EBUSY;
		}
		cd.transition_active = true;
		cd.wifi_up = false;
		cd.sr_coex_enabled = false;
		cd.transition_active = false;
		k_mutex_unlock(&cd.lock);
		return 0;

	case COEX_WIFI_POWERED_UP_READY:
		k_mutex_lock(&cd.lock, K_FOREVER);
		if (cd.transition_active) {
			k_mutex_unlock(&cd.lock);
			return -EBUSY;
		}
		cd.transition_active = true;
		k_mutex_unlock(&cd.lock);

		ret = cd_apply_cm_config();

		k_mutex_lock(&cd.lock, K_FOREVER);
		cd.wifi_up = (ret == 0);
		cd.sr_coex_enabled = cd.wifi_up && cd.sr_up;
		cd.transition_active = false;
		k_mutex_unlock(&cd.lock);

		return (ret == 0) ? 0 : -EIO;

	default:
		return -EINVAL;
	}
}

static int nrf71_sr_coex_init(void)
{
	int ret;

	k_mutex_init(&cd.lock);
	k_mutex_init(&cd.cmd_lock);
	k_sem_init(&cd.cm_event_sem, 0, K_SEM_MAX_LIMIT);

	cd.wifi_range = default_wifi_range;
	cd.sr_range = default_sr_range;
	cd.user_params = default_user_params;

	(void)nrf71_wifi_coex_register_event_cb(coex_event_handler, NULL);

	cd.wifi_up = nrf71_wifi_coex_is_ready();
	cd.sr_up = false;
	cd.sr_coex_enabled = false;

	if (cd.wifi_up) {
		ret = cd_apply_cm_config();
		if (ret) {
			LOG_WRN("Deferred coex config (%d); will retry on Wi-Fi power-up", ret);
			cd.wifi_up = false;
		} else {
			LOG_INF("nRF71 SR coexistence configured");
		}
	} else {
		LOG_DBG("Wi-Fi transport not ready; coex config deferred");
	}

	return 0;
}

SYS_INIT(nrf71_sr_coex_init, APPLICATION, CONFIG_NRF71_SR_COEX_DRIVER_INIT_PRIORITY);
