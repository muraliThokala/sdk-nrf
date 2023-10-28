/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef THREAD_UTILS_H_
#define THREAD_UTILS_H_


#include <stdbool.h>
#include <openthread/platform/radio.h>

/**
 * Initialize Thread throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int thread_throughput_test_init(bool is_thread_client);

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
int thread_throughput_test_exit(void);

/**
 * @brief Check state of the thread device
 *
 * @return None.
 */
const char* check_ot_state(void);

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
 * @brief Stop openThread joiner
 *
 * @return None.
 */
//void thread_start_joiner(const char *pskd);
void thread_start_joiner(const char *pskd, otInstance *instance);
/* void thread_start_joiner(const char *pskd, otInstance *instance, struct openthread_context *context); */

/**
 * @brief Set openThread network key to null
 *
 * @return None.
 */
void setNullNetworkKey(otInstance *aInstance);

#endif /* THREAD_UTILS_H_ */
