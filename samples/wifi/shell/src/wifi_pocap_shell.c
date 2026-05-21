/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief WiFi pre-silicon testing shell
 */

#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(pocap_mode, CONFIG_LOG_DEFAULT_LEVEL);
#define NRF_WIFICORE_RPURFBUS_RFCTRL		     ((NRF_WIFICORE_RPURFBUS_BASE) + 0x00010000UL)
#define NRF_WIFICORE_RPURFBUS_BASE		     0x48020000UL
#define NRF_WIFICORE_RPURFBUS_RFCTRL_AXIMASTERACCESS ((NRF_WIFICORE_RPURFBUS_RFCTRL) + 0x00000000UL)

#define RDW(addr)	(*(volatile unsigned int *)(addr))
#define WRW(addr, data) (*(volatile unsigned int *)(addr) = (data))

struct rf_pocap_reg_config {
	uint32_t offset;
	uint32_t value;
	const char *reg_name;
	const char *desc;
};

static int configure_playout_capture(uint32_t rx_mode, uint32_t tx_mode,
				     uint32_t rx_holdoff_length, uint32_t rx_wrap_length,
				     uint32_t back_to_back_mode)
{
	unsigned int value;
	int i;

	LOG_INF("Setting RF Playout Capture Config");

	WRW(NRF_WIFICORE_RPURFBUS_RFCTRL_AXIMASTERACCESS, 0x31);
	while (RDW(NRF_WIFICORE_RPURFBUS_RFCTRL_AXIMASTERACCESS) != 0x31) {
	}

	struct rf_pocap_reg_config reg_configs[] = {
		{.offset = 0x0000,
		 .value = (tx_mode << 1) | rx_mode,
		 .reg_name = "WLAN_RF_POCAP_CONFIG",
		 .desc = "RX/TX mode"},
		{.offset = 0x0004,
		 .value = rx_holdoff_length & 0xFF,
		 .reg_name = "WLAN_RF_POCAP_RX_HOLDOFF_CONFIG",
		 .desc = "RX holdoff length"},
		{.offset = 0x0008,
		 .value = rx_wrap_length,
		 .reg_name = "WLAN_RF_POCAP_RX_WRAP_CONFIG",
		 .desc = "RX wrap length"},
		{.offset = 0x000C,
		 .value = (back_to_back_mode << 1),
		 .reg_name = "WLAN_POCAP_VERSION_CONFIG",
		 .desc = "Back-to-back mode"}
	};

	for (i = 0; i < ARRAY_SIZE(reg_configs); i++) {
		struct rf_pocap_reg_config *cfg = &reg_configs[i];

		if (back_to_back_mode && cfg->offset != 0x000C) {
			continue;
		}

		WRW(NRF_WIFICORE_RPURFBUS_BASE + cfg->offset, cfg->value);
		value = RDW(NRF_WIFICORE_RPURFBUS_BASE + cfg->offset);
		LOG_INF("%s (0x%04x) = 0x%x - %s", cfg->reg_name, cfg->offset, value, cfg->desc);
	}

	LOG_INF("RF Playout Capture Config completed");

	return 0;
}

static int cmd_wifi_pocap(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t rx_mode = 0, tx_mode = 0, rx_holdoff = 0, rx_wrap = 0, b2b_mode = 0;
	int opt;

	optind = 1;

	while ((opt = getopt(argc, argv, "r:t:h:w:b:")) != -1) {
		switch (opt) {
		case 'r':
			rx_mode = (uint32_t)atoi(optarg);
			break;
		case 't':
			tx_mode = (uint32_t)atoi(optarg);
			break;
		case 'h':
			rx_holdoff = (uint32_t)atoi(optarg);
			break;
		case 'w':
			rx_wrap = (uint32_t)atoi(optarg);
			break;
		case 'b':
			b2b_mode = (uint32_t)atoi(optarg);
			break;
		default:
			shell_print(
				sh,
				"Usage: wifi_pocap [-b b2b_mode] [other opts]\n"
				"b2b_mode=0: -r rx_mode -t tx_mode -h rx_holdoff_len -w "
				"rx_wrap_len\n"
				"Defaults: rx_mode=0 tx_mode=0 rx_holdoff=0 rx_wrap=0 b2b_mode=0");
			return -EINVAL;
		}
	}

	if (b2b_mode) {
		shell_print(sh, "Config: back_to_back_mode=%u (others ignored in B2B mode)",
			    b2b_mode);
		configure_playout_capture(0, 0, 0, 0, b2b_mode);
	} else {
		shell_print(sh, "Config: rx=%u tx=%u holdoff=%u wrap=%u b2b=%u", rx_mode, tx_mode,
			    rx_holdoff, rx_wrap, b2b_mode);
		configure_playout_capture(rx_mode, tx_mode, rx_holdoff, rx_wrap, 0);
	}

	return 0;
}

SHELL_CMD_REGISTER(wifi_pocap, NULL, "Configure RF Playout Capture", cmd_wifi_pocap);
