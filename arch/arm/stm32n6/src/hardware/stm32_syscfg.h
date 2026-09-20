/****************************************************************************
 * arch/arm/stm32n6/src/hardware/stm32_syscfg.h
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

#ifndef __ARCH_ARM_STM32N6_SRC_HARDWARE_STM32_SYSCFG_H
#define __ARCH_ARM_STM32N6_SRC_HARDWARE_STM32_SYSCFG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register Offsets *********************************************************/

/* These offsets are taken from the ST CMSIS SYSCFG_TypeDef in
 * stm32n647xx.h (the authoritative register map for our STM32N647X0),
 * NOT from apache/nuttx upstream's stm32n6xxx_syscfg.h.  Upstream lists
 * VDDIO2CCCR at 0x054 and VDDIO3CCCR at 0x05c, but in the ST CMSIS layout
 * (identical for N647 and N657) those offsets are VDDIO4CCCR / VDDIO5CCCR;
 * the real VDDIO2/VDDIO3 compensation-cell registers are at 0x044 / 0x04c.
 * Our USART1 console pins PE5/PE6 sit on the VDDIO2/VDDIO3 domains, so the
 * offsets below are the ones that actually matter for the ES0620
 * mitigation on this board.
 */

/* Cortex-M55 secure vector-table base control register */

#define STM32_SYSCFG_INITSVTORCR_OFFSET  0x010

/* VDDIO2 compensation cell control register */

#define STM32_SYSCFG_VDDIO2CCCR_OFFSET   0x044

/* VDDIO3 compensation cell control register */

#define STM32_SYSCFG_VDDIO3CCCR_OFFSET   0x04c

/* VDD compensation cell control register */

#define STM32_SYSCFG_VDDCCCR_OFFSET      0x064

/* Register Addresses *******************************************************/

#define STM32_SYSCFG_INITSVTORCR \
  (STM32_SYSCFG_BASE + STM32_SYSCFG_INITSVTORCR_OFFSET)
#define STM32_SYSCFG_VDDIO2CCCR \
  (STM32_SYSCFG_BASE + STM32_SYSCFG_VDDIO2CCCR_OFFSET)
#define STM32_SYSCFG_VDDIO3CCCR \
  (STM32_SYSCFG_BASE + STM32_SYSCFG_VDDIO3CCCR_OFFSET)
#define STM32_SYSCFG_VDDCCCR \
  (STM32_SYSCFG_BASE + STM32_SYSCFG_VDDCCCR_OFFSET)

/* Register Bitfield Definitions ********************************************/

/* Compensation cell control register (VDDxCCCR / VDDIOxCCCR).  All of the
 * banks share an identical layout, verified against the CMSIS
 * SYSCFG_VDDIO2CCCR_* bit definitions in stm32n647xx.h.
 */

#define SYSCFG_CCCR_CS              (1 << 9)  /* Bit 9: Code source select */
#define SYSCFG_CCCR_EN              (1 << 8)  /* Bit 8: Enable comp cell */
#define SYSCFG_CCCR_RAPSRC_SHIFT    (4)       /* Bits 7-4: PMOS comp code */
#define SYSCFG_CCCR_RAPSRC_MASK     (0xf << SYSCFG_CCCR_RAPSRC_SHIFT)
#define SYSCFG_CCCR_RANSRC_SHIFT    (0)       /* Bits 3-0: NMOS comp code */
#define SYSCFG_CCCR_RANSRC_MASK     (0xf << SYSCFG_CCCR_RANSRC_SHIFT)

/* ES0620 I/O compensation mitigation: write 0x00000287 (EN=0, CS=1,
 * RAPSRC=0x8, RANSRC=0x7) to VDDCCCR and every VDDIOxCCCR in use before
 * driving high-speed pads.  This disables the cell and overrides the
 * broken silicon defaults (RAPSRC=0x7, RANSRC=0x8) that deform output
 * edges.  Bit assignments confirmed against CMSIS stm32n647xx.h.
 */

#define SYSCFG_CCCR_ES0620_MANUAL   (SYSCFG_CCCR_CS | \
                                     (8 << SYSCFG_CCCR_RAPSRC_SHIFT) | \
                                     (7 << SYSCFG_CCCR_RANSRC_SHIFT))

#endif /* __ARCH_ARM_STM32N6_SRC_HARDWARE_STM32_SYSCFG_H */
