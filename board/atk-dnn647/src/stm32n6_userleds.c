/****************************************************************************
 * vendor/openvela/boards/atk-dnn647/src/stm32n6_userleds.c
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

#include <nuttx/board.h>
#include <nuttx/leds/userled.h>

#include <arch/board/board.h>

#include "chip.h"
#include "arm_internal.h"
#include "stm32n6_gpio.h"

#ifndef CONFIG_ARCH_LEDS

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Map the logical LED number to its GPIO configuration.
 *
 * ALIENTEK ATK-DNN647:
 *   LED0 = PG10, LED1 = PE10  (active-low, see board.h GPIO_LED0/GPIO_LED1)
 *
 * Both LEDs are driven active-low, so board_userled() / board_userled_all()
 * invert the logical on/off state before writing the pin.
 */

static const uint32_t g_ledcfg[BOARD_NLEDS] =
{
  GPIO_LED0,
  GPIO_LED1,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_userled_initialize
 *
 * Description:
 *   Configure the LED GPIOs as push-pull outputs.  The GPIO pins are
 *   initialized high (LED off) via GPIO_OUTPUT_SET in the pin config.
 *
 ****************************************************************************/

uint32_t board_userled_initialize(void)
{
  int i;

  for (i = 0; i < BOARD_NLEDS; i++)
    {
      stm32n6_configgpio(g_ledcfg[i]);
    }

  return BOARD_NLEDS;
}

/****************************************************************************
 * Name: board_userled
 *
 * Description:
 *   Set the state of one LED.  LED GPIOs are active-low on this board.
 *
 ****************************************************************************/

void board_userled(int led, bool ledon)
{
  if ((unsigned)led < BOARD_NLEDS)
    {
      stm32n6_gpiowrite(g_ledcfg[led], !ledon);
    }
}

/****************************************************************************
 * Name: board_userled_all
 *
 * Description:
 *   Set the state of all LEDs.  LED GPIOs are active-low on this board.
 *
 ****************************************************************************/

void board_userled_all(uint32_t ledset)
{
  int i;

  for (i = 0; i < BOARD_NLEDS; i++)
    {
      stm32n6_gpiowrite(g_ledcfg[i], (ledset & (1 << i)) == 0);
    }
}

#endif /* !CONFIG_ARCH_LEDS */
