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
#include <openthread/thread.h>

#include "zephyr/net/openthread.h"
#include "zephyr_fmac_main.h"
#include <zephyr/logging/log.h>


#include "thread_utils.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* To check if the OT device role is as expected */
char* ot_state                   = "router"; /* set to "leader" or "router"*/
uint32_t ot_state_chk_iteration  = 0;
uint32_t total_duration_to_check = 20; /* seconds */
uint32_t time_gap_between_checks = 1; /* seconds */
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


	/** Step1: Set null network key i.e,
	 *  ot networkkey 00000000000000000000000000000000 
	 */ 
	otInstance *instance = openthread_get_default_instance();
	struct openthread_context *context = openthread_get_default_context();
	setNullNetworkKey(instance);

	/** Step2: Bring up the interface and start joining to the network on DK2 with pre-shared key. 
	 *   i.e. ot ifconfig up 
	 *        ot joiner start FEDCBA9876543210
	 */
	otIp6SetEnabled(instance, true); /* ot ifconfig up */
	
	
	//thread_start_joiner("FEDCBA9876543210");
	thread_start_joiner("FEDCBA9876543210", instance); /* ot joiner start <PSK> ... PSK = FEDCBA9876543210 */
	//thread_start_joiner("FEDCBA9876543210", instance, context); /* ot joiner start <PSK> ... PSK = FEDCBA9876543210 */
	

	/* check OT device join status periodically until device join is success (or) timeout */
	max_iterations = total_duration_to_check/time_gap_between_checks;	
	LOG_INF("Check OT device join status for every %d seconds..", time_gap_between_checks);
	LOG_INF("until the device join is success (or) timeout of %d seconds", total_duration_to_check);

	while(1) {
		ot_state_chk_iteration++;

		/* exit when OT device join success (or) timeout */
		if (is_ot_join_done) {
			LOG_INF("OT device join Success");
			break;
		}
		if (ot_state_chk_iteration >= max_iterations) {
			LOG_ERR("OT device join timeout");
			goto err;
			break;
		}
		k_sleep(K_SECONDS(time_gap_between_checks));	
	}	
	
	/** Step3: Start Thread for joiner. 
	 *   i.e ot thread start
	 *   Note: Device should join Thread network within 20s of timeout.
	 */
	otError err = otThreadSetEnabled(instance, true);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
		goto err;
	}
	
	/** Step4: Ping from joiner to commissioner to verify connectivity.
	 *  i.e. ot ping <commissioner address>
	 */
    /* PENDING */
	
	k_sleep(K_SECONDS(3));
	//thread_throughput_test_exit();

	LOG_INF("Exiting the test after thread device joined the network");
	goto end_of_main;

err:
	LOG_INF("Returning with error");

end_of_main:

	return ret;
}
