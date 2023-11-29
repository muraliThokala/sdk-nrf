/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_utils.h"
#include "openthread/ping_sender.h"
#include "zephyr/net/openthread.h"

#include <zephyr/logging/log.h>
#include <openthread/thread.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

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
//#else
//	#define OT_TX_PWR_CTRL_RSSI
#endif

uint32_t repeat_ot_discovery;
uint32_t ot_discov_success_cnt;
uint32_t ot_discov_attempt_cnt;
uint32_t ot_discov_no_result_cnt;
extern uint32_t ot_discov_timeout;

uint32_t ot_connection_success_cnt;
uint32_t ot_connection_attempt_cnt;
uint32_t ot_join_success;

uint32_t ot_disconnection_attempt_cnt;
uint32_t ot_disconnection_success_cnt;
uint32_t ot_disconnection_fail_cnt;
uint32_t ot_discon_no_conn_cnt;
uint32_t ot_discon_no_conn;

uint32_t wifi_scan_cmd_cnt;
extern uint32_t run_ot_client_wait_in_conn;

extern uint32_t ot_datalen_failed;
extern uint32_t ot_phy_update_failed;
extern uint32_t ot_datalen_timeout;

extern uint32_t ot_phy_update_timeout;
extern uint32_t ot_conn_param_update_failed;
extern uint32_t ot_conn_param_update_timeout;

int8_t ot_tx_power = TXPOWER_INIT_VALUE;
int8_t ot_rssi = RSSI_INIT_VALUE;
static int print_ot_connnection_status_once = 1;
static int is_calback_from_loop = 0;

//#ifdef OT_TX_PWR_CTRL_RSSI
//	/* to get/set Thread Tx power and read Thread RSSI for coex sample */
//	#include <stddef.h>
//	#include <zephyr/sys/printk.h>
//	#include <zephyr/sys/util.h>
//	#include <zephyr/sys/byteorder.h>
//	#include <zephyr/bluetooth/hci_vs.h>
//
//	#include <zephyr/bluetooth/services/hrs.h>
//
//	static uint16_t default_conn_handle;
//#endif


#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
/*#include <zephyr/sys/LOG_INF.h>*/

#include <zephyr/types.h>

#include <zephyr/shell/shell_uart.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define THROUGHPUT_CONFIG_TIMEOUT 20
#define SCAN_CONFIG_TIMEOUT 20


/* #define PRINT_OT_UPDATES */
#define WAIT_TIME_FOR_OT_DISC K_SECONDS(4)
#define SCAN_START_CONFIG_TIMEOUT K_SECONDS(10)
#define WAIT_TIME_FOR_OT_CON K_SECONDS(4)
#define WAIT_TIME_FOR_OT_DISCON K_SECONDS(5)
#define K_SLEEP_DUR_FOR_OT_CONN K_SECONDS(3)


static K_SEM_DEFINE(throughput_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(connected_sem, 0, 1);

extern uint8_t wait4_peer_ot2_start_connection;
bool ot_server_connected;
bool ot_client_connected;

uint64_t ot_disc2conn_start_time;
int64_t ot_scan2conn_time;
uint32_t ot_disconn_cnt_stability;

static volatile bool data_length_req;
static volatile bool test_ready;

//static struct bt_conn *default_conn;
//static struct bt_throughput throughput;
//static struct bt_uuid *uuid128 = BT_UUID_THROUGHPUT;
//static struct bt_gatt_exchange_params exchange_params;

//static struct bt_le_conn_param *conn_param =
//	BT_LE_CONN_PARAM(CONFIG_BT_INTERVAL_MIN, CONFIG_BT_INTERVAL_MAX, 0, 400);

//static const struct bt_data ad[] = {
//	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
//	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
//		0xBB, 0x4A, 0xFF, 0x4F, 0xAD, 0x03, 0x41, 0x5D,
//		0xA9, 0x6C, 0x9D, 0x6C, 0xDD, 0xDA, 0x83, 0x04),
//};

//static const struct bt_data sd[] = {
//	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
//};

//void button_handler_cb(uint32_t button_state, uint32_t has_changed);

//#ifdef PRINT_OT_UPDATES
//static const char *phy2str(uint8_t phy)
//{
//	switch (phy) {
//	case 0: return "No packets";
//	case BT_GAP_LE_PHY_1M: return "LE 1M";
//	case BT_GAP_LE_PHY_2M: return "LE 2M";
//	case BT_GAP_LE_PHY_CODED: return "LE Coded";
//	default: return "Unknown";
//	}
//}
//#endif

//void instruction_print(void)
//{
//	/**
//	 * LOG_INF("Type 'config' to change the configuration parameters.");
//	 * LOG_INF("You can use the Tab key to autocomplete your input.");
//	 * LOG_INF("Type 'run' when you are ready to run the test.");
//	 */
//}


//void scan_filter_match(struct bt_scan_device_info *device_info,
//		       struct bt_scan_filter_match *filter_match,
//		       bool connectable)
//{
//	char addr[BT_ADDR_LE_STR_LEN];
//
//	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
//
//#ifdef PRINT_OT_UPDATES
//	LOG_INF("Filters matched. Address: %s connectable: %d",
//		addr, connectable);
//#endif
//}

//void scan_filter_no_match(struct bt_scan_device_info *device_info,
//			  bool connectable)
//{
//	char addr[BT_ADDR_LE_STR_LEN];
//
//	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
//	/* #ifdef CONFIG_PRINTS_FOR_AUTOMATION
//	printk("Filter not match. Address: %s connectable: %d\n",
//				addr, connectable);
//	#endif */
//}

//void scan_connecting_error(struct bt_scan_device_info *device_info)
//{
//	LOG_ERR("Connecting failed");
//}
//
//BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match,
//		scan_connecting_error, NULL);

//void exchange_func(struct bt_conn *conn, uint8_t att_err,
//			  struct bt_gatt_exchange_params *params)
//{
//	struct bt_conn_info info = {0};
//	int err;
//#ifdef PRINT_OT_UPDATES
//	LOG_INF("MTU exchange %s", att_err == 0 ? "successful" : "failed");
//#endif
//
//	err = bt_conn_get_info(conn, &info);
//	if (err) {
//		LOG_ERR("Failed to get connection info %d", err);
//		return;
//	}
//
//	if (info.role == BT_CONN_ROLE_CENTRAL) {
//		instruction_print();
//		test_ready = true;
//	}
//	k_sem_give(&connected_sem);
//}

//void discovery_complete(struct bt_gatt_dm *dm,
//			       void *context)
//{
//	int err;
//	struct bt_throughput *throughput = context;
//#ifdef PRINT_OT_UPDATES
//	LOG_INF("Service discovery completed");
//#endif
//	bt_gatt_dm_data_print(dm);
//	bt_throughput_handles_assign(dm, throughput);
//	bt_gatt_dm_data_release(dm);
//
//	exchange_params.func = exchange_func;
//
//	err = bt_gatt_exchange_mtu(default_conn, &exchange_params);
//
//	if (err) {
//#ifdef PRINT_OT_UPDATES
//		LOG_ERR("MTU exchange failed (err %d)", err);
//#endif
//	} else {
//#ifdef PRINT_OT_UPDATES
//		LOG_INF("MTU exchange pending");
//#endif
//	}
//}

//void discovery_service_not_found(struct bt_conn *conn,
//					void *context)
//{
//	LOG_INF("Service not found");
//}

//void discovery_error(struct bt_conn *conn,
//			    int err,
//			    void *context)
//{
//	LOG_INF("Error while discovering GATT database: (%d)", err);
//}

//struct bt_gatt_dm_cb discovery_cb = {
//	.completed         = discovery_complete,
//	.service_not_found = discovery_service_not_found,
//	.error_found       = discovery_error,
//};

/* call back Thread device joiner */
static void ot_joiner_start_handler(otError error, void *context)
{
	ot_join_success = 0; //as default
    switch (error)
    {
		case OT_ERROR_NONE:
			ot_connection_success_cnt++;			
			ot_join_success = 1;
			ot_client_connected = true;
			//LOG_INF("ot_client_connected: %d",ot_client_connected);
			
			LOG_INF("Thread Join success");

// +++++++++++++++++++ TODO Enable this.

#if 0		
		//if (ot_connection_attempt_cnt==1) { // do this only once after first join attempt.
			/** Step3: Start Thread i.e ot thread start
			 *   Note: Device should join Thread network within 20s of timeout.
			 */
			LOG_ERR("++++++++++++ ot thread start done only once ");
			openthread_api_mutex_lock(openthread_get_default_context());
			otError err = otThreadSetEnabled(openthread_get_default_instance(), true); /*  ot thread start */
			if (err != OT_ERROR_NONE) {
				LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
			}
			openthread_api_mutex_unlock(openthread_get_default_context());
		//}
#endif	
			
			
			// not required for Thread connection stability tests.
			k_sem_give(&connected_sem);
        break;

		default:
			LOG_ERR("Join failed [%s]", otThreadErrorToString(error));
			ot_join_success = 0;
			break;
		}
}
//void connected(struct bt_conn *conn, uint8_t hci_err)
//{
//	struct bt_conn_info info = {0};
//	int err;
//
//	if (hci_err) {
//		if (hci_err == BT_HCI_ERR_UNKNOWN_CONN_ID) {
//			/* Canceled creating connection */
//			return;
//		}
//		LOG_ERR("Connection failed (err 0x%02x)", hci_err);
//		return;
//	}
//
//	if (default_conn) {
//		LOG_INF("Connection exists, disconnect second connection");
//		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
//		return;
//	}
//
//	default_conn = bt_conn_ref(conn);
//
//	err = bt_conn_get_info(default_conn, &info);
//	if (err) {
//		LOG_ERR("Failed to get connection info %d", err);
//
//		return;
//	}
//	ot_connection_success_cnt++;
//	ot_client_connected = true;
//	ot_server_connected = true;
//#ifdef CONFIG_PRINTS_FOR_AUTOMATION
//	is_calback_from_loop++;
//	if (is_calback_from_loop == 2) { 
//		run_ot_client_wait_in_conn = 1;
//	}
//	if (print_ot_connnection_status_once) {
//		LOG_INF("Connected as %s", info.role ==
//			BT_CONN_ROLE_CENTRAL ? "central" : "peripheral");
//		LOG_INF("Conn. interval is %u units", info.le.interval);
//		print_ot_connnection_status_once = 0;
//	}
//#endif
//	if (info.role == BT_CONN_ROLE_CENTRAL) {
//		err = bt_gatt_dm_start(default_conn,
//				BT_UUID_THROUGHPUT,
//				&discovery_cb,
//				&throughput);
//
//		if (err) {
//			LOG_ERR("Discover failed (err %d)", err);
//		}
//	}
//	//k_sem_give(&connected_sem);
//	#ifdef OT_TX_PWR_CTRL_RSSI
//		char addr[BT_ADDR_LE_STR_LEN];
//		int8_t get_txp = 0;
//		int8_t set_txp = 0;
//		int ret;
//		int8_t rssi = 0xFF;
//
//		printk("Thread Target Tx power %d\n", set_txp);
//		default_conn = bt_conn_ref(conn);
//		ret = bt_hci_get_conn_handle(default_conn,
//						 &default_conn_handle);
//		if (ret) {
//			printk("No connection handle (err %d)\n", ret);
//		} else {
//			read_conn_rssi(default_conn_handle, &rssi);
//			/* printk("Connected (%d) - RSSI = %d\n", */
//			/*	   default_conn_handle, rssi); */
//
//			/* Send first at the default selected power */
//			bt_addr_le_to_str(bt_conn_get_dst(conn),
//							  addr, sizeof(addr));
//			/* printk("Connected via connection (%d) at %s\n", */
//			/*	   default_conn_handle, addr); */
//			get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
//					 default_conn_handle, &get_txp);
//			/* printk("Connection (%d) - Initial Tx Power = %d\n", */
//			/*    default_conn_handle, get_txp); */
//			/* sets Tx power to RADIO_TXP_DEFAULT */
//			/* printk("Changing Tx power to = %d\n", set_txp); */
//
//			set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
//					 default_conn_handle,
//					 set_txp);
//			get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
//					 default_conn_handle, &get_txp);
//			printk("Thread connection (%d)\n", default_conn_handle);
//			printk("coex sample -->connected(): Thread Tx Power: %d\n", get_txp);
//			read_conn_rssi(default_conn_handle, &rssi);
//			printk("coex sample -->connected(): Thread RSSI: %d\n", rssi);
//			ot_tx_power = get_txp;
//			ot_rssi = rssi;
//		}
//	#endif
//}

//void scan_init(void)
//{
//	int err;
//	struct bt_le_scan_param scan_param = {
//		.type = BT_LE_SCAN_TYPE_PASSIVE,
//		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
//		.interval = CONFIG_BT_LE_SCAN_INTERVAL,
//		.window = CONFIG_BT_LE_SCAN_WINDOW,
//	};
//
//	struct bt_scan_init_param scan_init = {
//		.connect_if_match = 1,
//		.scan_param = &scan_param,
//		.conn_param = conn_param
//	};
//
//	bt_scan_init(&scan_init);
//	bt_scan_cb_register(&scan_cb);
//
//	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, uuid128);
//	if (err) {
//		LOG_INF("Scanning filters cannot be set");
//
//		return;
//	}
//
//	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
//	if (err) {
//		LOG_INF("Filters cannot be turned on");
//	}
//}

//void scan_start(void)
//{
//	int err;
//
//	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
//	if (err) {
//	#ifdef PRINT_OT_UPDATES
//		LOG_ERR("Starting scanning failed (err %d)", err);
//	#endif
//		return;
//	}
//}

//void adv_start(void)
//{
//	struct bt_le_adv_param *adv_param =
//		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE |
//				BT_LE_ADV_OPT_ONE_TIME,
//				CONFIG_BT_GAP_ADV_FAST_INT_MIN_2,
//				CONFIG_BT_GAP_ADV_FAST_INT_MAX_2,
//				NULL);
//	int err;
//
//	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd,
//			      ARRAY_SIZE(sd));
//	if (err) {
//		LOG_ERR("Failed to start advertiser (%d)", err);
//		return;
//	}
//}

//void disconnected(struct bt_conn *conn, uint8_t reason)
//{
//#ifdef OT_ITERATIVE_CONNECTION
//	struct bt_conn_info info = {0};
//	int err = 0;
//#endif 
//
//#ifdef PRINT_OT_UPDATES
//	LOG_INF("Disconnected (reason 0x%02x)", reason);
//#endif
//
//	test_ready = false;
//	ot_server_connected = false;
//	ot_client_connected = false;
//	if (default_conn) {
//		bt_conn_unref(default_conn);
//		default_conn = NULL;
//	}
//
//#ifdef OT_ITERATIVE_CONNECTION
//	/* Disconnection count for central is available in ot_disconnect_client() */
//	if (!IS_ENABLED(CONFIG_OT_ROLE_CLIENT)) {
//		ot_disconnection_success_cnt++;
//	}
//
//	err = bt_conn_get_info(conn, &info);
//	if (err) {
//		printk("Failed to get connection info (%d)\n", err);
//		return;
//	}
//		/* Re-connect using same roles */
//		if (info.role == BT_CONN_ROLE_CENTRAL) {
//			ot_connection_attempt_cnt++;
//			scan_start(); 		
//		} else {
//			adv_start();
//		}
//		k_sem_give(&disconnected_sem);
//#endif
//}

//static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
//{
//	LOG_INF("Connection parameters update request received.");
//	LOG_INF("Minimum interval: %d, Maximum interval: %d",
//	       param->interval_min, param->interval_max);
//	LOG_INF("Latency: %d, Timeout: %d", param->latency, param->timeout);
//
//	return true;
//}

//void le_param_updated(struct bt_conn *conn, uint16_t interval,
//			     uint16_t latency, uint16_t timeout)
//{
//	LOG_INF("Connection parameters updated."
//	       " interval: %d, latency: %d, timeout: %d",
//	       interval, latency, timeout);
//
//	k_sem_give(&throughput_sem);
//}

//void le_phy_updated(struct bt_conn *conn,
//			   struct bt_conn_le_phy_info *param)
//{
//#ifdef PRINT_OT_UPDATES
//	LOG_INF("LE PHY updated: TX PHY %s, RX PHY %s",
//	       phy2str(param->tx_phy), phy2str(param->rx_phy));
//#endif
//	k_sem_give(&throughput_sem);
//}

//void le_data_length_updated(struct bt_conn *conn,
//				   struct bt_conn_le_data_len_info *info)
//{
//	if (!data_length_req) {
//		return;
//	}
//#ifdef PRINT_OT_UPDATES
//	LOG_INF("LE data len updated: TX (len: %d time: %d)"
//	       " RX (len: %d time: %d)", info->tx_max_len,
//	       info->tx_max_time, info->rx_max_len, info->rx_max_time);
//#endif
//	data_length_req = false;
//	k_sem_give(&throughput_sem);
//}


//static uint8_t ot_throughput_read(const struct bt_throughput_metrics *met)
static uint8_t ot_throughput_read(void)
{
//	LOG_INF("[peer] received %u bytes (%u KB)"
//	       " in %u GATT writes at %u bps",
//	       met->write_len, met->write_len / 1024, met->write_count,
//	       met->write_rate);
//
//	k_sem_give(&throughput_sem);
//
//	return BT_GATT_ITER_STOP;
	return 0;
}

//void ot_throughput_received(const struct bt_throughput_metrics *met)
void ot_throughput_received(void)
{
//	static uint32_t kb;
//
//	if (met->write_len == 0) {
//		kb = 0;
//#ifdef CONFIG_PRINTS_FOR_AUTOMATION
//		wait4_peer_ot2_start_connection = 1;
//		LOG_INF("");
//#endif
//
//		return;
//	}
//
//	if ((met->write_len / 1024) != kb) {
//		kb = (met->write_len / 1024);
//		#ifndef CONFIG_PRINTS_FOR_AUTOMATION
//			LOG_INF("=");
//		#endif
//	}
}
//void ot_throughput_send(const struct bt_throughput_metrics *met)
void ot_throughput_send(void)
{
//	LOG_INF("[local] received %u bytes (%u KB)"
//		" in %u GATT writes at %u bps",
//		met->write_len, met->write_len / 1024,
//		met->write_count, met->write_rate);
}

//static struct button_handler button = {
//	.cb = button_handler_cb,
//};

//void select_role(bool is_central)
//{
//	int err;
//	static bool role_selected;
//
//	if (role_selected) {
//		LOG_INF("Cannot change role after it was selected once.");
//		return;
//	} else if (is_central) {
//		scan_start();
//	} else {
//		adv_start();
//	}
//
//	role_selected = true;
//
//	/* The role has been selected, button are not needed any more. */
//	err = dk_button_handler_remove(&button);
//	if (err) {
//		LOG_INF("Button disable error: %d", err);
//	}
//}

//void button_handler_cb(uint32_t button_state, uint32_t has_changed)
//{
//	ARG_UNUSED(has_changed);
//
//	if (button_state & DK_BTN1_MSK) {
//		select_role(true);
//	} else if (button_state & DK_BTN2_MSK) {
//		select_role(false);
//	}
//}

//void buttons_init(void)
//{
//	int err = 0;
//
//	err = dk_buttons_init(NULL);
//	if (err) {
//		LOG_ERR("Buttons initialization failed.");
//		return;
//	}
//
//	/**
//	 *Add dynamic buttons handler. Buttons should be activated only when
//	 * during the board role choosing.
//	 */
//	dk_button_handler_add(&button);
//}

//int connection_configuration_set(const struct bt_le_conn_param *conn_param,
//			const struct bt_conn_le_phy_param *phy,
//			const struct bt_conn_le_data_len_param *data_len)
//{
//	int err = 0;
//	struct bt_conn_info info = {0};
//
//	err = bt_conn_get_info(default_conn, &info);
//	if (err) {
//		LOG_ERR("Failed to get connection info %d", err);
//		return err;
//	}
//
//	if (info.role != BT_CONN_ROLE_CENTRAL) {
//		LOG_INF("'run' command shall be executed only on the central board");
//	}
//
//	err = bt_conn_le_phy_update(default_conn, phy);
//	if (err) {
//		ot_phy_update_failed++;
//		LOG_ERR("PHY update failed: %d\n", err);
//		return err;
//	}
////#if 0 //#ifdef PRINT_OT_UPDATES
//	LOG_INF("PHY update pending");
////#endif
//	err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT));
//	if (err) {
//		ot_phy_update_timeout++;
//		LOG_INF("PHY update timeout");
//		return err;
//	}
//
//	if (info.le.data_len->tx_max_len != data_len->tx_max_len) {
//		data_length_req = true;
//
//		err = bt_conn_le_data_len_update(default_conn, data_len);
//		if (err) {
//			ot_datalen_failed++;
//			LOG_ERR("LE data length update failed: %d",
//				    err);
//			return err;
//		}
////#if 0 //ifdef PRINT_OT_UPDATES
//		LOG_INF("LE Data length update pending");
////#endif
//		err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT));
//		if (err) {
//			ot_datalen_timeout++;
//			LOG_INF("LE Data Length update timeout");
//			return err;
//		}
//	}
//
//	if (info.le.interval != conn_param->interval_max) {
//		err = bt_conn_le_param_update(default_conn, conn_param);
//		if (err) {
//			ot_conn_param_update_failed++;
//			LOG_ERR("Connection parameters update failed: %d",
//				    err);
//			return err;
//		}
//
//		LOG_INF("Connection parameters update pending");
//		err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT));
//		if (err) {
//			ot_conn_param_update_timeout++;
//			LOG_INF("Connection parameters update timeout");
//			return err;
//		}
//	}
//	/* LOG_INF("supervision timeout %d", conn_param->timeout); */
//	ot_supervision_timeout = conn_param->timeout;
//
//	return 0;
//}

int ot_throughput_test_run(void)
{
	
		int64_t stamp;
	int64_t delta;
	int err = 0;
	/* get cycle stamp */
	stamp = k_uptime_get_32();

	delta = 0;
#if 0 //murali
	while (true) {
		/* start a new connection */
		//if (ot_discon_no_conn != 0) {
		//	ot_discon_no_conn = 0;
			ot_connection_attempt_cnt++;
			
			/* start joining to the network with pre-shared key = FEDCBA9876543210 */
			ot_start_joiner("FEDCBA9876543210");			
		//}

		//Note: with sleep of 2/3 seconds, not observing the issue of one success for 
		// two join attempts. But, observing issue with ksleep of 1sec
		k_sleep(K_SECONDS(2)); /* time sleep between two joiner attempts */


		err = k_sem_take(&connected_sem, WAIT_TIME_FOR_OT_CON);
		
		if ((k_uptime_get_32() - stamp) > CONFIG_COEX_TEST_DURATION) {
			break;
		}
		
		#if 0		
				if (ot_join_success) {
					ot_disconnection_attempt_cnt++;
			
					/** +++++++++++++++  TODO +++++++++++++
					  *remove/stop joiner here to disconnect Thread 
					  */
					ot_stop_joiner();
				}
		
				/** +++++++++++++++  TODO +++++++++++++
				 * currently ot_start_joiner()  is called in this function. this should be called
				 * from ot_stop_joiner()/disconnect() like Thread scan 
				 * is called from the disconnected() callback.
				 */
		
				err = k_sem_take(&disconnected_sem, WAIT_TIME_FOR_OT_DISCON);
				//k_sleep(K_SLEEP_DUR_FOR_OT_CONN); // ksleep already available above. So commenting this.
		#endif
	}
#else
		ot_start_joiner("FEDCBA9876543210");
		k_sleep(K_SECONDS(2));
		err = k_sem_take(&connected_sem, WAIT_TIME_FOR_OT_CON);
#endif

	{
		LOG_INF("Starting openthread.");
		openthread_api_mutex_lock(openthread_get_default_context());
		otError err = otThreadSetEnabled(openthread_get_default_instance(), true); /*  ot thread start */
		if (err != OT_ERROR_NONE) {
			LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
		}
		otDeviceRole current_role = otThreadGetDeviceRole(openthread_get_default_instance());
		openthread_api_mutex_unlock(openthread_get_default_context());
		while (current_role != OT_DEVICE_ROLE_CHILD) {
			LOG_INF("Current role of Thread device: %s", otThreadDeviceRoleToString(current_role));
			k_sleep(K_MSEC(1000));
			openthread_api_mutex_lock(openthread_get_default_context());
			current_role = otThreadGetDeviceRole(openthread_get_default_instance());
			openthread_api_mutex_unlock(openthread_get_default_context());
		}
		zperf_test();
	}
//	int err;
//	int64_t stamp;
//	int64_t delta;
//	uint32_t data = 0;
//
//	/* a dummy data buffer */
//	static char dummy[495];
//
//	if (!default_conn) {
//		LOG_INF("Device is disconnected %s",
//			    "Connect to the peer device before running test");
//		return -EFAULT;
//	}
//
//	if (!test_ready) {
//		LOG_INF("Device is not ready."
//			"Please wait for the service discovery and MTU exchange end");
//		return 0;
//	}
//
//	/* LOG_INF("==== Starting throughput test ===="); */
//
//	/* reset peer metrics */
//	err = ot_throughput_write(&throughput, dummy, 1);
//	if (err) {
//		LOG_ERR("Reset peer metrics failed.");
//		return err;
//	}
//
//	/* get cycle stamp */
//	stamp = k_uptime_get_32();
//
//	delta = 0;
//	while (true) {
//		err = ot_throughput_write(&throughput, dummy, 495);
//		if (err) {
//			LOG_ERR("GATT write failed (err %d)", err);
//			break;
//		}
//		data += 495;
//		if ((k_uptime_get_32() - stamp) > CONFIG_COEX_TEST_DURATION) {
//			break;
//		}
//	}
//
//	delta = k_uptime_delta(&stamp);
//
//	LOG_INF("Done");
//	LOG_INF("[local] sent %u bytes (%u KB) in %lld ms at %llu kbps",
//	       data, data / 1024, delta, ((uint64_t)data * 8 / delta));
//
//	/* read back char from peer */
//    //Note: use Thread throughput	
//	//err = bt_throughput_read(&throughput);
//	if (err) {
//		LOG_ERR("GATT read failed (err %d)", err);
//		return err;
//	}
//
//	k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT));
//
//	instruction_print();

	return 0;
}


void ot_start_joiner(const char *pskd) 
{
	LOG_INF("Starting joiner");

	otInstance *instance = openthread_get_default_instance();
	struct openthread_context *context = openthread_get_default_context();

	openthread_api_mutex_lock(context);
	
	/** Step1: Set null network key i.e,
	 *  ot networkkey 00000000000000000000000000000000 
	 */
	ot_setNullNetworkKey(instance); /* added new */

	/** Step2: Bring up the interface and start joining to the network
	 *  		on DK2 with pre-shared key. 
	 *   i.e. ot ifconfig up 
	 *        ot joiner start FEDCBA9876543210
	 */
	otIp6SetEnabled(instance, true); /* ot ifconfig up */
	otJoinerStart(instance, pskd, NULL,
				"Zephyr", "Zephyr",
				KERNEL_VERSION_STRING, NULL,
				&ot_joiner_start_handler, NULL);
	openthread_api_mutex_unlock(context);
	//LOG_INF("Thread start joiner Done.");
}

void ot_stop_joiner(void) 
{
	LOG_INF("Stopping joiner");

	otInstance *instance = openthread_get_default_instance();
	struct openthread_context *context = openthread_get_default_context();

	openthread_api_mutex_lock(context);	
	otJoinerStop(instance);
	openthread_api_mutex_unlock(context);
	LOG_INF("Thread stop joiner Done.");
	
	ot_disconnection_success_cnt++;
	k_sem_give(&disconnected_sem);
}

void ot_conn_test_run(void)
{

	int64_t stamp;
	int64_t delta;
	int err = 0;
	/* get cycle stamp */
	stamp = k_uptime_get_32();

	delta = 0;
#if 0 //murali
	while (true) {
		/* start a new connection */
		//if (ot_discon_no_conn != 0) {
		//	ot_discon_no_conn = 0;
			ot_connection_attempt_cnt++;
			
			/* start joining to the network with pre-shared key = FEDCBA9876543210 */
			ot_start_joiner("FEDCBA9876543210");			
		//}

		//Note: with sleep of 2/3 seconds, not observing the issue of one success for 
		// two join attempts. But, observing issue with ksleep of 1sec
		k_sleep(K_SECONDS(2)); /* time sleep between two joiner attempts */


		err = k_sem_take(&connected_sem, WAIT_TIME_FOR_OT_CON);
		
		if ((k_uptime_get_32() - stamp) > CONFIG_COEX_TEST_DURATION) {
			break;
		}
		
		#if 0		
				if (ot_join_success) {
					ot_disconnection_attempt_cnt++;
			
					/** +++++++++++++++  TODO +++++++++++++
					  *remove/stop joiner here to disconnect Thread 
					  */
					ot_stop_joiner();
				}
		
				/** +++++++++++++++  TODO +++++++++++++
				 * currently ot_start_joiner()  is called in this function. this should be called
				 * from ot_stop_joiner()/disconnect() like Thread scan 
				 * is called from the disconnected() callback.
				 */
		
				err = k_sem_take(&disconnected_sem, WAIT_TIME_FOR_OT_DISCON);
				//k_sleep(K_SLEEP_DUR_FOR_OT_CONN); // ksleep already available above. So commenting this.
		#endif
	}
#else
		ot_start_joiner("FEDCBA9876543210");
		k_sleep(K_SECONDS(2));
		err = k_sem_take(&connected_sem, WAIT_TIME_FOR_OT_CON);
#endif

	{
		LOG_INF("Starting openthread.");
		openthread_api_mutex_lock(openthread_get_default_context());
		otError err = otThreadSetEnabled(openthread_get_default_instance(), true); /*  ot thread start */
		if (err != OT_ERROR_NONE) {
			LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
		}
		otDeviceRole current_role = otThreadGetDeviceRole(openthread_get_default_instance());
		openthread_api_mutex_unlock(openthread_get_default_context());
		while (current_role != OT_DEVICE_ROLE_CHILD) {
			LOG_INF("Current role of Thread device: %s", otThreadDeviceRoleToString(current_role));
			k_sleep(K_MSEC(1000));
			openthread_api_mutex_lock(openthread_get_default_context());
			current_role = otThreadGetDeviceRole(openthread_get_default_instance());
			openthread_api_mutex_unlock(openthread_get_default_context());
		}
		zperf_test();
	}
}
//static const struct ot_throughput_cb throughput_cb = {
//	.data_read = ot_throughput_read,
//	.data_received = ot_throughput_received,
//	.data_send = ot_throughput_send
//};

int ot_throughput_test_init(bool is_ot_client)
{
//	int err;
//	int64_t stamp;
//
//	//err = bt_enable(NULL);
//	if (err) {
//		LOG_ERR("thread init failed (err %d)", err);
//		return err;
//	}
//
//	/* LOG_INF("thread initialized"); */
//	scan_init();
//
//	err = bt_throughput_init(&throughput, &throughput_cb);
//	if (err) {
//		LOG_ERR("Throughput service initialization failed.");
//		return err;
//	}
//
//	buttons_init();
//
//	select_role(is_ot_client);
//
//	/**
//	 *LOG_INF("Waiting for connection.");
//	 *ot_disc2conn_start_time = k_uptime_get_32();
//	 *ot_scan2conn_time = 0;
//	 */
//
//	stamp = k_uptime_get_32();
//	while (k_uptime_delta(&stamp) / MSEC_PER_SEC < THROUGHPUT_CONFIG_TIMEOUT) {
//		if (default_conn) {
//			break;
//		}
//		k_sleep(K_SECONDS(1));
//	}
//
//	if (!default_conn) {
//		LOG_INF("Cannot set up connection.");
//		return -ENOTCONN;
//	}
//
//	/**
//	 *ot_scan2conn_time = k_uptime_delta(&ot_disc2conn_start_time);
//	 *LOG_INF("Time taken for scan %lld ms", ot_scan2conn_time);
//	 *ot_disc2conn_start_time = k_uptime_get_32();
//	 *ot_scan2conn_time = 0;
//	 */
//
//	uint32_t conn_cfg_status = connection_configuration_set(
//			BT_LE_CONN_PARAM(CONFIG_BT_INTERVAL_MIN,
//			CONFIG_BT_INTERVAL_MAX,
//			CONFIG_BT_CONN_LATENCY, CONFIG_BT_SUPERVISION_TIMEOUT),
//			BT_CONN_LE_PHY_PARAM_2M,
//			BT_LE_DATA_LEN_PARAM_MAX);
//	/**
//	 *ot_scan2conn_time = k_uptime_delta(&ot_disc2conn_start_time);
//	 *LOG_INF("Time taken for connecion %lld ms", ot_scan2conn_time);
//	 */
//
//	return conn_cfg_status;
	return 0;
}



int ot_connection_init(bool is_ot_client)
{

//
//	int err;
//	int64_t stamp;
//
//	err = bt_enable(NULL);
//	if (err) {
//		LOG_ERR("thread init failed (err %d)", err);
//		return err;
//	}
//
//	/* LOG_INF("thread initialized"); */
//	scan_init();
//
//	/**
//	 *err = bt_throughput_init(&throughput, &throughput_cb);
//	 *if (err) {
//	 *	LOG_ERR("Throughput service initialization failed.");
//	 *	return err;
//	 *}
//	 */
//	buttons_init();
//
//	select_role(is_ot_client);
//
//	/**
//	 *LOG_INF("Waiting for connection.");
//	 *ot_disc2conn_start_time = k_uptime_get_32();
//	 *ot_scan2conn_time = 0;
//	 */
//
//	stamp = k_uptime_get_32();
//	while (k_uptime_delta(&stamp) / MSEC_PER_SEC < SCAN_CONFIG_TIMEOUT) {
//		if (default_conn) {
//			break;
//		}
//		k_sleep(K_SECONDS(1));
//	}
//
//	if (!default_conn) {
//		LOG_INF("Cannot set up connection.");
//		return -ENOTCONN;
//	}
//
//	/**
//	 *ot_scan2conn_time = k_uptime_delta(&ot_disc2conn_start_time);
//	 *LOG_INF("Time taken for scan %lld ms", ot_scan2conn_time);
//	 *ot_disc2conn_start_time = k_uptime_get_32();
//	 *ot_scan2conn_time = 0;
//	 */
//
//	uint32_t conn_cfg_status = connection_configuration_set(
//			BT_LE_CONN_PARAM(CONFIG_BT_INTERVAL_MIN,
//			CONFIG_BT_INTERVAL_MAX,
//			CONFIG_BT_CONN_LATENCY, CONFIG_BT_SUPERVISION_TIMEOUT),
//			BT_CONN_LE_PHY_PARAM_2M,
//			BT_LE_DATA_LEN_PARAM_MAX);
//	/**
//	 *ot_scan2conn_time = k_uptime_delta(&ot_disc2conn_start_time);
//	 *LOG_INF("Time taken for connecion %lld ms", ot_scan2conn_time);
//	 */
//	return conn_cfg_status;
    return 0; // nothing for now.
}


int ot_disconnect_client(void)
{
//	int err;
//
//	if (!default_conn) {
//		/* LOG_INF("Not connected!"); */
//		ot_discon_no_conn_cnt++;
//		ot_discon_no_conn++;
//		return -ENOTCONN;
//	}
//
//	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
//	if (err) {
//		/* LOG_INF("Cannot disconnect!"); */
//		ot_disconnection_fail_cnt++;
//		return err;
//	}
//
//	ot_disconnection_success_cnt++;
	return 0;
}
int ot_tput_test_exit(void)
{
//	int err;
//
//	if (!default_conn) {
//		LOG_INF("Not connected!");
//		return -ENOTCONN;
//	}
//	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
//	if (err) {
//		LOG_INF("Cannot disconnect!");
//		return err;
//	}
	return 0;
}

//#ifdef OT_TX_PWR_CTRL_RSSI
//
//void get_tx_power(uint8_t handle_type, uint16_t handle, int8_t *tx_pwr_lvl)
//{
//	struct bt_hci_cp_vs_read_tx_power_level *cp;
//	struct bt_hci_rp_vs_read_tx_power_level *rp;
//	struct net_buf *buf, *rsp = NULL;
//	int err;
//
//	*tx_pwr_lvl = 0xFF;
//	buf = bt_hci_cmd_create(BT_HCI_OP_VS_READ_TX_POWER_LEVEL,
//				sizeof(*cp));
//	if (!buf) {
//		printk("coex sample, get tx pow,  Unable to allocate command buffer\n");
//		return;
//	}
//
//	cp = net_buf_add(buf, sizeof(*cp));
//	cp->handle = sys_cpu_to_le16(handle);
//	cp->handle_type = handle_type;
//
//	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_TX_POWER_LEVEL,
//				   buf, &rsp);
//	if (err) {
//		uint8_t reason = rsp ?
//			((struct bt_hci_rp_vs_read_tx_power_level *)
//			  rsp->data)->status : 0;
//		printk("coex sample, get tx pow, Read Tx power err: %d reason 0x%02x\n",
//			err, reason);
//		return;
//	}
//
//	rp = (void *)rsp->data;
//	*tx_pwr_lvl = rp->tx_power_level;
//
//	net_buf_unref(rsp);
//}
//
//void read_conn_rssi(uint16_t handle, int8_t *rssi)
//{
//	struct net_buf *buf, *rsp = NULL;
//	struct bt_hci_cp_read_rssi *cp;
//	struct bt_hci_rp_read_rssi *rp;
//
//	int err;
//
//	buf = bt_hci_cmd_create(BT_HCI_OP_READ_RSSI, sizeof(*cp));
//	if (!buf) {
//		printk("coex sample, read conn rssi, Unable to allocate command buffer\n");
//		return;
//	}
//
//	cp = net_buf_add(buf, sizeof(*cp));
//	cp->handle = sys_cpu_to_le16(handle);
//
//	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf, &rsp);
//	if (err) {
//		uint8_t reason = rsp ?
//			((struct bt_hci_rp_read_rssi *)rsp->data)->status : 0;
//		printk("coex sample, read conn rssi, Read RSSI err: %d reason 0x%02x\n",
//			err, reason);
//		return;
//	}
//
//	rp = (void *)rsp->data;
//	*rssi = rp->rssi;
//
//	net_buf_unref(rsp);
//}
//
//
//void set_tx_power(uint8_t handle_type, uint16_t handle, int8_t tx_pwr_lvl)
//{
//	struct bt_hci_cp_vs_write_tx_power_level *cp;
//	struct bt_hci_rp_vs_write_tx_power_level *rp;
//	struct net_buf *buf, *rsp = NULL;
//	int err;
//
//	buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
//				sizeof(*cp));
//	if (!buf) {
//		printk("coex sample, set tx pow, Unable to allocate command buffer\n");
//		return;
//	}
//
//	cp = net_buf_add(buf, sizeof(*cp));
//	cp->handle = sys_cpu_to_le16(handle);
//	cp->handle_type = handle_type;
//	cp->tx_power_level = tx_pwr_lvl;
//
//	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
//				   buf, &rsp);
//	if (err) {
//		uint8_t reason = rsp ?
//			((struct bt_hci_rp_vs_write_tx_power_level *)
//			  rsp->data)->status : 0;
//		printk("coex sample, set tx pow,  Set Tx power err: %d reason 0x%02x\n",
//			err, reason);
//		return;
//	}
//
//	rp = (void *)rsp->data;
//	/* printk("Actual Tx Power: %d\n", rp->selected_tx_power); */
//
//	net_buf_unref(rsp);
//}
//#endif


//BT_CONN_CB_DEFINE(conn_callbacks) = {
//	.connected = connected,
//	.disconnected = disconnected,
//	.le_param_req = le_param_req,
//	.le_param_updated = le_param_updated,
//	.le_phy_updated = le_phy_updated,
//	.le_data_len_updated = le_data_length_updated
//};

void ot_discovery_test_run(void)
{
	/* LOG_INF("In ot_discovery_test_run() function"); */
	
	uint64_t test_start_time;
	int err = 0;
	/* get cycle stamp */
	test_start_time = k_uptime_get_32();
	
	ot_discov_attempt_cnt++;
	/* LOG_INF("calling Thread discover for %d time",ot_discov_attempt_cnt); */
	ot_start_discovery();
	
	while (true) {
		if ((k_uptime_get_32() - test_start_time) > CONFIG_COEX_TEST_DURATION) {
			break;
		}
		k_sleep(K_MSEC(100)); /* in milliseconds. can be reduced to 1ms?? */
	}
}

void ot_setNullNetworkKey(otInstance *aInstance)
{	
    otOperationalDataset aDataset;

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    /* Set network key to null */
    uint8_t key[OT_NETWORK_KEY_SIZE] = {0x00, 0x00, 0x00, 0x00,
										0x00, 0x00, 0x00, 0x00,
										0x00, 0x00, 0x00, 0x00,
										0x00, 0x00, 0x00, 0x00};
    memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
    aDataset.mComponents.mIsNetworkKeyPresent = true;
	
    otDatasetSetActive(aInstance, &aDataset);
}

static void ot_setNetworkConfiguration(otInstance *aInstance)
{
	static char          aNetworkName[] = "TestNetwork";
    otOperationalDataset aDataset;

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    /*
     * Fields that can be configured in otOperationDataset to override defaults:
     *     Network Name, Mesh Local Prefix, Extended PAN ID, PAN ID, Delay Timer,
     *     Channel, Channel Mask Page 0, Network Key, PSKc, Security Policy
     */
    aDataset.mActiveTimestamp.mSeconds             = 1;
    aDataset.mActiveTimestamp.mTicks               = 0;
    aDataset.mActiveTimestamp.mAuthoritative       = false;
    aDataset.mComponents.mIsActiveTimestampPresent = true;

    /* Set Channel */
    aDataset.mChannel                      = CONFIG_OT_CHANNEL;
    aDataset.mComponents.mIsChannelPresent = true;

    /* Set Pan ID */
    aDataset.mPanId                      = (otPanId)CONFIG_OT_PAN_ID;
    aDataset.mComponents.mIsPanIdPresent = true;

    /* Set Extended Pan ID */
    /* uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0xC0, 0xDE, 0x1A, 0xB5,
												0xC0, 0xDE, 0x1A, 0xB5}; */
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x11, 0x11, 0x11, 0x11,
											0x11, 0x11, 0x11, 0x11};
    memcpy(aDataset.mExtendedPanId.m8, extPanId, sizeof(aDataset.mExtendedPanId));
    aDataset.mComponents.mIsExtendedPanIdPresent = true;

    /* Set network key */
    /* uint8_t key[OT_NETWORK_KEY_SIZE] = 
				{0x12, 0x34, 0xC0, 0xDE, 0x1A, 0xB5, 0x12, 0x34,
				0xC0, 0xDE, 0x1A, 0xB5, 0x12, 0x34, 0xC0, 0xDE}; */
    uint8_t key[OT_NETWORK_KEY_SIZE] =
				{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
				0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
    aDataset.mComponents.mIsNetworkKeyPresent = true;

    /* Set Network Name */
    size_t length = strlen(aNetworkName);
    assert(length <= OT_NETWORK_NAME_MAX_SIZE);
    memcpy(aDataset.mNetworkName.m8, aNetworkName, length);
    aDataset.mComponents.mIsNetworkNamePresent = true;

    otDatasetSetActive(aInstance, &aDataset);
}


void ot_handle_active_discov_result(struct otActiveScanResult *result, void *context)
{
	if (!result) {
		ot_discov_no_result_cnt++;
	} else {
		/* LOG_INF("panid: %04x channel: %2u rssi: %3d",
					result->mPanId, result->mChannel, result->mRssi); */
		ot_rssi = result->mRssi;
		ot_discov_success_cnt++;
	}
	
	/* LOG_INF("repeat_ot_discovery: %3d",repeat_ot_discovery); */
	if (repeat_ot_discovery == 1) {
		ot_discov_attempt_cnt++;
		/* LOG_INF("calling Thread discover for %d time",ot_discov_attempt_cnt); */
		ot_start_discovery();
	}
	k_sleep(K_MSEC(30));	
}

int ot_initialization(void)
{
	struct openthread_context *context = openthread_get_default_context();
	otInstance *instance = openthread_get_default_instance();
	LOG_INF("Updating thread parameters");
	ot_setNetworkConfiguration(instance);
	LOG_INF("Enabling thread");
	
	/* otIp6SetEnabled(instance, true); */ /* cli `ifconfig up` */
	/* otThreadSetEnabled(instance, true); */ /* cli `thread start` */	
	otError err = openthread_start(context);	// 'ifconfig up && thread start'
	if (err != OT_ERROR_NONE) {
		LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
	}

	otDeviceRole current_role = otThreadGetDeviceRole(instance);
	LOG_INF("Current role of Thread device: %s", otThreadDeviceRoleToString(current_role));

	return 0;
}

void ot_start_discovery(void) {
	struct openthread_context *context = openthread_get_default_context();
	otInstance *instance = openthread_get_default_instance();
	
	/* LOG_INF("Performing Thread discover"); */
	openthread_api_mutex_lock(context);
	otThreadDiscover(openthread_get_default_instance(), 0 /* all channels */,
		OT_PANID_BROADCAST, false, false, ot_handle_active_discov_result, NULL);
	openthread_api_mutex_unlock(openthread_get_default_context());
}
const char* ot_check_device_state()
{
	otDeviceRole current_role = otThreadGetDeviceRole(openthread_get_default_instance());
	/* LOG_INF("Current state of thread device: %s",
				otThreadDeviceRoleToString(current_role)); */
	return(otThreadDeviceRoleToString(current_role));
}


int ot_device_disable(void)
{
	openthread_api_mutex_lock(openthread_get_default_context());
	otThreadSetEnabled(openthread_get_default_instance(), false);
	otIp6SetEnabled(openthread_get_default_instance(), false);
	openthread_api_mutex_unlock(openthread_get_default_context());
	return 0;
}

#define ZPERF_DEFAULT_PORT 5001U
typedef struct peer_address_info {
	char address_string[OT_IP6_ADDRESS_STRING_SIZE];
	bool address_found;
} peer_address_info_t;

peer_address_info_t peer_address_info  = {.address_found = false};

void handle_ping_reply(const otPingSenderReply *reply, void *context) {
	otIp6Address add = reply->mSenderAddress;
	char string[OT_IP6_ADDRESS_STRING_SIZE];
	otIp6AddressToString(&add, string, OT_IP6_ADDRESS_STRING_SIZE);
	LOG_WRN("Reply received from: %s\n", string);
	if (!peer_address_info.address_found) {
		strcpy(peer_address_info.address_string, string);
		peer_address_info.address_found = true;
	}
}

void get_peer_address(uint64_t timeout_ms) {
	LOG_INF("Finding other devices...");
    otPingSenderConfig config;
    memset(&config, 0, sizeof(config));
	config.mReplyCallback = handle_ping_reply;

	const char *dest = "ff03::1";	// Mesh-Local anycast for all FTDs and MEDs
	openthread_api_mutex_lock(openthread_get_default_context());
	otIp6AddressFromString(dest, &config.mDestination);
	otPingSenderPing(openthread_get_default_instance(), &config);
	openthread_api_mutex_unlock(openthread_get_default_context());
	uint64_t start_time = k_uptime_get();
	while (!peer_address_info.address_found && k_uptime_get() < start_time + timeout_ms) {
		k_sleep(K_MSEC(100));
	}
}

void start_zperf_test_send(const char *peer_addr, uint32_t duration_sec, uint32_t packet_size_bytes, uint32_t rate_bps) {
    const struct shell *shell = shell_backend_uart_get_ptr();
    char cmd[128];

    snprintf(cmd, sizeof(cmd), "zperf udp upload %s %u %u %u %u", peer_addr, ZPERF_DEFAULT_PORT, duration_sec, packet_size_bytes, rate_bps);
    shell_execute_cmd(shell, cmd);
}

void start_zperf_test_recv() {
    const struct shell *shell = shell_backend_uart_get_ptr();
    shell_execute_cmd(shell, "zperf udp download");
}

void zperf_test() {
	get_peer_address(5000);
	if (!peer_address_info.address_found) {
		LOG_WRN("Peer address not found. Not continuing with zperf test.");
		return;
	}
	start_zperf_test_send(peer_address_info.address_string, 5, 256,1e4);
}
