/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_fdcan.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.  See the License for the specific language governing
 * permissions and limitations under the License.
 *
 * STM32N6 FDCAN driver header.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_FDCAN_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_FDCAN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Initialize FDCAN peripheral
 * @param bus_num FDCAN instance (1 or 2)
 * @param bitrate CAN bitrate (500000 or 1000000)
 * @return 0 on success
 */

int stm32n6_fdcan_initialize(int bus_num, uint32_t bitrate);

/**
 * @brief Send a CAN frame
 * @param bus_num FDCAN instance
 * @param id CAN ID (11-bit standard)
 * @param data Data bytes (max 8)
 * @param len Data length
 * @return 0 on success
 */

int stm32n6_fdcan_send(int bus_num, uint32_t id,
                        const uint8_t *data, uint8_t len);

/**
 * @brief Receive a CAN frame
 * @param bus_num FDCAN instance
 * @param id Output CAN ID
 * @param data Output data buffer
 * @param len Output data length
 * @param timeout_ms Timeout in milliseconds (-1 = wait forever)
 * @return 0 on success, -EAGAIN if no message
 */

int stm32n6_fdcan_receive(int bus_num, uint32_t *id,
                           uint8_t *data, uint8_t *len,
                           int timeout_ms);

/**
 * @brief Deinitialize FDCAN peripheral
 * @param bus_num FDCAN instance
 */

void stm32n6_fdcan_deinit(int bus_num);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_FDCAN_H */
