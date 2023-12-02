/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "wifi_utils.h"

#ifdef CONFIG_TWT_ENABLE
	
	bool twt_resp_received;
	bool twt_supported;

	static int setup_twt(void)
	{
		struct net_if *iface = net_if_get_first_wifi();
		struct wifi_twt_params params = { 0 };
		int ret;

		params.operation = WIFI_TWT_SETUP;
		params.negotiation_type = WIFI_TWT_INDIVIDUAL;
		params.setup_cmd = WIFI_TWT_SETUP_CMD_REQUEST;
		params.dialog_token = 1;
		params.flow_id = 1;
		params.setup.responder = 0;
		params.setup.trigger = IS_ENABLED(CONFIG_TWT_TRIGGER_ENABLE);
		params.setup.implicit = 1;
		params.setup.announce = IS_ENABLED(CONFIG_TWT_ANNOUNCED_MODE);
		params.setup.twt_wake_interval = CONFIG_TWT_WAKE_INTERVAL;
		params.setup.twt_interval = CONFIG_TWT_INTERVAL;

		ret = net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params));
		if (ret) {
			LOG_INF("TWT setup failed: %d", ret);
			return ret;
		}

		LOG_INF("TWT setup requested");

		return 0;
	}

	static int teardown_twt(void)
	{
		struct net_if *iface = net_if_get_first_wifi();
		struct wifi_twt_params params = { 0 };
		int ret;

		params.operation = WIFI_TWT_TEARDOWN;
		params.negotiation_type = WIFI_TWT_INDIVIDUAL;
		params.setup_cmd = WIFI_TWT_TEARDOWN;
		params.dialog_token = 1;
		params.flow_id = 1;

		ret = net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params));
		if (ret) {
			LOG_ERR("%s with %s failed, reason : %s",
				wifi_twt_operation2str[params.operation],
				wifi_twt_negotiation_type2str[params.negotiation_type],
				get_twt_err_code_str(params.fail_reason));
			return ret;
		}

		LOG_INF("TWT teardown success");

		return 0;
	}
	static void handle_wifi_twt_event(struct net_mgmt_event_callback *cb)
	{
		const struct wifi_twt_params *resp =
			(const struct wifi_twt_params *)cb->info;

		if (resp->operation == WIFI_TWT_TEARDOWN) {
			LOG_INF("TWT teardown received for flow ID %d\n",
			      resp->flow_id);
			return;
		}

		if (resp->resp_status == WIFI_TWT_RESP_RECEIVED) {
			twt_resp_received = 1;
			LOG_INF("TWT response: %s",
			      wifi_twt_setup_cmd2str[resp->setup_cmd]);
			LOG_INF("== TWT negotiated parameters ==");
			print_twt_params(resp->dialog_token,
					 resp->flow_id,
					 resp->negotiation_type,
					 resp->setup.responder,
					 resp->setup.implicit,
					 resp->setup.announce,
					 resp->setup.trigger,
					 resp->setup.twt_wake_interval,
					 resp->setup.twt_interval);
		} else {
			LOG_INF("TWT response timed out\n");
		}
	}
	
#endif

int cmd_wifi_status(void)
{
	struct net_if *iface = net_if_get_first_wifi();

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &wifi_if_status,
				sizeof(struct wifi_iface_status))) {
		LOG_INF("Status request failed");

		return -ENOEXEC;
	}

	#ifndef CONFIG_PRINTS_FOR_AUTOMATION
		LOG_INF("Status: successful");
		LOG_INF("==================");
		LOG_INF("State: %s", wifi_state_txt(wifi_if_status.state));
	#endif

	if (wifi_if_status.state >= WIFI_STATE_ASSOCIATED) {
		uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

		if (print_wifi_conn_status_once == 1) {

			LOG_INF("Interface Mode: %s",
				   wifi_mode_txt(wifi_if_status.iface_mode));
			LOG_INF("Link Mode: %s",
				   wifi_link_mode_txt(wifi_if_status.link_mode));
			LOG_INF("SSID: %-32s", wifi_if_status.ssid);
			LOG_INF("BSSID: %s",
				   net_sprint_ll_addr_buf(
					wifi_if_status.bssid, WIFI_MAC_ADDR_LEN,
					mac_string_buf, sizeof(mac_string_buf)));
			LOG_INF("Band: %s", wifi_band_txt(wifi_if_status.band));
			LOG_INF("Channel: %d", wifi_if_status.channel);
			LOG_INF("Security: %s", wifi_security_txt(wifi_if_status.security));
			/* LOG_INF("MFP: %s", wifi_mfp_txt(wifi_if_status.mfp)); */
			LOG_INF("Wi-Fi RSSI: %d", wifi_if_status.rssi);
#ifdef CONFIG_TWT_ENABLE
			LOG_INF("TWT: %s", wifi_if_status.twt_capable ? "Supported" : "Not supported");

			if (wifi_if_status.twt_capable) {
				twt_supported = 1;
			}
#endif
			print_wifi_conn_status_once++;
		}
		wifi_rssi = wifi_if_status.rssi;
	}

	return 0;
}

void memset_context(void)
{
	memset(&wifi_cintext, 0, sizeof(wifi_cintext));
}

void wifi_net_mgmt_callback_functions(void)
{
	net_mgmt_init_event_callback(&wifi_sta_mgmt_cb, wifi_mgmt_event_handler,
		WIFI_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_sta_mgmt_cb);

	net_mgmt_init_event_callback(&net_addr_mgmt_cb, net_mgmt_event_handler,
		NET_EVENT_IPV4_DHCP_BOUND);

	net_mgmt_add_event_callback(&net_addr_mgmt_cb);

#ifdef CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
#endif

	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock/MHZ(1));

	k_sleep(K_SECONDS(1));
}


void wifi_init(void) {

	memset_context();
	
	wifi_net_mgmt_callback_functions();
}

void net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
		struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_IPV4_DHCP_BOUND:
		print_dhcp_ip(cb);
		break;
	default:
		break;
	}
}

void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
		uint32_t mgmt_event, struct net_if *iface)
{
	const struct device *dev = iface->if_dev->dev;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;

	vif_ctx_zep = dev->data;

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	case NET_EVENT_WIFI_SCAN_RESULT:
		vif_ctx_zep->scan_in_progress = 0;
		handle_wifi_scan_result(cb);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		handle_wifi_scan_done(cb);
		break;
#ifdef CONFIG_TWT_ENABLE
	case NET_EVENT_WIFI_TWT:
		handle_wifi_twt_event(cb);
	break;
#endif
	default:
		break;
	}
}

int __wifi_args_to_params(struct wifi_connect_req_params *params)
{
	params->timeout = SYS_FOREVER_MS;

	/* SSID */
	params->ssid = CONFIG_STA_SSID;
	params->ssid_length = strlen(params->ssid);

#if defined(CONFIG_STA_KEY_MGMT_WPA2)
	params->security = 1;
#elif defined(CONFIG_STA_KEY_MGMT_WPA2_256)
	params->security = 2;
#elif defined(CONFIG_STA_KEY_MGMT_WPA3)
	params->security = 3;
#else
	params->security = 0;
#endif

#if !defined(CONFIG_STA_KEY_MGMT_NONE)
	params->psk = CONFIG_STA_PASSWORD;
	params->psk_length = strlen(params->psk);
#endif
	params->channel = WIFI_CHANNEL_ANY;

	/* MFP (optional) */
	params->mfp = WIFI_MFP_OPTIONAL;

	return 0;
}


int cmd_wifi_scan(void)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_scan_params params = {0};

	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params, sizeof(struct wifi_scan_params))) {
		LOG_ERR("Scan request failed");
		return -ENOEXEC;
	}
#ifdef CONFIG_DEBUG_PRINT_WIFI_SCAN_INFO
	LOG_INF("Scan requested");
#endif
	return 0;
}

int wifi_connect(void)
{
	struct net_if *iface = net_if_get_first_wifi();
	static struct wifi_connect_req_params cnx_params = {0};

	/* LOG_INF("Connection requested"); */
	__wifi_args_to_params(&cnx_params);

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
			&cnx_params, sizeof(struct wifi_connect_req_params))) {
		LOG_ERR("Wi-Fi Connection request failed");
		return -ENOEXEC;
	}
	return 0;
}

int wifi_disconnect(void)
{
	struct net_if *iface = net_if_get_first_wifi();
	int wifi_if_status;

	wifi_cintext.disconnect_requested = true;

	wifi_if_status = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	if (wifi_if_status) {
		wifi_cintext.disconnect_requested = false;

		if (wifi_if_status == -EALREADY) {
			/* LOG_ERR("Already disconnected"); */
			wifi_disconn_no_conn_cnt++;
		} else {
			/* LOG_ERR("Disconnect request failed"); */
			wifi_disconn_fail_cnt++;
			return -ENOEXEC;
		}
	} else {
		wifi_disconn_success_cnt++;
	}
	return 0;
}

int parse_ipv4_addr(char *host, struct sockaddr_in *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}
	ret = net_addr_pton(AF_INET, host, &addr->sin_addr);
	if (ret < 0) {
		LOG_ERR("Invalid IPv4 address %s", host);
		return -EINVAL;
	}
	LOG_INF("Wi-Fi peer IPv4 address %s", host);

	return 0;
}
