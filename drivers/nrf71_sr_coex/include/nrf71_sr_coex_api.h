/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Public coexistence driver APIs for CM configuration, runtime commands,
 *        and retained CM statistics.
 *
 * Each coex_cd_* API posts a CD2CM command and blocks until the matching CM2CD
 * completion event is received before returning to the caller.
 *
 * CD2CM command                  CM2CD completion event
 * -------------------------  ---------------------------------
 * CD2CM_ENABLE_COEXISTENCE       CM2CD_ENABLE_COEXISTENCE_EVENT
 * CD2CM_SET_PRIORITY_RANGES      CM2CD_SET_PRIORITY_RANGES_EVENT
 * CD2CM_UPDATE_COEX_USER_PARAMS  CM2CD_UPDATE_COEX_USER_PARAMS_EVENT
 * CD2CM_UPDATE_COEX_PARAMS       CM2CD_UPDATE_COEX_PARAMS_EVENT
 * CD2CM_ALLOCATE_PPW             CM2CD_ALLOCATE_PPW_EVENT
 * CD2CM_WIFI_SW_CLIENT_REQUEST   CM2CD_WIFI_SW_CLIENT_STATUS_EVENT
 * CD2CM_GET_STATS                CM2CD_STATISTICS_EVENT
 */

#ifndef NRF71_SR_COEX_API_H__
#define NRF71_SR_COEX_API_H__

#include <stddef.h>
#include <stdbool.h>

#include <nrf71_coex_hw_regs.h>

/** Post CD2CM_ENABLE_COEXISTENCE and wait for CM2CD_ENABLE_COEXISTENCE_EVENT. */
int coex_cd_enable(bool enable);

/** Post CD2CM_SET_PRIORITY_RANGES and wait for CM2CD_SET_PRIORITY_RANGES_EVENT. */
int coex_cd_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range);

/** Post CD2CM_UPDATE_COEX_USER_PARAMS and wait for CM2CD_UPDATE_COEX_USER_PARAMS_EVENT. */
int coex_cd_update_user_params(const struct coex_user_params_t *user_params);

/** Post CD2CM_UPDATE_COEX_PARAMS (default NRF_COEX_PARAMS blob) and 
 *  wait for CM2CD_UPDATE_COEX_PARAMS_EVENT. 
 */
int coex_cd_update_coex_params(void);

/** Post CD2CM_UPDATE_COEX_PARAMS with a caller-supplied blob and 
 *  wait for CM2CD_UPDATE_COEX_PARAMS_EVENT. 
 */
int coex_cd_update_coex_params_blob(const uint8_t *blob, size_t blob_len);

/** Post CD2CM_ALLOCATE_PPW and wait for CM2CD_ALLOCATE_PPW_EVENT. */
int coex_cd_allocate_ppw(const struct coex_ppw_parameters_t *ppw_params);

/** Post CD2CM_WIFI_SW_CLIENT_REQUEST and wait for CM2CD_WIFI_SW_CLIENT_STATUS_EVENT. */
int coex_cd_wifi_sw_client_request(const struct coex_sw_client_params_t *params);

/**
 * Post CD2CM_GET_STATS, wait for CM2CD_STATISTICS_EVENT, and retain the
 * latest statistics snapshot inside the driver.
 */
int coex_cd_get_stats(void);

/** Return the latest retained cm_stats_t snapshot, or NULL if unavailable. */
const struct cm_stats_t *coex_cd_get_last_stats(void);

/** Return patch stats retained from the last CM2CD_STATISTICS_EVENT, or NULL. */
const struct cm_fsm_patch_stats_t *coex_cd_get_last_patch_stats(void);

/** Configure and enable COEXC hardware (host CCMALLOW / CCCONF programming). */
int coex_cd_configure_COEXC(enum coex_antenna_cfg_type antenna_cfg_type);

/** Read back COEXC CCMALLOW and enable state for verification. */
int coex_cd_coexc_verify_configured(enum coex_antenna_cfg_type antenna_cfg_type,
				    uint32_t *ccmallow_client0_mode0);

#endif /* NRF71_SR_COEX_API_H__ */
