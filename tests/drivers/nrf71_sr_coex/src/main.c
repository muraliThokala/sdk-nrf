/* main.c - Host-side test bench for the nRF71 SR coexistence driver */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "coex_tb.h"
#include "coex_tb_tests.h"
#include "coex_tb_util.h"

/**
 * Entry point of the coexistence driver test bench.
 *
 * Startup order:
 *   1. The coexistence driver has already initialised itself through SYS_INIT
 *      by the time main() runs. If the Wi-Fi transport happened to be up that
 *      early, the driver has also configured the CM already.
 *   2. Wait here until the Wi-Fi FMAC transport is up, because the driver
 *      reaches the RPU over the Wi-Fi driver's control path.
 *   3. Run the test steps. Every coex_cd_* call returns once the driver has
 *      finished, so this application only has to check return codes.
 *
 * @retval 0 Every enabled test step passed.
 * @retval 1 The transport never came up, or a test step failed.
 */
int main(void)
{
	int ret;

	printk("nRF71 SR coexistence driver test bench\n");

	ret = coex_tb_wait_transport_ready(COEX_TB_TRANSPORT_TIMEOUT_MS);
	if (ret != 0) {
		printk("FMAC transport not ready; aborting tests\n");
		return 1;
	}

	/* Propagate the verdict, so an automated runner can see a failure. */
	ret = coex_tb_run_all();

	return (ret == 0) ? 0 : 1;
}
