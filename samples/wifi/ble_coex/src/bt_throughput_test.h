/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_THROUGHPUT_TEST_H_
#define BT_THROUGHPUT_TEST_H_

#include "coex.h"

/**
 * Initialize BLE throughput test init
 *
 * @return Zero on success
 */
int bt_throughput_test_init(const struct shell *shell, size_t argc,
	char **argv);

/**
 * @brief Run BLE throughput test
 *
 * @return Zero on success
 */
int bt_throughput_test_run(const struct shell *shell,
	     const struct bt_le_conn_param *conn_param,
	     const struct bt_conn_le_phy_param *phy,
	     const struct bt_conn_le_data_len_param *data_len);

/**
 * @brief Exit BLE throughput test
 *
 * @return Zero on success
 */
int bt_throughput_test_exit(void);

/**
 * @brief Configure BT throughput (init)
 *
 * @return Zero on success
 */
static int configure_bt_throughput(const struct shell *shell, size_t argc,
			char **argv);

#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
/**
 * @brief Configure SR side switch
 *
 * @return Zero on success
 */
static int configure_sr_switch(const struct shell *shell, size_t argc,
			char **argv);
#endif

/**
 * @brief Disable coexistence hardware
 *
 * @return Zero on success
 */
static int disable_coex_hardware(const struct shell *shell, size_t argc,
			char **argv);
#endif /* THROUGHPUT_MAIN_H_ */
