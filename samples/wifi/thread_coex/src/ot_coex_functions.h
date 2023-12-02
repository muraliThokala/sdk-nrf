/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_COEX_FUNCTIONS_
#define OT_COEX_FUNCTIONS_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ot_coex_functions, CONFIG_LOG_DEFAULT_LEVEL);

#include <nrfx_clock.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/zperf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>

/* For net_sprint_ll_addr_buf */
#include "net_private.h"
#include "coex.h"
#include "coex_struct.h"
#include "ot_utils.h" 
#include "fmac_main.h"

#define MAX_SSID_LEN 32
#define WIFI_CONNECTION_TIMEOUT 30 /* in seconds */
#define WIFI_DHCP_TIMEOUT 10 /* in seconds */

#define DEMARCATE_TEST_START

#define TXPOWER_INIT_VALUE 127
#define RSSI_INIT_VALUE 127

#define HIGHEST_CHANNUM_24G 14


#define KSLEEP_WIFI_DISCON_2SEC K_SECONDS(2)
#define KSLEEP_WIFI_CON_10MSEC K_SECONDS(1)
#define KSLEEP_WIFI_DISCON_10MSEC K_SECONDS(1)
#define KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC K_SECONDS(1)
#define KSLEEP_WHILE_PERIP_CONN_CHECK_1SEC K_SECONDS(1)


static uint32_t wifi_scan_cnt_24g;
static uint32_t wifi_scan_cnt_5g;
extern uint32_t wifi_scan_cmd_cnt;
uint64_t wifi_scan_start_time;
uint64_t wifi_scan_time;
uint32_t print_wifi_scan_time;

static uint32_t wifi_conn_attempt_cnt;
static uint32_t wifi_conn_success_cnt;
static uint32_t wifi_conn_fail_cnt;
static uint32_t wifi_conn_timeout_cnt;
static uint32_t wifi_conn_cnt_stability;
static uint32_t wifi_dhcp_timeout_cnt;

//extern uint32_t wifi_disconn_no_conn_cnt;
extern uint32_t wifi_disconn_no_conn_cnt;
extern uint32_t wifi_disconn_success_cnt;
extern uint32_t wifi_disconn_fail_cnt;

//extern uint32_t wifi_disconn_fail_cnt;
//extern uint32_t wifi_disconn_success_cnt;
static uint32_t wifi_disconn_attempt_cnt;
static uint32_t wifi_disconn_cnt_stability;

static uint8_t wait4_peer_wifi_client_to_start_tp_test;

extern uint32_t ot_discov_attempt_cnt;
extern uint32_t ot_discov_success_cnt;
extern uint32_t ot_discov_no_result_cnt;
extern uint32_t repeat_ot_discovery;
uint32_t ot_discov_timeout;
uint32_t ot_discov_attempts_before_test_starts;

extern uint32_t ot_connection_attempt_cnt;
extern uint32_t ot_connection_success_cnt;
uint8_t wait4_peer_ot2_start_connection;
uint32_t ot_conn_attempts_before_test_starts;
uint32_t run_ot_client_wait_in_conn;
extern bool ot_server_connected;
extern bool ot_client_connected;
extern uint32_t ot_disconnection_attempt_cnt;
extern uint32_t ot_disconnection_success_cnt;
extern uint32_t ot_disconnection_fail_cnt;
extern uint32_t ot_discon_no_conn_cnt;
extern uint32_t ot_discon_no_conn;
extern uint32_t ot_disconn_cnt_stability;

static struct sockaddr_in in4_addr_my = {
	.sin_family = AF_INET,
	.sin_port = htons(CONFIG_NET_CONFIG_PEER_IPV4_PORT),
};



///**
// * @brief Handle Wi-Fi management events
// *
// * @return No return value.
// */
//void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
//		struct net_if *iface);

static uint32_t scan_result_count;

/**
 * @brief Disable RPU.
 *
 * @return None
 */
void rpu_disable(void);

/**
 * @brief BT throughtput test run
 *
 * @return None
 */
void run_ot_benchmark_test(void);

/**
 * @brief Thread discovery test run
 *
 * @return None
 */
void run_ot_discovery_test(void);

/**
 * @brief BT connection test run
 *
 * @return None
 */
void run_ot_connection_test(void);

/**
 * @brief Wi-Fi scan test run
 *
 * @return None
 */
void run_wifi_scan_test(void);

/**
 * @brief Wi-Fi connection test run
 *
 * @return None
 */
void run_wifi_conn_test(void);

/**
 * @brief commnad to start Wi-Fi scan
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int cmd_wifi_scan(void);
/**
 * @brief Call Wi-Fi connection event
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_connection(void);
/**
 * @brief configure PTA registers
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int config_pta(bool is_ant_mode_sep, bool is_ot_client,
				bool is_wifi_server);
/**
 * @brief Start wi-fi traffic for zperf udp upload or configure
 * zperf traffic for udp download
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int run_wifi_traffic_udp(void);

/**
 * @brief Start wi-fi traffic for zperf tcp upload or configure
 * zperf traffic for tcp download
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int run_wifi_traffic_tcp(void);

/**
 * @brief start Thread connection/traffic using thread start
 *
 * @return None
 */
void start_ot_activity(void);

/**
 * @brief start Wi-Fi scan using thread start
 *
 * @return None
 */
void start_wifi_activity(void);

/**
 * @brief check if iperf traffic is complete
 *
 * @return None
 */
void check_wifi_traffic(void);
/**
 * @brief Run Thread traffic using thread join
 *
 * @return None
 */
void run_ot_activity(void);
/**
 * @brief Disconnect Wi-Fi
 *
 * @return None
 */
void wifi_disconnection(void);
/**
 * @brief Exit Thread throughput test
 *
 * @return None
 */
void ot_throughput_test_exit(void);

static struct {
	uint8_t connected :1;
	uint8_t disconnect_requested: 1;
	uint8_t _unused : 6;
} context;

K_SEM_DEFINE(wait_for_next, 0, 1);
K_SEM_DEFINE(udp_tcp_callback, 0, 1);

struct wifi_iface_status status = { 0 };
uint32_t repeat_wifi_scan = 1;

#if defined(CONFIG_WIFI_SCAN_OT_TP_CLIENT) || \
	defined(CONFIG_WIFI_SCAN_OT_TP_SERVER) || \
	defined(CONFIG_WIFI_CON_SCAN_OT_TP_CLIENT) || \
	defined(CONFIG_WIFI_CON_SCAN_OT_TP_SERVER) || \
	defined(CONFIG_WIFI_CON_OT_TP_CLIENT) || \
	defined(CONFIG_WIFI_CON_OT_TP_SERVER) || \
	defined(CONFIG_WIFI_TP_UDP_CLIENT_OT_TP_CLIENT) || \
	defined(CONFIG_WIFI_TP_UDP_CLIENT_OT_TP_SERVER) || \
	defined(CONFIG_WIFI_TP_UDP_SERVER_OT_TP_CLIENT) || \
	defined(CONFIG_WIFI_TP_UDP_SERVER_OT_TP_SERVER) || \
	defined(CONFIG_WIFI_TP_TCP_CLIENT_OT_TP_CLIENT) || \
	defined(CONFIG_WIFI_TP_TCP_CLIENT_OT_TP_SERVER) || \
	defined(CONFIG_WIFI_TP_TCP_SERVER_OT_TP_CLIENT) || \
	defined(CONFIG_WIFI_TP_TCP_SERVER_OT_TP_SERVER) || \
	defined(CONFIG_WIFI_CON_STABILITY_OT_TP_CLIENT_INTERFERENCE) || \
	defined(CONFIG_WIFI_CON_STABILITY_OT_TP_SERVER_INTERFERENCE) || \
	defined(CONFIG_WIFI_SHUTDOWN_OT_TP_CLIENT) || \
	defined(CONFIG_WIFI_SHUTDOWN_OT_TP_SERVER)

	#define ENABLE_OT_TRAFFIC_TEST

	K_THREAD_DEFINE(run_ot_traffic,
		CONFIG_WIFI_THREAD_STACK_SIZE,
		run_ot_benchmark_test,
		NULL,
		NULL,
		NULL,
		CONFIG_WIFI_THREAD_PRIORITY,
		0,
		K_TICKS_FOREVER);
#endif
	
#if defined(CONFIG_WIFI_SCAN_OT_DISCOV) || \
	defined(CONFIG_WIFI_CON_SCAN_OT_DISCOV) || \
	defined(CONFIG_WIFI_CON_OT_DISCOV) || \
	defined(CONFIG_WIFI_TP_UDP_CLIENT_OT_DISCOV) || \
	defined(CONFIG_WIFI_TP_UDP_SERVER_OT_DISCOV) || \
	defined(CONFIG_WIFI_TP_TCP_CLIENT_OT_DISCOV) || \
	defined(CONFIG_WIFI_TP_TCP_SERVER_OT_DISCOV) || \
	defined(CONFIG_WIFI_CON_STABILITY_OT_DISC_INTERFERENCE) || \
	defined(CONFIG_WIFI_SHUTDOWN_OT_DISCOV)

	#define ENABLE_OT_DISCOV_TEST

	K_THREAD_DEFINE(run_ot_discovery,
		CONFIG_WIFI_THREAD_STACK_SIZE,
		run_ot_discovery_test,
		NULL,
		NULL,
		NULL,
		CONFIG_WIFI_THREAD_PRIORITY,
		0,
		K_TICKS_FOREVER);
#endif

	
#if defined(CONFIG_WIFI_SCAN_OT_CONNECTION) || \
	defined(CONFIG_WIFI_CON_SCAN_OT_CONNECTION) || \
	defined(CONFIG_WIFI_TP_UDP_CLIENT_OT_CONNECTION) || \
	defined(CONFIG_WIFI_TP_UDP_SERVER_OT_CONNECTION) || \
	defined(CONFIG_WIFI_TP_TCP_CLIENT_OT_CONNECTION) || \
	defined(CONFIG_WIFI_TP_TCP_SERVER_OT_CONNECTION) || \
	defined(CONFIG_OT_CON_STABILITY_WIFI_SCAN_INTERFERENCE) || \
	defined(CONFIG_OT_CON_STABILITY_WIFI_CON_SCAN_INTERFERENCE) || \
	defined(CONFIG_OT_CON_STABILITY_WIFI_TP_UDP_CLIENT_INTERFERENCE) || \
	defined(CONFIG_OT_CON_STABILITY_WIFI_TP_UDP_SERVER_INTERFERENCE) || \
	defined(CONFIG_OT_CON_STABILITY_WIFI_TP_TCP_CLIENT_INTERFERENCE) || \
	defined(CONFIG_OT_CON_STABILITY_WIFI_TP_TCP_SERVER_INTERFERENCE) || \
	defined(CONFIG_WIFI_SHUTDOWN_OT_CONNECTION)

	#define ENABLE_OT_CONN_TEST

	K_THREAD_DEFINE(run_ot_connection,
		CONFIG_WIFI_THREAD_STACK_SIZE,
		run_ot_connection_test,
		NULL,
		NULL,
		NULL,
		CONFIG_WIFI_THREAD_PRIORITY,
		0,
		K_TICKS_FOREVER);
#endif

#if defined(CONFIG_WIFI_SCAN_OT_DISCOV) || \
	defined(CONFIG_WIFI_SCAN_OT_CONNECTION) || \
	defined(CONFIG_WIFI_SCAN_OT_TP_CLIENT) || \
	defined(CONFIG_WIFI_SCAN_OT_TP_SERVER) || \
	defined(CONFIG_WIFI_CON_SCAN_OT_DISCOV) || \
	defined(CONFIG_WIFI_CON_SCAN_OT_CONNECTION) || \
	defined(CONFIG_WIFI_CON_SCAN_OT_TP_CLIENT) || \
	defined(CONFIG_WIFI_CON_SCAN_OT_TP_SERVER)	|| \
	defined(CONFIG_OT_CON_STABILITY_WIFI_SCAN_INTERFERENCE) || \
	defined(CONFIG_OT_CON_STABILITY_WIFI_CON_SCAN_INTERFERENCE)

	#define ENABLE_WIFI_SCAN_TEST

	K_THREAD_DEFINE(run_wlan_scan,
		CONFIG_WIFI_THREAD_STACK_SIZE,
		run_wifi_scan_test,
		NULL,
		NULL,
		NULL,
		CONFIG_WIFI_THREAD_PRIORITY,
		0,
		K_TICKS_FOREVER);
#endif
	
#if defined(CONFIG_WIFI_CON_OT_DISCOV) ||\
	defined(CONFIG_WIFI_CON_OT_TP_CLIENT) ||\
	defined(CONFIG_WIFI_CON_OT_TP_SERVER)
	
	/* Note: dont use stability tests here */
	#define ENABLE_WIFI_CONN_TEST

	K_THREAD_DEFINE(run_wlan_conn,
		CONFIG_WIFI_THREAD_STACK_SIZE,
		run_wifi_conn_test,
		NULL,
		NULL,
		NULL,
		CONFIG_WIFI_THREAD_PRIORITY,
		0,
		K_TICKS_FOREVER);
#endif

/**
 * @brief Print Wi-Fi status
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int cmd_wifi_status(void);
/**
 * @brief Initailise Wi-Fi arguments in variables
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int __wifi_args_to_params(struct wifi_connect_req_params *params);
/**
 * @brief Request Wi-Fi connection
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_connect(void);
/**
 * @brief Request Wi-Fi disconnection
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_disconnect(void);
/**
 * @brief parse Wi-Fi IPv4 address
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_ipv4_addr(char *host, struct sockaddr_in *addr);
/**
 * @brief wait for next Wi-Fi event
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wait_for_next_event(const char *event_name, int timeout);
/**
 * @brief Callback for UDP download results
 *
 * @return Zero on success or (negative) error code otherwise.
 */
void udp_download_results_cb(enum zperf_status status,
							struct zperf_results *result,
							void *user_data);
/**
 * @brief Callback for UDP upload results
 *
 * @return Zero on success or (negative) error code otherwise.
 */
void udp_upload_results_cb(enum zperf_status status,
							struct zperf_results *result,
							void *user_data);
/**
 * @brief Callback for TCP download results
 *
 * @return Zero on success or (negative) error code otherwise.
 */
void tcp_download_results_cb(enum zperf_status status,
							struct zperf_results *result,
							void *user_data);
/**
 * @brief Callback for TCP upload results
 *
 * @return Zero on success or (negative) error code otherwise.
 */
void tcp_upload_results_cb(enum zperf_status status,
							struct zperf_results *result,
							void *user_data);
/**
 * @brief Run Wi-Fi scan test
 */
void wifi_scan_test_run(void);

/**
 * @brief Run Wi-Fi connection test
 */
void wifi_connection_test_run(void);

/**
 * @brief Start Thread advertisement
 *
 * @return None
 */
void adv_start(void);

/**
 * @brief Start Thread scan
 *
 * @return None
 */
void scan_start(void);

/**
 * @brief Callback for Wi-Fi DHCP IP addreds assigned
 *
 * @return No return value.
 */
void print_dhcp_ip(struct net_mgmt_event_callback *cb);
/**
 * @brief Print common test parameters info
 *
 * @return None
 */
void print_common_test_params(bool is_ant_mode_sep, bool test_thread, bool test_wifi,
	bool is_ot_client);
/**
 * @brief Print Thread connection test parameters info
 *
 * @return None
 */
void print_ot_connection_test_params(bool is_ot_client);

#ifdef CONFIG_TWT_ENABLE
	/**
	 * @brief Tear down TWT
	 *
	 * @return success/fail
	 */
	static int teardown_twt(void);

	/**
	 * @brief Wait for TWT response
	 *
	 * @return success/fail
	 */
	static int wait_for_twt_resp_received(void);

	/**
	 * @brief Setup TWT
	 *
	 * @return success/fail
	 */
	static int setup_twt(void);

	/**
	 * @brief Print TWT parameters
	 *
	 * @return None
	 */
	void print_twt_params(uint8_t dialog_token, uint8_t flow_id,
				     enum wifi_twt_negotiation_type negotiation_type,
				     bool responder, bool implicit, bool announce,
				     bool trigger, uint32_t twt_wake_interval,
				     uint64_t twt_interval);
#endif

#endif /* OT_COEX_FUNCTIONS_ */
