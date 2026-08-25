/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Host-side COEXC hardware initialization (CCMALLOW, CCCONF, TURNAROUND).
 *
 * Ported from @c configure_coexc() and @c enable_coexc() in coex_manager_tb.c
 * (TEST_CONFIGURE_ENABLE_COEXC), per Coexistence Driver Implementation
 * Specification Section 4.1.
 */

#include <errno.h>

#include <zephyr/logging/log.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
#include <nrf71_coex_hw_regs.h>
#include <nrf71_coex_if.h>

LOG_MODULE_DECLARE(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

#define WLAN_HIGH_PTI_RX_CCCONF_PTI 1U
#define WLAN_HIGH_PTI_TX_CCCONF_PTI 2U
#define WLAN_LOW_PTI_RX_CCCONF_PTI  4U
#define WLAN_LOW_PTI_TX_CCCONF_PTI  5U
#define SR_RX_CCCONF_PTI            2U
#define SR_TX_CCCONF_PTI            3U
#define EXT_RX_CCCONF_PTI           1U
#define EXT_TX_CCCONF_PTI           1U

#define TURNAROUND_WIFI_IN_US 1U
#define TURNAROUND_SR_IN_US   1U
#define TURNAROUND_EXT_IN_US  1U

#define TURNAROUND_WIFI (TURNAROUND_WIFI_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_SR   (TURNAROUND_SR_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_EXT  (TURNAROUND_EXT_IN_US * COEXC_CLK_FREQ_MHZ)

/** Write a COEXC register through the Wi-Fi FMAC coexistence transport. */
static int cd_coexc_reg_write(uint32_t abs_addr, uint32_t value)
{
	int ret = nrf71_wifi_coex_reg_write(abs_addr, value);

	if (ret == -ENODEV) {
		return -EACCES;
	}

	return ret;
}

/** Read a COEXC register through the Wi-Fi FMAC coexistence transport. */
static int cd_coexc_reg_read(uint32_t abs_addr, uint32_t *value)
{
	if (value == NULL) {
		return -EINVAL;
	}

	return nrf71_wifi_coex_reg_read(abs_addr, value);
}

/**
 * Program COEXC CCMALLOW, CCCONF, and TURNAROUND tables for the selected
 * shared- or separate-antenna configuration.
 */
int cd_coexc_configure(enum coex_antenna_cfg_type antenna_cfg_type)
{
	uint32_t ccconf_config[COEXC_NUM_CLIENTS];
	const uint32_t turnaround_config[COEXC_NUM_CLIENTS] = {
		TURNAROUND_WIFI, TURNAROUND_WIFI, TURNAROUND_WIFI, TURNAROUND_WIFI,
		TURNAROUND_SR, TURNAROUND_SR, TURNAROUND_EXT, TURNAROUND_EXT,
	};
	enum coexc_mode_wifi_t coexc_mode_wifi = COEXC_MODE_WIFI_2PT4G;
	unsigned int coexc_mode_sr = 0U;
	unsigned int coexc_mode_ext = 0U;
	const uint32_t ccmallow_sep_ant[COEXC_NUM_CLIENTS][COEXC_NUM_MODES] = {
		{0x00011111UL, 0x00222222UL, 0U, 0U},
		{0x00001111UL, 0x00202222UL, 0U, 0U},
		{0x00011111UL, 0x00222222UL, 0U, 0U},
		{0x00101111UL, 0x00202222UL, 0U, 0U},
		{0x00110101UL, 0x00220202UL, 0U, 0U},
		{0x00111000UL, 0x00222222UL, 0U, 0U},
		{0x11000000UL, 0x22000000UL, 0U, 0U},
		{0x11000000UL, 0x22000000UL, 0U, 0U},
	};
	const uint32_t ccmallow_sha_ant[COEXC_NUM_CLIENTS][COEXC_NUM_MODES] = {
		{0x00011111UL, 0x00022222UL, 0U, 0U},
		{0x00001111UL, 0x00202222UL, 0U, 0U},
		{0x00011111UL, 0x00022222UL, 0U, 0U},
		{0x00001111UL, 0x00202222UL, 0U, 0U},
		{0x00110101UL, 0x00220202UL, 0U, 0U},
		{0x00110000UL, 0x00222020UL, 0U, 0U},
		{0x11000000UL, 0x22000000UL, 0U, 0U},
		{0x11000000UL, 0x22000000UL, 0U, 0U},
	};
	const uint32_t (*p_ccmallow)[COEXC_NUM_MODES];
	int ret;

	if ((antenna_cfg_type != COEX_SEPARATE_ANT_CFG) &&
	    (antenna_cfg_type != COEX_SHARED_ANT_CFG)) {
		return -EINVAL;
	}

	if (!nrf71_wifi_coex_is_ready()) {
		return -EACCES;
	}

	p_ccmallow = (antenna_cfg_type == COEX_SEPARATE_ANT_CFG) ? ccmallow_sep_ant :
								 ccmallow_sha_ant;

	LOG_INF("Configuring COEXC (CCMALLOW, CCCONF, TURNAROUND)");

	for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
		const uint32_t base_client_offset = client * COEXC_OFFSET_BETWEEN_CLIENTS;

		for (uint32_t mode = 0U; mode < COEXC_NUM_MODES; mode++) {
			const uint32_t offset = (COEXC_OFFSET_BETWEEN_MODES * mode) +
						base_client_offset;

			ret = cd_coexc_reg_write(ABS_COEXC_CCMALLOW_0_MODE_0 + offset,
						 p_ccmallow[client][mode]);
			if (ret != 0) {
				LOG_ERR("CCMALLOW write failed (client=%u mode=%u)", client, mode);
				return ret;
			}
		}
	}

	ccconf_config[0] = (WLAN_HIGH_PTI_RX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
			   (coexc_mode_wifi << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[1] = (WLAN_HIGH_PTI_TX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
			   (coexc_mode_wifi << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[2] = (WLAN_LOW_PTI_RX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
			   (coexc_mode_wifi << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[3] = (WLAN_LOW_PTI_TX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
			   (coexc_mode_wifi << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[4] = (SR_RX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
			   (coexc_mode_sr << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[5] = (SR_TX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
			   (coexc_mode_sr << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[6] = (EXT_RX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
			   (coexc_mode_ext << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[7] = (EXT_TX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
			   (coexc_mode_ext << COEXC_CCCONF_0_MODE_SHIFT);

	for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
		ret = cd_coexc_reg_write(ABS_COEXC_CCCONF_0 +
						 (COEXC_OFFSET_BETWEEN_MODES * client),
					 ccconf_config[client]);
		if (ret != 0) {
			LOG_ERR("CCCONF write failed (client=%u)", client);
			return ret;
		}
	}

	for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
		ret = cd_coexc_reg_write(ABS_COEXC_TURNAROUND_0 +
						 (COEXC_OFFSET_BETWEEN_MODES * client),
					 turnaround_config[client]);
		if (ret != 0) {
			LOG_ERR("TURNAROUND write failed (client=%u)", client);
			return ret;
		}
	}

	return 0;
}

/** Set or clear the COEX enable bit in PMB_WLAN_MAC_CTRL_COEX. */
int cd_coexc_enable(enum coexc_hw_enable enable)
{
	uint32_t coexc_config;
	int ret;

	if (!nrf71_wifi_coex_is_ready()) {
		return -EACCES;
	}

	ret = cd_coexc_reg_read(ABS_PMB_WLAN_MAC_CTRL_COEX, &coexc_config);
	if (ret != 0) {
		LOG_ERR("Failed to read PMB_WLAN_MAC_CTRL_COEX");
		return ret;
	}

	coexc_config = (coexc_config & ~PMB_WLAN_MAC_CTRL_COEX_ENABLE_MASK) |
		       ((uint32_t)enable << PMB_WLAN_MAC_CTRL_COEX_ENABLE_SHIFT);

	ret = cd_coexc_reg_write(ABS_PMB_WLAN_MAC_CTRL_COEX, coexc_config);
	if (ret != 0) {
		LOG_ERR("Failed to write PMB_WLAN_MAC_CTRL_COEX");
		return ret;
	}

	LOG_INF("COEXC %s", (enable == COEXC_HW_ENABLE) ? "enabled" : "disabled");
	return 0;
}

/** Configure COEXC tables and enable coexistence hardware in one step. */
int cd_coexc_configure_and_enable(enum coex_antenna_cfg_type antenna_cfg_type)
{
	int ret;

	ret = cd_coexc_configure(antenna_cfg_type);
	if (ret != 0) {
		return ret;
	}

	return cd_coexc_enable(COEXC_HW_ENABLE);
}

/**
 * Verify that COEXC CCMALLOW client0/mode0 and the enable bit match the
 * expected shared- or separate-antenna configuration.
 */
int cd_coexc_verify_configured(enum coex_antenna_cfg_type antenna_cfg_type,
			       uint32_t *ccmallow_client0_mode0)
{
	const uint32_t ccmallow_sha_ant_client0_mode0 = 0x00011111UL;
	const uint32_t ccmallow_sep_ant_client0_mode0 = 0x00011111UL;
	uint32_t expected;
	uint32_t readback;
	uint32_t pmb;
	int ret;

	if (ccmallow_client0_mode0 == NULL) {
		return -EINVAL;
	}

	if (!nrf71_wifi_coex_is_ready()) {
		return -EACCES;
	}

	expected = (antenna_cfg_type == COEX_SEPARATE_ANT_CFG) ?
			   ccmallow_sep_ant_client0_mode0 :
			   ccmallow_sha_ant_client0_mode0;

	ret = cd_coexc_reg_read(ABS_COEXC_CCMALLOW_0_MODE_0, &readback);
	if (ret != 0) {
		return ret;
	}

	if (readback != expected) {
		LOG_ERR("CCMALLOW verify mismatch: got 0x%08x expected 0x%08x", readback,
			expected);
		return -EIO;
	}

	ret = cd_coexc_reg_read(ABS_PMB_WLAN_MAC_CTRL_COEX, &pmb);
	if (ret != 0) {
		return ret;
	}

	if ((pmb & PMB_WLAN_MAC_CTRL_COEX_ENABLE_MASK) == 0U) {
		LOG_ERR("COEXC enable bit not set in PMB_WLAN_MAC_CTRL_COEX");
		return -EIO;
	}

	*ccmallow_client0_mode0 = readback;
	return 0;
}
