/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_coex_functions.h"

bool is_ot_device_role_client;
uint8_t ot_wait4_ping_reply_from_peer;

void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (status->status) {
		LOG_ERR("Wi-Fi Connection request failed (%d)", status->status);
	} else {
#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
		LOG_INF("Connected");
#endif
		context.connected = true;
	}

	cmd_wifi_status();

	k_sem_give(&wait_for_next);
}

void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	if (context.disconnect_requested) {
		context.disconnect_requested = false;
	} else {
		context.connected = false;
	}

#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
	cmd_wifi_status();
#endif
}

void print_dhcp_ip(struct net_mgmt_event_callback *cb)
{
	/* Get DHCP info from struct net_if_dhcpv4 and print */
	const struct net_if_dhcpv4 *dhcpv4 = cb->info;
	const struct in_addr *addr = &dhcpv4->requested_ip;
	char dhcp_info[128];

	net_addr_ntop(AF_INET, addr, dhcp_info, sizeof(dhcp_info));

#ifdef CONFIG_DEBUG_PRINT_WIFI_DHCP_INFO
	LOG_INF("IP address: %s", dhcp_info);
#endif
	k_sem_give(&wait_for_next);
}
int wifi_wait_for_next_event(const char *event_name, int timeout)
{
	int wait_result;

#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
	if (event_name) {
		LOG_INF("Waiting for %s", event_name);
	}
#endif
	wait_result = k_sem_take(&wait_for_next, K_SECONDS(timeout));
	if (wait_result) {
		LOG_ERR("Timeout waiting for %s -> %d", event_name, wait_result);
		return -1;
	}
#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
	LOG_INF("Got %s", event_name);
#endif
	k_sem_reset(&wait_for_next);

	return 0;
}

void tcp_upload_results_cb(enum zperf_status status, struct zperf_results *result,
		void *user_data)
{
	uint32_t client_rate_in_kbps;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New TCP session started.\n");
		wait4_peer_wifi_client_to_start_tp_test = 1;
		break;

	case ZPERF_SESSION_FINISHED: {

		if (result->client_time_in_us != 0U) {
			client_rate_in_kbps = (uint32_t)
				(((uint64_t)result->nb_packets_sent *
				  (uint64_t)result->packet_size * (uint64_t)8 *
				  (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)result->client_time_in_us * 1024U));
		} else {
			client_rate_in_kbps = 0U;
		}

		LOG_INF("Duration:\t%u", result->client_time_in_us);
		LOG_INF("Num packets:\t%u", result->nb_packets_sent);
		LOG_INF("Num errors:\t%u (retry or fail)\n",
						result->nb_packets_errors);
		LOG_INF("\nclient data rate = %u kbps", client_rate_in_kbps);
		k_sem_give(&udp_tcp_callback);
		break;
	}

	case ZPERF_SESSION_ERROR:
		LOG_INF("TCP upload failed\n");
		break;
	}
}

void tcp_download_results_cb(enum zperf_status status, struct zperf_results *result,
		void *user_data)
{
	uint32_t rate_in_kbps;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New TCP session started.\n");
		wait4_peer_wifi_client_to_start_tp_test = 1;
		break;

	case ZPERF_SESSION_FINISHED: {

		/* Compute baud rate */
		if (result->time_in_us != 0U) {
			rate_in_kbps = (uint32_t)
				(((uint64_t)result->total_len * 8ULL *
				  (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)result->time_in_us * 1024ULL));
		} else {
			rate_in_kbps = 0U;
		}

		LOG_INF("TCP session ended\n");
		LOG_INF("%u bytes in %u ms:", result->total_len,
					result->time_in_us/USEC_PER_MSEC);
		LOG_INF("\nThroughput:%u kbps", rate_in_kbps);
		LOG_INF("");
		k_sem_give(&udp_tcp_callback);
		break;
	}

	case ZPERF_SESSION_ERROR:
		LOG_INF("TCP session error.\n");
		break;
	}
}

void udp_download_results_cb(enum zperf_status status, struct zperf_results *result,
		void *user_data)
{
	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New session started.");
		wait4_peer_wifi_client_to_start_tp_test = 1;
		break;

	case ZPERF_SESSION_FINISHED: {
		uint32_t rate_in_kbps;

		/* Compute baud rate */
		if (result->time_in_us != 0U) {
			rate_in_kbps = (uint32_t)
				(((uint64_t)result->total_len * 8ULL *
				  (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)result->time_in_us * 1024ULL));
		} else {
			rate_in_kbps = 0U;
		}

		LOG_INF("End of session!");

		LOG_INF("Download results:");
		LOG_INF("%u bytes in %u ms",
				(result->nb_packets_rcvd * result->packet_size),
				(result->time_in_us / USEC_PER_MSEC));
		/**
		 *LOG_INF(" received packets:\t%u",
		 *		  result->nb_packets_rcvd);
		 *LOG_INF(" nb packets lost:\t%u",
		 *		  result->nb_packets_lost);
		 *LOG_INF(" nb packets outorder:\t%u",
		 *		  result->nb_packets_outorder);
		 */
		LOG_INF("\nThroughput:%u kbps", rate_in_kbps);
		LOG_INF("");
		k_sem_give(&udp_tcp_callback);
		break;
	}

	case ZPERF_SESSION_ERROR:
		LOG_INF("UDP session error.");
		break;
	}
}


void udp_upload_results_cb(enum zperf_status status, struct zperf_results *result,
		void *user_data)
{
	unsigned int client_rate_in_kbps;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New UDP session started");
		wait4_peer_wifi_client_to_start_tp_test = 1;
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
		LOG_INF("%u bytes in %u ms",
				(result->nb_packets_sent * result->packet_size),
				(result->client_time_in_us / USEC_PER_MSEC));
		/**LOG_INF("%u packets sent", result->nb_packets_sent);
		 *LOG_INF("%u packets lost", result->nb_packets_lost);
		 *LOG_INF("%u packets received", result->nb_packets_rcvd);
		 */
		LOG_INF("client data rate = %u kbps", client_rate_in_kbps);
		k_sem_give(&udp_tcp_callback);
		break;
	case ZPERF_SESSION_ERROR:
		LOG_ERR("UDP session error");
		break;
	}
}

enum nrf_wifi_pta_wlan_op_band wifi_mgmt_to_pta_band(enum wifi_frequency_bands band)
{
	switch (band) {
	case WIFI_FREQ_BAND_2_4_GHZ:
		return NRF_WIFI_PTA_WLAN_OP_BAND_2_4_GHZ;
	case WIFI_FREQ_BAND_5_GHZ:
		return NRF_WIFI_PTA_WLAN_OP_BAND_5_GHZ;
	default:
		return NRF_WIFI_PTA_WLAN_OP_BAND_NONE;
	}
}

int run_wifi_traffic_tcp(bool is_wifi_server)
{
	int ret = 0;

	if (is_wifi_server) {
		struct zperf_download_params params = {0};

		params.port = CONFIG_NET_CONFIG_PEER_IPV4_PORT;

		/* get the DHCP IP address assigned */
		struct net_if *iface = net_if_get_first_wifi();
		struct net_if_ipv4 *ipv4;
		struct net_if_addr *unicast;

		ipv4 = iface->config.ip.ipv4;
		unicast = &ipv4->unicast[0];
		LOG_INF("DHCP IP address %s", net_sprint_ipv4_addr(&unicast->address.in_addr));

		/* get socket address */
		char *addr_str = net_sprint_ipv4_addr(&unicast->address.in_addr);
		struct sockaddr addr;

		memset(&addr, 0, sizeof(addr));

		ret = net_ipaddr_parse(addr_str, strlen(addr_str), &addr);
		if (ret < 0) {
			LOG_INF("Cannot parse address \"%s\"\n",
					  addr_str);
			return ret;
		}
		memcpy(&params.addr, &addr, sizeof(struct sockaddr));


		ret = zperf_tcp_download(&params, tcp_download_results_cb, NULL);
		if (ret != 0) {
			LOG_ERR("Failed to start TCP server session: %d", ret);
			return ret;
		}
		LOG_INF("TCP server started on port %u\n", params.port);
	} else {
		struct zperf_upload_params params = {0};
		/* Start Wi-Fi TCP traffic */
		LOG_INF("Starting Wi-Fi benchmark: Zperf TCP client");
		params.duration_ms = CONFIG_COEX_TEST_DURATION;
		params.rate_kbps = CONFIG_WIFI_ZPERF_RATE;
		params.packet_size = CONFIG_WIFI_ZPERF_PKT_SIZE;
		parse_ipv4_addr(CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &in4_addr_my);
		net_sprint_ipv4_addr(&in4_addr_my.sin_addr);

		memcpy(&params.peer_addr, &in4_addr_my, sizeof(in4_addr_my));

		ret = zperf_tcp_upload_async(&params, tcp_upload_results_cb, NULL);
		if (ret != 0) {
			LOG_ERR("Failed to start TCP session: %d", ret);
			return ret;
		}
	}

	return 0;
}

int run_wifi_traffic_udp(bool is_wifi_server)
{
	int ret = 0;

	if (is_wifi_server) {
		struct zperf_download_params params = {0};

		params.port = CONFIG_NET_CONFIG_PEER_IPV4_PORT;

		/*get the DHCP IP address assigned */
		struct net_if *iface = net_if_get_first_wifi();
		struct net_if_ipv4 *ipv4;
		struct net_if_addr *unicast;

		ipv4 = iface->config.ip.ipv4;
		unicast = &ipv4->unicast[0];
		LOG_INF("DHCP IP address %s", net_sprint_ipv4_addr(&unicast->address.in_addr));

		/* get socket address */
		char *addr_str = net_sprint_ipv4_addr(&unicast->address.in_addr);
		struct sockaddr addr;

		memset(&addr, 0, sizeof(addr));

		ret = net_ipaddr_parse(addr_str, strlen(addr_str), &addr);
		if (ret < 0) {
			LOG_INF("Cannot parse address \"%s\"\n", addr_str);
			return ret;
		}
		memcpy(&params.addr, &addr, sizeof(struct sockaddr));
		strncpy(&params.device_name, CONFIG_ZPERF_WIFI_INTERFACE, 8);

		ret = zperf_udp_download(&params, udp_download_results_cb, NULL);
		if (ret != 0) {
			LOG_ERR("Failed to start UDP server session: %d", ret);
			return ret;
		}
	} else {
		struct zperf_upload_params params = {0};

		/* Start Wi-Fi UDP traffic */
		LOG_INF("Starting Wi-Fi benchmark: Zperf UDP client");
		params.duration_ms = CONFIG_COEX_TEST_DURATION;
		params.rate_kbps = CONFIG_WIFI_ZPERF_RATE;
		params.packet_size = CONFIG_WIFI_ZPERF_PKT_SIZE;

		parse_ipv4_addr(CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &in4_addr_my);
		net_sprint_ipv4_addr(&in4_addr_my.sin_addr);

		memcpy(&params.peer_addr, &in4_addr_my, sizeof(in4_addr_my));
		strncpy(&params.device_name, CONFIG_ZPERF_WIFI_INTERFACE, 8);
		ret = zperf_udp_upload_async(&params, udp_upload_results_cb, NULL);
		if (ret != 0) {
			LOG_ERR("Failed to start Wi-Fi UDP benchmark: %d", ret);
			return ret;
		}
	}

	return 0;
}

void check_wifi_traffic(void)
{
	/* Run Wi-Fi traffic */
	if (k_sem_take(&udp_tcp_callback, K_FOREVER) != 0) {
		LOG_ERR("Results are not ready");
	} else {
#ifdef CONFIG_WIFI_ZPERF_PROT_UDP
		LOG_INF("Wi-Fi UDP session finished");
#else
		LOG_INF("Wi-Fi TCP session finished");
#endif
	}
}

int wifi_connection(void)
{
	/* Wi-Fi connection */
	wifi_connect();

	if (wifi_wait_for_next_event("Wi-Fi Connection", WIFI_CONNECTION_TIMEOUT_SEC)) {
		return -1;
	}
	if (wifi_wait_for_next_event("Wi-Fi DHCP", WIFI_DHCP_TIMEOUT_SEC)) {
		return -1;
	}

	return 0;
}

void wifi_disconnection(void)
{
	int ret = 0;

	/* Wi-Fi disconnection */
#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
	LOG_INF("Disconnecting Wi-Fi");
#endif
	ret = wifi_disconnect();
	if (ret != 0) {
		LOG_INF("Disconnect failed");
	}
}

int config_pta(bool is_ant_mode_sep, bool is_ot_client, bool is_wifi_server)
{
	int ret = 0;
	enum nrf_wifi_pta_wlan_op_band wlan_band = wifi_mgmt_to_pta_band(status.band);

	if (wlan_band == NRF_WIFI_PTA_WLAN_OP_BAND_NONE) {
		LOG_ERR("Invalid Wi-Fi band: %d", wlan_band);
		return -1;
	}

	/* Configure PTA registers of Coexistence Hardware */
	LOG_INF("Configuring PTA for %s", wifi_band_txt(status.band));
	ret = nrf_wifi_coex_config_pta(wlan_band, is_ant_mode_sep, is_ot_client,
			is_wifi_server);
	if (ret != 0) {
		LOG_ERR("Failed to configure PTA coex hardware: %d", ret);
		return -1;
	}
	return 0;
}

void ot_throughput_test_exit(void)
{
	/** This is called if role is client. Disconnection in the
	 *case of server is taken care by the peer client
	 */
	ot_tput_test_exit();
}

void print_common_test_params(bool is_ant_mode_sep, bool test_thread, bool test_wifi,
		bool is_ot_client)
{
	bool ot_coex_enable = IS_ENABLED(CONFIG_MPSL_CX);
	bool is_wifi_band_2pt4g = IS_ENABLED(CONFIG_WIFI_BAND_2PT4G);

	LOG_INF("-------------------------------- Test parameters");

	if (test_wifi && test_thread) {
		LOG_INF("Running Wi-Fi and Thread tests");
	} else {
		if (test_wifi) {
			LOG_INF("Running Wi-Fi only test");
		} else {
			LOG_INF("Running Thread only test");
		}
	}
	LOG_INF("Test duration in milliseconds: %d", CONFIG_COEX_TEST_DURATION);
	if (is_wifi_band_2pt4g) {
		LOG_INF("Wi-Fi operates in 2.4G band");
	} else {
		LOG_INF("Wi-Fi operates in 5G band");
	}
	if (is_ant_mode_sep) {
		LOG_INF("Antenna mode : Separate antennas");
	} else {
		LOG_INF("Antenna mode : Shared antennas");
	}
	if (is_ot_client) {
		LOG_INF("Thread device role : Client");
	} else {
		LOG_INF("Thread device role : Server");
	}
	if (ot_coex_enable) {
		LOG_INF("Thread device posts requests to PTA");
	} else {
		LOG_INF("Thread device doesn't post requests to PTA");
	}
	LOG_INF("--------------------------------");
}


int print_wifi_tput_ot_tput_test_details(bool is_ot_client, bool is_wifi_server,
	bool is_wifi_zperf_udp, bool is_ot_zperf_udp)
{
	if (is_wifi_zperf_udp && (!is_wifi_server) && is_ot_zperf_udp && is_ot_client) {
		LOG_INF(" Test case:  Wi-Fi UDP client Thread UDP client");
	} else if (is_wifi_zperf_udp && (!is_wifi_server) && is_ot_zperf_udp &&
		(!is_ot_client)) {
		LOG_INF(" Test case:  Wi-Fi UDP client Thread UDP server");
	} else if (is_wifi_zperf_udp && (!is_wifi_server) && (!is_ot_zperf_udp) &&
		is_ot_client) {
		LOG_INF(" Test case:  Wi-Fi UDP client Thread TCP client");
	} else if (is_wifi_zperf_udp && (!is_wifi_server) && (!is_ot_zperf_udp) &&
		(!is_ot_client)) {
		LOG_INF(" Test case:  Wi-Fi UDP client Thread TCP server");
	} else if ((!is_wifi_zperf_udp) && (!is_wifi_server) && is_ot_zperf_udp &&
		is_ot_client) {
		LOG_INF(" Test case:  Wi-Fi TCP client Thread UDP client");
	} else if ((!is_wifi_zperf_udp) && (!is_wifi_server) && is_ot_zperf_udp &&
		(!is_ot_client)) {
		LOG_INF(" Test case:  Wi-Fi TCP client Thread UDP server");
	} else if ((!is_wifi_zperf_udp) && (!is_wifi_server) && (!is_ot_zperf_udp) &&
		is_ot_client) {
		LOG_INF(" Test case:  Wi-Fi TCP client Thread TCP client");
	} else if ((!is_wifi_zperf_udp) && (!is_wifi_server) && (!is_ot_zperf_udp) &&
		(!is_ot_client)) {
		LOG_INF(" Test case:  Wi-Fi TCP client Thread TCP server");
	} else if (is_wifi_zperf_udp && (is_wifi_server) && is_ot_zperf_udp &&
		is_ot_client) {
		LOG_INF(" Test case:  Wi-Fi UDP server Thread UDP client");
	} else if (is_wifi_zperf_udp && (is_wifi_server) && is_ot_zperf_udp &&
		(!is_ot_client)) {
		LOG_INF(" Test case:  Wi-Fi UDP server Thread UDP server");
	} else if (is_wifi_zperf_udp && (is_wifi_server) && (!is_ot_zperf_udp) &&
		is_ot_client) {
		LOG_INF(" Test case:  Wi-Fi UDP server Thread TCP client");
	} else if (is_wifi_zperf_udp && (is_wifi_server) && (!is_ot_zperf_udp) &&
		(!is_ot_client)) {
		LOG_INF(" Test case:  Wi-Fi UDP server Thread TCP server");
	} else if ((!is_wifi_zperf_udp) && (is_wifi_server) && is_ot_zperf_udp &&
		is_ot_client) {
		LOG_INF(" Test case:  Wi-Fi TCP server Thread UDP client");
	} else if ((!is_wifi_zperf_udp) && (is_wifi_server) && is_ot_zperf_udp &&
		(!is_ot_client)) {
		LOG_INF(" Test case:  Wi-Fi TCP server Thread UDP server");
	} else if ((!is_wifi_zperf_udp) && (is_wifi_server) && (!is_ot_zperf_udp)
			&& is_ot_client) {
		LOG_INF(" Test case:  Wi-Fi TCP server Thread TCP client");
	} else if ((!is_wifi_zperf_udp) && (is_wifi_server) && (!is_ot_zperf_udp) &&
		(!is_ot_client)) {
		LOG_INF(" Test case:  Wi-Fi TCP server Thread TCP server");
	} else {
		LOG_INF(" Test case:wifi_tput_ot_tput WRONG Wi-Fi and OT UDP/TCP option");
		return -1;
	}
	return 0;
}

int wifi_tput_ot_tput(bool test_wifi, bool is_ant_mode_sep, bool test_thread, bool is_ot_client,
	bool is_wifi_server, bool is_wifi_zperf_udp, bool is_ot_zperf_udp)
{
	int ret = 0;
	uint64_t test_start_time = 0;

	LOG_INF(" Test case:wifi_tput_ot_tput");
#if 0
	LOG_INF(" test_wifi = %d", test_wifi);
	LOG_INF(" is_ant_mode_sep = %d", is_ant_mode_sep);
	LOG_INF(" test_thread = %d", test_thread);
	LOG_INF(" is_ot_client = %d", is_ot_client);
	LOG_INF(" is_wifi_server = %d", is_wifi_server);
	LOG_INF(" is_wifi_zperf_udp = %d", is_wifi_zperf_udp);
	LOG_INF(" is_ot_zperf_udp = %d", is_ot_zperf_udp);
#endif

	ret = print_wifi_tput_ot_tput_test_details(is_ot_client, is_wifi_server,
			is_wifi_zperf_udp,	is_ot_zperf_udp);
	if (ret != 0) {
		return ret;
	}
	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);

	if (is_ot_client) {
		is_ot_device_role_client = true;
	} else {
		is_ot_device_role_client = false;
	}

	if (test_thread) {
		if (!is_ot_client) {
			LOG_INF("Make sure peer Thread role is client");
			k_sleep(K_SECONDS(3));
		}
		ret = ot_throughput_test_init(is_ot_client, is_ot_zperf_udp);
		k_sleep(K_SECONDS(3));
		if (ret != 0) {
			LOG_ERR("Thread throughput init failed: %d", ret);
			return ret;
		}

		if (is_ot_client) {
			/* nothing */
		} else {
			/* wait until the peer client joins the network */
			while (ot_wait4_ping_reply_from_peer == 0) {
				LOG_INF("Waiting on ping reply from peer");
				ot_get_peer_address(5000);
				k_sleep(K_SECONDS(1));
				if (ot_wait4_ping_reply_from_peer) {
					break;
				}
			}
		}
	}
	if (!is_wifi_server) {
#ifdef DEMARCATE_TEST_START
		LOG_INF("-------------------------start");
#endif
	}

	if (!is_wifi_server) {
		test_start_time = k_uptime_get_32();
	}

	if (test_wifi) {
		ret = wifi_connection();
		k_sleep(K_SECONDS(3));
		if (ret != 0) {
			LOG_ERR("Wi-Fi connection failed. Running the test");
			LOG_ERR("further is not meaningful. So, exiting the test");
			return ret;
		}
#if defined(CONFIG_NRF700X_BT_COEX)
		config_pta(is_ant_mode_sep, is_ot_client, is_wifi_server);
#endif/* CONFIG_NRF700X_BT_COEX */
	}

	if (test_thread) {
		if (is_ot_client) {
			/* run Thread activity and wait for the test duration */
			ot_throughput_test_run(is_ot_zperf_udp);
		} else {
			/* Peer Thread that acts as client runs the traffic. */
			while (true) {
				if ((k_uptime_get_32() - test_start_time) >
					CONFIG_COEX_TEST_DURATION) {
					break;
				}
				k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
			}
		}
	}

	if (test_wifi) {
		if (is_wifi_zperf_udp) {
			ret = run_wifi_traffic_udp(is_wifi_server);
		} else {
			ret = run_wifi_traffic_tcp(is_wifi_server);
		}

		if (ret != 0) {
			LOG_ERR("Failed to start Wi-Fi benchmark: %d", ret);
			return ret;
		}
		if (is_wifi_server) {
			while (!wait4_peer_wifi_client_to_start_tp_test) {
				k_sleep(K_SECONDS(1));
			}
			wait4_peer_wifi_client_to_start_tp_test = 0;
			test_start_time = k_uptime_get_32();
		}
	}
	if (is_wifi_server) {
#ifdef DEMARCATE_TEST_START
		LOG_INF("-------------------------start");
#endif
	}

	if (test_wifi) {
		check_wifi_traffic();
	}

	/** ToDo
	 * for thread : should have something like check_wifi_traffic().
	 * this should be commented once async version of zperf is used for thread.
	 */
	if (test_thread) {
		if (is_ot_client) {
			while (1) {
				k_sleep(K_MSEC(1000));
				if ((k_uptime_get_32() - test_start_time) >
					CONFIG_COEX_TEST_DURATION) {
					break;
				}
			}
		}
	}

	if (test_wifi) {
		wifi_disconnection();
	}

	if (test_thread) {
		if (is_ot_client) {
			ot_throughput_test_exit();
		}
	}

#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
#endif

	return 0;
}

