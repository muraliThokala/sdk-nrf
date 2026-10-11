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
