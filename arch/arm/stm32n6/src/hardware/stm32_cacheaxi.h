/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_cacheaxi.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_CACHEAXI_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_CACHEAXI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register offsets (RM0486 "AXI cache" (CACHEAXI) chapter).  Offsets are
 * given relative to STM32_CACHEAXI_BASE.
 */

#define STM32_CACHEAXI_CR1_OFFSET        0x0000  /* Control register 1 */
#define STM32_CACHEAXI_SR_OFFSET         0x0004  /* Status register */
#define STM32_CACHEAXI_IER_OFFSET        0x0008  /* Interrupt enable register */
#define STM32_CACHEAXI_FCR_OFFSET        0x000c  /* Flag clear register */
#define STM32_CACHEAXI_CR2_OFFSET        0x0100  /* Control register 2 */
#define STM32_CACHEAXI_CMDRSADDRR_OFFSET 0x0104  /* Command start address */
#define STM32_CACHEAXI_CMDREADDRR_OFFSET 0x0108  /* Command end address */

#define STM32_CACHEAXI_CR1           (STM32_CACHEAXI_BASE + STM32_CACHEAXI_CR1_OFFSET)
#define STM32_CACHEAXI_SR            (STM32_CACHEAXI_BASE + STM32_CACHEAXI_SR_OFFSET)
#define STM32_CACHEAXI_IER           (STM32_CACHEAXI_BASE + STM32_CACHEAXI_IER_OFFSET)
#define STM32_CACHEAXI_FCR           (STM32_CACHEAXI_BASE + STM32_CACHEAXI_FCR_OFFSET)
#define STM32_CACHEAXI_CR2           (STM32_CACHEAXI_BASE + STM32_CACHEAXI_CR2_OFFSET)
#define STM32_CACHEAXI_CMDRSADDRR    (STM32_CACHEAXI_BASE + STM32_CACHEAXI_CMDRSADDRR_OFFSET)
#define STM32_CACHEAXI_CMDREADDRR    (STM32_CACHEAXI_BASE + STM32_CACHEAXI_CMDREADDRR_OFFSET)

/* CACHEAXI_CR1 - control register 1 */

#define CACHEAXI_CR1_EN              (1 << 0)   /* AXI cache enable */
#define CACHEAXI_CR1_CACHEINV        (1 << 1)   /* Invalidate whole cache */

/* CACHEAXI_SR - status register */

#define CACHEAXI_SR_BUSYF            (1 << 0)   /* Cache busy flag */
#define CACHEAXI_SR_BSYENDF          (1 << 1)   /* Busy end flag */
#define CACHEAXI_SR_ERRF             (1 << 2)   /* Cache error flag */
#define CACHEAXI_SR_BUSYCMDF         (1 << 3)   /* Command busy flag */
#define CACHEAXI_SR_CMDENDF          (1 << 4)   /* Command end flag */

#define CACHEAXI_SR_BUSY             (CACHEAXI_SR_BUSYF | CACHEAXI_SR_BUSYCMDF)

/* CACHEAXI_IER - interrupt enable register */

#define CACHEAXI_IER_BSYENDIE        (1 << 1)   /* Busy end interrupt enable */
#define CACHEAXI_IER_ERRIE           (1 << 2)   /* Error interrupt enable */
#define CACHEAXI_IER_CMDENDIE        (1 << 4)   /* Command end interrupt enable */

/* CACHEAXI_FCR - flag clear register (write 1 to clear) */

#define CACHEAXI_FCR_CBSYENDF        (1 << 1)   /* Clear busy end flag */
#define CACHEAXI_FCR_CERRF           (1 << 2)   /* Clear error flag */
#define CACHEAXI_FCR_CCMDENDF        (1 << 4)   /* Clear command end flag */

/* CACHEAXI_CR2 - control register 2 */

#define CACHEAXI_CR2_STARTCMD        (1 << 0)   /* Start the range command */
#define CACHEAXI_CR2_CACHECMD_SHIFT  (1)        /* Maintenance operation, 2 bits */
#define CACHEAXI_CR2_CACHECMD_MASK   (3 << CACHEAXI_CR2_CACHECMD_SHIFT)

#define CACHEAXI_CMD_CLEAN           (1 << 1)   /* Clean by address range */
#define CACHEAXI_CMD_CLEAN_INVALID   (3 << 1)   /* Clean+invalidate range */

/* Software constraints of the range commands
 *
 *   - start address must be aligned to a cache line (32 bytes)
 *   - end address is inclusive and must be aligned to a cache line
 *
 * The driver aligns the range defensively, so callers do not have to.
 */

#define CACHEAXI_LINE_SIZE           (32)
#define CACHEAXI_LINE_MASK           (CACHEAXI_LINE_SIZE - 1)

/* Timeouts (matching the values used by the ST HAL driver) */

#define CACHEAXI_ENABLE_TIMEOUT_MS   (1)
#define CACHEAXI_DISABLE_TIMEOUT_MS  (1)
#define CACHEAXI_COMMAND_TIMEOUT_MS  (200)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_CACHEAXI_H */