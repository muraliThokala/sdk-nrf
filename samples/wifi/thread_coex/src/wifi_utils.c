/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "wifi_utils.h"

int cmd_wifi_status(void)
{
	struct net_if *iface = net_if_get_first_wifi();

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &wifi_if_status,
				sizeof(struct wifi_iface_status))) {
		LOG_INF("Status request failed");

		return -ENOEXEC;
	}

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

			print_wifi_conn_status_once++;
		}
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

void wifi_init(void)
{
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
		} else {
			/* LOG_ERR("Disconnect request failed"); */
			return -ENOEXEC;
		}
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
