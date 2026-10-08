/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence driver test steps and the top-level runner.
 */

#ifndef COEX_TB_TESTS_H__
#define COEX_TB_TESTS_H__

/**
 * Run every enabled test step in order, stopping at the first failure.
 *
 * Which steps are enabled is controlled by the COEX_TB_TEST_* macros in
 * coex_tb.h. Steps are ordered deliberately: COEXC programming and argument
 * validation come first, then radio bring-up through the Wi-Fi and SR power
 * notifications, then the runtime commands, then the statistics read-back that
 * confirms which commands the driver actually emitted.
 *
 * @retval 0  All enabled steps passed.
 * @retval <0 The negative errno reported by the first failing step.
 */
int coex_tb_run_all(void);

#endif /* COEX_TB_TESTS_H__ */
