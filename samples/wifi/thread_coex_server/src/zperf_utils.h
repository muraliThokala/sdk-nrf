/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <zephyr/kernel.h>


#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/zperf.h>



int zperf_upload(const char *peer_addr, uint32_t duration_sec, uint32_t packet_size_bytes,
                 uint32_t rate_bps, bool is_ot_zperf_udp);

int zperf_download(bool is_udp);
