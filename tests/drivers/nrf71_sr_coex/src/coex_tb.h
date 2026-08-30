/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF71 SR coexistence driver test bench configuration.
 *
 * The bench exercises the driver's interfaces:
 *
 *   1. Application / configuration APIs (coex_cd_enable, priority ranges, user
 *      params, coex params, PPW allocation, Wi-Fi SW client requests, stats).
 *      These turn into CD2CM commands.
 *   2. Wi-Fi driver to CD notifications (coex_cd_wifi_power_notify), which drive
 *      the driver's CM bring-up and teardown sequence.
 *   3. SR driver to CD notifications (coex_cd_sr_power_notify,
 *      coex_cd_update_short_range_activity_info,
 *      coex_cd_sr_software_client_request).
 *   4. Direct COEXC hardware programming (coex_cd_configure_COEXC and its
 *      read-back verify), which is register access rather than messaging.
 *
 * TEST ORDER MATTERS
 * ------------------
 * The steps below are not independent. COEXC hardware must be programmed before
 * CM arbitration has any effect, and both radios must report "powered up" before
 * the driver's SR activity gate opens. Individual steps can be commented out,
 * but keep the relative order.
 */

#ifndef COEX_TB_H__
#define COEX_TB_H__

#include <stdint.h>

#include <nrf71_coex_if.h>
#include <nrf71_coex_patch_if.h>
#include <nrf71_sr_coex_api.h>
/* Timing (all values in milliseconds) */
/** Pause after a command whose effect needs a moment to be observable. */
#define COEX_TB_CMD_WAIT_MS           500U

/** How long to let Periodic Priority Windows run before stopping them. */
#define COEX_TB_PPW_TOTAL_DURATION_MS 5000U

/** Settling time after stopping PPWs, before the next test step. */
#define COEX_TB_PPW_POST_WAIT_MS      1000U

/** Final drain time before the verdict is printed. */
#define COEX_TB_FINAL_WAIT_MS         2000U

/** Interval between polls while waiting for the Wi-Fi FMAC transport. */
#define COEX_TB_POLL_INTERVAL_MS      100U

/** Give up waiting for the Wi-Fi transport after this long. */
#define COEX_TB_TRANSPORT_TIMEOUT_MS  60000U

/*
 * ---------------------------------------------------------------------------
 * Buffer sizes and blob layout
 * ---------------------------------------------------------------------------
 */

/**
 * Size of the CD2CM_UPDATE_COEX_PARAMS blob buffer.
 *
 * Taken straight from the driver's advertised limit rather than repeating the
 * number, so a driver-side change cannot silently leave the bench sending blobs
 * the driver will reject. Comfortably larger than the decoded length of
 * NRF_COEX_PARAMS, which is 66 bytes today.
 */
#define CD2CM_COEX_PARAMS_MAX_BLOB_LEN 128U
#define COEX_TB_COEX_PARAMS_BLOB_MAX  CD2CM_COEX_PARAMS_MAX_BLOB_LEN

/**
 * Byte offset of the shared-LNA-switch control field inside the decoded
 * NRF_COEX_PARAMS blob.
 */
#define COEX_TB_BLOB_LNASW_OFFSET     44U

/*
 * ---------------------------------------------------------------------------
 * Test selection: comment out a line to skip that step
 * ---------------------------------------------------------------------------
 */

/** Program the COEXC arbitration tables (direct register writes, no CM command). */
#define COEX_TB_TEST_CONFIGURE_ENABLE_COEXC

/** Reject bad arguments without disturbing CM state. Runs early on purpose. */
#define COEX_TB_TEST_NEGATIVE_ARGS

/** Report both radios as powered up, which triggers full CM configuration. */
#define COEX_TB_TEST_INIT_AND_ENABLE

/** Send Wi-Fi and SR PTI priority ranges. */
#define COEX_TB_TEST_SET_PRIORITY_RANGES

/** Send protection probabilities and the shared-antenna mode. */
#define COEX_TB_TEST_UPDATE_COEX_USER_PARAMS

/** Send the built-in coexistence parameter blob. */
#define COEX_TB_TEST_UPDATE_COEX_PARAMETERS

/** Toggle coexistence off and on in the CM. */
#define COEX_TB_TEST_ENABLE_COEXISTENCE

/**
 * Number of enable/disable iterations.
 *
 * Iteration 1 enables, iteration 2 disables, and so on. Use an even count to
 * exercise both directions; the test re-enables coexistence when it finishes
 * regardless, so later steps always start from a known state.
 */
#define COEX_TB_NUM_ENABLE_COEX_TESTS 2U

/** Walk the shared-antenna allocation modes (dynamic, static Wi-Fi, static SR). */
#define COEX_TB_TEST_FORCE_ANTENNA_ALLOC

/** Walk the shared-LNA-switch control modes. */
#define COEX_TB_TEST_FORCE_LNASW_ALLOC

/** Start and stop Periodic Priority Windows directly. */
#define COEX_TB_TEST_GENERATE_PPW

/** Drive PPWs the way the SR driver does, through the activity-info API. */
#define COEX_TB_TEST_SR_ACTIVITY_INFO

/** Request and release antenna access as a Wi-Fi software client. */
#define COEX_TB_TEST_WIFI_SW_CLIENT_REQUEST

/** Let a Wi-Fi software client request expire by timeout. */
#define COEX_TB_TEST_WIFI_SW_CLIENT_TIMEOUT

/** Read the CM counters and cross-check them. Keep this last. */
#define COEX_TB_TEST_GET_CM_STATS

/*
 * ---------------------------------------------------------------------------
 * SR activity-info test parameters
 * ---------------------------------------------------------------------------
 * The SR driver describes an activity (for example a BLE connection event) and
 * the coexistence driver turns that into a PPW start or stop request.
 */

/** Activity ON time per period, in milliseconds. */
#define COEX_TB_SR_ACTIVITY_DURATION_MS 10U

/** Activity period, in milliseconds. */
#define COEX_TB_SR_ACTIVITY_INTERVAL_MS 20U

/** Activity timeout handed to the CM, in milliseconds. */
#define COEX_TB_SR_ACTIVITY_TIMEOUT_MS  (30U * 1000U)

/** A deliberately out-of-range sr_activity_action, used to test rejection. */
#define COEX_TB_SR_ACTIVITY_BAD_ACTION  99U

#endif /* COEX_TB_H__ */
