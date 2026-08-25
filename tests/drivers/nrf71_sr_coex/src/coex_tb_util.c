/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>

#include "coex_tb.h"
#include "coex_tb_util.h"

/** Poll until the Wi-Fi FMAC coexistence transport reports ready. */
int coex_tb_wait_transport_ready(uint32_t timeout_ms)
{
	uint32_t elapsed_ms = 0U;

	while (!nrf71_wifi_coex_is_ready()) {
		if (elapsed_ms >= timeout_ms) {
			printk("Coex TB: transport not ready after %u ms\n", timeout_ms);
			return -ETIMEDOUT;
		}

		k_msleep(100);
		elapsed_ms += 100U;
	}

	return 0;
}

/** Sleep for the requested interval between test steps. */
void coex_tb_wait_ms(uint32_t wait_ms)
{
	k_msleep(wait_ms);
}
