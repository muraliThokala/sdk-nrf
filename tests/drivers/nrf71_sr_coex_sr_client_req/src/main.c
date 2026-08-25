/* main.c - Host-side Coexistence Manager test bench (SR SW client variant) */

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

int main(void)
{
	int ret;

	printk("nRF71 SR coexistence driver CM test bench (SR SW client variant)\n");

	ret = coex_tb_wait_transport_ready(COEX_TB_TRANSPORT_TIMEOUT_MS);
	if (ret != 0) {
		printk("FMAC transport not ready; aborting tests\n");
		return 1;
	}

	coex_tb_run_all();

	return 0;
}
