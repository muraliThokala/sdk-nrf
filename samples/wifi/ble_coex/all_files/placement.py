# Allocate some ROM for the application.

import sys
topdir = os.path.join(UNIT_DIR, '..')
sys.path.append(os.path.join(topdir, 'loader','build','smake'))

app = App.Get('lmac_system')
core = app.core
RAM_A = core.rams[0] #0x7800 - retension

# Allocate a place in memory for the patchTable and link buggy functions with their patch
romFile = os.path.abspath(os.path.join(topdir, '..', '..', 'lmac', 'loader', 'build', 'smake', os.environ["CONFIG"] + '_MIPSGCC','LMAC_APP.elf'))
app.romToPatch = romFile
# Define where to place the patch binary image in retention ram.
#app.patchRAM = [CoreMemRegion(RAM_A, 0, 0x400 - 1)]
#0x4000 ==> Patch Binary
#0x9000 ==> Patch BIMG
#0x0 to 0x5FFF ==> Lmac Retention Variables Placed
app.patchRAM = [CoreMemRegion(RAM_A, 0x3a80, 0xBc00 - 1)]
# Replace the old function with a patched one.
#app.romFuncsToPatch["smake_generated_main"] = "sram_smake_generated_main"
#pp.romFuncsToPatch["lmacInit"] = "sram_lmacInit"
app.romFuncsToPatch["processRespPacket"] = "patch_processRespPacket"
app.romFuncsToPatch["TxAckTimeout"] = "patch_TxAckTimeout"
app.romFuncsToPatch["waitForCurrentTxDoneAndBlockTx"] = "patch_waitForCurrentTxDoneAndBlockTx"
app.romFuncsToPatch["retrieveFramesFromAp"] = "patch_retrieveFramesFromAp"
app.romFuncsToPatch["triggerChannelSwitch"] = "patch_triggerChannelSwitch"
app.romFuncsToPatch["PHY_cmWindowRequest"] = "patch_PHY_cmWindowRequest"
app.romFuncsToPatch["processHostCmd"] = "patch_processHostCmd"
app.romFuncsToPatch["defragRcvFrame"] = "patch_defragRcvFrame"
#app.romFuncsToPatch["macCtrlInit"] = "patch_macCtrlInit"
app.romFuncsToPatch["wlanWindowRequest"] = "patch_wlanWindowRequest"
app.romFuncsToPatch["processVifConfig"] = "patch_processVifConfig"
app.romFuncsToPatch["lmacTask"] = "patch_lmacTask"
app.romFuncsToPatch["getLTCFreezeTime"] = "patch_getLTCFreezeTime"
app.romFuncsToPatch["updateCurrTWT"] = "patch_updateCurrTWT"
app.romFuncsToPatch["processConfigTWTCmd"] = "patch_processConfigTWTCmd"
app.romFuncsToPatch["calTWTSP"] = "patch_calTWTSP"
app.romFuncsToPatch["processTWTSleepRequest"] = "patch_processTWTSleepRequest"
app.romFuncsToPatch["processTWTSleepResponse"] = "patch_processTWTSleepResponse"
app.romFuncsToPatch["patchableTxVect"] = "patch_patchableTxVect"
app.romFuncsToPatch["PHY_perPacketCOnfig"] = "patch_PHY_perPacketCOnfig"
app.romFuncsToPatch["dpd_convert_coeffs_to_regvalues"] = "patch_dpd_convert_coeffs_to_regvalues"
app.romFuncsToPatch["patchSupportForCalib"] = "patch_patchSupportForCalib"
app.romFuncsToPatch["PHY_patchPlaceHolder"] = "patch_PHY_patchPlaceHolder"
app.romFuncsToPatch["measureTempAndVbat"] = "patch_measureTempAndVbat"
app.romFuncsToPatch["RodinWlanFreqDepSettings"] = "patch_RodinWlanFreqDepSettings"
app.romFuncsToPatch["wlan_set_rf_power_state"] = "patch_wlan_set_rf_power_state"
app.romFuncsToPatch["prepareTxCryptoCmd"] = "patch_prepareTxCryptoCmd"
app.romFuncsToPatch["WLANBB_channelSwitch"] = "patch_WLANBB_channelSwitch"
app.romFuncsToPatch["dpd_energy_detect"] = "patch_dpd_energy_detect"
app.romFuncsToPatch["RXSCP_config"]      = "patch_RXSCP_config"
app.romFuncsToPatch["isHarmonicExists"]  = "patch_isHarmonicExists"
app.romFuncsToPatch["getRxKeyInfo"]="patch_getRxKeyInfo"
app.romFuncsToPatch["rf_rxDcCalibration"]  = "patch_rf_rxDcCalibration"
app.romFuncsToPatch["RodinWlanSetIntDcCalPath"]  = "patch_RodinWlanSetIntDcCalPath"
app.romFuncsToPatch["RodinWlPllRetune"]  = "patch_RodinWlPllRetune"
#app.romFuncsToPatch["RodinWlPllOn"]  = "patch_RodinWlPllOn"
app.romFuncsToPatch["rf_comp"]  = "patch_rf_comp"
app.romFuncsToPatch["PHY_getMaxPowInfo"] = "patch_PHY_getMaxPowInfo"
app.romFuncsToPatch["rf_fillMaxPowInfo"]  = "patch_rf_fillMaxPowInfo"

# coexistence related patch functions
app.romFuncsToPatch["CM_initialization"] = "patch_CM_initialization"
app.romFuncsToPatch["CM_coexProcessCmd"] = "patch_CM_coexProcessCmd"
app.romFuncsToPatch["CM_dynamicConfigCoexHardware"] = "patch_CM_dynamicConfigCoexHardware"
app.romFuncsToPatch["CM_restoreConfig"] = "patch_CM_restoreConfig"
app.romFuncsToPatch["CM_checkCollisionAllocateWindow"] = "patch_CM_checkCollisionAllocateWindow"

app.romFuncsToPatch["resetCmdProcess"] = "patch_resetCmdProcess"
app.romFuncsToPatch["rxBeaconProcess"] = "patch_rxBeaconProcess"
#app.romFuncsToPatch["setupSysParams"] = "patch_setupSysParams"
app.romFuncsToPatch["generalPurposeTimerIsr"] = "patch_generalPurposeTimerIsr"
app.romFuncsToPatch["flushHETBACList"] = "patch_flushHETBACList"
app.romFuncsToPatch["TxengEdcaISR"] = "patch_TxengEdcaISR"
app.romFuncsToPatch["lmacCommandProc"] = "patch_lmacCommandProc"
app.romFuncsToPatch["rf_applyProdCalVal"] = "patch_rf_applyProdCalVal"
app.romFuncsToPatch["PHY_phyConfig"] = "patch_PHY_phyConfig"
app.romFuncsToPatch["PHY_phyChannelSwitch"] = "patch_PHY_phyChannelSwitch"
app.romFuncsToPatch["getAggregationThreshold"] = "patch_getAggregationThreshold"
app.romFuncsToPatch["processTriggerFrame"] = "patch_processTriggerFrame"
app.romFuncsToPatch["wlan_prepare_txvector"] = "patch_wlan_prepare_txvector"
app.romFuncsToPatch["processTxCmd"] = "patch_processTxCmd"
app.romFuncsToPatch["nw_lost_check_func"] = "patch_nw_lost_check_func"
#app.romFuncsToPatch["RodinWlanLdoEn"] = "patch_RodinWlanLdoEn"
app.romFuncsToPatch["updateTWTPowerBit"] = "patch_updateTWTPowerBit"
app.romFuncsToPatch["filterAndProcessMpdu"] = "patch_filterAndProcessMpdu"
app.romFuncsToPatch["setGenPurposTimer"] = "patch_setGenPurposTimer"

app.romFuncsToPatch["sendTxDoneToUmac"]  = "patch_sendTxDoneToUmac"
app.romFuncsToPatch["waitForUmacBootComplete"] = "patch_waitForUmacBootComplete"
app.romFuncsToPatch["assertSleepRequest"] = "patch_assertSleepRequest"
app.romFuncsToPatch["lmacRxIsr"] = "patch_lmacRxIsr"
app.romFuncsToPatch["configureInputAddrQ"]="patch_configureInputAddrQ"
app.romFuncsToPatch["phyToActiveState"]="patch_phyToActiveState"
app.romFuncsToPatch["processSifsFrame"]="patch_processSifsFrame"
app.romFuncsToPatch["prepareRxEventParams"]="patch_prepareRxEventParams"
app.romFuncsToPatch["processDecrypt"]="patch_processDecrypt"
app.romFuncsToPatch["checkLmacActivity"]="patch_checkLmacActivity"
app.romFuncsToPatch["processRetry"]="patch_processRetry"
app.romFuncsToPatch["updateRateInfo"]="patch_updateRateInfo"
#app.romFuncsToPatch["handleSleepTimer"]="patch_handleSleepTimer"
#app.romFuncsToPatch["lmac_reconfig"]="patch_lmac_reconfig"
app.romFuncsToPatch["ED_configRegs"]="patch_ED_configRegs"
app.romFuncsToPatch["rf_txDcCalibration"]="patch_rf_txDcCalibration"
app.romFuncsToPatch["WLANAPI_txDcLutMode"]="patch_WLANAPI_txDcLutMode"
app.romFuncsToPatch["WLANAPI_enableTSSIandRX"]="patch_WLANAPI_enableTSSIandRX"
app.romFuncsToPatch["WLANAPI_disableTSSIandRX"]="patch_WLANAPI_disableTSSIandRX"
app.romFuncsToPatch["PHY_setDeactivateState"]="patch_PHY_setDeactivateState"
app.romFuncsToPatch["PHY_enterCalibMode"]="patch_PHY_enterCalibMode"
app.romFuncsToPatch["PHY_exitCalibMode"]="patch_PHY_exitCalibMode"



print App.Get('lmac_system').layout
print "App patched."

#Debug Logs
print hex(RAM_A.baseAddr)
print hex(RAM_A.endAddr)
print RAM_A.name
print hex(RAM_A.size)

