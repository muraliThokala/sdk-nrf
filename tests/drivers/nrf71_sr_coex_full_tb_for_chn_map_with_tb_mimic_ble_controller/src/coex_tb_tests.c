/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence Driver (CD) test steps and the top-level runner.
 *
 * Each tb_* function below is one test step against a coexistence driver API.
 * Most steps drive a CD2CM command and its completion event, but several cover
 * driver behaviour with no CM involvement at all: COEXC register programming,
 * radio power notifications from the SR driver, and argument rejection.
 *
 * A step returns 0 when everything it tried was accepted, or a negative errno on
 * the first thing that was not. The runner at the bottom of the file executes
 * the enabled steps in order and stops at the first failure, so later steps can
 * safely assume that earlier ones succeeded.
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
#include <nrf71_coex_if.h>
#include <nrf71_cd_sr_if.h>
#include <drivers/nrf71_sr_coex/nrf71_sr_coex.h>
#include <drivers/nrf71_sr_coex/nrf71_sr_coex_sr.h>

#include "nrf71_sr_coex_internal.h"

#include "coex_tb.h"
#ifdef CONFIG_COEX_TB_MOCK_BT_CHAN_MAP
#include "coex_tb_bt_mock.h"
#endif
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

/* ---- Short-Range driver callback stubs ---- */

/*
 * Stand-in for the SR driver: records what the CD asks of it so the steps
 * below can check that the callbacks are actually invoked.
 */
static struct {
	unsigned int enable_calls;
	bool last_enable;
	unsigned int set_priority_calls;
	struct nrf71_sr_coex_sr_prio last_prio;
} tb_sr_cb_state;

static int tb_sr_enable(bool enable)
{
	tb_sr_cb_state.enable_calls++;
	tb_sr_cb_state.last_enable = enable;
	return 0;
}

static int tb_sr_set_priority(const struct nrf71_sr_coex_sr_prio *prio)
{
	tb_sr_cb_state.set_priority_calls++;
	tb_sr_cb_state.last_prio = *prio;
	return 0;
}

static const struct nrf71_sr_coex_sr_driver_cb tb_sr_driver_cb = {
	.enable = tb_sr_enable,
	.set_priority = tb_sr_set_priority,
};

/*
 * Registered from SYS_INIT, as the SR driver does, so the callbacks are in
 * place before the Wi-Fi driver can trigger CM bring-up.
 */
static int tb_sr_driver_cb_init(void)
{
	int ret = nrf71_sr_coex_sr_register(&tb_sr_driver_cb);

	if (ret != 0) {
		printk("Coex TB: SR driver callback registration failed: %s (%d)\n",
		       coex_tb_errstr(ret), ret);
	}

	return 0;
}

SYS_INIT(tb_sr_driver_cb_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#ifdef COEX_TB_TEST_CONFIGURE_COEXC
/** Program the COEXC arbitration tables for the shared-antenna configuration. */
static int tb_configure_coexc(void)
{
	cd_coexc_configure(NRF71_SR_COEX_ANTENNA_SHARED);

	return 0;
}
#endif /* COEX_TB_TEST_CONFIGURE_COEXC */

/**
 * Confirm the driver rejects bad input instead of forwarding it to the CM.
 *
 * This runs before the SR radio is reported up. None of these calls should
 * reach the RPU, so none of them disturb CM state or the counters checked at
 * the end of the run. The rejected nrf71_sr_coex_init() calls do not count as
 * the single application configuration, which stays available for later steps.
 */
static int tb_negative_args(void)
{
	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;
	struct nrf71_sr_coex_prio prio;
	struct nrf71_sr_coex_user_params user;
	struct nrf71_sr_coex_cfg cfg;
	uint8_t blob[COEX_TB_COEX_PARAMS_BLOB_MAX + 1U] = {0};
	unsigned int failures = 0U;
	int bad_enum_value = 2; /* Not a valid antenna, SR power event or allocation. */

	printk("\n=================== argument validation (nothing reaches the CM)\n");

	coex_tb_params_priority_ranges(&wifi_range, &sr_range);

	/* NULL pointers must be refused by the driver, not dereferenced. */
	failures += tb_expect_errno(coex_cm_set_priority_ranges(NULL, &sr_range), -EINVAL,
				    "set_priority_ranges(NULL wifi)");
	failures += tb_expect_errno(coex_cm_set_priority_ranges(&wifi_range, NULL), -EINVAL,
				    "set_priority_ranges(NULL sr)");
	failures += tb_expect_errno(coex_cm_update_user_params(NULL), -EINVAL,
				    "update_user_params(NULL)");

	/* Blob length must be non-zero and within the driver's internal cap. */
	failures += tb_expect_errno(coex_cm_update_coex_params_blob(NULL, 16U), -EINVAL,
				    "update_coex_params_blob(NULL)");
	failures += tb_expect_errno(coex_cm_update_coex_params_blob(blob, 0U), -EINVAL,
				    "update_coex_params_blob(len=0)");
	failures += tb_expect_errno(coex_cm_update_coex_params_blob(
					    blob, COEX_TB_COEX_PARAMS_BLOB_MAX + 1U),
				    -EINVAL, "update_coex_params_blob(len>cap)");

	/*
	 * The application configuration is validated as a whole before anything is
	 * stored or the COEXC tables are touched, so none of these disturb the
	 * tables programmed by the previous step.
	 */
	failures += tb_expect_errno(nrf71_sr_coex_init(NULL), -EINVAL, "init(NULL)");

	cfg = (struct nrf71_sr_coex_cfg){
		.antenna = (enum nrf71_sr_coex_antenna)bad_enum_value,
	};
	failures += tb_expect_errno(nrf71_sr_coex_init(&cfg), -EINVAL, "init(bad antenna)");

	cfg = (struct nrf71_sr_coex_cfg){
		.antenna = NRF71_SR_COEX_ANTENNA_SHARED,
		.coex_params = blob,
		.coex_params_len = 0U,
	};
	failures += tb_expect_errno(nrf71_sr_coex_init(&cfg), -EINVAL,
				    "init(coex_params len=0)");

	cfg.coex_params_len = COEX_TB_COEX_PARAMS_BLOB_MAX + 1U;
	failures += tb_expect_errno(nrf71_sr_coex_init(&cfg), -EINVAL,
				    "init(coex_params len>cap)");

	coex_tb_params_prio(&prio);
	prio.sr.tx.step = 0U;
	cfg = (struct nrf71_sr_coex_cfg){
		.antenna = NRF71_SR_COEX_ANTENNA_SHARED,
		.prio = &prio,
	};
	failures += tb_expect_errno(nrf71_sr_coex_init(&cfg), -EINVAL, "init(prio step=0)");

	coex_tb_params_prio(&prio);
	prio.wifi.hp_rx.start = 0U;
	failures += tb_expect_errno(nrf71_sr_coex_init(&cfg), -EINVAL,
				    "init(prio start<end)");

	coex_tb_params_user(&user);
	user.wifi_conn_prot = 101U;
	cfg = (struct nrf71_sr_coex_cfg){
		.antenna = NRF71_SR_COEX_ANTENNA_SHARED,
		.user = &user,
	};
	failures += tb_expect_errno(nrf71_sr_coex_init(&cfg), -EINVAL,
				    "init(protection > 100%)");

	coex_tb_params_user(&user);
	user.ant_alloc = (enum nrf71_sr_coex_ant_alloc)(bad_enum_value + 1);
	failures += tb_expect_errno(nrf71_sr_coex_init(&cfg), -EINVAL, "init(bad ant_alloc)");

	failures += tb_expect_errno(
		nrf71_sr_coex_sr_power_notify((enum nrf71_sr_coex_sr_power_event)bad_enum_value),
		-EINVAL, "sr_power_notify(bad event)");

	failures += tb_expect_errno(nrf71_sr_coex_sr_register(NULL), -EINVAL,
				    "sr_register(NULL)");
	failures += tb_expect_errno(nrf71_sr_coex_sr_register(&tb_sr_driver_cb), -EALREADY,
				    "sr_register() twice");

	printk("  ---- %u argument checks failed ----\n", failures);

	return (failures == 0U) ? 0 : -EIO;
}

/**
 * Wait for the CM configuration and report the SR radio as powered up.
 *
 * The driver runs its full CM bring-up sequence by itself as soon as the Wi-Fi
 * interface is up: priority ranges, user params, coex params and finally enable,
 * each waiting for its completion event. The SR notification then enables
 * coexistence in the SR driver through its registered callback.
 *
 * When this step is disabled, main() waits for the CM configuration before the
 * test runner starts (see coex_tb_prepare()).
 */
static int tb_init_and_enable(void)
{
	unsigned int enable_calls;
	int ret;

	ret = coex_tb_wait_cm_active(COEX_TB_CM_BRINGUP_TIMEOUT_MS);
	if (ret != 0) {
		printk("Coex TB: CM bring-up failed: %s (%d)\n", coex_tb_errstr(ret), ret);
		return ret;
	}

	printk("Coex TB: CM configured on Wi-Fi up\n");

	enable_calls = tb_sr_cb_state.enable_calls;
	ret = nrf71_sr_coex_sr_power_notify(NRF71_SR_COEX_SR_POWERED_UP_READY);
	if (ret != 0) {
		printk("Coex TB: SR power notify failed: %s (%d)\n", coex_tb_errstr(ret), ret);
		return ret;
	}

	if ((tb_sr_cb_state.enable_calls == enable_calls) || !tb_sr_cb_state.last_enable) {
		printk("Coex TB: SR driver enable callback not invoked on power-up\n");
		return -EIO;
	}

	printk("Coex TB: SR power notify accepted\n");
	printk("Coex TB: radios reported ready\n");
	return 0;
}

#ifdef COEX_TB_TEST_STORE_BEFORE_UP
/**
 * Check that a configuration set while Wi-Fi is down is kept and sent by the bring-up.
 *
 * The Wi-Fi interface is taken down, the priority ranges and user params are
 * handed to nrf71_sr_coex_init(), and the interface is brought up again. While
 * down, the configuration must be accepted without sending anything (so the SR
 * driver is not touched) and CM commands must be refused. The bring-up must then
 * send the stored values, which shows up as the SR driver receiving the stored
 * SR ranges.
 *
 * The CM is configured again from scratch by the bring-up, so this runs before
 * the steps that set their own values.
 */
static int tb_store_before_up(void)
{
	struct nrf71_sr_coex_prio prio;
	struct nrf71_sr_coex_user_params user;
	struct nrf71_sr_coex_cfg cfg = {
		.antenna = NRF71_SR_COEX_ANTENNA_SHARED,
		.prio = &prio,
		.user = &user,
	};
	unsigned int set_priority_calls;
	unsigned int failures = 0U;
	int ret;

	printk("\n=================== settings stored while Wi-Fi is down\n");

	coex_tb_params_prio(&prio);
	coex_tb_params_user(&user);

	ret = coex_tb_wifi_iface_down();
	if (ret != 0) {
		return ret;
	}

	memset(&tb_sr_cb_state.last_prio, 0, sizeof(tb_sr_cb_state.last_prio));
	set_priority_calls = tb_sr_cb_state.set_priority_calls;

	failures += tb_expect_true(!nrf71_wifi_coex_is_ready(),
				   "Wi-Fi transport down after interface down");
	failures += tb_expect_true(!coex_tb_cm_is_active(), "CM not active after Wi-Fi down");
	failures += tb_expect_errno(nrf71_sr_coex_init(&cfg), 0, "init() while Wi-Fi down");
	failures += tb_expect_errno(nrf71_sr_coex_init(&cfg), -EALREADY, "init() twice");
	failures += tb_expect_errno(coex_cm_enable(true), -ENETDOWN, "enable() while Wi-Fi down");
	failures += tb_expect_true(tb_sr_cb_state.set_priority_calls == set_priority_calls,
				   "nothing mirrored to the SR driver while down");

	/* Always bring Wi-Fi up again, so a failure above does not break later steps. */
	ret = coex_tb_wifi_iface_up();
	if (ret != 0) {
		return ret;
	}

	ret = coex_tb_wait_cm_active(COEX_TB_CM_BRINGUP_TIMEOUT_MS);
	if (ret != 0) {
		printk("Coex TB: CM bring-up after Wi-Fi up failed: %s (%d)\n",
		       coex_tb_errstr(ret), ret);
		return ret;
	}

	failures += tb_expect_true(coex_tb_cm_is_active(), "CM active after Wi-Fi up");
	failures += tb_expect_true(memcmp(&tb_sr_cb_state.last_prio, &prio.sr,
					  sizeof(prio.sr)) == 0,
				   "bring-up sent the stored ranges");

	printk("  ---- %u stored-setting checks failed ----\n", failures);

	return (failures == 0U) ? 0 : -EIO;
}
#endif /* COEX_TB_TEST_STORE_BEFORE_UP */

/** Send Wi-Fi and SR PTI priority ranges (CD2CM_SET_PRIORITY_RANGES). */
static int tb_set_priority_ranges(void)
{
	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;

	/* Get the pre-defined parameter values */
	coex_tb_params_priority_ranges(&wifi_range, &sr_range);

	return coex_cm_set_priority_ranges(&wifi_range, &sr_range);
}

/** Send protection probabilities and antenna mode (CD2CM_UPDATE_COEX_USER_PARAMS). */
static int tb_update_coex_user_parameters(void)
{
	struct coex_user_params_t user_params;

	/* Get the pre-defined user parameters */
	coex_tb_params_user_params(&user_params);

	return coex_cm_update_user_params(&user_params);
}

/** Send NRF_COEX_PARAMS, the built-in parameter blob (CD2CM_UPDATE_COEX_PARAMS). */
static int tb_update_coex_parameters(void)
{
	return coex_cm_update_coex_params();
}

/**
 * Toggle coexistence off and on inside MAC controller via the Coexistence Manager (CM).
 *
 * Iteration 1 enables, iteration 2 disables, and so on, so an even
 * COEX_TB_NUM_ENABLE_COEX_TESTS exercises both directions.
 *
 * Whatever the loop ends on, coexistence is re-enabled before returning. The
 * steps that follow (antenna allocation, LNA control) are only meaningful with
 * coexistence active, and leaving it disabled here would silently turn them
 * into no-ops.
 */
static int tb_enable_disable_coex(uint32_t num_tests)
{
	int ret;

	printk("\n========================== enable/disable coexistence (%u iterations)\n",
	       num_tests);

	for (uint32_t test = 1U; test <= num_tests; test++) {
		bool enable = ((test % 2U) == 1U);

		printk("\n--- COEX %s test %u ---\n", enable ? "ENABLE" : "DISABLE", test);
		ret = coex_cm_enable(enable);
		if (ret != 0) {
			return ret;
		}
	}

	printk("\n--- COEX ENABLE (restore known state for following tests) ---\n");
	return coex_cm_enable(true);
}

/** Run tb_enable_disable_coex() with the iteration count from coex_tb.h. */
static int tb_enable_disable_coex_default(void)
{
	return tb_enable_disable_coex(COEX_TB_NUM_ENABLE_COEX_TESTS);
}

/**
 * Walk the antenna allocation modes.
 *
 * The modes are listed in enum declaration order so the array can be checked
 * against antenna_allocation_t at a glance. DYNAMIC is restored explicitly
 * afterwards, because the steps that follow are only meaningful while the CM
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

		ret = coex_cm_update_user_params(&user_params);
		if (ret != 0) {
			return ret;
		}
	}

	printk("--- FORCE_ANTENNA_ALLOC: DYNAMIC (restore for following tests) ---\n");
	coex_tb_params_user_params_ant_alloc(&user_params, ANT_ALLOC_DYNAMIC);

	return coex_cm_update_user_params(&user_params);
}

/**
 * Walk the shared-LNA-switch control modes.
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

		ret = coex_cm_update_coex_params_blob(blob, blob_len);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

#ifdef COEX_TB_TEST_BLE_CHAN_MAP
BUILD_ASSERT(CONFIG_NRF71_SR_COEX_BLE_GUARD_BAND_MHZ == COEX_TB_WIFI_CH6_GUARD_MHZ,
	     "the expected channel 6 map assumes a 3 MHz guard band");
BUILD_ASSERT(CD_BLE_CHAN_MAP_SIZE == COEX_TB_BLE_CHAN_MAP_SIZE,
	     "BLE channel map size does not match the driver");

/** Time for one map to be applied, including the driver's minimum interval. */
#define TB_BLE_APPLY_WAIT_MS (COEX_TB_BLE_CHAN_MAP_MIN_INTERVAL_MS + COEX_TB_CMD_WAIT_MS)

/**
 * Compare a BLE channel map against the expected five octets.
 *
 * @return 1 if the maps differ, 0 if they match.
 */
static unsigned int tb_expect_chan_map(const uint8_t *actual, const uint8_t *expected,
				       const char *what)
{
	bool pass = (memcmp(actual, expected, COEX_TB_BLE_CHAN_MAP_SIZE) == 0);

	printk("  %-40s got %02x %02x %02x %02x %02x, want %02x %02x %02x %02x %02x : %s\n", what,
	       actual[0], actual[1], actual[2], actual[3], actual[4], expected[0], expected[1],
	       expected[2], expected[3], expected[4], pass ? "PASS" : "FAIL");

	return pass ? 0U : 1U;
}

#ifdef CONFIG_COEX_TB_MOCK_BT_CHAN_MAP
BUILD_ASSERT(COEX_TB_BT_MOCK_MAP_SIZE == COEX_TB_BLE_CHAN_MAP_SIZE,
	     "BLE channel map size of the mock does not match the bench");

/** Short pause so the driver can store the result of a call the mock has just seen. */
#define TB_APPLY_SETTLE_MS 50U

/**
 * Check what the driver hands to the Bluetooth Controller, with
 * bt_le_set_chan_map() replaced by the recording mock.
 *
 * This shows that the map offered to the Controller has the expected content,
 * that a rejected first attempt is retried, that the newest map wins, that the
 * 5 GHz and 6 GHz bands leave every data channel available, that invalid
 * channels never reach the Controller and that a disconnect delivers the default
 * map. Every map the Controller is offered is printed by the mock, and the
 * checks compare it with the expected map.
 *
 * @return Number of failed checks.
 */
static unsigned int tb_ble_apply_mock(const uint8_t *expected_ch6)
{
	/* All 37 data channels available, bits 37 to 39 clear. */
	static const uint8_t expected_all[COEX_TB_BLE_CHAN_MAP_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF,
									 0x1F};
	uint8_t seen[COEX_TB_BT_MOCK_MAP_SIZE];
	uint8_t applied[COEX_TB_BLE_CHAN_MAP_SIZE];
	unsigned int before;
	unsigned int failures = 0U;
	const bool retry_stage = (CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP_RETRIES > 0);
	const uint32_t settle_ms = COEX_TB_BLE_CHAN_MAP_MIN_INTERVAL_MS + 2U * COEX_TB_CMD_WAIT_MS;

	printk("  ---- bt_le_set_chan_map() mocked: checking what reaches the Controller ----\n");
	coex_tb_bt_mock_reset();

	/* Stage 1: the Controller rejects the first attempt, the driver has to retry. */
	if (retry_stage) {
		coex_tb_bt_mock_fail_next(1U, -EAGAIN);
	}
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, COEX_TB_WIFI_CH6_BANDWIDTH_MHZ);

	if (retry_stage) {
		coex_tb_wait_ms(COEX_TB_CMD_WAIT_MS);
		failures += tb_expect_true(coex_tb_bt_mock_calls() == 1U,
					   "first attempt reached the Controller");
		failures += tb_expect_true(coex_tb_bt_mock_accepted() == 0U,
					   "first attempt rejected by the Controller");
		failures += tb_expect_true(!cd_ble_last_chan_map(applied),
					   "rejected map not reported as applied");
	}

	failures += tb_expect_true(coex_tb_bt_mock_wait_accepted(
					   1U, CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP_RETRY_DELAY_MS +
						       2U * COEX_TB_CMD_WAIT_MS),
				   retry_stage ? "map accepted after the retry"
					       : "map accepted by the Controller");
	/* The mock counts the call before the driver stores the result. */
	coex_tb_wait_ms(TB_APPLY_SETTLE_MS);
	if (!coex_tb_bt_mock_last_accepted(seen)) {
		printk("  no map reached the Controller : FAIL\n");
		return failures + 1U;
	}
	if (retry_stage) {
		failures += tb_expect_true(coex_tb_bt_mock_calls() == 2U,
					   "exactly one retry was needed");
	}
	failures += tb_expect_chan_map(seen, expected_ch6, "map delivered to the Controller");

	/* Reserved bits 37 to 39 must never be set. */
	failures += tb_expect_true((seen[COEX_TB_BLE_CHAN_MAP_SIZE - 1U] & 0xE0U) == 0U,
				   "reserved bits 37-39 clear");

	if (!cd_ble_last_chan_map(applied)) {
		printk("  driver reports no applied map : FAIL\n");
		return failures + 1U;
	}
	failures += tb_expect_chan_map(applied, expected_ch6, "map reported by the driver");

	/*
	 * Stage 2: newest map wins. The one second spacing has just started, so the
	 * channel 6 map has to wait, and the 5 GHz map (the default map) queued
	 * after it replaces it. Exactly one more map, the default one, may reach the
	 * Controller.
	 */
	before = coex_tb_bt_mock_accepted();
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, COEX_TB_WIFI_CH6_BANDWIDTH_MHZ);
	cd_ble_wifi_connected(COEX_TB_WIFI_5G_CENTER_MHZ, COEX_TB_WIFI_5G_BANDWIDTH_MHZ);
	coex_tb_wait_ms(settle_ms);
	failures += tb_expect_true(coex_tb_bt_mock_accepted() == before + 1U,
				   "one map reached the Controller, the old one was dropped");
	if (coex_tb_bt_mock_last_accepted(seen)) {
		failures += tb_expect_chan_map(seen, expected_all, "5 GHz map delivered");
	}
	failures += tb_expect_true(!cd_ble_last_chan_map(applied),
				   "default map applied, nothing retained");

	/* Stage 3: 6 GHz does not reach the BLE band either. */
	before = coex_tb_bt_mock_accepted();
	cd_ble_wifi_connected(COEX_TB_WIFI_6G_CENTER_MHZ, COEX_TB_WIFI_5G_BANDWIDTH_MHZ);
	failures += tb_expect_true(coex_tb_bt_mock_wait_accepted(before + 1U, settle_ms),
				   "6 GHz map accepted");
	coex_tb_wait_ms(TB_APPLY_SETTLE_MS);
	if (coex_tb_bt_mock_last_accepted(seen)) {
		failures += tb_expect_chan_map(seen, expected_all, "6 GHz map delivered");
	}

	/*
	 * Stage 4: zero bandwidth, and a bandwidth wide enough to cover the whole
	 * BLE band (fewer than two data channels left), must never reach the
	 * Controller.
	 */
	before = coex_tb_bt_mock_calls();
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, 0U);
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, COEX_TB_WIFI_TOO_WIDE_MHZ);
	coex_tb_wait_ms(settle_ms);
	failures += tb_expect_true(coex_tb_bt_mock_calls() == before,
				   "zero / too wide bandwidth never reach the Controller");

	/* Stage 5: apply channel 6 again, then a disconnect restores the default. */
	before = coex_tb_bt_mock_accepted();
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, COEX_TB_WIFI_CH6_BANDWIDTH_MHZ);
	failures += tb_expect_true(coex_tb_bt_mock_wait_accepted(before + 1U, settle_ms),
				   "channel 6 map accepted again");
	if (coex_tb_bt_mock_last_accepted(seen)) {
		failures += tb_expect_chan_map(seen, expected_ch6, "channel 6 map delivered");
	}

	cd_ble_wifi_disconnected();
	failures += tb_expect_true(coex_tb_bt_mock_wait_accepted(before + 2U, settle_ms),
				   "default map accepted after disconnect");
	coex_tb_wait_ms(TB_APPLY_SETTLE_MS);
	if (coex_tb_bt_mock_last_accepted(seen)) {
		failures += tb_expect_chan_map(seen, expected_all,
					       "default map delivered on disconnect");
	}
	failures += tb_expect_true(!cd_ble_last_chan_map(applied),
				   "default map restored after disconnect");

	return failures;
}
#else /* CONFIG_COEX_TB_MOCK_BT_CHAN_MAP */
/**
 * Apply channel maps through the real Bluetooth Controller.
 *
 * cd_ble_wifi_connected() only queues the map and returns. bt_le_set_chan_map()
 * runs afterwards and needs a Controller that supports Set Host Channel
 * Classification. If the map never shows up as applied, that is reported and the
 * checks that depend on it are skipped; they do not fail the step. Enable
 * CONFIG_COEX_TB_MOCK_BT_CHAN_MAP to check the driver without that dependency.
 *
 * @return Number of failed checks.
 */
static unsigned int tb_ble_apply_real(const uint8_t *expected_ch6)
{
	uint8_t applied[COEX_TB_BLE_CHAN_MAP_SIZE];
	bool have_map;
	unsigned int attempt;
	unsigned int failures = 0U;

	/* Wi-Fi channel 6, 20 MHz. A second update right away is queued as well. */
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, COEX_TB_WIFI_CH6_BANDWIDTH_MHZ);
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, COEX_TB_WIFI_CH6_BANDWIDTH_MHZ);

	/*
	 * Give the work queue time to run the HCI command. If the Controller rejects
	 * the first attempt, the driver repeats it after the retry delay, so keep
	 * waiting through the whole retry window before declaring the map not applied.
	 * This covers the retry path whenever the first attempt fails.
	 */
	coex_tb_wait_ms(COEX_TB_CMD_WAIT_MS);
	have_map = cd_ble_last_chan_map(applied);
	for (attempt = 0U; !have_map && (attempt < CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP_RETRIES);
	     attempt++) {
		printk("  BLE channel map not applied yet, waiting for retry %u\n", attempt + 1U);
		coex_tb_wait_ms(CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP_RETRY_DELAY_MS +
				COEX_TB_CMD_WAIT_MS);
		have_map = cd_ble_last_chan_map(applied);
	}

	if (!have_map) {
		printk("  Bluetooth Controller did not accept the classification (see the\n"
		       "  bt_hci_core warning above); skipping the apply, newest-wins and\n"
		       "  restore checks. Set CONFIG_COEX_TB_MOCK_BT_CHAN_MAP=y to check them.\n");
		return failures;
	}

	failures += tb_expect_chan_map(applied, expected_ch6, "2.4 GHz ch6 map applied");

	/* Reserved bits 37 to 39 must never be set. */
	failures += tb_expect_true((applied[COEX_TB_BLE_CHAN_MAP_SIZE - 1U] & 0xE0U) == 0U,
				   "reserved bits 37-39 clear");

	/*
	 * A newer map queued while an older one is still waiting has to win.
	 * The minimum interval since the last apply has not elapsed, so the
	 * channel 6 map below is still waiting when the 5 GHz map (all channels
	 * available, which is the default map) replaces it. After the interval
	 * only the default map may have been applied, so nothing is retained.
	 */
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, COEX_TB_WIFI_CH6_BANDWIDTH_MHZ);
	cd_ble_wifi_connected(COEX_TB_WIFI_5G_CENTER_MHZ, COEX_TB_WIFI_5G_BANDWIDTH_MHZ);
	coex_tb_wait_ms(TB_BLE_APPLY_WAIT_MS);
	failures += tb_expect_true(!cd_ble_last_chan_map(applied),
				   "5 GHz: default map applied, nothing retained");

	/* Neither does 6 GHz reach the BLE band. */
	cd_ble_wifi_connected(COEX_TB_WIFI_6G_CENTER_MHZ, COEX_TB_WIFI_5G_BANDWIDTH_MHZ);
	coex_tb_wait_ms(TB_BLE_APPLY_WAIT_MS);
	failures += tb_expect_true(!cd_ble_last_chan_map(applied),
				   "6 GHz: default map applied, nothing retained");

	/*
	 * Rejections: zero bandwidth, and a bandwidth wide enough to cover the whole
	 * BLE band, which would leave fewer than two data channels available. The
	 * driver must refuse both rather than hand them to the Controller.
	 */
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, 0U);
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, COEX_TB_WIFI_TOO_WIDE_MHZ);
	coex_tb_wait_ms(TB_BLE_APPLY_WAIT_MS);
	failures += tb_expect_true(!cd_ble_last_chan_map(applied),
				   "zero / too wide bandwidth rejected");

	/* Apply channel 6 again, then check that disconnect restores the default. */
	cd_ble_wifi_connected(COEX_TB_WIFI_CH6_CENTER_MHZ, COEX_TB_WIFI_CH6_BANDWIDTH_MHZ);
	coex_tb_wait_ms(TB_BLE_APPLY_WAIT_MS);
	have_map = cd_ble_last_chan_map(applied);
	failures += tb_expect_true(have_map, "channel 6 map applied again");
	if (have_map) {
		failures += tb_expect_chan_map(applied, expected_ch6, "channel 6 map");
	}

	cd_ble_wifi_disconnected();
	coex_tb_wait_ms(TB_BLE_APPLY_WAIT_MS);
	failures += tb_expect_true(!cd_ble_last_chan_map(applied),
				   "default map restored after disconnect");

	return failures;
}
#endif /* CONFIG_COEX_TB_MOCK_BT_CHAN_MAP */

/**
 * Map connected-state Wi-Fi channels onto the BLE data-channel classification.
 *
 * The 2.4 GHz case is the worked example carried by both specifications: Wi-Fi
 * channel 6 at 20 MHz with a 3 MHz guard band guards 2424 to 2450 MHz, which
 * overlaps BLE data channels 10 through 22 and leaves the rest available. The
 * 5 GHz and 6 GHz cases must leave all 37 data channels available, because
 * those bands do not reach into the BLE band at all.
 *
 * The bench feeds the channels to the driver the way the Wi-Fi state notifier
 * does, through cd_ble_wifi_connected() and cd_ble_wifi_disconnected(). Both only
 * queue the map, which the driver applies later from its own work queue through
 * bt_le_set_chan_map(): the recording mock with CONFIG_COEX_TB_MOCK_BT_CHAN_MAP,
 * the real Controller otherwise.
 */
static int tb_ble_chan_map(void)
{
	/* Channels 10 to 22 bad, bits 37 to 39 clear. */
	static const uint8_t expected_ch6[COEX_TB_BLE_CHAN_MAP_SIZE] = {0xFF, 0x03, 0x80, 0xFF,
									 0x1F};
	unsigned int failures;

	printk("\n=================== BLE bad-channel mapping\n");

#ifdef CONFIG_COEX_TB_MOCK_BT_CHAN_MAP
	failures = tb_ble_apply_mock(expected_ch6);
#else
	failures = tb_ble_apply_real(expected_ch6);
#endif

	printk("  ---- %u BLE channel map checks failed ----\n", failures);

	return (failures == 0U) ? 0 : -EIO;
}
#endif /* COEX_TB_TEST_BLE_CHAN_MAP */

/**
 * Read the CM counters back and cross-check them.
 *
 * cd_stats_fetch() posts the request on the driver's work queue and waits for
 * the statistics event; the driver stores the payload as a side effect of
 * receiving that event and copies the snapshot out.
 *
 * A failing consistency check fails this step, so a run that produced
 * inconsistent CM bookkeeping cannot report an overall pass.
 */
static int tb_get_coex_manager_stats(void)
{
	struct cm_stats_t stats;
	struct cm_fsm_patch_stats_t patch_stats;
	bool have_patch_stats = false;
	unsigned int failures;
	int ret;

	ret = cd_stats_fetch(&stats, &patch_stats, &have_patch_stats);
	if (ret != 0) {
		printk("Coex TB: get stats failed: %s (%d)\n", coex_tb_errstr(ret), ret);
		return ret;
	}

	failures = coex_tb_print_and_validate_stats(&stats,
						    have_patch_stats ? &patch_stats : NULL);
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
	const char *test_macro;
	int (*fn)(void);
};

#define TB_STEP(macro, label, fn) \
	{ label, #macro, fn }

/*
 * Steps run top to bottom. The order encodes real dependencies:
 * COEXC configuration, then argument validation while coexistence is still down,
 * then runtime commands, then the statistics read-back.
 */
static const struct tb_step tb_steps[] = {
#ifdef COEX_TB_TEST_CONFIGURE_COEXC
	TB_STEP(COEX_TB_TEST_CONFIGURE_COEXC, "configure COEXC", tb_configure_coexc),
#endif
#ifdef COEX_TB_TEST_NEGATIVE_ARGS
	TB_STEP(COEX_TB_TEST_NEGATIVE_ARGS, "argument validation", tb_negative_args),
#endif
#ifdef COEX_TB_TEST_INIT_AND_ENABLE
	TB_STEP(COEX_TB_TEST_INIT_AND_ENABLE, "radio power notify", tb_init_and_enable),
#endif
#ifdef COEX_TB_TEST_STORE_BEFORE_UP
	TB_STEP(COEX_TB_TEST_STORE_BEFORE_UP, "settings stored while Wi-Fi is down",
		tb_store_before_up),
#endif
#ifdef COEX_TB_TEST_SET_PRIORITY_RANGES
	TB_STEP(COEX_TB_TEST_SET_PRIORITY_RANGES, "set priority ranges", tb_set_priority_ranges),
#endif
#ifdef COEX_TB_TEST_UPDATE_COEX_USER_PARAMS
	TB_STEP(COEX_TB_TEST_UPDATE_COEX_USER_PARAMS, "update coex user params",
		tb_update_coex_user_parameters),
#endif
#ifdef COEX_TB_TEST_UPDATE_COEX_PARAMETERS
	TB_STEP(COEX_TB_TEST_UPDATE_COEX_PARAMETERS, "update coex params",
		tb_update_coex_parameters),
#endif
#ifdef COEX_TB_TEST_ENABLE_COEXISTENCE
	TB_STEP(COEX_TB_TEST_ENABLE_COEXISTENCE, "enable/disable coexistence",
		tb_enable_disable_coex_default),
#endif
#ifdef COEX_TB_TEST_FORCE_ANTENNA_ALLOC
	TB_STEP(COEX_TB_TEST_FORCE_ANTENNA_ALLOC, "force antenna allocation",
		tb_force_antenna_allocation),
#endif
#ifdef COEX_TB_TEST_FORCE_LNASW_ALLOC
	TB_STEP(COEX_TB_TEST_FORCE_LNASW_ALLOC, "force shared LNA switch", tb_control_shared_lna),
#endif
#ifdef COEX_TB_TEST_BLE_CHAN_MAP
	TB_STEP(COEX_TB_TEST_BLE_CHAN_MAP, "BLE bad-channel mapping", tb_ble_chan_map),
#endif
#ifdef COEX_TB_TEST_GET_CM_STATS
	TB_STEP(COEX_TB_TEST_GET_CM_STATS, "get CM stats", tb_get_coex_manager_stats),
#endif
};

int coex_tb_run_all(void)
{
	const char *failed_step = NULL;
	int ret = 0;

	printk("\n===========================================================================\n");
	printk("nRF71 SR coexistence driver test bench start\n");
	printk("Running %u enabled test step(s)\n", (unsigned int)ARRAY_SIZE(tb_steps));
	printk("===========================================================================\n");

	for (size_t i = 0U; i < ARRAY_SIZE(tb_steps); i++) {
		printk("\n=================== Running %s (%s)\n", tb_steps[i].name,
		       tb_steps[i].test_macro);
		ret = tb_steps[i].fn();
		if (ret != 0) {
			failed_step = tb_steps[i].name;
			break;
		}
	}

	coex_tb_wait_ms(COEX_TB_FINAL_WAIT_MS);

	printk("===========================================================================\n");
	if (ret == 0) {
		printk("nRF71 SR coexistence driver test bench PASS\n");
	} else {
		printk("nRF71 SR coexistence driver test bench FAIL: %s: %s (%d)\n",
		       (failed_step != NULL) ? failed_step : "unknown step", coex_tb_errstr(ret),
		       ret);
	}
	printk("===========================================================================\n");

	return ret;
}
