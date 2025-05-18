#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_ble_coex)

target_sources(app PRIVATE
	src/ble_coex_main.c
	src/ble_coex_shell.c
)

# For net_private.h
target_include_directories(app PUBLIC
	${ZEPHYR_BASE}/subsys/net/ip
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
