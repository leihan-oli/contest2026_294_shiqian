/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_hash.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_HASH_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_HASH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register Offsets *********************************************************/

#define STM32_HASH_CR_OFFSET      0x0000  /* Control register */
#define STM32_HASH_DIN_OFFSET     0x0004  /* Data input register */
#define STM32_HASH_STR_OFFSET     0x0008  /* Start register */
#define STM32_HASH_HR0_OFFSET     0x000c  /* Digest word 0 */
#define STM32_HASH_HR1_OFFSET     0x0010  /* Digest word 1 */
#define STM32_HASH_HR2_OFFSET     0x0014  /* Digest word 2 */
#define STM32_HASH_HR3_OFFSET     0x0018  /* Digest word 3 */
#define STM32_HASH_HR4_OFFSET     0x001c  /* Digest word 4 */
#define STM32_HASH_IMR_OFFSET     0x0020  /* Interrupt enable register */
#define STM32_HASH_SR_OFFSET      0x0024  /* Status register */

/* The upper digest words (5..7 for SHA-256) live in a separate aliased
 * bank at HASH base + 0x0310; HR5 is thus base + 0x0310 + 5*4.
 */

#define STM32_HASH_DIGEST_OFFSET  0x0310  /* Aliased digest bank base */

/* Register Addresses *******************************************************/

#define STM32_HASH_CR             (STM32_HASH_BASE + STM32_HASH_CR_OFFSET)
#define STM32_HASH_DIN            (STM32_HASH_BASE + STM32_HASH_DIN_OFFSET)
#define STM32_HASH_STR            (STM32_HASH_BASE + STM32_HASH_STR_OFFSET)
#define STM32_HASH_HR0            (STM32_HASH_BASE + STM32_HASH_HR0_OFFSET)
#define STM32_HASH_HR1            (STM32_HASH_BASE + STM32_HASH_HR1_OFFSET)
#define STM32_HASH_HR2            (STM32_HASH_BASE + STM32_HASH_HR2_OFFSET)
#define STM32_HASH_HR3            (STM32_HASH_BASE + STM32_HASH_HR3_OFFSET)
#define STM32_HASH_HR4            (STM32_HASH_BASE + STM32_HASH_HR4_OFFSET)
#define STM32_HASH_IMR            (STM32_HASH_BASE + STM32_HASH_IMR_OFFSET)
#define STM32_HASH_SR             (STM32_HASH_BASE + STM32_HASH_SR_OFFSET)

/* Aliased upper digest words: HRn = digest bank + n*4 */

#define STM32_HASH_DIGEST_HR(n) \
  (STM32_HASH_BASE + STM32_HASH_DIGEST_OFFSET + ((n) << 2))

/* HASH_CR Register Bitfield Definitions ************************************/

#define HASH_CR_INIT              (1 << 2)  /* Bit 2: Initialize digest */
#define HASH_CR_DMAE              (1 << 3)  /* Bit 3: DMA enable */
#define HASH_CR_DATATYPE_SHIFT    4         /* Bits 4-5: Data type select */
#define HASH_CR_DATATYPE_MASK     (0x3 << HASH_CR_DATATYPE_SHIFT)
#  define HASH_CR_DATATYPE_32B    (0x0 << HASH_CR_DATATYPE_SHIFT)
#  define HASH_CR_DATATYPE_16B    (0x1 << HASH_CR_DATATYPE_SHIFT)
#  define HASH_CR_DATATYPE_8B     (0x2 << HASH_CR_DATATYPE_SHIFT)
#  define HASH_CR_DATATYPE_1B     (0x3 << HASH_CR_DATATYPE_SHIFT)
#define HASH_CR_MODE              (1 << 6)  /* Bit 6: 0=hash, 1=HMAC */
#define HASH_CR_ALGO_SHIFT        17        /* Bits 17-20: Algorithm */
#define HASH_CR_ALGO_MASK         (0xf << HASH_CR_ALGO_SHIFT)
#  define HASH_CR_ALGO_SHA1       (0x0 << HASH_CR_ALGO_SHIFT)
#  define HASH_CR_ALGO_SHA224     (0x2 << HASH_CR_ALGO_SHIFT)
#  define HASH_CR_ALGO_SHA256     (0x3 << HASH_CR_ALGO_SHIFT)

/* HASH_STR Register Bitfield Definitions ***********************************/

#define HASH_STR_NBLW_SHIFT       0         /* Bits 0-4: Valid bits in last */
#define HASH_STR_NBLW_MASK        (0x1f << HASH_STR_NBLW_SHIFT)
#define HASH_STR_DCAL             (1 << 8)  /* Bit 8: Digest calculation */

/* HASH_SR Register Bitfield Definitions ************************************/

#define HASH_SR_DINIS             (1 << 0)  /* Bit 0: Data input ready */
#define HASH_SR_DCIS              (1 << 1)  /* Bit 1: Digest calc complete */
#define HASH_SR_DMAS              (1 << 2)  /* Bit 2: DMA active */
#define HASH_SR_BUSY              (1 << 3)  /* Bit 3: Core busy */

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_HASH_H */
