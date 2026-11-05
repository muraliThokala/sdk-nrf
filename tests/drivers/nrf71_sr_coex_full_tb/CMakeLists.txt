#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# The WICR LMAC/UMAC ROM patch addresses must point at the patch regions, or
# the RPU boots unpatched ROM, which has no CM2CD command handling. The dtsi
# must match the patch selected through WIFI_NRF71_PATCH_VERSION. Do not also
# pass a patch dtsi through EXTRA_DTC_OVERLAY_FILE.
list(APPEND EXTRA_DTC_OVERLAY_FILE
     ${CMAKE_CURRENT_LIST_DIR}/../../../../nrfxlib/nrf71_wifi/bins/0.1.0/nrf7120_wifi_patch.dtsi)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf71_sr_coex)

target_sources(app PRIVATE
  src/main.c
  src/coex_tb_util.c
  src/coex_tb_params.c
  src/coex_tb_stats.c
  src/coex_tb_tests.c
)

# The coexistence driver application API is the public header
# <drivers/nrf71_sr_coex/nrf71_sr_coex.h>. The bench also drives the CD2CM command
# builders, COEXC programming, BLE channel mapping and statistics through the
# driver's internal header nrf71_sr_coex_internal.h. The Wi-Fi directories give
# access to the Wi-Fi firmware interface headers (CD2CM/CM2CD message types).
target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/nrf71_sr_coex/src
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/inc
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/inc/common/fw_if
)
