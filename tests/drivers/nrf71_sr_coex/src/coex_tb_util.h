/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence test bench utilities (transport wait and timing helpers).
 *
 * CM2CD event waiting is handled inside the coexistence driver; the test bench
 * calls coex_cd_* APIs and validates retained statistics where applicable.
 */

#ifndef COEX_TB_UTIL_H__
#define COEX_TB_UTIL_H__

#include <stdint.h>

/** Wait until the Wi-Fi FMAC coexistence transport is ready. */
int coex_tb_wait_transport_ready(uint32_t timeout_ms);

/** Sleep helper used between test steps. */
void coex_tb_wait_ms(uint32_t wait_ms);

#endif /* COEX_TB_UTIL_H__ */
