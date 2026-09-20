/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_spi.h
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
 * STM32N6 SPI driver header.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_SPI_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_SPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SPI mode flags (standard SPI CPOL/CPHA) */

#define SPI_CPOL         (1 << 0)
#define SPI_CPHA         (1 << 1)
#define SPI_MODE_0       0
#define SPI_MODE_1       SPI_CPHA
#define SPI_MODE_2       SPI_CPOL
#define SPI_MODE_3       (SPI_CPOL | SPI_CPHA)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Initialize SPI bus
 * @param bus_num SPI instance (1 or 2)
 * @return 0 on success
 */

int stm32n6_spi_initialize(int bus_num);

/**
 * @brief SPI transfer (full duplex)
 * @param bus_num SPI instance
 * @param tx TX buffer (NULL = send 0xFF)
 * @param rx RX buffer (NULL = discard)
 * @param len Transfer length in bytes
 * @return 0 on success
 */

int stm32n6_spi_transfer(int bus_num, const uint8_t *tx,
                           uint8_t *rx, uint32_t len);

/**
 * @brief Deinitialize SPI bus
 * @param bus_num SPI instance
 */

void stm32n6_spi_deinit(int bus_num);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_SPI_H */
