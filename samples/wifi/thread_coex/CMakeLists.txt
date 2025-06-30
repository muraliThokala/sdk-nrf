#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_thread_coex)

target_sources(app PRIVATE
	src/zperf_utils.c
	src/thread_coex_main.c
	src/thread_coex_shell.c
)

# For net_private.h
target_include_directories(app PUBLIC
	${ZEPHYR_BASE}/subsys/net/ip
)

