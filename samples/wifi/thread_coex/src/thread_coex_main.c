/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi and Bluetooth LE coexistence sample
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/zperf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>

#include "coex.h"
#include "fmac_main.h"


/**
 * @brief Function to test Wi-Fi throughput client/server and Thread throughput client/server
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_tput_ot_tput(bool test_wifi, bool is_ant_mode_sep, bool test_thread,
		bool is_ot_client, bool is_wifi_server, bool is_wifi_zperf_udp,
		bool is_ot_zperf_udp, bool is_sr_protocol_ble);

/**
 * @brief memset_context
 *
 * @return No return value.
 */
void memset_context(void);

/**
 * @brief Handle net management callbacks
 *
 * @return No return value.
 */
void wifi_mgmt_callback_functions(void);

/**
 * @brief Wi-Fi init.
 *
 * @return None
 */
void wifi_init(void);

/**
 * @brief Network configuration
 *
 * @return status
 */
int net_config_init_app(const struct device *dev, const char *app_info);


#include <zephyr/kernel.h>
#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
#include <nrfx_clock.h>
#endif

int setup_interfaces(void)
{
	const struct device *dev = device_get_binding("wlan0");
	/* Without this, DHCPv4 starts on first interface and if that is not Wi-Fi or
	 * only supports IPv6, then its an issue. (E.g., OpenThread)
	 *
	 * So, DHCPv4 is started on Wi-Fi interface always, independent of the ordering.
	 */
	struct net_if *wifi_iface = net_if_lookup_by_dev(dev);

	/* As both are Ethernet, need to set specific interface */
	net_if_set_default(wifi_iface);

	net_config_init_app(dev, "Initializing network");

	return 0;
}

int main(void)
{

	int ret = 0;

#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
	/* For now hardcode to 128MHz */
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK,
			       NRF_CLOCK_HFCLK_DIV_1);
#endif
	printk("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock/MHZ(1));

	/* LOG_INF("Set up interfaces"); */
	ret = setup_interfaces();
	if (ret) {
		LOG_INF("Failed to setup ifaces: %d\n", ret);
		return -1;
	}

#if !defined(CONFIG_COEX_SEP_ANTENNAS) && !(CONFIG_NRF70_SR_COEX_RF_SWITCH)
	BUILD_ASSERT("Shared antenna support is not available with nRF7002 shields");
#endif

	/* register callback functions */
	//wifi_init();

	return 0;
}
