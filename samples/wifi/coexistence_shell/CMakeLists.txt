#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_sr_coex)

target_sources(app PRIVATE
	src/sr_coex_main.c
	src/sr_coex_shell.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

# coex_cd_sr_power_notify() is declared in the Wi-Fi driver's firmware interface headers.
if(CONFIG_NRF71_SR_COEX_DRIVER)
  zephyr_library_include_directories(${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/inc)
endif()
