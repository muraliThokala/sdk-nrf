/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Parameter builders for the Coexistence Driver (CD) APIs under test.
 */

#ifndef COEX_TB_PARAMS_H__
#define COEX_TB_PARAMS_H__

#include <stddef.h>
#include <stdint.h>

#include <nrf71_coex_if.h>
#include <nrf71_sr_coex_api.h>

/**
 * Fill in Wi-Fi and SR priority ranges.
 *
 * A "PTI range" is {start, end, step}. Lower numbers mean higher priority, so
 * start is the lowest-priority value of the range and end the highest. The CM
 * uses these to decide which radio wins when both want the antenna.
 */
void coex_tb_params_priority_ranges(struct coex_wifi_priority_range_t *wifi_range,
				    struct coex_sr_priority_range_t *sr_range);

/** Fill in the default user parameters. */
void coex_tb_params_user_params(struct coex_user_params_t *user_params);

/**
 * Same as coex_tb_params_user_params() but with an explicit shared-antenna mode.
 *
 * @param ant_mode ANT_ALLOC_DYNAMIC lets the CM arbitrate; ANT_ALLOC_STATIC_WIFI
 *                 and ANT_ALLOC_STATIC_SR pin the antenna to one radio.
 */
void coex_tb_params_user_params_ant_alloc(struct coex_user_params_t *user_params,
					  enum antenna_allocation_t ant_mode);

/**
 * Decode the built-in NRF_COEX_PARAMS hex string into a binary blob.
 *
 * NRF_COEX_PARAMS is a compile-time hex string of CM tuning values. The CM
 * wants the binary form, so it is decoded here.
 *
 * @param blob      Destination buffer.
 * @param blob_size Size of blob; must be at least half the hex string length.
 * @param out_len   Out: number of decoded bytes.
 *
 * @retval 0       Decoded successfully.
 * @retval -EINVAL NULL argument, or the hex string did not fit / was malformed.
 */
int coex_tb_params_default_coex_blob(uint8_t *blob, size_t blob_size, size_t *out_len);

/**
 * Build a coex-params blob with the shared-LNA-switch control byte replaced.
 *
 * Starts from the default blob and patches one byte at
 * COEX_TB_BLOB_LNASW_OFFSET.
 *
 * @param lna_control  0 = dynamic, 1 = statically enabled, 2 = statically disabled.
 * @param out_original Out: byte value before patching, for logging. May be NULL.
 *
 * @retval 0       Blob built.
 * @retval -EINVAL NULL argument, or the blob is too short to hold the offset.
 */
int coex_tb_params_coex_blob_lna(uint8_t lna_control, uint8_t *blob, size_t blob_size,
				 size_t *out_len, uint8_t *out_original);

/**
 * Fill in a connected-state Wi-Fi channel description, as the Wi-Fi driver would.
 *
 * @param band          Operating band. Passed as int so a deliberately invalid
 *                      value can be used to test rejection.
 * @param center_mhz    Channel centre frequency in MHz.
 * @param bandwidth_mhz Channel bandwidth in MHz.
 * @param guard_mhz     Guard band applied either side of the channel, in MHz.
 */
void coex_tb_params_wifi_channel(struct coex_wifi_channel_info_t *channel_info, int band,
				 unsigned short center_mhz, unsigned short bandwidth_mhz,
				 unsigned char guard_mhz);

#endif /* COEX_TB_PARAMS_H__ */
