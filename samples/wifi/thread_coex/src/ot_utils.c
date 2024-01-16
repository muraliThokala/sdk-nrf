/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ot_utils.h"

#include "openthread/ping_sender.h"
#include <zephyr/logging/log.h>
#include <openthread/thread.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

extern uint8_t ot_wait4_ping_reply_from_peer;

LOG_MODULE_REGISTER(ot_utils, CONFIG_LOG_DEFAULT_LEVEL);

#if defined(CONFIG_WIFI_SCAN_OT_CONNECTION) || \
	defined(CONFIG_WIFI_CON_SCAN_OT_CONNECTION) || \
	defined(CONFIG_WIFI_TP_UDP_CLIENT_OT_CONNECTION) || \
	defined(CONFIG_WIFI_TP_UDP_SERVER_OT_CONNECTION) || \
	defined(CONFIG_WIFI_TP_TCP_CLIENT_OT_CONNECTION) || \
	defined(CONFIG_WIFI_TP_TCP_SERVER_OT_CONNECTION) || \
	defined(CONFIG_WIFI_SHUTDOWN_OT_CONNECTION)
	/**nothing . These are the tests in which the Thread connection
	 *is done multiple times in a loop
	 */
	 #define OT_ITERATIVE_CONNECTION
#endif

#include <zephyr/kernel.h>
#include <zephyr/console/console.h>

#include <zephyr/types.h>

#include <zephyr/shell/shell_uart.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define THROUGHPUT_CONFIG_TIMEOUT 20
#define SCAN_CONFIG_TIMEOUT 20

#define WAIT_TIME_FOR_OT_DISC K_SECONDS(4)
#define SCAN_START_CONFIG_TIMEOUT K_SECONDS(10)
#define WAIT_TIME_FOR_OT_CON K_SECONDS(4)
#define WAIT_TIME_FOR_OT_DISCON K_SECONDS(5)
#define K_SLEEP_DUR_FOR_OT_CONN K_SECONDS(3)

extern bool is_ot_device_role_client;

uint32_t ot_connection_success_cnt;
uint32_t ot_connection_attempt_cnt;
uint32_t ot_join_success;

static K_SEM_DEFINE(throughput_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(connected_sem, 0, 1);

bool ot_client_connected;

static void ot_commissioner_state_changed(otCommissionerState aState, void *aContext)
{
	LOG_INF("OT commissioner state changed");
	if(aState == OT_COMMISSIONER_STATE_ACTIVE)
	{
		otCommissionerAddJoiner(openthread_get_default_instance(), NULL, "FEDCBA9876543210",2000);
	}
}

static void ot_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data)
{
	LOG_INF("OT device state changed");
	if (flags & OT_CHANGED_THREAD_ROLE) {
		otDeviceRole ot_role = otThreadGetDeviceRole(ot_context->instance);
		if (ot_role != OT_DEVICE_ROLE_DETACHED && ot_role != OT_DEVICE_ROLE_DISABLED) {
			/* ot commissioner start */
			LOG_INF("ot commissioner start");
			otCommissionerStart(ot_context->instance, &ot_commissioner_state_changed, NULL, NULL);
		}
	}
}

static struct openthread_state_changed_cb ot_state_chaged_cb = {
	.state_changed_cb = ot_thread_state_changed
};


/* call back Thread device joiner */
static void ot_joiner_start_handler(otError error, void *context)
{
	ot_join_success = 0; /* as default */

	switch (error) {

	case OT_ERROR_NONE:
		ot_connection_success_cnt++;
		ot_join_success = 1;
		ot_client_connected = true;
		/* LOG_INF("ot_client_connected: %d",ot_client_connected); */

		LOG_INF("Thread Join success");
#if 0
	/* if (ot_connection_attempt_cnt==1) { */
		/** Step3: Start Thread i.e ot thread start
		 *   Note: Device should join Thread network within 20s of timeout.
		 */
		LOG_ERR("++++++++++++ ot thread start done only once ");
		openthread_api_mutex_lock(openthread_get_default_context());
		/*  ot thread start */
		otError err = otThreadSetEnabled(openthread_get_default_instance(), true);

		if (err != OT_ERROR_NONE) {
			LOG_ERR("Starting openthread: %d (%s)",
				err, otThreadErrorToString(err));
		}
		openthread_api_mutex_unlock(openthread_get_default_context());
	/* } */
#endif
		/* not required for Thread connection stability tests. */
		k_sem_give(&connected_sem);
	break;

	default:
		LOG_ERR("Join failed [%s]", otThreadErrorToString(error));
		ot_join_success = 0;
	break;
	}
}

int ot_throughput_test_run(bool is_ot_zperf_udp)
{
	otError err = 0;

	if (is_ot_device_role_client) {
		ot_start_joiner("FEDCBA9876543210");
		k_sleep(K_SECONDS(2));
		err = k_sem_take(&connected_sem, WAIT_TIME_FOR_OT_CON);

		LOG_INF("Starting openthread.");
		openthread_api_mutex_lock(openthread_get_default_context());
		/*  ot thread start */
		err = otThreadSetEnabled(openthread_get_default_instance(), true);
		if (err != OT_ERROR_NONE) {
			LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
		}

		otDeviceRole current_role =
			otThreadGetDeviceRole(openthread_get_default_instance());
		openthread_api_mutex_unlock(openthread_get_default_context());

		while (current_role != OT_DEVICE_ROLE_CHILD) {
			LOG_INF("Current role of Thread device: %s",
				otThreadDeviceRoleToString(current_role));
			k_sleep(K_MSEC(1000));
			openthread_api_mutex_lock(openthread_get_default_context());
			current_role = otThreadGetDeviceRole(openthread_get_default_instance());
			openthread_api_mutex_unlock(openthread_get_default_context());
		}
		ot_zperf_test(is_ot_zperf_udp);
		/* Only for client case. Server case is handled as part of init */
	}
	return 0;
}


void ot_start_joiner(const char *pskd)
{
	LOG_INF("Starting joiner");

	otInstance *instance = openthread_get_default_instance();
	struct openthread_context *context = openthread_get_default_context();

	openthread_api_mutex_lock(context);

	/** Step1: Set null network key i.e,
	 * ot networkkey 00000000000000000000000000000000
	 */
	ot_setNullNetworkKey(instance); /* added new */

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
	/* LOG_INF("Thread start joiner Done."); */
}

int ot_throughput_test_init(bool is_ot_client, bool is_ot_zperf_udp)
{

	if (!is_ot_client) { /* only for server */
		
		
		ot_initialization();

		struct openthread_context *ot_context = openthread_get_default_context();
		openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);

		LOG_INF("Waiting until OT init is done");		
#if 0
		LOG_INF("OT device initialization with ksleep after init");	
		while (otCommissionerGetState(ot_context->instance) != OT_COMMISSIONER_STATE_ACTIVE){
			/** IPC timeout occurs without this. 
			  * spinel_ipc_backend_rsp_ntf: No response within timeout 500 
			  */
			k_sleep(K_MSEC(100));
		}
#else
		LOG_INF("OT device initialization with ksleep after init");	
		k_sleep(K_MSEC(10000));
		
#endif		
		LOG_INF("ot commissioner joiner add * FEDCBA9876543210 2000");
		k_sleep(K_MSEC(2000));
		otCommissionerAddJoiner(ot_context->instance, NULL, "FEDCBA9876543210",2000);

		LOG_INF("Starting zperf server");
		ot_start_zperf_test_recv(is_ot_zperf_udp);

		LOG_INF("Run OT client on peer");
	}
	return 0;
}

int ot_tput_test_exit(void)
{
	/* currently blank. This should contain disconnect/remove the device from network */
	return 0;
}

void ot_setNullNetworkKey(otInstance *aInstance)
{
	otOperationalDataset aDataset;
	uint8_t key[OT_NETWORK_KEY_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	memset(&aDataset, 0, sizeof(otOperationalDataset));

	/* Set network key to null */
	memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
	aDataset.mComponents.mIsNetworkKeyPresent = true;
	otDatasetSetActive(aInstance, &aDataset);
}

static void ot_setNetworkConfiguration(otInstance *aInstance)
{
	static char aNetworkName[] = "TestNetwork";
	otOperationalDataset aDataset;
	size_t length;
	uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x11, 0x11, 0x11, 0x11,	0x11, 0x11, 0x11, 0x11};
	uint8_t key[OT_NETWORK_KEY_SIZE] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
		0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

	memset(&aDataset, 0, sizeof(otOperationalDataset));

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

	/* LOG_INF("Updating thread parameters"); */
	ot_setNetworkConfiguration(instance);
	/* LOG_INF("Enabling thread"); */
	otError err = openthread_start(context); /* 'ifconfig up && thread start' */

	if (err != OT_ERROR_NONE) {
		LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
	}
	otDeviceRole current_role = otThreadGetDeviceRole(instance);

	LOG_INF("Current role of Thread device: %s", otThreadDeviceRoleToString(current_role));
	return 0;
}

typedef struct peer_address_info {
	char address_string[OT_IP6_ADDRESS_STRING_SIZE];
	bool address_found;
} peer_address_info_t;

peer_address_info_t peer_address_info  = {.address_found = false};

void ot_handle_ping_reply(const otPingSenderReply *reply, void *context)
{
	otIp6Address add = reply->mSenderAddress;
	char string[OT_IP6_ADDRESS_STRING_SIZE];

	otIp6AddressToString(&add, string, OT_IP6_ADDRESS_STRING_SIZE);
	LOG_WRN("Reply received from: %s\n", string);

	ot_wait4_ping_reply_from_peer = 1;

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

	LOG_INF("Finding other devices...");
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
#ifdef CONFIG_NET_SHELL
	const struct shell *shell = shell_backend_uart_get_ptr();
	char cmd[128];
	
#if 0
	LOG_INF("peer addr %s ", peer_addr);
	LOG_INF("port %u ", CONFIG_NET_CONFIG_THREAD_PORT);
	LOG_INF("duration_sec %u ", duration_sec);
	LOG_INF("packet_size_bytes %u", packet_size_bytes);
	LOG_INF("rate_bps %u", rate_bps);
	LOG_INF("interface %s", CONFIG_ZPERF_THREAD_INTERFACE);
	LOG_INF("string size %u", OT_IP6_ADDRESS_STRING_SIZE);
#endif

	if (is_ot_zperf_udp) {
		snprintf(cmd, sizeof(cmd), "zperf udp upload %s %u %u %u %u %s", peer_addr,
			CONFIG_NET_CONFIG_THREAD_PORT, duration_sec, packet_size_bytes, rate_bps,
			CONFIG_ZPERF_THREAD_INTERFACE);
	} else {
		/* PENDING .. to be updated with right command. */
		snprintf(cmd, sizeof(cmd), "zperf tcp upload %s %u %u %u %u %s", peer_addr,
			CONFIG_NET_CONFIG_THREAD_PORT, duration_sec, packet_size_bytes, rate_bps,
			CONFIG_ZPERF_THREAD_INTERFACE);
	}
	shell_execute_cmd(shell, cmd);
#endif
}

void ot_start_zperf_test_recv(bool is_ot_zperf_udp)
{
#ifdef CONFIG_NET_SHELL
	const struct shell *shell = shell_backend_uart_get_ptr();

	if (is_ot_zperf_udp) {
		shell_execute_cmd(shell, "zperf udp download");
	} else {
		/* PENDING .. to be updated correctly (should add port info?) */
		shell_execute_cmd(shell, "zperf tcp download");
	}
#endif
}

void ot_zperf_test(bool is_ot_zperf_udp)
{
	if (is_ot_device_role_client) {
		ot_get_peer_address(5000);
		if (!peer_address_info.address_found) {
			LOG_WRN("Peer address not found. Not continuing with zperf test.");
			return;
		}
		ot_start_zperf_test_send(peer_address_info.address_string, CONFIG_OT_ZPERF_DURATION,
		CONFIG_OT_PACKET_SIZE, CONFIG_OT_RATE_BPS, is_ot_zperf_udp);
	}
	/* Note: ot_start_zperf_test_recv() done as part of init */
}
