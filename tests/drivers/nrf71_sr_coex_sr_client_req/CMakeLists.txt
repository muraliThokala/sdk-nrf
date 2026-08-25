# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf71_sr_coex_sr_client_req)

target_sources(app PRIVATE
  src/main.c
  src/coex_tb_util.c
  src/coex_tb_params.c
  src/coex_tb_stats.c
  src/coex_tb_tests.c
)

# Coexistence driver public API (coex_cd_*).
target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/nrf71_sr_coex_sr_client_req/include
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/fw_if
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/osal/hw_if/hal/inc
)
