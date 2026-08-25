/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief CM statistics validation (ROM 1.0 + SR SW client patch stats).
 */

#ifndef COEX_TB_STATS_H__
#define COEX_TB_STATS_H__

#include <nrf71_coex_if.h>

#include "coex_tb_sr_patch_if.h"

/** Print and run consistency checks on ROM CM statistics and SR patch stats. */
void coex_tb_print_and_validate_stats(const struct cm_stats_t *stats,
				      const struct cm_fsm_patch_stats_sr_t *patch_stats);

#endif /* COEX_TB_STATS_H__ */
