/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence Manager test bench configuration.
 *
 * Tests call coex_cd_* APIs; the driver posts each CD2CM command and waits for
 * the matching CM2CD completion event before returning to the test bench.
 */

#ifndef COEX_TB_H__
#define COEX_TB_H__

#include <stdint.h>

/* Timing (milliseconds). Host tests use shorter waits than the viper emulator TB. */
#define COEX_TB_CMD_WAIT_MS          500U
#define COEX_TB_PPW_TOTAL_DURATION_MS 5000U
#define COEX_TB_PPW_POST_WAIT_MS     1000U
#define COEX_TB_FINAL_WAIT_MS        2000U
#define COEX_TB_STATS_POLL_MS        100U
#define COEX_TB_TRANSPORT_TIMEOUT_MS 60000U

/* Test cases to run (enable/disable individually). */
#define COEX_TB_TEST_CONFIGURE_ENABLE_COEXC
#define COEX_TB_TEST_INIT_AND_ENABLE
#define COEX_TB_TEST_SET_PRIORITY_RANGES
#define COEX_TB_TEST_UPDATE_COEX_USER_PARAMS
#define COEX_TB_TEST_UPDATE_COEX_PARAMETERS
#define COEX_TB_TEST_ENABLE_COEXISTENCE
#define COEX_TB_NUM_ENABLE_COEX_TESTS  1U
#define COEX_TB_TEST_FORCE_ANTENNA_ALLOC
#define COEX_TB_TEST_FORCE_LNASW_ALLOC
#define COEX_TB_TEST_GENERATE_PPW
#define COEX_TB_TEST_WIFI_SW_CLIENT_REQUEST
#define COEX_TB_TEST_WIFI_SW_CLIENT_TIMEOUT
#define COEX_TB_TEST_GET_CM_STATS

#endif /* COEX_TB_H__ */
