/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence test bench utilities (transport wait and timing helpers).
 */

#ifndef COEX_TB_UTIL_H__
#define COEX_TB_UTIL_H__

#include <stdint.h>

int coex_tb_wait_transport_ready(uint32_t timeout_ms);

void coex_tb_wait_ms(uint32_t wait_ms);

#endif /* COEX_TB_UTIL_H__ */
