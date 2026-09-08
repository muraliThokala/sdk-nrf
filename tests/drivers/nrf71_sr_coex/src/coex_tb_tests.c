/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence driver test steps and the top-level runner.
 *
 * Each tb_* function below is one test step against a coexistence driver API.
 * Most steps drive a CD2CM command and its completion event, but several cover
 * driver behaviour with no CM involvement at all: COEXC register programming,
 * radio power notifications from the Wi-Fi and SR drivers, and argument
 * rejection.
 *
 * A step returns 0 when everything it tried was accepted, or a negative errno on
 * the first thing that was not. The runner at the bottom of the file executes
 * the enabled steps in order and stops at the first failure, so later steps can
 * safely assume that earlier ones succeeded.
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <nrf71_coex_if.h>
#include <nrf71_cd_sr_if.h>
#include <nrf71_sr_coex_api.h>

#include "coex_tb.h"
#include "coex_tb_params.h"
#include "coex_tb_stats.h"
#include "coex_tb_tests.h"
#include "coex_tb_util.h"

/**
 * Check that an API returned exactly the errno the driver contract promises.
 *
 * Used by the negative test, where the expected outcome is a rejection. A
 * "wrong error" is as much a bug as no error at all, so the actual and expected
 * values are both printed.
 *
 * @return 1 if the check failed, 0 if it passed, so callers can sum the result.
 */
static unsigned int tb_expect_errno(int actual, int expected, const char *what)
{
	bool pass = (actual == expected);

	printk("  %-54s got %s (%d), want %s (%d) : %s\n", what, coex_tb_errstr(actual),
	       actual, coex_tb_errstr(expected), expected, pass ? "PASS" : "FAIL");

	return pass ? 0U : 1U;
}

/**
 * Check a plain condition that has no errno to report.
 *
 * @return 1 if the check failed, 0 if it passed.
 */
static unsigned int tb_expect_true(bool condition, const char *what)
{
	printk("  %-54s %s\n", what, condition ? "PASS" : "FAIL");

	return condition ? 0U : 1U;
}

/**
 * Program the COEXC arbitration tables, then read one back to confirm.
 *
 */
static int tb_configure_coexc(void)
{
	uint32_t ccmallow;
	int ret;

	printk("\n=================== configure and enable COEXC\n");

	ret = coex_cd_configure_COEXC(COEX_SHARED_ANT_CFG);
	if (ret != 0) {
		printk("Coex TB: COEXC configure/enable failed: %s (%d)\n", coex_tb_errstr(ret),
		       ret);
		return ret;
	}

	/*
	 * Read back CCMALLOW client 0 / mode 0. It acts as a fingerprint of the
	 * programmed antenna configuration, so a matching value confirms the
	 * whole table was written rather than silently dropped.
	 */
	ret = coex_cd_coexc_verify_configured(COEX_SHARED_ANT_CFG, &ccmallow);
	if (ret != 0) {
		printk("Coex TB: COEXC verify failed: %s (%d)\n", coex_tb_errstr(ret), ret);
		return ret;
	}

	printk("Coex TB: COEXC OK (CCMALLOW[0][0]=0x%08x)\n", ccmallow);
	return 0;
}

/**
 * Confirm the driver rejects bad input instead of forwarding it to the CM.
 *
 * This runs before the radios are reported up, which is deliberate: it also
 * covers the "coexistence is not running yet" gate on the SR activity API. None
 * of these calls should reach the RPU, so none of them disturb CM state or the
 * counters checked at the end of the run.
 */
static int tb_negative_args(void)
{
	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;
#ifdef COEX_DRIVER_POST_PHASE_1
	struct short_range_activity_info_t activity;
	struct coex_sr_sw_client_params_t sr_client = {0};
	enum coex_sr_sw_client_req_status_t grant_status = SR_SW_CLIENT_REQ_SUCCESS;
#endif
	uint8_t blob[COEX_TB_COEX_PARAMS_BLOB_MAX + 1U] = {0};
	unsigned int failures = 0U;
	int bad_enum_value = 2; /* Neither COEX_SHARED_ANT_CFG nor COEX_SEPARATE_ANT_CFG. */

	printk("\n=================== argument validation (nothing reaches the CM)\n");

	coex_tb_params_priority_ranges(&wifi_range, &sr_range);

	/* NULL pointers must be refused by the driver, not dereferenced. */
	failures += tb_expect_errno(coex_cd_set_priority_ranges(NULL, &sr_range), -EINVAL,
				    "set_priority_ranges(NULL wifi)");
	failures += tb_expect_errno(coex_cd_set_priority_ranges(&wifi_range, NULL), -EINVAL,
				    "set_priority_ranges(NULL sr)");
	failures += tb_expect_errno(coex_cd_update_user_params(NULL), -EINVAL,
				    "update_user_params(NULL)");
	failures += tb_expect_errno(coex_cd_wifi_channel_notify(NULL), -EINVAL,
				    "wifi_channel_notify(NULL)");
#ifdef COEX_DRIVER_POST_PHASE_1
	failures += tb_expect_errno(coex_cd_allocate_ppw(NULL), -EINVAL,
				    "allocate_ppw(NULL)");
	failures += tb_expect_errno(coex_cd_wifi_sw_client_request(NULL), -EINVAL,
				    "wifi_sw_client_request(NULL)");
	failures += tb_expect_errno(coex_cd_update_short_range_activity_info(NULL), -EINVAL,
				    "sr_activity_info(NULL)");
#endif /* COEX_DRIVER_POST_PHASE_1 */

	/* Blob length must be non-zero and within the driver's internal cap. */
	failures += tb_expect_errno(coex_cd_update_coex_params_blob(NULL, 16U), -EINVAL,
				    "update_coex_params_blob(NULL)");
	failures += tb_expect_errno(coex_cd_update_coex_params_blob(blob, 0U), -EINVAL,
				    "update_coex_params_blob(len=0)");
	failures += tb_expect_errno(coex_cd_update_coex_params_blob(
					    blob, COEX_TB_COEX_PARAMS_BLOB_MAX + 1U),
				    -EINVAL, "update_coex_params_blob(len>cap)");

	/*
	 * Out-of-range enums. The driver validates the antenna configuration
	 * before touching hardware, so this must not disturb the COEXC tables
	 * programmed by the previous step.
	 */
	failures += tb_expect_errno(
		coex_cd_configure_COEXC((enum coex_antenna_cfg_type)bad_enum_value), -EINVAL,
		"configure_COEXC(bad cfg)");
	failures += tb_expect_errno(
		coex_cd_sr_power_notify((enum coex_sr_power_event_t)bad_enum_value), -EINVAL,
		"sr_power_notify(bad event)");
	failures += tb_expect_errno(
		coex_cd_wifi_power_notify((enum coex_wifi_power_event_t)bad_enum_value),
		-EINVAL, "wifi_power_notify(bad event)");

#ifdef COEX_DRIVER_POST_PHASE_1
	/*
	 * sr_activity_action is an unsigned int on the wire, so a value outside
	 * the START/END enum is representable and must be rejected. Checked
	 * before the "is coexistence running" gate, so it fails with -EINVAL
	 * rather than -EACCES.
	 */
	coex_tb_params_sr_activity(&activity, COEX_TB_SR_ACTIVITY_BAD_ACTION);
	failures += tb_expect_errno(coex_cd_update_short_range_activity_info(&activity), -EINVAL,
				    "sr_activity_info(bad action)");

	/*
	 * A well-formed activity is still refused while coexistence is not
	 * running, because neither radio has reported itself powered up yet.
	 */
	coex_tb_params_sr_activity(&activity, SR_ACTIVITY_START);
	failures += tb_expect_errno(coex_cd_update_short_range_activity_info(&activity), -EACCES,
				    "sr_activity_info(START before bring-up)");

	/* The SR software-client path is a documented stub in this driver variant. */
	failures += tb_expect_errno(
		coex_cd_sr_software_client_request(&sr_client, &grant_status), -ENOTSUP,
		"sr_software_client_request()");
#endif /* COEX_DRIVER_POST_PHASE_1 */

	/*
	 * No statistics have been requested yet, so the cache must be empty.
	 * This relies on running before the GET_STATS step, which is last.
	 */
	failures += tb_expect_true(coex_cd_get_last_stats() == NULL,
				   "get_last_stats() is NULL before first GET_STATS");
	failures += tb_expect_true(coex_cd_get_last_patch_stats() == NULL,
				   "get_last_patch_stats() is NULL before first GET_STATS");

	printk("  ---- %u argument checks failed ----\n", failures);

	return (failures == 0U) ? 0 : -EIO;
}

/**
 * Tell the driver both radios are powered up and ready.
 *
 * The Wi-Fi notification is what triggers the driver's full CM bring-up
 * sequence: priority ranges, user params, coex params, and finally enable, each
 * waiting for its completion event. The SR notification then opens the runtime
 * gate that the SR activity API checks.
 *
 * When this step is disabled, main() performs Wi-Fi CM bring-up automatically
 * before the test runner starts (see coex_tb_prepare()).
 */
static int tb_init_and_enable(void)
{
	int ret;

	printk("\n=================== init: Wi-Fi and SR power notify\n");

	ret = coex_cd_wifi_power_notify(COEX_WIFI_POWERED_UP_READY);
	if (ret != 0) {
		printk("Coex TB: Wi-Fi power notify failed: %s (%d)\n", coex_tb_errstr(ret), ret);
		return ret;
	}

	ret = coex_cd_sr_power_notify(COEX_SR_POWERED_UP_READY);
	if (ret != 0) {
		printk("Coex TB: SR power notify failed: %s (%d)\n", coex_tb_errstr(ret), ret);
		return ret;
	}

	printk("Coex TB: radios reported ready\n");
	return 0;
}

/** Send Wi-Fi and SR PTI priority ranges (CD2CM_SET_PRIORITY_RANGES). */
static int tb_set_priority_ranges(void)
{
	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;

	printk("\n============================================= set priority ranges\n");
	coex_tb_params_priority_ranges(&wifi_range, &sr_range);

	/*
	 * On success the driver also retains these ranges and mirrors the SR
	 * half into the SR driver, so a later SR power-up replays these values
	 * rather than the build-time defaults.
	 */
	return coex_cd_set_priority_ranges(&wifi_range, &sr_range);
}

/** Send protection probabilities and antenna mode (CD2CM_UPDATE_COEX_USER_PARAMS). */
static int tb_update_coex_user_parameters(void)
{
	struct coex_user_params_t user_params;

	printk("\n================================================== update coex user params\n");
	coex_tb_params_user_params(&user_params);

	return coex_cd_update_user_params(&user_params);
}

/** Send the built-in parameter blob (CD2CM_UPDATE_COEX_PARAMS). */
static int tb_update_coex_parameters(void)
{
	printk("\n================================================== update coex params\n");

	return coex_cd_update_coex_params();
}

/**
 * Toggle coexistence off and on inside the CM.
 *
 * Iteration 1 enables, iteration 2 disables, and so on, so an even
 * COEX_TB_NUM_ENABLE_COEX_TESTS exercises both directions.
 *
 * Whatever the loop ends on, coexistence is re-enabled before returning. The
 * steps that follow (antenna allocation, LNA control, PPW generation) are only
 * meaningful with coexistence active, and leaving it disabled here would
 * silently turn them into no-ops.
 */
static int tb_enable_disable_coex(uint32_t num_tests)
{
	int ret;

	printk("\n========================== enable/disable coexistence (%u iterations)\n",
	       num_tests);

	for (uint32_t test = 1U; test <= num_tests; test++) {
		bool enable = ((test % 2U) == 1U);

		printk("\n--- COEX %s test %u ---\n", enable ? "ENABLE" : "DISABLE", test);
		ret = coex_cd_enable(enable);
		if (ret != 0) {
			return ret;
		}
	}

	printk("\n--- COEX ENABLE (restore known state for following tests) ---\n");
	return coex_cd_enable(true);
}


static int tb_enable_disable_coex_default(void)
{
	return tb_enable_disable_coex(COEX_TB_NUM_ENABLE_COEX_TESTS);
}

/**
 *  Shared-antenna allocation modes.
 *
 * The modes are listed in enum declaration order so the array can be checked
 * against antenna_allocation_t at a glance. DYNAMIC is restored explicitly 
  * afterwards so that steps that follow are only meaningful while the CM
 * is actually arbitrating.
 */
static int tb_force_antenna_allocation(void)
{
	static const enum antenna_allocation_t modes[] = {
		ANT_ALLOC_DYNAMIC,
		ANT_ALLOC_STATIC_WIFI,
		ANT_ALLOC_STATIC_SR,
	};
	static const char *const labels[] = {"DYNAMIC", "STATIC_WIFI", "STATIC_SR"};
	struct coex_user_params_t user_params;
	int ret;

	BUILD_ASSERT(ARRAY_SIZE(modes) == ARRAY_SIZE(labels),
		     "antenna mode and label arrays must stay in step");

	printk("\n========================== force shared antenna allocation\n");

	for (size_t i = 0U; i < ARRAY_SIZE(modes); i++) {
		printk("--- FORCE_ANTENNA_ALLOC: %s ---\n", labels[i]);
		coex_tb_params_user_params_ant_alloc(&user_params, modes[i]);

		ret = coex_cd_update_user_params(&user_params);
		if (ret != 0) {
			return ret;
		}
	}

	printk("--- FORCE_ANTENNA_ALLOC: DYNAMIC (restore for following tests) ---\n");
	coex_tb_params_user_params_ant_alloc(&user_params, ANT_ALLOC_DYNAMIC);

	return coex_cd_update_user_params(&user_params);
}

/**
 * Shared-LNA-switch control modes.
 *
 * The LNA (low-noise amplifier) switch is part of the shared receive path. Its
 * control byte lives inside the opaque coexistence parameter blob, so each
 * iteration rebuilds the default blob with that one byte replaced.
 *
 * The blob has no struct definition in the firmware interface headers, so the
 * patched offset cannot be verified at compile time. The before/after bytes are
 * printed to make it checkable on hardware; see COEX_TB_BLOB_LNASW_OFFSET.
 */
static int tb_control_shared_lna(void)
{
	static const uint8_t lna_modes[] = {0U, 1U, 2U};
	static const char *const labels[] = {"SHA_LNA_DYNAMIC", "SHA_LNA_STATIC_EN",
					     "SHA_LNA_STATIC_DIS"};
	uint8_t blob[COEX_TB_COEX_PARAMS_BLOB_MAX];
	uint8_t original;
	size_t blob_len;
	int ret;

	printk("\n============================ force shared LNA switch\n");

	for (size_t i = 0U; i < ARRAY_SIZE(lna_modes); i++) {
		printk("--- FORCE_LNASW: %s ---\n", labels[i]);

		ret = coex_tb_params_coex_blob_lna(lna_modes[i], blob, sizeof(blob), &blob_len,
						   &original);
		if (ret != 0) {
			printk("Coex TB: LNA blob build failed: %s (%d)\n", coex_tb_errstr(ret),
			       ret);
			return ret;
		}

		printk("    blob_len=%u, byte[%u]: 0x%02x -> 0x%02x\n", (unsigned int)blob_len,
		       COEX_TB_BLOB_LNASW_OFFSET, original, lna_modes[i]);

		ret = coex_cd_update_coex_params_blob(blob, blob_len);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

/**
 * Compare a computed BLE channel map against the expected five octets.
 *
 * @return 1 if the maps differ, 0 if they match.
 */
static unsigned int tb_expect_chan_map(const uint8_t *actual, const uint8_t *expected,
				       const char *what)
{
	bool pass = (memcmp(actual, expected, COEX_BLE_CHAN_MAP_SIZE) == 0);

	printk("  %-40s got %02x %02x %02x %02x %02x, want %02x %02x %02x %02x %02x : %s\n", what,
	       actual[0], actual[1], actual[2], actual[3], actual[4], expected[0], expected[1],
	       expected[2], expected[3], expected[4], pass ? "PASS" : "FAIL");

	return pass ? 0U : 1U;
}

/**
 * Map connected-state Wi-Fi channels onto the BLE data-channel classification.
 *
 * The 2.4 GHz case is the worked example carried by both specifications: Wi-Fi
 * channel 6 at 20 MHz with a 3 MHz guard band guards 2424 to 2450 MHz, which
 * overlaps BLE data channels 10 through 22 and leaves the rest available. The
 * 5 GHz and 6 GHz cases must leave all 37 data channels available, because
 * those bands do not reach into the BLE band at all.
 *
 * Computation is checked through coex_cd_ble_chan_map_from_wifi(), which does
 * not need a Bluetooth Controller. coex_cd_wifi_channel_notify() is exercised
 * separately to cover the one-second minimum interval between updates.
 */
static int tb_ble_chan_map(void)
{
	/* Channels 10 to 22 bad, bits 37 to 39 clear. */
	static const uint8_t expected_ch6[COEX_BLE_CHAN_MAP_SIZE] = {0xFF, 0x03, 0x80, 0xFF, 0x1F};
	/* All 37 data channels available, bits 37 to 39 clear. */
	static const uint8_t expected_all[COEX_BLE_CHAN_MAP_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
	struct coex_wifi_channel_info_t channel;
	uint8_t chan_map[COEX_BLE_CHAN_MAP_SIZE];
	unsigned int failures = 0U;
	int ret;

	printk("\n=================== BLE bad-channel mapping\n");

	/* Wi-Fi channel 6, 20 MHz, 3 MHz guard band. */
	coex_tb_params_wifi_channel(&channel, COEX_WIFI_BAND_2_4_GHZ, COEX_TB_WIFI_CH6_CENTER_MHZ,
				    COEX_TB_WIFI_CH6_BANDWIDTH_MHZ, COEX_TB_WIFI_CH6_GUARD_MHZ);
	failures += tb_expect_errno(coex_cd_ble_chan_map_from_wifi(&channel, chan_map), 0,
				    "chan_map_from_wifi(2.4 GHz ch6)");
	failures += tb_expect_chan_map(chan_map, expected_ch6, "2.4 GHz ch6 map");

	/* 5 GHz does not reach the BLE band. */
	coex_tb_params_wifi_channel(&channel, COEX_WIFI_BAND_5_GHZ, COEX_TB_WIFI_5G_CENTER_MHZ,
				    COEX_TB_WIFI_5G_BANDWIDTH_MHZ, COEX_TB_WIFI_CH6_GUARD_MHZ);
	failures += tb_expect_errno(coex_cd_ble_chan_map_from_wifi(&channel, chan_map), 0,
				    "chan_map_from_wifi(5 GHz)");
	failures += tb_expect_chan_map(chan_map, expected_all, "5 GHz map");

	/* Neither does 6 GHz. */
	coex_tb_params_wifi_channel(&channel, COEX_WIFI_BAND_6_GHZ, COEX_TB_WIFI_6G_CENTER_MHZ,
				    COEX_TB_WIFI_5G_BANDWIDTH_MHZ, COEX_TB_WIFI_CH6_GUARD_MHZ);
	failures += tb_expect_errno(coex_cd_ble_chan_map_from_wifi(&channel, chan_map), 0,
				    "chan_map_from_wifi(6 GHz)");
	failures += tb_expect_chan_map(chan_map, expected_all, "6 GHz map");

	/* Reserved bits 37 to 39 must never be set, whatever the input. */
	failures += tb_expect_true((chan_map[COEX_BLE_CHAN_MAP_SIZE - 1U] & 0xE0U) == 0U,
				   "reserved bits 37-39 clear");

	/* Rejections: bad pointer, bad band, zero bandwidth, oversized guard band. */
	failures += tb_expect_errno(coex_cd_ble_chan_map_from_wifi(&channel, NULL), -EINVAL,
				    "chan_map_from_wifi(NULL map)");
	failures += tb_expect_errno(coex_cd_ble_chan_map_from_wifi(NULL, chan_map), -EINVAL,
				    "chan_map_from_wifi(NULL info)");

	coex_tb_params_wifi_channel(&channel, COEX_TB_WIFI_BAD_BAND, COEX_TB_WIFI_CH6_CENTER_MHZ,
				    COEX_TB_WIFI_CH6_BANDWIDTH_MHZ, COEX_TB_WIFI_CH6_GUARD_MHZ);
	failures += tb_expect_errno(coex_cd_ble_chan_map_from_wifi(&channel, chan_map), -EINVAL,
				    "chan_map_from_wifi(bad band)");

	coex_tb_params_wifi_channel(&channel, COEX_WIFI_BAND_2_4_GHZ, COEX_TB_WIFI_CH6_CENTER_MHZ,
				    0U, COEX_TB_WIFI_CH6_GUARD_MHZ);
	failures += tb_expect_errno(coex_cd_ble_chan_map_from_wifi(&channel, chan_map), -EINVAL,
				    "chan_map_from_wifi(zero bandwidth)");

	coex_tb_params_wifi_channel(&channel, COEX_WIFI_BAND_2_4_GHZ, COEX_TB_WIFI_CH6_CENTER_MHZ,
				    COEX_TB_WIFI_CH6_BANDWIDTH_MHZ, COEX_TB_WIFI_BAD_GUARD_MHZ);
	failures += tb_expect_errno(coex_cd_ble_chan_map_from_wifi(&channel, chan_map), -EINVAL,
				    "chan_map_from_wifi(guard band too wide)");

	/*
	 * A bandwidth wide enough to cover the whole BLE band would leave fewer
	 * than two data channels available, which the driver must refuse rather
	 * than hand to the Controller.
	 */
	coex_tb_params_wifi_channel(&channel, COEX_WIFI_BAND_2_4_GHZ, COEX_TB_WIFI_CH6_CENTER_MHZ,
				    200U, 0U);
	failures += tb_expect_errno(coex_cd_ble_chan_map_from_wifi(&channel, chan_map), -EINVAL,
				    "chan_map_from_wifi(too few channels left)");

	/*
	 * Apply the channel 6 classification for real. bt_le_set_chan_map()
	 * needs a Controller that supports Set Host Channel Classification, so
	 * a failure here is reported but does not fail the step; what is being
	 * checked is that the driver reaches the Bluetooth host and enforces the
	 * one-second minimum interval between successive updates.
	 */
	coex_tb_params_wifi_channel(&channel, COEX_WIFI_BAND_2_4_GHZ, COEX_TB_WIFI_CH6_CENTER_MHZ,
				    COEX_TB_WIFI_CH6_BANDWIDTH_MHZ, COEX_TB_WIFI_CH6_GUARD_MHZ);
	ret = coex_cd_wifi_channel_notify(&channel);
	printk("  %-54s got %s (%d)\n", "wifi_channel_notify(2.4 GHz ch6)",
	       coex_tb_errstr(ret), ret);

	if (ret == 0) {
		const uint8_t *applied = coex_cd_get_last_ble_chan_map();

		failures += tb_expect_true(applied != NULL, "get_last_ble_chan_map() retained");
		if (applied != NULL) {
			failures += tb_expect_chan_map(applied, expected_ch6, "applied map");
		}

		/* A second update inside one second must be refused. */
		failures += tb_expect_errno(coex_cd_wifi_channel_notify(&channel), -EBUSY,
					    "wifi_channel_notify(before 1 s elapsed)");

		/* After the interval has elapsed the same update is accepted again. */
		coex_tb_wait_ms(COEX_BLE_CHAN_MAP_MIN_INTERVAL_MS + COEX_TB_CMD_WAIT_MS);
		failures += tb_expect_errno(coex_cd_wifi_channel_notify(&channel), 0,
					    "wifi_channel_notify(after 1 s elapsed)");
	} else {
		printk("  Bluetooth Controller did not accept the classification; "
		       "skipping rate-limit checks\n");
	}

	printk("  ---- %u BLE channel map checks failed ----\n", failures);

	return (failures == 0U) ? 0 : -EIO;
}

#ifdef COEX_DRIVER_POST_PHASE_1

/**
 * Start and stop Periodic Priority Windows directly (CD2CM_ALLOCATE_PPW).
 *
 * This calls the PPW API directly, which is not gated on radio power state.
 * tb_sr_activity_info() covers the gated path that the real SR driver uses.
 */
static int tb_generate_ppws(void)
{
	struct coex_ppw_parameters_t ppw;
	int ret;

	printk("\n================================================== generate PPWs\n");

	for (uint32_t case_idx = 0U; case_idx < 2U; case_idx++) {
		bool first_wifi = (case_idx == 0U);

		printk("--- PPW start (%s first window) ---\n", first_wifi ? "Wi-Fi" : "SR");
		coex_tb_params_ppw_start(&ppw, first_wifi);

		ret = coex_cd_allocate_ppw(&ppw);
		if (ret != 0) {
			return ret;
		}

		/* Let the CM actually generate windows before stopping it. */
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
 * Drive PPWs through the SR activity API.
 *
 * Unlike tb_generate_ppws(), this path is gated: it only works while both
 * radios are up, which is why the step runs after tb_init_and_enable().
 */
static int tb_sr_activity_info(void)
{
	struct short_range_activity_info_t activity;
	int ret;

	printk("\n=========================== SR activity info (gated PPW path)\n");

	printk("--- SR_ACTIVITY_START (duration=%u ms, interval=%u ms, timeout=%u ms) ---\n",
	       COEX_TB_SR_ACTIVITY_DURATION_MS, COEX_TB_SR_ACTIVITY_INTERVAL_MS,
	       COEX_TB_SR_ACTIVITY_TIMEOUT_MS);

	coex_tb_params_sr_activity(&activity, SR_ACTIVITY_START);
	ret = coex_cd_update_short_range_activity_info(&activity);
	if (ret != 0) {
		printk("Coex TB: SR activity start failed: %s (%d)\n", coex_tb_errstr(ret), ret);
		return ret;
	}

	coex_tb_wait_ms(COEX_TB_PPW_TOTAL_DURATION_MS);

	printk("--- SR_ACTIVITY_END ---\n");
	coex_tb_params_sr_activity(&activity, SR_ACTIVITY_END);
	ret = coex_cd_update_short_range_activity_info(&activity);
	if (ret != 0) {
		printk("Coex TB: SR activity end failed: %s (%d)\n", coex_tb_errstr(ret), ret);
		return ret;
	}

	coex_tb_wait_ms(COEX_TB_PPW_POST_WAIT_MS);

	return 0;
}

/**
 * Request and release antenna access as a Wi-Fi software client.
 *
 * The protection probability is swept between 100 and 0 percent so the CM's
 * arbitration is driven both ways: at 100 the Wi-Fi activity is always
 * protected from SR, at 0 it never is. Each request is followed by an explicit
 * release, so the request/release counters must balance at the end of the run.
 */
static int tb_wifi_sw_client_request(void)
{
	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;
	struct coex_user_params_t user_params;
	struct coex_sw_client_params_t sw_params;
	int ret;

	printk("\n================= Wi-Fi SW client request/release\n");

	/* Known starting state: coexistence on, ranges as this bench expects. */
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

		/*
		 * Long timeout: the release below should happen first, so this
		 * pair lands in the "explicit release" counters rather than the
		 * timeout counters.
		 */
		coex_tb_params_wifi_sw_client(&sw_params, WIFI_SW_CLIENT_REQUEST, 30U * 1000U,
					      COEXC_MODE_WIFI_2PT4G);
		ret = coex_cd_wifi_sw_client_request(&sw_params);
		if (ret != 0) {
			return ret;
		}

		coex_tb_params_wifi_sw_client(&sw_params, WIFI_SW_CLIENT_RELEASE, 0U,
					      COEXC_MODE_WIFI_2PT4G);
		ret = coex_cd_wifi_sw_client_request(&sw_params);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

/**
 * Let Wi-Fi software client requests expire instead of releasing them.
 *
 * A short 50 ms timeout is used and no release is sent, so the CM must release
 * each grant itself. Iterations alternate between the 2.4 GHz and 5 GHz bands,
 * because only 2.4 GHz actually contends with SR: 5 GHz requests should be
 * granted without arbitration.
 *
 * The final wait is longer than the request timeout so every grant has expired
 * before the statistics are read, otherwise the request/release identity checks
 * would see a request still outstanding.
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

	/* Make sure the last grant has timed out before statistics are read. */
	coex_tb_wait_ms(COEX_TB_PPW_POST_WAIT_MS);

	return 0;
}

#endif /* COEX_DRIVER_POST_PHASE_1 */

/**
 * Read the CM counters back and cross-check them.
 *
 * coex_cd_get_stats() posts the request and waits for the statistics event; the
 * driver stores the payload as a side effect of receiving that event, and the
 * two get_last_* accessors then read the cached copy.
 *
 * A failing consistency check fails this step, so a run that produced
 * inconsistent CM bookkeeping cannot report an overall pass.
 */
static int tb_get_coex_manager_stats(void)
{
	unsigned int failures;
	int ret;

	printk("\n========================================================== get CM stats\n");

	ret = coex_cd_get_stats();
	if (ret != 0) {
		printk("Coex TB: get stats failed: %s (%d)\n", coex_tb_errstr(ret), ret);
		return ret;
	}

	failures = coex_tb_print_and_validate_stats(coex_cd_get_last_stats(),
						    coex_cd_get_last_patch_stats());
	if (failures != 0U) {
		printk("Coex TB: %u statistics consistency check(s) FAILED\n", failures);
		return -EIO;
	}

	return 0;
}

/* ---- Test runner ---- */

/** One entry per test step, so the runner can name the step that failed. */
struct tb_step {
	const char *name;
	int (*fn)(void);
};

/*
 * Steps run top to bottom. The order encodes real dependencies:
 * hardware tables, then argument validation while coexistence is still down,
 * then radio bring-up, then runtime commands, then the statistics read-back.
 */
static const struct tb_step tb_steps[] = {
#ifdef COEX_TB_TEST_CONFIGURE_ENABLE_COEXC
	{"configure COEXC", tb_configure_coexc},
#endif
#ifdef COEX_TB_TEST_NEGATIVE_ARGS
	{"argument validation", tb_negative_args},
#endif
#ifdef COEX_TB_TEST_INIT_AND_ENABLE
	{"radio power notify", tb_init_and_enable},
#endif
#ifdef COEX_TB_TEST_SET_PRIORITY_RANGES
	{"set priority ranges", tb_set_priority_ranges},
#endif
#ifdef COEX_TB_TEST_UPDATE_COEX_USER_PARAMS
	{"update coex user params", tb_update_coex_user_parameters},
#endif
#ifdef COEX_TB_TEST_UPDATE_COEX_PARAMETERS
	{"update coex params", tb_update_coex_parameters},
#endif
#ifdef COEX_TB_TEST_ENABLE_COEXISTENCE
	{"enable/disable coexistence", tb_enable_disable_coex_default},
#endif
#ifdef COEX_TB_TEST_FORCE_ANTENNA_ALLOC
	{"force antenna allocation", tb_force_antenna_allocation},
#endif
#ifdef COEX_TB_TEST_FORCE_LNASW_ALLOC
	{"force shared LNA switch", tb_control_shared_lna},
#endif
#ifdef COEX_TB_TEST_BLE_CHAN_MAP
	{"BLE bad-channel mapping", tb_ble_chan_map},
#endif
#ifdef COEX_TB_TEST_GENERATE_PPW
	{"generate PPWs", tb_generate_ppws},
#endif
#ifdef COEX_TB_TEST_SR_ACTIVITY_INFO
	{"SR activity info", tb_sr_activity_info},
#endif
#ifdef COEX_TB_TEST_WIFI_SW_CLIENT_REQUEST
	{"Wi-Fi SW client request", tb_wifi_sw_client_request},
#endif
#ifdef COEX_TB_TEST_WIFI_SW_CLIENT_TIMEOUT
	{"Wi-Fi SW client timeout", tb_wifi_sw_client_timeout},
#endif
#ifdef COEX_TB_TEST_GET_CM_STATS
	{"get CM stats", tb_get_coex_manager_stats},
#endif
};

int coex_tb_run_all(void)
{
	const char *failed_step = NULL;
	int ret = 0;

	printk("\n\n========== nRF71 SR coexistence driver test bench start ==========\n");
	printk("Running %u enabled test step(s)\n", (unsigned int)ARRAY_SIZE(tb_steps));

	for (size_t i = 0U; i < ARRAY_SIZE(tb_steps); i++) {
		ret = tb_steps[i].fn();
		if (ret != 0) {
			failed_step = tb_steps[i].name;
			break;
		}
	}

	coex_tb_wait_ms(COEX_TB_FINAL_WAIT_MS);

	if (ret == 0) {
		printk("\n========== nRF71 SR coexistence driver test bench PASS ==========\n");
	} else {
		printk("\n========== nRF71 SR coexistence driver test bench FAIL: %s: %s "
		       "(%d) =====\n",
		       (failed_step != NULL) ? failed_step : "unknown step", coex_tb_errstr(ret),
		       ret);
	}

	return ret;
}
