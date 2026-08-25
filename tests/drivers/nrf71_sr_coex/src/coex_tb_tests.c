/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <nrf71_coex_if.h>
#include <nrf71_sr_coex_api.h>

#include "coex_tb.h"
#include "coex_tb_params.h"
#include "coex_tb_stats.h"
#include "coex_tb_tests.h"
#include "coex_tb_util.h"

/** Configure COEXC hardware (not a CD2CM command). */
static int tb_configure_enable_coexc(void)
{
	uint32_t ccmallow;
	int ret;

	printk("\n=================== configure and enable COEXC\n");

	ret = coex_cd_coexc_configure_and_enable(COEX_SHARED_ANT_CFG);
	if (ret != 0) {
		printk("Coex TB: COEXC configure/enable failed (%d)\n", ret);
		return ret;
	}

	ret = coex_cd_coexc_verify_configured(COEX_SHARED_ANT_CFG, &ccmallow);
	if (ret != 0) {
		printk("Coex TB: COEXC verify failed (%d)\n", ret);
		return ret;
	}

	printk("Coex TB: COEXC OK (CCMALLOW[0][0]=0x%08x)\n", ccmallow);
	return 0;
}

/**
 * Re-apply CM configuration after Wi-Fi power-up (CD2CM_SET_PRIORITY_RANGES,
 * CD2CM_UPDATE_COEX_USER_PARAMS, CD2CM_UPDATE_COEX_PARAMS,
 * CD2CM_ENABLE_COEXISTENCE and their CM2CD completion events).
 */
static int tb_init_and_enable(void)
{
	int ret;

	printk("\n=================== init: Wi-Fi and SR power notify\n");

	ret = coex_cd_wifi_power_notify(COEX_WIFI_POWERED_UP_READY);
	if (ret != 0) {
		printk("Coex TB: Wi-Fi power notify failed (%d)\n", ret);
		return ret;
	}

	ret = coex_cd_sr_power_notify(COEX_SR_POWERED_UP_READY);
	if (ret != 0) {
		printk("Coex TB: SR power notify failed (%d)\n", ret);
		return ret;
	}

	printk("Coex TB: radios reported ready\n");
	return 0;
}

/** Post CD2CM_SET_PRIORITY_RANGES and wait for CM2CD_SET_PRIORITY_RANGES_EVENT. */
static int tb_set_priority_ranges(void)
{
	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;

	printk("\n============================================= set priority ranges\n");
	coex_tb_params_priority_ranges(&wifi_range, &sr_range);

	return coex_cd_set_priority_ranges(&wifi_range, &sr_range);
}

/** Post CD2CM_UPDATE_COEX_USER_PARAMS and wait for CM2CD_UPDATE_COEX_USER_PARAMS_EVENT. */
static int tb_update_coex_user_parameters(void)
{
	struct coex_user_params_t user_params;

	printk("\n================================================== update coex user params\n");
	coex_tb_params_user_params(&user_params);

	return coex_cd_update_user_params(&user_params);
}

/** Post CD2CM_UPDATE_COEX_PARAMS and wait for CM2CD_UPDATE_COEX_PARAMS_EVENT. */
static int tb_update_coex_parameters(void)
{
	printk("\n================================================== update coex params\n");

	return coex_cd_update_coex_params();
}

/** Post CD2CM_ENABLE_COEXISTENCE and wait for CM2CD_ENABLE_COEXISTENCE_EVENT. */
static int tb_enable_disable_coex(uint32_t num_tests)
{
	int ret;

	printk("\n========================== enable/disable coexistence (%u iterations)\n",
	       num_tests);

	for (uint32_t test = 1U; test <= num_tests; test++) {
		bool enable = ((test % 2U) == 0U);

		printk("\n--- COEX %s test %u ---\n", enable ? "ENABLE" : "DISABLE", test);
		ret = coex_cd_enable(enable);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

/** Post CD2CM_UPDATE_COEX_USER_PARAMS and wait for CM2CD_UPDATE_COEX_USER_PARAMS_EVENT. */
static int tb_force_antenna_allocation(void)
{
	static const enum antenna_allocation_t modes[] = {
		ANT_ALLOC_DYNAMIC,
		ANT_ALLOC_STATIC_WIFI,
		ANT_ALLOC_STATIC_SR,
	};
	static const char *labels[] = {"DYNAMIC", "STATIC_WIFI", "STATIC_SR"};
	struct coex_user_params_t user_params;
	int ret;

	printk("\n========================== force shared antenna allocation\n");

	for (size_t i = 0U; i < ARRAY_SIZE(modes); i++) {
		printk("--- FORCE_ANTENNA_ALLOC: %s ---\n", labels[i]);
		coex_tb_params_user_params_ant_alloc(&user_params, modes[i]);
		ret = coex_cd_update_user_params(&user_params);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

/** Post CD2CM_UPDATE_COEX_PARAMS and wait for CM2CD_UPDATE_COEX_PARAMS_EVENT. */
static int tb_control_shared_lna(void)
{
	static const uint8_t lna_modes[] = {0U, 1U, 2U};
	static const char *labels[] = {"SHA_LNA_DYNAMIC", "SHA_LNA_STATIC_EN",
				       "SHA_LNA_STATIC_DIS"};
	uint8_t blob[128];
	size_t blob_len;
	int ret;

	printk("\n============================ force shared LNA switch\n");

	for (size_t i = 0U; i < ARRAY_SIZE(lna_modes); i++) {
		printk("--- FORCE_LNASW: %s ---\n", labels[i]);

		ret = coex_tb_params_coex_blob_lna(lna_modes[i], blob, sizeof(blob), &blob_len);
		if (ret != 0) {
			return ret;
		}

		ret = coex_cd_update_coex_params_blob(blob, blob_len);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

/** Post CD2CM_ALLOCATE_PPW and wait for CM2CD_ALLOCATE_PPW_EVENT. */
static int tb_generate_ppws(void)
{
	struct coex_ppw_parameters_t ppw;
	int ret;

	printk("\n================================================== generate PPWs\n");

	for (uint32_t case_idx = 0U; case_idx < 2U; case_idx++) {
		bool first_wifi = (case_idx == 0U);

		printk("--- PPW start (%s first window) ---\n",
		       first_wifi ? "Wi-Fi" : "SR");
		coex_tb_params_ppw_start(&ppw, first_wifi);

		ret = coex_cd_allocate_ppw(&ppw);
		if (ret != 0) {
			return ret;
		}

		coex_tb_wait_ms(COEX_TB_PPW_TOTAL_DURATION_MS);

		printk("--- PPW stop ---\n");
		coex_tb_params_ppw_stop(&ppw);

		ret = coex_cd_allocate_ppw(&ppw);
		if (ret != 0) {
			return ret;
		}

		coex_tb_wait_ms(COEX_TB_PPW_POST_WAIT_MS);
	}

	return 0;
}

/**
 * Post CD2CM_WIFI_SW_CLIENT_REQUEST and wait for CM2CD_WIFI_SW_CLIENT_STATUS_EVENT
 * (request and release).
 */
static int tb_wifi_sw_client_request(void)
{
	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;
	struct coex_user_params_t user_params;
	struct coex_sw_client_params_t sw_params;
	int ret;

	printk("\n================= Wi-Fi SW client request/release\n");

	ret = coex_cd_enable(true);
	if (ret != 0) {
		return ret;
	}

	coex_tb_params_priority_ranges(&wifi_range, &sr_range);
	ret = coex_cd_set_priority_ranges(&wifi_range, &sr_range);
	if (ret != 0) {
		return ret;
	}

	for (uint32_t iter = 1U; iter <= 2U; iter++) {
		uint32_t prob = ((iter % 2U) == 1U) ? 100U : 0U;

		printk("--- Wi-Fi SW client iteration %u (prob=%u%%) ---\n", iter, prob);
		coex_tb_params_user_params_wifi_prob(&user_params, prob);

		ret = coex_cd_update_user_params(&user_params);
		if (ret != 0) {
			return ret;
		}

		coex_tb_params_wifi_sw_client(&sw_params, WIFI_SW_CLIENT_REQUEST,
					      30U * 1000U, 0U);
		ret = coex_cd_wifi_sw_client_request(&sw_params);
		if (ret != 0) {
			return ret;
		}

		coex_tb_params_wifi_sw_client(&sw_params, WIFI_SW_CLIENT_RELEASE, 0U, 0U);
		ret = coex_cd_wifi_sw_client_request(&sw_params);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

/**
 * Post CD2CM_WIFI_SW_CLIENT_REQUEST and wait for CM2CD_WIFI_SW_CLIENT_STATUS_EVENT
 * with short timeout on 2.4G and 5G bands.
 */
static int tb_wifi_sw_client_timeout(void)
{
	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;
	struct coex_user_params_t user_params;
	struct coex_sw_client_params_t sw_params;
	int ret;

	printk("\n================================= Wi-Fi SW client timeout test\n");

	ret = coex_cd_enable(true);
	if (ret != 0) {
		return ret;
	}

	coex_tb_params_priority_ranges(&wifi_range, &sr_range);
	ret = coex_cd_set_priority_ranges(&wifi_range, &sr_range);
	if (ret != 0) {
		return ret;
	}

	coex_tb_params_user_params(&user_params);
	ret = coex_cd_update_user_params(&user_params);
	if (ret != 0) {
		return ret;
	}

	for (uint32_t iter = 1U; iter <= 4U; iter++) {
		unsigned int band = ((iter % 2U) == 1U) ? COEXC_MODE_WIFI_2PT4G :
							    COEXC_MODE_WIFI_5G;

		printk("--- Wi-Fi SW client timeout iteration %u (band=%u, timeout=50 ms) ---\n",
		       iter, band);

		coex_tb_params_wifi_sw_client(&sw_params, WIFI_SW_CLIENT_REQUEST, 50U, band);
		ret = coex_cd_wifi_sw_client_request(&sw_params);
		if (ret != 0) {
			return ret;
		}

		coex_tb_wait_ms(COEX_TB_CMD_WAIT_MS);
	}

	coex_tb_wait_ms(COEX_TB_PPW_POST_WAIT_MS);

	return 0;
}

/** Post CD2CM_GET_STATS, wait for CM2CD_STATISTICS_EVENT, and validate stats. */
static int tb_get_coex_manager_stats(void)
{
	int ret;

	printk("\n========================================================== get CM stats\n");

	ret = coex_cd_get_stats();
	if (ret != 0) {
		return ret;
	}

	coex_tb_print_and_validate_stats(coex_cd_get_last_stats(),
					 coex_cd_get_last_patch_stats());
	return 0;
}

/** Run all enabled coexistence test bench cases in sequence. */
void coex_tb_run_all(void)
{
	int ret = 0;

	printk("\n\n========== Coexistence Manager host test bench start ==========\n");

#ifdef COEX_TB_TEST_CONFIGURE_ENABLE_COEXC
	if (ret == 0) {
		ret = tb_configure_enable_coexc();
	}
#endif

#ifdef COEX_TB_TEST_INIT_AND_ENABLE
	if (ret == 0) {
		ret = tb_init_and_enable();
	}
#endif

#ifdef COEX_TB_TEST_SET_PRIORITY_RANGES
	if (ret == 0) {
		ret = tb_set_priority_ranges();
	}
#endif

#ifdef COEX_TB_TEST_UPDATE_COEX_USER_PARAMS
	if (ret == 0) {
		ret = tb_update_coex_user_parameters();
	}
#endif

#ifdef COEX_TB_TEST_UPDATE_COEX_PARAMETERS
	if (ret == 0) {
		ret = tb_update_coex_parameters();
	}
#endif

#ifdef COEX_TB_TEST_ENABLE_COEXISTENCE
	if (ret == 0) {
		ret = tb_enable_disable_coex(COEX_TB_NUM_ENABLE_COEX_TESTS);
	}
#endif

#ifdef COEX_TB_TEST_FORCE_ANTENNA_ALLOC
	if (ret == 0) {
		ret = tb_force_antenna_allocation();
	}
#endif

#ifdef COEX_TB_TEST_FORCE_LNASW_ALLOC
	if (ret == 0) {
		ret = tb_control_shared_lna();
	}
#endif

#ifdef COEX_TB_TEST_GENERATE_PPW
	if (ret == 0) {
		ret = tb_generate_ppws();
	}
#endif

#ifdef COEX_TB_TEST_WIFI_SW_CLIENT_REQUEST
	if (ret == 0) {
		ret = tb_wifi_sw_client_request();
	}
#endif

#ifdef COEX_TB_TEST_WIFI_SW_CLIENT_TIMEOUT
	if (ret == 0) {
		ret = tb_wifi_sw_client_timeout();
	}
#endif

#ifdef COEX_TB_TEST_GET_CM_STATS
	if (ret == 0) {
		ret = tb_get_coex_manager_stats();
	}
#endif

	coex_tb_wait_ms(COEX_TB_FINAL_WAIT_MS);

	if (ret == 0) {
		printk("\n========== Coexistence Manager host test bench PASS ==========\n");
	} else {
		printk("\n========== Coexistence Manager host test bench FAIL (%d) ==========\n",
		       ret);
	}
}
