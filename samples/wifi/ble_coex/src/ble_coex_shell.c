/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ble_coex, CONFIG_LOG_DEFAULT_LEVEL);

#if NRFX_CLOCK_ENABLED && (defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M)
#include <nrfx_clock.h>
#endif
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


static struct sockaddr_in in4_addr_my = {
	.sin_family = AF_INET,
	.sin_port = htons(5001),
};
K_SEM_DEFINE(udp_callback, 0, 1);

//--------------------------------------
 
#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/throughput.h>
#include <bluetooth/scan.h>
#include <bluetooth/gatt_dm.h>

#include <zephyr/shell/shell_uart.h>

#include <dk_buttons_and_leds.h>


#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define THROUGHPUT_CONFIG_TIMEOUT K_SECONDS(20)
#define THROUGHPUT_CONFIG_TIMEOUT_SEC 20

static K_SEM_DEFINE(throughput_sem, 0, 1);

static volatile bool data_length_req;
static volatile bool test_ready;
static struct bt_conn *default_conn;
static struct bt_throughput throughput;
static const struct bt_uuid *uuid128 = BT_UUID_THROUGHPUT;
static struct bt_gatt_exchange_params exchange_params;
static struct bt_le_conn_param *conn_param =
	BT_LE_CONN_PARAM(CONFIG_INTERVAL_MIN,
		CONFIG_INTERVAL_MAX,
		CONFIG_CONN_LATENCY,
		CONFIG_SUPERVISION_TIMEOUT);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		0xBB, 0x4A, 0xFF, 0x4F, 0xAD, 0x03, 0x41, 0x5D,
		0xA9, 0x6C, 0x9D, 0x6C, 0xDD, 0xDA, 0x83, 0x04),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void button_handler_cb(uint32_t button_state, uint32_t has_changed);

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

static void instruction_print(void)
{
	printk("You can use the Tab key to autocomplete commands.\n");
}

void scan_filter_match(struct bt_scan_device_info *device_info,
		       struct bt_scan_filter_match *filter_match,
		       bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %d\n",
		addr, connectable);
}

void scan_filter_no_match(struct bt_scan_device_info *device_info,
			  bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filter not match. Address: %s connectable: %d\n",
				addr, connectable);
}

void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match,
		scan_connecting_error, NULL);

static void exchange_func(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_exchange_params *params)
{
	struct bt_conn_info info = {0};
	int err;

	printk("MTU exchange %s \n", att_err == 0 ? "successful" : "failed");

	err = bt_conn_get_info(conn, &info);
	if (err) {
		printk("Failed to get connection info %d \n", err);
		return;
	}

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		instruction_print();
		test_ready = true;
	}
}

static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	int err;
	struct bt_throughput *throughput = context;

	printk("Service discovery completed \n");

	bt_gatt_dm_data_print(dm);
	bt_throughput_handles_assign(dm, throughput);
	bt_gatt_dm_data_release(dm);

	exchange_params.func = exchange_func;

	err = bt_gatt_exchange_mtu(default_conn, &exchange_params);
	if (err) {
		printk("MTU exchange failed (err %d) \n", err);
	} else {
		printk("MTU exchange pending \n");
	}
}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	printk("Service not found \n");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	printk("Error while discovering GATT database: (%d) \n", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void connected(struct bt_conn *conn, uint8_t hci_err)
{
	struct bt_conn_info info = {0};
	int err;

	if (hci_err) {
		if (hci_err == BT_HCI_ERR_UNKNOWN_CONN_ID) {
			/* Canceled creating connection */
			return;
		}

		printk("Connection failed, err 0x%02x %s \n", hci_err, bt_hci_err_to_str(hci_err));
		return;
	}

	if (default_conn) {
		printk("Connection exists, disconnect second connection \n");
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	default_conn = bt_conn_ref(conn);

	err = bt_conn_get_info(default_conn, &info);
	if (err) {
		printk("Failed to get connection info %d \n", err);
		return;
	}

	printk("Connected as %s \n",
	       info.role == BT_CONN_ROLE_CENTRAL ? "central" : "peripheral");
	printk("Conn. interval is %u units \n", info.le.interval);

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		err = bt_gatt_dm_start(default_conn,
					   BT_UUID_THROUGHPUT,
					   &discovery_cb,
					   &throughput);

		if (err) {
			printk("Discover failed (err %d) \n", err);
		}
	}
}


static void scan_init(void)
{
	int err;
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = 0x0010,
		.window = 0x0010,
	};

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
		.scan_param = &scan_param,
		.conn_param = conn_param
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, uuid128);
	if (err) {
		printk("Scanning filters cannot be set \n");

		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on \n");
	}
}

static void scan_start(void)
{
	int err;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		printk("Starting scanning failed (err %d) \n", err);
		return;
	}
}

static void adv_start(void)
{
	const struct bt_le_adv_param *adv_param =
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE |
				BT_LE_ADV_OPT_ONE_TIME,
				BT_GAP_ADV_FAST_INT_MIN_2,
				BT_GAP_ADV_FAST_INT_MAX_2,
				NULL);
	int err;

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		printk("Failed to start advertiser (%d) \n", err);
		return;
	}
}


static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s \n", reason, bt_hci_err_to_str(reason));

	test_ready = false;
	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}




static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	printk("Connection parameters update request received. \n");
	printk("Minimum interval: %d, Maximum interval: %d \n",
	       param->interval_min, param->interval_max);
	printk("Latency: %d, Timeout: %d \n", param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("Connection parameters updated. \n"
	       " interval: %d, latency: %d, timeout: %d",
	       interval, latency, timeout);

	k_sem_give(&throughput_sem);
}

static void le_phy_updated(struct bt_conn *conn,
			   struct bt_conn_le_phy_info *param)
{
	printk("LE PHY updated: TX PHY %s, RX PHY %s \n",
	       phy2str(param->tx_phy), phy2str(param->rx_phy));

	k_sem_give(&throughput_sem);
}

static void le_data_length_updated(struct bt_conn *conn,
				   struct bt_conn_le_data_len_info *info)
{
	if (!data_length_req) {
		return;
	}

	printk("LE data len updated: TX (len: %d time: %d)"
	       " RX (len: %d time: %d) \n", info->tx_max_len,
	       info->tx_max_time, info->rx_max_len, info->rx_max_time);

	data_length_req = false;
	k_sem_give(&throughput_sem);
}

static uint8_t throughput_read(const struct bt_throughput_metrics *met)
{
	printk("[peer] received %u bytes (%u KB)"
	       " in %u GATT writes at %u bps \n",
	       met->write_len, met->write_len / 1024, met->write_count,
	       met->write_rate);

	k_sem_give(&throughput_sem);

	return BT_GATT_ITER_STOP;
}

static void throughput_received(const struct bt_throughput_metrics *met)
{
	static uint32_t kb;

	if (met->write_len == 0) {
		kb = 0;
		printk("\n");

		return;
	}
	/* printk("\nThroughput in progress. \n"); */
	if ((met->write_len / 1024) != kb) {
		kb = (met->write_len / 1024);
		printk("=");
	}
}

static void throughput_send(const struct bt_throughput_metrics *met)
{
	printk("\n[local] received %u bytes (%u KB)"
		" in %u GATT writes at %u bps",
		met->write_len, met->write_len / 1024,
		met->write_count, met->write_rate);
}

static const struct bt_throughput_cb throughput_cb = {
	.data_read = throughput_read,
	.data_received = throughput_received,
	.data_send = throughput_send
};

static struct button_handler button = {
	.cb = button_handler_cb,
};

void select_role(bool is_central)
{
	int err;
	static bool role_selected;

	if (role_selected) {
		printk("\nCannot change role after it was selected once. \n");
		return;
	} else if (is_central) {
		printk("\nCentral. Starting scanning \n");
		scan_start();
	} else {
		printk("\nPeripheral. Starting advertising \n");
		adv_start();
	}

	role_selected = true;

	/* The role has been selected, button are not needed any more. */
	err = dk_button_handler_remove(&button);
	if (err) {
		printk("Button disable error: %d \n", err);
	}
}

static void button_handler_cb(uint32_t button_state, uint32_t has_changed)
{
	ARG_UNUSED(has_changed);

	if (button_state & DK_BTN1_MSK) {
		select_role(true);
	} else if (button_state & DK_BTN2_MSK) {
		select_role(false);
	}
}

static void buttons_init(void)
{
	int err;

	err = dk_buttons_init(NULL);
	if (err) {
		printk("Buttons initialization failed.\n");
		return;
	}

	/* Add dynamic buttons handler. Buttons should be activated only when
	 * during the board role choosing.
	 */
	dk_button_handler_add(&button);
}

int connection_configuration_set(const struct bt_le_conn_param *conn_param,
			const struct bt_conn_le_phy_param *phy,
			const struct bt_conn_le_data_len_param *data_len)
{
	int err;
	struct bt_conn_info info = {0};

	err = bt_conn_get_info(default_conn, &info);
	if (err) {
		printk("Failed to get connection info %d \n", err);
		return err;
	}

	if (info.role != BT_CONN_ROLE_CENTRAL) {
		printk("'run' command shall be executed only on the central board \n");
	}

	err = bt_conn_le_phy_update(default_conn, phy);
	if (err) {
		printk("PHY update failed: %d \n", err);
		return err;
	}

	printk("PHY update pending \n");
	err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC));
	if (err) {
		printk("PHY update timeout \n");
		return err;
	}

	if (info.le.data_len->tx_max_len != data_len->tx_max_len) {
		data_length_req = true;

		err = bt_conn_le_data_len_update(default_conn, data_len);
		if (err) {
			printk("LE data length update failed: %d \n",
					err);
			return err;
		}

		printk("LE Data length update pending \n");
		err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC));
		if (err) {
			printk("LE Data Length update timeout \n");
			return err;
		}
	}

	if (info.le.interval != conn_param->interval_max) {
		err = bt_conn_le_param_update(default_conn, conn_param);
		if (err) {
			printk("Connection parameters update failed: %d \n",
					err);
			return err;
		}

		printk("Connection parameters update pending \n");
		err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC));
		if (err) {
			printk("Connection parameters update timeout \n");
			return err;
		}
	}

	return 0;
}


static int bt_throughput_test_run(const struct shell *shell,
	     const struct bt_le_conn_param *conn_param,
	     const struct bt_conn_le_phy_param *phy,
	     const struct bt_conn_le_data_len_param *data_len,
		 uint32_t ble_test_duration)
{
	int err;
	uint64_t stamp;
	int64_t delta;
	uint32_t data = 0;

	/* a dummy data buffer */
	static char dummy[495];

	if (!default_conn) {
		shell_error(shell, "Device is disconnected %s",
			    "Connect to the peer device before running test");
		return -EFAULT;
	}

	if (!test_ready) {
		shell_error(shell, "Device is not ready."
			"Please wait for the service discovery and MTU exchange end");
		return 0;
	}

	shell_print(shell, "\nStarting Bluetooth LE throughput test \n");

	
	err = connection_configuration_set(conn_param, phy, data_len);
	if (err) {
		return err;
	}

	shell_print(shell, "Bluetooth LE throughput test is in progress ");
	shell_print(shell, "and requires around %d seconds to complete.\n", ble_test_duration);

	/* Make sure that all BLE procedures are finished. */
	k_sleep(K_MSEC(500));


	/* reset peer metrics */
	err = bt_throughput_write(&throughput, dummy, 1);
	if (err) {
		shell_error(shell, "Reset peer metrics failed.");
		return err;
	}

	/* get cycle stamp */
	stamp = k_uptime_get_32();

	delta = 0;
	while (true) {
		err = bt_throughput_write(&throughput, dummy, 495);
		if (err) {
			shell_error(shell, "GATT write failed (err %d)", err);
			break;
		}
		data += 495;
		if (k_uptime_get_32() - stamp > (ble_test_duration*1000) ) {
			break;
		}
	}

	delta = k_uptime_delta(&stamp);

	printk("\nDone \n");
	printk("[local] sent %u bytes (%u KB) in %lld ms at %llu kbps",
	       data, data / 1024, delta, ((uint64_t)data * 8 / delta));

	/* read back char from peer */
	err = bt_throughput_read(&throughput);
	if (err) {
		shell_error(shell, "GATT read failed (err %d)", err);
		return err;
	}

	k_sem_take(&throughput_sem, THROUGHPUT_CONFIG_TIMEOUT);

	instruction_print();

	return 0;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
	.le_phy_updated = le_phy_updated,
	.le_data_len_updated = le_data_length_updated,
	#ifdef ADDED_FROM_BLE_TPUT_SAMPLE
	.security_changed = security_changed
	#endif
};

static int bt_throughput_test_init(const struct shell *shell, size_t argc,
			char *argv[])
{
	int err;
	int64_t stamp;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d) \n", err);
		return err;
	}

	printk("Bluetooth initialized \n");

	scan_init();

	err = bt_throughput_init(&throughput, &throughput_cb);
	if (err) {
		printk("Throughput service initialization failed. \n");
		return err;
	}

	buttons_init();
	
	if((bool)strtoul(argv[1], NULL, 0)){		
		printk("Bluetooth LE role is central \n");
	} else {
		printk("Bluetooth LE role is peripheral \n");
	}
		
	select_role((bool)strtoul(argv[1], NULL, 0));

	printk("BLE connection interval MIN = %d \n",CONFIG_INTERVAL_MIN);
	printk("BLE connection interval MAX = %d \n",CONFIG_INTERVAL_MAX);
	printk("BLE connection latency = %d \n",CONFIG_CONN_LATENCY);
	printk("BLE supervision timeout = %d \n",CONFIG_SUPERVISION_TIMEOUT);

	printk("Waiting for connection. \n");

	stamp = k_uptime_get_32();
	while (k_uptime_delta(&stamp) / MSEC_PER_SEC < THROUGHPUT_CONFIG_TIMEOUT_SEC) {
		if (default_conn) {
			break;
		}
		k_sleep(K_SECONDS(1));
	}

	if (!default_conn) {
		printk("Cannot set up connection. \n");
		return -ENOTCONN;
	}
	return connection_configuration_set(
			BT_LE_CONN_PARAM(CONFIG_INTERVAL_MIN,
			CONFIG_INTERVAL_MAX,
			CONFIG_CONN_LATENCY, CONFIG_SUPERVISION_TIMEOUT),
			BT_CONN_LE_PHY_PARAM_2M,
			BT_LE_DATA_LEN_PARAM_MAX);
}

static int bt_disconnection(const struct shell *shell, size_t argc,
			char **argv)
{
	int err;

	if (!default_conn) {
		printk("Not connected! \n");
		return -ENOTCONN;
	}

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	if (err) {
		shell_fprintf(shell, SHELL_ERROR, "Bluetooth LE disconnection - FAIL \n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Bluetooth LE disconnection - SUCCESS \n");
	}
	
	return err;
}

static struct test_params {
	struct bt_le_conn_param *conn_param;
	struct bt_conn_le_phy_param *phy;
	struct bt_conn_le_data_len_param *data_len;
} test_params = {
	.conn_param = BT_LE_CONN_PARAM(CONFIG_INTERVAL_MIN, CONFIG_INTERVAL_MAX, CONFIG_CONN_LATENCY,
				       CONFIG_SUPERVISION_TIMEOUT),
	.phy = BT_CONN_LE_PHY_PARAM_2M,
	.data_len = BT_LE_DATA_LEN_PARAM_MAX
};

static int bt_run_throughput(const struct shell *shell, size_t argc,
			char **argv)
{
	uint32_t status;
	uint32_t ble_test_duration;
	
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "invalid # of args : %d \n", argc);
		shell_fprintf(shell, SHELL_ERROR, "Usage: bt_run_tput ble_test_duration \n\n");
		shell_fprintf(shell, SHELL_ERROR, "       ble_test_duration: Test duration in seconds \n");
		return -ENOEXEC;
	}

	ble_test_duration = strtoul(argv[1], NULL, 0);
	
	status = bt_throughput_test_run(shell, test_params.conn_param, test_params.phy,
			test_params.data_len, ble_test_duration);

	if (status) {
		shell_fprintf(shell, SHELL_ERROR, "Bluetooth LE disconnection - FAIL \n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Bluetooth LE disconnection - SUCCESS \n");
	}
	
	return status;
}


																									
static int bt_configure_throughput(const struct shell *shell, size_t argc,
			char *argv[])
{
	
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "invalid # of args : %d \n", argc);
		shell_fprintf(shell, SHELL_ERROR, "Usage: bt_cfg_tput bt_role \n\n");
		printk("       bt_role: 1 for central, 0 for peripheral \n");
		return -ENOEXEC;
	}

	/* BLE connection */
	int ret = bt_throughput_test_init(shell, argc, argv);

	if (ret != 0) {
		shell_fprintf(shell, SHELL_ERROR, "Bluetooth LE throughput configuration - FAIL \n");
		return ret;
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Bluetooth LE throughput configuration - SUCCESS \n");
		return 0;
	}
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
			shell_fprintf(shell, SHELL_ERROR, "invalid # of args : %d \n", argc);
			shell_fprintf(shell, SHELL_ERROR, "Usage: coex_config_pta wifi_band is_sep_antennas ");
			shell_fprintf(shell, SHELL_ERROR, "is_sr_ble \n\n");
			printk("       wlan_band: 0 for 2.4GHz, 1 for 5GHz \n");
			printk("       is_sep_antennas: 0 for shared antenna, 1 for separate antennas \n");
			printk("       is_sr_ble: 0 for Thread, 1 for Bluetooth protocol \n");
			return -ENOEXEC;
		}

		wlan_band  = strtoul(argv[1], NULL, 0);
		separate_antennas  = strtoul(argv[2], NULL, 0);
		is_sr_protocol_ble  = strtoul(argv[3], NULL, 0);
		
		if (wlan_band) {
			printk("WLAN operating band: 5GHz \n");
		} else {
			printk("WLAN operating band: 2.4GHz \n");
		}
		if (separate_antennas) {
			printk("Antenna mode: Wi-Fi and SR uses separate antennas \n");
		} else {
			printk("Antenna mode: Wi-Fi and SR shares antenna \n");
		}
		if (is_sr_protocol_ble) {
			printk("SR protocol: Bluetooth LE \n");
		} else {
			printk("SR protocol: Thread \n");
		}

		result_non_pta = nrf_wifi_coex_config_non_pta(separate_antennas, is_sr_protocol_ble);
		result_pta = nrf_wifi_coex_config_pta(wlan_band, separate_antennas, is_sr_protocol_ble);
		result = result_non_pta & result_pta;
		
		/**
		 * printk("\nresult_non_pta = %d \n", result_non_pta);		 
		 * printk("result_pta = %d \n", result_pta);		 
		 * printk("result = %d \n", result);
		 */
		
		if (result) {
			shell_fprintf(shell, SHELL_ERROR, "Configuration of PTA - FAIL \n");
		} else {
			shell_fprintf(shell, SHELL_ERROR, "Configuration of PTA - SUCCESS \n");
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
		shell_fprintf(shell, SHELL_ERROR, "COEXC disable - FAIL \n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "COEXC disable - SUCCESS \n");
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
		shell_fprintf(shell, SHELL_ERROR, "invalid # of args : %d \n", argc);
		shell_fprintf(shell, SHELL_ERROR, "Usage: config_sr_switch is_sep_antennas \n\n");
		printk("       is_sep_antennas: 0 for shared antenna, 1 for separate antennas \n");
		return -ENOEXEC;
	}

	
	separate_antennas  = strtoul(argv[1], NULL, 0);
	if (separate_antennas) {
		printk("Antenna mode: Wi-Fi and SR uses separate antennas \n");
	} else {
		printk("Antenna mode: Wi-Fi and SR shares antenna \n");
	}

	ret = nrf_wifi_config_sr_switch(separate_antennas);
	if (ret != 0) {
		shell_fprintf(shell, SHELL_ERROR, "Configuration of SR side switch - FAIL \n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Configuration of SR side switch - SUCCESS \n");
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
		printk("New UDP session started \n");
		break;
	case ZPERF_SESSION_PERIODIC_RESULT:
		/* Ignored. */
		break;
	case ZPERF_SESSION_FINISHED:
		printk("Wi-Fi benchmark: Upload completed! \n");
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
		printk("Upload results: \n");
		printk("%u bytes in %llu ms \n",
				(result->nb_packets_sent * result->packet_size),
				(result->client_time_in_us / USEC_PER_MSEC));
		printk("%u packets sent \n", result->nb_packets_sent);
		printk("%u packets lost \n", result->nb_packets_lost);
		printk("%u packets received \n", result->nb_packets_rcvd);
		k_sem_give(&udp_callback);
		break;
	case ZPERF_SESSION_ERROR:
		printk("UDP session error \n");
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
		printk("Invalid IPv4 address %s \n", host);
		return -EINVAL;
	}

	printk("IPv4 address %s \n", host);

	return 0;
}


static int wifi_run_throughput(const struct shell *shell, size_t argc,
			char **argv)
{
	struct zperf_upload_params params;
	int ret = 0;

	if (argc < 8) {
		shell_fprintf(shell, SHELL_ERROR, "invalid # of args : %d \n", argc);
		shell_fprintf(shell, SHELL_ERROR, "Usage: wifi_run_tput protocol direction peerIP ");
		shell_fprintf(shell, SHELL_ERROR, "destPort duration pktSize baudrate \n\n");
		printk("       protocol - udp or tcp \n");
		printk("       direction - upload or download \n");
		printk("       peerIP - IP of the peer device \n");
		printk("       destPort - port of the peer device \n");
		printk("       duration - test duration in seconds \n");
		printk("       pktSize - in bytes or kilobyte (with suffix K) \n");
		printk("       baudrate - baudrate in kilobyte/megabyte (with suffix K/M) \n");
		return -ENOEXEC;
	}
	
	uint32_t port  = strtoul(argv[4], NULL, 0);
	uint32_t duration_ms  = strtoul(argv[5], NULL, 0);
	uint32_t packet_size  = strtoul(argv[6], NULL, 0);
	uint32_t rate_kbps  = strtoul(argv[7], NULL, 0);
	
	
	parse_ipv4_addr(argv[3], &in4_addr_my);
	params.duration_ms = duration_ms * 1000; /* sec to ms */
	params.rate_kbps = rate_kbps*1000;
	params.packet_size = packet_size * 1024;
	
	net_sprint_ipv4_addr(&in4_addr_my.sin_addr);
	in4_addr_my.sin_port = htons(port);

	memcpy(&params.peer_addr, &in4_addr_my, sizeof(in4_addr_my));
	
	if (strcmp(argv[1], "udp") == 0 && strcmp(argv[2], "upload") == 0) {		
		printk("Starting Wi-Fi benchmark: Zperf udp client \n");

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
		shell_fprintf(shell, SHELL_ERROR, "Running Wi-Fi throughput - FAIL \n");
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
SHELL_CMD_REGISTER(bt_cfg_tput, NULL, "Run BT config for throughput", bt_configure_throughput);
SHELL_CMD_REGISTER(bt_run_tput, NULL, "Run BT throughput", bt_run_throughput);
SHELL_CMD_REGISTER(bt_disconnect, NULL, "Run BT disconnect", bt_disconnection);
SHELL_CMD_REGISTER(wifi_run_tput, NULL, "Run Wi-Fi throughput", wifi_run_throughput);
