/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence Manager (CM) command construction and transport (SR variant).
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
#include <nrf71_coex_if.h>
#include <nrf71_cd_sr_if.h>

#include "nrf71_sr_coex_internal.h"

LOG_MODULE_DECLARE(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

/** Post a marshalled CD2CM command to the CM over the Wi-Fi FMAC path. */
int coex_cm_send(const void *cmd, size_t len)
{
	int ret = nrf71_wifi_coex_cmd_send(cmd, len);

	if (ret == -ENODEV) {
		LOG_DBG("CD2CM command not sent: transport not ready");
		return -EACCES;
	}

	return ret;
}

/** Post CD2CM_ENABLE_COEXISTENCE and wait for CM2CD_ENABLE_COEXISTENCE_EVENT. */
int coex_cm_enable(bool enable)
{
	struct cd2cm_enable_coexistence_t cmd = {
		.message_id = CD2CM_ENABLE_COEXISTENCE,
		.coex_en_or_dis = enable ? COEX_ENABLE : COEX_DISABLE,
	};

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_ENABLE_COEXISTENCE_EVENT);
}

/** Post CD2CM_SET_PRIORITY_RANGES and wait for CM2CD_SET_PRIORITY_RANGES_EVENT. */
int coex_cm_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range)
{
	struct cd2cm_set_priority_ranges_t cmd;

	if ((wifi_range == NULL) || (sr_range == NULL)) {
		return -EINVAL;
	}

	cmd.message_id = CD2CM_SET_PRIORITY_RANGES;
	cmd.wifi_pti_range = *wifi_range;
	cmd.sr_pti_range = *sr_range;

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_SET_PRIORITY_RANGES_EVENT);
}

/** Post CD2CM_UPDATE_COEX_USER_PARAMS and wait for CM2CD_UPDATE_COEX_USER_PARAMS_EVENT. */
int coex_cm_update_user_params(const struct coex_user_params_t *user_params)
{
	struct cd2cm_coex_user_params_t cmd;

	if (user_params == NULL) {
		return -EINVAL;
	}

	cmd.message_id = CD2CM_UPDATE_COEX_USER_PARAMS;
	cmd.user_params = *user_params;
	cmd.user_params.message_id = CD2CM_UPDATE_COEX_USER_PARAMS;

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_UPDATE_COEX_USER_PARAMS_EVENT);
}

/**
 * Post CD2CM_UPDATE_COEX_PARAMS and wait for CM2CD_UPDATE_COEX_PARAMS_EVENT.
 *
 * Sends the fixed NRF_COEX_PARAMS default blob. For caller-supplied blobs use
 * coex_cm_update_coex_params_blob() instead.
 */
int coex_cm_update_coex_params(void)
{
	uint8_t cmd[sizeof(uint32_t) + (sizeof(NRF_COEX_PARAMS) / 2U)];
	size_t blob_len;

	sys_put_le32(CD2CM_UPDATE_COEX_PARAMS, cmd);

	blob_len = hex2bin(NRF_COEX_PARAMS, strlen(NRF_COEX_PARAMS), &cmd[sizeof(uint32_t)],
			   sizeof(cmd) - sizeof(uint32_t));
	if (blob_len == 0U) {
		LOG_ERR("Malformed NRF_COEX_PARAMS");
		return -EINVAL;
	}

	return coex_cd_cm_send_and_wait(cmd, sizeof(uint32_t) + blob_len,
					CM2CD_UPDATE_COEX_PARAMS_EVENT);
}

/** Post CD2CM_GET_STATS and wait for CM2CD_STATISTICS_EVENT. */
int coex_cm_get_stats(void)
{
	struct cd2cm_get_coex_stats_t cmd = {
		.message_id = CD2CM_GET_STATS,
	};

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_STATISTICS_EVENT);
}

/** Post CD2CM_ALLOCATE_PPW and wait for CM2CD_ALLOCATE_PPW_EVENT. */
int coex_cm_allocate_ppw(const struct coex_ppw_parameters_t *ppw_params)
{
	struct cd2cm_genarate_ppw_t cmd;

	if (ppw_params == NULL) {
		return -EINVAL;
	}

	cmd.message_id = CD2CM_ALLOCATE_PPW;
	cmd.ppw_parameters = *ppw_params;

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_ALLOCATE_PPW_EVENT);
}

/** Post CD2CM_WIFI_SW_CLIENT_REQUEST and wait for CM2CD_WIFI_SW_CLIENT_STATUS_EVENT. */
int coex_cm_wifi_sw_client_request(const struct coex_sw_client_params_t *params)
{
	struct cd2cm_wifi_sw_client_request_t cmd;

	if (params == NULL) {
		return -EINVAL;
	}

	cmd.message_id = CD2CM_WIFI_SW_CLIENT_REQUEST;
	cmd.sw_client_parameters = *params;

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_WIFI_SW_CLIENT_STATUS_EVENT);
}

/** Post CD2CM_SR_SW_CLIENT_REQUEST and wait for CM2CD_SR_SW_CLIENT_STATUS_EVENT. */
int coex_cm_sr_sw_client_request(const struct coex_sr_sw_client_params_t *params)
{
	struct cd2cm_sr_sw_client_request_t cmd;

	if (params == NULL) {
		return -EINVAL;
	}

	cmd.message_id = CD2CM_SR_SW_CLIENT_REQUEST;
	cmd.sr_sw_client_parameters = *params;

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_SR_SW_CLIENT_STATUS_EVENT);
}

/**
 * Post CD2CM_UPDATE_COEX_PARAMS and wait for CM2CD_UPDATE_COEX_PARAMS_EVENT.
 *
 * Same wire format as coex_cm_update_coex_params(), but the blob is supplied
 * by the caller instead of the fixed NRF_COEX_PARAMS default.
 */
int coex_cm_update_coex_params_blob(const uint8_t *blob, size_t blob_len)
{
	uint8_t cmd[sizeof(uint32_t) + 128U];
	size_t max_blob = sizeof(cmd) - sizeof(uint32_t);

	if ((blob == NULL) || (blob_len == 0U) || (blob_len > max_blob)) {
		return -EINVAL;
	}

	sys_put_le32(CD2CM_UPDATE_COEX_PARAMS, cmd);
	memcpy(&cmd[sizeof(uint32_t)], blob, blob_len);

	return coex_cd_cm_send_and_wait(cmd, sizeof(uint32_t) + blob_len,
					CM2CD_UPDATE_COEX_PARAMS_EVENT);
}
