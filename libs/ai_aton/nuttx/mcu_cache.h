/****************************************************************************
 * libs/ai_aton/nuttx/mcu_cache.h
 *
 * NuttX implementation of the "MCU cache" interface expected by the ST ATON
 * runtime (vendor/ll_aton/ll_aton_platform.h includes "mcu_cache.h" for the
 * LL_ATON_PLAT_STM32N6 platform).
 *
 * The vendor file (Middlewares/AI/Npu/Devices/STM32N6XX/mcu_cache.c) is
 * written against the CMSIS SCB_* cache API; this port forwards the same
 * calls to the NuttX cache interface (up_clean_dcache / up_invalidate_dcache
 * / up_flush_dcache), which performs the identical set/way maintenance
 * operations on the Cortex-M55 D-Cache.
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

#ifndef __LIBS_AI_ATON_NUTTX_MCU_CACHE_H
#define __LIBS_AI_ATON_NUTTX_MCU_CACHE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int mcu_cache_enable(void);
int mcu_cache_disable(void);
int mcu_cache_invalidate(void);
int mcu_cache_clean(void);
int mcu_cache_clean_invalidate(void);
int mcu_cache_invalidate_range(uint32_t start_addr, uint32_t end_addr);
int mcu_cache_clean_range(uint32_t start_addr, uint32_t end_addr);
int mcu_cache_clean_invalidate_range(uint32_t start_addr, uint32_t end_addr);

#ifdef __cplusplus
}
#endif

#endif /* __LIBS_AI_ATON_NUTTX_MCU_CACHE_H */
