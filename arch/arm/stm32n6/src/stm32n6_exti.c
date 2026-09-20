/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_exti.c
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

/* ADR-010: EXTI extended interrupt controller driver.
 *
 * The STM32N6 EXTI controller provides up to 96 interrupt/event lines
 * organized in 3 banks of 32 lines each. Lines 0-15 of bank 1 map to
 * GPIO pins via the EXTICR registers. Each line can be configured for
 * rising edge, falling edge, or both edge detection.
 *
 * Register layout (bank 1):
 *   RTSR1    @ 0x00: Rising trigger selection
 *   FTSR1    @ 0x04: Falling trigger selection
 *   SWIER1   @ 0x08: Software interrupt event
 *   RPR1     @ 0x0C: Rising pending (W1C)
 *   FPR1     @ 0x10: Falling pending (W1C)
 *   IMR1     @ 0x80: Interrupt mask
 *   EMR1     @ 0x84: Event mask
 *   EXTICR[4] @ 0x60-0x6C: External interrupt configuration
 *
 * Bank 2 (lines 32+) and bank 3 (lines 64+) follow the same pattern
 * at offsets 0x20 and 0x40 respectively.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include "arm_internal.h"
#include "chip.h"
#include "stm32n6_exti.h"
#include "stm32n6_gpio.h"
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Number of GPIO EXTI lines supported (lines 0-15) */

#define EXTI_NUM_GPIO_LINES  16

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Per-line callback storage */

struct exti_callback_s
{
  xcpt_t callback;
  void  *arg;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Callback table for GPIO EXTI lines 0-15 */

static struct exti_callback_s g_exti_callbacks[EXTI_NUM_GPIO_LINES];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_exti_get_port
 *
 * Description:
 *   Extract GPIO port index (0=A, 1=B, ...) from encoded pinset.
 *
 ****************************************************************************/

static unsigned int stm32n6_exti_get_port(uint32_t pinset)
{
  return (pinset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
}

/****************************************************************************
 * Name: stm32n6_exti_get_pin
 *
 * Description:
 *   Extract GPIO pin number (0-15) from encoded pinset.
 *
 ****************************************************************************/

static unsigned int stm32n6_exti_get_pin(uint32_t pinset)
{
  return (pinset & GPIO_PIN_MASK) >> GPIO_PIN_SHIFT;
}

/****************************************************************************
 * Name: stm32n6_exti_config_line
 *
 * Description:
 *   Configure EXTICR to map a GPIO port to an EXTI line and set
 *   trigger edge polarity.
 *
 ****************************************************************************/

static void stm32n6_exti_config_line(unsigned int port,
                                     unsigned int line,
                                     bool rising,
                                     bool falling)
{
  unsigned int idx;
  unsigned int shift;
  uint32_t regval;
  irqstate_t flags;
  uint32_t mask;

  DEBUGASSERT(line < EXTI_NUM_GPIO_LINES);

  /* EXTICR register index and bit shift for this line */

  idx   = STM32N6_EXTI_EXTICR_INDEX(line);
  shift = STM32N6_EXTI_EXTICR_SHIFT(line);

  flags = up_irq_save();

  /* Configure EXTICR: select which GPIO port maps to this EXTI line.
   * Each EXTICR entry is 8 bits wide, 4 entries per register.
   * Port value: 0=A, 1=B, ... 10=Z.
   */

  regval  = getreg32(STM32N6_EXTI_EXTICR(idx));
  regval &= ~(0xffu << shift);
  regval |= ((uint32_t)port << shift);
  putreg32(regval, STM32N6_EXTI_EXTICR(idx));

  /* Configure rising/falling trigger.
   * Bank 1 RTSR1/FTSR1 cover lines 0-21.
   */

  mask = STM32N6_EXTI_LINE_MASK(line);

  if (rising)
    {
      modifyreg32(STM32N6_EXTI_RTSR1, 0, mask);
    }
  else
    {
      modifyreg32(STM32N6_EXTI_RTSR1, mask, 0);
    }

  if (falling)
    {
      modifyreg32(STM32N6_EXTI_FTSR1, 0, mask);
    }
  else
    {
      modifyreg32(STM32N6_EXTI_FTSR1, mask, 0);
    }

  up_irq_restore(flags);
}

/****************************************************************************
 * Name: stm32n6_exti_enable_interrupt
 *
 * Description:
 *   Enable or disable interrupt for the given EXTI line in IMR1.
 *
 ****************************************************************************/

static void stm32n6_exti_enable_interrupt(unsigned int line, bool enable)
{
  uint32_t mask;
  irqstate_t flags;

  DEBUGASSERT(line < 22);  /* Bank 1 covers lines 0-21 */

  mask  = STM32N6_EXTI_LINE_MASK(line);
  flags = up_irq_save();

  if (enable)
    {
      modifyreg32(STM32N6_EXTI_IMR1, 0, mask);
    }
  else
    {
      modifyreg32(STM32N6_EXTI_IMR1, mask, 0);
    }

  up_irq_restore(flags);
}

/****************************************************************************
 * Name: stm32n6_exti_dispatch
 *
 * Description:
 *   Common dispatcher for a group of EXTI lines. Reads the pending
 *   register and invokes callbacks for active lines.
 *
 ****************************************************************************/

static int stm32n6_exti_dispatch(int irq, void *context,
                                 int first, int last)
{
  uint32_t rpr;
  uint32_t fpr;
  int pin;
  int ret = OK;

  rpr = getreg32(STM32N6_EXTI_RPR1);
  fpr = getreg32(STM32N6_EXTI_FPR1);

  for (pin = first; pin <= last; pin++)
    {
      uint32_t mask = STM32N6_EXTI_LINE_MASK(pin);

      if ((rpr & mask) != 0 || (fpr & mask) != 0)
        {
          /* Clear the pending interrupt(s) */

          putreg32(mask, STM32N6_EXTI_RPR1);
          putreg32(mask, STM32N6_EXTI_FPR1);

          /* Dispatch to registered callback */

          if (g_exti_callbacks[pin].callback != NULL)
            {
              xcpt_t callback = g_exti_callbacks[pin].callback;
              void   *cbarg   = g_exti_callbacks[pin].arg;
              int tmp;

              tmp = callback(irq, context, cbarg);
              if (tmp < 0)
                {
                  ret = tmp;
                }
            }
        }
    }

  return ret;
}

/****************************************************************************
 * Name: stm32n6_exti0_isr through stm32n6_exti4_isr
 *
 * Description:
 *   Individual ISR handlers for EXTI lines 0-4 (one IRQ each).
 *
 ****************************************************************************/

static int stm32n6_exti0_isr(int irq, void *context, void *arg)
{
  int ret = OK;

  putreg32(0x0001, STM32N6_EXTI_RPR1);
  putreg32(0x0001, STM32N6_EXTI_FPR1);

  if (g_exti_callbacks[0].callback != NULL)
    {
      ret = g_exti_callbacks[0].callback(
        irq, context, g_exti_callbacks[0].arg);
    }

  return ret;
}

static int stm32n6_exti1_isr(int irq, void *context, void *arg)
{
  int ret = OK;

  putreg32(0x0002, STM32N6_EXTI_RPR1);
  putreg32(0x0002, STM32N6_EXTI_FPR1);

  if (g_exti_callbacks[1].callback != NULL)
    {
      ret = g_exti_callbacks[1].callback(
        irq, context, g_exti_callbacks[1].arg);
    }

  return ret;
}

static int stm32n6_exti2_isr(int irq, void *context, void *arg)
{
  int ret = OK;

  putreg32(0x0004, STM32N6_EXTI_RPR1);
  putreg32(0x0004, STM32N6_EXTI_FPR1);

  if (g_exti_callbacks[2].callback != NULL)
    {
      ret = g_exti_callbacks[2].callback(
        irq, context, g_exti_callbacks[2].arg);
    }

  return ret;
}

static int stm32n6_exti3_isr(int irq, void *context, void *arg)
{
  int ret = OK;

  putreg32(0x0008, STM32N6_EXTI_RPR1);
  putreg32(0x0008, STM32N6_EXTI_FPR1);

  if (g_exti_callbacks[3].callback != NULL)
    {
      ret = g_exti_callbacks[3].callback(
        irq, context, g_exti_callbacks[3].arg);
    }

  return ret;
}

static int stm32n6_exti4_isr(int irq, void *context, void *arg)
{
  int ret = OK;

  putreg32(0x0010, STM32N6_EXTI_RPR1);
  putreg32(0x0010, STM32N6_EXTI_FPR1);

  if (g_exti_callbacks[4].callback != NULL)
    {
      ret = g_exti_callbacks[4].callback(
        irq, context, g_exti_callbacks[4].arg);
    }

  return ret;
}

/****************************************************************************
 * Name: stm32n6_exti5_9_isr
 *
 * Description:
 *   Shared ISR handler for EXTI lines 5-9.
 *
 ****************************************************************************/

static int stm32n6_exti5_9_isr(int irq, void *context, void *arg)
{
  return stm32n6_exti_dispatch(irq, context, 5, 9);
}

/****************************************************************************
 * Name: stm32n6_exti10_15_isr
 *
 * Description:
 *   Shared ISR handler for EXTI lines 10-15.
 *
 ****************************************************************************/

static int stm32n6_exti10_15_isr(int irq, void *context, void *arg)
{
  return stm32n6_exti_dispatch(irq, context, 10, 15);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_exti_initialize
 *
 * Description:
 *   Initialize the EXTI controller. Clears all pending interrupts
 *   and disables all interrupt/event mask registers.
 *
 * Returned Value:
 *   Zero (OK) on success.
 *
 ****************************************************************************/

int stm32n6_exti_initialize(void)
{
  /* Disable all interrupts in bank 1 (lines 0-21) */

  putreg32(0, STM32N6_EXTI_IMR1);
  putreg32(0, STM32N6_EXTI_EMR1);

  /* Clear all pending bits in bank 1 */

  putreg32(0xffffffff, STM32N6_EXTI_RPR1);
  putreg32(0xffffffff, STM32N6_EXTI_FPR1);

  /* Clear trigger configuration */

  putreg32(0, STM32N6_EXTI_RTSR1);
  putreg32(0, STM32N6_EXTI_FTSR1);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_gpiosetevent
 *
 * Description:
 *   Sets/clears GPIO-based event and interrupt triggers on an EXTI line.
 *
 *   The pinset encodes both the GPIO port (A-Z) and pin number (0-15).
 *   Only lines 0-15 can be mapped to GPIO pins.
 *
 * Input Parameters:
 *   - pinset:      GPIO pin configuration (encodes port and pin number)
 *   - risingedge:  Enable interrupt on rising edges
 *   - fallingedge: Enable interrupt on falling edges
 *   - event:       Generate event when set
 *   - func:        When non-NULL, interrupt callback handler
 *   - arg:         Argument passed to the interrupt callback
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_gpiosetevent(uint32_t pinset, bool risingedge,
                         bool fallingedge, bool event,
                         xcpt_t func, void *arg)
{
  unsigned int port;
  unsigned int pin;
  int irq;
  xcpt_t handler;
  int nshared;

  port = stm32n6_exti_get_port(pinset);
  pin  = stm32n6_exti_get_pin(pinset);

  if (pin >= EXTI_NUM_GPIO_LINES)
    {
      return -EINVAL;
    }

  /* Select the IRQ and handler based on pin number.
   * EXTI0-EXTI4 each have a dedicated IRQ.
   * EXTI5-EXTI9 share one IRQ.
   * EXTI10-EXTI15 share one IRQ.
   */

  if (pin < 5)
    {
      irq     = STM32_IRQ_EXTI0 + pin;
      nshared = 1;

      switch (pin)
        {
          case 0:
            handler = stm32n6_exti0_isr;
            break;

          case 1:
            handler = stm32n6_exti1_isr;
            break;

          case 2:
            handler = stm32n6_exti2_isr;
            break;

          case 3:
            handler = stm32n6_exti3_isr;
            break;

          default:
            handler = stm32n6_exti4_isr;
            break;
        }
    }
  else if (pin < 10)
    {
      irq     = STM32_IRQ_EXTI5;
      handler = stm32n6_exti5_9_isr;
      nshared = 5;
    }
  else
    {
      irq     = STM32_IRQ_EXTI10;
      handler = stm32n6_exti10_15_isr;
      nshared = 6;
    }

  /* Store the callback */

  g_exti_callbacks[pin].callback = func;
  g_exti_callbacks[pin].arg      = arg;

  /* Install or remove the interrupt handler */

  if (func)
    {
      irq_attach(irq, handler, NULL);
      up_enable_irq(irq);
    }
  else
    {
      /* Only disable IRQ if no other lines in the shared group
       * have active callbacks.
       */

      int first;
      int idx;

      if (pin < 5)
        {
          first = pin;
        }
      else if (pin < 10)
        {
          first = 5;
        }
      else
        {
          first = 10;
        }

      for (idx = first; idx < first + nshared; idx++)
        {
          if (g_exti_callbacks[idx].callback != NULL)
            {
              break;
            }
        }

      if (idx == first + nshared)
        {
          up_disable_irq(irq);
        }
    }

  /* Configure the EXTI line: port mapping and trigger edges */

  stm32n6_exti_config_line(port, pin, risingedge, fallingedge);

  /* Enable or disable the interrupt mask for this line */

  stm32n6_exti_enable_interrupt(pin, func != NULL);

  /* Enable or disable the event mask for this line */

  {
    uint32_t mask = STM32N6_EXTI_LINE_MASK(pin);

    if (event)
      {
        modifyreg32(STM32N6_EXTI_EMR1, 0, mask);
      }
    else
      {
        modifyreg32(STM32N6_EXTI_EMR1, mask, 0);
      }
  }

  return OK;
}
