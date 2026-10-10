/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ble_coex_shell, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/net/zperf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>

#include <net/wifi_mgmt_ext.h>

#ifdef CONFIG_NRF70_SR_COEX
#include "coex.h"
#endif
#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/throughput.h>
#include <bluetooth/scan.h>
#include <bluetooth/gatt_dm.h>

#include <zephyr/shell/shell_uart.h>

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define THROUGHPUT_CONFIG_TIMEOUT_SEC 20

/* Payload of one GATT write: the 498-byte L2CAP MTU minus the 3-byte ATT header. */
#define BT_TPUT_WRITE_LEN 495

/* Only used to set up the connection. CONFIG_INTERVAL_* is applied afterwards by
 * connection_configuration_set(). Establishing at a long interval is unreliable.
 */
#define SETUP_INTERVAL_MIN 6		/* 6 units, 7.5 ms */
#define SETUP_INTERVAL_MAX 6		/* 6 units, 7.5 ms */
#define SETUP_SUPERVISION_TIMEOUT 400	/* 400 units, 4 s */

/* Requesting frame space of 0 us will result in the minimum frame space being used. */
#define REQUEST_MIN_FRAME_SPACE_US 0

/* Largest UDP payload that still fits a 1500 byte MTU after the IPv4 and UDP headers. */
#define WIFI_TPUT_MAX_UNFRAGMENTED_PKT 1472

/* Comfortably above the iperf datagram and client headers that zperf prepends. */
#define WIFI_TPUT_MIN_PKT 64

static K_SEM_DEFINE(throughput_sem, 0, 1);

static volatile bool data_length_req;
static volatile bool test_ready;
static volatile bool connect_pending;
static bool role_central;
static bool adv_active;
static uint8_t bt_role;
static struct bt_conn *default_conn;
static struct bt_throughput throughput;
static const struct bt_uuid *uuid128 = BT_UUID_THROUGHPUT;
static struct bt_gatt_exchange_params exchange_params;

static struct bt_le_conn_param *setup_conn_param =
	BT_LE_CONN_PARAM(SETUP_INTERVAL_MIN, SETUP_INTERVAL_MAX, CONFIG_CONN_LATENCY,
			 SETUP_SUPERVISION_TIMEOUT);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		0xBB, 0x4A, 0xFF, 0x4F, 0xAD, 0x03, 0x41, 0x5D,
		0xA9, 0x6C, 0x9D, 0x6C, 0xDD, 0xDA, 0x83, 0x04),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct test_params {
	struct bt_le_conn_param *conn_param;
	struct bt_conn_le_phy_param *phy;
	struct bt_conn_le_data_len_param *data_len;
	uint16_t frame_space_us;
} test_params = {
	.conn_param = BT_LE_CONN_PARAM(CONFIG_INTERVAL_MIN, CONFIG_INTERVAL_MAX,
				       CONFIG_CONN_LATENCY, CONFIG_SUPERVISION_TIMEOUT),
	.phy = BT_CONN_LE_PHY_PARAM_2M,
	.data_len = BT_LE_DATA_LEN_PARAM_MAX,
	.frame_space_us = REQUEST_MIN_FRAME_SPACE_US,
};

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
	LOG_INF("You can use the Tab key to autocomplete your input.");
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d", addr, connectable);
}

static void scan_filter_no_match(struct bt_scan_device_info *device_info,
				 bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filter not match. Address: %s connectable: %d", addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_ERR("Connecting failed");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match,
		scan_connecting_error, NULL);

static void exchange_func(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_exchange_params *params)
{
	struct bt_conn_info info = {0};
	int err;

	if (att_err) {
		LOG_ERR("MTU exchange failed (ATT err 0x%02x)", att_err);
		return;
	}

	LOG_INF("MTU exchange successful");

	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("Failed to get connection info %d", err);
		return;
	}

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		instruction_print();
		test_ready = true;
	}
}

static void discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	struct bt_throughput *tput = context;
	int err;

	LOG_INF("Service discovery completed");

	bt_gatt_dm_data_print(dm);
	bt_throughput_handles_assign(dm, tput);

	exchange_params.func = exchange_func;

	err = bt_gatt_exchange_mtu(bt_gatt_dm_conn_get(dm), &exchange_params);
	if (err) {
		LOG_ERR("MTU exchange failed (err %d)", err);
	} else {
		LOG_INF("MTU exchange pending");
	}

	bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	LOG_ERR("Service not found");
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
	LOG_ERR("Error while discovering GATT database: (%d)", err);
}

static struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void scan_start(void);
static void adv_start(void);

/* Restart scanning or advertising in the selected role while bt_cfg_tput is
 * still waiting for a connection.
 */
static void connect_retry(void)
{
	if (!connect_pending) {
		return;
	}

	if (role_central) {
		scan_start();
	} else {
		adv_start();
	}
}

static void connected(struct bt_conn *conn, uint8_t hci_err)
{
	struct bt_conn_info info = {0};
	int err;

	if (hci_err) {
		if (hci_err != BT_HCI_ERR_UNKNOWN_CONN_ID) {
			LOG_ERR("Connection failed, err 0x%02x %s", hci_err,
				bt_hci_err_to_str(hci_err));
		}

		/* UNKNOWN_CONN_ID means connection creation was canceled, for example
		 * because the peer did not respond in time. Scanning has stopped
		 * either way, so restart it if bt_cfg_tput is still waiting.
		 */
		connect_retry();
		return;
	}

	if (default_conn) {
		LOG_INF("Connection exists, disconnect second connection");
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	default_conn = bt_conn_ref(conn);

	err = bt_conn_get_info(default_conn, &info);
	if (err) {
		LOG_ERR("Failed to get connection info %d", err);
		return;
	}

	bt_role = info.role;

	LOG_INF("Connected as %s", info.role == BT_CONN_ROLE_CENTRAL ? "central" : "peripheral");
	LOG_INF("Conn. interval is %u us", info.le.interval_us);

	if (info.role == BT_CONN_ROLE_PERIPHERAL) {
		/* The controller stops advertising when the connection is made. */
		adv_active = false;

		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (err) {
			LOG_ERR("Failed to set security: %d", err);
		}
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err security_err)
{
	struct bt_conn_info info = {0};
	int err;

	LOG_INF("Security changed: level %d, err: %d %s", level, security_err,
		bt_security_err_to_str(security_err));

	if (security_err != 0) {
		LOG_ERR("Failed to encrypt link");
		bt_conn_disconnect(conn, BT_HCI_ERR_PAIRING_NOT_SUPPORTED);
		return;
	}

	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("Failed to get connection info %d", err);
		return;
	}

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		err = bt_gatt_dm_start(conn, BT_UUID_THROUGHPUT, &discovery_cb, &throughput);
		if (err) {
			LOG_ERR("Discover failed (err %d)", err);
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

	struct bt_scan_init_param init_param = {
		.connect_if_match = 1,
		.scan_param = &scan_param,
		.conn_param = setup_conn_param,
	};

	bt_scan_init(&init_param);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, uuid128);
	if (err) {
		LOG_ERR("Scanning filters cannot be set");
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on");
	}
}

static void scan_start(void)
{
	int err;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		LOG_ERR("Starting scanning failed (err %d)", err);
	}
}

static void adv_start(void)
{
	const struct bt_le_adv_param *adv_param =
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN,
				BT_GAP_ADV_FAST_INT_MIN_2,
				BT_GAP_ADV_FAST_INT_MAX_2,
				NULL);
	int err;

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Failed to start advertiser (%d)", err);
		return;
	}

	adv_active = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));

	/* A second connection rejected in connected() must not drop the tracked one. */
	if (conn != default_conn) {
		return;
	}

	test_ready = false;
	bt_conn_unref(default_conn);
	default_conn = NULL;

	/* Missing the first connection event (0x3e) happens intermittently on
	 * nrf7120dk, so keep retrying for as long as bt_cfg_tput is waiting.
	 */
	connect_retry();
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	LOG_INF("Connection parameters update request received");
	LOG_INF("Minimum interval: %d, Maximum interval: %d",
		param->interval_min, param->interval_max);
	LOG_INF("Latency: %d, Timeout: %d", param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	LOG_INF("Connection parameters updated. interval: %d, latency: %d, timeout: %d",
		interval, latency, timeout);

	k_sem_give(&throughput_sem);
}

static void le_phy_updated(struct bt_conn *conn,
			   struct bt_conn_le_phy_info *param)
{
	LOG_INF("LE PHY updated: TX PHY %s, RX PHY %s",
		phy2str(param->tx_phy), phy2str(param->rx_phy));

	k_sem_give(&throughput_sem);
}

static void le_data_length_updated(struct bt_conn *conn,
				   struct bt_conn_le_data_len_info *info)
{
	if (!data_length_req) {
		return;
	}

	LOG_INF("LE data len updated: TX (len: %d time: %d) RX (len: %d time: %d)",
		info->tx_max_len, info->tx_max_time, info->rx_max_len, info->rx_max_time);

	data_length_req = false;
	k_sem_give(&throughput_sem);
}

#if defined(CONFIG_BT_FRAME_SPACE_UPDATE)
static int frame_space_cmd(const struct shell *shell, size_t argc, char **argv)
{
	long frame_space_us;
	char *end = NULL;

	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (argc > 2) {
		shell_error(shell, "%s: bad parameters count", argv[0]);
		return -EINVAL;
	}

	frame_space_us = strtol(argv[1], &end, 10);

	if (end == argv[1] || *end != '\0' || frame_space_us < 0 || frame_space_us > 10000) {
		shell_error(shell, "%s: Invalid setting: %ld", argv[0], frame_space_us);
		shell_error(shell, "Frame space must be between: 0 and 10000");
		return -EINVAL;
	}

	test_params.frame_space_us = (uint16_t)frame_space_us;

	shell_print(shell, "Minimum frame space: %d us", test_params.frame_space_us);
	return 0;
}

static void frame_space_updated(struct bt_conn *conn,
				const struct bt_conn_le_frame_space_updated *params)
{
	if (params->status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("Frame space update failed: %d", params->status);
		return;
	}

	LOG_INF("Frame space updated: frame space %d us, PHYs 0x%04x, spacing types 0x%04x",
		params->frame_space, params->phys, params->spacing_types);

	k_sem_give(&throughput_sem);
}
#endif /* CONFIG_BT_FRAME_SPACE_UPDATE */

static const char *phy_str(const struct bt_conn_le_phy_param *phy)
{
	static const char *const str[] = {
		"1 Mbps",
		"2 Mbps",
		"Coded S2",
		"Coded S8",
		"Unknown"
	};

	switch (phy->pref_tx_phy) {
	case BT_GAP_LE_PHY_1M:
		return str[0];
	case BT_GAP_LE_PHY_2M:
		return str[1];
	case BT_GAP_LE_PHY_CODED:
		if (phy->options == BT_CONN_LE_PHY_OPT_CODED_S2) {
			return str[2];
		} else if (phy->options == BT_CONN_LE_PHY_OPT_CODED_S8) {
			return str[3];
		}
		__fallthrough;
	default:
		return str[4];
	}
}

static int print_cmd(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "==== Current BT throughput configuration ====");
	shell_print(shell, "Data length:\t\t%d", test_params.data_len->tx_max_len);
	shell_print(shell, "Connection interval:\t%d units",
		    test_params.conn_param->interval_min);
	shell_print(shell, "Preferred PHY:\t\t%s", phy_str(test_params.phy));
#if defined(CONFIG_BT_FRAME_SPACE_UPDATE)
	shell_print(shell, "Frame space:\t\t%d us", test_params.frame_space_us);
#endif
	return 0;
}

static uint8_t throughput_read(const struct bt_throughput_metrics *met)
{
	LOG_INF("[peer] received %u bytes (%u KB) in %u GATT writes at %u bps",
		met->write_len, met->write_len / 1024, met->write_count, met->write_rate);

	k_sem_give(&throughput_sem);

	return BT_GATT_ITER_STOP;
}

static void throughput_received(const struct bt_throughput_metrics *met)
{
	static uint32_t kb;

	if (met->write_len == 0) {
		kb = 0;
		LOG_INF("");
		return;
	}

	if ((met->write_len / 1024) != kb) {
		kb = (met->write_len / 1024);
		LOG_INF("=");
	}
}

static void throughput_send(const struct bt_throughput_metrics *met)
{
	LOG_INF("[local] received %u bytes (%u KB) in %u GATT writes at %u bps",
		met->write_len, met->write_len / 1024, met->write_count, met->write_rate);
}

static const struct bt_throughput_cb throughput_cb = {
	.data_read = throughput_read,
	.data_received = throughput_received,
	.data_send = throughput_send
};

static void scan_adv_stop(void)
{
	int err;

	err = bt_scan_stop();
	if (err && err != -EALREADY) {
		LOG_ERR("Stopping scanning failed (err %d)", err);
	}

	/* bt_le_adv_stop() logs an error when no advertiser exists, so only call it
	 * when this sample started advertising and no connection has ended it.
	 */
	if (adv_active) {
		err = bt_le_adv_stop();
		if (err) {
			LOG_ERR("Stopping advertising failed (err %d)", err);
		}
		adv_active = false;
	}
}

static void select_role(bool is_central)
{
	/* bt_cfg_tput can be run again with a different role, so stop whatever the
	 * previous role left running before starting the new one.
	 */
	scan_adv_stop();

	if (is_central) {
		LOG_INF("Central. Starting scanning");
		scan_start();
	} else {
		LOG_INF("Peripheral. Starting advertising");
		adv_start();
	}

	role_central = is_central;
}

/* default_conn is cleared from the Bluetooth RX thread on disconnection, so shell
 * commands take their own reference instead of using it directly. The caller
 * must release the returned reference with bt_conn_unref().
 */
static struct bt_conn *default_conn_get(void)
{
	struct bt_conn *conn = NULL;

	k_sched_lock();
	if (default_conn) {
		conn = bt_conn_ref(default_conn);
	}
	k_sched_unlock();

	return conn;
}

static int connection_configuration_set(const struct bt_le_conn_param *conn_param,
					const struct bt_conn_le_phy_param *phy,
					const struct bt_conn_le_data_len_param *data_len,
					uint16_t frame_space_us)
{
	int err;
	struct bt_conn_info info = {0};
	struct bt_conn *conn = default_conn_get();

	if (!conn) {
		LOG_ERR("Not connected");
		return -ENOTCONN;
	}

	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("Failed to get connection info %d", err);
		goto out;
	}

	if (info.role != BT_CONN_ROLE_CENTRAL) {
		LOG_INF("'run' command shall be executed only on the central board");
	}

	/* Drop any completion left over from a procedure the peer started, so each
	 * wait below only returns for the request made just before it.
	 */
	k_sem_reset(&throughput_sem);
	err = bt_conn_le_phy_update(conn, phy);
	if (err) {
		LOG_ERR("PHY update failed: %d", err);
		goto out;
	}

	LOG_INF("PHY update pending");
	err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC));
	if (err) {
		LOG_ERR("PHY update timeout");
		goto out;
	}

	if (BT_GAP_US_TO_CONN_INTERVAL(info.le.interval_us) != conn_param->interval_max) {
		k_sem_reset(&throughput_sem);
		err = bt_conn_le_param_update(conn, conn_param);
		if (err) {
			LOG_ERR("Connection parameters update failed: %d", err);
			goto out;
		}

		LOG_INF("Connection parameters update pending");
		err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC));
		if (err) {
			LOG_ERR("Connection parameters update timeout");
			goto out;
		}
	}

	if (info.le.data_len->tx_max_len != data_len->tx_max_len) {
		data_length_req = true;
		k_sem_reset(&throughput_sem);

		err = bt_conn_le_data_len_update(conn, data_len);
		if (err) {
			data_length_req = false;
			LOG_ERR("LE data length update failed: %d", err);
			goto out;
		}

		LOG_INF("LE Data length update pending");
		err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC));
		if (err) {
			data_length_req = false;
			LOG_ERR("LE Data Length update timeout");
			goto out;
		}
	}

	if (IS_ENABLED(CONFIG_BT_FRAME_SPACE_UPDATE)) {
		struct bt_conn_le_frame_space_update_param fsu_params;

		fsu_params.frame_space_min = frame_space_us;
		fsu_params.frame_space_max = MAX(150, frame_space_us);
		fsu_params.phys = phy->pref_tx_phy | phy->pref_rx_phy;
		fsu_params.spacing_types = BT_CONN_LE_FRAME_SPACE_TYPES_MASK_ACL_IFS;

		k_sem_reset(&throughput_sem);
		err = bt_conn_le_frame_space_update(conn, &fsu_params);
		if (err) {
			LOG_ERR("Frame space update failed: %d", err);
			goto out;
		}

		LOG_INF("Frame space update pending");
		err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC));
		if (err) {
			LOG_ERR("Frame space update timeout");
			goto out;
		}
	}

out:
	bt_conn_unref(conn);
	return err;
}

static int bt_throughput_test_run(const struct shell *shell,
				  const struct bt_le_conn_param *conn_param,
				  const struct bt_conn_le_phy_param *phy,
				  const struct bt_conn_le_data_len_param *data_len,
				  uint32_t ble_test_duration,
				  uint16_t frame_space_us)
{
	int err;
	int64_t stamp;
	int64_t delta;
	int write_err;
	uint64_t data = 0;

	/* a dummy data buffer */
	static char dummy[BT_TPUT_WRITE_LEN];

	if (!default_conn) {
		shell_error(shell, "Device is disconnected. "
			    "Connect to the peer device before running test");
		return -ENOTCONN;
	}

	if (bt_role == BT_CONN_ROLE_CENTRAL && !test_ready) {
		shell_error(shell, "Device is not ready. "
			    "Please wait for the service discovery and MTU exchange end");
		return -EAGAIN;
	}

	shell_print(shell, "\nStarting Bluetooth LE throughput test\n");

	err = connection_configuration_set(conn_param, phy, data_len, frame_space_us);
	if (err) {
		return err;
	}

	shell_print(shell, "Bluetooth LE throughput test is in progress ");
	shell_print(shell, "and requires around %u seconds to complete.\n", ble_test_duration);

	/* Make sure that all BLE procedures are finished. */
	k_sleep(K_MSEC(500));

	/* reset peer metrics */
	err = bt_throughput_write(&throughput, dummy, 1);
	if (err) {
		shell_error(shell, "Reset peer metrics failed.");
		return err;
	}

	stamp = k_uptime_get();

	while (true) {
		write_err = bt_throughput_write(&throughput, dummy, sizeof(dummy));
		if (write_err) {
			shell_error(shell, "GATT write failed (err %d)", write_err);
			break;
		}
		data += sizeof(dummy);
		if (k_uptime_get() - stamp > (int64_t)ble_test_duration * MSEC_PER_SEC) {
			break;
		}
	}

	delta = k_uptime_delta(&stamp);

	LOG_INF("Done");
	LOG_INF("[local] sent %llu bytes (%llu KB) in %lld ms at %llu kbps",
		data, data / 1024, delta, delta > 0 ? (data * 8 / (uint64_t)delta) : 0ULL);

	/* The link is usually gone after a write error, so skip the peer read. */
	if (write_err) {
		return write_err;
	}

	/* read back char from peer */
	k_sem_reset(&throughput_sem);
	err = bt_throughput_read(&throughput);
	if (err) {
		shell_error(shell, "GATT read failed (err %d)", err);
		return err;
	}

	if (k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC))) {
		shell_warn(shell, "Peer metrics were not received");
	}

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
#if defined(CONFIG_BT_FRAME_SPACE_UPDATE)
	.frame_space_updated = frame_space_updated,
#endif
	.security_changed = security_changed
};

/* One-time Bluetooth setup. The stock throughput sample is used as reference;
 * here it runs on the first bt_cfg_tput so Bluetooth stays off until it is needed.
 */
static int bt_stack_init(void)
{
	static bool initialized;
	int err;

	if (initialized) {
		return 0;
	}

	err = bt_enable(NULL);
	if (err && err != -EALREADY) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");

	scan_init();

	err = bt_throughput_init(&throughput, &throughput_cb);
	if (err) {
		LOG_ERR("Throughput service initialization failed");
		return err;
	}

	initialized = true;

	return 0;
}

/* Poll until a connection exists (or no longer exists) or the configuration
 * timeout expires. default_conn is updated from the Bluetooth callbacks.
 */
static bool wait_for_connection_state(bool connected)
{
	int64_t stamp = k_uptime_get();

	while (k_uptime_delta(&stamp) / MSEC_PER_SEC < THROUGHPUT_CONFIG_TIMEOUT_SEC) {
		if ((default_conn != NULL) == connected) {
			return true;
		}
		k_sleep(K_MSEC(100));
	}

	return (default_conn != NULL) == connected;
}

/* On the central, wait for service discovery and the MTU exchange to finish
 * (test_ready) so the PHY, connection parameter and data length updates do not
 * compete with them on the link. Gives up early if the link drops.
 */
static int wait_for_test_ready(void)
{
	int64_t stamp = k_uptime_get();

	LOG_INF("Waiting for service discovery and MTU exchange");

	while (k_uptime_delta(&stamp) / MSEC_PER_SEC < THROUGHPUT_CONFIG_TIMEOUT_SEC) {
		if (test_ready) {
			return 0;
		}
		if (!default_conn) {
			LOG_ERR("Disconnected before service discovery completed");
			return -ENOTCONN;
		}
		k_sleep(K_MSEC(100));
	}

	if (test_ready) {
		return 0;
	}

	LOG_ERR("Service discovery and MTU exchange timeout");
	return -ETIMEDOUT;
}

static int bt_throughput_test_init(bool is_central)
{
	struct bt_conn *conn;
	int err;

	err = bt_stack_init();
	if (err) {
		return err;
	}

	LOG_INF("Bluetooth LE role is %s", is_central ? "central" : "peripheral");

	conn = default_conn_get();
	if (conn) {
		if ((bt_role == BT_CONN_ROLE_CENTRAL) == is_central) {
			bt_conn_unref(conn);
			LOG_INF("Already connected in this role, applying the test configuration");
			goto configure;
		}

		LOG_INF("Connected in the other role, disconnecting to change role");
		err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		bt_conn_unref(conn);
		if (err) {
			LOG_ERR("Disconnect failed (err %d)", err);
			return err;
		}

		if (!wait_for_connection_state(false)) {
			LOG_ERR("Disconnect timeout");
			return -ETIMEDOUT;
		}
	}

	connect_pending = true;
	select_role(is_central);

	LOG_INF("BLE setup connection interval MIN = %d", SETUP_INTERVAL_MIN);
	LOG_INF("BLE setup connection interval MAX = %d", SETUP_INTERVAL_MAX);
	LOG_INF("BLE connection latency = %d", CONFIG_CONN_LATENCY);
	LOG_INF("BLE setup supervision timeout = %d", SETUP_SUPERVISION_TIMEOUT);
	LOG_INF("BLE test connection interval = %d", CONFIG_INTERVAL_MAX);
	LOG_INF("Waiting for connection");
	wait_for_connection_state(true);
	connect_pending = false;

	if (!default_conn) {
		LOG_ERR("Cannot set up connection");
		return -ENOTCONN;
	}

configure:
	if (is_central) {
		err = wait_for_test_ready();
		if (err) {
			return err;
		}
	}

	return connection_configuration_set(test_params.conn_param, test_params.phy,
					    test_params.data_len, test_params.frame_space_us);
}

static int bt_disconnection(const struct shell *shell, size_t argc, char **argv)
{
	struct bt_conn *conn;
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	conn = default_conn_get();
	if (!conn) {
		shell_error(shell, "Not connected!");
		return -ENOTCONN;
	}

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	bt_conn_unref(conn);
	if (err) {
		shell_error(shell, "Bluetooth LE disconnection - FAIL");
	} else {
		shell_print(shell, "Bluetooth LE disconnection - SUCCESS");
	}

	return err;
}

static int bt_run_throughput(const struct shell *shell, size_t argc, char **argv)
{
	char *end = NULL;
	unsigned long ble_test_duration;
	int err;

	if (argc < 2) {
		shell_error(shell, "invalid # of args : %zu", argc);
		shell_print(shell, "Usage: bt_run_tput ble_test_duration\n");
		shell_print(shell, "       ble_test_duration: Test duration in seconds");
		return -ENOEXEC;
	}

	ble_test_duration = strtoul(argv[1], &end, 10);
	if (end == argv[1] || *end != '\0' || ble_test_duration == 0 ||
	    ble_test_duration > UINT32_MAX / MSEC_PER_SEC) {
		shell_error(shell, "ble_test_duration: '%s' is not a valid number of seconds",
			    argv[1]);
		return -EINVAL;
	}

	err = bt_throughput_test_run(shell, test_params.conn_param, test_params.phy,
				     test_params.data_len, (uint32_t)ble_test_duration,
				     test_params.frame_space_us);
	if (err) {
		shell_error(shell, "Bluetooth LE throughput - FAIL");
	} else {
		shell_print(shell, "Bluetooth LE throughput - SUCCESS");
	}

	return err;
}

/* Parse a shell argument that must be exactly "0" or "1". */
static int parse_flag(const struct shell *shell, const char *name, const char *arg, bool *out)
{
	char *end = NULL;
	unsigned long value = strtoul(arg, &end, 10);

	if (end == arg || *end != '\0' || value > 1) {
		shell_error(shell, "%s: '%s' must be 0 or 1", name, arg);
		return -EINVAL;
	}

	*out = (value == 1);

	return 0;
}

static int bt_configure_throughput(const struct shell *shell, size_t argc, char **argv)
{
	bool is_central;
	int ret;

	if (argc < 2) {
		shell_error(shell, "invalid # of args : %zu", argc);
		shell_print(shell, "Usage: bt_cfg_tput bt_role\n");
		shell_print(shell, "       bt_role: 1 for central, 0 for peripheral");
		return -ENOEXEC;
	}

	ret = parse_flag(shell, "bt_role", argv[1], &is_central);
	if (ret) {
		shell_print(shell, "       bt_role: 1 for central, 0 for peripheral");
		return ret;
	}

	ret = bt_throughput_test_init(is_central);
	if (ret) {
		shell_error(shell, "Bluetooth LE tput config - FAIL");
	} else {
		shell_print(shell, "Bluetooth LE tput config - SUCCESS");
	}

	return ret;
}

#ifdef CONFIG_NRF70_SR_COEX
static int coex_configure_the_pta(const struct shell *shell, size_t argc, char **argv)
{
	int result;
	bool wlan_band;
	bool separate_antennas;
	bool is_sr_protocol_ble;

	if (argc < 4) {
		shell_error(shell, "invalid # of args : %zu", argc);
		shell_print(shell, "Usage: coex_config_pta wifi_band is_sep_antennas is_sr_ble\n");
		shell_print(shell, "       wifi_band: 0 for 2.4GHz, 1 for 5GHz");
		shell_print(shell, "       is_sep_antennas: 0 for shared antenna,");
		shell_print(shell, "                        1 for separate antennas");
		shell_print(shell, "       is_sr_ble: 0 for Thread, 1 for Bluetooth protocol");
		return -ENOEXEC;
	}

	if (parse_flag(shell, "wifi_band", argv[1], &wlan_band) ||
	    parse_flag(shell, "is_sep_antennas", argv[2], &separate_antennas) ||
	    parse_flag(shell, "is_sr_ble", argv[3], &is_sr_protocol_ble)) {
		shell_error(shell, "Configuration of PTA - FAIL");
		return -EINVAL;
	}

	LOG_INF("WLAN operating band: %s", wlan_band ? "5GHz" : "2.4GHz");
	LOG_INF("Antenna mode: Wi-Fi and SR %s",
		separate_antennas ? "uses separate antennas" : "shares antenna");
	LOG_INF("SR protocol: %s", is_sr_protocol_ble ? "Bluetooth LE" : "Thread");

	result = nrf_wifi_coex_config_non_pta(separate_antennas, is_sr_protocol_ble);
	if (!result) {
		result = nrf_wifi_coex_config_pta(wlan_band, separate_antennas,
						  is_sr_protocol_ble);
	}

	if (result) {
		shell_error(shell, "Configuration of PTA - FAIL");
	} else {
		shell_print(shell, "Configuration of PTA - SUCCESS");
	}

	return result;
}

static int coex_hardware_disable(const struct shell *shell, size_t argc, char **argv)
{
	int status;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	status = nrf_wifi_coex_hw_reset();
	if (status) {
		shell_error(shell, "COEXC disable - FAIL");
	} else {
		shell_print(shell, "COEXC disable - SUCCESS");
	}

	return status;
}
#endif /* CONFIG_NRF70_SR_COEX */

#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
static int coex_configure_sr_switch(const struct shell *shell, size_t argc, char **argv)
{
	/* Configure SR side switch - to handle both Shared and separate antennas modes */
	int ret;
	bool separate_antennas;

	if (argc < 2) {
		shell_error(shell, "invalid # of args : %zu", argc);
		shell_print(shell, "Usage: coex_cfg_sr_switch is_sep_antennas\n");
		shell_print(shell, "       is_sep_antennas: 0 for shared antenna, "
			    "1 for separate antennas");
		return -ENOEXEC;
	}

	ret = parse_flag(shell, "is_sep_antennas", argv[1], &separate_antennas);
	if (ret) {
		shell_error(shell, "Configuration of SR side switch - FAIL");
		return ret;
	}

	LOG_INF("Antenna mode: Wi-Fi and SR %s",
		separate_antennas ? "uses separate antennas" : "shares antenna");

	ret = nrf_wifi_config_sr_switch(separate_antennas);
	if (ret != 0) {
		shell_error(shell, "Configuration of SR side switch - FAIL");
	} else {
		shell_print(shell, "Configuration of SR side switch - SUCCESS");
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
		LOG_INF("New UDP session started");
		break;
	case ZPERF_SESSION_PERIODIC_RESULT:
		/* Ignored. */
		break;
	case ZPERF_SESSION_FINISHED:
		LOG_INF("Wi-Fi benchmark: Upload completed!");
		if (!result) {
			LOG_ERR("Result is NULL, Zperf session error");
			break;
		}

		if (result->client_time_in_us != 0U) {
			client_rate_in_kbps = (uint32_t)
				(((uint64_t)result->nb_packets_sent *
				  (uint64_t)result->packet_size * (uint64_t)8 *
				  (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)result->client_time_in_us * 1024U));
		} else {
			client_rate_in_kbps = 0U;
		}

		LOG_INF("Upload results:");
		LOG_INF("%llu bytes in %llu ms",
			(uint64_t)result->nb_packets_sent * result->packet_size,
			(result->client_time_in_us / USEC_PER_MSEC));
		LOG_INF("%u packets sent", result->nb_packets_sent);
		LOG_INF("%u packets lost", result->nb_packets_lost);
		LOG_INF("%u packets received", result->nb_packets_rcvd);
		LOG_INF("Client rate: %u kbps", client_rate_in_kbps);
		break;
	case ZPERF_SESSION_ERROR:
		LOG_ERR("UDP session error");
		break;
	}
}

static int parse_ipv4_addr(const char *host, struct sockaddr_in *addr)
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

	LOG_INF("IPv4 address %s", host);

	return 0;
}

/* Parse a packet size given either as plain bytes ("1024") or kilobytes ("1K"). */
static int parse_packet_size(const struct shell *shell, const char *arg, uint16_t *out)
{
	char *end = NULL;
	unsigned long value = strtoul(arg, &end, 10);
	unsigned long bytes;

	if (end == arg) {
		shell_error(shell, "pktSize: '%s' is not a number", arg);
		return -EINVAL;
	}

	if (*end == 'K' || *end == 'k') {
		/* Clamp before scaling so a huge value cannot wrap into the valid range. */
		bytes = MIN(value, CONFIG_NET_ZPERF_MAX_PACKET_SIZE + 1UL) * 1024;
		end++;
	} else {
		bytes = value;
	}

	if (*end != '\0') {
		shell_error(shell,
			    "pktSize: trailing characters in '%s', expected bytes or a K suffix",
			    arg);
		return -EINVAL;
	}

	if (bytes < WIFI_TPUT_MIN_PKT || bytes > CONFIG_NET_ZPERF_MAX_PACKET_SIZE) {
		shell_error(shell, "pktSize: %lu out of range, must be %d to %d bytes",
			    bytes, WIFI_TPUT_MIN_PKT, CONFIG_NET_ZPERF_MAX_PACKET_SIZE);
		shell_error(shell, "zperf silently clamps above its maximum, "
			    "so the request is rejected here");
		return -EINVAL;
	}

	if (bytes > WIFI_TPUT_MAX_UNFRAGMENTED_PKT) {
		shell_warn(shell, "pktSize: %lu exceeds %d, datagrams will be IP fragmented",
			   bytes, WIFI_TPUT_MAX_UNFRAGMENTED_PKT);
	}

	*out = (uint16_t)bytes;

	return 0;
}

/* Parse a rate given in bit/s ("10000000"), kbit/s ("10000K") or Mbit/s ("10M"). */
static int parse_rate_kbps(const struct shell *shell, const char *arg, uint32_t *out)
{
	char *end = NULL;
	unsigned long value = strtoul(arg, &end, 10);
	unsigned long kbps;

	if (end == arg) {
		shell_error(shell, "baudrate: '%s' is not a number", arg);
		return -EINVAL;
	}

	if (*end == 'M' || *end == 'm') {
		if (value > UINT32_MAX / 1000) {
			shell_error(shell, "baudrate: '%s' is too large", arg);
			return -EINVAL;
		}
		kbps = value * 1000;
		end++;
	} else if (*end == 'K' || *end == 'k') {
		kbps = value;
		end++;
	} else {
		kbps = value / 1000;
	}

	if (*end != '\0' || kbps == 0) {
		shell_error(shell,
			    "baudrate: '%s' must be at least 1K, with an optional K or M suffix",
			    arg);
		return -EINVAL;
	}

	*out = (uint32_t)kbps;

	return 0;
}

static int wifi_link_check(const struct shell *shell)
{
	struct wifi_iface_status status = { 0 };
	struct net_if *iface = net_if_get_first_wifi();
	int ret;

	if (!iface) {
		shell_error(shell, "No Wi-Fi interface found");
		return -ENODEV;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status, sizeof(status));
	if (ret) {
		shell_error(shell, "Failed to read Wi-Fi interface status (%d)", ret);
		return ret;
	}

	if (status.state < WIFI_STATE_COMPLETED) {
		shell_error(shell, "Wi-Fi is not connected (state: %s)",
			    wifi_state_txt(status.state));
		shell_error(shell,
			    "Connect first: wifi connect -s <SSID> -k <key_mgmt> -p <passphrase>");
		return -ENOTCONN;
	}

	if (net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED) == NULL) {
		shell_error(shell, "Wi-Fi is connected but has no IPv4 address yet");
		shell_error(shell, "Wait for DHCP to complete, then retry");
		return -EADDRNOTAVAIL;
	}

	return 0;
}

static int wifi_run_throughput(const struct shell *shell, size_t argc, char **argv)
{
	struct zperf_upload_params params = { 0 };
	struct sockaddr_in peer_addr = {
		.sin_family = AF_INET,
	};
	char *end = NULL;
	unsigned long port;
	unsigned long duration_sec;
	uint16_t packet_size = 0;
	uint32_t rate_kbps = 0;
	int ret;

	if (argc < 8) {
		shell_error(shell, "invalid # of args : %zu", argc);
		shell_print(shell, "Usage: wifi_run_tput protocol direction peerIP destPort "
			    "duration pktSize baudrate\n");
		shell_print(shell, "       protocol - udp or tcp");
		shell_print(shell, "       direction - upload or download");
		shell_print(shell, "       peerIP - IP of the peer device");
		shell_print(shell, "       destPort - port of the peer device");
		shell_print(shell, "       duration - test duration in seconds");
		shell_print(shell, "       pktSize - bytes (e.g. 512, 1024) or kilobytes (e.g. 1K)");
		shell_print(shell, "       baudrate - bit/s, or kbit/s or Mbit/s "
			    "with a K or M suffix");
		return -ENOEXEC;
	}

	if (strcmp(argv[1], "udp") != 0 && strcmp(argv[1], "tcp") != 0) {
		shell_error(shell, "protocol: '%s' must be udp or tcp", argv[1]);
		return -EINVAL;
	}

	if (strcmp(argv[2], "upload") != 0 && strcmp(argv[2], "download") != 0) {
		shell_error(shell, "direction: '%s' must be upload or download", argv[2]);
		return -EINVAL;
	}

	if (strcmp(argv[1], "udp") != 0 || strcmp(argv[2], "upload") != 0) {
		shell_error(shell, "Currently only udp upload is supported");
		return -ENOEXEC;
	}

	ret = wifi_link_check(shell);
	if (ret) {
		shell_error(shell, "Running Wi-Fi throughput - FAIL");
		return ret;
	}

	ret = parse_ipv4_addr(argv[3], &peer_addr);
	if (ret) {
		shell_error(shell, "peerIP: '%s' is not a valid IPv4 address", argv[3]);
		return ret;
	}

	port = strtoul(argv[4], &end, 10);
	if (end == argv[4] || *end != '\0' || port == 0 || port > UINT16_MAX) {
		shell_error(shell, "destPort: '%s' must be 1 to %d", argv[4], UINT16_MAX);
		return -EINVAL;
	}

	duration_sec = strtoul(argv[5], &end, 10);
	if (end == argv[5] || *end != '\0' || duration_sec == 0 ||
	    duration_sec > UINT32_MAX / MSEC_PER_SEC) {
		shell_error(shell, "duration: '%s' is not a valid number of seconds", argv[5]);
		return -EINVAL;
	}

	ret = parse_packet_size(shell, argv[6], &packet_size);
	if (ret) {
		shell_error(shell, "Running Wi-Fi throughput - FAIL");
		return ret;
	}

	ret = parse_rate_kbps(shell, argv[7], &rate_kbps);
	if (ret) {
		shell_error(shell, "Running Wi-Fi throughput - FAIL");
		return ret;
	}

	peer_addr.sin_port = htons((uint16_t)port);

	params.duration_ms = (uint32_t)duration_sec * MSEC_PER_SEC;
	params.rate_kbps = rate_kbps;
	params.packet_size = packet_size;
	memcpy(&params.peer_addr, &peer_addr, sizeof(peer_addr));

	LOG_INF("Starting Wi-Fi benchmark: Zperf udp client");

	ret = zperf_udp_upload_async(&params, udp_upload_results_cb, NULL);
	if (ret != 0) {
		shell_error(shell, "Running Wi-Fi throughput - FAIL");
	}

	return ret;
}

#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
SHELL_CMD_REGISTER(coex_cfg_sr_switch, NULL, "Configure SR side switch",
		   coex_configure_sr_switch);
#endif /* CONFIG_NRF70_SR_COEX_RF_SWITCH */

#ifdef CONFIG_NRF70_SR_COEX
SHELL_CMD_REGISTER(coex_config_pta, NULL, "Configure the coexistence PTA",
		   coex_configure_the_pta);
SHELL_CMD_REGISTER(coex_hw_disable, NULL, "Disable coexistence hardware",
		   coex_hardware_disable);
#endif /* CONFIG_NRF70_SR_COEX */

SHELL_CMD_REGISTER(bt_cfg_tput, NULL, "Run BT config for throughput", bt_configure_throughput);
SHELL_CMD_REGISTER(bt_run_tput, NULL, "Run BT throughput", bt_run_throughput);
SHELL_CMD_REGISTER(bt_disconnect, NULL, "Run BT disconnect", bt_disconnection);
SHELL_CMD_REGISTER(wifi_run_tput, NULL, "Run Wi-Fi throughput", wifi_run_throughput);
#if defined(CONFIG_BT_FRAME_SPACE_UPDATE)
SHELL_CMD_REGISTER(bt_frame_space, NULL, "Configure frame space <us>", frame_space_cmd);
#endif
SHELL_CMD_REGISTER(bt_print_cfg, NULL, "Print current BT throughput configuration", print_cmd);
