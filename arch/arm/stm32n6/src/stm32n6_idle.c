/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_idle.c
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

#include <nuttx/arch.h>

#include "arm_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_idle
 *
 * Description:
 *   up_idle() is the logic that will be executed when there is no other
 *   ready-to-run task.  This is processor idle time and will continue
 *   until some interrupt occurs to cause a context switch from the idle
 *   task.
 *
 *   Ported from apache/nuttx upstream stm32_idle.c.  The generic weak
 *   implementation in arch/arm/src/common/arm_idle.c has its WFI call
 *   wrapped in "#if 0 ... #endif" (a template placeholder, not a real
 *   implementation), so without this override the idle task busy-loops
 *   instead of ever sleeping the CPU -- a real, previously-unnoticed
 *   power-consumption gap in this port, not a functional bug (the
 *   scheduler and timer tick both work correctly either way; the CPU
 *   simply never enters a low-power WFI wait state while idle).
 *
 *   SLEEPDEEP is cleared once at boot (see stm32n6_start.c's
 *   __start_c()), so WFI here enters plain SLEEP mode.  __start_c() also
 *   sets the BUSLPENR/MEMLPENR LPEN bits (and, per ES0620, keeps BSECEN
 *   set) so the AXISRAM bus/RAM clocks this image runs from keep running
 *   through CSLEEP; SysTick therefore continues to fire and wakes the
 *   CPU on the next tick.  Without those LPEN bits WFI would stall the
 *   RAM clock and the core would never wake.  This repo has no
 *   CONFIG_ARCH_LEDS/LED_IDLE board LED abstraction, so upstream's
 *   BEGIN_IDLE()/END_IDLE() LED hooks are omitted.
 *
 ****************************************************************************/

void up_idle(void)
{
#if defined(CONFIG_SUPPRESS_INTERRUPTS) || defined(CONFIG_SUPPRESS_TIMER_INTS)
  /* If the system is idle and there are no timer interrupts, then
   * process "fake" timer interrupts.  Hopefully, something will wake
   * up.
   */

  nxsched_process_timer();
#else
  asm("WFI");
#endif
}
