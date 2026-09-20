/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_hash.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/mutex.h>

#include "arm_internal.h"
#include "hardware/stm32_hash.h"
#include "hardware/stm32_rcc.h"

#ifdef CONFIG_STM32_HASH

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SHA-256 produces a 256-bit (32-byte / 8-word) digest.  The engine is
 * fast; a bounded spin keeps a stuck flag from hanging the caller.
 */

#define HASH_SHA256_WORDS   8
#define HASH_POLL_LIMIT     1000000

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* One shared HASH engine — serialize callers. */

static mutex_t g_hash_lock = NXMUTEX_INITIALIZER;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hash_pack_le
 *
 * Description:
 *   Pack up to four message bytes into a little-endian 32-bit word.  With
 *   HASH_CR_DATATYPE_8B the engine performs the byte swap internally, so
 *   feeding the native little-endian word preserves the message byte order.
 *
 ****************************************************************************/

static uint32_t hash_pack_le(const uint8_t *p, size_t n)
{
  uint32_t w = 0;
  size_t i;

  for (i = 0; i < n; i++)
    {
      w |= (uint32_t)p[i] << (8 * i);
    }

  return w;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_sha256
 *
 * Description:
 *   Compute the SHA-256 digest of a single in-memory buffer using the
 *   STM32N6 HASH accelerator in polling mode.  This is a one-shot helper
 *   (no streaming/context-swap); it hashes the whole buffer in one call.
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

int stm32n6_sha256(const uint8_t *input, size_t len, uint8_t *digest)
{
  uint32_t regval;
  uint32_t count;
  size_t nfull;
  size_t rem;
  size_t i;
  int ret;

  if (input == NULL || digest == NULL)
    {
      return -EINVAL;
    }

  ret = nxmutex_lock(&g_hash_lock);
  if (ret < 0)
    {
      return ret;
    }

  /* Enable the HASH peripheral clock (atomic read-modify-write). */

  modifyreg32(STM32_RCC_AHB3ENR, 0, RCC_AHB3ENR_HASHEN);

  /* Configure the engine following the ST HAL ordering: first latch the
   * algorithm and data type (plain hash mode, MODE=0), then pulse INIT as a
   * separate write so the core resets with the configuration already stable.
   */

  regval = HASH_CR_ALGO_SHA256 | HASH_CR_DATATYPE_8B;
  putreg32(regval, STM32_HASH_CR);
  putreg32(regval | HASH_CR_INIT, STM32_HASH_CR);

  /* Set NBLW = number of valid bits in the last word BEFORE feeding data
   * (0 when the message is a whole number of words), matching the HAL.
   */

  nfull = len / 4;
  rem   = len % 4;

  putreg32((uint32_t)(rem * 8) << HASH_STR_NBLW_SHIFT, STM32_HASH_STR);

  /* Feed all complete 32-bit words, then the partial trailing word. */

  for (i = 0; i < nfull; i++)
    {
      putreg32(hash_pack_le(input + (i * 4), 4), STM32_HASH_DIN);
    }

  if (rem > 0)
    {
      putreg32(hash_pack_le(input + (nfull * 4), rem), STM32_HASH_DIN);
    }

  /* Start the message padding then the digest calculation. */

  modifyreg32(STM32_HASH_STR, 0, HASH_STR_DCAL);

  /* Wait for the digest-calculation-complete flag with a bounded spin. */

  count = 0;
  while ((getreg32(STM32_HASH_SR) & HASH_SR_DCIS) == 0)
    {
      if (++count > HASH_POLL_LIMIT)
        {
          nxmutex_unlock(&g_hash_lock);
          return -ETIMEDOUT;
        }
    }

  /* Read the eight digest words.  Words 0..4 are in the main HR bank;
   * words 5..7 are in the aliased digest bank.  Each hardware word is
   * big-endian relative to the byte stream, so byte-swap into the buffer.
   */

  for (i = 0; i < HASH_SHA256_WORDS; i++)
    {
      if (i < 5)
        {
          regval = getreg32(STM32_HASH_HR0 + (i << 2));
        }
      else
        {
          regval = getreg32(STM32_HASH_DIGEST_HR(i));
        }

      digest[(i * 4) + 0] = (uint8_t)(regval >> 24);
      digest[(i * 4) + 1] = (uint8_t)(regval >> 16);
      digest[(i * 4) + 2] = (uint8_t)(regval >> 8);
      digest[(i * 4) + 3] = (uint8_t)(regval);
    }

  nxmutex_unlock(&g_hash_lock);
  return OK;
}

#endif /* CONFIG_STM32_HASH */
