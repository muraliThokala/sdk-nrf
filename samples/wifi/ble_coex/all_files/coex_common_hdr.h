
#ifndef _COEX_COMMON_HEADERS_H_
#define _COEX_COMMON_HEADERS_H_

/* Max size of message buffer (exchanged between host and MAC). This is in "bytes" */
#define MAX_MESSAGE_BUF_SIZE_NCS 256
/* Number of elements in coex_ch_configuration other than configbuf[] */
#define NUM_ELEMENTS_EXCL_CONFIGBUF_NCS 4
/* Each configuration value is of type uint32_t */
#define MAX_NUM_CONFIG_VALUES_NCS ((MAX_MESSAGE_BUF_SIZE_NCS-\
	(NUM_ELEMENTS_EXCL_CONFIGBUF_NCS*sizeof(uint32_t)))>>2)
/* Number of elements in coex_sr_traffic_info other than sr_traffic_info[] */

	#include "cm_api.h"
	#include "mem_attributes.h"
	#include "cm_functions.h"

	int update_pta_config(uint32_t wifi_op_band);

	/*! IDs of different messages posted from Coexistence Driver to Coexistence Manager */ 
	typedef enum 
	{        
		/*! To insturct Coexistence Manager to collect and post SR traffic information */
		CD2CM_COLLECT_SR_TRAFFIC_INFO1 = 1,
		/*! To insturct Coexistence Manager to allocate a priority window to SR */
		CD2CM_ALLOCATE_PTI_WINDOW1,        
		/*! To do configuration of hardware related to coexistence */
		CD2CM_HW_CONFIGURATION1,    
		/*! To start allocating periodic priority windows to WLAN and SR */        
		CD2CM_ALLOCATE_PPW1,        
		/*! To start allocating virtual priority windows to WLAN */
		CD2CM_ALLOCATE_VPW1,
		/*! To configure CM SW parameters */
		CD2CM_SW_CONFIGURATION1,
		/*! To control sheliak side switch */
		CD2CM_UPDATE_SWITCH_CONFIG,
	}CD2CM_MSG_ID_PATCH_T; 
	/*! Message from CD to CM  to configure CH */    
    typedef struct 
    {
		/*! Indicates message ID. This is to be set to CD2CM_HW_CONFIGURATION */
		uint32_t messageID;
		/*! Indicates the number of registers to be configured */
		uint32_t numOfRegistersToConfigure;
		/*! Indicates the hardware block that is to be configured */
		uint32_t hwToConfig;
		/*! Base address of the hardware block to be configured */
		uint32_t hwBlockBaseAddress;
		/*! Configuration buffer that holds packed offset and configuration value */
		uint32_t configbuf[MAX_NUM_CONFIG_VALUES_NCS];
		/*! Indicates if Coexistence Hardware configuration is to enabled or not. */
		uint32_t config_ch_enable;
		/*! Indicates if Coexistence Hardware is to be enabled or not. */
		uint32_t is_ch_enable;
		/*! Indicates if WLAN side RF switch is to Wi-Fi by default. */
		uint32_t is_rf_switch_to_wifi;
		/*! Indicates if CH configuration is for non-PTA. */
		uint32_t config_non_pta;
		/*! Indicates if CH configuration is for PTA. */
		uint32_t config_pta;
		/*! Indicates PTA configuration is for 2.4G or not. */
		uint32_t is_pta_config_for_2pt4g;
    }CD2CM_HW_CONFIGURATION_PATCH_T;
		
	/*! Message from CD to CM to control sheliak side switch  */    
	typedef struct 
	{
		/*! Indicates message ID. This is to be set to CD2CM_UPDATE_SWITCH_CONFIG */
		CD2CM_MSG_ID_T messageID;
		/*! Indicates switch A connection */
		uint32_t switch_A;
	}CD2CM_UPDATE_SWITCH_CONFIG_T;


	
	void ROM_CM_coexProcessCmd(uint8_t *msgRcvd);
	void ROM_CM_initialization(BOOT_TYPE_T wlanBootType);

	
	void patch_CM_coexProcessCmd(uint8_t *msgRcvd) ;
	void patch_CM_initialization(BOOT_TYPE_T wlanBootType);
	
	/*! Antenna configuration */ 
	typedef enum 
	{        
		/*! Indicates Wi-Fi and SR are using shared antenna */
		SHARED_ANTENNA = 0,
		/*! Indicates Wi-Fi and SR are using separate antennaa */
		SEPARATE_ANTENNAS        
	}ANTENNA_CONFIG_TYPE;

	/*! Protocol used in coexistence */ 
	typedef enum 
	{        
		/*! Indicates SR protocol is non-BLE */
		NON_BLE_PROTOCOL = 0,
		/*! Indicates SR protocol is BLE */
		BLE_PROTOCOL
	}COEX_PROTOCOL_TYPE;

	/*! Bit positions of parameters packed */
	typedef enum 
	{        
		/* bit position of config_ch_enable */
		CONFIG_CH_ENABLE_SHIFT = 16,
		/* bit position of is_ch_enable */
		IS_CH_ENABLE_SHIFT,
		/* bit position of is_rf_switch_to_wifi */
		IS_RF_SWITCH_TO_WIFI_SHIFT,
		/* bit position of config_non_pta */
		CONFIG_NON_PTA_SHIFT,
		/* bit position of config_pta */
		CONFIG_PTA_SHIFT,
		/* bit position of  is_pta_config_for_2pt4g */
		IS_PTA_CONFIG_FOR_2PT4G_SHIFT
	}COEX_PARAMS_PACKED_SHIFTS;
#endif /* _COEX_COMMON_HEADERS_H_  */
