/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_coex_functions.h"

bool is_ot_device_role_client;

uint8_t wait4_ping_reply_from_peer;

#ifdef CONFIG_TWT_ENABLE
	#define STATUS_POLLING_MS 300
	#define TWT_RESP_TIMEOUT_S 20
	extern bool twt_resp_received;
	extern bool twt_supported;

//	static int setup_twt(void)
//	{
//		struct net_if *iface = net_if_get_first_wifi();
//		struct wifi_twt_params params = { 0 };
//		int ret;
//
//		params.operation = WIFI_TWT_SETUP;
//		params.negotiation_type = WIFI_TWT_INDIVIDUAL;
//		params.setup_cmd = WIFI_TWT_SETUP_CMD_REQUEST;
//		params.dialog_token = 1;
//		params.flow_id = 1;
//		params.setup.responder = 0;
//		params.setup.trigger = IS_ENABLED(CONFIG_TWT_TRIGGER_ENABLE);
//		params.setup.implicit = 1;
//		params.setup.announce = IS_ENABLED(CONFIG_TWT_ANNOUNCED_MODE);
//		params.setup.twt_wake_interval = CONFIG_TWT_WAKE_INTERVAL;
//		params.setup.twt_interval = CONFIG_TWT_INTERVAL;
//
//		ret = net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params));
//		if (ret) {
//			LOG_INF("TWT setup failed: %d", ret);
//			return ret;
//		}
//
//		LOG_INF("TWT setup requested");
//
//		return 0;
//	}
//
//	static int teardown_twt(void)
//	{
//		struct net_if *iface = net_if_get_first_wifi();
//		struct wifi_twt_params params = { 0 };
//		int ret;
//
//		params.operation = WIFI_TWT_TEARDOWN;
//		params.negotiation_type = WIFI_TWT_INDIVIDUAL;
//		params.setup_cmd = WIFI_TWT_TEARDOWN;
//		params.dialog_token = 1;
//		params.flow_id = 1;
//
//		ret = net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params));
//		if (ret) {
//			LOG_ERR("%s with %s failed, reason : %s",
//				wifi_twt_operation2str[params.operation],
//				wifi_twt_negotiation_type2str[params.negotiation_type],
//				get_twt_err_code_str(params.fail_reason));
//			return ret;
//		}
//
//		LOG_INF("TWT teardown success");
//
//		return 0;
//	}
//	static void handle_wifi_twt_event(struct net_mgmt_event_callback *cb)
//	{
//		const struct wifi_twt_params *resp =
//			(const struct wifi_twt_params *)cb->info;
//
//		if (resp->operation == WIFI_TWT_TEARDOWN) {
//			LOG_INF("TWT teardown received for flow ID %d\n",
//			      resp->flow_id);
//			return;
//		}
//
//		if (resp->resp_status == WIFI_TWT_RESP_RECEIVED) {
//			twt_resp_received = 1;
//			LOG_INF("TWT response: %s",
//			      wifi_twt_setup_cmd2str[resp->setup_cmd]);
//			LOG_INF("== TWT negotiated parameters ==");
//			print_twt_params(resp->dialog_token,
//					 resp->flow_id,
//					 resp->negotiation_type,
//					 resp->setup.responder,
//					 resp->setup.implicit,
//					 resp->setup.announce,
//					 resp->setup.trigger,
//					 resp->setup.twt_wake_interval,
//					 resp->setup.twt_interval);
//		} else {
//			LOG_INF("TWT response timed out\n");
//		}
//	}

	static int wait_for_twt_resp_received(void)
	{
		int i, timeout_polls = (TWT_RESP_TIMEOUT_S * 1000) / STATUS_POLLING_MS;
		
		for (i = 0; i < timeout_polls; i++) {
			k_sleep(K_MSEC(STATUS_POLLING_MS));
			if (twt_resp_received) {
				return 1;
			}
		}

		return 0;
	}
	
	void print_twt_params(uint8_t dialog_token, uint8_t flow_id,
				     enum wifi_twt_negotiation_type negotiation_type,
				     bool responder, bool implicit, bool announce,
				     bool trigger, uint32_t twt_wake_interval,
				     uint64_t twt_interval)
	{
		LOG_INF("TWT Dialog token: %d",
		      dialog_token);
		LOG_INF("TWT flow ID: %d",
		      flow_id);
		LOG_INF("TWT negotiation type: %s",
		      wifi_twt_negotiation_type2str[negotiation_type]);
		LOG_INF("TWT responder: %s",
		       responder ? "true" : "false");
		LOG_INF("TWT implicit: %s",
		      implicit ? "true" : "false");
		LOG_INF("TWT announce: %s",
		      announce ? "true" : "false");
		LOG_INF("TWT trigger: %s",
		      trigger ? "true" : "false");
		LOG_INF("TWT wake interval: %d us",
		      twt_wake_interval);
		LOG_INF("TWT interval: %lld us",
		      twt_interval);
		LOG_INF("========================");
	}

#endif


//int cmd_wifi_status(void)
//{
//	struct net_if *iface = net_if_get_first_wifi();
//
//	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
//				sizeof(struct wifi_iface_status))) {
//		LOG_INF("Status request failed");
//
//		return -ENOEXEC;
//	}
//
//	#ifndef CONFIG_PRINTS_FOR_AUTOMATION
//		LOG_INF("Status: successful");
//		LOG_INF("==================");
//		LOG_INF("State: %s", wifi_state_txt(status.state));
//	#endif
//
//	if (status.state >= WIFI_STATE_ASSOCIATED) {
//		uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];
//
//		if (print_wifi_conn_status_once == 1) {
//
//			LOG_INF("Interface Mode: %s",
//				   wifi_mode_txt(status.iface_mode));
//			LOG_INF("Link Mode: %s",
//				   wifi_link_mode_txt(status.link_mode));
//			LOG_INF("SSID: %-32s", status.ssid);
//			LOG_INF("BSSID: %s",
//				   net_sprint_ll_addr_buf(
//					status.bssid, WIFI_MAC_ADDR_LEN,
//					mac_string_buf, sizeof(mac_string_buf)));
//			LOG_INF("Band: %s", wifi_band_txt(status.band));
//			LOG_INF("Channel: %d", status.channel);
//			LOG_INF("Security: %s", wifi_security_txt(status.security));
//			/* LOG_INF("MFP: %s", wifi_mfp_txt(status.mfp)); */
//			LOG_INF("Wi-Fi RSSI: %d", status.rssi);
//#ifdef CONFIG_TWT_ENABLE
//			LOG_INF("TWT: %s", status.twt_capable ? "Supported" : "Not supported");
//
//			if (status.twt_capable) {
//				twt_supported = 1;
//			}
//#endif
//			print_wifi_conn_status_once++;
//		}
//		wifi_rssi = status.rssi;
//	}
//
//	return 0;
//}

//void memset_context(void)
//{
//	memset(&context, 0, sizeof(context));
//}
//
//void wifi_net_mgmt_callback_functions(void)
//{
//	net_mgmt_init_event_callback(&wifi_sta_mgmt_cb, wifi_mgmt_event_handler,
//		WIFI_MGMT_EVENTS);
//
//	net_mgmt_add_event_callback(&wifi_sta_mgmt_cb);
//
//	net_mgmt_init_event_callback(&net_addr_mgmt_cb, net_mgmt_event_handler,
//		NET_EVENT_IPV4_DHCP_BOUND);
//
//	net_mgmt_add_event_callback(&net_addr_mgmt_cb);
//
//#ifdef CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT
//	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
//#endif
//
//	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock/MHZ(1));
//
//	k_sleep(K_SECONDS(1));
//}
//
//
//void wifi_init(void) {
//
//	memset_context();
//	
//	wifi_net_mgmt_callback_functions();
//}

//void net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
//		struct net_if *iface)
//{
//	switch (mgmt_event) {
//	case NET_EVENT_IPV4_DHCP_BOUND:
//		print_dhcp_ip(cb);
//		break;
//	default:
//		break;
//	}
//}

//void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
//		uint32_t mgmt_event, struct net_if *iface)
//{
//	const struct device *dev = iface->if_dev->dev;
//	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
//
//	vif_ctx_zep = dev->data;
//
//	switch (mgmt_event) {
//	case NET_EVENT_WIFI_CONNECT_RESULT:
//		handle_wifi_connect_result(cb);
//		break;
//	case NET_EVENT_WIFI_DISCONNECT_RESULT:
//		handle_wifi_disconnect_result(cb);
//		break;
//	case NET_EVENT_WIFI_SCAN_RESULT:
//		vif_ctx_zep->scan_in_progress = 0;
//		handle_wifi_scan_result(cb);
//		break;
//	case NET_EVENT_WIFI_SCAN_DONE:
//		handle_wifi_scan_done(cb);
//		break;
//#ifdef CONFIG_TWT_ENABLE
//	case NET_EVENT_WIFI_TWT:
//		handle_wifi_twt_event(cb);
//	break;
//#endif
//	default:
//		break;
//	}
//}

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
	#ifndef CONFIG_PRINTS_FOR_AUTOMATION
		const struct wifi_status *status = (const struct wifi_status *) cb->info;
	#endif

	if (context.disconnect_requested) {
		#ifndef CONFIG_PRINTS_FOR_AUTOMATION
		LOG_INF("Disconnection request %s (%d)", status->status ? "failed" : "done",
			status->status);
		#endif
		context.disconnect_requested = false;
	} else {
		#ifndef CONFIG_PRINTS_FOR_AUTOMATION
		LOG_INF("Disconnected");
		#endif
		context.connected = false;
	}
	wifi_disconn_cnt_stability++;
#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
	cmd_wifi_status();
#endif
}

void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry =
			(const struct wifi_scan_result *)cb->info;

	scan_result_count++;

#ifdef CONFIG_DEBUG_PRINT_WIFI_SCAN_INFO
	LOG_INF("%-4d | %-12s | %-4u  | %-4d",
		scan_result_count, entry->ssid, entry->channel, entry->rssi);
	LOG_INF("Wi-Fi scan results for %d times", scan_result_count);
#endif

	if (entry->channel <= HIGHEST_CHANNUM_24G) {
		wifi_scan_cnt_24g++;
	} else {
		wifi_scan_cnt_5g++;
	}
}

void handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	wifi_scan_time = k_uptime_get_32() - wifi_scan_start_time;
	/**if (print_wifi_scan_time) {
	 *	LOG_INF("wifi_scan_time=%d",wifi_scan_time);
	 *	}
	 */
	print_wifi_scan_time++;
	wifi_scan_start_time = k_uptime_get_32();

	k_sleep(K_MSEC(1));
#ifdef ENABLE_WIFI_SCAN_TEST
	if (repeat_wifi_scan == 1) {
		wifi_scan_cmd_cnt++;
		cmd_wifi_scan();
	}
#endif

#ifdef CONFIG_DEBUG_PRINT_WIFI_SCAN_INFO
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;
	if (status->status) {
		LOG_ERR("Scan request failed (%d)", status->status);
	} else {
		LOG_INF("Scan request done");
	}
	/* in milliseconds, do not reduce further */
	k_sleep(K_MSEC(1));
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

//int __wifi_args_to_params(struct wifi_connect_req_params *params)
//{
//	params->timeout = SYS_FOREVER_MS;
//
//	/* SSID */
//	params->ssid = CONFIG_STA_SSID;
//	params->ssid_length = strlen(params->ssid);
//
//#if defined(CONFIG_STA_KEY_MGMT_WPA2)
//	params->security = 1;
//#elif defined(CONFIG_STA_KEY_MGMT_WPA2_256)
//	params->security = 2;
//#elif defined(CONFIG_STA_KEY_MGMT_WPA3)
//	params->security = 3;
//#else
//	params->security = 0;
//#endif
//
//#if !defined(CONFIG_STA_KEY_MGMT_NONE)
//	params->psk = CONFIG_STA_PASSWORD;
//	params->psk_length = strlen(params->psk);
//#endif
//	params->channel = WIFI_CHANNEL_ANY;
//
//	/* MFP (optional) */
//	params->mfp = WIFI_MFP_OPTIONAL;
//
//	return 0;
//}

//int cmd_wifi_scan(void)
//{
//	struct net_if *iface = net_if_get_first_wifi();
//	struct wifi_scan_params params = {0};
//
//	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params, sizeof(struct wifi_scan_params))) {
//		LOG_ERR("Scan request failed");
//		return -ENOEXEC;
//	}
//#ifdef CONFIG_DEBUG_PRINT_WIFI_SCAN_INFO
//	LOG_INF("Scan requested");
//#endif
//	return 0;
//}
//
//int wifi_connect(void)
//{
//	struct net_if *iface = net_if_get_first_wifi();
//	static struct wifi_connect_req_params cnx_params = {0};
//
//	/* LOG_INF("Connection requested"); */
//	__wifi_args_to_params(&cnx_params);
//
//	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
//			&cnx_params, sizeof(struct wifi_connect_req_params))) {
//		LOG_ERR("Wi-Fi Connection request failed");
//		return -ENOEXEC;
//	}
//	return 0;
//}
//
//int wifi_disconnect(void)
//{
//	struct net_if *iface = net_if_get_first_wifi();
//	int status;
//
//	context.disconnect_requested = true;
//
//	status = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
//
//	if (status) {
//		context.disconnect_requested = false;
//
//		if (status == -EALREADY) {
//			/* LOG_ERR("Already disconnected"); */
//			wifi_disconn_no_conn_cnt++;
//		} else {
//			/* LOG_ERR("Disconnect request failed"); */
//			wifi_disconn_fail_cnt++;
//			return -ENOEXEC;
//		}
//	} else {
//		wifi_disconn_success_cnt++;
//	}
//	return 0;
//}
//
//int parse_ipv4_addr(char *host, struct sockaddr_in *addr)
//{
//	int ret;
//
//	if (!host) {
//		return -EINVAL;
//	}
//	ret = net_addr_pton(AF_INET, host, &addr->sin_addr);
//	if (ret < 0) {
//		LOG_ERR("Invalid IPv4 address %s", host);
//		return -EINVAL;
//	}
//	LOG_INF("Wi-Fi peer IPv4 address %s", host);
//
//	return 0;
//}

int wait_for_next_event(const char *event_name, int timeout)
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

void run_ot_benchmark_test(void)
{
	ot_throughput_test_run();
}

void run_ot_discovery_test(void)
{
	ot_discovery_test_run();
}

void run_ot_connection_test(void)
{
	ot_conn_test_run();
}



void run_wifi_scan_test(void)
{
	wifi_scan_test_run();
}

void run_wifi_conn_test(void)
{
	wifi_connection_test_run();
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

int run_wifi_traffic_tcp(void)
{
	int ret = 0;

#ifdef CONFIG_WIFI_ZPERF_SERVER
	struct zperf_download_params params = {0};

	params.port = CONFIG_NET_CONFIG_PEER_IPV4_PORT;

//-----------------------------------get the DHCP IP address assigned
struct net_if *iface = net_if_get_first_wifi();

//#if defined(CONFIG_NET_IPV4)
struct net_if_ipv4 *ipv4;
//#endif
//#if defined(CONFIG_NET_IP)
struct net_if_addr *unicast;
//#endif
ipv4 = iface->config.ip.ipv4;

//PR("IPv4 unicast addresses (max %d):\n", NET_IF_MAX_IPV4_ADDR);
//for (i = 0; ipv4 && i < NET_IF_MAX_IPV4_ADDR; i++) {
	//unicast = &ipv4->unicast[i];
	unicast = &ipv4->unicast[0];

//	if (!unicast->is_used) {
//		continue;
//	}

	//PR("\t%s %s %s%s\n",
	//   net_sprint_ipv4_addr(&unicast->address.in_addr),
	//   addrtype2str(unicast->addr_type),
	//   addrstate2str(unicast->addr_state),
	//   unicast->is_infinite ? " infinite" : "");

//	count++;
//}

LOG_INF("DHCP IP address %s",net_sprint_ipv4_addr(&unicast->address.in_addr));
//-----------------------------------

//----------------------------------- get socket address
char *addr_str = net_sprint_ipv4_addr(&unicast->address.in_addr); //"192.168.1.254";
struct sockaddr addr;

memset(&addr, 0, sizeof(addr));

ret = net_ipaddr_parse(addr_str, strlen(addr_str), &addr);
if (ret < 0) {
	LOG_INF("Cannot parse address \"%s\"\n",
			  addr_str);
	return ret;
}
memcpy(&params.addr, &addr, sizeof(struct sockaddr));
//-----------------------------------

	ret = zperf_tcp_download(&params, tcp_download_results_cb, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to start TCP server session: %d", ret);
		return ret;
	}
	LOG_INF("TCP server started on port %u\n", params.port);
#else
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
#endif

	return 0;
}

int run_wifi_traffic_udp(void)
{
	int ret = 0;

#ifdef CONFIG_WIFI_ZPERF_SERVER
	struct zperf_download_params params = {0};

	params.port = CONFIG_NET_CONFIG_PEER_IPV4_PORT;

//-----------------------------------get the DHCP IP address assigned
struct net_if *iface = net_if_get_first_wifi();

//#if defined(CONFIG_NET_IPV4)
struct net_if_ipv4 *ipv4;
//#endif
//#if defined(CONFIG_NET_IP)
struct net_if_addr *unicast;
//#endif
ipv4 = iface->config.ip.ipv4;

//PR("IPv4 unicast addresses (max %d):\n", NET_IF_MAX_IPV4_ADDR);
//for (i = 0; ipv4 && i < NET_IF_MAX_IPV4_ADDR; i++) {
	//unicast = &ipv4->unicast[i];
	unicast = &ipv4->unicast[0];

//	if (!unicast->is_used) {
//		continue;
//	}

	//PR("\t%s %s %s%s\n",
	//   net_sprint_ipv4_addr(&unicast->address.in_addr),
	//   addrtype2str(unicast->addr_type),
	//   addrstate2str(unicast->addr_state),
	//   unicast->is_infinite ? " infinite" : "");

//	count++;
//}

LOG_INF("DHCP IP address %s",net_sprint_ipv4_addr(&unicast->address.in_addr));
//-----------------------------------

//----------------------------------- get socket address
char *addr_str = net_sprint_ipv4_addr(&unicast->address.in_addr); //"192.168.1.254";
struct sockaddr addr;

memset(&addr, 0, sizeof(addr));

ret = net_ipaddr_parse(addr_str, strlen(addr_str), &addr);
if (ret < 0) {
	LOG_INF("Cannot parse address \"%s\"\n",
			  addr_str);
	return ret;
}
memcpy(&params.addr, &addr, sizeof(struct sockaddr));
//-----------------------------------
		

	ret = zperf_udp_download(&params, udp_download_results_cb, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to start UDP server session: %d", ret);
		return ret;
	}
#else
	struct zperf_upload_params params = {0};

	/* Start Wi-Fi UDP traffic */
	LOG_INF("Starting Wi-Fi benchmark: Zperf UDP client");
	params.duration_ms = CONFIG_COEX_TEST_DURATION;
	params.rate_kbps = CONFIG_WIFI_ZPERF_RATE;
	params.packet_size = CONFIG_WIFI_ZPERF_PKT_SIZE;
	parse_ipv4_addr(CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &in4_addr_my);
	net_sprint_ipv4_addr(&in4_addr_my.sin_addr);

	memcpy(&params.peer_addr, &in4_addr_my, sizeof(in4_addr_my));
	ret = zperf_udp_upload_async(&params, udp_upload_results_cb, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to start Wi-Fi UDP benchmark: %d", ret);
		return ret;
	}
#endif

	return 0;
}

void start_wifi_activity(void)
{
	/* Start Wi-Fi scan or connection based on the test case */
#ifdef ENABLE_WIFI_SCAN_TEST
	k_thread_start(run_wlan_scan);
#endif
#ifdef ENABLE_WIFI_CONN_TEST
	k_thread_start(run_wlan_conn);
#endif
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

void run_wifi_activity(void)
{
#ifdef ENABLE_WIFI_SCAN_TEST
	k_thread_join(run_wlan_scan, K_FOREVER);
#endif
#ifdef ENABLE_WIFI_CONN_TEST
	k_thread_join(run_wlan_conn, K_FOREVER);
#endif
}

int wifi_connection(void)
{
	wifi_conn_attempt_cnt++;
	/* Wi-Fi connection */
	wifi_connect();

	if (wait_for_next_event("Wi-Fi Connection", WIFI_CONNECTION_TIMEOUT)) {
		wifi_conn_timeout_cnt++;
		wifi_conn_fail_cnt++;
		return -1;
	}
	
	wifi_conn_success_cnt++;
	
	if (wait_for_next_event("Wi-Fi DHCP", WIFI_DHCP_TIMEOUT)) {
		wifi_dhcp_timeout_cnt++;
		wifi_conn_fail_cnt++;
		return -1;
	}

	return 0;
}

void wifi_disconnection(void)
{
	int ret = 0;

	wifi_disconn_attempt_cnt++;
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

void wifi_scan_test_run(void)
{
	uint64_t test_start_time;

	test_start_time = k_uptime_get_32();

	wifi_scan_cmd_cnt++;
	cmd_wifi_scan();

	while (true) {
		if ((k_uptime_get_32() - test_start_time) > CONFIG_COEX_TEST_DURATION) {
			break;
		}
		k_sleep(K_MSEC(100)); /* in milliseconds. can be reduced to 1ms?? */
	}
}

void wifi_connection_test_run(void)
{
	uint64_t test_start_time = k_uptime_get_32();
	int ret = 0;

	while (true) {
		ret = wifi_connection();
		if (ret != 0) {
			LOG_INF("Wi-Fi connection failed");
		}
		k_sleep(KSLEEP_WIFI_CON_10MSEC);

		wifi_disconnection();
		k_sleep(KSLEEP_WIFI_DISCON_10MSEC);

		if ((k_uptime_get_32() - test_start_time) > CONFIG_COEX_TEST_DURATION) {
			break;
		}
	}
}

void start_ot_activity(void)
{
	/* Start Thread connection or throughput based on the test case */
	#ifdef ENABLE_OT_DISCOV_TEST
		k_thread_start(run_ot_discovery);
	#endif
	#ifdef ENABLE_OT_CONN_TEST
		k_thread_start(run_ot_connection);
	#endif
	#ifdef ENABLE_OT_TRAFFIC_TEST
		k_thread_start(run_ot_traffic);
	#endif
}

void run_ot_activity(void)
{
	/* In case Thread is server, skip running Thread connection/traffic */
	#ifdef ENABLE_OT_DISCOV_TEST
		k_thread_join(run_ot_discovery, K_FOREVER);
	#endif
	#ifdef ENABLE_OT_CONN_TEST
		k_thread_join(run_ot_connection, K_FOREVER);
	#endif
	#ifdef ENABLE_OT_TRAFFIC_TEST
		k_thread_join(run_ot_traffic, K_FOREVER);
	#endif
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

void print_ot_connection_test_params(bool is_ot_client)
{
	//if (is_ot_client) {
	//	LOG_INF("Thread Scan interval max %u\n", CONFIG_BT_LE_SCAN_INTERVAL);
	//	LOG_INF("Thread Scan window %u\n", CONFIG_BT_LE_SCAN_WINDOW);
	//} else {
	//	LOG_INF("Thread advertisement interval min %u\n", CONFIG_BT_GAP_ADV_FAST_INT_MIN_2);
	//	LOG_INF("Thread advertisement interval max %u\n", CONFIG_BT_GAP_ADV_FAST_INT_MAX_2);
	//}
	//LOG_INF("Thread connection interval min %u\n", CONFIG_BT_INTERVAL_MIN);
	//LOG_INF("Thread connection interval max %u\n", CONFIG_BT_INTERVAL_MAX);
	//LOG_INF("Thread supervision timeout %u\n", CONFIG_BT_SUPERVISION_TIMEOUT);
	//LOG_INF("Thread connection latency %u\n", CONFIG_BT_CONN_LATENCY);
}


int wifi_scan_ot_discov(bool is_ant_mode_sep, bool test_thread, bool test_wifi,
		bool is_ot_client, bool is_wifi_conn_scan)
{
	uint64_t test_start_time = 0;
	int ret = 0;

	/* Wi-Fi client/server role has no meaning in Wi-Fi scan */
	bool is_wifi_server = false;
	//LOG_INF("test_wifi=%d", test_wifi);
	//LOG_INF("test_thread=%d", test_thread);
	
	if (is_ot_client) {
		if (is_wifi_conn_scan) {
			LOG_INF("Test case: wifi_scan_ot_discov");
			LOG_INF("Wi-Fi connected scan, thread client");
		} else {
			LOG_INF("Test case: wifi_scan_ot_discov");
			LOG_INF("Wi-Fi scan, thread client");
		}
	} else {
		if (is_wifi_conn_scan) {
			LOG_INF("Test case: wifi_scan_ot_discov");
			LOG_INF("Wi-Fi connected scan, thread server");
		} else {
			LOG_INF("Test case: wifi_scan_ot_discov");
			LOG_INF("Wi-Fi scan,  thread server");
		}
	}

	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);	

	if (test_wifi) {
		if (is_wifi_conn_scan) {
			/* for connected scan */
#ifndef CHECK_WIFI_CONN_STATUS
			wifi_connection();
#else
			ret = wifi_connection();
			k_sleep(K_SECONDS(3));
			if (ret != 0) {
				LOG_ERR("Wi-Fi connection failed. Running the test");
				LOG_ERR("further is not meaningful. So, exiting the test");
				return ret;
			}
#endif
		}
#if defined(CONFIG_NRF700X_BT_COEX)
			config_pta(is_ant_mode_sep, is_ot_client, is_wifi_server);
#endif/* CONFIG_NRF700X_BT_COEX */
	}
	if (test_thread) {
		/* Initialize Thread by selecting role and connect it to peer device. */
		ot_discov_attempt_cnt++;
		ot_initialization();
		k_sleep(K_SECONDS(3));

		if (is_ot_client) {
			/**If Thread is client, disconnect the connection.
			 * Connection and disconnection happens in loop later.
			 */
			//ot_disconnection_attempt_cnt++;
			//ot_disconnect_client();
		} 
		//note: Thread connection is done in default client role. 
		//else {
		//	/**If Thread is server, wait until peer Thread client
		//	 *  initiates the connection, DUT is connected to peer client
		//	 *  and update the PHY parameters.
		//	 */
		//	while (true) {
		//		if (ot_server_connected) {
		//			break;
		//		}
		//		k_sleep(KSLEEP_WHILE_PERIP_CONN_CHECK_1SEC);
		//	}
		//}
	}

	if (test_thread) {
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
		/* peer client waits on this in automation */
		LOG_INF("Run Thread client");
		k_sleep(K_SECONDS(1));
#endif

		//if (!is_ot_client) {
		//	LOG_INF("DUT is in server role.");
		//	LOG_INF("Check for Thread connection counts on peer Thread side.");
		//}
	}

#ifdef DEMARCATE_TEST_START
		LOG_INF("-------------------------start");
#endif

	repeat_wifi_scan = 1;
	repeat_ot_discovery = 1;
	test_start_time = k_uptime_get_32();

	/* Begin Thread discovery for a period of Thread test duration */
	if (test_thread) {
		if (is_ot_client) {
			start_ot_activity();
		} else {
			/* If DUT Thread is server then the peer starts the activity. */
		}
	}

	/* Begin Wi-Fi scan and continue for test duration */
	if (test_wifi) {
		start_wifi_activity();
	}

	/* Wait for Thread activity completion i.e., for test duration */
	if (test_thread) {
		if (is_ot_client) {
			/* Run Thread activity and wait for the test duration */
			run_ot_activity();
		} else {
			/** If DUT Thread is in server role then peer Thread runs the activity.
			 *wait for test duration
			 */
			while (1) {
				if ((k_uptime_get_32() - test_start_time) >
				CONFIG_COEX_TEST_DURATION) {
					break;
				}
				k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
			}
		}
	}
	/* Wait for the completion of Wi-Fi scan and disconnect Wi-Fi if connected scan */
	if (test_wifi) {
		run_wifi_activity();
		/* Disconnect wifi if connected scan */
		if (is_wifi_conn_scan) {
			wifi_disconnection();
		}
	}
	/* Stop further Wi-Fi scan*/
	repeat_wifi_scan = 0;
	repeat_ot_discovery = 0;

#ifdef DEMARCATE_TEST_START
		LOG_INF("-------------------------end");
#endif

	if (test_thread) {
		//if (is_ot_client) {
			ot_device_disable(); // to disable thread device after the test completion
		//}
	}
	
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	ot_discov_attempts_before_test_starts = 0;
	if (test_thread) {
		LOG_INF("ot_discov_attempt_cnt = %u",
		ot_discov_attempt_cnt - ot_discov_attempts_before_test_starts);
		LOG_INF("ot_discov_success_cnt = %u",
			ot_discov_success_cnt -	ot_discov_attempts_before_test_starts);
		#if 0
		LOG_INF("ot_discov_timeout = %u",	ot_discov_timeout); // this can be commented. as ot_discov_no_result_cnt is available.
		#endif
		LOG_INF("ot_discov_no_result_cnt = %u",	ot_discov_no_result_cnt);
	}
	if (test_wifi) {
		LOG_INF("wifi_scan_cmd_cnt = %u", wifi_scan_cmd_cnt);
		LOG_INF("wifi_scan_cnt_24g = %u", wifi_scan_cnt_24g);
		LOG_INF("wifi_scan_cnt_5g = %u", wifi_scan_cnt_5g);
	}	
#endif

	return 0;
}

int wifi_scan_ot_connection(bool is_ant_mode_sep, bool test_thread, bool test_wifi,
		bool is_ot_client, bool is_wifi_conn_scan)
{
	uint64_t test_start_time = 0;

	/* Wi-Fi client/server role has no meaning in Wi-Fi scan */
	bool is_wifi_server = false;
	LOG_INF("test_wifi=%d", test_wifi);
		
	if (is_ot_client) {
		if (is_wifi_conn_scan) {
			LOG_INF("Test case: wifi_scan_ot_connection");
			LOG_INF("Wi-Fi connected scan, Thread client");
		} else {
			LOG_INF("Test case: wifi_scan_ot_connection");
			LOG_INF("Wi-Fi scan, Thread client");
		}
	} else {
		if (is_wifi_conn_scan) {
			LOG_INF("Test case: wifi_scan_ot_connection");
			LOG_INF("Wi-Fi connected scan, Thread server");
		} else {
			LOG_INF("Test case: wifi_scan_ot_connection");
			LOG_INF("Wi-Fi scan, Thread server");
		}
	}

	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);
	//print_ot_connection_test_params(is_ot_client);

	if (test_wifi) {
		if (is_wifi_conn_scan) {
			/* for connected scan */
			int ret = 0;

			ret = wifi_connection();
			k_sleep(K_SECONDS(3));
			if (ret != 0) {
				LOG_ERR("Wi-Fi connection failed. Running the test");
				LOG_ERR("further is not meaningful. So, exiting the test");
				return ret;
			}
		}
		#if defined(CONFIG_NRF700X_BT_COEX)
			config_pta(is_ant_mode_sep, is_ot_client, is_wifi_server);
		#endif/* CONFIG_NRF700X_BT_COEX */
	}
	if (test_thread) {
		/* Initialize Thread by selecting role and connect it to peer device. */
		ot_connection_attempt_cnt++;
		ot_connection_init(is_ot_client);
		k_sleep(K_SECONDS(3));

		if (is_ot_client) {
			/**If Thread is client, disconnect the connection.
			 * Connection and disconnection happens in loop later.
			 */
			ot_disconnection_attempt_cnt++;
			ot_disconnect_client();
		}
		// Thread connection is done using default client role
		//else {
		//	/**If Thread is server, wait until peer Thread client
		//	 *  initiates the connection, DUT is connected to peer client
		//	 *  and update the PHY parameters.
		//	 */
		//	while (true) {
		//		if (ot_server_connected) {
		//			break;
		//		}
		//		k_sleep(KSLEEP_WHILE_PERIP_CONN_CHECK_1SEC);
		//	}
		//}
	}

	if (test_thread) {
		#ifdef CONFIG_PRINTS_FOR_AUTOMATION
		/* peer client waits on this in automation */
		LOG_INF("Run Thread client");
		k_sleep(K_SECONDS(1));
		#endif

		//if (!is_ot_client) {
		//	LOG_INF("DUT is in server role.");
		//	LOG_INF("Check for Thread connection counts on peer Thread side.");
		//}
	}

	#ifdef DEMARCATE_TEST_START
		LOG_INF("-------------------------start");
	#endif

	repeat_wifi_scan = 1;
	test_start_time = k_uptime_get_32();

	/* Begin Thread conections and disconnections for a period of Thread test duration */
	if (test_thread) {
		if (is_ot_client) {
			start_ot_activity();
		} else {
			/* If DUT Thread is server then the peer starts the activity. */
		}
	}

	/* Begin Wi-Fi scan and continue for test duration */
	if (test_wifi) {
		start_wifi_activity();
	}

	/* Wait for Thread activity completion i.e., for test duration */
	if (test_thread) {
		if (is_ot_client) {
			/* Run Thread activity and wait for the test duration */
			run_ot_activity();
		} else {
			/** If DUT Thread is in server role then peer Thread runs the activity.
			 *wait for test duration
			 */
			while (1) {
				if ((k_uptime_get_32() - test_start_time) >
				CONFIG_COEX_TEST_DURATION) {
					break;
				}
				k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
			}
		}
	}
	/* Wait for the completion of Wi-Fi scan and disconnect Wi-Fi if connected scan */
	if (test_wifi) {
		run_wifi_activity();
		/* Disconnect wifi if connected scan */
		if (is_wifi_conn_scan) {
			wifi_disconnection();
		}
	}
	/* Stop further Wi-Fi scan*/
	repeat_wifi_scan = 0;
#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
#endif

#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	ot_conn_attempts_before_test_starts = 0;
	if (test_thread) {
		//if (is_ot_client) {
		LOG_INF("ot_connection_attempt_cnt = %u",
			ot_connection_attempt_cnt -
			ot_conn_attempts_before_test_starts);
		LOG_INF("ot_connection_success_cnt = %u",
			ot_connection_success_cnt -
			ot_conn_attempts_before_test_starts);


		LOG_INF("ot_disconnection_attempt_cnt = %u",
			ot_disconnection_attempt_cnt);
		LOG_INF("ot_disconnection_success_cnt = %u",
			ot_disconnection_success_cnt);
		LOG_INF("ot_disconnection_fail_cnt = %u",
			ot_disconnection_fail_cnt);
		LOG_INF("ot_discon_no_conn_cnt = %u",
			ot_discon_no_conn_cnt);
	}
	if (test_wifi) {
		LOG_INF("wifi_scan_cmd_cnt = %u", wifi_scan_cmd_cnt);
		LOG_INF("wifi_scan_cnt_24g = %u", wifi_scan_cnt_24g);
		LOG_INF("wifi_scan_cnt_5g = %u", wifi_scan_cnt_5g);
	}
	#endif

	return 0;
}


int wifi_scan_ot_tput(bool is_ant_mode_sep, bool test_thread, bool test_wifi,
			bool is_ot_client, bool is_wifi_conn_scan)
{
	int ret = 0;
	uint64_t test_start_time = 0;

	/* Wi-Fi client/server role has no meaning in Wi-Fi scan*/
	bool is_wifi_server = false;

	if (is_ot_client) {
		if (is_wifi_conn_scan) {
			LOG_INF("Test case: wifi_scan_ot_tput");
			LOG_INF("Wi-Fi connected scan, Thread client");
		} else {
			LOG_INF("Test case: wifi_scan_ot_tput");
			LOG_INF("Wi-Fi scan, Thread client");
		}
	} else {
		if (is_wifi_conn_scan) {
			LOG_INF("Test case: wifi_scan_ot_tput");
			LOG_INF("Wi-Fi connected scan, Thread server");
		} else {
			LOG_INF("Test case: wifi_scan_ot_tput");
			LOG_INF("Wi-Fi scan, Thread server");
		}
	}

	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);

	if (is_ot_client) {
		is_ot_device_role_client = true;
	} else {
		is_ot_device_role_client = false;
	}
	
	if (test_wifi) {
		if (is_wifi_conn_scan) {
			ret = wifi_connection(); /* for connected scan */
			k_sleep(K_SECONDS(3));
			if (ret != 0) {
				LOG_ERR("Wi-Fi connection failed. Running the test");
				LOG_ERR("further is not meaningful. So, exiting the test");
				return ret;
			}
		}
		#if defined(CONFIG_NRF700X_BT_COEX)
			config_pta(is_ant_mode_sep, is_ot_client, is_wifi_server);
		#endif/* CONFIG_NRF700X_BT_COEX */
	}

	if (test_thread) {	
		
		ret = ot_throughput_test_init(is_ot_client);
		k_sleep(K_SECONDS(3));
		if (ret != 0) {
			LOG_ERR("Thread throughput init failed: %d", ret);
			return ret;
		}
		
		if (is_ot_client) {
			/* nothing */
		} else {
			/* wait until the peer client joins the network */
			while (wait4_ping_reply_from_peer==0) {				
					LOG_INF("Waiting on ping reply from peer");
					get_peer_address(5000);
					k_sleep(K_SECONDS(1));
					if(wait4_ping_reply_from_peer) {
						break;
					}
			}		
		}
	}

	repeat_wifi_scan = 1;

	if (test_thread) {
		/** In case of client, start Thread traffic for OT_TEST_DURATION.
		 * In case of server, peer device begins the traffic.
		 */
		if (is_ot_client) {
#ifdef DEMARCATE_TEST_START
			LOG_INF("-------------------------start");
#endif
			test_start_time = k_uptime_get_32();
			start_ot_activity();
		} else {
			/* If DUT Thread is server then the peer client starts zperf Tx */
			start_ot_activity(); /* starts zperf Rx */
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
			while (!wait4_peer_ot2_start_connection) {
				/* Peer Thread starts the the test. */
				LOG_INF("Run Thread client on peer");
				k_sleep(K_SECONDS(1));
			}
			wait4_peer_ot2_start_connection = 0;
#endif
#ifdef DEMARCATE_TEST_START
				LOG_INF("-------------------------start");
#endif
			test_start_time = k_uptime_get_32();
		}
	}

	/* Begin Wi-Fi scan and repeat it for Test Duration period. */
	if (test_wifi) {
		start_wifi_activity();
	}

	if (test_thread) {
		if (is_ot_client) {
			/* Run Thread activity and wait for test duration */
			run_ot_activity();
			ot_throughput_test_exit();
		} else {
			/** If Thread is server, peer Thread runs the activity.
			 * Wait for the test duration.
			 */
			while (1) {
				if ((k_uptime_get_32() - test_start_time) >
					CONFIG_COEX_TEST_DURATION) {
					break;
				}
				k_sleep(K_MSEC(100));
			}
		}
	}
	/* Wait for the completion of Wi-Fi activity i.e., for COEX_TEST_DURATION */
	if (test_wifi) {
		run_wifi_activity();
		/* Disconnect wifi if connected scan */
		if (is_wifi_conn_scan) {
			wifi_disconnection();
		}
	}
	/* Stop further Wi-Fi scan*/
	repeat_wifi_scan = 0;

#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
#endif
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	if (test_wifi) {
		LOG_INF("wifi_scan_cmd_cnt = %u", wifi_scan_cmd_cnt);
		LOG_INF("wifi_scan_cnt_24g = %u", wifi_scan_cnt_24g);
		LOG_INF("wifi_scan_cnt_5g = %u", wifi_scan_cnt_5g);
	}
#endif
	return 0;
}


int wifi_con_ot_discov(bool test_wifi, bool is_ant_mode_sep,	bool test_thread, bool is_ot_client)
{
	int ret = 0;
	uint64_t test_start_time = 0;
	/* Wi-Fi client/server role has no meaning in the Wi-Fi connection. */
	/* Thread client/server role has no meaning in the Thread connection. */
	bool is_wifi_server = false;
	
	if (is_ot_client) {
		LOG_INF("Test case: wifi_con_ot_disc, Thread client");
	} else {
		LOG_INF("Test case: wifi_con_ot_disc, Thread server");
	}


	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);

	if (test_wifi) {
#if defined(CONFIG_NRF700X_BT_COEX)
		config_pta(is_ant_mode_sep, is_ot_client, is_wifi_server);
#endif/* CONFIG_NRF700X_BT_COEX */
	}

	if (test_thread) {
		/* Initialize Thread by selecting role and connect it to peer device. */
		ot_discov_attempt_cnt++;
		ot_initialization();
		k_sleep(K_SECONDS(3));

		if (is_ot_client) {
			/**If Thread is client, disconnect the connection.
			 * Connection and disconnection happens in loop later.
			 */
			//ot_disconnection_attempt_cnt++;
			//ot_disconnect_client();
		} 
		/* Thread client/server role has no meaning in the Thread connection. */
		//else {
		//	/**If Thread is server, wait until peer Thread client
		//	 *  initiates the connection, DUT is connected to peer client
		//	 *  and update the PHY parameters.
		//	 */
		//	while (true) {
		//		if (ot_server_connected) {
		//			break;
		//		}
		//		k_sleep(KSLEEP_WHILE_PERIP_CONN_CHECK_1SEC);
		//	}
		//}
	}

	if (test_thread) {
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
		/* peer client waits on this in automation */
		LOG_INF("Run Thread client");
		k_sleep(K_SECONDS(1));
#endif

		//if (!is_ot_client) {
		//	LOG_INF("DUT is in server role.");
		//	LOG_INF("Check for Thread connection counts on peer Thread side.");
		//}
	}

#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------start");
#endif
	repeat_ot_discovery = 1;
	test_start_time = k_uptime_get_32();

	/* start Wi-Fi connection and disconnection sequence for test duration period */
	if (test_wifi) {
		start_wifi_activity();
	}

	/* Begin Thread discovery for a period of Thread test duration */
	if (test_thread) {
		if (is_ot_client) {
			start_ot_activity();
		} 
		/* Thread client/server role has no meaning in the Thread connection. */	
		//else {
		//	/* If DUT Thread is server then the peer starts the activity. */
		//}
	}

	/* Wait for Thread activity completion i.e., for test duration */
	if (test_thread) {
		if (is_ot_client) {
			/* Run Thread activity and wait for the test duration */
			run_ot_activity();
		}
		/* Thread client/server role has no meaning in the Thread connection. */		
		//else {
		//	/** If DUT Thread is in server role then peer Thread runs the activity.
		//	 *wait for test duration
		//	 */
		//	while (1) {
		//		if ((k_uptime_get_32() - test_start_time) >
		//		CONFIG_COEX_TEST_DURATION) {
		//			break;
		//		}
		//		k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
		//	}
		//}
	}

	if (test_wifi) {
		/* Wait for the completion of Wi-Fi activity */
		run_wifi_activity();
	}
	repeat_ot_discovery = 0;
#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
#endif
	if (test_thread) {
		//if (is_ot_client) {
			ot_device_disable(); // to disable thread device after the test completion
		//}
	}
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	ot_discov_attempts_before_test_starts = 0;
	if (test_thread) {
		LOG_INF("ot_discov_attempt_cnt = %u",
		ot_discov_attempt_cnt - ot_discov_attempts_before_test_starts);
		LOG_INF("ot_discov_success_cnt = %u",
			ot_discov_success_cnt -	ot_discov_attempts_before_test_starts);
		#if 0
		LOG_INF("ot_discov_timeout = %u",	ot_discov_timeout); // this can be commented. as ot_discov_no_result_cnt is available.
		#endif
		LOG_INF("ot_discov_no_result_cnt = %u",	ot_discov_no_result_cnt);
	}
	if (test_wifi) {
		LOG_INF("wifi_conn_attempt_cnt = %u", wifi_conn_attempt_cnt);
		LOG_INF("wifi_conn_success_cnt = %u", wifi_conn_success_cnt);
		LOG_INF("wifi_conn_fail_cnt = %u", wifi_conn_fail_cnt);
		LOG_INF("wifi_conn_timeout_cnt = %u", wifi_conn_timeout_cnt);
		LOG_INF("wifi_dhcp_timeout_cnt = %u", wifi_dhcp_timeout_cnt);
		LOG_INF("wifi_disconn_attempt_cnt = %u", wifi_disconn_attempt_cnt);
		LOG_INF("wifi_disconn_success_cnt = %u", wifi_disconn_success_cnt);
		LOG_INF("wifi_disconn_fail_cnt = %u", wifi_disconn_fail_cnt);
		LOG_INF("wifi_disconn_no_conn_cnt = %u", wifi_disconn_no_conn_cnt);
	}
#endif
	return 0;
}

int wifi_con_ot_tput(bool test_wifi, bool is_ant_mode_sep,	bool test_thread, bool is_ot_client)
{
	int ret = 0;
	uint64_t test_start_time = 0;
	/* Wi-Fi clinet/server role has no meaning in the Wi-Fi connection. */
	bool is_wifi_server = false;

	if (is_ot_client) {
		LOG_INF("Test case: wifi_con_ot_tput, Thread client");
	} else {
		LOG_INF("Test case: wifi_con_ot_tput, Thread server");
	}

	LOG_INF("test_wifi = %d", test_wifi);
	LOG_INF("test_thread = %d", test_thread);

	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);

	if (is_ot_client) {
		is_ot_device_role_client = true;
	} else {
		is_ot_device_role_client = false;
	}
	
	if (test_wifi) {
#if defined(CONFIG_NRF700X_BT_COEX)
		config_pta(is_ant_mode_sep, is_ot_client, is_wifi_server);
#endif/* CONFIG_NRF700X_BT_COEX */
	}

	if (test_thread) {
		ret = ot_throughput_test_init(is_ot_client);
		k_sleep(K_SECONDS(3));
		if (ret != 0) {
			LOG_ERR("Thread throughput init failed: %d", ret);
			return ret;
		}
		
		if (is_ot_client) {
			/* nothing */
		} else {
			/* wait until the peer client joins the network */
			while (wait4_ping_reply_from_peer==0) {				
					LOG_INF("Waiting on ping reply from peer");
					get_peer_address(5000);
					k_sleep(K_SECONDS(1));
					if(wait4_ping_reply_from_peer) {
						break;
					}
			}		
		}
	}

#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------start");
#endif

	/* start Wi-Fi connection and disconnection sequence for test duration period */
	if (test_wifi) {
		start_wifi_activity();
	}

	if (test_thread) {
		/** Start Thread traffic for OT_TEST_DURATION. In case of server,
		 *peer device begins traffic, this is a dummy function
		 */
		if (is_ot_client) {
			test_start_time = k_uptime_get_32();
			start_ot_activity();
		} else {
			/* If DUT Thread is server then the peer client starts zperf Tx */
			start_ot_activity(); /* starts zperf Rx */
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
			while (!wait4_peer_ot2_start_connection) {
				/* Peer Thread starts the the test. */
				LOG_INF("Run Thread client on peer");
				k_sleep(K_SECONDS(1));
			}
			wait4_peer_ot2_start_connection = 0;
#endif

#ifdef DEMARCATE_TEST_START
			LOG_INF("-------------------------start");
#endif
			test_start_time = k_uptime_get_32();
		}

		if (is_ot_client) {
			/* Run Thread activity and wait for test duration */
			run_ot_activity();
			ot_throughput_test_exit();
		} else {
			/** If Thread is server then peer runs the Thread activity.
			 *wait for Thread test to complete.
			 */
			while (1) {
				if ((k_uptime_get_32() - test_start_time) >
					CONFIG_COEX_TEST_DURATION) {
					break;
				}
				k_sleep(K_MSEC(100));
			}
		}
	}
	if (test_wifi) {
		/* Wait for the completion of Wi-Fi activity */
		run_wifi_activity();
	}
#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
#endif
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	if (test_wifi) {
		LOG_INF("wifi_conn_attempt_cnt = %u", wifi_conn_attempt_cnt);
		LOG_INF("wifi_conn_success_cnt = %u", wifi_conn_success_cnt);
		LOG_INF("wifi_conn_fail_cnt = %u", wifi_conn_fail_cnt);
		LOG_INF("wifi_conn_timeout_cnt = %u", wifi_conn_timeout_cnt);
		LOG_INF("wifi_dhcp_timeout_cnt = %u", wifi_dhcp_timeout_cnt);
		LOG_INF("wifi_disconn_attempt_cnt = %u", wifi_disconn_attempt_cnt);
		LOG_INF("wifi_disconn_success_cnt = %u", wifi_disconn_success_cnt);
		LOG_INF("wifi_disconn_fail_cnt = %u", wifi_disconn_fail_cnt);
		LOG_INF("wifi_disconn_no_conn_cnt = %u", wifi_disconn_no_conn_cnt);
	}
#endif
	return 0;
}


int wifi_tput_ot_discov(bool test_wifi, bool test_thread, bool is_ot_client,
		bool is_wifi_server, bool is_ant_mode_sep, bool is_wifi_zperf_udp)
{
	int ret = 0;
	uint64_t test_start_time = 0;

	//LOG_INF("test_wifi=%d", test_wifi);
	//LOG_INF("test_thread=%d", test_thread);
	
	if (is_ot_client) {
		if (is_wifi_server) {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case: wifi_tput_ot_discov");
				LOG_INF("thread client, Wi-Fi UDP server");
			} else {
				LOG_INF("thread client, Wi-Fi TCP server");
			}
		} else {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case: wifi_tput_ot_discov");
				LOG_INF("thread client, Wi-Fi UDP client");
			} else {
				LOG_INF("thread client, Wi-Fi TCP client");
			}
		}
	} else {
		if (is_wifi_server) {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case: wifi_tput_ot_discov");
				LOG_INF("thread server, Wi-Fi UDP server");
			} else {
				LOG_INF("thread server, Wi-Fi TCP server");
			}
		} else {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case: wifi_tput_ot_discov");
				LOG_INF("thread server, Wi-Fi UDP client");
			} else {
				LOG_INF("thread server, Wi-Fi TCP client");
			}
		}
	}

	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);
	//print_ot_connection_test_params(is_ot_client);

	if (test_wifi) {
#ifndef CHECK_WIFI_CONN_STATUS
		wifi_connection();
#else
		ret = wifi_connection();
		k_sleep(K_SECONDS(3));
		if (ret != 0) {
			LOG_ERR("Wi-Fi connection failed. Running the test");
			LOG_ERR("further is not meaningful. So, exiting the test");
			return ret;
		}
#endif
#if defined(CONFIG_NRF700X_BT_COEX)
		config_pta(is_ant_mode_sep, is_ot_client, is_wifi_server);
#endif/* CONFIG_NRF700X_BT_COEX */
	}
	if (test_thread) {
		/* Initialize Thread */
		//ot_discov_attempt_cnt++;
		ot_initialization();
		k_sleep(K_SECONDS(3)); /* B4 start. not in loop. no need to reduce */
		if (is_ot_client) {
			/* nothing */
		} else {
			/**If Thread is server, wait until peer Thread client
			 * initiates the connection, DUT is connected to peer client
			 *and update the PHY parameters.
			 */
			while (true) {
				if (ot_server_connected) {
					break;
				}
				k_sleep(KSLEEP_WHILE_PERIP_CONN_CHECK_1SEC);
			}
		}
	}

	if (!is_wifi_server) {
		if (is_ot_client) {
			/* nothing */
		} else {
			if (test_wifi && test_thread) {
				#ifdef CONFIG_PRINTS_FOR_AUTOMATION
				while (!run_ot_client_wait_in_conn) {
						/* Peer Thread starts the the test. */
						LOG_INF("Run Thread client on peer");
						k_sleep(K_SECONDS(1));
				}
				run_ot_client_wait_in_conn = 0;
				#endif
			}
		}
	}

	#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------start");
	#endif

	/* Begin Thread conections and disconnections for a period of Thread test duration */
	if (test_wifi) {
		if (is_wifi_zperf_udp) {
			ret = run_wifi_traffic_udp();
		} else {
			ret = run_wifi_traffic_tcp();
		}
		if (ret != 0) {
			LOG_ERR("Failed to start Wi-Fi benchmark: %d", ret);
			return ret;
		}
	}
	if (test_wifi) {
		if (is_wifi_server) {
			while (!wait4_peer_wifi_client_to_start_tp_test) {
				#ifdef CONFIG_PRINTS_FOR_AUTOMATION
				LOG_INF("start Wi-Fi client on peer");
				#endif
				k_sleep(K_SECONDS(1));
			}
			wait4_peer_wifi_client_to_start_tp_test = 0;
		}
	}
	test_start_time = k_uptime_get_32();
	repeat_ot_discovery = 1;

	if (test_thread) {
		if (is_ot_client) {
			start_ot_activity();
		} else {
			/* If DUT Thread is server then the peer starts the activity. */
		}
	}

	/* Wait for Thread activity completion i.e., for test duration */
	if (test_thread) {
		if (is_ot_client) {
			/* run Thread activity and wait for the test duration */
			run_ot_activity();
		} else {
			/** If DUT Thread is in server role, peer Thread runs the test.
			 *wait for test duration.
			 */
			while (1) {
				if ((k_uptime_get_32() - test_start_time) >
				CONFIG_COEX_TEST_DURATION) {
					break;
				}
				k_sleep(K_SECONDS(3)); // reduce this
			}
		}
	}

	if (test_wifi) {
		check_wifi_traffic();
		wifi_disconnection();
	}
	if (test_thread) {
		//if (is_ot_client) {
			ot_device_disable(); // to disable thread device after the test completion
		//}
	}

	#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
	#endif
	
	repeat_ot_discovery = 0;

	#ifdef CONFIG_PRINTS_FOR_AUTOMATION
		ot_discov_attempts_before_test_starts = 0;
		if (test_thread) {
			if (is_ot_client) {
			LOG_INF("ot_discov_attempt_cnt = %u",
				ot_discov_attempt_cnt - ot_discov_attempts_before_test_starts);
			LOG_INF("ot_discov_success_cnt = %u",
				ot_discov_success_cnt - ot_discov_attempts_before_test_starts);
			#if 0
			LOG_INF("ot_discov_timeout = %u",	ot_discov_timeout); // this can be commented. as ot_discov_no_result_cnt is available.
			#endif
			LOG_INF("ot_discov_no_result_cnt = %u",	ot_discov_no_result_cnt);
			}
		}
	#endif


	return 0;
}

int wifi_tput_ot_connection(bool test_wifi, bool test_thread, bool is_ot_client,
		bool is_wifi_server, bool is_ant_mode_sep, bool is_wifi_zperf_udp)
{
	int ret = 0;
	uint64_t test_start_time = 0;

	if (is_ot_client) {
		if (is_wifi_server) {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case: wifi_tput_ot_connection");
				LOG_INF("Thread client, Wi-Fi UDP server");
			} else {
				LOG_INF("Thread client, Wi-Fi TCP server");
			}
		} else {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case: wifi_tput_ot_connection");
				LOG_INF("Thread client, Wi-Fi UDP client");
			} else {
				LOG_INF("Thread client, Wi-Fi TCP client");
			}
		}
	} else {
		if (is_wifi_server) {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case: wifi_tput_ot_connection");
				LOG_INF("Thread server, Wi-Fi UDP server");
			} else {
				LOG_INF("Thread server, Wi-Fi TCP server");
			}
		} else {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case: wifi_tput_ot_connection");
				LOG_INF("Thread server, Wi-Fi UDP client");
			} else {
				LOG_INF("Thread server, Wi-Fi TCP client");
			}
		}
	}

	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);
	//print_ot_connection_test_params(is_ot_client);

	if (test_wifi) {
		ret=wifi_connection();
		k_sleep(K_SECONDS(3));
		if (ret != 0) {
			LOG_ERR("Wi-Fi connection failed. Running the test");
			LOG_ERR("further is not meaningful.So, exiting the test");
			return ret;
		}
		#if defined(CONFIG_NRF700X_BT_COEX)
			config_pta(is_ant_mode_sep, is_ot_client, is_wifi_server);
		#endif/* CONFIG_NRF700X_BT_COEX */
	}
	if (test_thread) {
		/* Initialize Thread by selecting role and connect it to peer device. */
		//ot_connection_attempt_cnt++;
		ot_connection_init(is_ot_client);
		k_sleep(K_SECONDS(3)); /* B4 start. not in loop. no need to reduce */
		if (is_ot_client) {
			/** If Thread is client, disconnect the connection.
			 *Connection and disconnection happens in loop later.
			 */
			//ot_disconnection_attempt_cnt++;
			//ot_disconnect_client();
			//k_sleep(K_SECONDS(2));
		} else {
			/**If Thread is server, wait until peer Thread client
			 * initiates the connection, DUT is connected to peer client
			 *and update the PHY parameters.
			 */
			while (true) {
				if (ot_server_connected) {
					break;
				}
				k_sleep(KSLEEP_WHILE_PERIP_CONN_CHECK_1SEC);
			}
		}
	}

	if (!is_wifi_server) {
		if (is_ot_client) {
			/* nothing */
		} else {
			if (test_wifi && test_thread) {
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
				while (!run_ot_client_wait_in_conn) {
					/* Peer Thread starts the the test. */
					LOG_INF("Run Thread client on peer");
					k_sleep(K_SECONDS(1));
				}
				run_ot_client_wait_in_conn = 0;
#endif
			}
		}
	}
#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------start");
#endif

	/* Begin Thread conections and disconnections for a period of Thread test duration */
	if (test_wifi) {
		if (is_wifi_zperf_udp) {
			ret = run_wifi_traffic_udp();
		} else {
			ret = run_wifi_traffic_tcp();
		}
		if (ret != 0) {
			LOG_ERR("Failed to start Wi-Fi benchmark: %d", ret);
			return ret;
		}
	}
	if (test_wifi) {
		if (is_wifi_server) {
			while (!wait4_peer_wifi_client_to_start_tp_test) {
				#ifdef CONFIG_PRINTS_FOR_AUTOMATION
				LOG_INF("start Wi-Fi client on peer");
				#endif
				k_sleep(K_SECONDS(1));
			}
			wait4_peer_wifi_client_to_start_tp_test = 0;
		}
	}
	test_start_time = k_uptime_get_32();
	if (test_thread) {
		if (is_ot_client) {
			start_ot_activity();
		} else {
			/* If DUT Thread is server then the peer starts the activity. */
		}
	}

	/* Wait for Thread activity completion i.e., for test duration */
	if (test_thread) {
		if (is_ot_client) {
			/* run Thread activity and wait for the test duration */
			run_ot_activity();
		} else {
			/** If DUT Thread is in server role, peer Thread runs the test.
			 *wait for test duration.
			 */
			while (1) {
				if ((k_uptime_get_32() - test_start_time) >
				CONFIG_COEX_TEST_DURATION) {
					break;
				}
				k_sleep(K_SECONDS(3));
			}
		}
	}

	if (test_wifi) {
		check_wifi_traffic();
		wifi_disconnection();
	}
	if (test_thread) {
		if (is_ot_client) {
			/* to stop scan after the test duration is complete */
			//scan_init();
		}
	}

#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
#endif

#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	ot_conn_attempts_before_test_starts = 0;
	if (test_thread) {
		//if (is_ot_client) {
		LOG_INF("ot_connection_attempt_cnt = %u",
			ot_connection_attempt_cnt -
			ot_conn_attempts_before_test_starts);
		LOG_INF("ot_connection_success_cnt = %u",
			ot_connection_success_cnt -
			ot_conn_attempts_before_test_starts);

		LOG_INF("ot_disconnection_attempt_cnt = %u",
			ot_disconnection_attempt_cnt);
		LOG_INF("ot_disconnection_success_cnt = %u",
			ot_disconnection_success_cnt);
		LOG_INF("ot_disconnection_fail_cnt = %u",
			ot_disconnection_fail_cnt);
		LOG_INF("ot_discon_no_conn_cnt = %u", ot_discon_no_conn_cnt);
		} //else {
			/* counts for server case are printed on peer Thread */
		//}
#endif

	return 0;
}
int wifi_tput_ot_tput(bool test_wifi, bool is_ant_mode_sep,
	bool test_thread, bool is_ot_client, bool is_wifi_server, bool is_wifi_zperf_udp)
{
	int ret = 0;
	uint64_t test_start_time = 0;

	if (is_ot_client) {
		if (is_wifi_server) {
			if (is_wifi_zperf_udp) {
				LOG_INF(" Test case: wifi_tput_ot_tput");
				LOG_INF(" Thread client, Wi-Fi UDP server");
			} else {
				LOG_INF(" Test case: wifi_tput_ot_tput");
				LOG_INF(" Thread client, Wi-Fi TCP server");
			}
		} else {
			if (is_wifi_zperf_udp) {
				LOG_INF(" Test case: wifi_tput_ot_tput");
				LOG_INF(" Thread client, Wi-Fi UDP client");
			} else {
				LOG_INF(" Thread client, Wi-Fi TCP client");
			}
		}
	} else {
		if (is_wifi_server) {
			if (is_wifi_zperf_udp) {
				LOG_INF(" Test case: wifi_tput_ot_tput");
				LOG_INF(" Thread server, Wi-Fi UDP server");
			} else {
				LOG_INF(" Test case: wifi_tput_ot_tput");
				LOG_INF(" Thread server, Wi-Fi TCP server");
			}
		} else {
			if (is_wifi_zperf_udp) {
				LOG_INF(" Test case: wifi_tput_ot_tput");
				LOG_INF(" Thread server, Wi-Fi UDP client");
			} else {
				LOG_INF(" Thread server, Wi-Fi TCP client");
			}
		}
	}

	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);

	if (is_ot_client) {
		is_ot_device_role_client = true;
	} else {
		is_ot_device_role_client = false;
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

#ifdef CONFIG_TWT_ENABLE
	if (test_wifi) {
		if (!twt_supported) {
			LOG_INF("AP is not TWT capable, exiting the sample\n");
			return 1;
		}

		LOG_INF("AP is TWT capable, establishing TWT");

		ret = setup_twt();
		if (ret) {
			LOG_ERR("Failed to establish TWT flow: %d\n", ret);
			return 1;
		} else {
			LOG_INF("Establishing TWT flow: success\n");
		}
		LOG_INF("Waiting for TWT response");
		ret = wait_for_twt_resp_received()
		if (ret) {
			LOG_INF("TWT resp received. TWT Setup success");
		} else {
			LOG_ERR("TWT resp NOT received. TWT Setup timed out\n");
		}
	}
#endif
	if (test_thread) {
		if (!is_ot_client) {
			LOG_INF("Make sure peer Thread role is client");
			k_sleep(K_SECONDS(3));
		}
		ret = ot_throughput_test_init(is_ot_client);
		k_sleep(K_SECONDS(3));
		if (ret != 0) {
			LOG_ERR("Thread throughput init failed: %d", ret);
			return ret;
		}
		
		if (is_ot_client) {
			/* nothing */
		} else {
			/* wait until the peer client joins the network */
			while (wait4_ping_reply_from_peer==0) {				
					LOG_INF("Waiting on ping reply from peer");
					get_peer_address(5000);
					k_sleep(K_SECONDS(1));
					if(wait4_ping_reply_from_peer) {
						break;
					}
			}		
		}
		
		if (!is_wifi_server) {
			if (is_ot_client) {
				/* nothing */
			} else {
				if (test_wifi && test_thread) {
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
					while (!wait4_peer_ot2_start_connection) {
						/* Peer Thread starts the the test. */
						LOG_INF("Run Thread client on peer");
						k_sleep(K_SECONDS(1));
					}
					wait4_peer_ot2_start_connection = 0;
#endif
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
		if (is_wifi_zperf_udp) {
			ret = run_wifi_traffic_udp();
		} else {
			ret = run_wifi_traffic_tcp();
		}

		if (ret != 0) {
			LOG_ERR("Failed to start Wi-Fi benchmark: %d", ret);
			return ret;
		}
		if (is_wifi_server) {
			while (!wait4_peer_wifi_client_to_start_tp_test) {
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
				LOG_INF("start Wi-Fi client on peer");
#endif
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

	if (test_thread) {
		if (is_ot_client) {
			start_ot_activity();
		} else {
			/* If DUT Thread is server then the peer client starts zperf Tx */
			start_ot_activity(); /* starts zperf Rx */			
		}
	}

	if (test_wifi) {
		check_wifi_traffic();
	}

	if (test_thread) {
		if (is_ot_client) {
			/* run Thread activity and wait for the test duration */
			run_ot_activity();
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
		#ifdef CONFIG_TWT_ENABLE
			ret = teardown_twt();
			if (ret) {
				LOG_ERR("Failed to teardown TWT flow: %d\n", ret);
				return 1;
			}
		#endif
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


int wifi_con_stability_ot_discov_interference(bool test_wifi, bool test_thread, bool is_ot_client,
		bool is_ant_mode_sep)
{
	int ret = 0;
	uint64_t test_start_time = 0;
	uint32_t wifi_con_intact_cnt = 0;
	/* Wi-Fi client/server role has no meaning in the Wi-Fi connection. */
	/* Thread client/server role has no meaning in the Thread discovery. */
	bool is_wifi_server = false;

	if (is_ot_client) {
		LOG_INF("Test case: wifi_con_stability_ot_discov_interference, Thread client");
	} else {
		LOG_INF("Test case: wifi_con_stability_ot_discov_interference, Thread server");
	}

	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);

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
		/* Initialize Thread by selecting role and connect it to peer device. */
		ot_discov_attempt_cnt++;
		ot_initialization();
		k_sleep(K_SECONDS(3));
	}

	/* For Wi-Fi only test */
	if (test_wifi && !test_thread) {
		LOG_INF("-------------------------start");
		test_start_time = k_uptime_get_32();
	}

	/* Thread only and Wi-Fi+Thread tests : Start Thread interference */
	if (test_thread) {
		if (is_ot_client) {
			#ifdef DEMARCATE_TEST_START
			LOG_INF("-------------------------start");
			#endif
			repeat_ot_discovery = 1;
			test_start_time = k_uptime_get_32();

			start_ot_activity();
		} 
	}

	/* Wait for test duration to complete if WLAN only case */
	if (test_wifi && !test_thread) {		
		while (true) {
			if ((k_uptime_get_32() - test_start_time) > CONFIG_COEX_TEST_DURATION) {
				break;
			}
			k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
		}
	}

	/* Thread only and Wi-Fi+Thread tests  */
	if (test_thread) {
		if (is_ot_client) {
			/* Run Thread activity and wait for the test duration */
			run_ot_activity();
		} 
	}

	/* check Wi-Fi connection status. Disconnect if not disconnected already */
	if (test_wifi) {
		if (status.state < WIFI_STATE_ASSOCIATED) {
			LOG_INF("Wi-Fi disconnected");
		} else {
			LOG_INF("Wi-Fi connection intact");
			wifi_con_intact_cnt++;
			wifi_disconnection();
			k_sleep(KSLEEP_WIFI_DISCON_2SEC);
		}
	}
#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
#endif
	if (test_thread) {
		//if (is_ot_client) {
			ot_device_disable(); // to disable thread device after the test completion
		//}
	}
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	ot_discov_attempts_before_test_starts = 0;

	if (test_thread) {			
		LOG_INF("ot_discov_attempt_cnt = %u",
		ot_discov_attempt_cnt - ot_discov_attempts_before_test_starts);
		LOG_INF("ot_discov_success_cnt = %u",
			ot_discov_success_cnt -	ot_discov_attempts_before_test_starts);
		#if 0
		LOG_INF("ot_discov_timeout = %u",	ot_discov_timeout); // this can be commented. as ot_discov_no_result_cnt is available.
		#endif
		LOG_INF("ot_discov_no_result_cnt = %u",	ot_discov_no_result_cnt);
		LOG_INF(" Note: Thread counts can be ignored as this just acts as interference.");
	}
	if (test_wifi) {
		LOG_INF("wifi_conn_attempt_cnt = %u", wifi_conn_attempt_cnt);
		LOG_INF("wifi_conn_success_cnt = %u", wifi_conn_success_cnt);
		LOG_INF("wifi_con_intact_cnt = %llu", wifi_con_intact_cnt);
	}
#endif

	return 0;
}

int wifi_con_stability_ot_tput_interference(bool test_wifi, bool is_ant_mode_sep, bool test_thread,
		bool is_ot_client)
{
	int ret = 0;
	uint64_t test_start_time = 0;
	uint32_t wifi_con_intact_cnt = 0;
	/* Wi-Fi clinet/server role has no meaning in Wi-Fi connection. */
	bool is_wifi_server = false;

	if (is_ot_client) {
		LOG_INF("Test case: wifi_con_stability_ot_tput_interference, Thread client");
	} else {
		LOG_INF("Test case: wifi_con_stability_ot_tput_interference, Thread server");
	}

	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);

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
		if (!is_ot_client) {
			LOG_INF("Make sure peer Thread role is client");
			k_sleep(K_SECONDS(3));
		}
	}

	if (test_thread) {
		ret = ot_throughput_test_init(is_ot_client);
		k_sleep(K_SECONDS(3));
		if (ret != 0) {
			LOG_ERR("Thread throughput init failed: %d", ret);
			return ret;
		}
		
		if (is_ot_client) {
			/* nothing */
		} else {
			/* wait until the peer client joins the network */
			while (wait4_ping_reply_from_peer==0) {				
					LOG_INF("Waiting on ping reply from peer");
					get_peer_address(5000);
					k_sleep(K_SECONDS(1));
					if(wait4_ping_reply_from_peer) {
						break;
					}
			}		
		}
	}
	if (test_wifi && !test_thread) {
		LOG_INF("-------------------------start");
		test_start_time = k_uptime_get_32();
	}
	/* Thread only and Wi-Fi+Thread tests : Start Thread interference */
	if (test_thread) {
		if (is_ot_client) {
#ifdef DEMARCATE_TEST_START
			LOG_INF("-------------------------start");
#endif
			test_start_time = k_uptime_get_32();

			start_ot_activity();
		} else {
			/* If DUT Thread is server then the peer client starts zperf Tx */
			start_ot_activity(); /* starts zperf Rx */
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
			while (!wait4_peer_ot2_start_connection) {
				/* Peer Thread starts the the test. */
				LOG_INF("Run Thread client on peer");
				k_sleep(K_SECONDS(1));
			}
			wait4_peer_ot2_start_connection = 0;
#endif

#ifdef DEMARCATE_TEST_START
			LOG_INF("-------------------------start");
#endif
			test_start_time = k_uptime_get_32();
		}
	}

	/* Wait for test duration to complete if WLAN only case */
	if (test_wifi && !test_thread) {
		while (true) {
			if ((k_uptime_get_32() - test_start_time) > CONFIG_COEX_TEST_DURATION) {
				break;
			}
			k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
		}
	}

	/* Thread only and Wi-Fi+Thread tests  */
	if (test_thread) {
		if (is_ot_client) {
			/* run and wait for the test duration */
			run_ot_activity();
			ot_throughput_test_exit();
		} else {
			while (true) {
				if ((k_uptime_get_32() - test_start_time) >
					CONFIG_COEX_TEST_DURATION) {
					break;
				}
				k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
			}
			/* Peer Thread does the disconnects in the case of server */
		}
	}

	/* check Wi-Fi connection status. Disconnect if not disconnected already */
	if (test_wifi) {
		if (status.state < WIFI_STATE_ASSOCIATED) {
			LOG_INF("Wi-Fi disconnected");
		} else {
			LOG_INF("Wi-Fi connection intact");
			wifi_con_intact_cnt++;
			wifi_disconnection();
			k_sleep(KSLEEP_WIFI_DISCON_2SEC);
		}
	}
#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
#endif

#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	if (test_wifi) {
		LOG_INF("wifi_conn_attempt_cnt = %u", wifi_conn_attempt_cnt);
		LOG_INF("wifi_conn_success_cnt = %u", wifi_conn_success_cnt);
		LOG_INF("wifi_con_intact_cnt = %llu", wifi_con_intact_cnt);

		LOG_INF("Thread throughput results may be ignored.");
		LOG_INF("That provides information on whether Thread acted as interference");
	}
#endif

	return 0;
}

int ot_con_stability_wifi_scan_interference(bool is_ant_mode_sep, bool test_thread, bool test_wifi,
		bool is_ot_client, bool is_wifi_conn_scan)
{
	uint64_t test_start_time = 0;
	uint64_t ot_connection_intact_cnt = 0;
	int ret = 0;
	/* Wi-Fi client/server role has no meaning in Wi-Fi scan */
	bool is_wifi_server = false;

	if (is_ot_client) {
		if (is_wifi_conn_scan) {
			LOG_INF("Test case: ot_con_stability_wifi_scan_interference");
			LOG_INF("Thread client, Wi-Fi connected scan");
		} else {
			LOG_INF("Test case: ot_con_stability_wifi_scan_interference");
			LOG_INF("Thread client, Wi-Fi scan");
		}
	} else {
		if (is_wifi_conn_scan) {
			LOG_INF("Test case: ot_con_stability_wifi_scan_interference");
			LOG_INF("Thread client, Wi-Fi connected scan");
		} else {
			LOG_INF("Test case: ot_con_stability_wifi_scan_interference");
			LOG_INF("Thread server, Wi-Fi scan");
		}
	}

	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);
	//print_ot_connection_test_params(is_ot_client);

	/* one time Thread connection */
	if (test_thread) {
		ot_connection_attempt_cnt++;
			
		/* start joining to the network with pre-shared key = FEDCBA9876543210 */
		ot_start_joiner("FEDCBA9876543210");			
		//}

		//Note: with sleep of 2/3 seconds, not observing the issue of one success for 
		// two join attempts. But, observing issue with ksleep of 1sec
		k_sleep(K_SECONDS(2)); /* time sleep between two joiner attempts */
	}
	if (test_wifi) {
		if (is_wifi_conn_scan) {
			ret = wifi_connection();
			if (ret != 0) {
				LOG_ERR("Wi-Fi connection failed. Running the test");
				LOG_ERR("further is not meaningful. So, exiting the test");
				return ret;
			}
		}
#if defined(CONFIG_NRF700X_BT_COEX)
			config_pta(is_ant_mode_sep, is_ot_client, is_wifi_server);
#endif/* CONFIG_NRF700X_BT_COEX */
	}
	/* start Wi-Fi scan and continue for test duration */
	if (test_wifi) {
		cmd_wifi_scan();
	}

#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------start");
#endif
	test_start_time = k_uptime_get_32();

	/* wait for the Wi-Fi interferecne to cover for test duration */
	while (true) {
		if ((k_uptime_get_32() - test_start_time) > CONFIG_COEX_TEST_DURATION) {
			break;
		}
		k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
	}

	/* stop the Wi-Fi iterative scan interference happening further */
	if (test_wifi) {
		repeat_wifi_scan = 0;
	}
	LOG_INF("ot_client_connected: %d",ot_client_connected);
	/* check Thread connection status */
	if (test_thread) {
		if (ot_client_connected) {
			LOG_INF("Thread Conn Intact");
			ot_connection_intact_cnt++;
		} else {
			LOG_INF("Thread disconnected");
		}
		ot_disconnect_client();
		k_sleep(K_SECONDS(2));
	}

#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
#endif

#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	if (test_wifi) {
		LOG_INF("wifi_scan_cmd_cnt = %u", wifi_scan_cmd_cnt);
		LOG_INF("wifi_scan_cnt_24g = %u", wifi_scan_cnt_24g);
		LOG_INF("wifi_scan_cnt_5g = %u", wifi_scan_cnt_5g);
	}
	if (test_wifi && test_thread) {
		LOG_INF("Wi-Fi scan results may be ignored. That provides information");
		LOG_INF("on whether Wi-Fi acted as interference or not");
		LOG_INF("");
	}
	if (test_thread) {
		LOG_INF("ot_connection_attempt_cnt = %u", ot_connection_attempt_cnt);
		LOG_INF("ot_connection_success_cnt = %u", ot_connection_success_cnt);
		LOG_INF("ot_connection_intact_cnt = %llu", ot_connection_intact_cnt);
	}
#endif

	return 0;
}

int ot_connection_stability_wifi_tput_interference(bool test_wifi, bool test_thread,
		bool is_ot_client, bool is_wifi_server, bool is_ant_mode_sep, bool is_wifi_zperf_udp)
{
	int ret = 0;
	uint64_t test_start_time = 0;
	uint64_t ot_connection_intact_cnt = 0;
	//LOG_INF("test_thread=%d", test_thread);
	print_common_test_params(is_ant_mode_sep, test_thread, test_wifi, is_ot_client);

	//print_ot_connection_test_params(is_ot_client);

	 if (is_wifi_server) {
		if (is_ot_client) {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case:	ot_connection_stability_wifi_tput_interference");
				LOG_INF("Thread client, Wi-Fi UDP server");
			} else {
				LOG_INF("Test case:	ot_connection_stability_wifi_tput_interference");
				LOG_INF("Thread client, Wi-Fi TCP server");
			}
		} else {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case:	ot_connection_stability_wifi_tput_interference");
				LOG_INF("Thread server, Wi-Fi UDP server");
			} else {
				LOG_INF("Test case:	ot_connection_stability_wifi_tput_interference");
				LOG_INF("Thread server, Wi-Fi TCP server");
			}
		}
	} else {
		if (is_ot_client) {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case:	ot_connection_stability_wifi_tput_interference");
				LOG_INF("Thread client, Wi-Fi UDP client");
			} else {
				LOG_INF("Test case:	ot_connection_stability_wifi_tput_interference");
				LOG_INF("Thread client, Wi-Fi TCP client");
			}
		} else {
			if (is_wifi_zperf_udp) {
				LOG_INF("Test case:	ot_connection_stability_wifi_tput_interference");
				LOG_INF("Thread server, Wi-Fi UDP client");
			} else {
				LOG_INF("Test case:	ot_connection_stability_wifi_tput_interference");
				LOG_INF("Thread server, Wi-Fi TCP client");
			}
		}
	} 

	/* one time Thread connection */
	if (test_thread) {
		ot_connection_attempt_cnt++;
			
		/* start joining to the network with pre-shared key = FEDCBA9876543210 */
		ot_start_joiner("FEDCBA9876543210");			
		//}

		//Note: with sleep of 2/3 seconds, not observing the issue of one success for 
		// two join attempts. But, observing issue with ksleep of 1sec
		k_sleep(K_SECONDS(2)); /* time sleep between two joiner attempts */
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

	/* start Wi-Fi throughput interference */
	if (test_wifi) {
		if (is_wifi_zperf_udp) {
			ret = run_wifi_traffic_udp();
		} else {
			ret = run_wifi_traffic_tcp();
		}
		if (ret != 0) {
			LOG_ERR("Failed to start Wi-Fi benchmark: %d", ret);
			return ret;
		}
	}

	if (test_wifi) {
		if (is_wifi_server) {
			while (!wait4_peer_wifi_client_to_start_tp_test) {
				#ifdef CONFIG_PRINTS_FOR_AUTOMATION
				LOG_INF("start Wi-Fi client on peer");
				#endif
				k_sleep(K_SECONDS(1));
			}
			wait4_peer_wifi_client_to_start_tp_test = 0;
		}
	}

	#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------start");
	#endif
	test_start_time = k_uptime_get_32();

	/* Thread only case    : continue until test duration is complete */
	/* Thread + Wi-Fi cases: continue Wi-Fi interference until test duration is complete */
	if (test_thread) {
		while (true) {
			if ((k_uptime_get_32() - test_start_time) > CONFIG_COEX_TEST_DURATION) {
				break;
			}
			k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
		}
	}

	if (test_wifi) {
		check_wifi_traffic();
		wifi_disconnection();
	}

	/* check Thread connection status */
	if (test_thread) {
		if (ot_client_connected) {
			LOG_INF("Thread Conn Intact");
			ot_connection_intact_cnt++;
		} else {
			LOG_INF("Thread disconnected");
		}
		ot_disconnect_client();
		k_sleep(K_SECONDS(2));
	}

	#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
	#endif



	LOG_INF("Note: Wi-Fi throughput results may be ignored.");
	LOG_INF("That provides information on whether Wi-Fi acted as interference or not");

	#ifdef CONFIG_PRINTS_FOR_AUTOMATION
		if (test_thread) {
		LOG_INF("ot_connection_attempt_cnt = %u", ot_connection_attempt_cnt);
		LOG_INF("ot_connection_success_cnt = %u", ot_connection_success_cnt);
		LOG_INF("ot_connection_intact_cnt = %llu", ot_connection_intact_cnt);
		}
	#endif

	return 0;
}


int wifi_shutdown_ot_discov(bool is_ot_client)
{
	int ret = 0;
	uint64_t test_start_time = 0;
	bool ot_coex_enable = IS_ENABLED(CONFIG_MPSL_CX);
	/* Thread client/server role has no meaning in the Thread discovery */

	if (is_ot_client) {
		LOG_INF("Test case: wifi_shutdown_ot_disc, Thread client");
	} else {
		LOG_INF("Test case: wifi_shutdown_ot_disc, Thread server");
	}

	LOG_INF("Test duration in milliseconds: %d", CONFIG_COEX_TEST_DURATION);
	if (ot_coex_enable) {
		LOG_INF("Thread posts requests to PTA");
	} else {
		LOG_INF("Thread doesn't post requests to PTA");
	}

	/* disable RPU i.e. Wi-Fi shutdown */
	rpu_disable();

	/* Initialize Thread by selecting role and connect it to peer device. */
	ot_discov_attempt_cnt++;
	ot_initialization();
	k_sleep(K_SECONDS(3));

	if (is_ot_client) {
		/**If Thread is client, disconnect the connection.
		 * Connection and disconnection happens in loop later.
		 */
		//ot_disconnection_attempt_cnt++;
		//ot_disconnect_client();
	} 
	//note: Thread connection is done in default client role. 
	//else {
	//	/**If Thread is server, wait until peer Thread client
	//	 *  initiates the connection, DUT is connected to peer client
	//	 *  and update the PHY parameters.
	//	 */
	//	while (true) {
	//		if (ot_server_connected) {
	//			break;
	//		}
	//		k_sleep(KSLEEP_WHILE_PERIP_CONN_CHECK_1SEC);
	//	}
	//}



#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	/* peer client waits on this in automation */
	LOG_INF("Run Thread client");
	k_sleep(K_SECONDS(1));
#endif

	//if (!is_ot_client) {
	//	LOG_INF("DUT is in server role.");
	//	LOG_INF("Check for Thread connection counts on peer Thread side.");
	//}


#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------start");
#endif
	repeat_ot_discovery = 1;
	test_start_time = k_uptime_get_32();

	/* Begin Thread discovery for a period of Thread test duration */
	if (is_ot_client) {
		start_ot_activity();
	} else {
		/* If DUT Thread is server then the peer starts the activity. */
	}

	/* Wait for Thread activity completion i.e., for test duration */
	if (is_ot_client) {
		/* Run Thread activity and wait for the test duration */
		run_ot_activity();
	} else {
		/** If DUT Thread is in server role then peer Thread runs the activity.
		 *wait for test duration
		 */
		while (1) {
			if ((k_uptime_get_32() - test_start_time) >
			CONFIG_COEX_TEST_DURATION) {
				break;
			}
			k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
		}
	}

	repeat_ot_discovery = 0;
#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
#endif

	//if (is_ot_client) {
		ot_device_disable(); // to disable thread device after the test completion
	//}

#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	ot_discov_attempts_before_test_starts = 0;

	LOG_INF("ot_discov_attempt_cnt = %u",
	ot_discov_attempt_cnt - ot_discov_attempts_before_test_starts);
	LOG_INF("ot_discov_success_cnt = %u",
		ot_discov_success_cnt -	ot_discov_attempts_before_test_starts);
	#if 0
	LOG_INF("ot_discov_timeout = %u",	ot_discov_timeout); // this can be commented. as ot_discov_no_result_cnt is available.
	#endif
	LOG_INF("ot_discov_no_result_cnt = %u",	ot_discov_no_result_cnt);

#endif
	return 0;
}

int wifi_shutdown_ot_connection(bool is_ot_client)
{
	uint64_t test_start_time = 0;
	bool ot_coex_enable = IS_ENABLED(CONFIG_MPSL_CX);

	if (is_ot_client) {
		LOG_INF("Test case: wifi_shutdown_ot_connection, Thread client");
	} else {
		LOG_INF("Test case: wifi_shutdown_ot_connection, Thread server");
	}

	LOG_INF("Test duration in milliseconds: %d", CONFIG_COEX_TEST_DURATION);
	if (ot_coex_enable) {
		LOG_INF("Thread posts requests to PTA");
	} else {
		LOG_INF("Thread doesn't post requests to PTA");
	}

	/* disable RPU i.e. Wi-Fi shutdown */
	rpu_disable();
	//print_ot_connection_test_params(is_ot_client);

	/* Thread onetime connection */
	/* Initialize Thread by selecting role and connect it to peer device. */
	ot_discov_attempt_cnt++;
	ot_connection_init(is_ot_client);
	k_sleep(K_SECONDS(2));
	if (is_ot_client) {
		/** If Thread is client, disconnect the connection.
		 *Connection and disconnection happens in loop later.
		 */
		ot_disconnection_attempt_cnt++;
		ot_disconnect_client();
		k_sleep(K_SECONDS(2));
	} else {
		/**If Thread is server, wait until peer Thread client
		 * initiates the connection, DUT is connected to peer client
		 * and update the PHY parameters.
		 */
		while (true) {
			if (ot_server_connected) {
				break;
			}
			k_sleep(KSLEEP_WHILE_PERIP_CONN_CHECK_1SEC);
		}
	}

	#ifdef CONFIG_PRINTS_FOR_AUTOMATION
		/* peer client waits on this in automation */
		LOG_INF("Run Thread client");
	#endif

	if (!is_ot_client) {
		LOG_INF("DUT is in server role.");
		LOG_INF("Check for Thread connection counts on peer Thread side");
	}

	#ifdef DEMARCATE_TEST_START
		LOG_INF("-------------------------start");
	#endif

	test_start_time = k_uptime_get_32();

	/* Begin Thread conections and disconnections for a period of Thread test duration */

	if (is_ot_client) {
		start_ot_activity();
	} else {
		/* If DUT Thread is server then the peer starts the activity. */
	}

	if (is_ot_client) {
		/* run and wait for the test duration */
		run_ot_activity();
	} else {
		/** If DUT Thread is in server role then peer runs the activity.
		 *wait for test duration
		 */
		while (1) {
			if ((k_uptime_get_32() - test_start_time) > CONFIG_COEX_TEST_DURATION) {
				break;
			}
			k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
		}
	}

	#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
	#endif

	#ifdef CONFIG_PRINTS_FOR_AUTOMATION
	ot_conn_attempts_before_test_starts = 0;
	//if (is_ot_client) {
	LOG_INF("ot_connection_attempt_cnt = %u",
		ot_connection_attempt_cnt - ot_conn_attempts_before_test_starts);
	LOG_INF("ot_connection_success_cnt = %u",
		ot_connection_success_cnt - ot_conn_attempts_before_test_starts);


	LOG_INF("ot_disconnection_attempt_cnt = %u",
		ot_disconnection_attempt_cnt);
	LOG_INF("ot_disconnection_success_cnt = %u",
		ot_disconnection_success_cnt);
	LOG_INF("ot_disconnection_fail_cnt = %u",
		ot_disconnection_fail_cnt);
	LOG_INF("ot_discon_no_conn_cnt = %u",
		ot_discon_no_conn_cnt);
	#endif

	return 0;
}

int wifi_shutdown_ot_tput(bool is_ot_client)
{
	uint64_t test_start_time = 0;
	int ret = 0;
	bool ot_coex_enable = IS_ENABLED(CONFIG_MPSL_CX);
	
	if (is_ot_client) {
		LOG_INF("Test case: wifi_shutdown_ot_tput, Thread client");
	} else {
		LOG_INF("Test case: wifi_shutdown_ot_tput, Thread server");
	}

	LOG_INF("Test duration in milliseconds: %d", CONFIG_COEX_TEST_DURATION);

	if (ot_coex_enable) {
		LOG_INF("Thread posts requests to PTA");
	} else {
		LOG_INF("Thread doesn't post requests to PTA");
	}

	if (is_ot_client) {
		is_ot_device_role_client = true;
	} else {
		is_ot_device_role_client = false;
	}

	/* Disable RPU i.e. Wi-Fi shutdown */
	rpu_disable();

	if (!is_ot_client) { /* server */
		LOG_INF("Make sure peer Thread role is client");
		k_sleep(K_SECONDS(3));
	}
	
	ret = ot_throughput_test_init(is_ot_client);

	k_sleep(K_SECONDS(3));
	if (ret != 0) {
		LOG_ERR("Thread throughput init failed: %d", ret);
		return ret;
	}
	
	if (is_ot_client) {
		/* nothing */
	} else {
		/* wait until the peer client joins the network */
		while (wait4_ping_reply_from_peer==0) {				
				LOG_INF("Waiting on ping reply from peer");
				get_peer_address(5000);
				k_sleep(K_SECONDS(1));
				if(wait4_ping_reply_from_peer) {
					break;
				}
		}		
	}

	#ifdef DEMARCATE_TEST_START
	LOG_INF("");
	LOG_INF("-------------------------start");
	LOG_INF("");
	#endif
	test_start_time = k_uptime_get_32();

	if (is_ot_client) {
		start_ot_activity(); /* starts zperf Tx */
	} else {
		///* If DUT Thread is server then the peer starts the zperf Tx. */
		//start_ot_activity(); /* starts zperf Rx */
	}

	if (is_ot_client) {
		/* run Thread activity and wait for the test duration */
		run_ot_activity();
	} else {
		/* If Thread DUT is server then the peer runs activity. Wait for test duration */
		while (true) {
			if ((k_uptime_get_32() - test_start_time) > CONFIG_COEX_TEST_DURATION) {
				break;
			}
			k_sleep(KSLEEP_WHILE_ONLY_TEST_DUR_CHECK_1SEC);
		}
	}
	if (is_ot_client) {
		ot_throughput_test_exit();
	}

	#ifdef DEMARCATE_TEST_START
	LOG_INF("-------------------------end");
	#endif

	return 0;
}