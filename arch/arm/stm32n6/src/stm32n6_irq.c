/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_irq.c
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
#include <assert.h>
#include <debug.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>

#include "arm_internal.h"
#include "ram_vectors.h"
#include "nvic.h"
#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NVIC_ENA_OFFSET    (0)
#define NVIC_CLRENA_OFFSET (NVIC_IRQ0_31_CLEAR - NVIC_IRQ0_31_ENABLE)

#define DEFPRIORITY32 \
  (NVIC_SYSH_PRIORITY_DEFAULT << 24 | \
   NVIC_SYSH_PRIORITY_DEFAULT << 16 | \
   NVIC_SYSH_PRIORITY_DEFAULT << 8 | \
   NVIC_SYSH_PRIORITY_DEFAULT)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_irqinfo
 *
 * Description:
 *   Given an IRQ number, return the register and bit to
 *   enable/disable that interrupt.
 *
 ****************************************************************************/

static int stm32n6_irqinfo(int irq, uintptr_t *regaddr,
                            uint32_t *bit, int offset)
{
  int n;

  if (irq >= STM32_IRQ_FIRST && irq < NR_IRQS)
    {
      n = irq - STM32_IRQ_FIRST;
      *regaddr = NVIC_IRQ_ENABLE(n) + offset;
      *bit = 1 << (n & 0x1f);
    }
  else if (irq >= STM32_IRQ_MEMFAULT && irq <= STM32_IRQ_USAGEFAULT)
    {
      *regaddr = NVIC_SYSHCON;
      if (irq == STM32_IRQ_MEMFAULT)
        {
          *bit = NVIC_SYSHCON_MEMFAULTENA;
        }
      else if (irq == STM32_IRQ_BUSFAULT)
        {
          *bit = NVIC_SYSHCON_BUSFAULTENA;
        }
      else
        {
          *bit = NVIC_SYSHCON_USGFAULTENA;
        }
    }
  else if (irq == STM32_IRQ_SYSTICK)
    {
      *regaddr = NVIC_SYSTICK_CTRL;
      *bit = NVIC_SYSTICK_CTRL_ENABLE;
    }
  else
    {
      return -EINVAL;
    }

  return OK;
}

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int up_prioritize_irq(int irq, int priority);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_irqinitialize
 *
 * Description:
 *   Initialize the NVIC.
 *
 ****************************************************************************/

void up_irqinitialize(void)
{
  int nprioreg;
  uintptr_t regaddr;
  int i;

  /* Disable all interrupts */

  for (i = 0; i < NR_IRQS - STM32_IRQ_FIRST; i += 32)
    {
      putreg32(0xffffffff, NVIC_IRQ_CLEAR(i));
    }

  /* Set the vector table address */

  putreg32((uint32_t)_vectors, NVIC_VECTAB);

#ifdef CONFIG_ARCH_RAMVECTORS
  arm_ramvec_initialize();
#endif

  /* Set all interrupts (and exceptions) to the default priority */

  putreg32(DEFPRIORITY32, NVIC_SYSH4_7_PRIORITY);
  putreg32(DEFPRIORITY32, NVIC_SYSH8_11_PRIORITY);
  putreg32(DEFPRIORITY32, NVIC_SYSH12_15_PRIORITY);

  nprioreg = (getreg32(NVIC_ICTR) + 1) * 8;
  regaddr  = NVIC_IRQ0_3_PRIORITY;

  while (nprioreg--)
    {
      putreg32(DEFPRIORITY32, regaddr);
      regaddr += 4;
    }

  /* Attach system exception handlers */

  irq_attach(STM32_IRQ_SVCALL, arm_svcall, NULL);
  irq_attach(STM32_IRQ_HARDFAULT, arm_hardfault, NULL);

  /* Set PendSV to lowest priority; SVCALL to high priority.
   *
   * SysTick's priority is intentionally NOT set here: apache/nuttx
   * upstream sets it to NVIC_SYSH_PRIORITY_DEFAULT (not MIN) inside
   * stm32_timerisr.c's up_timer_initialize(), not in this function.
   * A previous revision of this function set it to
   * NVIC_SYSH_PRIORITY_MIN here, which both duplicated the
   * responsibility and used a different priority than upstream.
   * See stm32n6_timerisr.c for the DEFAULT-priority SysTick setup
   * ported from upstream.
   */

  up_prioritize_irq(STM32_IRQ_PENDSV,
                    NVIC_SYSH_PRIORITY_MIN);
  up_prioritize_irq(STM32_IRQ_SVCALL,
                    NVIC_SYSH_SVCALL_PRIORITY);

#ifdef CONFIG_ARM_MPU
  irq_attach(STM32_IRQ_MEMFAULT, arm_memfault, NULL);

  /* NOTE: do NOT call up_enable_irq(STM32_IRQ_MEMFAULT) here.
   * Ported from apache/nuttx upstream stm32_irq.c: on Cortex-M55 in
   * Secure state, setting MEMFAULTENA in SHCSR causes D-cache
   * set/way operations (DCCISW) to silently fail, breaking DMA
   * cache coherency.  MemFault escalates to HardFault when
   * disabled, which is acceptable since arm_hardfault is always
   * attached and still decodes the fault: escalated faults
   * populate CFSR MMFSR bits and MMFAR, and arm_hardfault checks
   * HFSR.FORCED to recover them.  The only trade-off is losing
   * independent MemManage priority/preemption, which does not
   * matter here since all faults are fatal in this configuration.
   * A previous revision of this function called
   * up_enable_irq(STM32_IRQ_MEMFAULT) immediately after attaching
   * the handler, which is exactly the pattern upstream's comment
   * warns against.
   */
#endif

#ifdef CONFIG_ARCH_INTERRUPTSTACK
  arm_stack_color((void *)up_get_intstackbase(0),
                  CONFIG_ARCH_INTERRUPTSTACK);
#endif

  up_irq_enable();
}

/****************************************************************************
 * Name: up_disable_irq
 *
 * Description:
 *   Disable the IRQ specified by 'irq'.
 *
 ****************************************************************************/

void up_disable_irq(int irq)
{
  uintptr_t regaddr;
  uint32_t regval;
  uint32_t bit;

  if (stm32n6_irqinfo(irq, &regaddr, &bit,
                       NVIC_CLRENA_OFFSET) == OK)
    {
      if (irq >= STM32_IRQ_FIRST)
        {
          putreg32(bit, regaddr);
        }
      else
        {
          regval  = getreg32(regaddr);
          regval &= ~bit;
          putreg32(regval, regaddr);
        }
    }
}

/****************************************************************************
 * Name: up_enable_irq
 *
 * Description:
 *   Enable the IRQ specified by 'irq'.
 *
 ****************************************************************************/

void up_enable_irq(int irq)
{
  uintptr_t regaddr;
  uint32_t regval;
  uint32_t bit;

  if (stm32n6_irqinfo(irq, &regaddr, &bit,
                       NVIC_ENA_OFFSET) == OK)
    {
      if (irq >= STM32_IRQ_FIRST)
        {
          putreg32(bit, regaddr);
        }
      else
        {
          regval  = getreg32(regaddr);
          regval |= bit;
          putreg32(regval, regaddr);
        }
    }
}

/****************************************************************************
 * Name: arm_ack_irq
 *
 * Description:
 *   Acknowledge the IRQ.
 *
 ****************************************************************************/

void arm_ack_irq(int irq)
{
  UNUSED(irq);
}

/****************************************************************************
 * Name: up_prioritize_irq
 *
 * Description:
 *   Set the priority of an IRQ.
 *
 ****************************************************************************/

int up_prioritize_irq(int irq, int priority)
{
  uint32_t regaddr;
  uint32_t regval;
  int shift;

  if (irq < STM32_IRQ_FIRST)
    {
      regaddr = NVIC_SYSH_PRIORITY(irq);
    }
  else
    {
      irq -= STM32_IRQ_FIRST;
      regaddr = NVIC_IRQ_PRIORITY(irq);
    }

  shift   = ((irq & 3) << 3);
  regval  = getreg32(regaddr);
  regval &= ~(0xff << shift);
  regval |= (priority << shift);
  putreg32(regval, regaddr);

  return OK;
}
