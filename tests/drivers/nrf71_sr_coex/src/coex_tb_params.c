/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Parameter builders for the coexistence driver APIs under test.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>

#include "coex_tb.h"
#include "coex_tb_params.h"

void coex_tb_params_priority_ranges(struct coex_wifi_priority_range_t *wifi_range,
				    struct coex_sr_priority_range_t *sr_range)
{
	if (wifi_range != NULL) {
		/*
		 * Wi-Fi has four hardware clients plus a software-request range.
		 * The two "high priority" clients sit at low PTI numbers (15-25)
		 * so they beat SR; the two "low priority" clients sit far above
		 * the SR ranges (120-160) so SR beats them.
		 */
		*wifi_range = (struct coex_wifi_priority_range_t){
			.sw_request_priority_range = {10, 5, 1},
			.client0_ccconf_pti_range = {20, 15, 1},   /* high-prio Wi-Fi Rx */
			.client1_ccconf_pti_range = {25, 20, 1},   /* high-prio Wi-Fi Tx */
			.client2_ccconf_pti_range = {140, 120, 3}, /* low-prio Wi-Fi Rx */
			.client3_ccconf_pti_range = {160, 145, 3}, /* low-prio Wi-Fi Tx */
			.hw_client_priority_level = 5,
		};
	}

	if (sr_range != NULL) {
		/*
		 * SR has a normal and a "critical" range for both Rx and Tx.
		 * The critical ranges sit at low PTI numbes than Wi-Fi ranges so
		 * that time-critical SR traffic can win the antenna.
		 */
		*sr_range = (struct coex_sr_priority_range_t){
			.sr_rx_client_ccconf_pti_range = {30, 25, 1},
			.sr_tx_client_ccconf_pti_range = {40, 30, 2},
			.client_priority_level = 5,
			.sr_rx_client_critical_ccconf_pti_range = {5, 0, 1},
			.sr_tx_client_critical_ccconf_pti_range = {5, 0, 1},
		};
	}
}

void coex_tb_params_user_params(struct coex_user_params_t *user_params)
{
	/*
	 * Dynamic antenna allocation, matching the coexistence driver default.
	 */
	coex_tb_params_user_params_ant_alloc(user_params, ANT_ALLOC_DYNAMIC);
}

void coex_tb_params_user_params_ant_alloc(struct coex_user_params_t *user_params,
					  enum antenna_allocation_t ant_mode)
{
	if (user_params == NULL) {
		return;
	}

	/*
	 * All probabilities are percentages. 100 means "always protect this
	 * activity", which makes the CM's behaviour deterministic and therefore
	 * testable. message_id is part of the payload as well as the command
	 * header, so it is set here too.
	 */
	*user_params = (struct coex_user_params_t){
		.message_id = CD2CM_UPDATE_COEX_USER_PARAMS,
		.listen2inactive_sr_rx_prot_prob_ps = 100,
		.inactive2listen_sr_rx_prot_prob_ps = 100,
		.inactive2listen_sr_rx_prot_prob_calib = 100,
		.listen2inactive_sr_rx_prot_prob_calib = 100,
		.wifi_scan_puncture_info = {.wifi_scan_prot_prob = 100},
		.wifi_beacon_prot_prob = 100,
		.wifi_conn_prot_prob = 100,
		.wifi_calib_prot_prob = 100,
		.shared_ant_control = ant_mode,
	};
}

void coex_tb_params_user_params_wifi_prob(struct coex_user_params_t *user_params,
					  uint32_t prob_percent)
{
	if (user_params == NULL) {
		return;
	}

	coex_tb_params_user_params(user_params);

	/* Override only the Wi-Fi side */
	user_params->wifi_scan_puncture_info.wifi_scan_prot_prob = prob_percent;
	user_params->wifi_beacon_prot_prob = prob_percent;
	user_params->wifi_conn_prot_prob = prob_percent;
	user_params->wifi_calib_prot_prob = prob_percent;
}

int coex_tb_params_default_coex_blob(uint8_t *blob, size_t blob_size, size_t *out_len)
{
	size_t decoded;

	if ((blob == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	/*
	 * hex2bin() returns the number of bytes written, or 0 if the string is
	 * malformed or does not fit in blob_size. The return type is unsigned,
	 * so 0 is the only failure indication.
	 */
	decoded = hex2bin(NRF_COEX_PARAMS, strlen(NRF_COEX_PARAMS), blob, blob_size);
	if (decoded == 0U) {
		return -EINVAL;
	}

	*out_len = decoded;
	return 0;
}

int coex_tb_params_coex_blob_lna(uint8_t lna_control, uint8_t *blob, size_t blob_size,
				 size_t *out_len, uint8_t *out_original)
{
	int ret;

	ret = coex_tb_params_default_coex_blob(blob, blob_size, out_len);
	if (ret != 0) {
		return ret;
	}

	/* Offset must be inside the decoded blob, not just inside the buffer. */
	if (*out_len <= COEX_TB_BLOB_LNASW_OFFSET) {
		return -EINVAL;
	}

	if (out_original != NULL) {
		*out_original = blob[COEX_TB_BLOB_LNASW_OFFSET];
	}

	blob[COEX_TB_BLOB_LNASW_OFFSET] = lna_control;
	return 0;
}

void coex_tb_params_ppw_start(struct coex_ppw_parameters_t *ppw, bool first_window_wifi)
{
	if (ppw == NULL) {
		return;
	}

	/*
	 * Two different shapes so the CM is exercised both ways round: which
	 * radio gets the first window of each period, and asymmetric window
	 * lengths. Durations are milliseconds; ppws_timeout is how long the CM
	 * keeps generating windows before stopping on its own.
	 */
	if (first_window_wifi) {
		*ppw = (struct coex_ppw_parameters_t){
			.start_or_stop_ppw = START_ALLOC_WINDOWS,
			.first_window_to_wifi_or_sr = WIFI_RADIO,
			.wifi_pti_window_duration = 10,
			.sr_pti_window_duration = 20,
			.ppws_timeout = 30U * 1000U,
		};
	} else {
		*ppw = (struct coex_ppw_parameters_t){
			.start_or_stop_ppw = START_ALLOC_WINDOWS,
			.first_window_to_wifi_or_sr = SR_RADIO,
			.wifi_pti_window_duration = 30,
			.sr_pti_window_duration = 15,
			.ppws_timeout = 50U * 1000U,
		};
	}
}

void coex_tb_params_ppw_stop(struct coex_ppw_parameters_t *ppw)
{
	if (ppw == NULL) {
		return;
	}

	ppw->start_or_stop_ppw = STOP_ALLOC_WINDOWS;
}

void coex_tb_params_wifi_sw_client(struct coex_sw_client_params_t *params,
				   enum coex_wifi_sw_client_req_type_t req_type,
				   uint32_t timeout_ms,
				   unsigned int wifi_operating_band)
{
	if (params == NULL) {
		return;
	}

	*params = (struct coex_sw_client_params_t){
		.sw_client_request = req_type,
		.sw_client_pti_level = WIFI_SW_CLIENT_REQ_PTI_LOW,
		.sw_client_type = WIFI_CONNECTION,
		.request_timeout_in_ms = timeout_ms,
		.wifi_operating_band = wifi_operating_band,
	};
}

void coex_tb_params_sr_activity(struct short_range_activity_info_t *activity_info,
				unsigned int action)
{
	if (activity_info == NULL) {
		return;
	}

	*activity_info = (struct short_range_activity_info_t){
		.sr_activity_type = SR_BLE_CONNECTED_DATA_TRANSFER,
		.sr_activity_action = action,
		.start_time_of_activity = 0U,
		.activity_interval = COEX_TB_SR_ACTIVITY_INTERVAL_MS,
		.activity_duration = COEX_TB_SR_ACTIVITY_DURATION_MS,
		.activity_timeout = COEX_TB_SR_ACTIVITY_TIMEOUT_MS,
	};
}
