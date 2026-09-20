/****************************************************************************
 * vendor/openvela/boards/atk-dnn647/src/stm32n6_userbuttons.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <arch/board/board.h>

#include "chip.h"
#include "arm_internal.h"
#include "stm32n6_gpio.h"
#include "stm32n6_exti.h"

#ifdef CONFIG_ARCH_BUTTONS

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Pin configuration for each ALIENTEK ATK-DNN647 button.  Indexed by the
 * BUTTON_* definitions in board.h:
 *
 *   BUTTON_KEY0 (PC6)   - pull-up input, pressed = low
 *   BUTTON_KEY1 (PD1)   - pull-up input, pressed = low
 *   BUTTON_KEY2 (PG11)  - pull-up input, pressed = low
 *   BUTTON_WKUP (PC13)  - pull-down input, pressed = high
 */

static const uint32_t g_buttons[NUM_BUTTONS] =
{
  GPIO_BTN_KEY0,
  GPIO_BTN_KEY1,
  GPIO_BTN_KEY2,
  GPIO_BTN_WKUP,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_button_initialize
 *
 * Description:
 *   Configure the button GPIOs as inputs.
 *
 ****************************************************************************/

uint32_t board_button_initialize(void)
{
  int i;

  for (i = 0; i < NUM_BUTTONS; i++)
    {
      stm32n6_configgpio(g_buttons[i]);
    }

  return NUM_BUTTONS;
}

/****************************************************************************
 * Name: board_buttons
 *
 * Description:
 *   Return the set of depressed buttons as a bit set.
 *
 ****************************************************************************/

uint32_t board_buttons(void)
{
  uint32_t ret = 0;
  int i;

  for (i = 0; i < NUM_BUTTONS; i++)
    {
      bool high = stm32n6_gpioread(g_buttons[i]);
      bool pressed;

      /* KEY0/1/2 are active-low; WKUP is active-high */

      if (i == BUTTON_WKUP)
        {
          pressed = high;
        }
      else
        {
          pressed = !high;
        }

      if (pressed)
        {
          ret |= (1 << i);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: board_button_irq
 *
 * Description:
 *   Attach/detach an EXTI interrupt handler for one button.
 *
 ****************************************************************************/

#ifdef CONFIG_ARCH_IRQBUTTONS
int board_button_irq(int id, xcpt_t irqhandler, void *arg)
{
  bool rising;
  bool falling;
  int ret = -EINVAL;

  if (id >= 0 && id < NUM_BUTTONS)
    {
      /* KEY0/1/2 are pressed on the falling edge; WKUP on the rising edge */

      if (id == BUTTON_WKUP)
        {
          rising  = true;
          falling = false;
        }
      else
        {
          rising  = false;
          falling = true;
        }

      ret = stm32n6_gpiosetevent(g_buttons[id], rising, falling,
                                 false, irqhandler, arg);
    }

  return ret;
}
#endif /* CONFIG_ARCH_IRQBUTTONS */
#endif /* CONFIG_ARCH_BUTTONS */
