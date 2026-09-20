/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_gpio.c
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

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include <nuttx/spinlock.h>

#include "arm_internal.h"
#include "stm32n6_gpio.h"
#include "hardware/stm32_memorymap.h"
#include "hardware/stm32_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* GPIO register offsets */

#define GPIO_MODER_OFFSET    0x00
#define GPIO_OTYPER_OFFSET   0x04
#define GPIO_OSPEEDR_OFFSET  0x08
#define GPIO_PUPDR_OFFSET    0x0c
#define GPIO_IDR_OFFSET      0x10
#define GPIO_ODR_OFFSET      0x14
#define GPIO_BSRR_OFFSET     0x18
#define GPIO_AFRL_OFFSET     0x20
#define GPIO_AFRH_OFFSET     0x24

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Serializes read-modify-write access to the GPIO configuration
 * registers.  Ported from apache/nuttx upstream stm32_gpio.c, which
 * protects against concurrent stm32_configgpio() calls racing on the
 * same port's MODER/OTYPER/OSPEEDR/PUPDR/AFRL/AFRH registers.
 */

static spinlock_t g_configgpio_lock = SP_UNLOCKED;

/* Port base address table.
 *
 * CMSIS stm32n647xx.h confirms A-H + N/O/P/Q (12 ports); see the
 * note in stm32n6_gpio.h.
 */

static const uintptr_t g_gpiobase[] =
{
  STM32_GPIOA_BASE,   /* 0: GPIOA */
  STM32_GPIOB_BASE,   /* 1: GPIOB */
  STM32_GPIOC_BASE,   /* 2: GPIOC */
  STM32_GPIOD_BASE,   /* 3: GPIOD */
  STM32_GPIOE_BASE,   /* 4: GPIOE */
  STM32_GPIOF_BASE,   /* 5: GPIOF */
  STM32_GPIOG_BASE,   /* 6: GPIOG */
  STM32_GPIOH_BASE,   /* 7: GPIOH */
  STM32_GPION_BASE,   /* 8: GPION */
  STM32_GPIOO_BASE,   /* 9: GPIOO */
  STM32_GPIOP_BASE,   /* 10: GPIOP */
  STM32_GPIOQ_BASE,   /* 11: GPIOQ */
};

/* Per-port AHB4ENR clock-enable bit, indexed identically to g_gpiobase.
 * GPION/O/P/Q are NOT contiguous with GPIOA-H (bits 13-16 vs 0-7), so a
 * table lookup is required.  Positions verified against CMSIS
 * stm32n647xx.h RCC_AHB4ENR_GPIOxEN_Pos.
 */

static const uint32_t g_gpioclken[] =
{
  RCC_AHB4ENR_GPIOAEN,   /* 0: GPIOA */
  RCC_AHB4ENR_GPIOBEN,   /* 1: GPIOB */
  RCC_AHB4ENR_GPIOCEN,   /* 2: GPIOC */
  RCC_AHB4ENR_GPIODEN,   /* 3: GPIOD */
  RCC_AHB4ENR_GPIOEEN,   /* 4: GPIOE */
  RCC_AHB4ENR_GPIOFEN,   /* 5: GPIOF */
  RCC_AHB4ENR_GPIOGEN,   /* 6: GPIOG */
  RCC_AHB4ENR_GPIOHEN,   /* 7: GPIOH */
  RCC_AHB4ENR_GPIONEN,   /* 8: GPION */
  RCC_AHB4ENR_GPIOOEN,   /* 9: GPIOO */
  RCC_AHB4ENR_GPIOPEN,   /* 10: GPIOP */
  RCC_AHB4ENR_GPIOQEN,   /* 11: GPIOQ */
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_configgpio
 *
 * Description:
 *   Configure a GPIO pin based on encoded pin attributes.
 *
 * Returned Value:
 *   OK (0) on success; a negated errno value (-EINVAL) if the port
 *   field decodes to an unsupported port index.
 *
 ****************************************************************************/

int stm32n6_configgpio(uint32_t cfgset)
{
  unsigned int port;
  unsigned int pin;
  unsigned int mode;
  unsigned int af;
  unsigned int speed;
  unsigned int pupd;
  unsigned int otype;
  uintptr_t base;
  uint32_t regval;
  irqstate_t flags;

  port  = (cfgset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
  pin   = (cfgset & GPIO_PIN_MASK)  >> GPIO_PIN_SHIFT;
  mode  = (cfgset & GPIO_MODE_MASK) >> GPIO_MODE_SHIFT;
  af    = (cfgset & GPIO_AF_MASK)   >> GPIO_AF_SHIFT;
  speed = (cfgset >> GPIO_SPEED_SHIFT) & 0x3;
  pupd  = (cfgset >> GPIO_PUPD_SHIFT)  & 0x3;
  otype = (cfgset >> GPIO_OTYPE_SHIFT) & 0x1;

  if (port >= sizeof(g_gpiobase) / sizeof(g_gpiobase[0]))
    {
      return -EINVAL;
    }

  base = g_gpiobase[port];

  /* Enable the port's peripheral clock before touching any of its
   * registers.  The reset RCC state leaves most GPIO port clocks
   * gated, so writes to a gated port are silently dropped and reads
   * return zero -- which on real silicon manifests as an output pin
   * that never drives and an input that never changes.  (Renode does
   * not model this gating, so it masks the bug.)  Serialize the
   * read-modify-write with the same lock used for the port config
   * registers below.
   */

  flags = spin_lock_irqsave(&g_configgpio_lock);
  /* AHB4ENR is READ-ONLY status on STM32N6; clock gating is done via
   * the AHB4ENSR write-1-to-set alias.  Writing ENR silently does
   * nothing (ports stay gated -> pins never drive). */
  putreg32(g_gpioclken[port], STM32_RCC_AHB4ENSR);
  spin_unlock_irqrestore(&g_configgpio_lock, flags);

  /* If this pin is being configured as an output, drive the
   * requested initial level (GPIO_OUTPUT_SET) on the ODR/BSRR
   * *before* switching MODER to output below.  Ported from
   * apache/nuttx upstream stm32_configgpio(): setting the output
   * level ahead of the mode switch avoids a brief glitch where the
   * pin would otherwise momentarily drive whatever stale ODR value
   * was already latched from a previous (e.g. input/analog)
   * configuration.
   */

  if (mode == (GPIO_MODE_OUTPUT >> GPIO_MODE_SHIFT))
    {
      stm32n6_gpiowrite(cfgset, (cfgset & GPIO_OUTPUT_SET) != 0);
    }

  /* Interrupts must be disabled from here on out so that we have
   * mutually exclusive access to all of the GPIO configuration
   * registers for this port.
   */

  flags = spin_lock_irqsave(&g_configgpio_lock);

  /* Set mode (2 bits per pin) */

  regval  = getreg32(base + GPIO_MODER_OFFSET);
  regval &= ~(3u << (pin * 2));
  regval |= (mode << (pin * 2));
  putreg32(regval, base + GPIO_MODER_OFFSET);

  /* Set output type (1 bit per pin) */

  regval  = getreg32(base + GPIO_OTYPER_OFFSET);
  regval &= ~(1u << pin);
  regval |= (otype << pin);
  putreg32(regval, base + GPIO_OTYPER_OFFSET);

  /* Set speed (2 bits per pin) */

  regval  = getreg32(base + GPIO_OSPEEDR_OFFSET);
  regval &= ~(3u << (pin * 2));
  regval |= (speed << (pin * 2));
  putreg32(regval, base + GPIO_OSPEEDR_OFFSET);

  /* Set pull-up/down (2 bits per pin) */

  regval  = getreg32(base + GPIO_PUPDR_OFFSET);
  regval &= ~(3u << (pin * 2));
  regval |= (pupd << (pin * 2));
  putreg32(regval, base + GPIO_PUPDR_OFFSET);

  /* Set alternate function (4 bits per pin) */

  if (mode == 2)
    {
      uintptr_t afr_reg;
      unsigned int afr_shift;

      if (pin < 8)
        {
          afr_reg   = base + GPIO_AFRL_OFFSET;
          afr_shift = pin * 4;
        }
      else
        {
          afr_reg   = base + GPIO_AFRH_OFFSET;
          afr_shift = (pin - 8) * 4;
        }

      regval  = getreg32(afr_reg);
      regval &= ~(0xfu << afr_shift);
      regval |= (af << afr_shift);
      putreg32(regval, afr_reg);
    }

  spin_unlock_irqrestore(&g_configgpio_lock, flags);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_unconfiggpio
 *
 * Description:
 *   Unconfigure a GPIO pin: reuse the port and pin fields from cfgset
 *   and reconfigure that pin to a default, safe, high-impedance input
 *   with no pull-up/down.  Ported from apache/nuttx upstream
 *   stm32_unconfiggpio(): this is a safety function, primarily meant
 *   to be called before repurposing a pin that was previously driven
 *   as a fixed-level GPIO output or PWM/timer channel output, so the
 *   pin does not keep driving a stale fixed level (which, for a motor
 *   or LED driver channel, can trigger an over-current condition)
 *   once the peripheral or application code that owned it is done.
 *
 * Returned Value:
 *   OK (0) on success; a negated errno value (-EINVAL) if the port
 *   field decodes to an unsupported port index.
 *
 ****************************************************************************/

int stm32n6_unconfiggpio(uint32_t cfgset)
{
  cfgset &= GPIO_PORT_MASK | GPIO_PIN_MASK;
  cfgset |= GPIO_MODE_INPUT | GPIO_PUPD_NONE;

  return stm32n6_configgpio(cfgset);
}

/****************************************************************************
 * Name: stm32n6_gpioread
 *
 * Description:
 *   Read the current state of a GPIO pin.
 *
 ****************************************************************************/

bool stm32n6_gpioread(uint32_t pinset)
{
  unsigned int port;
  unsigned int pin;
  uintptr_t base;

  port = (pinset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
  pin  = (pinset & GPIO_PIN_MASK)  >> GPIO_PIN_SHIFT;

  if (port >= sizeof(g_gpiobase) / sizeof(g_gpiobase[0]))
    {
      return false;
    }

  base = g_gpiobase[port];
  return (getreg32(base + GPIO_IDR_OFFSET) & (1u << pin)) != 0;
}

/****************************************************************************
 * Name: stm32n6_gpiowrite
 *
 * Description:
 *   Set or clear a GPIO pin using the bit-set/reset register.
 *
 ****************************************************************************/

void stm32n6_gpiowrite(uint32_t pinset, bool value)
{
  unsigned int port;
  unsigned int pin;
  uintptr_t base;

  port = (pinset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
  pin  = (pinset & GPIO_PIN_MASK)  >> GPIO_PIN_SHIFT;

  if (port >= sizeof(g_gpiobase) / sizeof(g_gpiobase[0]))
    {
      return;
    }

  base = g_gpiobase[port];

  if (value)
    {
      putreg32(1u << pin, base + GPIO_BSRR_OFFSET);
    }
  else
    {
      putreg32(1u << (pin + 16), base + GPIO_BSRR_OFFSET);
    }
}

/****************************************************************************
 * Name: stm32n6_gpiosetevent
 *
 * Description:
 *   Attach a pin-change interrupt handler.  STUB: EXTI-based pin interrupts
 *   are not ported for stm32n6 yet; always returns -ENOSYS so boards that
 *   reference the symbol (e.g. userbuttons CONFIG_ARCH_IRQBUTTONS) link.
 *
 ****************************************************************************/

int stm32n6_gpiosetevent(uint32_t pinset, bool risingedge, bool fallingedge,
                         bool either, xcpt_t func, void *arg)
{
  return -ENOSYS;
}
