/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Test bench utilities: transport readiness and timing helpers.
 *
 * The test bench never waits for CM2CD events itself, because the coexistence
 * driver already does that inside every coex_cd_* call. The waiting done here is
 * for the Wi-Fi transport to come up at startup, for the RPU to settle, and for
 * optional CM bring-up before CM-dependent test steps run.
 */

#ifndef COEX_TB_UTIL_H__
#define COEX_TB_UTIL_H__

#include <stdint.h>

/**
 * Print the Wi-Fi core bring-up state: WICR contents against the values
 * devicetree expects, and the ipc0 shared-memory regions.
 *
 * Diagnostic for a transport that never comes up. The Wi-Fi driver's
 * "Firmware booted successfully" line reports a stubbed constant and proves
 * nothing, so this reads the two things that do carry evidence: whether the
 * WICR words (LMAC/UMAC entry points and IPC mailbox descriptors) actually
 * took, and whether the ICMsg magic appeared in each direction. Magic in the
 * host TX region but not in RX means the Wi-Fi core never ran its IPC init.
 *
 * Safe to call before the transport is up; it only reads memory.
 */
void coex_tb_dump_wifi_bringup_state(void);

/**
 * Block until the Wi-Fi FMAC coexistence transport is ready.
 *
 * Coexistence commands travel to the RPU over the Wi-Fi driver's control path,
 * so nothing can be tested until the Wi-Fi driver has booted the RPU. Requires
 * several consecutive successful readiness polls before returning.
 *
 * @param timeout_ms Give up after this long.
 *
 * @retval 0           Transport is ready.
 * @retval -ETIMEDOUT  Still not ready after @p timeout_ms.
 */
int coex_tb_wait_transport_ready(uint32_t timeout_ms);

/**
 * Prepare the test bench environment before running test steps.
 *
 * Waits for a stable transport, pauses for RPU/VIF settle time, and runs CM
 * bring-up via coex_cd_wifi_power_notify() when a CM-dependent test is enabled
 * and COEX_TB_TEST_INIT_AND_ENABLE is not.
 *
 * @retval 0           Preparation succeeded.
 * @retval -ETIMEDOUT  Transport not ready in time.
 * @retval -EIO        CM bring-up failed.
 */
int coex_tb_prepare(void);

/** Sleep between test steps, letting the CM act on what was just sent. */
void coex_tb_wait_ms(uint32_t wait_ms);

/**
 * Human-readable description of a return code.
 *
 * Negative values are interpreted as errno codes (for example -22 → "EINVAL").
 * Zero returns "success". Positive values are described as a Bluetooth or
 * driver-specific status code.
 */
const char *coex_tb_errstr(int err);

#endif /* COEX_TB_UTIL_H__ */
