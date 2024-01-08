/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "main.h"

int main(void)
{
	int ret = 0;
	bool is_ant_mode_sep = IS_ENABLED(CONFIG_COEX_SEP_ANTENNAS);
	bool is_ot_client = IS_ENABLED(CONFIG_OT_ROLE_CLIENT);
	bool is_wifi_server = IS_ENABLED(CONFIG_WIFI_ZPERF_SERVER);
	bool is_wifi_zperf_udp = IS_ENABLED(CONFIG_WIFI_ZPERF_PROT_UDP);
	bool is_ot_zperf_udp = IS_ENABLED(CONFIG_OT_ZPERF_PROT_UDP);
	bool test_wifi = IS_ENABLED(CONFIG_TEST_TYPE_WLAN);
	bool test_thread = IS_ENABLED(CONFIG_TEST_TYPE_OT);


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
	LOG_INF("Configure SR side switch");
	ret = nrf_wifi_config_sr_switch(is_ant_mode_sep);
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

	ret = wifi_tput_ot_tput(test_wifi, is_ant_mode_sep,
		test_thread, is_ot_client, is_wifi_server, is_wifi_zperf_udp, is_ot_zperf_udp);

	if (ret != 0) {
		LOG_INF("Test case failed");
		goto err;
	}
	LOG_INF("Test case(s) complete");

	return 0;

err:
	LOG_INF("Returning with error");
	return ret;
}
