/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Test bench utilities: transport readiness and timing helpers.
 *
 * The test bench never waits for CM2CD events itself, because the coexistence
 * driver already does that inside every coex_cd_* call. The only waiting done
 * here is for the Wi-Fi transport to come up at startup, plus plain sleeps that
 * give the CM time to act between commands.
 */

#ifndef COEX_TB_UTIL_H__
#define COEX_TB_UTIL_H__

#include <stdint.h>

/**
 * Block until the Wi-Fi FMAC coexistence transport is ready.
 *
 * Coexistence commands travel to the RPU over the Wi-Fi driver's control path,
 * so nothing can be tested until the Wi-Fi driver has booted the RPU. This
 * polls rather than using a callback because the transport exposes only a
 * readiness query.
 *
 * @param timeout_ms Give up after this long.
 *
 * @retval 0           Transport is ready.
 * @retval -ETIMEDOUT  Still not ready after @p timeout_ms.
 */
int coex_tb_wait_transport_ready(uint32_t timeout_ms);

/** Sleep between test steps, letting the CM act on what was just sent. */
void coex_tb_wait_ms(uint32_t wait_ms);

#endif /* COEX_TB_UTIL_H__ */
