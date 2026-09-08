/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Test bench utilities.
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
#include <nrf71_coex_if.h>

#include "coex_tb.h"
#include "coex_tb_util.h"

const char *coex_tb_errstr(int err)
{
	if (err == 0) {
		return "success";
	}

	if (err < 0) {
		return strerror(-err);
	}

	return "positive status code";
}

#define WICR_NODE DT_NODELABEL(wicr)
#define IPC_NODE  DT_NODELABEL(ipc0)

#define WICR_REG(off) (*(volatile uint32_t *)(DT_REG_ADDR(WICR_NODE) + (off)))

/* Must match the magic in subsys/ipc/ipc_service/lib/icmsg.c: each side writes
 * it into its own TX region during endpoint setup and treats the endpoint as
 * bound once it shows up in the RX region.
 */
static const uint8_t icmsg_magic[] = {0x45, 0x6d, 0x31, 0x6c, 0x31, 0x4b,
				      0x30, 0x72, 0x6e, 0x33, 0x6c, 0x69, 0x34};

/* Wide enough to cover the pbuf header that precedes the magic in shared RAM. */
#define ICMSG_MAGIC_SEARCH_LEN 64U
#define REGION_DUMP_LEN        32U

struct wicr_expect {
	const char *name;
	uint16_t offset;
	uint32_t expect;
};

/* Offsets and expected values mirror wicr_words[] in soc/nordic/nrf71/wicr_setup.c. */
static const struct wicr_expect wicr_expected[] = {
	{"LMACINITPC", 0x000, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, firmware_lmacinitpc))},
	{"UMACINITPC", 0x004, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, firmware_umacinitpc))},
	{"LMACPATCH", 0x008, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, firmware_lmacrompatchaddr))},
	{"UMACPATCH", 0x00C, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, firmware_umacrompatchaddr))},
	{"CMDMBOX addr", 0x080, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, ipcconfig_commandmbox))},
	{"CMDMBOX size", 0x084, DT_REG_SIZE(DT_PHANDLE(WICR_NODE, ipcconfig_commandmbox))},
	{"EVTMBOX addr", 0x088, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, ipcconfig_eventmbox))},
	{"EVTMBOX size", 0x08C, DT_REG_SIZE(DT_PHANDLE(WICR_NODE, ipcconfig_eventmbox))},
	{"SPARE addr", 0x090, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, ipcconfig_sparembox))},
	{"SPARE size", 0x094, DT_REG_SIZE(DT_PHANDLE(WICR_NODE, ipcconfig_sparembox))},
};

static bool region_has_icmsg_magic(uintptr_t base)
{
	const volatile uint8_t *region = (const volatile uint8_t *)base;

	for (size_t off = 0U; off + sizeof(icmsg_magic) <= ICMSG_MAGIC_SEARCH_LEN; off++) {
		size_t i;

		for (i = 0U; i < sizeof(icmsg_magic); i++) {
			if (region[off + i] != icmsg_magic[i]) {
				break;
			}
		}

		if (i == sizeof(icmsg_magic)) {
			return true;
		}
	}

	return false;
}

static void dump_region(const char *name, uintptr_t base)
{
	const volatile uint8_t *region = (const volatile uint8_t *)base;

	printk("Coex TB: %s @ 0x%08lx:", name, (unsigned long)base);

	for (size_t i = 0U; i < REGION_DUMP_LEN; i++) {
		printk(" %02x", region[i]);
	}

	printk("\n");
}

void coex_tb_dump_wifi_bringup_state(void)
{
	uintptr_t tx = DT_REG_ADDR(DT_PHANDLE(IPC_NODE, tx_region));
	uintptr_t rx = DT_REG_ADDR(DT_PHANDLE(IPC_NODE, rx_region));

	printk("\n=================== Wi-Fi bring-up state\n");

	for (size_t i = 0U; i < ARRAY_SIZE(wicr_expected); i++) {
		uint32_t actual = WICR_REG(wicr_expected[i].offset);

		printk("Coex TB: WICR 0x%03x %-12s = 0x%08x (expected 0x%08x)%s\n",
		       (unsigned int)wicr_expected[i].offset, wicr_expected[i].name, actual,
		       wicr_expected[i].expect,
		       (actual == wicr_expected[i].expect) ? "" : "   <-- MISMATCH");
	}

	dump_region("ipc0 tx host->UMAC", tx);
	dump_region("ipc0 rx UMAC->host", rx);

	printk("Coex TB: ICMsg magic present: host tx %s, host rx %s\n",
	       region_has_icmsg_magic(tx) ? "yes" : "no",
	       region_has_icmsg_magic(rx) ? "yes" : "no");
}

/** Poll until the Wi-Fi FMAC coexistence transport reports ready. */
int coex_tb_wait_transport_ready(uint32_t timeout_ms)
{
	uint32_t elapsed_ms = 0U;
	unsigned int stable_polls = 0U;

	while (stable_polls < COEX_TB_TRANSPORT_STABLE_POLLS) {
		if (elapsed_ms >= timeout_ms) {
			printk("Coex TB: transport not ready after %u ms (%s)\n", timeout_ms,
			       coex_tb_errstr(-ETIMEDOUT));
			return -ETIMEDOUT;
		}

		if (nrf71_wifi_coex_is_ready()) {
			stable_polls++;
		} else {
			stable_polls = 0U;
		}

		k_msleep(COEX_TB_POLL_INTERVAL_MS);
		elapsed_ms += COEX_TB_POLL_INTERVAL_MS;
	}

	printk("Coex TB: transport stable after %u ms\n", elapsed_ms);

	return 0;
}

static int coex_tb_wait_net_if_up(uint32_t timeout_ms)
{
	uint32_t elapsed_ms = 0U;

	while (elapsed_ms < timeout_ms) {
		struct net_if *iface = net_if_get_default();

		if ((iface != NULL) && net_if_is_up(iface)) {
			printk("Coex TB: network interface up after %u ms\n", elapsed_ms);
			return 0;
		}

		k_msleep(COEX_TB_POLL_INTERVAL_MS);
		elapsed_ms += COEX_TB_POLL_INTERVAL_MS;
	}

	printk("Coex TB: network interface not up after %u ms (%s)\n", timeout_ms,
	       coex_tb_errstr(-ETIMEDOUT));

	return -ETIMEDOUT;
}

static int coex_tb_wait_wifi_cm_up(uint32_t timeout_ms)
{
	uint32_t elapsed_ms = 0U;

	while (elapsed_ms < timeout_ms) {
		if (coex_cd_wifi_is_up()) {
			printk("Coex TB: CM bring-up complete after %u ms\n", elapsed_ms);
			return 0;
		}

		k_msleep(COEX_TB_POLL_INTERVAL_MS);
		elapsed_ms += COEX_TB_POLL_INTERVAL_MS;
	}

	return -ETIMEDOUT;
}

/**
 * Wait for transport, let the RPU/VIF settle, and ensure CM bring-up completed.
 */
int coex_tb_prepare(void)
{
	int ret;

	ret = coex_tb_wait_transport_ready(COEX_TB_TRANSPORT_TIMEOUT_MS);
	if (ret != 0) {
		return ret;
	}

	ret = coex_tb_wait_net_if_up(COEX_TB_TRANSPORT_TIMEOUT_MS);
	if (ret != 0) {
		return ret;
	}

	printk("Coex TB: waiting %u ms for RPU/VIF settle\n", COEX_TB_RPU_SETTLE_MS);
	k_msleep(COEX_TB_RPU_SETTLE_MS);

#if defined(COEX_TB_NEEDS_CM_BRINGUP) && !defined(COEX_TB_TEST_INIT_AND_ENABLE)
	ret = coex_tb_wait_wifi_cm_up(COEX_TB_CM_BRINGUP_TIMEOUT_MS);
	if (ret != 0) {
		printk("Coex TB: Wi-Fi driver CM bring-up not done; posting power notify\n");
		ret = coex_cd_wifi_power_notify(COEX_WIFI_POWERED_UP_READY);
		if (ret != 0) {
			printk("Coex TB: Wi-Fi power notify failed: %s (%d)\n", coex_tb_errstr(ret),
			       ret);
			return ret;
		}
	}

	if (!coex_cd_wifi_is_up()) {
		printk("Coex TB: CM bring-up finished but wifi_up is still false (%s)\n",
		       coex_tb_errstr(-EIO));
		return -EIO;
	}
#endif

	return 0;
}

/** Sleep for the requested interval between test steps. */
void coex_tb_wait_ms(uint32_t wait_ms)
{
	k_msleep(wait_ms);
}
