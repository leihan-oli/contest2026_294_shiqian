/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_xspi.h
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
 * STM32N6 XSPI driver header.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_XSPI_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_XSPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Initialize XSPI peripheral
 * @param bus_num XSPI instance (1 or 2)
 * @return 0 on success
 */

int stm32n6_xspi_initialize(int bus_num);

/**
 * @brief Enable memory-mapped mode for XIP
 * @param bus_num XSPI instance
 * @return 0 on success
 */

int stm32n6_xspi_enable_mmap(int bus_num);

/**
 * @brief Read data from XSPI Flash
 * @param bus_num XSPI instance
 * @param offset Flash offset
 * @param buf Destination buffer
 * @param len Length to read
 * @return bytes read, or negative errno
 */

int stm32n6_xspi_read(int bus_num, uint32_t offset,
                       void *buf, uint32_t len);

/**
 * @brief Deinitialize XSPI peripheral
 * @param bus_num XSPI instance
 */

void stm32n6_xspi_deinit(int bus_num);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_XSPI_H */
