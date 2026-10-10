/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi and Bluetooth LE coexistence sample
 */

#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#if defined(CONFIG_NRFX_CLOCK_HFCLK) &&                                                            \
	(defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
#include <nrfx_clock_hfclk.h>
#endif
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_config.h>

int main(void)
{
#if defined(CONFIG_NRFX_CLOCK_HFCLK) &&                                                            \
	(defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
	/* For now hardcode to 128MHz */
	nrfx_clock_hfclk_divider_set(NRF_CLOCK_HFCLK_DIV_1);
#endif
	printk("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock / MHZ(1));

#if defined(CONFIG_NET_CONFIG_SETTINGS) && defined(CONFIG_WIFI)
	/* Without this, DHCPv4 starts on first interface and if that is not Wi-Fi or
	 * only supports IPv6, then its an issue. (E.g., OpenThread)
	 *
	 * So, we start DHCPv4 on Wi-Fi interface always, independent of the ordering.
	 */
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_wifi));
	struct net_if *wifi_iface = net_if_lookup_by_dev(dev);

	if (!wifi_iface) {
		printk("No network interface for the Wi-Fi device\n");
		return -ENODEV;
	}

	/* As both are Ethernet, we need to set specific interface */
	net_if_set_default(wifi_iface);

	net_config_init_app(dev, "Initializing network");
#endif

	return 0;
}
