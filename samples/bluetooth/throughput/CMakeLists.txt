#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(DTC_OVERLAY_FILE "dts.overlay")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)


# TODO: MbedTLS Remove this workaround
target_include_directories(mbedcrypto_common
        INTERFACE
        ${CMAKE_SOURCE_DIR}
)

FILE(GLOB app_sources src/*.c)
# NORDIC SDK APP START
target_sources(app PRIVATE
	${app_sources}
)
# NORDIC SDK APP END

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
