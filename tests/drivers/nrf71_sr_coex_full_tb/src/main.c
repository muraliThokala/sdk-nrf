/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Host-side test bench for the nRF71 Wi-Fi and Short-Range (SR)
 *        Coexistence Driver (CD).
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "coex_tb.h"
#include "coex_tb_tests.h"
#include "coex_tb_util.h"

/**
 * Entry point of the coexistence driver test bench.
 *
 * Startup order:
 *   1. The coexistence driver initialises through SYS_INIT (COEXC tables, work
 *      queue, CM2CD event callback, Wi-Fi state notifier). It configures the CM
 *      by itself once the Wi-Fi interface is up.
 *   2. coex_tb_prepare() waits for a stable Wi-Fi FMAC transport, lets the
 *      RPU/VIF settle, and waits for the driver's CM configuration when a
 *      CM-dependent test is enabled (unless INIT_AND_ENABLE waits for it itself).
 *   3. Run the test steps. Every coex_cm_* call returns once the driver has
 *      finished, so this application only has to check return codes.
 *
 * @retval 0 Every enabled test step passed.
 * @retval 1 Preparation or a test step failed.
 */
int main(void)
{
	int ret;

	printk("nRF71 Wi-Fi and Short-Range (SR) coexistence driver test bench\n");

#ifdef COEX_TB_DUMP_WIFI_BRINGUP_STATE
	coex_tb_dump_wifi_bringup_state();
#endif

	ret = coex_tb_prepare();
	if (ret != 0) {
		printk("Coex TB: preparation failed (%s)\n", coex_tb_errstr(ret));
		return 1;
	}

	ret = coex_tb_run_all();

	return (ret == 0) ? 0 : 1;
}
