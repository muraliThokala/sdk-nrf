#include "uccrt.h"
#include "phy_common_hdr.h"
#include "wlan_rf_struct.h"
#include "wlan_rf_api.h"
#include "wlan_phy_struct.h"
#include "wlan_afe_struct.h"
#include "lmac_common_hdr.h"
#include "wlan_bb_rf_commondefines.h"
#include "wlan_scp_utils.h"
#include "wlRegisters.h"
#include "rodincommondef.h"
#include "chipRfDef.h"
#include "wlan_calib_utils.h"
#include "cm_struct.h"
#include "patch_lmac_if.h"
#include "wlan_phy_rf_params_offsets.h"
#include "lmac_mm.h"
#include "rpu_config_api.h"
#include "wlan_txscp_api.h"
#include "wlan_txscp_api.h"
#include "captureSignal.h"

//#define COEX_SYSTEM_PATCH
#ifdef COEX_SYSTEM_PATCH
/* coex update */
#include "cm_api.h"
#include "coex_common_hdr.h"
#include "cm_functions.h"
#endif
#define ENHANCED_TXDC_ESTIMATION
#define TX_POWER_CONTROL_PARAMS_OFFSET 61
#define POWER_DET_PARAM_OFFSET 7
#define RX_GAIN_OFFSET 24 

#define FIVE_GHZ_CHAN1 (36)
#define FIVE_GHZ_CHAN2 (100)
#define FIVE_GHZ_CHAN3 (165)
#define DSSS_POWER_OFFSET (60)
#define BAND_2G_LW_ED_BKF_DSSS_OFST 155

#define RF_REGISTER_BASE_ADDRESS 0xA4009000

#ifdef OVERRIDE_TRIM_SUPPORT

#define NRF_WIFI_RF_PARAMS_OFF_OVERRIDE_TRIM 193
#define NRF_WIFI_RF_PARAMS_OFF_LB_PABIAS 194
#define NRF_WIFI_RF_PARAMS_OFF_LB_DABIAS 195
#define NRF_WIFI_RF_PARAMS_OFF_LB_PGABIAS 196
#define NRF_WIFI_RF_PARAMS_OFF_HB_PABIAS 197
#define NRF_WIFI_RF_PARAMS_OFF_HB_DABIAS 198
#define NRF_WIFI_RF_PARAMS_OFF_HB_PGABIAS 199

#endif

#define READ_REG_FIELD(base, offset, mask, shift, value) \
	((UCC_HW_READ32((base) + (offset)) & ~(mask)) | ((value) << (shift)))

PHY_CORE_RET uint8_t untrimmed_board = 0;
PHY_CORE_RET uint32_t calibBitMapInfo;

#ifdef ENHANCED_TXDC_ESTIMATION
PHY_CORE_RET uint8_t g_is_enhanced_txdc = 0;
#endif

extern LMAC_CORE_RET uint8_t csp_pkg_true;
extern LMAC_CORE_RET uint8_t paconf5_reg_val;
extern PHY_CORE_RET int8_t tempTrimCompvalid;
extern PHY_CORE_RET int8_t freqDepOffsetCompvld;
extern LMAC_CORE_RET struct edge_channel_info g_edge_chan[MAX_EDGE_CHANNELS];
extern LMAC_CORE_RET unsigned int g_num_channels;
extern PHY_CORE_RET uint8_t byte_24;

PHY_CORE_RET struct edge_bo_val g_edge_bo_val;
PHY_CORE_RET struct band_edge_backoff_info g_band_edge_backoff_info;

extern uint8_t maxPowOfdmInfo[8]; // OFDM:0-7
extern uint8_t maxPowDsssInfo[4];

int ROM_wlanWindowRequest(WINDOW_START_END_T windowStartOrEnd,
			  IMPORTANCE_LEVEL_T importanceOfRequest,
			  CAN_DEFER_T canBeDeferred);
void disbExtCapdetLogic();
void ROM_rf_resetCalibResults(RF_CONFIG_INFO *rfConfiguration);
void ROM_TXSCP_setDpdCoeffs(PA_CALIB_PARAMS_T *dpdParams, uint8_t powerIndex);
BOOL_E ROM_PHY_setDeactivateState();
BOOL_E ROM_PHY_enterCalibMode(PHY_OPR_MODE phyMaster);
BOOL_E ROM_PHY_exitCalibMode(uint8_t enableBbRx);
void ROM_rf_stdby();
void ROM_rf_awake(RF_CONFIG_INFO *rfConfiguration);
void ROM_WLANBB_channelSwitch(RF_CONFIG_INFO *rfConfiguration, uint8_t *rf_params);
void RXSCP_configAdcDiscardSampleCount(uint16_t txEndToRxStartDelay);
void RodinWlfillTXGainLUT(int8_t mintxpower, uint8_t txPowerIncStep);
void ROM_RXSCP_config(WLAN_PHY_RX_SCP_CONFIG_T *WLAN_PHY_RX_SCP_CONFIG,
		      WLAN_PHY_RX_SCP_FILTER_COEFF_CONFIG_T *WLAN_PHY_RX_SCP_FILTER_COEFF_CONFIG,
		      WLAN_AFE_FORMAT_T adcformat,
		      CHANNEL_BW_E cbw,
		      uint8_t ui8IQSwap,
		      uint32_t *scpCntrlVal,
		      WLAN_PHY_DELAY_PARAMS_T *phyDelayParams);
void ROM_ED_configRegs(uint32_t operBw,
		       uint32_t p20Flag,
		       ED_WLAN_RF_FREQ_BAND_T freqBand,
		       uint32_t channel_number,
		       ED_UTILS_OUT_STRUCT *edUtilsOut,
		       WLAN_ED_PARAMS_T *edParams);

int16_t ROM_rf_getIsample(uint32_t cplx);
int16_t ROM_rf_getQsample(uint32_t cplx);
uint32_t ROM_rf_makeComplexSample(int16_t vi, int16_t vq);
void upatedConfigData();
void ROM_RodinWlanSetIntDcCalPath(WLAN_SPI_DATA_T *spiData, WLAN_RF_FREQ_BAND_T Band);
void ROM_rf_setrxmode();
void ROM_rf_comp(RF_CONFIG_INFO *rfConfiguration, RF_CALIB_RESULT_T *rfCalibResult, uint8_t *rfParams);

void ROM_RodinWlPllRetune(WLAN_RF_FREQ_BAND_T Band, int Channel);
void ROM_RodinWlPllOn(WLAN_RF_FREQ_BAND_T Band, int Channel, WLAN_RF_BW_SEL ChBW);
void ROM_rf_rxDcCalibration(RF_CONFIG_INFO *rfConfiguration, RX_DCOC_CALIB_PARA_T *rxdcoffset, uint8_t *rfParams);
void RodinWlanTrxAlcBypassEnable();
void sxfailchknrecalib();
void patch_get_edge_chan_info(uint32_t chan_number);
uint32_t check_edge_chan(uint32_t chan_number, uint16_t *edge_bkf_type);
uint32_t get_chan_freq(uint32_t chan_number);
void get_edge_bo(uint32_t center_freq, uint16_t *edge_bkf_type);
void ROM_dpd_convert_coeffs_to_regvalues(uint32_t *dpdreg, int32_t *aI, int32_t *aQ);
void ROM_dpd_energy_detect(int16_t *edptr, uint32_t *dataIQ, int16_t ndata, RF_DC_ESTIMATE *dcest);

extern uint16_t dpdEdPointer;
extern RF_CALIB_RESULT_T RF_WORKING_CHAN_COMP_PARAMS[1];
extern WLAN_SPI_DATA_T *spiData;

#ifdef COEX_SYSTEM_PATCH
LMAC_CORE_RET uint32_t prev_band = ED_FREQ_BAND_INVALID;
#endif
PHY_CORE_RET uint8_t stby_state_en;
PHY_CORE_RET uint32_t compFacFiltGain20 = 0;

PHY_CORE_RET struct retrim_params retrim_data;

#ifdef OVERRIDE_TRIM_SUPPORT
PHY_CORE_RET uint8_t override_trim;
PHY_CORE_RET int32_t lb_pa_bias;
PHY_CORE_RET int32_t lb_da_bias;
PHY_CORE_RET int32_t lb_pga_bias;
PHY_CORE_RET int32_t hb_pa_bias;
PHY_CORE_RET int32_t hb_da_bias;
PHY_CORE_RET int32_t hb_pga_bias;
#endif

#define ABS(x) ((x) < 0 ? -(x) : (x))

/*!! RF Parameters*/
typedef struct __attribute__((__packed__))
{
	/*!Byte 32*/
	int8_t : 0;
	int8_t minTxPowerSupport : 7;
	uint8_t powerIncrStep : 1;
} TX_POWER_CONTROL_T;

typedef struct __attribute__((__packed__))
{
	/*!Byte7 to Byte10*/
	/*!Power detector adjustment factor for MCS7 channel 7*/
	int8_t pwrDetAdjFactLbChan1Mcs7;
	/*!Power detector adjustment factor for MCS7 channel 36*/
	int8_t pwrDetAdjFactHbChan1Mcs7 : 8;
	/*!Power detector adjustment factor for MCS7 channel 100*/
	int8_t pwrDetAdjFactHbChan2Mcs7 : 8;
	/*!Power detector adjustment factor for MCS7 channel 165*/
	int8_t pwrDetAdjFactHbChan3Mcs7 : 8;

	/*!Byte11 to Byte14*/
	/*! Systematic offset between setpower & measured power for LB.
	* syst_ofst: The systematic offset is used to eliminate any power discrepancy
	* between set power and measured power.For example, if the difference 
	* between set and measured power is 2 dB, the `syst_ofst` field will be 
	* set to 2. The RF TX gain LUT will then shift by 2 dB to compensate.
	*/
	int8_t syst_ofst : 4;
	
       /* 
	* The `dsss_tx_pwr_ofst` offset is used to eliminate any power discrepancy
	* between set power and measured power. But this cant be applied while
	* filling TX gain LUT, we need avoid the LUT changes happened because of
	* OFDM syst_offset and then apply the dsss_tx_pwr_ofst in addition to a 
	* 12 dB DSSS-specific power offset, is applied on a per-packet basis.
	*/
	int8_t dsss_tx_pwr_ofst : 4;

	/*!PSystematic offset between setpower & measured power for channel 36*/
	int8_t pwrDetAdjFactHbChan1Mcs0 : 8;
	/*!Systematic offset between setpower & measured power for channel 100*/
	int8_t pwrDetAdjFactHbChan2Mcs0 : 8;
	/*!Systematic offset between setpower & measured power for 165*/
	int8_t pwrDetAdjFactHbChan3Mcs0 : 8;
} POWER_DETECTOR_ADJUST_FACTOR_T;

void PHY_update_retrim_data(struct retrim_params *params)
{
	memcpy(&retrim_data, params, sizeof(struct retrim_params));
}

void ROM_PHY_perPacketCOnfig();

BOOL_E ROM_PHY_phyChannelSwitch(CHANNEL_BW_E channelBandWidth,
				uint32_t centreChannelNum,
				uint32_t p20Flag,
				uint32_t NRx,
				uint32_t NTx,
				uint32_t calibBitMap,
				RF_PERFORMANCE_E rfPerformance,
				uint8_t *phyRfParams,
				CHAIN_SEL_E chainSel,
				uint32_t forcedCalib,
				uint8_t *calibBuffer,
				uint8_t vbatCurrent,
				PHY_OPR_MODE phyOperMode,
				uint32_t *prodtest_trim,
				uint32_t prodctrl_disable5ghz,
				uint8_t *calibPerformed);

void ROM_PHY_phyConfig(PHY_STATE_E ctrl,
		       uint32_t NRx,
		       uint32_t NTx,
		       uint32_t calibBitMap,
		       CHAIN_SEL_E chainSel,
		       uint8_t *phyRfParams,
		       uint8_t *calibBuffer,
		       PHY_OPR_MODE phyOperMode,
		       uint32_t *prodtest_trim,
		       uint32_t prodctrl_disable5ghz);

void ROM_wlan_set_rf_power_state(
    uint32_t phyState,
    RF_CONFIG_INFO *rfConfiguration,
    RF_STATE_HIST *rf_state_hist_g,
    uint8_t *phyRfParams,
    WLAN_AFE_CONFIG_T *afeConfigParams_g,
    WLAN_AFE_BB_INTERFACE_T *afeBbInterface,
    AFE_CONFIG_INFO_T *afeConfigInfo,
    NOTCH_FILT_PARAMS *notch_filt,
    uint32_t *prodtest_trim);

void computeFreqDepOffset();

void commonPhyPatchConfigs()
{
	uint32_t regVal;
	uint32_t postfiltVal;

	/*
	 * SHEL-466 : Work around to observe Tx lock up observed with Sleep enabled.
	 * Description taken from CAL-1884
	 * 1. TX/RX switching through WSM_WLFSM_CONTROL (0x3000) register. Applicable both on FPGA and ASIC.
	 * 2. TX power update works. Gain and "TX" enable are performed at the same time.
	 */
	UCC_HW_WRITE32(ABS_EXT_SYS_WLANSYSTOP_IMG_WLAN_RX_TX_SWITCH_MODE, 0x1);

	/* CAL-1273: AACI detection threshold is tuned */
	UCC_WRITE_PERIP(ABS_PMB_WLAN_AGC_AACI_DET_THRESHOLD, 30);

	/*
	 * SHEL-1504: post filter configuration
	 * postfiltVal = (postRSfiltConfigHetb << 10) | (postRSfiltConfigHe << 8)| (postRSfiltConfigVht << 6) |
	 * (postRSfiltConfigHt << 4) | (postRSfiltConfigLg << 2) | (postRSfiltConfigDsss << 0);
	 */

	postfiltVal = 0x05AB;
	UCC_WRITE_PERIP(ABS_PMB_WLAN_MAC_CTRL_AGG_POST_RS_FILTER_CFG, postfiltVal);

	/*
	 * SHEL-1966: Enabling DPD for all MCS HESU and HETB
	 */
	regVal = 0;
	UCC_WRITE_PERIP(ABS_PMB_WLAN_MAC_CTRL_AGG_HE_DPD_BYPASS, regVal);

	upatedConfigData();
}

void patch_PHY_perPacketCOnfig()
{
	ROM_PHY_perPacketCOnfig();
	upatedConfigData();
}

void patch_dpd_convert_coeffs_to_regvalues(uint32_t *dpdreg, int32_t *aI, int32_t *aQ)
{

	/*SHEL-600 Patch*/
	aI[1] = aI[1] * 2;
	aQ[1] = aQ[1] * 2;
	aI[2] = aI[2] * 4;
	aQ[2] = aQ[2] * 4;

	ROM_dpd_convert_coeffs_to_regvalues(dpdreg, aI, aQ);
}

void patch_patchSupportForCalib(RF_CONFIG_INFO *rfConfiguration,
				uint32_t action,
				uint32_t dacTargetrms,
				uint8_t *rf_params,
				uint32_t calibBitMap,
				uint32_t forcedCalib,
				int32_t *calibBuffer,
				uint8_t otpDisb5GHzCalib)
{
	(void)action;
	(void)dacTargetrms;
	(void)calibBitMap;
	(void)forcedCalib;
	(void)calibBuffer;
	(void)otpDisb5GHzCalib;

	/*While entering into calibration set SCP_DISCARD_INITIAL_SAMPLES
	value as 7.5usec( expected value is 0x12C for 80MHz system frequency.
    Byte number 136 & 137 are used for this confuration.
	This is required for the proper loopback captures. If we change/reduce
	this value then our valid capture start point will change. This leads
	to change the loopback functions of all the calibrations. To avoid this
	scenario, set 7.5usec delay while entering the calibration.
	While exiting calibration revert this register configuration using
	the value defined in RF params.*/
	/*This function call happens before start of RF calibrations.
	Configuring discard count at this stage ensures required delay
	for current channel calibration but this configuration gets overwritten
	when SCAN channel calibration is called. To avoid this, one more call
	to RXSCP_configAdcDiscardSampleCount() function happens inside
	patch_WLANBB_channelSwitch()*/

	uint16_t txEndToRxStartDelay = 0x12C;

	RXSCP_configAdcDiscardSampleCount(txEndToRxStartDelay);

	int8_t txPowerAdj;

	POWER_DETECTOR_ADJUST_FACTOR_T *powerDetAdjFact = (POWER_DETECTOR_ADJUST_FACTOR_T *)(rf_params + POWER_DET_PARAM_OFFSET);

/*For all bands the systematic offset is 3 dB. So comment the below code till
the time we need differnt offsets for different bands.
SHEL-1148: Refer this ticket for more info related to systematic offset */
#ifdef BAND_DEP_SYST_OFFSET
	int slice = (int)rfConfiguration->freqBand;

	if (slice == 0)
	{
		txPowerAdj = powerDetAdjFact->pwrDetAdjFactLbChan1Mcs0;
	}
	else
	{
		int8_t diff1, diff2, diff3;

		diff1 = (rfConfiguration->channelNum - FIVE_GHZ_CHAN1);
		diff2 = (rfConfiguration->channelNum - FIVE_GHZ_CHAN2);
		diff3 = (rfConfiguration->channelNum - FIVE_GHZ_CHAN3);

		diff1 = ABS(diff1);
		diff2 = ABS(diff2);
		diff3 = ABS(diff3);

		// Initialize txPowerAdj to the default case
		txPowerAdj = powerDetAdjFact->pwrDetAdjFactHbChan3Mcs0;

		// Find the minimum difference
		int32_t minDiff = diff1;
		if (diff2 < minDiff)
		{
			minDiff = diff2;
		}
		if (diff3 < minDiff)
		{
			minDiff = diff3;
		}

		// Assign the appropriate txPowerAdj based on the minimum difference
		if (minDiff == diff1)
		{
			txPowerAdj = powerDetAdjFact->pwrDetAdjFactHbChan1Mcs0;
		}
		else if (minDiff == diff2)
		{
			txPowerAdj = powerDetAdjFact->pwrDetAdjFactHbChan2Mcs0;
		}
	}
#else
	txPowerAdj = powerDetAdjFact->syst_ofst;
	(void)rfConfiguration;

#endif

	uint8_t txPwrIncrStep;

	TX_POWER_CONTROL_T *txPowerCTrl = (TX_POWER_CONTROL_T *)(rf_params + TX_POWER_CONTROL_PARAMS_OFFSET);

	/*This is not required if we handle the configuration directly from LMAC and Harness*/
	txPwrIncrStep = txPowerCTrl->powerIncrStep;

	/*From RF params we will get txPwrIncrStep value as 0 for 1dBm increment and 1 for 2 dBm increment.
	The TX gain tables are having support for 0.5dB resolution. So final txPwrIncrStep should be 2 for
	1dBm power change and 4 for 2dBm power change */
	txPwrIncrStep = (txPwrIncrStep + 1) << 1;

	/*SHEL-1148: Get Systematic offset based on frequency and fill the TX LUT with new gain words. */
	RodinWlfillTXGainLUT(txPowerCTrl->minTxPowerSupport + txPowerAdj, txPwrIncrStep);
}

void patch_PHY_phyConfig(PHY_STATE_E ctrl,
			 uint32_t NRx,
			 uint32_t NTx,
			 uint32_t calibBitMap,
			 CHAIN_SEL_E chainSel,
			 uint8_t *phyRfParams,
			 uint8_t *calibBuffer,
			 PHY_OPR_MODE phyOperMode,
			 uint32_t *prodtest_trim,
			 uint32_t prodctrl_disable5ghz)
{
	/*SHEL-2414: Added fence in the Feed signal dcp driver
	Motivation is to add a missing fence to DCP code. This requires few instructions to be placed
	in the middle of DCP code. As we cannot accommodate the fix by replacing existing DCP instructions,
	we are jumping to a new GRAM location. Added new DCP code (Opcodes in dcp_patch_arr[]) at this
	new GRAM location. Same fix is required in two places of DCP code - one is in logic for "finite loop"
	and another in logic for "infinite loop".
	0xb70007b3 is the dcp code location in "infinite loop" logic from which branching to new GRAM location.
	0xb7000824 is the dcp code location in "finite loop" logic from which branching to new GRAM location.
	These addresses are obtained from DCP disassembly.
	Note that dcp opcode patch is same for finite loop and infinite loop and only jump location is difference
	so that  dcp_patch_arr1[7] changed  dcp_patch_arr1[7] = 0x83 for inifite loop branch and
	dcp_patch_arr1[7] = 0xBF for finite branch
	*/

	uint8_t dcp_patch_arr1[8];
	dcp_patch_arr1[0] = 0x48;
	dcp_patch_arr1[1] = 0x5D;
	dcp_patch_arr1[2] = 0x47;
	dcp_patch_arr1[3] = 0x5D;
	dcp_patch_arr1[4] = 0x4C;
	dcp_patch_arr1[5] = 0x00;
	dcp_patch_arr1[6] = 0x40;
	dcp_patch_arr1[7] = 0x83;

	/* infinite loop patch to the Feedsignal dcp driver */
	uint8_t *ptr;
	ptr = (uint8_t *)0xb70007b3;
	*ptr = 0x40;
	*(ptr + 1) = 0x79;
	ptr = (uint8_t *)0xb700082C;
	memcpy(ptr, dcp_patch_arr1, 8);

	/* finite loop patch to the Feedsignal dcp driver */
	ptr = (uint8_t *)0xb70007e7;
	*ptr = 0x40;
	*(ptr + 1) = 0x3d;
	dcp_patch_arr1[7] = 0xBF;
	ptr = (uint8_t *)0xb7000824;
	memcpy(ptr, dcp_patch_arr1, 8);

	//The byte offset RX_GAIN_OFFSET=24 is repurposed to indicate if it is Passive only scan or not 
	byte_24 = phyRfParams[RX_GAIN_OFFSET];
	if ((ctrl == WLAN_PHY_OPEN))
	{
		g_is_enhanced_txdc=0;
		ROM_wlanWindowRequest(START_REQ_WINDOW,
				      HIGHEST_IMPORTANCE,
				      NO);
	}

	ROM_PHY_phyConfig(ctrl,
			  NRx,
			  NTx,
			  calibBitMap,
			  chainSel,
			  phyRfParams,
			  calibBuffer,
			  phyOperMode,
			  prodtest_trim,
			  prodctrl_disable5ghz);

	if ((ctrl == WLAN_PHY_OPEN))
	{
		ROM_wlanWindowRequest(END_REQ_WINDOW,
				      HIGHEST_IMPORTANCE,
				      NO);

		if (sysParams.isWarmBoot == 0)
		{
			ROM_rf_stdby();
			stby_state_en = 1;
		}
	}
}

BOOL_E patch_PHY_phyChannelSwitch(CHANNEL_BW_E channelBandWidth,
				  uint32_t centreChannelNum,
				  uint32_t p20Flag,
				  uint32_t NRx,
				  uint32_t NTx,
				  uint32_t calibBitMap,
				  RF_PERFORMANCE_E rfPerformance,
				  uint8_t *phyRfParams,
				  CHAIN_SEL_E chainSel,
				  uint32_t forcedCalib,
				  uint8_t *calibBuffer,
				  uint8_t vbatCurrent,
				  PHY_OPR_MODE phyOperMode,
				  uint32_t *prodtest_trim,
				  uint32_t prodctrl_disable5ghz,
				  uint8_t *calibPerformed)
{
	RF_CONFIG_INFO rfConfiguration;
	BOOL_E status;
	uint32_t chan_freq;
	uint32_t is_edge_chan = 0;
	uint16_t edge_bkf_type;
	uint8_t index = 3;
	uint32_t data;

	if (centreChannelNum < 15)
	{
		index = 0;
	}
	else if (centreChannelNum <= 64)
	{
		index = 1;
	}
	else if (centreChannelNum <= 132)
	{
		index = 2;
	}

	compFacFiltGain20 = phyRfParams[PCB_LOSS_BYTE_2G_OFST + index];

#ifdef ENHANCED_TXDC_ESTIMATION
	g_is_enhanced_txdc = (calibBitMap >> 6) & 1;
#endif

	memset(&g_edge_bo_val, 0x0, sizeof(struct edge_bo_val));

	if (stby_state_en)
	{

		rfConfiguration.channelNum = centreChannelNum;
		rfConfiguration.cbw = channelBandWidth;
		rfConfiguration.freqBand = (centreChannelNum > 14) ? 1 : 0;
		rfConfiguration.nActiveTxChain = NTx;
		rfConfiguration.nActiveRxChain = NRx;
		rfConfiguration.txChainNum = 1;
		rfConfiguration.rxChainNum = 1;
		rfConfiguration.antennaSel = chainSel;

		ROM_rf_awake(&rfConfiguration);
		stby_state_en = 0;
	}

	memcpy(&g_band_edge_backoff_info, &phyRfParams[BAND_2G_LW_ED_BKF_DSSS_OFST], sizeof(struct band_edge_backoff_info));

	// is_edge_chan = check_edge_chan(centreChannelNum, &edge_bkf_type);

	for (uint8_t i = 0; i < g_num_channels; i++)
	{
		if (centreChannelNum == g_edge_chan[i].channel_num)
		{
			edge_bkf_type = g_edge_chan[i].flag;
			is_edge_chan = 1;
			break;
		}
	}

	if (is_edge_chan == 1)
	{
		chan_freq = get_chan_freq(centreChannelNum);

		// get_edge_bo(chan_freq, &edge_bkf_type);
		if (chan_freq >= 2412 && chan_freq <= 2484)
		{
			if (edge_bkf_type == BAND_LOWER_EDGE_CHANNEL)
			{
				g_edge_bo_val.dsss = g_band_edge_backoff_info.edge_bkoff_val_2g_lo.dsss;
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_2g_lo.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_2g_lo.he;
			}
			else
			{
				g_edge_bo_val.dsss = g_band_edge_backoff_info.edge_bkoff_val_2g_hi.dsss;
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_2g_hi.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_2g_hi.he;
			}
		}
		else if (chan_freq >= 5160 && chan_freq <= 5240)
		{
			if (edge_bkf_type == BAND_LOWER_EDGE_CHANNEL)
			{
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_1_lo.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_1_lo.he;
			}
			else
			{
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_1_hi.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_1_hi.he;
			}
		}
		else if (chan_freq >= 5260 && chan_freq <= 5340)
		{
			if (edge_bkf_type == BAND_LOWER_EDGE_CHANNEL)
			{
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_2a_lo.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_2a_lo.he;
			}
			else
			{
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_2a_hi.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_2a_hi.he;
			}
		}
		else if (chan_freq >= 5480 && chan_freq <= 5720)
		{
			if (edge_bkf_type == BAND_LOWER_EDGE_CHANNEL)
			{
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_2c_lo.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_2c_lo.he;
			}
			else
			{
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_2c_hi.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_2c_hi.he;
			}
		}
		else if (chan_freq >= 5745 && chan_freq <= 5825)
		{
			if (edge_bkf_type == BAND_LOWER_EDGE_CHANNEL)
			{
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_3_lo.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_3_lo.he;
			}
			else
			{
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_3_hi.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_3_hi.he;
			}
		}
		else if (chan_freq >= 5845 && chan_freq <= 5885)
		{
			if (edge_bkf_type == BAND_LOWER_EDGE_CHANNEL)
			{
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_4_lo.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_4_lo.he;
			}
			else
			{
				g_edge_bo_val.ht = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_4_hi.ht;
				g_edge_bo_val.he = g_band_edge_backoff_info.edge_bkoff_val_5g_unii_4_hi.he;
			}
		}
	}

	/*Storing CalibBitMap info to Reset DPD taps when needed*/
	calibBitMapInfo = calibBitMap;
	status = ROM_PHY_phyChannelSwitch(channelBandWidth,
					  centreChannelNum,
					  p20Flag,
					  NRx,
					  NTx,
					  calibBitMap,
					  rfPerformance,
					  phyRfParams,
					  chainSel,
					  forcedCalib,
					  calibBuffer,
					  vbatCurrent,
					  phyOperMode,
					  prodtest_trim,
					  prodctrl_disable5ghz,
					  calibPerformed);
	/* Read WL_HB0_TX_PACONF5. Will be used further for CSP package */
	paconf5_reg_val = UCC_HW_READ32(0xA4009FDC);

	/* Disabling SCP Notch0 HW control for 2.4G alone; as a result NOTCH0 is enabled for both OFDM, DSSS formats.
	 * In 5GHz Notch0 HW control is enabled. So, Notch is disabled for OFDM packets. */
	uint8_t hw_ctrl_notch_en = rfConfiguration.freqBand ? 1 : 0;
	data = UCC_READ_PERIP(ABS_PMB_SCP_NOTCH_BYPASS);
	data = (data & (~PMB_SCP_HWCONTROL_NOTCH0_MASK)) | (hw_ctrl_notch_en << PMB_SCP_HWCONTROL_NOTCH0_SHIFT);
	UCC_WRITE_PERIP(ABS_PMB_SCP_NOTCH_BYPASS,data);
	
	#ifdef COEX_SYSTEM_PATCH
	uint32_t current_band = rfConfiguration.freqBand;
	int pta_cfg_status = 0;
	if(current_band != prev_band) {
		pta_cfg_status = update_pta_config(current_band);
		(void)pta_cfg_status;
		prev_band = current_band;
	}
	#endif
	return (status);
}

uint32_t get_chan_freq(uint32_t channelNum)
{
	if (channelNum < 15)
	{
		return ((channelNum == 14) ? 2484 : (2412 + (channelNum - 1) * 5));
	}
	else
	{
		return (((channelNum <= 177) ? 5000 : 4000) + channelNum * 5);
	}
}

#define PRODTEST_FT_PROGVERSION_1 (1)
#define PRODTEST_FT_PROGVERSION_2 (2)
#define PRODTEST_FT_PROGVERSION_3 (3)

#define VERSION1_LB_PA_BIAS_BACKOFF (5)
#define VERSION1_LB_DA_BIAS_BACKOFF (0)
#define VERSION1_LB_PGA_BIAS_BACKOFF (0)

#define VERSION1_HB_PA_BIAS_BACKOFF (0)
#define VERSION1_HB_DA_BIAS_BACKOFF (0)
#define VERSION1_HB_PGA_BIAS_BACKOFF (0)

#define VERSION2_LB_PA_BIAS_BACKOFF (5)
#define VERSION2_LB_DA_BIAS_BACKOFF (0)
#define VERSION2_LB_PGA_BIAS_BACKOFF (0)

#define VERSION2_HB_PA_BIAS_BACKOFF (0)
#define VERSION2_HB_DA_BIAS_BACKOFF (0)
#define VERSION2_HB_PGA_BIAS_BACKOFF (0)

#define VERSION3_LB_PA_BIAS_BACKOFF (9)
#define VERSION3_LB_DA_BIAS_BACKOFF (3)
#define VERSION3_LB_PGA_BIAS_BACKOFF (3)

#define VERSION3_HB_PA_BIAS_BACKOFF (0)
#define VERSION3_HB_DA_BIAS_BACKOFF (0)
#define VERSION3_HB_PGA_BIAS_BACKOFF (0)

/** Package type information written to the OTP memory */
#define QFN_PACKAGE_INFO 0x5146
#define CSP_PACKAGE_INFO 0x4345

void patch_rf_applyProdCalVal(uint32_t *prodtest_trim)
{
#ifndef COEX_SYSTEM_PATCH
(void)prodtest_trim;
#else
	unsigned long int CalVal, Address;
	uint8_t Value;
	int i;
	uint32_t is5GHz;
	uint8_t temp_cnt = 0;

	uint8_t lb_pa_bias_backoff = 0;
	uint8_t lb_da_bias_backoff = 0;
	uint8_t lb_pga_bias_backoff = 0;

	uint8_t hb_pa_bias_backoff = 0;
	uint8_t hb_da_bias_backoff = 0;
	uint8_t hb_pga_bias_backoff = 0;

	/* Nordic specific*/
	unsigned int ft_prog_ver = (retrim_data.ft_prog_version >> 16) & 0xF;

	unsigned int package_type = (retrim_data.package_type_info);

	if (package_type == CSP_PACKAGE_INFO)
	{
		/*Already the backoffs are initialized to 0*/
		csp_pkg_true = 1;
	}
	else
	{
		if (ft_prog_ver == PRODTEST_FT_PROGVERSION_1)
		{
			lb_pa_bias_backoff = VERSION1_LB_PA_BIAS_BACKOFF;
			lb_da_bias_backoff = VERSION1_LB_DA_BIAS_BACKOFF;
			lb_pga_bias_backoff = VERSION1_LB_PGA_BIAS_BACKOFF;

			hb_pa_bias_backoff = VERSION1_HB_PA_BIAS_BACKOFF;
			hb_da_bias_backoff = VERSION1_HB_DA_BIAS_BACKOFF;
			hb_pga_bias_backoff = VERSION1_HB_PGA_BIAS_BACKOFF;
		}
		else if (ft_prog_ver == PRODTEST_FT_PROGVERSION_2)
		{
			lb_pa_bias_backoff = VERSION2_LB_PA_BIAS_BACKOFF;
			lb_da_bias_backoff = VERSION2_LB_DA_BIAS_BACKOFF;
			lb_pga_bias_backoff = VERSION2_LB_PGA_BIAS_BACKOFF;

			hb_pa_bias_backoff = VERSION2_HB_PA_BIAS_BACKOFF;
			hb_da_bias_backoff = VERSION2_HB_DA_BIAS_BACKOFF;
			hb_pga_bias_backoff = VERSION2_HB_PGA_BIAS_BACKOFF;
		}
		else if (ft_prog_ver == PRODTEST_FT_PROGVERSION_3)
		{
			lb_pa_bias_backoff = VERSION3_LB_PA_BIAS_BACKOFF;
			lb_da_bias_backoff = VERSION3_LB_DA_BIAS_BACKOFF;
			lb_pga_bias_backoff = VERSION3_LB_PGA_BIAS_BACKOFF;

			hb_pa_bias_backoff = VERSION3_HB_PA_BIAS_BACKOFF;
			hb_da_bias_backoff = VERSION3_HB_DA_BIAS_BACKOFF;
			hb_pga_bias_backoff = VERSION3_HB_PGA_BIAS_BACKOFF;
		}
	}

	/*NORDIC PROTECT REGION:
	  Apply backoff to the PA, DA and PGA registers based on the FT.PROGVERSION.*/
	if (prodtest_trim != NULL)
	{
		for (i = 0; i < 15; i++)
		{
			CalVal = prodtest_trim[i];

			if (((CalVal >> 8) & (0x3fff)) != 0x3fff)
			{
				temp_cnt++;
				Address = ((CalVal >> 8) >= 0x9000) ? (((CalVal >> 8) - 0x9000) & 0x3fff) : ((CalVal >> 8) & 0x3fff);
				Value = (uint8_t)(CalVal & 0xff);

				/*The adress for HB will have 0xF in bit positions 8 to 11
				and address for LB will have 0x9 in bit positions 8 to 11*/
				is5GHz = (((Address >> 8) & 0xF) == 0xF) ? 1 : 0;

				if (is5GHz) /*HB Address*/
				{
					if (Address == 0xF04) /*PA BIAS*/
					{
						Value -= hb_pa_bias_backoff;
					}
					else if (Address == 0xFD4) /*DA BIAS*/
					{
						Value -= hb_da_bias_backoff;
					}
					else if (Address == 0xFD8) /*PGA BIAS*/
					{
						Value -= hb_pga_bias_backoff;
					}
				}
				else /*LB address*/
				{
					if (Address == 0x904) /*PA BIAS*/
					{
						Value -= lb_pa_bias_backoff;
					}
					else if (Address == 0x9D4) /*DA BIAS*/
					{
						Value -= lb_da_bias_backoff;
					}
					else if (Address == 0x9D8) /*PGA BIAS*/
					{
						Value -= lb_pga_bias_backoff;
					}
				}

				UCC_HW_WRITE32(RF_REGISTER_BASE_ADDRESS + Address, Value);
			}
		}
	}

	if (temp_cnt == 0)
	{
		untrimmed_board = 1;
	}

	/*CUSTOMER PROTECT REGION:
	  Apply backoff to the PA, DA and PGA registers based on the PRODRETEST.PROGVERSION.*/
	for (i = 0; i < 15; i++)
	{
		CalVal = retrim_data.prodretest_trim[i];

		if (((CalVal >> 8) & (0x3fff)) != 0x3fff)
		{
			/*Till we get some version specific backoff info
			 or version specific RF register configuration
			 we assume that, customer also tries to configure
			 PA, DA & PGA BIAS register only. Keeping the
			 bias values to 0 sets the OTP trim values to the
			 intended register without any alteration*/
			lb_pa_bias_backoff = 0;
			lb_da_bias_backoff = 0;
			lb_pga_bias_backoff = 0;

			hb_pa_bias_backoff = 0;
			hb_da_bias_backoff = 0;
			hb_pga_bias_backoff = 0;

			Address = ((CalVal >> 8) >= 0x9000) ? (((CalVal >> 8) - 0x9000) & 0x3fff) : ((CalVal >> 8) & 0x3fff);
			Value = (uint8_t)(CalVal & 0xff);

			/*The adress for HB will have 0xF in bit positions 8 to 11
			and address for LB will have 0x9 in bit positions 8 to 11*/
			is5GHz = (((Address >> 8) & 0xF) == 0xF) ? 1 : 0;

			if (is5GHz) /*HB Address*/
			{
				if (Address == 0xF04) /*PA BIAS*/
				{
					Value -= hb_pa_bias_backoff;
				}
				else if (Address == 0xFD4) /*DA BIAS*/
				{
					Value -= hb_da_bias_backoff;
				}
				else if (Address == 0xFD8) /*PGA BIAS*/
				{
					Value -= hb_pga_bias_backoff;
				}
			}
			else /*LB address*/
			{
				if (Address == 0x904) /*PA BIAS*/
				{
					Value -= lb_pa_bias_backoff;
				}
				else if (Address == 0x9D4) /*DA BIAS*/
				{
					Value -= lb_da_bias_backoff;
				}
				else if (Address == 0x9D8) /*PGA BIAS*/
				{
					Value -= lb_pga_bias_backoff;
				}
			}

			UCC_HW_WRITE32(RF_REGISTER_BASE_ADDRESS + Address, Value);
		}
	}

#ifdef OVERRIDE_TRIM_SUPPORT
	/*Reconfigure PA, DA and PGA bias fields if override_trim is set to 1*/
	if (override_trim == 1)
	{
		UCC_HW_WRITE32(0xA4009904, lb_pa_bias);
		UCC_HW_WRITE32(0xA40099d4, lb_da_bias);
		UCC_HW_WRITE32(0xA40099d8, lb_pga_bias);
		UCC_HW_WRITE32(0xA4009f04, hb_pa_bias);
		UCC_HW_WRITE32(0xA4009fd4, hb_da_bias);
		UCC_HW_WRITE32(0xA4009fd8, hb_pga_bias);

		untrimmed_board = 0;
	}
#endif
	if(csp_pkg_true)
	{
	    uint8_t val_paldo;
	    uint32_t *paldo_regaddr = (uint32_t *)(REGEXTREG + WL_PWR_PALDO);
	    val_paldo = *paldo_regaddr;
	    val_paldo = val_paldo & 0xF;
	    val_paldo = val_paldo | 0x70;
            RodinRegWrite(WL_PWR_PALDO,val_paldo,spiData);
            rfReadWrite(spiData);
	}
#endif
}

void patch_wlan_set_rf_power_state(
    uint32_t phyState,
    RF_CONFIG_INFO *rfConfiguration,
    RF_STATE_HIST *rf_state_hist_g,
    uint8_t *phyRfParams,
    WLAN_AFE_CONFIG_T *afeConfigParams_g,
    WLAN_AFE_BB_INTERFACE_T *afeBbInterface,
    AFE_CONFIG_INFO_T *afeConfigInfo,
    NOTCH_FILT_PARAMS *notch_filt,
    uint32_t *prodtest_trim)
{
	if (phyState == PHY_OPEN)
	{
		// disbExtCapdetLogic();

		/* Bypass the Logic that determines presence of External decoupling capacitor all the times.
		 * This logic is wrongly declaring that the Decoupling capacitor is not present after the
		 * call to Standby funciton, though this is not the case. */
		// RodinRegFieldUpdate(WL_SX_LOLDOCTRL_1,1,WL_SX_LOLDOCTRL_1_BYPASS_MASK,WL_SX_LOLDOCTRL_1_BYPASS_SHIFT,spiData);
		// rfReadWrite(spiData);

		uint32_t reg_val = READ_REG_FIELD(RF_REGISTER_BASE_ADDRESS, WL_SX_LOLDOCTRL_1, WL_SX_LOLDOCTRL_1_BYPASS_MASK, WL_SX_LOLDOCTRL_1_BYPASS_SHIFT, 1);

		UCC_HW_WRITE32(RF_REGISTER_BASE_ADDRESS + WL_SX_LOLDOCTRL_1, reg_val);
	}

#ifdef OVERRIDE_TRIM_SUPPORT
	override_trim = phyRfParams[NRF_WIFI_RF_PARAMS_OFF_OVERRIDE_TRIM];

	if (override_trim == 1)
	{
		lb_pa_bias = phyRfParams[NRF_WIFI_RF_PARAMS_OFF_LB_PABIAS];
		lb_da_bias = phyRfParams[NRF_WIFI_RF_PARAMS_OFF_LB_DABIAS];
		lb_pga_bias = phyRfParams[NRF_WIFI_RF_PARAMS_OFF_LB_PGABIAS];

		hb_pa_bias = phyRfParams[NRF_WIFI_RF_PARAMS_OFF_HB_PABIAS];
		hb_da_bias = phyRfParams[NRF_WIFI_RF_PARAMS_OFF_HB_DABIAS];
		hb_pga_bias = phyRfParams[NRF_WIFI_RF_PARAMS_OFF_HB_PGABIAS];
	}
#endif

	ROM_wlan_set_rf_power_state(phyState,
				    rfConfiguration,
				    rf_state_hist_g,
				    phyRfParams,
				    afeConfigParams_g,
				    afeBbInterface,
				    afeConfigInfo,
				    notch_filt,
				    prodtest_trim);

	if (phyState == PHY_OPEN)
	{
		computeFreqDepOffset();
	}

	if (phyState == PHY_CLOSE)
	{
		freqDepOffsetCompvld = 0;
		tempTrimCompvalid = 0;
	}
}

void patch_WLANBB_channelSwitch(RF_CONFIG_INFO *rfConfiguration, uint8_t *rf_params)
{
	ROM_WLANBB_channelSwitch(rfConfiguration, rf_params);

	/*While entering into calibration set SCP_DISCARD_INITIAL_SAMPLES
	value as 7.5usec( expected value is 0x12C for 80MHz system frequency.
    Byte number 136 & 137 are used for this confuration.
	This is required for the proper loopback captures. If we change/reduce
	this value then our valid capture start point will change. This leads
	to change the loopback functions of all the calibrations. To avoid this
	scenario, set 7.5usec delay while entering the calibration.*/
	uint16_t txEndToRxStartDelay = 0x12C;

	RXSCP_configAdcDiscardSampleCount(txEndToRxStartDelay);
}

void patch_dpd_energy_detect(int16_t *edptr, uint32_t *dataIQ, int16_t ndata, RF_DC_ESTIMATE *dcest)
{
	ROM_dpd_energy_detect(edptr, dataIQ, ndata, dcest);

	edptr[0] = 174;
	dpdEdPointer = 174;
}

void patch_RXSCP_config(WLAN_PHY_RX_SCP_CONFIG_T *WLAN_PHY_RX_SCP_CONFIG,
			WLAN_PHY_RX_SCP_FILTER_COEFF_CONFIG_T *WLAN_PHY_RX_SCP_FILTER_COEFF_CONFIG,
			WLAN_AFE_FORMAT_T adcformat,
			CHANNEL_BW_E cbw,
			uint8_t ui8IQSwap,
			uint32_t *scpCntrlVal,
			WLAN_PHY_DELAY_PARAMS_T *phyDelayParams)
{
	int32_t scpNotch1Val;
	int32_t scpNotch0Val;

	/* Notch1 filter is getting disabled in RXSCP_config function,
	 * and added patch to retain the original value in Notch1 filter.
	 */

	scpNotch1Val = UCC_READ_PERIP(ABS_PMB_SCP_NOTCH_1);

	ROM_RXSCP_config(WLAN_PHY_RX_SCP_CONFIG,
			 WLAN_PHY_RX_SCP_FILTER_COEFF_CONFIG,
			 adcformat,
			 cbw,
			 ui8IQSwap,
			 scpCntrlVal,
			 phyDelayParams);

	scpNotch1Val = (scpNotch1Val & ~PMB_SCP_NOTCH0_S_MASK) | ((8 << PMB_SCP_NOTCH0_S_SHIFT) & PMB_SCP_NOTCH0_S_MASK);
	scpNotch1Val = (scpNotch1Val & ~PMB_SCP_NOTCH0_L_MASK) | ((0 << PMB_SCP_NOTCH0_L_SHIFT) & PMB_SCP_NOTCH0_L_MASK);

	UCC_WRITE_PERIP(ABS_PMB_SCP_NOTCH_1, scpNotch1Val);
	/* Notch 0 filer bandwidth changed to 15 KHz.*/
	scpNotch0Val = UCC_READ_PERIP(ABS_PMB_SCP_NOTCH_0);
	scpNotch0Val = (scpNotch0Val &~PMB_SCP_NOTCH0_S_MASK)| ((3 << PMB_SCP_NOTCH0_S_SHIFT) & PMB_SCP_NOTCH0_S_MASK);
	scpNotch0Val = (scpNotch0Val &~PMB_SCP_NOTCH0_L_MASK)| ((7 << PMB_SCP_NOTCH0_L_SHIFT) & PMB_SCP_NOTCH0_L_MASK);
	UCC_WRITE_PERIP(ABS_PMB_SCP_NOTCH_0	, scpNotch0Val);
}

/* This function expects channel number, frequency band and operating bandwidth as the
 * input parameters and check whether any Odd harmonics of the clock sources is falling
 * in our Band.
 */
uint8_t
patch_isHarmonicExists(
    CHANNEL_BW_E cbw,
    uint32_t channelNum,
    WLAN_RF_FREQ_BAND_T freqBand,
    WLAN_AFE_BB_INTERFACE_T *afeBbInterface,
    uint8_t fsSelect,
    int32_t *diffFreq)
{
	uint8_t isHarmonicPresent = 0;
	uint32_t margin_wrt_interference = 0;
	uint32_t index1 = 0;
	uint32_t cond_chk1 = 0;
	uint32_t freq1;
	uint32_t channelFreq = 0;
	uint32_t channel_bw = 20 << cbw;
	uint32_t low_freq = 0;
	uint32_t upper_freq = 0;

	channelFreq = get_chan_freq(channelNum);

	low_freq = channelFreq - (channel_bw >> 1) - margin_wrt_interference;
	upper_freq = channelFreq + (channel_bw >> 1) + margin_wrt_interference;

#ifdef ENABLE_AFE_DEBUG_DUMPS
	UCC_OUTMSG("low_freq = %d\n", low_freq);
	UCC_OUTMSG("upper_freq = %d\n", upper_freq);
	UCC_OUTMSG("channel_bw = %d\n", channel_bw);
	UCC_OUTMSG("channelFreq = %d\n", channelFreq);
#endif

	index1 = 0;
	while (!cond_chk1)
	{

		freq1 = (afeBbInterface->searchStartIndex[cbw][fsSelect][freqBand] + (index1)) *
			(afeBbInterface->adcFsSelect[cbw][fsSelect] >> afeBbInterface->adcFsShiftSel[cbw][fsSelect]);

#ifdef ENABLE_AFE_DEBUG_DUMPS
		UCC_OUTMSG("freq1 = %d\n", freq1);
		UCC_OUTMSG("afeBbInterface->searchStartIndex[cbw][fsSelect][freqBand] = %d\n", afeBbInterface->searchStartIndex[cbw][fsSelect][freqBand]);
		UCC_OUTMSG("afeBbInterface->adcFs=%d\n", afeBbInterface->adcFs);
		UCC_OUTMSG("freqBand=%d\t cbw=%d\t fsSelect=%d\n", freqBand, cbw, fsSelect);
		UCC_OUTMSG("\n");
#endif

		if ((freq1 >= low_freq) && (freq1 <= upper_freq))
		{
			*diffFreq = freq1 - channelFreq;
			isHarmonicPresent = 1;
		}
		cond_chk1 = (freq1 >= upper_freq);
		index1++;
	}
	return (isHarmonicPresent);
}

void patch_rf_rxDcCalibration(RF_CONFIG_INFO *rfConfiguration, RX_DCOC_CALIB_PARA_T *rxdcoffset, uint8_t *rfParams)
{
	ROM_rf_setrxmode();
	ROM_rf_rxDcCalibration(rfConfiguration, rxdcoffset, rfParams);
}

void patch_RodinWlanSetIntDcCalPath(WLAN_SPI_DATA_T *spiData, WLAN_RF_FREQ_BAND_T Band)
{
	uint32_t reg_val = READ_REG_FIELD(RF_REGISTER_BASE_ADDRESS, (RFSliceoffset * (int)Band) + WL_LB0_RX_BBDACCTRL, WL_LB0_RX_BBDACCTRL_DCOCDACRANGE_MASK, WL_LB0_RX_BBDACCTRL_DCOCDACRANGE_SHIFT, 0);

	UCC_HW_WRITE32(RF_REGISTER_BASE_ADDRESS + (RFSliceoffset * (int)Band) + WL_LB0_RX_BBDACCTRL, reg_val);

	ROM_RodinWlanSetIntDcCalPath(spiData, Band);
}

void sxlockchk(uint8_t *sxlockstatus)
{
	/* Read SX Status1 register and check if VCO is locked */
	*sxlockstatus = UCC_HW_READ32(REGEXTREG + WL_SX_STATUS1) & WL_SX_STATUS1_PLLLOCKDETECT_MASK;
}

void cfg_lodivldoctrlreg(uint8_t lodivldoen)
{
	uint8_t regVal;
	regVal = UCC_HW_READ32(REGEXTREG + WL_SX_LODIVLDOCTRL);
	/* Bit-0 : 0: Disable LODIV LDO, 1:Enable LODIV LDO */
	UCC_HW_WRITE32(REGEXTREG + WL_SX_LODIVLDOCTRL, ((regVal & 0xFE) | lodivldoen));
}

void cfg_vcoccal(uint8_t calen)
{
	uint8_t regVal;
	regVal = UCC_HW_READ32(REGEXTREG + WL_SX_LOGENCONF);
	/* Bit-4 : 1: VCOC Cal Enable */
	UCC_HW_WRITE32(REGEXTREG + WL_SX_LOGENCONF, ((regVal & (~WL_SX_LOGENCONF_VCOCEN_MASK)) | (calen << WL_SX_LOGENCONF_VCOCEN_SHIFT)));
}

void lodivldordychk(uint8_t *lodivldordy)
{
	uint8_t regVal;

	regVal = UCC_HW_READ32(REGEXTREG + WL_SX_STATUS0);
	/* '1' = LDO ready
	   '0' = LDO not ready
	Check status of LDO */
	*lodivldordy = (regVal & WL_SX_STATUS0_LODIVLDOREADY_MASK) >> WL_SX_STATUS0_LODIVLDOREADY_SHIFT;
}

void sxfailchknrecalib()
{
	uint8_t sxlockstatus;
	uint8_t lodivldoen, lodivldordy;
	uint32_t lodivldoPwroffwaitTime;
	uint32_t lodivldoPwronwaitTime;
	uint32_t rfpll_startupwaitTime;

	lodivldoPwroffwaitTime = 1000;
	lodivldoPwronwaitTime = 2;
	rfpll_startupwaitTime = 50;
	sxlockchk(&sxlockstatus);

	if (sxlockstatus == 0)
	{

		/*Disable LODIV LDO*/
		lodivldoen = 0;
		cfg_lodivldoctrlreg(lodivldoen);

		delayLoop(lodivldoPwroffwaitTime);

		lodivldordychk(&lodivldordy);
		/* Check if LDO is down */
		// if(lodivldordy == 1)
		{
			delayLoop(lodivldordy * lodivldoPwroffwaitTime);
		}

		/*Enable LODIV LDO*/
		lodivldoen = 1;
		cfg_lodivldoctrlreg(lodivldoen);

		delayLoop(lodivldoPwronwaitTime);
		/* Check if LDO is up */
		lodivldordychk(&lodivldordy);
		// if(lodivldordy == 0)
		{
			delayLoop((!lodivldordy) * lodivldoPwronwaitTime);
		}

		/* Trigger VCOC Calibration */
		cfg_vcoccal(1);
		delayLoop(rfpll_startupwaitTime);
		sxlockchk(&sxlockstatus);
		// if(sxlockstatus == 0)
		{
			delayLoop((!sxlockstatus) * rfpll_startupwaitTime);
		}
		/* Disable VCOC Calibration */
		cfg_vcoccal(0);
	}
}

void patch_RodinWlPllOn(WLAN_RF_FREQ_BAND_T Band, int Channel, WLAN_RF_BW_SEL ChBW)
{

	ROM_RodinWlPllOn(Band, Channel, ChBW);
	sxfailchknrecalib();
	RodinWlanTrxAlcBypassEnable(Band, Channel);
}

void RodinWlanTrxAlcBypassEnable(WLAN_RF_FREQ_BAND_T Band, int Freq)
{
	long int FreqInt, Val;
	int hbTrxAlcReg, lbTrxAlcReg;

	Freq = get_chan_freq(Freq);

	lbTrxAlcReg = UCC_HW_READ32(0xA4009730);

	hbTrxAlcReg = UCC_HW_READ32(0xA4009D30);

	FreqInt = (long int)(((1 - Band) * Freq) + Freq);

	// tuneTRX0 and tuneTRX1 controlled by ALC
	// tuneTRX0  = int(round((-0.019411 * rfFrequency + 119.872133)))
	Val = -159 * FreqInt + 986089;
	Val = Val >> 13;
	if (Val < 0)
	{
		Val = 0;
	}
	if (Val > 31)
	{
		Val = 31;
	}

	lbTrxAlcReg = (lbTrxAlcReg & (~WL_LB0_TRX_ALC_0_ALC_TUNE_BYPASS_MASK)) | (Val << WL_LB0_TRX_ALC_0_ALC_TUNE_BYPASS_SHIFT);

	lbTrxAlcReg = (lbTrxAlcReg & (~WL_LB0_TRX_ALC_0_ALC_BYPASS_MASK)) | (1 << WL_LB0_TRX_ALC_0_ALC_BYPASS_SHIFT);

	// tuneTRX1  = int(round((-0.033234 * rfFrequency + 199.722767)))
	Val = -272 * FreqInt + 1639201;
	Val = Val >> 13;
	if (Val < 0)
	{
		Val = 0;
	}
	if (Val > 31)
	{
		Val = 31;
	}

	hbTrxAlcReg = (hbTrxAlcReg & (~WL_LB0_TRX_ALC_0_ALC_TUNE_BYPASS_MASK)) | (Val << WL_LB0_TRX_ALC_0_ALC_TUNE_BYPASS_SHIFT);

	hbTrxAlcReg = (hbTrxAlcReg & (~WL_LB0_TRX_ALC_0_ALC_BYPASS_MASK)) | (1 << WL_LB0_TRX_ALC_0_ALC_BYPASS_SHIFT);

	UCC_HW_WRITE32(0xA4009730, lbTrxAlcReg);

	UCC_HW_WRITE32(0xA4009D30, hbTrxAlcReg);
}

void patch_rf_comp(RF_CONFIG_INFO *rfConfiguration, RF_CALIB_RESULT_T *rfCalibResult, uint8_t *rfParams)
{
	RF_TX_CALIB_RESULT_T txCalibRes;
	uint8_t ndpdcoefsets = 6, setindex;

	ROM_rf_comp(rfConfiguration, rfCalibResult, rfParams);

	uint32_t reg_val = READ_REG_FIELD(RF_REGISTER_BASE_ADDRESS, (RFSliceoffset * (int)rfConfiguration->freqBand) + WL_LB0_RX_BBDACCTRL, WL_LB0_RX_BBDACCTRL_DCOCDACRANGE_MASK, WL_LB0_RX_BBDACCTRL_DCOCDACRANGE_SHIFT, 0);

	UCC_HW_WRITE32(RF_REGISTER_BASE_ADDRESS + (RFSliceoffset * (int)rfConfiguration->freqBand) + WL_LB0_RX_BBDACCTRL, reg_val);

	/* Initialize DPD taps in the HW block to Unity if DPD is disabled */
	if ((!(calibBitMapInfo >> DPD_POS_IN_BITMAP)) & 0x1)
	{
		rf_paCalibInit(&txCalibRes.txDpdCalibRes[0]);
		for (setindex = 0; setindex < ndpdcoefsets; setindex++)
		{
			ROM_TXSCP_setDpdCoeffs(&txCalibRes.txDpdCalibRes[0], setindex);
		}
	}
}

void patch_PHY_getMaxPowInfo(uint8_t *phyRfParams, uint8_t rfChannel, int32_t currentTemp, uint8_t isLegacyRate, uint8_t datarateRMcs, uint8_t *maxPowVal, TX_VECTOR_PARAMS_T *phyTxVector)
{
	TX_POWER_CONTROL_T *txPowerCTrl = (TX_POWER_CONTROL_T *)(phyRfParams + TX_POWER_CONTROL_PARAMS_OFFSET);

	int8_t minTxPower = txPowerCTrl->minTxPowerSupport;

	unsigned char edge_bkf_val = 0;

	POWER_DETECTOR_ADJUST_FACTOR_T *powerDetAdjFact = (POWER_DETECTOR_ADJUST_FACTOR_T *)(phyRfParams + POWER_DET_PARAM_OFFSET);

	uint8_t dsss_bkf = powerDetAdjFact->dsss_tx_pwr_ofst;
	uint8_t ofdm_ofst = powerDetAdjFact->syst_ofst;	

	(rf_ops.getMaxPowInfo)(phyRfParams, rfChannel, currentTemp, isLegacyRate, datarateRMcs, maxPowVal);

	if (phyTxVector != NULL)
	{
		if ((phyTxVector->nonHtModulation == DSSS) && (phyTxVector->frameFormat == NON_HT_FORMAT))
		{
			edge_bkf_val = g_edge_bo_val.dsss;
		}
		else if (phyTxVector->frameFormat <= VHT_FORMAT)
		{
			edge_bkf_val = g_edge_bo_val.ht;
		}
		else
		{
			edge_bkf_val = g_edge_bo_val.he;
		}

		/* Check if the requested power is negative */
		if (phyTxVector->txPowerInDbm > 31)
		{
			phyTxVector->txPowerInDbm -= 64;
		}
		/* LMAC updates this field with the requested Tx power from Top, while providing phyTxvector to this function.
		 * This field needs a change only if the Requested power is greater than Maximum. */
		else if (phyTxVector->txPowerInDbm > *maxPowVal)
		{
			phyTxVector->txPowerInDbm = *maxPowVal;
		}

		/*CAL-2412: Subtraction of 12dB WRT power requested in Tx vector for DSSS packets. */
		if ((datarateRMcs <= ELEVEN_MBPS) && isLegacyRate)
		{
			phyTxVector->txPowerInDbm = phyTxVector->txPowerInDbm - *(phyRfParams + DSSS_POWER_OFFSET) - ofdm_ofst + dsss_bkf;
		}

		phyTxVector->txPowerInDbm -= edge_bkf_val;

		if (phyTxVector->txPowerInDbm < minTxPower)
		{
			phyTxVector->txPowerInDbm = minTxPower;
		}
	}
	else
	{
		if ((datarateRMcs <= ELEVEN_MBPS) && isLegacyRate)
		{
			*maxPowVal = *maxPowVal - *(phyRfParams + DSSS_POWER_OFFSET) - g_edge_bo_val.dsss - ofdm_ofst + dsss_bkf;
		}
		else
		{
			*maxPowVal -= g_edge_bo_val.ht;
		}

		if (*maxPowVal < minTxPower)
		{
			*maxPowVal = minTxPower;
		}
	}
}

void patch_rf_fillMaxPowInfo(uint8_t *phyRfParams, uint8_t rfChannelNum, uint8_t vbatinfo)
{
	uint8_t index;
	uint8_t slice = (rfChannelNum > MAX_CHANNEL_NUM_IN_TWO_PT_FOUR_GHZ);
	uint8_t maxPowDsss;
	uint8_t maxPowMcs0;
	uint8_t maxPowMcs7;
	uint8_t rfOptParamIndex;
	int8_t pwrBackOffVbat = 0;
	// int8_t pwrIncrWithMcs[6];
	int vbat_mon;

	(void)vbatinfo;

	/* rfOptParamIndex gives the Nearest 5GHz channel Index */
	rf_getNearestChanInd(rfChannelNum, &rfOptParamIndex);
	phyOps.PwrVbatGet(&vbat_mon);
	if (vbat_mon <= lmacConfigParams.temp_vbat_params.VthVeryLow)
	{
		/*Indices: 34 or 35*/
		pwrBackOffVbat = (int8_t)phyRfParams[VOLATGE_BASED_TX_PARAM_OFFSET + slice];
	}
	else if (vbat_mon <= lmacConfigParams.temp_vbat_params.VthLow)
	{
		/*Indices: 36 or 37*/
		pwrBackOffVbat = (int8_t)phyRfParams[VOLATGE_BASED_TX_PARAM_OFFSET + 2 + slice];
	}

	/*Right shift by 2 to take into account 0.25dB resolution of the input*/
	pwrBackOffVbat >>= 2;

	/*
	 * 15 : DSSS Max Power, 16, 17: OFDM Max power for MCS-7, MCS-0
	 * 18,21: Max Power for MCS-7,0 respectively in CH-36
	 * 19,22: Max Power for MCS-7,0 respectively in CH-100
	 * 20,23: Max Power for MCS-7,0 respectively in CH-165

	 * 32: Power backoff Info in 2.4G at Very low VBAT.
	 * 33: Power backoff Info in 5G   at Very low VBAT.
	 * 34: Power backoff Info in 2.4G at Low VBAT.
	 * 35: Power backoff Info in 5G   at Low VBAT.
	 */
	maxPowDsss = (phyRfParams[MAX_TX_POWER_OUT_OFFSET + 0]) >> 2;

	if (rfOptParamIndex == 0) // 2.4G Band
	{
		/*Right shift by 2 to take into account 0.25dB resolution of the input*/
		maxPowMcs7 = phyRfParams[MAX_TX_POWER_OUT_OFFSET + 1] >> 2;
		maxPowMcs0 = phyRfParams[MAX_TX_POWER_OUT_OFFSET + 2] >> 2;
	}
	else // 5G Band
	{
		/*Right shift by 2 to take into account 0.25dB resolution of the input*/
		maxPowMcs7 = phyRfParams[MAX_TX_POWER_OUT_OFFSET + 2 + rfOptParamIndex] >> 2;
		maxPowMcs0 = phyRfParams[MAX_TX_POWER_OUT_OFFSET + 2 + 3 + rfOptParamIndex] >> 2;
	}

	maxPowMcs7 = maxPowMcs7 + pwrBackOffVbat;
	maxPowMcs0 = maxPowMcs0 + pwrBackOffVbat;
	maxPowDsss = maxPowDsss + pwrBackOffVbat;

	memset(maxPowDsssInfo, maxPowDsss, sizeof(maxPowDsssInfo));

	/*Extract MCS Based power increments needed */
	for (index = 0; index < 3; index++)
	{
		int8_t data1, data2, index1 = (2 * index);
		/* Sign extension is done by doing <<4.
		 * 3 Bit signed value is assumed for MCS based offset.
		 */
		data1 = (phyRfParams[MCS_BASED_PWR_INCR_OFFSET + index] & 0xF) << 4;
		data2 = (phyRfParams[MCS_BASED_PWR_INCR_OFFSET + index] & 0xF0);
		// pwrIncrWithMcs[2*index]   = data1 >> 4;
		// pwrIncrWithMcs[2*index+1] = data2 >> 4;

		maxPowOfdmInfo[6 - index1] = min(maxPowMcs0, maxPowMcs7 + (data1 >> 4));
		maxPowOfdmInfo[6 - (index1 + 1)] = min(maxPowMcs0, maxPowMcs7 + (data2 >> 4));
	}

	maxPowOfdmInfo[0] = maxPowMcs0;
	maxPowOfdmInfo[7] = maxPowMcs7;
	/*
	maxPowOfdmInfo[1] = min(maxPowMcs0, maxPowMcs7 + pwrIncrWithMcs[5]);
	maxPowOfdmInfo[2] = min(maxPowMcs0, maxPowMcs7 + pwrIncrWithMcs[4]);
	maxPowOfdmInfo[3] = min(maxPowMcs0, maxPowMcs7 + pwrIncrWithMcs[3]);
	maxPowOfdmInfo[4] = min(maxPowMcs0, maxPowMcs7 + pwrIncrWithMcs[2]);
	maxPowOfdmInfo[5] = min(maxPowMcs0, maxPowMcs7 + pwrIncrWithMcs[1]);
	maxPowOfdmInfo[6] = min(maxPowMcs0, maxPowMcs7 + pwrIncrWithMcs[0]);
	*/
}

void patch_ED_configRegs(uint32_t operBw,
			 uint32_t p20Flag,
			 ED_WLAN_RF_FREQ_BAND_T freqBand,
			 uint32_t channel_number,
			 ED_UTILS_OUT_STRUCT *edUtilsOut,
			 WLAN_ED_PARAMS_T *edParams)
{

	ROM_ED_configRegs(operBw, p20Flag, freqBand, channel_number, edUtilsOut, edParams);

	uint32_t data = UCC_READ_PERIP(ABS_PMB_WLAN_ED_FILT_GAIN_COMP_FACTORS);
	UCC_WRITE_PERIP(ABS_PMB_WLAN_ED_FILT_GAIN_COMP_FACTORS, data - compFacFiltGain20);
}

BOOL_E patch_PHY_setDeactivateState()
{
        BOOL_E status;
        status = ROM_PHY_setDeactivateState();

        UCC_WRITE_PERIP(ABS_PMB_WLAN_SVD_DMA_LENGTH_EF_CLEAR, 1);
        /* DMA Pulsed Reset to comeout of  DMA job */
        captureSigDriver_dmaPulsedReset();

        return status;
}

BOOL_E patch_PHY_enterCalibMode(PHY_OPR_MODE phyMaster)
{
        BOOL_E status;
        /* SVD module reset */
        UCC_WRITE_PERIP(ABS_PMB_WLAN_SVD_RESET, (1 << PMB_WLAN_SVD_LATCHED_SOFTWARE_RESET_SHIFT));

        status = ROM_PHY_enterCalibMode(phyMaster);
        return status;
}

BOOL_E patch_PHY_exitCalibMode(uint8_t enableBbRx)
{
	BOOL_E status;

        /* Remove Reset of SVD module */
        UCC_WRITE_PERIP(ABS_PMB_WLAN_SVD_RESET,0x0);

	status = ROM_PHY_exitCalibMode(enableBbRx);
	return status;
}

void PHY_handleEntryExit(uint8_t phy_entry)
{
	uint8_t enableBbRx=1;
	BOOL_E status;

	if(phy_entry)
	{
		status = patch_PHY_enterCalibMode(MAC_CONTROLLED_OPERATION);
	}
	else
	{
		status = patch_PHY_exitCalibMode(enableBbRx);
	}
	(void) status;
}

