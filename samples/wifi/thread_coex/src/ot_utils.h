/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_UTILS_H_
#define OT_UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "zephyr/net/openthread.h"

#include <openthread/instance.h>
#include <version.h>
#include <openthread/config.h>
#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/error.h>
#include <openthread/joiner.h>
#include <openthread/link.h>
#include <openthread/tasklet.h>
#include <openthread/platform/logging.h>
#include <openthread/dataset_ftd.h>
#include <openthread/thread.h>

#include <openthread/platform/radio.h>

#define TXPOWER_INIT_VALUE 127
#define RSSI_INIT_VALUE 127



/**
 * Initialize Thread throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_throughput_test_init(bool is_ot_client);

/**
 * @brief Run Thread throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_throughput_test_run(void);


/**
 * @brief Run OpenThread discovery test
 */
void ot_discovery_test_run(void);

/**
 * @brief Run OpenThread connection test
 */
void ot_conn_test_run(void);

/**
 * @brief Exit Thread throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_tput_test_exit(void);
/**
 * Initialize Thread scan --> connection test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_connection_init(bool is_ot_client);

/**
 * @brief Initialization for BT connection
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_disconnect_client(void);

/**
 * @brief Start Thread advertisement
 *
 * @return None
 */
void adv_start(void);

/**
 * @brief Start Thread scan
 *
 * @return None
 */
void scan_start(void);

/**
 * @brief Read Thread RSSI
 *
 * @return None
 */
void read_conn_rssi(uint16_t handle, int8_t *rssi);

/**
 * @brief Set Thread Tx power
 *
 * @return None
 */
void set_tx_power(uint8_t handle_type, uint16_t handle, int8_t tx_pwr_lvl);

/**
 * @brief Get Thread Tx power
 *
 * @return None
 */
void get_tx_power(uint8_t handle_type, uint16_t handle, int8_t *tx_pwr_lvl);

//========================================================================================== Thread

/**
 * @brief Start OpenThread discovery
 *
 * @return None
 */
void ot_start_discovery(void);


/**
 * Initialize Thread device
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_initialization(void);

/**
 * @brief Run Thread throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int thread_throughput_test_run(void);

/**
 * @brief Exit Thread throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_device_disable(void);

/**
 * @brief Check state of the thread device
 *
 * @return None.
 */
const char* ot_check_device_state(void);


/**
 * @brief Start thread commissioner
 *
 * @return None.
 */
void thread_start_commissioner(const char* psk, const otExtAddress* allowed_eui);

/**
 * @brief Stop thread commissioner
 *
 * @return None.
 */
void thread_stop_commissioner(void);

/**
 * @brief Start thread joiner
 *
 * @return None.
 */
void ot_start_joiner(const char *pskd);
//void ot_start_joiner(const char *pskd, otInstance *instance);
/* void ot_start_joiner(const char *pskd, otInstance *instance, struct openthread_context *context); */

/**
 * @brief Stop thread joiner
 *
 * @return None.
 */
void ot_stop_joiner(void);

/**
 * @brief Set thread network key to null
 *
 * @return None.
 */
void ot_setNullNetworkKey(otInstance *aInstance);

void get_peer_address(uint64_t timeout_ms);

void start_zperf_test_send(const char *peer_addr, uint32_t duration_sec, uint32_t packet_size_bytes, uint32_t rate_bps);
void start_zperf_test_recv();
void zperf_test();

#endif /* OT_UTILS_H_ */
