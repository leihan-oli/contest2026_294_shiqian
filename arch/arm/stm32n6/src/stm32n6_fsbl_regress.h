/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_fsbl_regress.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_FSBL_REGRESS_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_FSBL_REGRESS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_STM32_FSBL_REGRESS

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_fsbl_regress
 *
 * Description:
 *   One-shot FSBL regression probe.  Verifies on real silicon that the
 *   custom FSBL's widened RISAF2/3 windows (AXISRAM1/2 open to every CID)
 *   and the Sleep clock keep-alive (LTDCLPEN + MEMLPENR) did not break the
 *   other CID0 bus masters that share SRAM: DMA2D R2M writes and a GPDMA1
 *   memory-to-memory copy, plus a read-back dump of the RIF/LPENR
 *   configuration.
 *
 * Returned Value:
 *   Zero (OK) if every probe landed; a negated errno otherwise.
 *
 ****************************************************************************/

int stm32n6_fsbl_regress(void);

#endif /* CONFIG_STM32_FSBL_REGRESS */
#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_FSBL_REGRESS_H */
