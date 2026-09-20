/****************************************************************************
 * arch/arm/src/arm_m/arm_hardfault.c
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

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/userspace.h>
#include <arch/irq.h>

#include "nvic.h"
#include "sau.h"
#include "arm_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_HARDFAULT_ALERT
#  define hfalert(format, ...) _alert(format, ##__VA_ARGS__)
#else
#  define hfalert(x...)
#endif

#ifdef CONFIG_DEBUG_HARDFAULT_INFO
#  define hfinfo(format, ...)  _info(format, ##__VA_ARGS__)
#else
#  define hfinfo(x...)
#endif

#define INSN_SVC0        0xdf00 /* insn: svc 0 */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm_hardfault
 *
 * Description:
 *   This is Hard Fault exception handler.  It also catches SVC call
 *   exceptions that are performed in bad contexts.
 *
 ****************************************************************************/

int arm_hardfault(int irq, void *context, void *arg)
{
  uint32_t hfsr = getreg32(NVIC_HFAULTS);
  uint32_t cfsr = getreg32(NVIC_CFAULTS);
#ifdef CONFIG_DEBUG_SECUREFAULT
  uint32_t sfsr = getreg32(SAU_SFSR);
#endif /* CONFIG_DEBUG_SECUREFAULT */

  UNUSED(cfsr);
  UNUSED(hfsr);

#ifdef CONFIG_ARCH_ARMV6M
  uint32_t *regs = (uint32_t *)context;
  uint16_t *pc = (uint16_t *)regs[REG_PC] - 1;

  /* Check if the pc lies in known FLASH memory.
   * REVISIT:  What if the PC lies in "unknown" external memory?
   */

#ifdef CONFIG_BUILD_PROTECTED
  /* In the kernel build, SVCalls are expected in either the base, kernel
   * FLASH region or in the user FLASH region.
   */

  if (((uintptr_t)pc >= (uintptr_t)_stext &&
      (uintptr_t)pc <  (uintptr_t)_etext) ||
      ((uintptr_t)pc >= (uintptr_t)USERSPACE->us_textstart &&
      (uintptr_t)pc <  (uintptr_t)USERSPACE->us_textend))
#else
  /* SVCalls are expected only from the base, kernel FLASH region */

  if ((uintptr_t)pc >= (uintptr_t)_stext &&
      (uintptr_t)pc <  (uintptr_t)_etext)
#endif
    {
      /* Fetch the instruction that caused the Hard fault */

      uint16_t insn = *pc;
      hfinfo("  PC: %p INSN: %04x\n", pc, insn);

      /* If this was the instruction 'svc 0', then forward processing
       * to the SVCall handler
       */

      if (insn == INSN_SVC0)
        {
          hfinfo("Forward SVCall\n");
          return arm_svcall(irq, context, NULL);
        }
    }
#endif

  if (hfsr & NVIC_HFAULTS_FORCED)
    {
      hfalert("Hard Fault escalation:\n");
#ifdef CONFIG_DEBUG_MEMFAULT
      if (cfsr & NVIC_CFAULTS_MEMFAULTSR_MASK)
        {
          return arm_memfault(irq, context, arg);
        }
#endif /* CONFIG_DEBUG_MEMFAULT */

#ifdef CONFIG_DEBUG_BUSFAULT
      if (cfsr & NVIC_CFAULTS_BUSFAULTSR_MASK)
        {
          return arm_busfault(irq, context, arg);
        }
#endif /* CONFIG_DEBUG_BUSFAULT */

#ifdef CONFIG_DEBUG_USAGEFAULT
      if (cfsr & NVIC_CFAULTS_USGFAULTSR_MASK)
        {
          return arm_usagefault(irq, context, arg);
        }
#endif /* CONFIG_DEBUG_USAGEFAULT */

#ifdef CONFIG_DEBUG_SECUREFAULT
      if (sfsr & SAU_SFSR_MASK)
        {
          return arm_securefault(irq, context, arg);
        }
#endif /* CONFIG_DEBUG_SECUREFAULT */
    }

  /* Dump some hard fault info */

  hfalert("PANIC!!! Hard Fault!:");
  hfalert("\tIRQ: %d regs: %p\n", irq, context);
  hfalert("\tBASEPRI: %08" PRIx8 " PRIMASK: %08" PRIx8 " IPSR: %08"
          PRIx32 " CONTROL: %08" PRIx32 "\n",
          getbasepri(), getprimask(), getipsr(), getcontrol());
  hfalert("\tCFSR: %08" PRIx32 " HFSR: %08" PRIx32 " DFSR: %08"
          PRIx32 " BFAR: %08" PRIx32 " AFSR: %08" PRIx32 "\n",
          cfsr, hfsr, getreg32(NVIC_DFAULTS),
          getreg32(NVIC_BFAULT_ADDR), getreg32(NVIC_AFAULTS));

  hfalert("Hard Fault Reason:\n");

  if (hfsr & NVIC_HFAULTS_VECTTBL)
    {
      hfalert("\tBusFault on a vector table read\n");
    }
  else if (hfsr & NVIC_HFAULTS_DEBUGEVT)
    {
      hfalert("\tDebug event\n");
    }

#ifdef CONFIG_DEBUG_HARDFAULT_ALERT
  if (arm_gen_nonsecurefault(irq, context))
    {
      return OK;
    }
#endif

#ifdef CONFIG_DEBUG_HARDFAULT_ALERT
  /* Print the hardware exception frame before handing over to the panic
   * machinery.  _assert() replaces the register set it is given with its own
   * saved context (see assert.c: up_saveusercontext(g_last_regs)), so the
   * register dump printed by the panic path describes the assertion code
   * itself instead of the instruction that faulted.  Here `context` is still
   * the real frame, so this is the only chance to see the faulting PC.
   */

  {
    FAR uint32_t *frame = (FAR uint32_t *)context;

    _alert("Fault frame: R0=%08" PRIx32 " R1=%08" PRIx32 " R2=%08" PRIx32
           " R3=%08" PRIx32 " R12=%08" PRIx32 "\n",
           frame[REG_R0], frame[REG_R1], frame[REG_R2], frame[REG_R3],
           frame[REG_R12]);
    _alert("Fault frame: SP=%08" PRIx32 " LR=%08" PRIx32 " PC=%08" PRIx32
           " xPSR=%08" PRIx32 "\n",
           frame[REG_SP], frame[REG_LR], frame[REG_PC], frame[REG_XPSR]);
  }
#endif

  up_irq_save();
  PANIC_WITH_REGS("panic", context);
  return OK;
}
