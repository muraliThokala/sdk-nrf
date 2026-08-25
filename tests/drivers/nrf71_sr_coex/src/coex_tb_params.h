/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Parameter initializers for each CD2CM command.
 */

#ifndef COEX_TB_PARAMS_H__
#define COEX_TB_PARAMS_H__

#include <stddef.h>
#include <stdint.h>

#include <nrf71_coex_if.h>
#include <nrf71_cd_sr_if.h>

/** Fill Wi-Fi and SR priority ranges. */
void coex_tb_params_priority_ranges(struct coex_wifi_priority_range_t *wifi_range,
				    struct coex_sr_priority_range_t *sr_range);

/** Fill default user coexistence parameters. */
void coex_tb_params_user_params(struct coex_user_params_t *user_params);

/** Fill user params with a specific shared-antenna allocation mode. */
void coex_tb_params_user_params_ant_alloc(struct coex_user_params_t *user_params,
					  enum antenna_allocation_t ant_mode);

/** Fill user params with a Wi-Fi client protection probability (0-100). */
void coex_tb_params_user_params_wifi_prob(struct coex_user_params_t *user_params,
					  uint32_t prob_percent);

/** Decode @c NRF_COEX_PARAMS into a binary blob for CD2CM_UPDATE_COEX_PARAMS. */
int coex_tb_params_default_coex_blob(uint8_t *blob, size_t blob_size, size_t *out_len);

/** Build a coex-params blob with shared LNA control byte patched (test index 8). */
int coex_tb_params_coex_blob_lna(uint8_t lna_control, uint8_t *blob, size_t blob_size,
				 size_t *out_len);

/** Fill enable/disable coexistence command parameters. */
void coex_tb_params_enable(bool enable, enum coex_en_or_dis_t *coex_en_or_dis);

/** Fill PPW start parameters (first window Wi-Fi or SR). */
void coex_tb_params_ppw_start(struct coex_ppw_parameters_t *ppw, bool first_window_wifi);

/** Fill PPW stop parameters. */
void coex_tb_params_ppw_stop(struct coex_ppw_parameters_t *ppw);

/** Fill Wi-Fi SW client request/release parameters. */
void coex_tb_params_wifi_sw_client(struct coex_sw_client_params_t *params,
				   enum coex_wifi_sw_client_req_type_t req_type,
				   uint32_t timeout_ms,
				   unsigned int wifi_operating_band);

#endif /* COEX_TB_PARAMS_H__ */
