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
		/* Wi-Fi has four hardware clients plus a software-request range. */
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
		 * The critical ranges sit at lower PTI numbers than the Wi-Fi ranges
		 * so that time-critical SR traffic can win the antenna.
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
	/* Dynamic antenna allocation, matching the coexistence driver default. */
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
	 * activity". message_id is part of the payload as well as the command
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

void coex_tb_params_prio(struct nrf71_sr_coex_prio *prio)
{
	if (prio == NULL) {
		return;
	}

	*prio = (struct nrf71_sr_coex_prio){
		.wifi = {
			.sw_request = {10, 5, 1},
			.hp_rx = {20, 15, 1},
			.hp_tx = {25, 20, 1},
			.lp_rx = {140, 120, 3},
			.lp_tx = {160, 145, 3},
			.level = 5,
		},
		.sr = {
			.rx = {30, 25, 1},
			.tx = {40, 30, 2},
			.rx_critical = {5, 0, 1},
			.tx_critical = {5, 0, 1},
			.level = 5,
		},
	};
}

void coex_tb_params_user(struct nrf71_sr_coex_user_params *user)
{
	if (user == NULL) {
		return;
	}

	*user = (struct nrf71_sr_coex_user_params){
		.sr_rx_prot_ps_listen2inactive = 100,
		.sr_rx_prot_ps_inactive2listen = 100,
		.sr_rx_prot_calib_listen2inactive = 100,
		.sr_rx_prot_calib_inactive2listen = 100,
		.wifi_scan_prot = 100,
		.wifi_beacon_prot = 100,
		.wifi_conn_prot = 100,
		.wifi_calib_prot = 100,
		.ant_alloc = NRF71_SR_COEX_ANT_ALLOC_DYNAMIC,
	};
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
