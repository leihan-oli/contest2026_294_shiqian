/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_cacheaxi.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <nuttx/clock.h>

#include "arm_internal.h"
#include "stm32n6_cacheaxi.h"

#include "hardware/stm32_cacheaxi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Registers and bits used below are defined in
 * hardware/stm32_cacheaxi.h.  The programming sequence mirrors the ST HAL
 * driver (stm32n6xx_hal_cacheaxi.c), re-implemented here so that the NuttX
 * port does not depend on the ST Cube HAL.
 */

#define CACHEAXI_ENABLE_TIMEOUT  MSEC2TICK(CACHEAXI_ENABLE_TIMEOUT_MS)
#define CACHEAXI_DISABLE_TIMEOUT MSEC2TICK(CACHEAXI_DISABLE_TIMEOUT_MS)
#define CACHEAXI_COMMAND_TIMEOUT MSEC2TICK(CACHEAXI_COMMAND_TIMEOUT_MS)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cacheaxi_wait_clear
 *
 * Description:
 *   Wait until none of the bits in 'mask' are set in the status register
 *   any more, or until 'timeout' ticks have elapsed.
 *
 * Returned Value:
 *   OK on success; -ETIMEDOUT on timeout.
 *
 ****************************************************************************/

static int cacheaxi_wait_clear(uint32_t mask, clock_t timeout)
{
  clock_t start = clock_systime_ticks();

  while ((getreg32(STM32_CACHEAXI_SR) & mask) != 0)
    {
      if ((clock_systime_ticks() - start) > timeout)
        {
          return -ETIMEDOUT;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: cacheaxi_wait_set
 *
 * Description:
 *   Wait until at least one of the bits in 'mask' is set in the status
 *   register, or until 'timeout' ticks have elapsed.
 *
 * Returned Value:
 *   OK on success; -ETIMEDOUT on timeout.
 *
 ****************************************************************************/

static int cacheaxi_wait_set(uint32_t mask, clock_t timeout)
{
  clock_t start = clock_systime_ticks();

  while ((getreg32(STM32_CACHEAXI_SR) & mask) == 0)
    {
      if ((clock_systime_ticks() - start) > timeout)
        {
          return -ETIMEDOUT;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: cacheaxi_command
 *
 * Description:
 *   Issue a range maintenance command (clean / clean-invalidate) covering
 *   [start, end) and wait for completion.
 *
 *   The start address is aligned down and the (exclusive) end address is
 *   aligned up to a 32-byte cache line, which is what the controller
 *   requires.
 *
 * Input Parameters:
 *   command - CACHEAXI_CMD_CLEAN or CACHEAXI_CMD_CLEAN_INVALID
 *   start   - Start address of the range
 *   end     - End address of the range (exclusive)
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int cacheaxi_command(uint32_t command, uintptr_t start, uintptr_t end)
{
  uintptr_t first;
  uintptr_t last;
  int ret;

  if (end <= start)
    {
      return OK;
    }

  first = start & ~(uintptr_t)CACHEAXI_LINE_MASK;
  last  = (((uintptr_t)end + CACHEAXI_LINE_MASK) &
           ~(uintptr_t)CACHEAXI_LINE_MASK) - 1;

  ret = cacheaxi_wait_clear(CACHEAXI_SR_BUSY, CACHEAXI_COMMAND_TIMEOUT);
  if (ret < 0)
    {
      return ret;
    }

  /* Clear the completion flags, program the range, select the operation and
   * start it.
   */

  putreg32(CACHEAXI_FCR_CBSYENDF | CACHEAXI_FCR_CCMDENDF, STM32_CACHEAXI_FCR);
  putreg32((uint32_t)first, STM32_CACHEAXI_CMDRSADDRR);
  putreg32((uint32_t)last, STM32_CACHEAXI_CMDREADDRR);

  modifyreg32(STM32_CACHEAXI_CR2, CACHEAXI_CR2_CACHECMD_MASK, command);

  /* Polled mode: the command end interrupt is not used */

  modifyreg32(STM32_CACHEAXI_IER, CACHEAXI_IER_CMDENDIE, 0);
  modifyreg32(STM32_CACHEAXI_CR2, 0, CACHEAXI_CR2_STARTCMD);

  return cacheaxi_wait_set(CACHEAXI_SR_CMDENDF, CACHEAXI_COMMAND_TIMEOUT);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_cacheaxi_initialize
 ****************************************************************************/

int stm32n6_cacheaxi_initialize(void)
{
  int ret;

  /* Make sure no maintenance operation is in flight, then enable the
   * cache.  A fresh controller may need a couple of retries before the
   * busy flag clears and the enable bit sticks.
   */

  ret = stm32n6_cacheaxi_enable();
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_cacheaxi_enable
 ****************************************************************************/

int stm32n6_cacheaxi_enable(void)
{
  int ret;

  ret = cacheaxi_wait_clear(CACHEAXI_SR_BUSY, CACHEAXI_ENABLE_TIMEOUT);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32_CACHEAXI_CR1, 0, CACHEAXI_CR1_EN);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_cacheaxi_disable
 ****************************************************************************/

int stm32n6_cacheaxi_disable(void)
{
  int ret;

  if (!stm32n6_cacheaxi_is_enabled())
    {
      return OK;
    }

  modifyreg32(STM32_CACHEAXI_CR1, CACHEAXI_CR1_EN, 0);

  ret = cacheaxi_wait_clear(CACHEAXI_SR_BUSY, CACHEAXI_DISABLE_TIMEOUT);
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_cacheaxi_is_enabled
 ****************************************************************************/

bool stm32n6_cacheaxi_is_enabled(void)
{
  return (getreg32(STM32_CACHEAXI_CR1) & CACHEAXI_CR1_EN) != 0;
}

/****************************************************************************
 * Name: stm32n6_cacheaxi_invalidate
 ****************************************************************************/

int stm32n6_cacheaxi_invalidate(void)
{
  int ret;

  ret = cacheaxi_wait_clear(CACHEAXI_SR_BUSY, CACHEAXI_COMMAND_TIMEOUT);
  if (ret < 0)
    {
      return ret;
    }

  putreg32(CACHEAXI_FCR_CBSYENDF | CACHEAXI_FCR_CCMDENDF, STM32_CACHEAXI_FCR);

  /* Select "no range command" and trigger a whole-cache invalidation. */

  modifyreg32(STM32_CACHEAXI_CR2, CACHEAXI_CR2_CACHECMD_MASK, 0);
  modifyreg32(STM32_CACHEAXI_CR1, 0, CACHEAXI_CR1_CACHEINV);

  return cacheaxi_wait_clear(CACHEAXI_SR_BUSYF, CACHEAXI_COMMAND_TIMEOUT);
}

/****************************************************************************
 * Name: stm32n6_cacheaxi_clean_range
 ****************************************************************************/

int stm32n6_cacheaxi_clean_range(uintptr_t start, uintptr_t end)
{
  return cacheaxi_command(CACHEAXI_CMD_CLEAN, start, end);
}

/****************************************************************************
 * Name: stm32n6_cacheaxi_clean_invalidate_range
 ****************************************************************************/

int stm32n6_cacheaxi_clean_invalidate_range(uintptr_t start, uintptr_t end)
{
  return cacheaxi_command(CACHEAXI_CMD_CLEAN_INVALID, start, end);
}

/****************************************************************************
 * Name: stm32n6_cacheaxi_status
 ****************************************************************************/

uint32_t stm32n6_cacheaxi_status(void)
{
  return getreg32(STM32_CACHEAXI_SR);
}
