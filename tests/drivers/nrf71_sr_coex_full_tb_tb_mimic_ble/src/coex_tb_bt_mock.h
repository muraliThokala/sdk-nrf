/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Mock of bt_le_set_chan_map() for the BLE channel map test.
 *
 * Built only when CONFIG_COEX_TB_MOCK_BT_CHAN_MAP is set. The linker redirects
 * every call to bt_le_set_chan_map(), including the one in the coexistence
 * driver, to the mock, so the test can see exactly what the driver hands to the
 * Bluetooth Controller without depending on what that Controller supports.
 */

#ifndef COEX_TB_BT_MOCK_H__
#define COEX_TB_BT_MOCK_H__

#include <stdbool.h>
#include <stdint.h>

/** Number of octets in a BLE channel map. */
#define COEX_TB_BT_MOCK_MAP_SIZE 5U

/** Forget all recorded calls and any pending rejection. */
void coex_tb_bt_mock_reset(void);

/**
 * Reject the next calls instead of accepting them.
 *
 * @param count Number of calls to reject.
 * @param err   Value the mock returns for each of them.
 */
void coex_tb_bt_mock_fail_next(unsigned int count, int err);

/** Calls seen since the last reset, accepted or not. */
unsigned int coex_tb_bt_mock_calls(void);

/** Calls the mock accepted since the last reset. */
unsigned int coex_tb_bt_mock_accepted(void);

/**
 * Copy out the map of the most recent accepted call.
 *
 * @retval true  A map was accepted since the last reset and has been copied.
 * @retval false Nothing was accepted yet.
 */
bool coex_tb_bt_mock_last_accepted(uint8_t map[COEX_TB_BT_MOCK_MAP_SIZE]);

/**
 * Wait until at least @p count calls have been accepted.
 *
 * @retval true  The count was reached.
 * @retval false Timed out first.
 */
bool coex_tb_bt_mock_wait_accepted(unsigned int count, uint32_t timeout_ms);

#endif /* COEX_TB_BT_MOCK_H__ */
