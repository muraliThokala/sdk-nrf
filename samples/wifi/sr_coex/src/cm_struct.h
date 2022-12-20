/**
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Structures and related enumerations used in Coexistence.
 */

#ifndef __CM_STRUCT_H__
#define __CM_STRUCT_H__

/* #include "uccrt.h" */
#include <stdint.h>
#include <stdbool.h>

/* Max size of message buffer (exchanged between host and MAC). This is in "bytes" */
#define MAX_MESSAGE_BUF_SIZE 320
/* Number of elements in coex_ch_configuration other than configbuf[] */
#define NUM_ELEMENTS_EXCL_CONFIGBUF 4
/* Each configuration value is of type uint32_t */
#define MAX_NUM_CONFIG_VALUES ((MAX_MESSAGE_BUF_SIZE-\
	(NUM_ELEMENTS_EXCL_CONFIGBUF*sizeof(uint32_t)))>>2)
/* Number of elements in coex_sr_traffic_info other than srTrafficInfo[] */
#define NUM_ELEMENTS_EXCL_SRINFOBUF 1
/* Each SR Traffic Info is of type uint32_t */
#define MAX_SR_TRAFFIC_BUF_SIZE 32


/* Indicates the type of priority window i.e., WLAN or SR */
enum {
	WLAN_WINDOW = 0,
	SR_WINDOW
} coex_pti_window_type;

/* Recommended power save mechanism for WLAN's downlink when a priority window is allocated. */
enum {
	PM_BIT = 0,
	PS_POLL,
	U_APSD,
	S_APSD,
	U_PSMP,
	S_PSMP,
	TRIGGER_FRAME,
} coex_wlan_pm_protocol_type;

/** Indicates device requesting a priority window. This is used to check if
 *  window requests from WLAN and SR collide.
 */
enum{
	SR_DEVICE = 0,
	WLAN_DEVICE
} coex_device_req_pti_window;

/* Indicates if a device requesting a priority window can defer its activity */
enum {
	NO = 0,
	YES
} coex_can_defer_activity;

/** Indicates if request from a module/device is to START or END a priority window.
 *  This is used by a module/device while posting a priority window request to CM.
 */
enum {
	END_REQ_WINDOW = 0,
	START_REQ_WINDOW
} coex_pti_win_req_start_end;

/* Indicates if allocation of PPW/VPW is to be started or stopped */
enum {
	STOP_ALLOC_WINDOWS = 0,
	START_ALLOC_WINDOWS
} coex_ppw_vpw_start_stop;

/* Indicates importance of the activity for which protection from interference is required */
enum {
	LESS_IMPORTANCE = 0,
	MEDIUM_IMPORTANCE,
	HIGH_IMPORTANCE,
	HIGHEST_IMPORTANCE
} coex_imp_activity;

enum {
	/** Used two different values for AGGREGATION module because offset from base is
	 *  beyond supported message buffer size for  WAIT_STATE_1_TIME register
	 */
	COEX_HARDWARE = 1,
	MAC_CTRL,
	MAC_CTRL_AGG_WAIT_STATE_1_TIME,
	MAC_CTRL_AGG,
	MAC_CTRL_DEAGG,
	WLAN_CTRL,
} coex_hardware_to_config;

/* IDs of different messages posted from Coexistence Driver to Coexistence Manager */
enum {
	/* To insturct Coexistence Manager to collect and post SR traffic information */
	COLLECT_SR_TRAFFIC_INFO = 1,
	/* To insturct Coexistence Manager to allocate a priority window to SR */
	ALLOCATE_PTI_WINDOW,
	/* To do configuration of hardware related to coexistence */
	HW_CONFIGURATION,
	/* To start allocating periodic priority windows to WLAN and SR */
	ALLOCATE_PPW,
	/* To start allocating virtual priority windows to WLAN */
	ALLOCATE_VPW,
	/* To configure CM SW parameters */
	SW_CONFIGURATION,
	/* To control sheliak side switch */
	UPDATE_SWITCH_CONFIG
} msg_id_to_cm;

/* ID(s) of different messages posted from Coexistence Manager to Coexistence Driver */
enum {
	/* To post SR traffic information */
	SR_TRAFFIC_INFO = 1
} msg_id_from_cm;

/**
 * struct coex_collect_sr_traffic_info - Message from CD to CM  to request SR traffic info.
 * @messageID: Indicates message ID. This is to be set to COLLECT_SR_TRAFFIC_INFO.
 * @numSetsRequested: Indicates the number of sets of duration and periodicity to be collected.
 *
 * Message from CD to CM  to request SR traffic information.
 */

struct coex_collect_sr_traffic_info {
	uint32_t messageID;
	uint32_t numSetsRequested;
};

/**
 * struct coex_ch_configuration -Message from CD to CM  to configure CH.
 * @messageID: Indicates message ID. This is to be set to HW_CONFIGURATION.
 * @numOfRegistersToConfigure: Indicates the number of registers to be configured.
 * @hwToConfig: Indicates the hardware block that is to be configured.
 * @hwBlockBaseAddress: Base address of the hardware block to be configured.
 * @configbuf: Configuration buffer that holds packed offset and configuration value.
 *
 * Message from CD to CM  to configure CH
 */

struct coex_ch_configuration {
	uint32_t messageID;
	uint32_t numOfRegistersToConfigure;
	uint32_t hwToConfig;
	uint32_t hwBlockBaseAddress;
	uint32_t configbuf[MAX_NUM_CONFIG_VALUES];
};

/**
 * struct coex_allocate_pti_window - Message to CM to request a priority window.
 * @messageID: Indicates message ID. This is to be set to ALLOCATE_PTI_WINDOW.
 * @deviceRequestingWindow: Indicates device requesting a priority window.
 * @windowStartOrEnd: Indicates if request is posted to START or END a priority window.
 * @importanceOfRequest: Indicates importance of activity for which the window is requested.
 * @canBeDeferred: activity of WLAN/SR, for which window is requested can be deferred or not.
 *
 * Message to CM to request a priority window
 */

struct coex_allocate_pti_window {
	uint32_t messageID;
	uint32_t deviceRequestingWindow;
	uint32_t windowStartOrEnd;
	uint32_t importanceOfRequest;
	uint32_t canBeDeferred;
};

/**
 * struct coex_allocate_ppw - Message from CD to CM  to allocate Periodic Priority Windows.
 * @messageID: Indicates message ID. This is to be set to ALLOCATE_PPW.
 * @startOrStop: Indiates start or stop allocation of PPWs.
 * @firstPriorityWindow: Indicates first priority window in the series of PPWs.
 * @powersaveMechanism: Indicates recommended powersave mechanism for WLAN's downlink.
 * @wlanWindowDuration: Indicates duration of WLAN priority window.
 * @srWindowDuration: Indicates duration of SR priority window.
 *
 * Message from CD to CM  to allocate Periodic Priority Windows.
 */

struct coex_allocate_ppw {
	uint32_t messageID;
	uint32_t startOrStop;
	uint32_t firstPriorityWindow;
	uint32_t powersaveMechanism;
	uint32_t wlanWindowDuration;
	uint32_t srWindowDuration;
};

/**
 * struct coex_allocate_vpw - Message from CD to CM  to allocate Virtual Priority Windows.
 * @messageID: Indicates message ID. This is to be set to ALLOCATE_VPW.
 * @startOrStop: Indicates start or stop allocation of VPWs.
 * @wlanWindowDuration: Indicates duration of WLAN virtual priority window.
 * @powersaveMechanism: Indicates recommended powersave mechanism for WLAN's downlink.
 *
 * Message from CD to CM  to allocate Virtual Priority Windows.
 */

struct coex_allocate_vpw {
	uint32_t messageID;
	uint32_t startOrStop;
	uint32_t wlanWindowDuration;
	uint32_t powersaveMechanism;
};

/**
 * struct coex_config_cm_params - Message from CD to CM  to configure CM parameters
 * @messageID: Indicates message ID. This is to be set to SW_CONFIGURATION.
 * @first_isr_trigger_period: microseconds . used to trigger the ISR mechanism.
 * @sr_window_poll_periodicity_vpw: microseconds. This is used to poll through SR window.
 *  that comes after WLAN window ends and next SR activity starts, in the case of VPWs.
 * @lead_time_from_end_of_wlan_win: microseconds. Lead time from the end of WLAN window.
 *  (to inform  AP that WLAN is entering powersave) in the case of PPW and VPW generation.
 * @sr_window_poll_count_threshold: This is equal to "WLAN contention timeout.
 *  threshold"/sr_window_poll_periodicity_vpw.
 *
 * Message from CD to CM  to configure CM parameters.
 */

struct coex_config_cm_params {
	uint32_t messageID;
	uint32_t first_isr_trigger_period;
	uint32_t sr_window_poll_periodicity_vpw;
	uint32_t lead_time_from_end_of_wlan_win;
	uint32_t sr_window_poll_count_threshold;
};

/**
 * struct coex_sr_traffic_info - Message from CM to CD to post SR traffic information.
 * @messageID: Indicates message ID. This is to be set to SR_TRAFFIC_INFO.
 * @srTrafficInfo: Traffic information buffer.
 *
 * Message from CM to CD to post SR traffic inforamtion
 */

struct coex_sr_traffic_info {
	uint32_t messageID;
	uint32_t srTrafficInfo[MAX_SR_TRAFFIC_BUF_SIZE];
};

#endif /* __CM_STRUCT_H__ */
