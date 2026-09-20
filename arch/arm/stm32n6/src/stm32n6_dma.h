/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_dma.h
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
 * STM32N6 GPDMA driver header.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_DMA_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_DMA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Initialize DMA channel
 * @param channel Channel number (0-15)
 * @return 0 on success
 */

int stm32n6_dma_init(int channel);

/**
 * @brief Start DMA transfer
 * @param channel Channel number
 * @param src Source address
 * @param dst Destination address
 * @param size Transfer size in bytes
 * @return 0 on success
 */

int stm32n6_dma_start(int channel, uint32_t src, uint32_t dst,
                       uint32_t size);

/**
 * @brief Start a peripheral-to-memory DMA transfer
 * @param channel Channel number (0-15)
 * @param paddr Peripheral source address (fixed, e.g. ADC data register)
 * @param maddr Memory destination address (incrementing)
 * @param size Transfer size in bytes
 * @param request GPDMA hardware request line (e.g. ADC2 = 8)
 * @return 0 on success
 */

int stm32n6_dma_start_p2m(int channel, uint32_t paddr, uint32_t maddr,
                          uint32_t size, uint8_t request);

/**
 * @brief Start a memory-to-peripheral DMA transfer
 * @param channel Channel number (0-15)
 * @param maddr Memory source address (incrementing)
 * @param paddr Peripheral destination address (fixed, e.g. SAI data register)
 * @param size Transfer size in bytes
 * @param request GPDMA hardware request line (e.g. SAI1_A = 91)
 * @param width   Transfer width as a log2 byte count: 0 byte, 1 halfword,
 *                2 word.  The SAI wants halfwords at 16 bits per sample.
 * @return 0 on success
 */

int stm32n6_dma_start_m2p(int channel, uint32_t maddr, uint32_t paddr,
                          uint32_t size, uint8_t request, uint8_t width);

/**
 * @brief Wait for DMA transfer complete
 * @param channel Channel number
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on complete, -ETIMEDOUT on timeout
 */

int stm32n6_dma_wait(int channel, int timeout_ms);

/**
 * @brief Deinitialize DMA channel
 * @param channel Channel number
 */

void stm32n6_dma_deinit(int channel);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_DMA_H */
