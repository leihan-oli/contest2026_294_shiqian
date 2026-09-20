/****************************************************************************
 * libs/ai_aton/nuttx/npu_cache.c
 *
 * NPU AXI cache (CACHEAXI) control for the ST ATON runtime.  See npu_cache.h
 * for the rationale; the register sequences live in the chip driver
 * arch/arm/src/stm32n6/stm32n6_cacheaxi.c.
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
#include <stdint.h>

#include "stm32n6_cacheaxi.h"

#include "npu_cache.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Bring the AXI cache up.  The peripheral clock and reset are handled by
 * stm32n6_clockconfig(), so this only has to enable the cache itself.
 *
 * The vendor implementation polls HAL_CACHEAXI_Enable() in a loop because
 * the first call often reports HAL_BUSY while a previous operation is still
 * draining; stm32n6_cacheaxi_enable() waits for the busy flag itself, so a
 * single call is enough here.
 */

void npu_cache_init(void)
{
  stm32n6_cacheaxi_initialize();
}

void npu_cache_enable(void)
{
  stm32n6_cacheaxi_enable();
}

void npu_cache_disable(void)
{
  stm32n6_cacheaxi_disable();
}

void npu_cache_invalidate(void)
{
  stm32n6_cacheaxi_invalidate();
}

void npu_cache_clean_range(uint32_t start_addr, uint32_t end_addr)
{
  stm32n6_cacheaxi_clean_range((uintptr_t)start_addr, (uintptr_t)end_addr);
}

void npu_cache_clean_invalidate_range(uint32_t start_addr, uint32_t end_addr)
{
  stm32n6_cacheaxi_clean_invalidate_range((uintptr_t)start_addr,
                                          (uintptr_t)end_addr);
}
