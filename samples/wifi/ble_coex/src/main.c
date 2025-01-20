/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi and Bluetooth LE coexistence sample
 */












#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(coex, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
#include <nrfx_clock.h>
#endif
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

#include <net/wifi_mgmt_ext.h>

/* For net_sprint_ll_addr_buf */
#include "net_private.h"

#include <coex.h>

#include "bt_throughput_test.h"

#define WIFI_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | \
				NET_EVENT_WIFI_DISCONNECT_RESULT)

#define MAX_SSID_LEN 32
#define WIFI_CONNECTION_TIMEOUT 30

//static struct sockaddr_in in4_addr_my = {
//	.sin_family = AF_INET,
//	.sin_port = htons(CONFIG_NET_CONFIG_PEER_IPV4_PORT),
//};

static struct net_mgmt_event_callback wifi_sta_mgmt_cb;
static struct net_mgmt_event_callback net_addr_mgmt_cb;

static struct {
	uint8_t connected :1;
	uint8_t disconnect_requested: 1;
	uint8_t _unused : 6;
} context;

K_SEM_DEFINE(wait_for_next, 0, 1);
K_SEM_DEFINE(udp_callback, 0, 1);



#define CONFIG_WIFI_THREAD_PRIORITY 5
#define CONFIG_WIFI_THREAD_STACK_SIZE 4096


struct wifi_iface_status status = { 0 };
















static void udp_upload_results_cb(enum zperf_status status,
			  struct zperf_results *result,
			  void *user_data)
{
	unsigned int client_rate_in_kbps;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New UDP session started");
		break;
	case ZPERF_SESSION_PERIODIC_RESULT:
		/* Ignored. */
		break;
	case ZPERF_SESSION_FINISHED:
		LOG_INF("Wi-Fi benchmark: Upload completed!");
		if (result->client_time_in_us != 0U) {
			client_rate_in_kbps = (uint32_t)
				(((uint64_t)result->nb_packets_sent *
				  (uint64_t)result->packet_size * (uint64_t)8 *
				  (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)result->client_time_in_us * 1024U));
		} else {
			client_rate_in_kbps = 0U;
		}
		/* print results */
		LOG_INF("Upload results:");
		LOG_INF("%u bytes in %llu ms",
				(result->nb_packets_sent * result->packet_size),
				(result->client_time_in_us / USEC_PER_MSEC));
		LOG_INF("%u packets sent", result->nb_packets_sent);
		LOG_INF("%u packets lost", result->nb_packets_lost);
		LOG_INF("%u packets received", result->nb_packets_rcvd);
		k_sem_give(&udp_callback);
		break;
	case ZPERF_SESSION_ERROR:
		LOG_ERR("UDP session error");
		break;
	}
}

static struct sockaddr_in in4_addr_my = {
	.sin_family = AF_INET,
	.sin_port = htons(5001),
};


static int parse_ipv4_addr(char *host, struct sockaddr_in *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET, host, &addr->sin_addr);
	if (ret < 0) {
		LOG_ERR("Invalid IPv4 address %s\n", host);
		return -EINVAL;
	}

	LOG_INF("IPv4 address %s", host);

	return 0;
}


static int run_wifi_throughput(const struct shell *shell, size_t argc,
			char **argv)
{


	struct zperf_upload_params params;
	int ret;

	/* Start Wi-Fi traffic */
	LOG_INF("Starting Wi-Fi benchmark: Zperf client");
	params.duration_ms = 20000; /* 20 sec */
	params.rate_kbps = 10000;
	params.packet_size = 1024;
	parse_ipv4_addr("192.168.1.253", &in4_addr_my);
	net_sprint_ipv4_addr(&in4_addr_my.sin_addr);

	memcpy(&params.peer_addr, &in4_addr_my, sizeof(in4_addr_my));

	ret = zperf_udp_upload_async(&params, udp_upload_results_cb, NULL);
	if (ret != 0) {
		printk("Failed to start Wi-Fi benchmark: %d\n", ret);
		return ret;
	}
}












int main(void)
{
#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
	/* For now hardcode to 128MHz */
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK,
			       NRF_CLOCK_HFCLK_DIV_1);
#endif
	printk("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock/MHZ(1));

	return 0;
}

SHELL_CMD_REGISTER(wifi_run_tput, NULL, "Run Wi-Fi throughput", run_wifi_throughput);