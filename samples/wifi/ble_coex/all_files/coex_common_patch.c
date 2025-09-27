#include "uccrt.h"
#include "uccDefs.h"

#include "coex_common_hdr.h"
#include "cm_functions.h"
#include "cm_isr.h"

#include "mem_attributes.h"

#define NUM_CH_REGISTERS ((EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_44-EXT_SYS_WLANSYSCOEX_CH_CONTROL)>>2)+1
#define NUM_WLAN_HW_REGISTERS 8

extern uint32_t HardwareConfiguration[NUM_CH_REGISTERS+NUM_WLAN_HW_REGISTERS];

void ROM_CM_saveConfig(void);
void ROM_CM_dynamicConfigCoexHardware(CM_LOCAL_STRUCT_T *localStructPtr);

/* patch for SHEL-1870 */
bool ROM_CM_checkCollisionAllocateWindow(CM_LOCAL_STRUCT_T *localStructPtr);

CM_STATS_T cmStats;

void update_wifi_registers_for_coex(uint32_t coex_hw_en_or_dis);
void ch_reset_bring_out_of_reset(void);

LMAC_CORE_RET bool config_ch_enable;
LMAC_CORE_RET bool is_ch_enable;
LMAC_CORE_RET bool is_rf_switch_to_wifi;
LMAC_CORE_RET bool config_non_pta;
LMAC_CORE_RET bool config_pta;
LMAC_CORE_RET bool is_pta_config_for_2pt4g;
LMAC_CORE_RET uint32_t packed_value_in;

 
/* Indicates WLAN frequency band of operation */
enum wifi_pta_wlan_op_band {
	NRF_WIFI_PTA_WLAN_OP_BAND_2_4_GHZ = 0,
	NRF_WIFI_PTA_WLAN_OP_BAND_5_GHZ,
	NRF_WIFI_PTA_WLAN_OP_BAND_NONE = 0xFF
};

LMAC_CORE_RET bool is_coex_hardware_enable;

/* PTA configuration, WLAN in 2.4GHz. SHA + 2.4G as default */
/* These tables contain WLAN pti window, SR pti window and NO pti window configuration values */
LMAC_CORE_RET uint16_t pta_config_buffer_2pt4G[]  = {
	0x0012, 0x0000, 0x0011, 0x0000, 0x0011,
	0x01F8, 0x01E0, 0x01F4, 0x01E0, 0x01F4,
	0x0019, 0x00F6, 0x0008, 0x00E2, 0x0015,
	0x00F5, 0x0019, 0x0019, 0x0004, 0x01F6,
	0x0008, 0x01E2, 0x00F5, 0x00F5, 0x01F6,
	0x01F6, 0x00E1, 0x00E1, 0x01E2, 0x0008,
	0x0004, 0x0004, 0x0019, 0x0019, 0x0008,
	0x0008, 0x0015, 0x00F5, 0x00F5, 0x00F5,
	0x0008, 0x01E2, 0x00E1, 0x00E1, 0x0004,
	0x01F6, 0x00F6, 0x0019, 0x00E2, 0x0019,
	0x00F6, 0x0008, 0x00E2, 0x0008, 0x001A
};

/* PTA configuration, WLAN in 5GHz. */
LMAC_CORE_RET uint16_t pta_config_buffer_5G[] = {
	0x0012, 0x0000, 0x0011, 0x0000, 0x0011,
	0x01F8, 0x01E0, 0x01F4, 0x01E0, 0x01F4,
	0x0039, 0x0076, 0x0028, 0x0062, 0x0075,
	0x0075, 0x0061, 0x0061, 0x0074, 0x0074,
	0x0060, 0x0060, 0x0075, 0x0075, 0x0064,
	0x0064, 0x0071, 0x0071, 0x0060, 0x0060,
	0x0064, 0x0064, 0x0061, 0x0061, 0x0060,
	0x0060, 0x0075, 0x0075, 0x0075, 0x0075,
	0x0060, 0x0060, 0x0071, 0x0071, 0x0074,
	0x0074, 0x0076, 0x0039, 0x0062, 0x0039,
	0x0076, 0x0028, 0x0062, 0x0028, 0x003A

};
LMAC_CORE_RET uint32_t non_pta_config[] = {
	0x00000028, 0x00000000, 0x001e1023, 0x00000000, 0x00000000,
	0x00000000, 0x00000021, 0x000002ca, 0x0000005A
};


void update_wifi_registers_for_coex(uint32_t coex_hw_en_or_dis) {
	uint32_t regValue, configValue;
	uint32_t phyRxReqEnable = coex_hw_en_or_dis;
	uint32_t macRxReqEnable = coex_hw_en_or_dis;
	uint32_t macTxReqEnable = coex_hw_en_or_dis;

	uint32_t txSignalExt     = coex_hw_en_or_dis ? TX_SIGNAL_EXT_OFDM : 0;
	uint32_t rxSignalExt     = coex_hw_en_or_dis ? RX_SIGNAL_EXT : 0;
	uint32_t phyRxAssertDurExt = coex_hw_en_or_dis ? PHY_RX_EXTENSION : 0;
	uint32_t tx_abort_timeout  = coex_hw_en_or_dis ? TX_ABORT_TIMEOUT : 0;
	uint32_t aggCoexEn         = coex_hw_en_or_dis;
	uint32_t deagg_coex_ctrl   = (coex_hw_en_or_dis << 1) | coex_hw_en_or_dis;
	uint32_t pbytes_for_deassert = coex_hw_en_or_dis ? NUM_PAYLOAD_BYTES_FOR_DEASSERTION : 0;
	uint32_t tx_abort_ctrl     = coex_hw_en_or_dis;

	// 1. Configure PTA Control
	regValue = UCC_READ_PERIP(ABS_PMB_WLAN_MAC_CTRL_PTA_CONTROL);
	configValue = regValue & (PMB_WLAN_MAC_CTRL_PTA_CONTROL_REG_MASK ^ MASK_TO_CLEAR_BITS_PERIP);
	uint32_t ptaIfCtrl = ((phyRxReqEnable << 2) | (macRxReqEnable << 1) | macTxReqEnable)
						 << PMB_WLAN_MAC_CTRL_PTA_CONTROL_REG_SHIFT;
	UCC_WRITE_PERIP(ABS_PMB_WLAN_MAC_CTRL_PTA_CONTROL, configValue | ptaIfCtrl);

	// 2. Configure Signal Extensions
	uint32_t ptaExt = (txSignalExt << PMB_WLAN_MAC_CTRL_TX_SIGNAL_EXT_SHIFT) |
					  (rxSignalExt << PMB_WLAN_MAC_CTRL_RX_SIGNAL_EXT_SHIFT) |
					  (phyRxAssertDurExt << PMB_WLAN_MAC_CTRL_PHY_RX_SIGNAL_EXT_SHIFT);
	UCC_WRITE_PERIP(ABS_PMB_WLAN_MAC_CTRL_PTA_EXT, ptaExt);

	// 3. TX Abort Timeout
	UCC_WRITE_PERIP(ABS_PMB_WLAN_MAC_CTRL_TX_ABORT_TIMEOUT,
					tx_abort_timeout << PMB_WLAN_MAC_CTRL_TX_ABORT_TIMEOUT_SHIFT);

	// 4. AGG Wait State Time
	UCC_WRITE_PERIP(ABS_PMB_WLAN_MAC_CTRL_AGG_WAIT_STATE_1_TIME,
					tx_abort_timeout << PMB_WLAN_MAC_CTRL_AGG_WAIT_STATE_1_TIME_SHIFT);

	// 5. AGG BT Coex Enable
	UCC_WRITE_PERIP(ABS_PMB_WLAN_MAC_CTRL_AGG_BT_COEX_ENABLE,
					aggCoexEn << PMB_WLAN_MAC_CTRL_AGG_BT_COEX_ENABLE_SHIFT);

	// 6. DEAGG BT Coex Control
	UCC_WRITE_PERIP(ABS_PMB_WLAN_MAC_CTRL_DEAGG_BT_COEX_CTRL,
					deagg_coex_ctrl << PMB_WLAN_MAC_CTRL_DEAGG_BT_COEX_CTRL_SHIFT);

	// 7. Payload Bytes for Deassertion
	UCC_WRITE_PERIP(ABS_PMB_WLAN_CTRL_NUM_PAYLOAD_BYTES_FOR_DEASSERTION,
					pbytes_for_deassert << PMB_WLAN_CTRL_NUM_PAYLOAD_BYTES_FOR_DEASSERTION_SHIFT);

	// 8. TX Abort Control
	UCC_WRITE_PERIP(ABS_PMB_WLAN_CTRL_TX_ABORT_CONTROL,
					tx_abort_ctrl << PMB_WLAN_CTRL_TX_ABORT_CONTROL_SHIFT);
}

void patch_CM_dynamicConfigCoexHardware(CM_LOCAL_STRUCT_T *localStructPtr)
{
	/* 0 for UCC_HW_WRITE32 and 1 for UCC_WRITE_PERIP */
	uint32_t peripOrHWwrite = (localStructPtr->hwToConfig == COEX_HARDWARE) ? 0 : 1;

	for (uint32_t index = 0; index < localStructPtr->numOfRegistersToConfigure; index++) {
		uint32_t rawConfig = localStructPtr->configBufferPtr[index];
		uint32_t offset = ((rawConfig >> 24) << 2);
		uint32_t *address = (uint32_t *)(localStructPtr->hwBlockBaseAddress + offset);
		uint32_t configValue = rawConfig & 0xFFFFFF;

		if (peripOrHWwrite) {
			/* non-coex hardware registers. Currently, Wi-Fi side coex registers */
			UCC_WRITE_PERIP(address, configValue);
		} else {
			/* write to the coex hardware register */
			UCC_HW_WRITE32(address, configValue);

			/* update the PTA table */
			if (config_pta) {
				/* Do the PTA table update */
				if (is_pta_config_for_2pt4g) {
					/* Update 2.4G PTA table */
					pta_config_buffer_2pt4G[index] = configValue;
				} else {
					/* Update 5G PTA table */
					pta_config_buffer_5G[index] = configValue;
				}
			} else {
				/* Do the non-PTA table update */
				non_pta_config[index] = configValue;
			}
		}
	}
}


//Note: ROM_CM_saveConfig(); is called after this.
void update_coex_hardware_enable(bool coexHwEn, bool rf_switch_to_wifi)
{
	/* Enable/Disable Coexistence Hardware */
	uint32_t configValue = UCC_HW_READ32(ABS_EXT_SYS_WLANSYSCOEX_CH_CONTROL);
	configValue = (configValue & ~EXT_SYS_WLANSYSCOEX_ENABLE_MASK) |
				  (coexHwEn << EXT_SYS_WLANSYSCOEX_ENABLE_SHIFT);
	UCC_HW_WRITE32(ABS_EXT_SYS_WLANSYSCOEX_CH_CONTROL, configValue);

	/* This block of code is to make sure that the RF switch is connected to right protocol */
	uint32_t configValueIn = UCC_HW_READ32(ABS_EXT_SYS_WLANSYSCOEX_CH_OUTPUT_DEBUG_MODE);
	/* when CH is enabled, set output selection to 0 */
	uint32_t configValueOut = configValueIn & (~EXT_SYS_WLANSYSCOEX_OUTPUT_SELECTION_MASK);

	if (!coexHwEn) {
		/* when CH is disabled, set output selection to 1 */
		configValueOut |= (1 << EXT_SYS_WLANSYSCOEX_OUTPUT_SELECTION_SHIFT);

		/* NOte: No need to configure CH_OUTPUT_DEBUG_MODE.SW_CTRL_DBG when CH is enabled */
		/* When CH is disabled, switch is connected to SR by default. To be updated as per the 
		   requirement. This depends on CH_SW_CTRL_MAP. 
		   Switch control:  WLAN_TX = 0x0, WLAN_RX = 0x1, SR_TX = 0x2, SR_RX = 0x3. */
		/* set SW_CTRL_DBG to 0 , as default to connect switch to Wi-Fi*/
		configValueOut &= (~EXT_SYS_WLANSYSCOEX_SW_CTRL_DBG_MASK);

		if (!rf_switch_to_wifi) {
			/* set SW_CTRL_DBG to 2 , if switch is not for Wi-Fi*/
			configValueOut |= (2 << EXT_SYS_WLANSYSCOEX_SW_CTRL_DBG_SHIFT);
		}
	}
	UCC_HW_WRITE32(ABS_EXT_SYS_WLANSYSCOEX_CH_OUTPUT_DEBUG_MODE, configValueOut);

	/* Update Wi-Fi side coex registers as per the CH enable/disable  and save the config */
	update_wifi_registers_for_coex(coexHwEn);

	// Note: ROM_CM_saveConfig is called after this function and so commenting here */
	// ROM_CM_saveConfig();
}

int update_non_pta_config(uint32_t *non_pta_config_buffer)
{
	uint32_t *writeAddr = (uint32_t *)ABS_EXT_SYS_WLANSYSCOEX_CH_TIME_REFERENCE;
	//uint32_t numberOfRegs = ((EXT_SYS_WLANSYSCOEX_CH_SR_INFO_STATUS -
	//						  EXT_SYS_WLANSYSCOEX_CH_TIME_REFERENCE) >> 2) + 1;

	uint32_t numberOfRegs = ((EXT_SYS_WLANSYSCOEX_CH_SW_CTRL_MAP -
							  EXT_SYS_WLANSYSCOEX_CH_TIME_REFERENCE) >> 2) + 1;

	for (uint32_t indexCH = 0; indexCH < numberOfRegs; indexCH++) {
		UCC_HW_WRITE32(writeAddr + indexCH, non_pta_config_buffer[indexCH]);
	}

	// Note: ROM_CM_saveConfig is called after this function and so commenting here */
	// ROM_CM_saveConfig();

	return 0;
}


int update_pta_config(uint32_t wifi_op_band)
{
	/* configuring WLAN and SR PTI windows, no PTI window lookup based on WLAN band */
	uint32_t *writeAddr = (uint32_t *)ABS_EXT_SYS_WLANSYSCOEX_CH_WLAN_WINDOW_LOOKUP_0;
	uint32_t numberOfRegs = ((EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_44 -
					 EXT_SYS_WLANSYSCOEX_CH_WLAN_WINDOW_LOOKUP_0) >> 2) + 1;

	const uint16_t *pta_config_buffer = (wifi_op_band == NRF_WIFI_PTA_WLAN_OP_BAND_2_4_GHZ) ?
						pta_config_buffer_2pt4G : pta_config_buffer_5G;

	for (uint32_t indexCH = 0; indexCH < numberOfRegs; indexCH++) {
		UCC_HW_WRITE32(writeAddr + indexCH, pta_config_buffer[indexCH]);
	}

	// Note: ROM_CM_saveConfig is called after this function and so commenting here */
	// ROM_CM_saveConfig();

	return 0;
}



/* Read CH and WLAN-peripheral blocks configuration from retention memory and configure the blocks */
void ch_reset_bring_out_of_reset(void) {
	/* Reset Coexistence Hardware and bring out of reset before configuring the registers */
	uint32_t chReset = 1;
	uint32_t configValue = 0;

	/* Reset coexistence hardware */
	configValue = UCC_HW_READ32(ABS_EXT_SYS_WLANSYSCOEX_CH_CONTROL);
	configValue = configValue & (~EXT_SYS_WLANSYSCOEX_RESET_MASK);
	configValue =  configValue | (chReset << EXT_SYS_WLANSYSCOEX_RESET_SHIFT);		
	UCC_HW_WRITE32(ABS_EXT_SYS_WLANSYSCOEX_CH_CONTROL, configValue);

	/* Bring coexistence hardware out of reset. Set reset bit to 0 */
	configValue = configValue & (~EXT_SYS_WLANSYSCOEX_RESET_MASK);
	UCC_HW_WRITE32(ABS_EXT_SYS_WLANSYSCOEX_CH_CONTROL, configValue);
}

void patch_CM_restoreConfig(void)
{
	/* Read configuration from buffer available in retention memory and write to CoexHardware */
	uint32_t configValue = 0;
	uint32_t *readAddr  = &HardwareConfiguration[0];
	uint32_t *writeAddr = (uint32_t *) ABS_EXT_SYS_WLANSYSCOEX_CH_CONTROL;
	uint32_t indexReadBuf=0;
	uint32_t indexCH;
	uint32_t numRegisters=((EXT_SYS_WLANSYSCOEX_CH_NO_WINDOW_LOOKUP_44-EXT_SYS_WLANSYSCOEX_CH_CONTROL)>>2)+1;

	for (indexCH = 0; indexCH < numRegisters; indexCH++)
	{
		configValue = *(readAddr+indexReadBuf); 
		UCC_HW_WRITE32 (writeAddr+indexCH, configValue);
		indexReadBuf++;
	}	

	/* Read configuration from buffer available in retention memory and write to WLAN side coexistence registers */
	uint32_t addressList[] = {ABS_PMB_WLAN_MAC_CTRL_PTA_CONTROL, ABS_PMB_WLAN_MAC_CTRL_PTA_EXT,
				    ABS_PMB_WLAN_MAC_CTRL_TX_ABORT_TIMEOUT, ABS_PMB_WLAN_MAC_CTRL_AGG_WAIT_STATE_1_TIME,
					ABS_PMB_WLAN_MAC_CTRL_AGG_BT_COEX_ENABLE, ABS_PMB_WLAN_MAC_CTRL_DEAGG_BT_COEX_CTRL,
					ABS_PMB_WLAN_CTRL_NUM_PAYLOAD_BYTES_FOR_DEASSERTION, ABS_PMB_WLAN_CTRL_TX_ABORT_CONTROL};
	uint32_t indexWLAN=0;
	numRegisters=sizeof(addressList)/sizeof(addressList[0]);

	for (indexWLAN = 0; indexWLAN < numRegisters; indexWLAN++)
	{
		configValue = *(readAddr+indexReadBuf);
		UCC_WRITE_PERIP (addressList[indexWLAN], configValue);
		indexReadBuf++;
	}
	/* Reset CH and bring out of reset after configuring the CH registers */
	ch_reset_bring_out_of_reset();

}

void patch_CM_initialization(BOOT_TYPE_T wlanBootType)
{
	/* Configuration for SR info done ISR */
	CM_register_user_defined_ISR();
	
	//memset(HardwareConfiguration,0,sizeof(HardwareConfiguration));
	
	if(wlanBootType == COLD_BOOT)
	{
		cmStatsPtr->cmInitColdBootCnt++;

		/* Disable coex hardware by default */
		bool coexHardwareEnable = 0; 
		/* Connect RF switch to Wi-Fi antenna by default */
		bool rf_switch_to_wifi = 1;
		/* This function also does non-CH configurations i.e. WLAN peripherals configs for coex. */
		update_coex_hardware_enable(coexHardwareEnable, rf_switch_to_wifi);

		/* non-PTA registers configuration . SHA as default.*/
		update_non_pta_config(non_pta_config);

		/* PTA registers configuration . SHA + 2.4G as default*/
		uint32_t wifi_op_band = NRF_WIFI_PTA_WLAN_OP_BAND_2_4_GHZ;
		update_pta_config(wifi_op_band);

		/* Reset CH and bring out of reset after configuring the CH registers */
		ch_reset_bring_out_of_reset ();

		/* Save the register configuration to restore during the warm-boot */
		ROM_CM_saveConfig();

	}else {
		cmStatsPtr->cmRestoreCfgCnt++;
		/* restore CH and WLAN-HW configuration */
		patch_CM_restoreConfig();
	}
} 

bool checkCollisionAllocateWindow(CM_LOCAL_STRUCT_T *localStructPtr)
{
	bool requestCanBeServiced = 0;
	uint32_t regValue = UCC_HW_READ32(ABS_EXT_SYS_WLANSYSCOEX_CH_EXT_WINDOW_REQUEST);
	uint32_t windowAllocation = (regValue & EXT_SYS_WLANSYSCOEX_ALLOCATE_WINDOW_MASK) >> EXT_SYS_WLANSYSCOEX_ALLOCATE_WINDOW_SHIFT;

	PTI_WINDOWS_PARAMS_T ptiWindowsParams = { .psMechanism = PS_POLL };
	PTI_WINDOWS_PARAMS_T *params = &ptiWindowsParams;

	bool isWLAN = (localStructPtr->deviceRequestingWindow == WLAN_DEVICE);
	bool isStartRequest = isWLAN ? (localStructPtr->WLANwindowStartOrEnd == START_REQ_WINDOW)
								 : (localStructPtr->SRwindowStartOrEnd == START_REQ_WINDOW);

	// Update stats
	if (isWLAN) {
		cmStatsPtr->WLANwindowRequests++;
		if (isStartRequest) cmStatsPtr->WLANwindowStartRequests++;
		else cmStatsPtr->WLANwindowEndRequests++;
	} else {
		cmStatsPtr->SRwindowRequests++;
		if (isStartRequest) cmStatsPtr->SRwindowStartRequests++;
		else cmStatsPtr->SRwindowEndRequests++;
	}

	if (isStartRequest) {
		if (windowAllocation == 0) {
			/* currently CM has not allocated any window to WLAN or SR. So, current request can be serviced */
			requestCanBeServiced = 1;
		} else {
		/* If window is allocated to one of the protocols, then follow the below steps.
		   Initially take decision based on "if protocol can be deferred or not".
		   If both the protocols can be deferred then take decision based on priority. */
			bool canBeDeferredRequester = isWLAN ? localStructPtr->WLANcanBeDeferred : localStructPtr->SRcanBeDeferred;
			bool canBeDeferredOther = isWLAN ? localStructPtr->SRcanBeDeferred : localStructPtr->WLANcanBeDeferred;
			uint8_t importanceRequester = isWLAN ? localStructPtr->WLANimportanceOfRequest : localStructPtr->SRimportanceOfRequest;
			uint8_t importanceOther = isWLAN ? localStructPtr->SRimportanceOfRequest : localStructPtr->WLANimportanceOfRequest;
			
			if (!canBeDeferredRequester && !canBeDeferredOther) {
				requestCanBeServiced = 0;
			} else if (!canBeDeferredOther && canBeDeferredRequester) {
				requestCanBeServiced = 0;
			} else if (canBeDeferredOther && !canBeDeferredRequester) {
				requestCanBeServiced = 1;
			} else {
				requestCanBeServiced = (importanceRequester > importanceOther);
			}
		}

		if (requestCanBeServiced) {
			/* stop allocating window to the protocol that failed in arbitration and allocate window to the protocol that won the arbitration */
			params->allocWindow = END_REQ_WINDOW;
			CM_allocateWindow(params);

			params->windowType = isWLAN ? WLAN_WINDOW : SR_WINDOW;
			CM_configSPWGparameters(params);

			params->allocWindow = START_REQ_WINDOW;
			CM_allocateWindow(params);

			/* Reset CH and bring out of reset after configuring the CH registers */
			ch_reset_bring_out_of_reset();
		}
	} else {
		params->allocWindow = END_REQ_WINDOW;
		CM_allocateWindow(params);
		requestCanBeServiced = 1;

		/* Reset CH and bring out of reset after configuring the CH registers */
		ch_reset_bring_out_of_reset();
	}

	if (requestCanBeServiced) {
		if (isWLAN) cmStatsPtr->WLANwindowRequestsServiced++;
		else cmStatsPtr->SRwindowRequestsServiced++;
	}

	return requestCanBeServiced;
}
bool patch_CM_checkCollisionAllocateWindow(CM_LOCAL_STRUCT_T *localStructPtr)
{
	bool requestCanBeServiced = 0;

	/* return success (1) when the coexistence hardware is disabled */
	if(!is_coex_hardware_enable) {
		return(1);
	}

	if(localStructPtr->deviceRequestingWindow==WLAN_DEVICE) 
	{
		/* current request for window is from WLAN. */
		requestCanBeServiced = checkCollisionAllocateWindow(localStructPtr);
		return(requestCanBeServiced); 
	}
	else 
	{
		/* current request for window is from SR. */
		cmStats.SRwindowRequests++;

		if(localStructPtr->SRwindowStartOrEnd==START_REQ_WINDOW)
		{
			cmStats.SRwindowStartRequests++;
		}
		else
		{
			cmStats.SRwindowEndRequests++;
		}
		requestCanBeServiced =0;
		return(requestCanBeServiced);
	}
}


void patch_CM_coexProcessCmd(uint8_t *msgRcvd) 
{
	CD2CM_UPDATE_SWITCH_CONFIG_T msgUpdateSwitchConfig;
	CD2CM_UPDATE_SWITCH_CONFIG_T *msgUpdateSwitchConfigPtr = &msgUpdateSwitchConfig;


	CD2CM_HW_CONFIGURATION_PATCH_T msgCHconf;
	CD2CM_HW_CONFIGURATION_PATCH_T *msgCHconfigPtr = &msgCHconf;
	
	CM_LOCAL_STRUCT_T localStructure;
	CM_LOCAL_STRUCT_T *localStructPtr=&localStructure;

	if((*msgRcvd)==CD2CM_HW_CONFIGURATION1)
	{
		/* collect all the parameters */
		cmStatsPtr->hwConfigMsgs++;
		msgCHconfigPtr = (CD2CM_HW_CONFIGURATION_PATCH_T *)msgRcvd;
		localStructPtr->hwToConfig = msgCHconfigPtr->hwToConfig;
		localStructPtr->hwBlockBaseAddress = msgCHconfigPtr->hwBlockBaseAddress;
		localStructPtr->configBufferPtr = &(msgCHconfigPtr->configbuf[0]);

		uint32_t packed_value = msgCHconfigPtr->numOfRegistersToConfigure;
		packed_value_in = packed_value;
		// all the following variables are of 1 bit.
		config_ch_enable = (packed_value >> CONFIG_CH_ENABLE_SHIFT) & 1;
		config_non_pta = (packed_value >> CONFIG_NON_PTA_SHIFT) & 1;
		config_pta = (packed_value >> CONFIG_PTA_SHIFT) & 1;

		/* numOfRegistersToConfigure (that is packed with other 
		parameters) is available in lower 16bits. */
		localStructPtr->numOfRegistersToConfigure = packed_value & 0xffff;

			if(config_non_pta == 1) {
				/* configure all non-PTA registers of CH */
				/* Step1: Configure other non-PTA registers of CH */
				patch_CM_dynamicConfigCoexHardware(localStructPtr);

				/* Step2: Enable CH and configure RF switch. */
				if(config_ch_enable) {
					is_ch_enable = (packed_value  >> IS_CH_ENABLE_SHIFT) & 1;
					is_rf_switch_to_wifi = (packed_value >> IS_RF_SWITCH_TO_WIFI_SHIFT) & 1;
					is_coex_hardware_enable = is_ch_enable;
					update_coex_hardware_enable(is_ch_enable, is_rf_switch_to_wifi);
					//UCC_HW_WRITE32(ABS_EXT_SYS_WLANSYSCOEX_CH_OUTPUT_DEBUG_MODE, 0xffffff);
					//UCC_HW_WRITE32(0xa401bc18, 0xffffff); // configured as expected.
				}
			}
			//UCC_HW_WRITE32(0xa401bc18, 0x22); // configured as expected.
			if(config_pta) {
				/* Configure PTA registers i.e. priority and non-priority window PTA tables */
				is_pta_config_for_2pt4g = (packed_value >> IS_PTA_CONFIG_FOR_2PT4G_SHIFT) & 1;
				patch_CM_dynamicConfigCoexHardware(localStructPtr);
			}

		/* Reset CH and bring out of reset after configuring the CH registers */
		ch_reset_bring_out_of_reset();

		/* Save configuration of CH and coex registers in WLAN */
		ROM_CM_saveConfig();

	} else if((*msgRcvd)==CD2CM_UPDATE_SWITCH_CONFIG) {
	
		msgUpdateSwitchConfigPtr = (CD2CM_UPDATE_SWITCH_CONFIG_T *)msgRcvd;

		uint32_t regVal=0;

		switch(msgUpdateSwitchConfigPtr->switch_A) 
		{
			case 0:
				/* SYS_SLEEP_CTRL_GPIO_CTRL->INVERT_SWCTRL0 = 1 =>  SW_CTRL0=1 => antenna A connects to WLAN */
				regVal = UCC_HW_READ32(ABS_SYS_SLEEP_CTRL_GPIO_CTRL);
				regVal |= (1 << SYS_SLEEP_CTRL_INVERT_SWCTRL0_OUTPUT_SHIFT);
				UCC_HW_WRITE32(ABS_SYS_SLEEP_CTRL_GPIO_CTRL, regVal);
			break;

			case 1:
				/* SYS_SLEEP_CTRL_GPIO_CTRL->INVERT_SWCTRL0 = 0  => SW_CTRL0=0 => antenna A connects to BT */
				regVal = UCC_HW_READ32(ABS_SYS_SLEEP_CTRL_GPIO_CTRL);
				regVal &= (0xFFFFFFFF ^ SYS_SLEEP_CTRL_INVERT_SWCTRL0_OUTPUT_MASK);
				UCC_HW_WRITE32(ABS_SYS_SLEEP_CTRL_GPIO_CTRL, regVal);
			break;
		}
	} else {
		/* Call CM proc command */
		ROM_CM_coexProcessCmd(msgRcvd);
	}
}
