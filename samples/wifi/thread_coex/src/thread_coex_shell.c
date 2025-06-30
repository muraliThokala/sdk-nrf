/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(thread_coex, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/net/zperf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>

#include <net/wifi_mgmt_ext.h>

#include "net_private.h"

#include "coex.h"

#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/types.h>

#include <zephyr/shell/shell_uart.h>

static struct sockaddr_in addr_my = {
	.sin_family = AF_INET,
	.sin_port = htons(5001),
};
K_SEM_DEFINE(udp_callback, 0, 1);


#include <stdio.h>

#include "thread_coex_shell.h"
#include "zperf_utils.h"

#include "openthread/ping_sender.h"
#include <openthread/thread.h>

/* #define WAIT_TIME_FOR_OT_CON K_SECONDS(4) */
/* Note: current value of WAIT_TIME_FOR_OT_CON = 4sec.
 * Sometimes, starting openthread happening before
 * the thread join failed.So, increase it to 10sec.
 */
#define WAIT_TIME_FOR_OT_CON K_SECONDS(10)
#define GET_PEER_ADDR_WAIT_TIME 5000
#define CHECK_OT_ROLE_WAIT_TIME 50

typedef struct peer_address_info {
	char address_string[OT_IP6_ADDRESS_STRING_SIZE];
	bool address_found;
} peer_address_info_t;

peer_address_info_t peer_address_info  = {.address_found = false};

static K_SEM_DEFINE(connected_sem, 0, 1);

static void ot_device_dettached(void *ptr)
{
	(void) ptr;

	printk("\nOT device dettached gracefully\n");
}

static void ot_commissioner_state_changed(otCommissionerState aState, void *aContext)
{
	printk("OT commissioner state changed");
	if (aState == OT_COMMISSIONER_STATE_ACTIVE) {
		printk("ot commissioner joiner add * FEDCBA9876543210 2000");
		otCommissionerAddJoiner(openthread_get_default_instance(), NULL,
		"FEDCBA9876543210", 2000);
		printk("\n\nRun thread application on client\n\n");
	}
}

static void ot_thread_state_changed(otChangedFlags flags,
			struct openthread_context *ot_context, void *user_data)
{
	printk("OT device state changed");
	if (flags & OT_CHANGED_THREAD_ROLE) {
		otDeviceRole ot_role = otThreadGetDeviceRole(ot_context->instance);

		if (ot_role != OT_DEVICE_ROLE_DETACHED && ot_role != OT_DEVICE_ROLE_DISABLED) {
			/* ot commissioner start */
			printk("ot commissioner start");
			otCommissionerStart(ot_context->instance, &ot_commissioner_state_changed,
			NULL, NULL);
		}
	}
}

static struct openthread_state_changed_cb ot_state_chaged_cb = {
	.state_changed_cb = ot_thread_state_changed
};

/* call back Thread device joiner */
static void ot_joiner_start_handler(otError error, void *context)
{
	switch (error) {

	case OT_ERROR_NONE:
		printk("Thread Join success");
		k_sem_give(&connected_sem);
	break;

	default:
		printk("Thread join failed [%s]\n", otThreadErrorToString(error));
	break;
	}
}

int ot_throughput_client_init(void)
{
	otError err = 0;
	uint32_t ot_role_non_child = 0;

	ot_start_joiner("FEDCBA9876543210");
	err = k_sem_take(&connected_sem, WAIT_TIME_FOR_OT_CON);
	struct openthread_context *context = openthread_get_default_context();

	printk("Starting openthread.");
	openthread_api_mutex_lock(context);
	/*  ot thread start */
	err = otThreadSetEnabled(openthread_get_default_instance(), true);
	if (err != OT_ERROR_NONE) {
		printk("Starting openthread: %d (%s)\n", err, otThreadErrorToString(err));
	}

	otDeviceRole current_role =
		otThreadGetDeviceRole(openthread_get_default_instance());
	openthread_api_mutex_unlock(context);

	printk("Current role: %s. Waiting to get child role",
		otThreadDeviceRoleToString(current_role));

	while (current_role != OT_DEVICE_ROLE_CHILD) {
		k_sleep(K_MSEC(CHECK_OT_ROLE_WAIT_TIME));
		openthread_api_mutex_lock(context);
		current_role = otThreadGetDeviceRole(openthread_get_default_instance());
		openthread_api_mutex_unlock(context);
		/* Avoid infinite waiting if the device role is not child */
		if (current_role == OT_DEVICE_ROLE_ROUTER) {
			ot_role_non_child = 1;
			break;
		}
	}

	if (ot_role_non_child) {
		printk("\nCurrent role of Thread device: %s\n",
			otThreadDeviceRoleToString(current_role));
		printk("Current role is not child, exiting the test. Re-run the test\n");
		return -1;
	}
	ot_get_peer_address(GET_PEER_ADDR_WAIT_TIME);
	if (!peer_address_info.address_found) {
		printk("Peer address not found. Not continuing with zperf test.\n");
		return -1;
	}
	return 0;
}

static int ot_run_throughput(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 4) {
		printk("invalid # of args : %d\n", argc);
		printk("Usage: ot_run_tput is_ot_client is_ot_zperf_udp\n\n");
		printk("       is_ot_client: 1 for client, 0 for server\n");
		printk("       is_ot_zperf_udp: 1 for UDP, 0 for TCP\n");
		printk("       test duration in seconds");
		return -ENOEXEC;
	}

	bool is_ot_client  = strtoul(argv[1], NULL, 0);
	bool is_ot_zperf_udp = strtoul(argv[2], NULL, 0);
	uint32_t test_duration = strtoul(argv[3], NULL, 0);

	if (is_ot_client) {
		ot_zperf_test(is_ot_client, is_ot_zperf_udp, test_duration);
		/* Only for client case. Server case is handled as part of init */
	}
	return 0;
}

void ot_start_joiner(const char *pskd)
{
	printk("Starting joiner");

	otInstance *instance = openthread_get_default_instance();
	struct openthread_context *context = openthread_get_default_context();

	openthread_api_mutex_lock(context);

	/** Step1: Set null network key i.e,
	 * ot networkkey 00000000000000000000000000000000
	 */
	/* ot_setNullNetworkKey(instance); */

	/** Step2: Bring up the interface and start joining to the network
	 * on DK2 with pre-shared key.
	 * i.e. ot ifconfig up
	 * ot joiner start FEDCBA9876543210
	 */
	otIp6SetEnabled(instance, true); /* ot ifconfig up */
	otJoinerStart(instance, pskd, NULL,
				"Zephyr", "Zephyr",
				KERNEL_VERSION_STRING, NULL,
				&ot_joiner_start_handler, NULL);
	openthread_api_mutex_unlock(context);
	/* printk("Thread start joiner Done."); */
}

int ot_configure_throughput(const struct shell *shell, size_t argc,	char *argv[])
{
	if (argc < 3) {
		printk("invalid # of args : %d\n", argc);
		printk("Usage: ot_cfg_tput is_ot_client is_ot_zperf_udp\n\n");
		printk("       is_ot_client: 1 for client, 0 for server\n");
		printk("       is_ot_zperf_udp: 1 for UDP, 0 for TCP\n");
		return -ENOEXEC;
	}

	bool is_ot_client  = strtoul(argv[1], NULL, 0);
	bool is_ot_zperf_udp = strtoul(argv[2], NULL, 0);

	int ret = 0;

	if (is_ot_client) { /* for client */
		ret = ot_throughput_client_init();
		if (ret != 0) {
			printk("Thread throughput client init - FAIL\n");
		} else {
			printk("Thread throughput client init - SUCCESS\n");
			return 0;
		}
		return ret;
	}
	if (!is_ot_client) { /* for server */
		ot_initialization();
		openthread_state_changed_cb_register(openthread_get_default_context(),
		&ot_state_chaged_cb);

		printk("Starting zperf server");
		ot_start_zperf_test_recv(is_ot_zperf_udp);
	}
	return 0;
}

static int ot_disconnection(const struct shell *shell, size_t argc,
			char *argv[])
{
	otInstance *instance = openthread_get_default_instance();
	struct openthread_context *context = openthread_get_default_context();

	otThreadDetachGracefully(instance, ot_device_dettached, context);
	k_sleep(K_MSEC(1000));

	return 0;
}

void ot_setNullNetworkKey(otInstance *aInstance)
{
	otOperationalDataset aDataset;
	uint8_t key[OT_NETWORK_KEY_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	/* memset(&aDataset, 0, sizeof(otOperationalDataset)); */ /* client */
	otDatasetCreateNewNetwork(aInstance, &aDataset); /* server */


	/* Set network key to null */
	memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
	aDataset.mComponents.mIsNetworkKeyPresent = true;
	otDatasetSetActive(aInstance, &aDataset);
}

void ot_setNetworkConfiguration(otInstance *aInstance)
{
	static const char aNetworkName[] = "TestNetwork";
	otOperationalDataset aDataset;
	size_t length;
	uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x11, 0x11, 0x11, 0x11,	0x11, 0x11, 0x11, 0x11};
	uint8_t key[OT_NETWORK_KEY_SIZE] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
		0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

	/* memset(&aDataset, 0, sizeof(otOperationalDataset)); */
	otDatasetCreateNewNetwork(aInstance, &aDataset);

	/**
	 * Fields that can be configured in otOperationDataset to override defaults:
	 *     Network Name, Mesh Local Prefix, Extended PAN ID, PAN ID, Delay Timer,
	 *     Channel, Channel Mask Page 0, Network Key, PSKc, Security Policy
	 */

	aDataset.mActiveTimestamp.mSeconds = 1;
	aDataset.mActiveTimestamp.mTicks = 0;
	aDataset.mActiveTimestamp.mAuthoritative = false;
	aDataset.mComponents.mIsActiveTimestampPresent = true;

	/* Set Channel */
	aDataset.mChannel = CONFIG_OT_CHANNEL;
	aDataset.mComponents.mIsChannelPresent = true;

	/* Set Pan ID */
	aDataset.mPanId = (otPanId)CONFIG_OT_PAN_ID;
	aDataset.mComponents.mIsPanIdPresent = true;

	/* Set Extended Pan ID */
	memcpy(aDataset.mExtendedPanId.m8, extPanId, sizeof(aDataset.mExtendedPanId));
	aDataset.mComponents.mIsExtendedPanIdPresent = true;

	/* Set network key */
	memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
	aDataset.mComponents.mIsNetworkKeyPresent = true;

	/* Set Network Name */
	length = strlen(aNetworkName);
	assert(length <= OT_NETWORK_NAME_MAX_SIZE);
	memcpy(aDataset.mNetworkName.m8, aNetworkName, length);
	aDataset.mComponents.mIsNetworkNamePresent = true;
	otDatasetSetActive(aInstance, &aDataset);
}

int ot_initialization(void)
{
	struct openthread_context *context = openthread_get_default_context();

	otInstance *instance = openthread_get_default_instance();

	/* printk("Updating thread parameters"); */
	ot_setNetworkConfiguration(instance);
	/* printk("Enabling thread"); */
	otError err = openthread_start(context); /* 'ifconfig up && thread start' */

	if (err != OT_ERROR_NONE) {
		printk("Starting openthread: %d (%s)\n", err, otThreadErrorToString(err));
	}
	otDeviceRole current_role = otThreadGetDeviceRole(instance);

	printk("Current role of Thread device: %s", otThreadDeviceRoleToString(current_role));
	return 0;
}

void ot_handle_ping_reply(const otPingSenderReply *reply, void *context)
{
	otIp6Address add = reply->mSenderAddress;
	char string[OT_IP6_ADDRESS_STRING_SIZE];

	otIp6AddressToString(&add, string, OT_IP6_ADDRESS_STRING_SIZE);
	printk("Reply received from: %s\n", string);

	if (!peer_address_info.address_found) {
		strcpy(peer_address_info.address_string, string);
		peer_address_info.address_found = true;
	}
}

void ot_get_peer_address(uint64_t timeout_ms)
{
	const char *dest = "ff03::1"; /* Mesh-Local anycast for all FTDs and MEDs */
	uint64_t start_time;
	otPingSenderConfig config;

	/* printk("Finding other devices..."); */
	memset(&config, 0, sizeof(config));
	config.mReplyCallback = ot_handle_ping_reply;

	openthread_api_mutex_lock(openthread_get_default_context());
	otIp6AddressFromString(dest, &config.mDestination);
	otPingSenderPing(openthread_get_default_instance(), &config);
	openthread_api_mutex_unlock(openthread_get_default_context());

	start_time = k_uptime_get();
	while (!peer_address_info.address_found && k_uptime_get() < start_time + timeout_ms) {
		k_sleep(K_MSEC(100));
	}
}

void ot_start_zperf_test_send(const char *peer_addr, uint32_t duration_sec,
	uint32_t packet_size_bytes,	uint32_t rate_bps, bool is_ot_zperf_udp)
{
	zperf_upload(peer_addr, duration_sec, packet_size_bytes, rate_bps, is_ot_zperf_udp);
}


void ot_start_zperf_test_recv(bool is_ot_zperf_udp)
{
	zperf_download(is_ot_zperf_udp);
}

void ot_zperf_test(bool is_ot_client, bool is_ot_zperf_udp, uint32_t test_duration)
{
	if (is_ot_client) {
		uint32_t ot_zperf_duration_sec = test_duration;

		ot_start_zperf_test_send(peer_address_info.address_string, ot_zperf_duration_sec,
		CONFIG_OT_PACKET_SIZE, CONFIG_OT_RATE_BPS, is_ot_zperf_udp);
	}
	/* Note: ot_start_zperf_test_recv() done as part of init */
}


#ifdef CONFIG_NRF70_SR_COEX
static int coex_configure_the_pta(const struct shell *shell,
			 size_t argc,
			 const char *argv[])
{
	/* Configure the PTA */
	int result_non_pta = 0;
	int result_pta = 0;
	int result = 0;
	bool wlan_band  = 0; /* default 2.4GHz */
	bool separate_antennas = 0; /* default shared antenna */
	bool is_sr_protocol_ble = 0; /* default Thread */

	if (argc < 4) {
		shell_fprintf(shell, SHELL_ERROR, "invalid # of args : %d\n", argc);
		shell_fprintf(shell, SHELL_ERROR, "Usage: coex_config_pta");
		shell_fprintf(shell, SHELL_ERROR, " wifi_band is_sep_antennas is_sr_ble");
		shell_fprintf(shell, SHELL_ERROR, " is_sr_ble\n\n");
		printk("       wlan_band: 0 for 2.4GHz, 1 for 5GHz\n");
		printk("       is_sep_antennas: 0 for shared antenna,");
		printk(" 1 for separate antennas\n");
		printk("       is_sr_ble: 0 for Thread, 1 for Bluetooth protocol\n");
		return -ENOEXEC;
	}

	wlan_band  = strtoul(argv[1], NULL, 0);
	separate_antennas  = strtoul(argv[2], NULL, 0);
	is_sr_protocol_ble  = strtoul(argv[3], NULL, 0);

	if (wlan_band) {
		printk("WLAN operating band: 5GHz\n");
	} else {
		printk("WLAN operating band: 2.4GHz\n");
	}
	if (separate_antennas) {
		printk("Antenna mode: Wi-Fi and SR uses separate antennas\n");
	} else {
		printk("Antenna mode: Wi-Fi and SR shares antenna\n");
	}
	if (is_sr_protocol_ble) {
		printk("SR protocol: Bluetooth LE\n");
	} else {
		printk("SR protocol: Thread\n");
	}

	result_non_pta = nrf_wifi_coex_config_non_pta(separate_antennas,
						is_sr_protocol_ble);
	result_pta = nrf_wifi_coex_config_pta(wlan_band, separate_antennas,
					is_sr_protocol_ble);
	result = result_non_pta & result_pta;

	if (result) {
		shell_fprintf(shell, SHELL_ERROR, "Configuration of PTA - FAIL\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Configuration of PTA - SUCCESS\n");
	}

	return result;
}
#endif

static int coex_hardware_disable(const struct shell *shell, size_t argc,
			char **argv)
{
	/* disable coexistence hardware */
	uint32_t status  = nrf_wifi_coex_hw_reset();

	if (status) {
		shell_fprintf(shell, SHELL_ERROR, "COEXC disable - FAIL\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "COEXC disable - SUCCESS\n");
	}
	return status;
}

#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
static int coex_configure_sr_switch(const struct shell *shell, size_t argc,
			char **argv)
{
	/* Configure SR side switch - to handle both Shared and separate antennas modes */
	int ret = 0;
	bool separate_antennas = 0;

	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "invalid # of args : %d\n", argc);
		shell_fprintf(shell, SHELL_ERROR, "Usage: config_sr_switch is_sep_antennas\n\n");
		printk("       is_sep_antennas: 0 for shared antenna, 1 for separate antennas\n");
		return -ENOEXEC;
	}

	separate_antennas  = strtoul(argv[1], NULL, 0);
	if (separate_antennas) {
		printk("Antenna mode: Wi-Fi and SR uses separate antennas\n");
	} else {
		printk("Antenna mode: Wi-Fi and SR shares antenna\n");
	}

	ret = nrf_wifi_config_sr_switch(separate_antennas);
	if (ret != 0) {
		shell_fprintf(shell, SHELL_ERROR, "Configuration of SR side switch - FAIL\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Configuration of SR side switch - SUCCESS\n");
	}
	return ret;
}
#endif /* CONFIG_NRF70_SR_COEX_RF_SWITCH */

static void udp_upload_results_cb(enum zperf_status status,
			  struct zperf_results *result,
			  void *user_data)
{
	unsigned int client_rate_in_kbps;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		printk("New UDP session started\n");
		break;
	case ZPERF_SESSION_PERIODIC_RESULT:
		/* Ignored. */
		break;
	case ZPERF_SESSION_FINISHED:
		printk("Wi-Fi benchmark: Upload completed!\n");
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
		printk("Upload results:\n");
		printk("%u bytes in %llu ms\n",
				(result->nb_packets_sent * result->packet_size),
				(result->client_time_in_us / USEC_PER_MSEC));
		printk("%u packets sent\n", result->nb_packets_sent);
		printk("%u packets lost\n", result->nb_packets_lost);
		printk("%u packets received\n", result->nb_packets_rcvd);
		k_sem_give(&udp_callback);
		break;
	case ZPERF_SESSION_ERROR:
		printk("UDP session error\n");
		break;
	}
}

static int parse_ipv4_addr(char *host, struct sockaddr_in *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET, host, &addr->sin_addr);
	if (ret < 0) {
		printk("Invalid IPv4 address %s\n", host);
		return -EINVAL;
	}

	printk("IPv4 address %s\n", host);

	return 0;
}

static int wifi_run_throughput(const struct shell *shell, size_t argc,
			char **argv)
{
	struct zperf_upload_params params;
	int ret = 0;

	if (argc < 8) {
		shell_fprintf(shell, SHELL_ERROR, "invalid # of args : %d\n", argc);
		shell_fprintf(shell, SHELL_ERROR, "Usage: wifi_run_tput protocol direction");
		shell_fprintf(shell, SHELL_ERROR, " peerIP destPort duration pktSize");
		shell_fprintf(shell, SHELL_ERROR, " baudrate\n\n");
		printk("       protocol - udp or tcp\n");
		printk("       direction - upload or download\n");
		printk("       peerIP - IP of the peer device\n");
		printk("       destPort - port of the peer device\n");
		printk("       duration - test duration in seconds\n");
		printk("       pktSize - in bytes or kilobyte (with suffix K)\n");
		printk("       baudrate - baudrate in kilobyte/megabyte (with suffix K/M)\n");
		return -ENOEXEC;
	}

	uint32_t port  = strtoul(argv[4], NULL, 0);
	uint32_t duration_ms  = strtoul(argv[5], NULL, 0);
	uint32_t packet_size  = strtoul(argv[6], NULL, 0);
	uint32_t rate_kbps  = strtoul(argv[7], NULL, 0);

	parse_ipv4_addr(argv[3], &addr_my);
	params.duration_ms = duration_ms * 1000; /* sec to ms */
	params.rate_kbps = rate_kbps*1000;
	params.packet_size = packet_size * 1024;

	net_sprint_ipv4_addr(&addr_my.sin_addr);
	addr_my.sin_port = htons(port);

	memcpy(&params.peer_addr, &addr_my, sizeof(addr_my));

	if (strcmp(argv[1], "udp") == 0 && strcmp(argv[2], "upload") == 0) {
		printk("Starting Wi-Fi benchmark: Zperf udp client\n");

		ret = zperf_udp_upload_async(&params, udp_upload_results_cb, NULL);
	} else if (strcmp(argv[1], "udp") == 0 && strcmp(argv[2], "download") == 0) {
		LOG_ERR("Currently not supported");
		ret = -ENOEXEC;
	} else if (strcmp(argv[1], "tcp") == 0 && strcmp(argv[2], "upload") == 0) {
		LOG_ERR("Currently not supported");
		ret = -ENOEXEC;
	} else if (strcmp(argv[1], "tcp") == 0 && strcmp(argv[2], "download") == 0) {
		LOG_ERR("Currently not supported");
		ret = -ENOEXEC;
	}

	if (ret != 0) {
		shell_fprintf(shell, SHELL_ERROR, "Running Wi-Fi throughput - FAIL\n");
	}
	return ret;
}

#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
SHELL_CMD_REGISTER(coex_cfg_sr_switch, NULL, "Configure SR side switch", coex_configure_sr_switch);
#endif /* CONFIG_NRF70_SR_COEX_RF_SWITCH */
#ifdef CONFIG_NRF70_SR_COEX
SHELL_CMD_REGISTER(coex_config_pta, NULL, "Run the test", coex_configure_the_pta);
#endif /* CONFIG_NRF70_SR_COEX */
SHELL_CMD_REGISTER(coex_hw_disable, NULL, "Disable coexistence hardwaret", coex_hardware_disable);

SHELL_CMD_REGISTER(wifi_run_tput, NULL, "Run Wi-Fi throughput", wifi_run_throughput);

SHELL_CMD_REGISTER(ot_cfg_tput, NULL, "Run Thread config for throughput", ot_configure_throughput);
SHELL_CMD_REGISTER(ot_run_tput, NULL, "Run Thread throughput", ot_run_throughput);
SHELL_CMD_REGISTER(ot_disconnect, NULL, "Run Thread disconnect", ot_disconnection);
