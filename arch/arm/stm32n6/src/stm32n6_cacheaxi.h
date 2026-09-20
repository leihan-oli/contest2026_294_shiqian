/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_cacheaxi.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_CACHEAXI_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_CACHEAXI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: stm32n6_cacheaxi_initialize
 *
 * Description:
 *   Bring up the Neural-ART AXI cache (CACHEAXI) and enable it.  The
 *   peripheral clock (RCC AHB5ENR.CACHEAXIEN) must already have been
 *   enabled and the reset released by stm32n6_clockconfig().
 *
 *   This is idempotent: calling it more than once only re-enables the
 *   cache if it has been disabled in the meantime.
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_cacheaxi_initialize(void);

/****************************************************************************
 * Name: stm32n6_cacheaxi_enable / stm32n6_cacheaxi_disable
 *
 * Description:
 *   Enable or disable the AXI cache.  Enabling waits for a pending cache
 *   operation to complete first, disabling waits for the cache to drain.
 *
 * Returned Value:
 *   OK on success; a negated errno value (typically -ETIMEDOUT) on failure.
 *
 ****************************************************************************/

int stm32n6_cacheaxi_enable(void);
int stm32n6_cacheaxi_disable(void);

/****************************************************************************
 * Name: stm32n6_cacheaxi_is_enabled
 *
 * Description:
 *   Report the CACHEAXI_CR1.EN bit.
 *
 ****************************************************************************/

bool stm32n6_cacheaxi_is_enabled(void);

/****************************************************************************
 * Name: stm32n6_cacheaxi_invalidate
 *
 * Description:
 *   Invalidate the whole AXI cache.  All dirty lines are discarded, so this
 *   must only be used when the contents of the cached RAM are known to be
 *   stale (e.g. during NPU bring-up).
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_cacheaxi_invalidate(void);

/****************************************************************************
 * Name: stm32n6_cacheaxi_clean_range
 * Name: stm32n6_cacheaxi_clean_invalidate_range
 *
 * Description:
 *   Clean (write back) or clean+invalidate the given address range.  The
 *   range is [start, end) - the caller passes an exclusive end address and
 *   the driver aligns it outwards to a cache line boundary.
 *
 * Input Parameters:
 *   start - Start address (aligned down to a cache line)
 *   end   - End address, exclusive (aligned up to a cache line)
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_cacheaxi_clean_range(uintptr_t start, uintptr_t end);
int stm32n6_cacheaxi_clean_invalidate_range(uintptr_t start, uintptr_t end);

/****************************************************************************
 * Name: stm32n6_cacheaxi_status
 *
 * Description:
 *   Return the raw CACHEAXI_SR register value (diagnostics only).
 *
 ****************************************************************************/

uint32_t stm32n6_cacheaxi_status(void);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_CACHEAXI_H */
