/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef THREAD_COEX_MAIN_H_
#define THREAD_COEX_MAIN_H_

/**
 * @brief Function to test Wi-Fi throughput client/server and Thread throughput client/server
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_tput_ot_tput(bool test_wifi, bool is_ant_mode_sep, bool test_thread,
		bool is_ot_client, bool is_wifi_server, bool is_wifi_zperf_udp,
		bool is_ot_zperf_udp, bool is_sr_protocol_ble);

/**
 * @brief memset_context
 *
 * @return No return value.
 */
void memset_context(void);

/**
 * @brief Handle net management callbacks
 *
 * @return No return value.
 */
void wifi_mgmt_callback_functions(void);

/**
 * @brief Wi-Fi init.
 *
 * @return None
 */
void wifi_init(void);

/**
 * @brief Network configuration
 *
 * @return status
 */
int net_config_init_app(const struct device *dev, const char *app_info);

#endif /* THREAD_COEX_MAIN_H_ */
