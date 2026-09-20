/****************************************************************************
 * vendor/openvela/boards/atk-dnn647/src/stm32n6_buzzer.c
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

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "chip.h"
#include "hardware/stm32_tim.h"
#include "hardware/stm32_rcc.h"

#include "stm32n6_gpio.h"

#include "stm32n6_buzzer.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Time base the tone engine runs at.  With a 1 MHz counter the auto-reload
 * register holds microseconds, so half a period is 500000 / hz ticks.
 */

#define BUZZER_TICK_FREQ   1000000

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Sustained tone state.  The frequency is what the interrupt compares
 * against, the level is what it flips; both are touched from the interrupt,
 * so they are volatile.
 */

static volatile uint32_t g_tone_hz;
static volatile bool     g_tone_level;

/* Cleared until the timer has a clock and an interrupt handler. */

static bool g_tone_ready;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_buzzer_tone_isr
 *
 * Description:
 *   Timer interrupt: flip the pin, which is the note.
 *
 ****************************************************************************/

static int stm32n6_buzzer_tone_isr(int irq, FAR void *context, FAR void *arg)
{
  putreg32(0, STM32_TIM3_BASE + STM32_TIM_SR_OFFSET);

  g_tone_level = !g_tone_level;
  stm32n6_gpiowrite(GPIO_BEEP, g_tone_level);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_buzzer_tone_engine
 *
 * Description:
 *   Give TIM3 an update period of half the note and let it run.  The
 *   interrupt enable is left to the caller so the first edge cannot arrive
 *   before the state is consistent.
 *
 ****************************************************************************/

static void stm32n6_buzzer_tone_engine(uint32_t hz)
{
  uint32_t psc = (STM32_APB1_TIM_FREQUENCY / BUZZER_TICK_FREQ) - 1;
  uint32_t arr = (BUZZER_TICK_FREQ / 2 / hz) - 1;

  putreg32(0, STM32_TIM3_BASE + STM32_TIM_CR1_OFFSET);
  putreg32(0, STM32_TIM3_BASE + STM32_TIM_DIER_OFFSET);
  putreg32(psc, STM32_TIM3_BASE + STM32_TIM_PSC_OFFSET);
  putreg32(arr, STM32_TIM3_BASE + STM32_TIM_ARR_OFFSET);
  putreg32(TIM_EGR_UG, STM32_TIM3_BASE + STM32_TIM_EGR_OFFSET);
  putreg32(0, STM32_TIM3_BASE + STM32_TIM_SR_OFFSET);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_buzzer_initialize
 *
 * Description:
 *   Take the buzzer pin and leave it silent.  Idempotent, so every entry
 *   point can just call it.
 *
 *   PD3 reaches the buzzer through a transistor: the pin drives its base
 *   through 1 k and the buzzer hangs off 3.3 V, so the pin is a switch and
 *   a square wave on it is a square wave across the buzzer.  The buzzer has
 *   no oscillator of its own, which is what makes it a musical one.
 *
 ****************************************************************************/

void stm32n6_buzzer_initialize(void)
{
  stm32n6_configgpio(GPIO_BEEP);
  stm32n6_gpiowrite(GPIO_BEEP, false);
}

/****************************************************************************
 * Name: stm32n6_buzzer_beep
 *
 * Description:
 *   Sound the buzzer for the given number of milliseconds, at its natural
 *   frequency (whatever that is - a passive buzzer has none, so this is a
 *   plain on/off which reads as a click).
 *
 ****************************************************************************/

void stm32n6_buzzer_beep(uint32_t ms)
{
  if (ms == 0)
    {
      return;
    }

  stm32n6_buzzer_initialize();
  stm32n6_gpiowrite(GPIO_BEEP, true);
  up_mdelay(ms);
  stm32n6_gpiowrite(GPIO_BEEP, false);
}

/****************************************************************************
 * Name: stm32n6_buzzer_beeps
 *
 * Description:
 *   Sound a pattern: count notes of on_ms each, separated by gap_ms.
 *
 ****************************************************************************/

void stm32n6_buzzer_beeps(int count, uint32_t on_ms, uint32_t gap_ms)
{
  int i;

  if (on_ms == 0)
    {
      return;
    }

  for (i = 0; i < count; i++)
    {
      if (i > 0 && gap_ms > 0)
        {
          up_mdelay(gap_ms);
        }

      stm32n6_buzzer_beep(on_ms);
    }
}

/****************************************************************************
 * Name: stm32n6_buzzer_tone
 *
 * Description:
 *   Sound a note of the given frequency for the given time.  Blocking, for
 *   the case where the caller has nothing else to do for that long - a
 *   single short announcement.
 *
 ****************************************************************************/

void stm32n6_buzzer_tone(uint32_t hz, uint32_t ms)
{
  uint32_t half_us;
  uint32_t cycles;
  uint32_t i;

  if (hz == 0 || ms == 0)
    {
      return;
    }

  /* Half a period in microseconds.  Below 50 Hz this stops being a note and
   * starts being a series of clicks, so the period is capped there.
   */

  half_us = 500000u / hz;
  if (half_us == 0)
    {
      half_us = 1;
    }
  else if (half_us > 10000u)
    {
      half_us = 10000u;
    }

  cycles = (ms / 1000u) * hz + ((ms % 1000u) * hz) / 1000u;

  stm32n6_buzzer_initialize();

  for (i = 0; i < cycles; i++)
    {
      stm32n6_gpiowrite(GPIO_BEEP, true);
      up_udelay(half_us);
      stm32n6_gpiowrite(GPIO_BEEP, false);
      up_udelay(half_us);
    }
}

/****************************************************************************
 * Name: stm32n6_buzzer_tone_async
 *
 * Description:
 *   Start, or keep, a note running without occupying the caller.
 *
 *   Calling this repeatedly with the same frequency is a no-op, which is
 *   what lets the interface loop say "still holding this action" once per
 *   frame without restarting the note every time.
 *
 ****************************************************************************/

void stm32n6_buzzer_tone_async(uint32_t hz)
{
  uint32_t regval;

  if (hz == 0)
    {
      stm32n6_buzzer_tone_stop();
      return;
    }

  if (!g_tone_ready)
    {
      stm32n6_buzzer_initialize();

      regval  = getreg32(STM32_RCC_APB1ENR1);
      regval |= RCC_APB1ENR1_TIM3EN;
      putreg32(regval, STM32_RCC_APB1ENR1);

      regval  = getreg32(STM32_RCC_APB1LPENR1);
      regval |= RCC_APB1LPENR1_TIM3LPEN;
      putreg32(regval, STM32_RCC_APB1LPENR1);

      irq_attach(STM32_IRQ_TIM3, stm32n6_buzzer_tone_isr, NULL);
      up_enable_irq(STM32_IRQ_TIM3);

      g_tone_ready = true;
    }

  if (g_tone_hz == hz && (getreg32(STM32_TIM3_BASE + STM32_TIM_CR1_OFFSET) &
                          TIM_CR1_CEN) != 0)
    {
      return;
    }

  stm32n6_buzzer_tone_stop();

  stm32n6_buzzer_tone_engine(hz);

  g_tone_level = false;
  g_tone_hz    = hz;

  putreg32(TIM_DIER_UIE, STM32_TIM3_BASE + STM32_TIM_DIER_OFFSET);
  putreg32(TIM_CR1_CEN, STM32_TIM3_BASE + STM32_TIM_CR1_OFFSET);
}

/****************************************************************************
 * Name: stm32n6_buzzer_tone_stop
 *
 * Description:
 *   Silence the buzzer and stop the timer.  Safe to call when nothing is
 *   playing, which keeps the callers free of state.
 *
 ****************************************************************************/

void stm32n6_buzzer_tone_stop(void)
{
  if (!g_tone_ready)
    {
      return;
    }

  putreg32(0, STM32_TIM3_BASE + STM32_TIM_CR1_OFFSET);
  putreg32(0, STM32_TIM3_BASE + STM32_TIM_DIER_OFFSET);
  putreg32(0, STM32_TIM3_BASE + STM32_TIM_SR_OFFSET);

  g_tone_hz    = 0;
  g_tone_level = false;

  stm32n6_gpiowrite(GPIO_BEEP, false);
}
