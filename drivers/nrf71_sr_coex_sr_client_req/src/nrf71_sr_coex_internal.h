/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal interface between the coexistence driver core and its CM layer.
 */

#ifndef NRF71_SR_COEX_INTERNAL_H__
#define NRF71_SR_COEX_INTERNAL_H__

#include <stddef.h>
#include <stdbool.h>

#include <nrf71_coex_if.h>
#include <nrf71_coex_hw_regs.h>

#include "nrf71_sr_coex_sr_patch_if.h"

/** Post a CD2CM command over the transport only (no CM2CD wait). */
int coex_cm_send(const void *cmd, size_t len);

/** Post a CD2CM command and block until the expected CM2CD event arrives. */
int coex_cd_cm_send_and_wait(const void *cmd, size_t len, enum cm_event_to_host_t expected_event);

/** Post CD2CM_ENABLE_COEXISTENCE and wait for CM2CD_ENABLE_COEXISTENCE_EVENT. */
int coex_cm_enable(bool enable);

/** Post CD2CM_SET_PRIORITY_RANGES and wait for CM2CD_SET_PRIORITY_RANGES_EVENT. */
int coex_cm_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range);

/** Post CD2CM_UPDATE_COEX_USER_PARAMS and wait for CM2CD_UPDATE_COEX_USER_PARAMS_EVENT. */
int coex_cm_update_user_params(const struct coex_user_params_t *user_params);

/** Post CD2CM_UPDATE_COEX_PARAMS and wait for CM2CD_UPDATE_COEX_PARAMS_EVENT. */
int coex_cm_update_coex_params(void);

/** Post CD2CM_GET_STATS and wait for CM2CD_STATISTICS_EVENT. */
int coex_cm_get_stats(void);

/** Post CD2CM_ALLOCATE_PPW and wait for CM2CD_ALLOCATE_PPW_EVENT. */
int coex_cm_allocate_ppw(const struct coex_ppw_parameters_t *ppw_params);

/** Post CD2CM_WIFI_SW_CLIENT_REQUEST and wait for CM2CD_WIFI_SW_CLIENT_STATUS_EVENT. */
int coex_cm_wifi_sw_client_request(const struct coex_sw_client_params_t *params);

/** Post CD2CM_SR_SW_CLIENT_REQUEST and wait for CM2CD_SR_SW_CLIENT_STATUS_EVENT. */
int coex_cm_sr_sw_client_request(const struct coex_sr_sw_client_params_t *params);

/** Post CD2CM_UPDATE_COEX_PARAMS and wait for CM2CD_UPDATE_COEX_PARAMS_EVENT. */
int coex_cm_update_coex_params_blob(const uint8_t *blob, size_t blob_len);

int cd_coexc_configure(enum coex_antenna_cfg_type antenna_cfg_type);

int cd_coexc_enable(enum coexc_hw_enable enable);

int cd_coexc_configure_and_enable(enum coex_antenna_cfg_type antenna_cfg_type);

int cd_coexc_verify_configured(enum coex_antenna_cfg_type antenna_cfg_type,
			       uint32_t *ccmallow_client0_mode0);

#endif /* NRF71_SR_COEX_INTERNAL_H__ */
