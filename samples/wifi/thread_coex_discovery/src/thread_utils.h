/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef THREAD_UTILS_H_
#define THREAD_UTILS_H_


#include <stdbool.h>

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


#endif /* THREAD_UTILS_H_ */
