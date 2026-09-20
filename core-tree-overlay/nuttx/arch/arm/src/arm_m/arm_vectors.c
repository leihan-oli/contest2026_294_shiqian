/****************************************************************************
 * arch/arm/src/arm_m/arm_vectors.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 *   Copyright (C) 2012 Michael Smith. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "chip.h"
#include "arm_internal.h"
#include "ram_vectors.h"
#include "nvic.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_ARCH_ARMV6M
#  define ARM_PERIPHERAL_INTERRUPTS ARMV6M_PERIPHERAL_INTERRUPTS
#elif defined(CONFIG_ARCH_ARMV7M)
#  define ARM_PERIPHERAL_INTERRUPTS ARMV7M_PERIPHERAL_INTERRUPTS
#elif defined(CONFIG_ARCH_ARMV8M)
#  define ARM_PERIPHERAL_INTERRUPTS ARMV8M_PERIPHERAL_INTERRUPTS
#endif

#define IDLE_STACK      (_ebss + CONFIG_IDLETHREAD_STACKSIZE)

#ifndef ARM_PERIPHERAL_INTERRUPTS
#  error ARM_PERIPHERAL_INTERRUPTS must be defined to the number of I/O interrupts to be supported
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Chip-specific entrypoint */

extern void __start(void);

static void start(void)
{
#if defined(CONFIG_ARCH_CHIP_STM32N647X0)
  /* DIAG v5: LED0 ON via the SECURE GPIOG BSRR alias (0x56021818,
   * BR10=bit26).  MUST use the secure alias: the FSBL leaves the GPIO
   * pins in their reset SECURE state (no ConfigPinAttributes call), and
   * a write to the NS alias of a secure peripheral SecureFaults, which
   * previously made the board look like the jump itself failed.  This is
   * a pure "did the CPU reach start()" probe: if LED0 never lights, the
   * FSBL->NuttX jump itself failed (CPU never reached start()). */
  *(volatile uint32_t *)0x56021818UL = (1UL << 26);
#endif

  /* Set MSP & PSP to the value at reset */

  arm_initialize_stack();

#if defined(CONFIG_ARCH_CHIP_STM32N647X0)
  /* DIAG v5: LED1 ON via the SECURE GPIOE BSRR alias (0x56021018,
   * BR10=bit26) => arm_initialize_stack() returned without faulting.
   *   LED0 ON + LED1 OFF -> arm_initialize_stack() faulted (hang).
   *   LED0 ON + LED1 ON  -> start()+arm_initialize_stack() OK; the hang is
   *     in __start()/__start_c(). */
  *(volatile uint32_t *)0x56021018UL = (1UL << 26);
#endif

  /* Zero lr to mark the end of backtrace */

  asm volatile ("mov lr, %0\n\t"
                "bx      %1\n\t"
                :
                : "r"(0), "r"(__start));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Common exception entrypoint */

extern void exception_common(void);
extern void exception_direct(void);

/****************************************************************************
 * Public data
 ****************************************************************************/

/* The arm_m vector table consists of an array of function pointers, with the
 * first slot (vector zero) used to hold the initial stack pointer.
 *
 * As all exceptions (interrupts) are routed via exception_common, we just
 * need to fill this array with pointers to it.
 *
 * Note that the [ ... ] designated initializer is a GCC extension.
 */

const void * const _vectors[] locate_data(".vectors")
                              aligned_data(VECTAB_ALIGN) =
{
  /* Initial stack */

  IDLE_STACK,

  /* Reset exception handler */

  start,

  /* Vectors 2 - n point directly at the generic handler */

  [2 ... NVIC_IRQ_PENDSV] = &exception_common,
  [(NVIC_IRQ_PENDSV + 1) ... (15 + ARM_PERIPHERAL_INTERRUPTS)]
                          = &exception_direct
};
