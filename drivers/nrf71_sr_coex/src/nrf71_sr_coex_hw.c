/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief COEXC hardware initialization (CCMALLOW, CCCONF, TURNAROUND).
 * This file programs three register blocks of COEXC: CCMALLOW, CCCONF, and TURNAROUND
 * Two antenna layouts are supported:
 *   COEX_SHARED_ANT_CFG   � Wi-Fi and SR share one physical antenna
 *   COEX_SEPARATE_ANT_CFG � radios use separate antennas (different CCMALLOW tables)
 */

#include <errno.h>

#include <zephyr/logging/log.h>

/* Wi-Fi FMAC coexistence transport (cmd/event/reg access). */
#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
/* COEXC register addresses (local copy when fw_if header absent). */
#include <nrf71_coex_hw_regs.h>
/* CD2CM / CM2CD message structures shared with RPU firmware. */
#include <nrf71_coex_if.h>

LOG_MODULE_DECLARE(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

/*
 * CCCONF priority field: which PTI slot each COEXC client occupies. Wi-Fi uses
 * four clients (high/low priority Rx and Tx), SR uses two, and two more are
 * reserved for an external radio.
 */
#define WLAN_HIGH_PTI_RX_CCCONF_PTI 1U
#define WLAN_HIGH_PTI_TX_CCCONF_PTI 2U
#define WLAN_LOW_PTI_RX_CCCONF_PTI  4U
#define WLAN_LOW_PTI_TX_CCCONF_PTI  5U
#define SR_RX_CCCONF_PTI            2U
#define SR_TX_CCCONF_PTI            3U
#define EXT_RX_CCCONF_PTI           1U
#define EXT_TX_CCCONF_PTI           1U

/* Client turnaround (revoke) time in microseconds, and converted to COEXC clock ticks. */
#define TURNAROUND_WIFI_RX_IN_US    1U
#define TURNAROUND_WIFI_TX_IN_US    1U
#define TURNAROUND_SR_RX_IN_US      1U
#define TURNAROUND_SR_TX_IN_US      1U
#define TURNAROUND_EXT_RX_IN_US     1U
#define TURNAROUND_EXT_TX_IN_US     1U
        
#define TURNAROUND_WIFI_RX          (TURNAROUND_WIFI_RX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_WIFI_TX          (TURNAROUND_WIFI_TX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_SR_RX            (TURNAROUND_SR_RX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_SR_TX            (TURNAROUND_SR_TX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_EXT_RX           (TURNAROUND_EXT_RX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_EXT_TX           (TURNAROUND_EXT_TX_IN_US * COEXC_CLK_FREQ_MHZ)

/*
 * CCMALLOW client0/mode0 acts as a fingerprint of the programmed antenna
 * layout. cd_coexc_verify_configured() reads back this single register instead
 * of all COEXC_NUM_CLIENTS x COEXC_NUM_MODES entries.
 */
#define SHA_CT0_MOD0                0x00010000UL
#define SEP_CT0_MOD0                0x00011111UL

#define COEXC_OFFSET_BETWEEN_CLIENT_REGS COEXC_OFFSET_BETWEEN_MODES

/* Write one COEXC register on the RPU via the Wi-Fi FMAC transport. */
static int cd_coexc_reg_write(uint32_t abs_addr, uint32_t value)
{
    int ret = nrf71_wifi_coex_reg_write(abs_addr, value);

    if (ret != 0) {
        LOG_ERR("COEXC write 0x%08x = 0x%08x failed (%d)", abs_addr, value, ret);
    }

    return ret;
}

/* Read one COEXC register from the RPU via the Wi-Fi FMAC transport. */
static int cd_coexc_reg_read(uint32_t abs_addr, uint32_t *value)
{
    if (value == NULL) {
        return -EINVAL;
    }

    return nrf71_wifi_coex_reg_read(abs_addr, value);
}

/**
 * Program COEXC CCMALLOW, CCCONF, and TURNAROUND values for the selected
 * shared or separate antenna configuration.
 *
 * Any failing register access aborts the sequence and is reported to the
 * caller, so a partially programmed COEXC block is never mistaken for a
 * configured one.
 *
 * @retval 0        All three register blocks were written.
 * @retval -EINVAL  Unknown antenna configuration.
 * @retval -EACCES  Wi-Fi FMAC transport to the RPU is not up yet.
 * @retval -EIO     A register write failed (propagated from the transport).
 */
int cd_coexc_configuration(enum coex_antenna_cfg_type antenna_cfg_type)
{
    uint32_t ccconf_config[COEXC_NUM_CLIENTS];
    /* Turnaround register values per COEXC client */
    const uint32_t turnaround_config[COEXC_NUM_CLIENTS] = {
        TURNAROUND_WIFI_RX, TURNAROUND_WIFI_TX, TURNAROUND_WIFI_RX, TURNAROUND_WIFI_TX,
        TURNAROUND_SR_RX, TURNAROUND_SR_TX, TURNAROUND_EXT_RX, TURNAROUND_EXT_TX,
    };

    /* CCCONF mode field. Only Wi-Fi is band dependent; SR and external use 0. */
    enum coexc_mode_wifi_t coexc_mode_wifi = COEXC_MODE_WIFI_2PT4G;
    unsigned int coexc_mode_sr = 0U;
    unsigned int coexc_mode_ext = 0U;

    /*
     * Precomputed CCMALLOW tables for the separate antenna configuration.
     * One row per COEXC client, one column per operating mode.
     */
    const uint32_t ccmallow_sep_ant[COEXC_NUM_CLIENTS][COEXC_NUM_MODES] = {
        {SEP_CT0_MOD0, 0x00222222UL, 0x00444444UL, 0x00000000UL}, 
        {0x00001111UL, 0x00202222UL, 0x00404444UL, 0x00000000UL}, 
        {0x00011111UL, 0x00222222UL, 0x00444444UL, 0x00000000UL}, 
        {0x00101111UL, 0x00202222UL, 0x00404444UL, 0x00000000UL}, 
        {0x00110101UL, 0x00220202UL, 0x00440404UL, 0x00000000UL}, 
        {0x00111000UL, 0x00222222UL, 0x44000000UL, 0x00000000UL}, 
        {0x11000000UL, 0x22000000UL, 0x44000000UL, 0x00000000UL}, 
        {0x11000000UL, 0x22000000UL, 0x44000000UL, 0x00000000UL},
    }; 
 
    /* Same layout for shared-antenna configuration. */
    const uint32_t ccmallow_sha_ant[COEXC_NUM_CLIENTS][COEXC_NUM_MODES] = {
        {SHA_CT0_MOD0, 0x00022222UL, 0x00044444UL, 0x00000000UL}, 
        {0x00000000UL, 0x00202222UL, 0x00404444UL, 0x00000000UL}, 
        {0x00010000UL, 0x00022222UL, 0x00044444UL, 0x00000000UL}, 
        {0x00000000UL, 0x00202222UL, 0x00404444UL, 0x00000000UL}, 
        {0x00110101UL, 0x00220202UL, 0x00440404UL, 0x00000000UL}, 
        {0x00110000UL, 0x00222020UL, 0x44000000UL, 0x00000000UL}, 
        {0x11000000UL, 0x22000000UL, 0x44000000UL, 0x00000000UL}, 
        {0x11000000UL, 0x22000000UL, 0x44000000UL, 0x00000000UL}, 
    };

    const uint32_t (*p_ccmallow)[COEXC_NUM_MODES];
    int ret;

    if ((antenna_cfg_type != COEX_SEPARATE_ANT_CFG) &&
        (antenna_cfg_type != COEX_SHARED_ANT_CFG)) {
        return -EINVAL;
    }

    /* Cannot program COEXC until RPU is up. */
    if (!nrf71_wifi_coex_is_ready()) {
        return -EACCES;
    }

    p_ccmallow = (antenna_cfg_type == COEX_SEPARATE_ANT_CFG) ? ccmallow_sep_ant :
                                 ccmallow_sha_ant;

    LOG_INF("Configuring COEXC (CCMALLOW, CCCONF, TURNAROUND)");

    /*  Block 1: addressed as base + client stride + mode stride. */
    for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
        const uint32_t base_client_offset = client * COEXC_OFFSET_BETWEEN_CLIENTS;

        for (uint32_t mode = 0U; mode < COEXC_NUM_MODES; mode++) {
            const uint32_t offset = (COEXC_OFFSET_BETWEEN_MODES * mode) +
                base_client_offset;

            ret = cd_coexc_reg_write(ABS_COEXC_CCMALLOW_0_MODE_0 + offset,
                         p_ccmallow[client][mode]);
            if (ret != 0) {
                return ret;
            }
        }
    }

    /* Block 2: CCCONF, pack priority and mode per client. */
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
                     (COEXC_OFFSET_BETWEEN_CLIENT_REGS * client),
                     ccconf_config[client]);
        if (ret != 0) {
            return ret;
        }
    }

    /* Block 3: TURNAROUND, one register per client. */
    for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
        ret = cd_coexc_reg_write(ABS_COEXC_TURNAROUND_0 +
                     (COEXC_OFFSET_BETWEEN_CLIENT_REGS * client),
                     turnaround_config[client]);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

/**
 * Verify COEXC CCMALLOW client0/mode0.
 *
 * Sanity check used by cd_ensure_coexc_configured() to skip redundant programming.
 * Read back a single known register instead of all 32 CCMALLOW entries. 
 * If it matches the expected fingerprint for shared or separate antenna config, 
 * assume COEXC was programmed correctly.
 * @param antenna_cfg_type: Antenna configuration type.
 * @param ccmallow_client0_mode0 Out: value read back on success.
 *
 * @retval 0        Read-back matches the expected fingerprint.
 * @retval -EINVAL  @p ccmallow_client0_mode0 is NULL.
 * @retval -EACCES  Wi-Fi FMAC transport to the RPU is not up yet.
 * @retval -EIO     Read-back does not match the expected value.
 */
int cd_coexc_verify_configured(enum coex_antenna_cfg_type antenna_cfg_type,
                   uint32_t *ccmallow_client0_mode0)
{
    uint32_t expected;
    uint32_t readback;
    int ret;

    if (ccmallow_client0_mode0 == NULL) {
        return -EINVAL;
    }

    if (!nrf71_wifi_coex_is_ready()) {
        return -EACCES;
    }

    expected = (antenna_cfg_type == COEX_SEPARATE_ANT_CFG) ? SEP_CT0_MOD0 : SHA_CT0_MOD0;

    ret = cd_coexc_reg_read(ABS_COEXC_CCMALLOW_0_MODE_0, &readback);
    if (ret != 0) {
        return ret;
    }

    if (readback != expected) {
        LOG_ERR("CCMALLOW verify mismatch: got 0x%08x expected 0x%08x", readback,
            expected);
        return -EIO;
    }

    *ccmallow_client0_mode0 = readback;
    return 0;
}
