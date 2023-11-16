/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief SR coexistence sample test bench
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include <nrfx_clock.h>
#include "zephyr_fmac_main.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define RSSI_INIT_VALUE 127
#define TXPOWER_INIT_VALUE 127

int main(void)
{
	int ret = 0;
	bool is_ant_mode_sep = IS_ENABLED(CONFIG_COEX_SEP_ANTENNAS);
	bool is_ot_client = IS_ENABLED(CONFIG_OT_ROLE_CLIENT);
	bool is_wifi_server = IS_ENABLED(CONFIG_WIFI_ZPERF_SERVER);
	bool is_wifi_zperf_udp = IS_ENABLED(CONFIG_WIFI_ZPERF_PROT_UDP);
	bool test_wifi = IS_ENABLED(CONFIG_TEST_TYPE_WLAN);
	bool test_openThread = IS_ENABLED(CONFIG_TEST_TYPE_OT);

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
	bool ot_external_antenna = IS_ENABLED(CONFIG_OT_EXTERNAL_ANTENNA);
#endif

	bool is_wifi_conn_scan =
		IS_ENABLED(CONFIG_WIFI_CON_SCAN_OT_DISCOV) ||
		IS_ENABLED(CONFIG_WIFI_CON_SCAN_OT_CONNECTION) ||
		IS_ENABLED(CONFIG_WIFI_CON_SCAN_OT_TP_CLIENT) ||
		IS_ENABLED(CONFIG_WIFI_CON_SCAN_OT_TP_SERVER) ||
		IS_ENABLED(CONFIG_OT_CON_STABILITY_WIFI_CON_SCAN_INTERFERENCE);

#if !defined(CONFIG_COEX_SEP_ANTENNAS) && \
	!(defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP))
	BUILD_ASSERT("Shared antenna support is not available with nRF7002 shields");
#endif

	/* register callback functions etc */
	wifi_init();	

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
	#if defined(CONFIG_NRF700X_BT_COEX)
		/* Configure SR side (nRF5340 side) switch in nRF7x */
		LOG_INF("Configure SR side (nRF5340 side) switch");
		ret = nrf_wifi_config_sr_switch(is_ant_mode_sep, ot_external_antenna);
		if (ret != 0) {
			LOG_ERR("Unable to configure SR side switch: %d", ret);
			goto err;
		}
	#endif /* CONFIG_NRF700X_BT_COEX */
#endif

#if defined(CONFIG_NRF700X_BT_COEX)
	/* Configure non-PTA registers of Coexistence Hardware */
	LOG_INF("Configuring non-PTA registers.");
	ret = nrf_wifi_coex_config_non_pta(is_ant_mode_sep);
	if (ret != 0) {
		LOG_ERR("Configuring non-PTA registers of CoexHardware FAIL");
		goto err;
	}
#endif /* CONFIG_NRF700X_BT_COEX */

	if (IS_ENABLED(CONFIG_WIFI_SCAN_OT_DISCOV) ||
	IS_ENABLED(CONFIG_WIFI_CON_SCAN_OT_DISCOV)) {
		ret = wifi_scan_ot_discov(is_ant_mode_sep, test_openThread, test_wifi,
		is_ot_client, is_wifi_conn_scan);
	}

	if (IS_ENABLED(CONFIG_WIFI_SCAN_OT_CONNECTION) ||
	IS_ENABLED(CONFIG_WIFI_CON_SCAN_OT_CONNECTION)) {
		ret = wifi_scan_ot_connection(is_ant_mode_sep, test_openThread, test_wifi,
		is_ot_client, is_wifi_conn_scan);
	}
	
	if (IS_ENABLED(CONFIG_WIFI_SCAN_OT_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_CON_SCAN_OT_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_SCAN_OT_TP_SERVER) ||
	IS_ENABLED(CONFIG_WIFI_CON_SCAN_OT_TP_SERVER)) {
		ret = wifi_scan_ot_tput(is_ant_mode_sep, test_openThread, test_wifi,
		is_ot_client, is_wifi_conn_scan);
	}

	if (IS_ENABLED(CONFIG_WIFI_CON_OT_DISCOV)) {
		ret = wifi_con_ot_discov(test_wifi, is_ant_mode_sep, test_openThread, is_ot_client);
	}

	if (IS_ENABLED(CONFIG_WIFI_CON_OT_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_CON_OT_TP_SERVER)) {
		ret = wifi_con_ot_tput(test_wifi, is_ant_mode_sep, test_openThread, is_ot_client);
	}

	if (IS_ENABLED(CONFIG_WIFI_TP_UDP_CLIENT_OT_DISCOV) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_CLIENT_OT_DISCOV) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_SERVER_OT_DISCOV) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_SERVER_OT_DISCOV)) {
		ret = wifi_tput_ot_discov(test_wifi, test_openThread, is_ot_client,
		is_wifi_server, is_ant_mode_sep, is_wifi_zperf_udp);
	}

	if (IS_ENABLED(CONFIG_WIFI_TP_UDP_CLIENT_OT_CONNECTION) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_CLIENT_OT_CONNECTION) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_SERVER_OT_CONNECTION) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_SERVER_OT_CONNECTION)) {
		ret = wifi_tput_ot_connection(test_wifi, test_openThread, is_ot_client,
		is_wifi_server, is_ant_mode_sep, is_wifi_zperf_udp);
	}
	
	if (IS_ENABLED(CONFIG_WIFI_TP_UDP_CLIENT_OT_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_CLIENT_OT_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_CLIENT_OT_TP_SERVER) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_CLIENT_OT_TP_SERVER) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_SERVER_OT_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_SERVER_OT_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_SERVER_OT_TP_SERVER) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_SERVER_OT_TP_SERVER)) {
		ret = wifi_tput_ot_tput(test_wifi, is_ant_mode_sep,
		test_openThread, is_ot_client, is_wifi_server, is_wifi_zperf_udp);
	}

	if (IS_ENABLED(CONFIG_WIFI_CON_STABILITY_OT_DISC_INTERFERENCE)) {
		ret = wifi_con_stability_ot_discov_interference(test_wifi, test_openThread, is_ot_client,
		is_ant_mode_sep);
	}

	if (IS_ENABLED(CONFIG_WIFI_CON_STABILITY_OT_TP_CLIENT_INTERFERENCE) ||
	IS_ENABLED(CONFIG_WIFI_CON_STABILITY_OT_TP_SERVER_INTERFERENCE)) {
		ret = wifi_con_stability_ot_tput_interference(test_wifi, is_ant_mode_sep, test_openThread,
			is_ot_client);
	}

	if (IS_ENABLED(CONFIG_OT_CON_STABILITY_WIFI_SCAN_INTERFERENCE) ||
	IS_ENABLED(CONFIG_OT_CON_STABILITY_WIFI_CON_SCAN_INTERFERENCE)) {
		ret = ot_con_stability_wifi_scan_interference(is_ant_mode_sep, test_openThread, test_wifi,
		is_ot_client, is_wifi_conn_scan);
	}

	if (IS_ENABLED(CONFIG_OT_CON_STABILITY_WIFI_TP_UDP_CLIENT_INTERFERENCE) ||
	IS_ENABLED(CONFIG_OT_CON_STABILITY_WIFI_TP_TCP_CLIENT_INTERFERENCE) ||
	IS_ENABLED(CONFIG_OT_CON_STABILITY_WIFI_TP_UDP_SERVER_INTERFERENCE) ||
	IS_ENABLED(CONFIG_OT_CON_STABILITY_WIFI_TP_TCP_SERVER_INTERFERENCE)) {
		ret = ot_connection_stability_wifi_tput_interference(test_wifi, test_openThread,
		is_ot_client, is_wifi_server, is_ant_mode_sep, is_wifi_zperf_udp);
	}

	if (IS_ENABLED(CONFIG_WIFI_SHUTDOWN_OT_DISCOV)) {
		ret = wifi_shutdown_ot_discov(is_ot_client);
	}
	
	if (IS_ENABLED(CONFIG_WIFI_SHUTDOWN_OT_CONNECTION)) {
		ret = wifi_shutdown_ot_connection(is_ot_client);
	}
	
	if (IS_ENABLED(CONFIG_WIFI_SHUTDOWN_OT_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_SHUTDOWN_OT_TP_SERVER)) {
		ret = wifi_shutdown_ot_tput(is_ot_client);
	}
	
	if (ret != 0) {
		LOG_INF("Test case failed");
		goto err;
	}
	if (ot_tx_power != TXPOWER_INIT_VALUE) {
		LOG_INF("OT Tx power: %d", ot_tx_power);
	} else {
		LOG_INF("OT Tx power: N/A");
	}
	if (wifi_rssi != RSSI_INIT_VALUE) {
		LOG_INF("WiFi RSSI: %d", wifi_rssi);
	} else {
		LOG_INF("WiFi RSSI: N/A");
	}
	if (ot_rssi != RSSI_INIT_VALUE) {
		LOG_INF("OT RSSI: %d", ot_rssi);
	} else {
		LOG_INF("OT RSSI: N/A");
	}
	LOG_INF("Test case(s) complete");

	return 0;

err:
	LOG_INF("Returning with error");
	return ret;
}
