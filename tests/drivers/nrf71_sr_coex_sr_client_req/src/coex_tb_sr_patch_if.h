/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief SR SW client patch definitions for the host test bench.
 *
 * Matches @c cm_fsm_patch_stats.h from the ROM 1.0 SR SW client firmware patch.
 */

#ifndef COEX_TB_SR_PATCH_IF_H__
#define COEX_TB_SR_PATCH_IF_H__

#include <nrf71_coex_if.h>

/**
 * Patch statistics trailer with SR SW client counters.
 *
 * Appended after @ref cm_stats_t in @c CM2CD_STATISTICS_EVENT payloads when the
 * SR SW client patch is active.
 */
struct cm_fsm_patch_stats_sr_t {
	/** CD2CM command entry counts */
	unsigned int cmd_update_coex_params_cnt_patch;
	unsigned int cmd_update_user_params_cnt_patch;
	unsigned int cmd_enable_coex_cnt_patch;
	unsigned int cmd_allocate_ppw_cnt_patch;
	unsigned int cmd_set_pti_ranges_cnt_patch;
	unsigned int cmd_get_stats_cnt_patch;
	unsigned int cmd_wifi_sw_client_req_cnt_patch;
	unsigned int cmd_sr_sw_client_req_cnt_patch;
	unsigned int cm_coex_process_cmd_cnt_patch;
	/** CM2CD event counters */
	unsigned int wifi_sw_client_event_to_host_cnt;
	unsigned int sr_sw_client_event_to_host_cnt;
	unsigned int coex_params_event_to_host_cnt;
	unsigned int user_params_event_to_host_cnt;
	unsigned int enable_coex_event_to_host_cnt;
	unsigned int allocate_ppw_event_to_host_cnt;
	unsigned int set_pti_ranges_event_to_host_cnt;
} __NRF_WIFI_PKD;

#endif /* COEX_TB_SR_PATCH_IF_H__ */
