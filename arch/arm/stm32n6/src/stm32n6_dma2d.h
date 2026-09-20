/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_dma2d.h
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
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_DMA2D_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_DMA2D_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_STM32_DMA2D

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_dma2d_probe
 *
 * Description:
 *   One-shot DMA2D register-to-memory write probe.  Grants DMA2D the CID
 *   the firmware SRAM region already trusts, fills a known word into SRAM
 *   via DMA2D, and verifies the bytes landed.  Determines whether DMA2D can
 *   write SRAM in DEV boot or is firewalled by RISAF.
 *
 * Returned Value:
 *   Zero (OK) if DMA2D wrote the fill word into SRAM; a negated errno
 *   otherwise.
 *
 ****************************************************************************/

int stm32n6_dma2d_probe(void);

#endif /* CONFIG_STM32_DMA2D */
#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_DMA2D_H */
