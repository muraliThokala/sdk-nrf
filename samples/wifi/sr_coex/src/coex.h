/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef COEX_H_
#define COEX_H_

enum{
	SEA1 = 1,
	SEA2,
	SEA3,
	SEA4,
	SEA5
};

enum{
	SHA1 = 1,
	SHA2,
	SHA3,
	SHA4,
	SHA5
};

int nrf_wifi_coex_hw_enable(uint32_t coexHwEn);
int nrf_wifi_coex_enable(uint32_t wifiCoexEn);
int nrf_wifi_coex_config_non_pta(uint32_t coexEnable);
int nrf_wifi_coex_config_pta(uint32_t ptaTblType);
int nrf_wifi_coex_allocate_spw(uint32_t deviceRequestingWindow,
	uint32_t windowStartOrEnd,
	uint32_t importanceOfRequest,
	uint32_t canBeDeferred);
int nrf_wifi_coex_allocate_ppw(int32_t startOrStop,
	uint32_t firstPriorityWindow,
	uint32_t powersaveMechanism,
	uint32_t wlanWindowDuration,
	uint32_t srWindowDuration);

#endif /* COEX_H_ */
