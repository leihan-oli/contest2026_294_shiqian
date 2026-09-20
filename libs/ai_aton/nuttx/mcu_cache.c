/****************************************************************************
 * libs/ai_aton/nuttx/mcu_cache.c
 *
 * CPU D-Cache maintenance helpers used by the ST ATON runtime.  See
 * mcu_cache.h for the rationale.
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

#include <nuttx/cache.h>

#include "mcu_cache.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mcu_cache_enable(void)
{
  up_enable_dcache();
  return 0;
}

int mcu_cache_disable(void)
{
  up_disable_dcache();
  return 0;
}

int mcu_cache_invalidate(void)
{
  up_invalidate_dcache_all();
  return 0;
}

int mcu_cache_clean(void)
{
  up_clean_dcache_all();
  return 0;
}

int mcu_cache_clean_invalidate(void)
{
  up_flush_dcache_all();
  return 0;
}

/* The range helpers take an exclusive end address, matching the CMSIS
 * SCB_*_by_Addr() semantics used by the vendor implementation.  The NuttX
 * cache primitives round the start address down and also cover the line
 * holding the final address, so no extra alignment is required here.
 */

int mcu_cache_invalidate_range(uint32_t start_addr, uint32_t end_addr)
{
  if (end_addr > start_addr)
    {
      up_invalidate_dcache((uintptr_t)start_addr, (uintptr_t)end_addr);
    }

  return 0;
}

int mcu_cache_clean_range(uint32_t start_addr, uint32_t end_addr)
{
  if (end_addr > start_addr)
    {
      up_clean_dcache((uintptr_t)start_addr, (uintptr_t)end_addr);
    }

  return 0;
}

int mcu_cache_clean_invalidate_range(uint32_t start_addr, uint32_t end_addr)
{
  if (end_addr > start_addr)
    {
      up_flush_dcache((uintptr_t)start_addr, (uintptr_t)end_addr);
    }

  return 0;
}
