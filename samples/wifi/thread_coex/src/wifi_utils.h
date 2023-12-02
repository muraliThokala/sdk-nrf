/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WIFI_UTILS_
#define WIFI_UTILS_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_utils, CONFIG_LOG_DEFAULT_LEVEL);

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
#include <coex.h>
#include <coex_struct.h>

#include "ot_utils.h" 

#include "fmac_main.h"
#define MAX_SSID_LEN 32
#define WIFI_CONNECTION_TIMEOUT 30 /* in seconds */
#define WIFI_DHCP_TIMEOUT 10 /* in seconds */

#define DEMARCATE_TEST_START

#define TXPOWER_INIT_VALUE 127
#define RSSI_INIT_VALUE 127

#define HIGHEST_CHANNUM_24G 14

uint32_t wifi_disconn_success_cnt;
uint32_t wifi_disconn_fail_cnt;
uint32_t wifi_disconn_no_conn_cnt;

int8_t wifi_rssi = RSSI_INIT_VALUE;
static int print_wifi_conn_status_once = 1;

struct wifi_iface_status wifi_if_status = { 0 };

static struct {
	uint8_t connected :1;
	uint8_t disconnect_requested: 1;
	uint8_t _unused : 6;
} wifi_cintext;


static struct net_mgmt_event_callback wifi_sta_mgmt_cb;
static struct net_mgmt_event_callback net_addr_mgmt_cb;

#define WIFI_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
		struct net_if *iface);


/**
 * @brief Handle net management events
 *
 * @return No return value.
 */
void net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
		struct net_if *iface);

/**
 * @brief Callback for Wi-Fi connection result
 *
 * @return No return value.
 */
void handle_wifi_connect_result(struct net_mgmt_event_callback *cb);

/**
 * @brief Callback for Wi-Fi disconnection result
 *
 * @return No return value.
 */
void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb);
/**
 * @brief Callback for Wi-Fi scan result
 *
 * @return No return value.
 */
void handle_wifi_scan_result(struct net_mgmt_event_callback *cb);

/**
 * @brief Callback for Wi-Fi scan done
 *
 * @return No return value.
 */
void handle_wifi_scan_done(struct net_mgmt_event_callback *cb);

/**
 * @brief Callback for Wi-Fi DHCP IP address
 *
 * @return None
 */
void print_dhcp_ip(struct net_mgmt_event_callback *cb);
	
#endif /* WIFI_UTILS_ */
