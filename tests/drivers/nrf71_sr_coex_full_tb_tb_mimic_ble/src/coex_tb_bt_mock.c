/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Mock of bt_le_set_chan_map(), see coex_tb_bt_mock.h.
 *
 * The test CMakeLists.txt passes -Wl,--wrap=bt_le_set_chan_map, which makes the
 * linker resolve every reference to bt_le_set_chan_map() to
 * __wrap_bt_le_set_chan_map() below. The real function is not called.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/printk.h>

#include "coex_tb_bt_mock.h"

#define MOCK_POLL_MS 20

static struct k_spinlock mock_lock;
static unsigned int mock_calls;
static unsigned int mock_accepted;
static unsigned int mock_fail_left;
static int mock_fail_err;
static uint8_t mock_last_map[COEX_TB_BT_MOCK_MAP_SIZE];

/** Linker-wrapped replacement for bt_le_set_chan_map(). */
int __wrap_bt_le_set_chan_map(uint8_t chan_map[COEX_TB_BT_MOCK_MAP_SIZE]);

int __wrap_bt_le_set_chan_map(uint8_t chan_map[COEX_TB_BT_MOCK_MAP_SIZE])
{
	k_spinlock_key_t key;
	int ret = 0;

	key = k_spin_lock(&mock_lock);
	mock_calls++;
	if (mock_fail_left > 0U) {
		mock_fail_left--;
		ret = mock_fail_err;
	} else {
		mock_accepted++;
		memcpy(mock_last_map, chan_map, sizeof(mock_last_map));
	}
	k_spin_unlock(&mock_lock, key);

	printk("  [BT mock] bt_le_set_chan_map(%02x %02x %02x %02x %02x) -> %d (%s)\n",
	       chan_map[0], chan_map[1], chan_map[2], chan_map[3], chan_map[4], ret,
	       (ret == 0) ? "accepted" : "rejected");

	return ret;
}

void coex_tb_bt_mock_reset(void)
{
	k_spinlock_key_t key = k_spin_lock(&mock_lock);

	mock_calls = 0U;
	mock_accepted = 0U;
	mock_fail_left = 0U;
	mock_fail_err = 0;
	memset(mock_last_map, 0, sizeof(mock_last_map));
	k_spin_unlock(&mock_lock, key);
}

void coex_tb_bt_mock_fail_next(unsigned int count, int err)
{
	k_spinlock_key_t key = k_spin_lock(&mock_lock);

	mock_fail_left = count;
	mock_fail_err = err;
	k_spin_unlock(&mock_lock, key);
}

unsigned int coex_tb_bt_mock_calls(void)
{
	k_spinlock_key_t key = k_spin_lock(&mock_lock);
	unsigned int calls = mock_calls;

	k_spin_unlock(&mock_lock, key);

	return calls;
}

unsigned int coex_tb_bt_mock_accepted(void)
{
	k_spinlock_key_t key = k_spin_lock(&mock_lock);
	unsigned int accepted = mock_accepted;

	k_spin_unlock(&mock_lock, key);

	return accepted;
}

bool coex_tb_bt_mock_last_accepted(uint8_t map[COEX_TB_BT_MOCK_MAP_SIZE])
{
	k_spinlock_key_t key = k_spin_lock(&mock_lock);
	bool have_map = (mock_accepted > 0U);

	if (have_map) {
		memcpy(map, mock_last_map, sizeof(mock_last_map));
	}
	k_spin_unlock(&mock_lock, key);

	return have_map;
}

bool coex_tb_bt_mock_wait_accepted(unsigned int count, uint32_t timeout_ms)
{
	int64_t deadline = k_uptime_get() + (int64_t)timeout_ms;

	while (coex_tb_bt_mock_accepted() < count) {
		if (k_uptime_get() >= deadline) {
			return false;
		}
		k_msleep(MOCK_POLL_MS);
	}

	return true;
}
