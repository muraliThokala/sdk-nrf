/* main.c - build-only exercise of the nRF71 SR coexistence driver API */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <drivers/nrf71_sr_coex/nrf71_sr_coex.h>
#include <drivers/nrf71_sr_coex/nrf71_sr_coex_sr.h>

/*
 * This test builds the standalone nRF71 SR coexistence driver together with the
 * nRF71 Wi-Fi driver and exercises the driver API contract so the driver links
 * against the Wi-Fi FMAC coexistence transport. It also registers stub
 * Short-Range driver callbacks, as the real SR driver does at boot.
 */
static int test_sr_enable(bool enable)
{
	ARG_UNUSED(enable);
	return 0;
}

static int test_sr_set_priority(const struct nrf71_sr_coex_sr_prio *prio)
{
	ARG_UNUSED(prio);
	return 0;
}

static const struct nrf71_sr_coex_sr_driver_cb test_sr_driver_cb = {
	.enable = test_sr_enable,
	.set_priority = test_sr_set_priority,
};

int main(void)
{
	const struct nrf71_sr_coex_prio prio = {
		.wifi = {
			.sw_request = {10, 5, 1},
			.hp_rx = {20, 15, 1},
			.hp_tx = {25, 20, 1},
			.lp_rx = {140, 120, 3},
			.lp_tx = {160, 145, 3},
			.level = 5,
		},
		.sr = {
			.rx = {30, 25, 1},
			.tx = {40, 30, 2},
			.rx_critical = {5, 0, 1},
			.tx_critical = {5, 0, 1},
			.level = 5,
		},
	};
	const struct nrf71_sr_coex_user_params user = {
		.sr_rx_prot_ps_listen2inactive = 100,
		.sr_rx_prot_ps_inactive2listen = 100,
		.sr_rx_prot_calib_listen2inactive = 100,
		.sr_rx_prot_calib_inactive2listen = 100,
		.wifi_scan_prot = 100,
		.wifi_beacon_prot = 100,
		.wifi_conn_prot = 100,
		.wifi_calib_prot = 100,
		.ant_alloc = NRF71_SR_COEX_ANT_ALLOC_DYNAMIC,
	};
	const struct nrf71_sr_coex_cfg cfg = {
		.antenna = NRF71_SR_COEX_ANTENNA_SHARED,
		.prio = &prio,
		.user = &user,
	};
	int ret;

	ret = nrf71_sr_coex_sr_register(&test_sr_driver_cb);
	if (ret != 0) {
		printk("SR driver callback registration failed: %d\n", ret);
	}

	(void)nrf71_sr_coex_sr_power_notify(NRF71_SR_COEX_SR_POWERED_UP_READY);
	(void)nrf71_sr_coex_sr_power_notify(NRF71_SR_COEX_SR_PREPARE_POWER_DOWN);

	/*
	 * The priority ranges and user params are stored whether Wi-Fi is up or
	 * not, and applied to the CM every time Wi-Fi comes up, so this succeeds.
	 */
	ret = nrf71_sr_coex_init(&cfg);
	printk("nrf71_sr_coex_init(): %d (expected 0)\n", ret);

	/* The configuration can be set only once. */
	ret = nrf71_sr_coex_init(&cfg);
	printk("nrf71_sr_coex_init() twice: %d (expected %d)\n", ret, -EALREADY);

	printk("nRF71 SR coexistence driver built\n");

	return 0;
}
