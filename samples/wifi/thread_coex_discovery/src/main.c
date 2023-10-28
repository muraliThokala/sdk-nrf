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


#include "thread_utils.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* To check if the OT device role is as expected */
char* ot_state                   = "router"; /* set to "leader" or "router"*/
uint32_t ot_state_chk_iteration  = 0;
uint32_t total_duration_to_check = 120; /* seconds */
uint32_t time_gap_between_checks = 5; /* seconds */
uint32_t max_iterations          = 1;
uint32_t ret_strcmp              = 1; /* set to non-zero */
bool is_ot_state_matched         = false;

int main(void)
{
	int ret = 0;
	bool is_ant_mode_sep = IS_ENABLED(CONFIG_COEX_SEP_ANTENNAS);
	bool is_thread_client = IS_ENABLED(CONFIG_THREAD_ROLE_CLIENT);
	bool is_wlan_server = IS_ENABLED(CONFIG_WIFI_ZPERF_SERVER);
	bool is_zperf_udp = IS_ENABLED(CONFIG_WIFI_ZPERF_PROT_UDP);
	bool test_wlan = IS_ENABLED(CONFIG_TEST_TYPE_WLAN);
	bool test_thread = IS_ENABLED(CONFIG_TEST_TYPE_THREAD);

#if !defined(CONFIG_COEX_SEP_ANTENNAS) && \
	!(defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP))
	BUILD_ASSERT("Shared antenna support is not available with nRF7002 shields");
#endif


wifi_memset_context();
	
wifi_net_mgmt_callback_functions();

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
	#if defined(CONFIG_NRF700X_SR_COEX)
		/* Configure SR side (nRF5340 side) switch in nRF7002 DK */
		LOG_INF("Configure SR side (nRF5340 side) switch");
		ret = nrf_wifi_config_sr_switch(is_ant_mode_sep);
		if (ret != 0) {
			LOG_ERR("Unable to configure SR side switch: %d", ret);
			goto err;
		}
	#endif /* CONFIG_NRF700X_SR_COEX */
#endif

#if defined(CONFIG_NRF700X_SR_COEX)
	/* Configure non-PTA registers of Coexistence Hardware */
	LOG_INF("Configuring non-PTA registers.");
	ret = nrf_wifi_coex_config_non_pta(is_ant_mode_sep);
	if (ret != 0) {
		LOG_ERR("Configuring non-PTA registers of CoexHardware FAIL");
		goto err;
	}
#endif /* CONFIG_NRF700X_SR_COEX */


	thread_throughput_test_init(false);
	
	k_sleep(K_SECONDS(3));
	
	/* LOG_INF("Waiting for OT discover to complete"); */
	while (1) {
		if (is_ot_discovery_done) {
			break;
		} 
	}
	LOG_INF("OT discover complete");

	/* check OT device state periodically until the expected state is found */
	max_iterations = total_duration_to_check/time_gap_between_checks;	
	LOG_INF("Check OT device state for every %d seconds..", time_gap_between_checks);
	LOG_INF("until the expected state is found (or) timeout of %d seconds", total_duration_to_check);
	
	LOG_INF("OT device expected state %s", ot_state);
	while(1) {
		ot_state_chk_iteration++;
		/* LOG_INF("ot_state_chk_iteration: %d",ot_state_chk_iteration); */
		ret_strcmp = strcmp(ot_state, check_ot_state());

		/* exit when OT device role matches the expected state (or) timeout */
		if (ret_strcmp == 0) {
			is_ot_state_matched = true;
			break;
		}
		if (ot_state_chk_iteration >= max_iterations) {
			is_ot_state_matched = false;
			break;
		}
		k_sleep(K_SECONDS(time_gap_between_checks));	
	}	
	if (is_ot_state_matched == true) {
		LOG_INF("OT device state matched with the expected role");
	} else {
		LOG_ERR("OT device state NOT matched with the expected role");
	}
	k_sleep(K_SECONDS(3));
	thread_throughput_test_exit();

	LOG_INF("Exiting the test after open thread discovery");
	goto end_of_main;

#if 0	
	if (IS_ENABLED(CONFIG_WIFI_TP_UDP_CLIENT_THREAD_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_CLIENT_THREAD_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_CLIENT_THREAD_TP_SERVER) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_CLIENT_THREAD_TP_SERVER) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_SERVER_THREAD_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_SERVER_THREAD_TP_CLIENT) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_SERVER_THREAD_TP_SERVER) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_SERVER_THREAD_TP_SERVER)) {
		ret = wifi_tput_thread_tput(test_wlan, is_ant_mode_sep,
		test_thread, is_thread_client, is_wlan_server, is_zperf_udp);
	}
	if (ret != 0) {
		LOG_INF("Test case failed");
		goto err;
	}
	return 0;


#endif

err:
	LOG_INF("Returning with error");

end_of_main:

	return ret;
}
