/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief nRF71 Wi-Fi coexistence transport.
 *
 * Thin, stable wrapper that lets the host-side Coexistence Driver forward
 * CD2CM command buffers to the Coexistence Manager (running in RPU firmware)
 * over the Wi-Fi FMAC control path, without depending on the Wi-Fi driver's
 * private FMAC context layout.
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr/logging/log.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>

#include <system/main.h>
#include <common/fmac_api_common.h>
#include <common/rpu_if.h>
#include <common/fmac_structs_common.h>
#include <common/hal_structs_common.h>
#include <bal_api.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

static nrf71_wifi_coex_event_cb_t coex_event_cb;
static void *coex_event_cb_ctx;

static struct nrf_wifi_ctx_zep *coex_rpu_ctx(void)
{
	return &rpu_drv_priv_zep.rpu_ctx_zep;
}

static uint32_t coex_abs_to_host_addr(uint32_t abs_rpu_addr)
{
	uint32_t region = abs_rpu_addr & 0xFF000000U;

	if (region == 0x40000000U) {
		return RPU_ADDR_PBUS_START + (abs_rpu_addr - 0x40000000U);
	}

	if (region == 0x48000000U) {
		return RPU_ADDR_PBUS_START + (abs_rpu_addr - 0x48000000U);
	}

	return abs_rpu_addr;
}

static int coex_hal_reg_access(uint32_t abs_rpu_addr, uint32_t *value, bool write)
{
	struct nrf_wifi_ctx_zep *rpu_ctx = coex_rpu_ctx();
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx;
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx;
	uint32_t host_addr;

	if (value == NULL) {
		return -EINVAL;
	}

	if ((rpu_ctx == NULL) || (rpu_ctx->rpu_ctx == NULL)) {
		return -ENODEV;
	}

	fmac_dev_ctx = rpu_ctx->rpu_ctx;
	hal_dev_ctx = fmac_dev_ctx->hal_dev_ctx;
	if ((hal_dev_ctx == NULL) || (hal_dev_ctx->bal_dev_ctx == NULL)) {
		return -ENODEV;
	}

	host_addr = coex_abs_to_host_addr(abs_rpu_addr);

	if (write) {
		nrf_wifi_bal_write_word(hal_dev_ctx->bal_dev_ctx, host_addr, *value);
	} else {
		*value = nrf_wifi_bal_read_word(hal_dev_ctx->bal_dev_ctx, host_addr);
	}

	return 0;
}

bool nrf71_wifi_coex_is_ready(void)
{
	struct nrf_wifi_ctx_zep *rpu_ctx = coex_rpu_ctx();

	return (rpu_ctx != NULL) && (rpu_ctx->rpu_ctx != NULL);
}

int nrf71_wifi_coex_cmd_send(const void *cmd, size_t len)
{
	struct nrf_wifi_ctx_zep *rpu_ctx = coex_rpu_ctx();
	enum nrf_wifi_status status;

	if ((cmd == NULL) || (len == 0)) {
		return -EINVAL;
	}

	if (!nrf71_wifi_coex_is_ready()) {
		LOG_DBG("Coex transport not ready, dropping command");
		return -ENODEV;
	}

	/* nrf_wifi_fmac_conf_srcoex() takes a non-const buffer; the FMAC layer
	 * copies it into the command message and does not modify the caller's
	 * data.
	 */
	status = nrf_wifi_fmac_conf_srcoex(rpu_ctx->rpu_ctx, (void *)cmd, (unsigned int)len);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("Failed to send coex command to RPU");
		return -EIO;
	}

	return 0;
}

int nrf71_wifi_coex_register_event_cb(nrf71_wifi_coex_event_cb_t cb, void *ctx)
{
	coex_event_cb = cb;
	coex_event_cb_ctx = ctx;

	return 0;
}

void nrf71_wifi_coex_on_event(const void *event, size_t len)
{
	if (coex_event_cb != NULL) {
		coex_event_cb(coex_event_cb_ctx, event, len);
	} else {
		LOG_DBG("Coex event dropped: no handler registered");
	}
}

int nrf71_wifi_coex_reg_write(uint32_t abs_rpu_addr, uint32_t value)
{
	if (!nrf71_wifi_coex_is_ready()) {
		return -ENODEV;
	}

	return coex_hal_reg_access(abs_rpu_addr, &value, true);
}

int nrf71_wifi_coex_reg_read(uint32_t abs_rpu_addr, uint32_t *value)
{
	if (value == NULL) {
		return -EINVAL;
	}

	if (!nrf71_wifi_coex_is_ready()) {
		return -ENODEV;
	}

	return coex_hal_reg_access(abs_rpu_addr, value, false);
}
