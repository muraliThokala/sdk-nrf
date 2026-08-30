/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief CM statistics printing and consistency checking.
 *
 * The counters belong to the CM, but they are used here as evidence about the
 * driver: how many commands of each type the driver actually emitted, and how
 * the CM reacted to them. That makes them the closest thing available to an
 * external observation of the driver's outgoing traffic.
 *
 * The CM keeps a large set of counters: how many requests it saw, how
 * many it granted, how many commands of each type arrived, and so on. Reading
 * them back through coex_cd_get_stats() confirms that the commands the driver
 * sent on the bench's behalf arrived and were understood.
 *
 * Two payloads can arrive with a statistics event:
 *   cm_stats_t            - always present, the ROM 1.0 counters
 *   cm_fsm_patch_stats_t  - only with patched CM firmware, adds per-event counters
 */

#ifndef COEX_TB_STATS_H__
#define COEX_TB_STATS_H__

#include <nrf71_coex_if.h>
#include <nrf71_coex_patch_if.h>

/**
 * Print the CM counters and run consistency checks over them.
 *
 * @param stats       ROM counters. NULL means no statistics were retained, which
 *                    is itself reported as one failure.
 * @param patch_stats Patch counters, or NULL if the firmware did not send them.
 *                    Absence is reported but is not counted as a failure.
 *
 * @return Number of failed checks. Zero means everything was consistent.
 */
unsigned int coex_tb_print_and_validate_stats(const struct cm_stats_t *stats,
					      const struct cm_fsm_patch_stats_t *patch_stats);

#endif /* COEX_TB_STATS_H__ */
