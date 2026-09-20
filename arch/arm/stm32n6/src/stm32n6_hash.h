/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_hash.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_HASH_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_HASH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stddef.h>

#ifdef CONFIG_STM32_HASH

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#if defined(__cplusplus)
extern "C"
{
#endif

/****************************************************************************
 * Name: stm32n6_sha256
 *
 * Description:
 *   Compute the SHA-256 digest of a single in-memory buffer using the
 *   STM32N6 HASH accelerator in polling mode.
 *
 * Input Parameters:
 *   input  - Message bytes to hash.
 *   len    - Message length in bytes.
 *   digest - Output buffer, must hold 32 bytes.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno on failure.
 *
 ****************************************************************************/

int stm32n6_sha256(const uint8_t *input, size_t len, uint8_t *digest);

#if defined(__cplusplus)
}
#endif

#endif /* CONFIG_STM32_HASH */
#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_HASH_H */
