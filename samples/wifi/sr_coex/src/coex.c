/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/sys/printk.h>

#include "coex.h"
#include "cm_struct.h"
#include "uccp530_77_registers.h"
#include "uccp530_77_registers_ext_sys_bus.h"

#include <zephyr_util.h>
#include <zephyr_fmac_main.h>

	/* #include <nrf_wifi_radio_test_shell.h> */
	extern struct wifi_nrf_drv_priv_zep rpu_drv_priv_zep;
	struct wifi_nrf_ctx_zep *ctx = &rpu_drv_priv_zep.rpu_ctx_zep;

	#define CH_BASE_ADDRESS ABS_EXT_SYS_WLANSYSCOEX_CH_CONTROL
	#define WLAN_MAC_CTRL_BASE_ADDRESS ABS_PMB_WLAN_MAC_CTRL_PULSED_SOFTWARE_RESET
	#define COEX_CONFIG_FIXED_PARAMS 4
	#define COEX_MAX_WINDOW_MUL_100US 4095
	#define COEX_WLAN_MAC_CTRL_CLK_FREQ 0x50
	#define COEX_REG_CFG_OFFSET_SHIFT 24

	/* #define COEX_DEBUG_ENABLE 1 */


	/* separate antenna configuration tables */

	/* 2.4GHz WLAN and SR overlap as default, to configure NO_WINDOW lookup */
	uint16_t configBuffer_SEA1[] = {
		0x19, 0xF6, 0x8, 0x62, 0xF5,
		0xF5, 0x19, 0x19, 0x74, 0x74,
		0x8, 0x1E2, 0xD5, 0xD5, 0x1F6,
		0x1F6, 0x61, 0x61, 0x1E2, 0x8,
		0x1F6, 0x1F6, 0x19, 0x19, 0x1E2,
		0x8, 0xF5, 0xF5, 0xD5, 0xD5,
		0x8, 0x1E2, 0x51, 0x51, 0x74,
		0x74, 0xF6, 0x19, 0x62, 0x19,
		0xF6, 0x8, 0x62, 0x8, 0x1A
	};

	/* 2.4GHz WLAN and SR non-overlap, to configure NO_WINDOW lookup */
	uint16_t configBuffer_SEA2[] = {
		0x19, 0xF6, 0x8, 0x62, 0xF5,
		0xF5, 0x19, 0x19, 0x74, 0x1F6,
		0x60, 0x60, 0xD5, 0xD5, 0x1F6,
		0x1F6, 0x61, 0x19, 0x60, 0x60,
		0x1F6, 0x1F6, 0x19, 0x19, 0x60,
		0x60, 0xF5, 0xF5, 0xD5, 0xD5,
		0x60, 0x60, 0x61, 0x19, 0x74,
		0x1F6, 0xF6, 0x19, 0xE2, 0x19,
		0xF6, 0x8, 0xE2, 0x8, 0x1A
	};

	/* SR window */
	uint16_t configBuffer_SEA3[] = {
		0x1F8, 0x1E0, 0x1F4, 0x1E0, 0x1F4
	};

	/* WLAN window */
	uint16_t configBuffer_SEA4[] = {
		0x12, 0x0, 0x11, 0x0, 0x11
	};

	/* WLAN in 5GHz, both windows, to configure NO_WINDOW lookup */
	uint16_t configBuffer_SEA5[] = {
		0x19, 0x76, 0x8, 0x62, 0x75,
		0x75, 0x61, 0x61, 0x74, 0x74,
		0x60, 0x60, 0x55, 0x55, 0x44,
		0x44, 0x51, 0x51, 0x40, 0x40,
		0x44, 0x44, 0x61, 0x61, 0x40,
		0x40, 0x75, 0x75, 0x55, 0x55,
		0x60, 0x60, 0x51, 0x51, 0x74,
		0x74, 0x76, 0x19, 0x62, 0x19,
		0x76, 0x8, 0x62, 0x8, 0x1A
	};


/* Shared antenna configuration tables */
/* Request posted before the transaction, to configure NO_WINDOW lookup */
uint16_t configBuffer_SHA1[] = {
	0x19, 0xF6, 0x8, 0xE2, 0x15,
	0xF5, 0x19, 0x19, 0x4, 0x1F6,
	0x8, 0x1E2, 0xF5, 0xF5, 0x1F6,
	0x1F6, 0xE1, 0xE1, 0x1E2, 0x8,
	0x1F6, 0x1F6, 0x19, 0x19, 0x1E2,
	0x8, 0x15, 0xF5, 0xF5, 0xF5,
	0x8, 0x1E2, 0xE1, 0xE1, 0x4,
	0x1F6, 0xF6, 0x19, 0xE2, 0x19,
	0xF6, 0x8, 0xE2, 0x8, 0x1A
};

/* Request posted during the transaction, to configure NO_WINDOW lookup */
uint16_t configBuffer_SHA2[] = {
	0x19, 0x16, 0x8, 0xE2, 0x15,
	0x15, 0x19, 0x19, 0x4, 0x4,
	0x8, 0x1E2, 0xF5, 0xF5, 0x1F6,
	0x1F6, 0xE1, 0xE1, 0x1E2, 0x8,
	0x1F6, 0x1F6, 0x19, 0x19, 0x1E2,
	0x8, 0x15, 0x15, 0xF5, 0xF5,
	0x8, 0x1E2, 0xE1, 0xE1, 0x4,
	0x4, 0xF6, 0x19, 0xE2, 0x19,
	0xF6, 0x8, 0xE2, 0x8, 0x1A
};

/* SR window */
uint16_t configBuffer_SHA3[] = {
	0x1F8, 0x1E0, 0x1F4, 0x1E0, 0x1F4
};

/* WLAN window */
uint16_t configBuffer_SHA4[] = {
	0x12, 0x0, 0x11, 0x0, 0x11
};

/* WLAN in 5GHz, both windows, to configure NO_WINDOW lookup */
uint16_t configBuffer_SHA5[] = {
	0x19, 0x76, 0x8,  0x62, 0x75,
	0x75, 0x61, 0x61, 0x74, 0x74,
	0x60, 0x60, 0x55, 0x55, 0x44,
	0x44, 0x51, 0x51, 0x40, 0x40,
	0x44, 0x44, 0x61, 0x61, 0x40,
	0x40, 0x75, 0x75, 0x55, 0x55,
	0x60, 0x60, 0x51, 0x51, 0x74,
	0x74, 0x76, 0x19, 0x62, 0x19,
	0x76, 0x8,  0x62, 0x8,  0x1A
};

/* non-PTA register configuration of coexistence hardware */
/* Shared antenna,  coex disabled */
uint32_t ch_config_sha_cd[] = {
	0x00000028, 0x00000000, 0x001e1023, 0x00000000, 0x00000000,
	0x00000000, 0x00000021, 0x000002ca, 0x00000055, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000
};

/* Shared antenna, coex enabled */
uint32_t ch_config_sha_ce[] = {
	0x00000028, 0x00000000, 0x001e1023, 0x00000000, 0x00000000,
	0x00000000, 0x00000021, 0x000002ca, 0x00000050, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000
};

/* Separate antennas */
uint32_t ch_config_sea[] = {
	0x00000028, 0x00000000, 0x001e1023, 0x00000000, 0x00000000,
	0x00000000, 0x00000021, 0x000002ca, 0x00000055, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000
};

/* coexistence hardware enable/disable */
/* coexHwEn: [0 - Disable  1 - Enable] */
int nrf_wifi_coex_hw_enable(uint32_t coexHwEn)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct coex_ch_configuration params  = { 0 };
	uint32_t coexHwEnable = coexHwEn;
	uint32_t numReg2Config = 1;
	uint32_t startOffset = 0;
	uint32_t index = 0;

	if (coexHwEnable > 1) {
		printk("Invalid coex enable config (%d)\n", coexHwEnable);
		return -ENOEXEC;
	}

	params.messageID = HW_CONFIGURATION;
	params.numOfRegistersToConfigure = numReg2Config;
	params.hwToConfig = COEX_HARDWARE;
	params.hwBlockBaseAddress = CH_BASE_ADDRESS;

	startOffset = ((EXT_SYS_WLANSYSCOEX_CH_CONTROL - EXT_SYS_WLANSYSCOEX_CH_CONTROL) >> 2);
	params.configbuf[index] = (startOffset << COEX_REG_CFG_OFFSET_SHIFT) |
	(coexHwEnable << EXT_SYS_WLANSYSCOEX_ENABLE_SHIFT);

	/* uint32_t cmd_len = sizeof(struct coex_ch_configuration); */
	uint32_t cmd_len = (COEX_CONFIG_FIXED_PARAMS + numReg2Config)*sizeof(uint32_t);

	status = wifi_nrf_fmac_conf_btcoex(ctx->rpu_ctx, (void *)(&params), cmd_len);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		printk("Coex: CH enable configuration failed\n");
		return -ENOEXEC;
	}

	return 0;
}




/* wifiCoexEn: [0 - Disable  1 - Enable] */
int nrf_wifi_coex_enable(uint32_t wifiCoexEn)
{

	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct coex_ch_configuration params  = { 0 };
	uint32_t coexEnable = wifiCoexEn;
	uint32_t numReg2Config = 1;

	if (coexEnable > 1) {
		printk("Invalid WLAN coex enable config (%d)\n", coexEnable);
		return -ENOEXEC;
	}

	/* ABS_PMB_WLAN_MAC_CTRL_PTA_CONTROL */
	/* parameters for coex enable as default */
	uint32_t regValue = 0;
	uint32_t phyRxReqEnable = 1;
	uint32_t macRxReqEnable = 1;
	uint32_t macTxReqEnable = 1;

	if (coexEnable == 0) {
		phyRxReqEnable = 0;
		macRxReqEnable = 0;
		macTxReqEnable = 0;
	}

	uint32_t ptaIfCtrl = 0;
	uint32_t configValue = 0;

	numReg2Config = 1;

	/*regValue = UCC_READ_PERIP(ABS_PMB_WLAN_MAC_CTRL_PTA_CONTROL); */
	regValue = COEX_WLAN_MAC_CTRL_CLK_FREQ << PMB_WLAN_MAC_CTRL_CLK_FREQ_MHZ_SHIFT;
	configValue = regValue & (~PMB_WLAN_MAC_CTRL_PTA_CONTROL_REG_MASK);
	ptaIfCtrl = (phyRxReqEnable << 2)  | (macRxReqEnable << 1) | (macTxReqEnable << 0);
	ptaIfCtrl = (ptaIfCtrl << PMB_WLAN_MAC_CTRL_PTA_CONTROL_REG_SHIFT);
	ptaIfCtrl = configValue | ptaIfCtrl;

	uint32_t pta_control_offset = (ABS_PMB_WLAN_MAC_CTRL_PTA_CONTROL -
	WLAN_MAC_CTRL_BASE_ADDRESS) >> 2;

	params.messageID = HW_CONFIGURATION;
	params.numOfRegistersToConfigure = numReg2Config;
	params.hwToConfig = MAC_CTRL;
	params.hwBlockBaseAddress = WLAN_MAC_CTRL_BASE_ADDRESS;

	params.configbuf[0] = (pta_control_offset<<COEX_REG_CFG_OFFSET_SHIFT) | (ptaIfCtrl);

	/* cmd_len = sizeof(struct coex_ch_configuration); */
	uint32_t cmd_len = (COEX_CONFIG_FIXED_PARAMS + numReg2Config)*sizeof(uint32_t);

	status = wifi_nrf_fmac_conf_btcoex(ctx->rpu_ctx, (void *)(&params), cmd_len);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		printk("Coex: WLAN coex enable configuration failed\n");
		return -ENOEXEC;
	}

	return 0;
}


/* Coexistence hardware non-pta configuration. */
int nrf_wifi_coex_config_non_pta(uint32_t coexEnable)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct coex_ch_configuration params  = { 0 };
	uint32_t *configBufferPtr = NULL;
	uint32_t startOffset = 0;
	uint32_t numReg2Config = 1;

	/* Offset from the base address of CH */
	startOffset = ((EXT_SYS_WLANSYSCOEX_CH_TIME_REFERENCE -
	EXT_SYS_WLANSYSCOEX_CH_CONTROL) >> 2);
	/* Number of registers to be configured */
	numReg2Config = ((EXT_SYS_WLANSYSCOEX_CH_SR_INFO_STATUS -
	EXT_SYS_WLANSYSCOEX_CH_TIME_REFERENCE) >> 2) + 1;

	if (CONFIG_COEX_SEP_ANTENNAS) {
		configBufferPtr = ch_config_sea;
	} else {
		if (coexEnable) {
			configBufferPtr = ch_config_sha_ce;
		} else {
			configBufferPtr = ch_config_sha_cd;
		}
	}

	/* configuration buffer : offset and configValue packed */
	uint32_t index = 0;

	params.messageID = HW_CONFIGURATION;
	params.numOfRegistersToConfigure = numReg2Config;
	params.hwToConfig = COEX_HARDWARE;
	params.hwBlockBaseAddress = CH_BASE_ADDRESS;

	for (index = 0; index < numReg2Config; index++) {
		params.configbuf[index] = (startOffset << COEX_REG_CFG_OFFSET_SHIFT) |
			(*(configBufferPtr+index));
		startOffset++;
	}

	/* uint32_t cmd_len = sizeof(struct coex_ch_configuration); */
	uint32_t cmd_len = (COEX_CONFIG_FIXED_PARAMS + numReg2Config)*sizeof(uint32_t);

	status = wifi_nrf_fmac_conf_btcoex(ctx->rpu_ctx,
					   (void *)(&params), cmd_len);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		/* printk("Coex: non-PTA configuration failed\n"); */
		return -ENOEXEC;
	}

	return 0;
}



/**Coexistence PTA table configuration.
 * ptaTblType:
 * [SEA: 1 - Freq overlap  2 - Freq non-overlap  3 - SR Win  4 - WLAN Win  5 - Both Windows]
 * [SHA: 1 - Req before txn  2 - Req during txn  3 - SR Win  4 - WLAN Win  5 - Both Windows]
 */
int nrf_wifi_coex_config_pta(uint32_t ptaTblType)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct coex_ch_configuration params  = { 0 };
	uint32_t ptaTableType = ptaTblType;
	uint16_t *configBufferPtr = NULL;
	uint32_t startOffset = 0;
	uint32_t numReg2Config = 1;

	/* common for both SHA and SEA */
	if ((ptaTableType == SEA1) || (ptaTableType == SEA2) || (ptaTableType == SEA5) ||
		 (ptaTableType == SHA1) || (ptaTableType == SHA2) || (ptaTableType == SHA5)) {
		/* NO_WINDOW */
		/* Indicates offset from the base address of CH */
		startOffset = ((EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_0 -
		EXT_SYS_WLANSYSCOEX_CH_CONTROL) >> 2);
		/* Number of contiguous registers to be configured starting from base+offset */
		numReg2Config = ((EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_44 -
		EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_0) >> 2) + 1;
	} else {
		if ((ptaTableType == SEA3) || (ptaTableType == SHA3)) {
			/* SR window */
			/* Indicates offset from the base address of CH */
			startOffset = ((EXT_SYS_WLANSYSCOEX_CH_SR_WINDOW_LOOKUP_0 -
			EXT_SYS_WLANSYSCOEX_CH_CONTROL)>>2);
			/* Number ofregisters to be configured */
			numReg2Config = ((EXT_SYS_WLANSYSCOEX_CH_SR_WINDOW_LOOKUP_4 -
			EXT_SYS_WLANSYSCOEX_CH_SR_WINDOW_LOOKUP_0) >> 2) + 1;
		}

		if ((ptaTableType == SEA4) || (ptaTableType == SHA4)) {
			/* WLAN window */
			/* Indicates offset from the base address of CH */
			startOffset = ((EXT_SYS_WLANSYSCOEX_CH_WLAN_WINDOW_LOOKUP_0 -
			EXT_SYS_WLANSYSCOEX_CH_CONTROL) >> 2);
			/* Number ofregisters to be configured */
			numReg2Config = ((EXT_SYS_WLANSYSCOEX_CH_WLAN_WINDOW_LOOKUP_4 -
			EXT_SYS_WLANSYSCOEX_CH_WLAN_WINDOW_LOOKUP_0) >> 2) + 1;
		}
	}

	if (CONFIG_COEX_SEP_ANTENNAS) {
		/* separate antennas configuration */
		switch (ptaTableType) {
		case SEA1:
			configBufferPtr = configBuffer_SEA1;
			break;
		case SEA2:
			configBufferPtr = configBuffer_SEA2;
			break;
		case SEA3:
			configBufferPtr = configBuffer_SEA3;
			break;
		case SEA4:
			configBufferPtr = configBuffer_SEA4;
			break;
		case SEA5:
			configBufferPtr = configBuffer_SEA5;
			break;
		default:
			configBufferPtr = configBuffer_SEA1;
			break;
		}
	} else {
		/* Shared antenna configuration */
		switch (ptaTableType) {
		case SHA1:
			configBufferPtr = configBuffer_SHA1;
		break;
		case SHA2:
			configBufferPtr = configBuffer_SHA2;
		break;
		case SHA3:
			configBufferPtr = configBuffer_SHA3;
		break;
		case SHA4:
			configBufferPtr = configBuffer_SHA4;
		break;
		case SHA5:
			configBufferPtr = configBuffer_SHA5;
		break;
		default:
			configBufferPtr = configBuffer_SHA1;
		break;

		}
	}

	/* configuration buffer : offset and configValue packed */
	uint32_t index = 0;

	params.messageID = HW_CONFIGURATION;
	params.numOfRegistersToConfigure = numReg2Config;
	params.hwToConfig = COEX_HARDWARE;
	params.hwBlockBaseAddress = CH_BASE_ADDRESS;

	for (index = 0; index < numReg2Config; index++) {
		params.configbuf[index] = (startOffset << COEX_REG_CFG_OFFSET_SHIFT) |
			(*(configBufferPtr+index));
		startOffset++;
	}

	/* uint32_t cmd_len = sizeof(struct coex_ch_configuration); */
	uint32_t cmd_len = (COEX_CONFIG_FIXED_PARAMS + numReg2Config)*sizeof(uint32_t);

	status = wifi_nrf_fmac_conf_btcoex(ctx->rpu_ctx,
					   (void *)(&params), cmd_len);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		/* printk("Coex: PTA configuration failed\n"); */
		return -ENOEXEC;
	}

	return 0;
}


/*
 * [deviceRequestingWindow: 0 - SR   1 - WLAN]
 * [windowStartOrEnd:       0 - End  1 - Start]
 * [importanceOfRequest:    0 - Less 1 - Medium  2 - High  3 - Highest]
 * [canBeDeferred:          0 - No   1 - Yes]
 */

/*  External single priority window */
int nrf_wifi_coex_allocate_spw(uint32_t deviceRequestingWindow,
	uint32_t windowStartOrEnd,
	uint32_t importanceOfRequest,
	uint32_t canBeDeferred)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	//char *ptr = NULL;
	struct coex_allocate_pti_window params  = { 0 };

	params.messageID = ALLOCATE_PTI_WINDOW;
	params.deviceRequestingWindow = deviceRequestingWindow;
	params.windowStartOrEnd = windowStartOrEnd;
	params.importanceOfRequest = importanceOfRequest;
	params.canBeDeferred = canBeDeferred;

	#ifdef COEX_DEBUG_ENABLE
		printk("messageID = (%d)\n",
				  params.messageID);
		printk("deviceRequestingWindow = (%d)\n",
				  params.deviceRequestingWindow);
		printk("windowStartOrEnd = (%d)\n",
				  params.windowStartOrEnd);
		printk("importanceOfRequest = (%d)\n",
				  params.importanceOfRequest);
		printk("canBeDeferred = (%d)\n",
				  params.canBeDeferred);
	#endif

	if (params.deviceRequestingWindow > WLAN_DEVICE) {
		printk("Invalid device requesting window (%d)\n",
				  params.deviceRequestingWindow);
		return -ENOEXEC;
	}
	if (params.windowStartOrEnd > START_REQ_WINDOW) {
		printk("Invalid window start/end (%d)\n",
				  params.windowStartOrEnd);
		return -ENOEXEC;
	}

	if (params.importanceOfRequest > HIGHEST_IMPORTANCE) {
		printk("Invalid importance level of window (%d)\n",
				  params.importanceOfRequest);
		return -ENOEXEC;
	}

	if (params.canBeDeferred > YES) {
		printk("Invalid window deferred option (%d)\n",
				  params.canBeDeferred);
		return -ENOEXEC;
	}

	uint32_t cmd_len = sizeof(struct coex_allocate_pti_window);

	status = wifi_nrf_fmac_conf_btcoex(ctx->rpu_ctx,
					   (void *)(&params), cmd_len);


	if (status != WIFI_NRF_STATUS_SUCCESS) {
		printk("Coex: configuration for SPW allocation failed\n");
		return -ENOEXEC;
	}

	return 0;
}



/* Periodic priority windows */
/**
 * [startOrStop:         0 - Stop  1 - Start]
 * [firstPriorityWindow: 0 - WLAN  1 - SR]
 * [powersaveMechanism:  Set to 1]
 * [wlanWindowDuration:  In multiples of 100us. Max of 4095]
 * [srWindowDuration:    In multiples of 100us. Max of 4095]
 */

int nrf_wifi_coex_allocate_ppw(int32_t startOrStop,
	uint32_t firstPriorityWindow,
	uint32_t powersaveMechanism,
	uint32_t wlanWindowDuration,
	uint32_t srWindowDuration)
{

	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	//char *ptr = NULL;
	struct coex_allocate_ppw params  = { 0 };


	params.messageID = ALLOCATE_PPW;
	params.startOrStop = startOrStop;
	params.firstPriorityWindow = firstPriorityWindow;
	params.powersaveMechanism = powersaveMechanism;
	params.wlanWindowDuration = wlanWindowDuration;
	params.srWindowDuration = srWindowDuration;

	#ifdef COEX_DEBUG_ENABLE
		printk("messageID = (%d)\n",
				  params.messageID);
		printk("startOrStop = (%d)\n",
				  params.startOrStop);
		printk("firstPriorityWindow = (%d)\n",
				  params.firstPriorityWindow);
		printk("powersaveMechanism = (%d)\n",
				  params.powersaveMechanism);
		printk("wlanWindowDuration = (%d)\n",
				  params.wlanWindowDuration);
		printk("srWindowDuration = (%d)\n",
				  params.srWindowDuration);
	#endif

	if (params.startOrStop > START_ALLOC_WINDOWS) {
		printk("Invalid start/stop PPW windows (%d)\n",
				  params.startOrStop);
		return -ENOEXEC;
	}
	if (params.firstPriorityWindow > SR_WINDOW) {
		printk("Invalid first priority window (%d)\n",
				  params.firstPriorityWindow);
		return -ENOEXEC;
	}

	if (params.powersaveMechanism > TRIGGER_FRAME) {
		printk("Invalid powersave mechanism (%d)\n",
				  params.powersaveMechanism);
		return -ENOEXEC;
	}

	if (params.wlanWindowDuration > COEX_MAX_WINDOW_MUL_100US) {
		printk("Invalid WLAN window duration (%d)\n",
				  params.wlanWindowDuration);
		return -ENOEXEC;
	}

	if (params.srWindowDuration > COEX_MAX_WINDOW_MUL_100US) {
		printk("Invalid SR window duration (%d)\n",
				  params.srWindowDuration);

		return -ENOEXEC;
	}

	uint32_t cmd_len = sizeof(struct coex_allocate_ppw);

	status = wifi_nrf_fmac_conf_btcoex(ctx->rpu_ctx, (void *)(&params), cmd_len);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		printk("Coex: configuration for PPW allocation failed\n");
		return -ENOEXEC;
	}

	return 0;
}
