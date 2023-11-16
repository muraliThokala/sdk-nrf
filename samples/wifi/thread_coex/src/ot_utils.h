/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_UTILS_H_
#define OT_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

#include <openthread/platform/radio.h>

#define TXPOWER_INIT_VALUE 127
#define RSSI_INIT_VALUE 127

/**
 * Initialize OT throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_throughput_test_init(bool is_ot_client);

/**
 * @brief Run OT throughput test
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
 * @brief Exit OT throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_tput_test_exit(void);
/**
 * Initialize OT scan --> connection test
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
 * @brief Start OT advertisement
 *
 * @return None
 */
void adv_start(void);

/**
 * @brief Start OT scan
 *
 * @return None
 */
void scan_start(void);

/**
 * @brief Read OT RSSI
 *
 * @return None
 */
void read_conn_rssi(uint16_t handle, int8_t *rssi);

/**
 * @brief Set OT Tx power
 *
 * @return None
 */
void set_tx_power(uint8_t handle_type, uint16_t handle, int8_t tx_pwr_lvl);

/**
 * @brief Get OT Tx power
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
 * @brief Start openThread commissioner
 *
 * @return None.
 */
void thread_start_commissioner(const char* psk, const otExtAddress* allowed_eui);

/**
 * @brief Stop openThread commissioner
 *
 * @return None.
 */
void thread_stop_commissioner(void);

/**
 * @brief Start openThread joiner
 *
 * @return None.
 */
void thread_start_joiner(const char *pskd);
//void thread_start_joiner(const char *pskd, otInstance *instance);
/* void thread_start_joiner(const char *pskd, otInstance *instance, struct openthread_context *context); */

/**
 * @brief Stop openThread joiner
 *
 * @return None.
 */
void thread_stop_joiner(void);

/**
 * @brief Set openThread network key to null
 *
 * @return None.
 */
void ot_setNullNetworkKey(otInstance *aInstance);

#endif /* OT_UTILS_H_ */
