/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_gpio.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_GPIO_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_GPIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

#ifdef CONFIG_DEV_GPIO
#  include <nuttx/ioexpander/gpio.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* GPIO pin encoding:
 *
 *   3322 2222 2222 1111 1111 11
 *   1098 7654 3210 9876 5432 1098 7654 3210
 *   ---- ---- ---- ---- ---- ---- ---- ----
 *   MMOO PPPP SSAA AAFF FFTT TTBB BBBB BBBB
 *
 * M = mode (2 bits: input/output/af/analog)
 * O = output type (1 bit: push-pull/open-drain) + speed (1 bit)
 * P = pull-up/down (2 bits) + reserved (2 bits)
 * S = speed (2 bits)
 * A = alternate function (4 bits)
 * F = reserved flags (4 bits)
 * T = port (4 bits: 0=A, 1=B, ...)
 * B = pin (4 bits: 0-15)
 *
 * Simplified encoding for skeleton:
 */

/* Mode field */

#define GPIO_MODE_SHIFT      30
#define GPIO_MODE_MASK       (3ul << GPIO_MODE_SHIFT)
#define GPIO_MODE_INPUT      (0ul << GPIO_MODE_SHIFT)
#define GPIO_MODE_OUTPUT     (1ul << GPIO_MODE_SHIFT)
#define GPIO_MODE_AF         (2ul << GPIO_MODE_SHIFT)
#define GPIO_MODE_ANALOG     (3ul << GPIO_MODE_SHIFT)

/* Output type */

#define GPIO_OTYPE_SHIFT     28
#define GPIO_OTYPE_PP        (0ul << GPIO_OTYPE_SHIFT)
#define GPIO_OTYPE_OD        (1ul << GPIO_OTYPE_SHIFT)

/* Speed */

#define GPIO_SPEED_SHIFT     24
#define GPIO_SPEED_LOW       (0ul << GPIO_SPEED_SHIFT)
#define GPIO_SPEED_HIGH      (3ul << GPIO_SPEED_SHIFT)

/* Pull-up/down
 *
 * Bit 20 (GPIO_OUTPUT_SET) selects the initial output level when
 * GPIO_MODE_OUTPUT is configured: 1=high, 0=low.  configgpio()
 * applies this level before the pin's MODER bits are switched to
 * output, avoiding a brief output glitch on the transition from
 * whatever the pin's previous mode was (ported from apache/nuttx
 * upstream stm32_configgpio(), which does the same ordering).
 */

#define GPIO_OUTPUT_SET_SHIFT 20
#define GPIO_OUTPUT_SET      (1ul << GPIO_OUTPUT_SET_SHIFT)
#define GPIO_OUTPUT_CLEAR    (0ul << GPIO_OUTPUT_SET_SHIFT)

#define GPIO_PUPD_SHIFT      22
#define GPIO_PUPD_NONE       (0ul << GPIO_PUPD_SHIFT)
#define GPIO_PUPD_PU         (1ul << GPIO_PUPD_SHIFT)
#define GPIO_PUPD_PD         (2ul << GPIO_PUPD_SHIFT)

/* Alternate function */

#define GPIO_AF_SHIFT        16
#define GPIO_AF_MASK         (0xful << GPIO_AF_SHIFT)
#define GPIO_AF(n)           ((uint32_t)(n) << GPIO_AF_SHIFT)

/* Port
 *
 * CMSIS stm32n647xx.h confirms this chip has GPIO ports A-H plus
 * N/O/P/Q (12 ports total, indices 0-11 below) -- there is no
 * GPIOI/GPIOJ/GPIOZ on this part.  A previous revision of this
 * header encoded a fictitious port set (A-J + Z); corrected to
 * match CMSIS and the apache/nuttx upstream STM32N657 port, which
 * uses the same A-H + N/O/P/Q port set.
 */

#define GPIO_PORT_SHIFT      4
#define GPIO_PORT_MASK       (0xful << GPIO_PORT_SHIFT)
#define GPIO_PORTA           (0ul << GPIO_PORT_SHIFT)
#define GPIO_PORTB           (1ul << GPIO_PORT_SHIFT)
#define GPIO_PORTC           (2ul << GPIO_PORT_SHIFT)
#define GPIO_PORTD           (3ul << GPIO_PORT_SHIFT)
#define GPIO_PORTE           (4ul << GPIO_PORT_SHIFT)
#define GPIO_PORTF           (5ul << GPIO_PORT_SHIFT)
#define GPIO_PORTG           (6ul << GPIO_PORT_SHIFT)
#define GPIO_PORTH           (7ul << GPIO_PORT_SHIFT)
#define GPIO_PORTN           (8ul << GPIO_PORT_SHIFT)
#define GPIO_PORTO           (9ul << GPIO_PORT_SHIFT)
#define GPIO_PORTP           (10ul << GPIO_PORT_SHIFT)
#define GPIO_PORTQ           (11ul << GPIO_PORT_SHIFT)

/* Pin */

#define GPIO_PIN_SHIFT       0
#define GPIO_PIN_MASK        (0xful << GPIO_PIN_SHIFT)
#define GPIO_PIN(n)          ((uint32_t)(n) << GPIO_PIN_SHIFT)


/* Buzzer: PD3.  The board fits an active buzzer - one with its own
 * oscillator - so the vendor BSP sounds it with a bare pin write and never
 * involves a timer.  High is on.
 */

#define GPIO_BEEP \
  (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | GPIO_SPEED_LOW | GPIO_PUPD_NONE | \
   GPIO_PORTD | GPIO_PIN(3))


/* Buzzer: PD3.  The board fits an active buzzer - one with its own
 * oscillator - so the vendor BSP sounds it with a bare pin write and never
 * involves a timer.  High is on.
 */

#define GPIO_BEEP \
  (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | GPIO_SPEED_LOW | GPIO_PUPD_NONE | \
   GPIO_PORTD | GPIO_PIN(3))

/* USART1 pins: PE5=TX AF7, PE6=RX AF7 */

#define GPIO_USART1_TX \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_PU | GPIO_AF(7) | GPIO_PORTE | GPIO_PIN(5))

#define GPIO_USART1_RX \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_PU | GPIO_AF(7) | GPIO_PORTE | GPIO_PIN(6))

/* LPTIM2 PWM output: PF1=LPTIM2_CH1 AF3 (push-pull, no pull) */

#define GPIO_LPTIM2_CH1 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(3) | GPIO_PORTF | GPIO_PIN(1))

/* XSPI1 pins — ALIENTEK ATK-DNN647 HyperRAM (W958D8NBYA5I), AF9
 * (SoftwarePackage FSBL stm32n6xx_hal_msp.c)
 *   PP0-7 = XSPIM_P1_IO0-7, PO0 = NCS1, PO2 = DQS0, PO4 = CLK, PO5 = NCLK
 */

#define GPIO_XSPI1_IO0 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTP | GPIO_PIN(0))
#define GPIO_XSPI1_IO1 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTP | GPIO_PIN(1))
#define GPIO_XSPI1_IO2 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTP | GPIO_PIN(2))
#define GPIO_XSPI1_IO3 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTP | GPIO_PIN(3))
#define GPIO_XSPI1_IO4 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTP | GPIO_PIN(4))
#define GPIO_XSPI1_IO5 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTP | GPIO_PIN(5))
#define GPIO_XSPI1_IO6 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTP | GPIO_PIN(6))
#define GPIO_XSPI1_IO7 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTP | GPIO_PIN(7))
#define GPIO_XSPI1_NCS1 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTO | GPIO_PIN(0))
#define GPIO_XSPI1_DQS0 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTO | GPIO_PIN(2))
#define GPIO_XSPI1_CLK \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTO | GPIO_PIN(4))
#define GPIO_XSPI1_NCLK \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTO | GPIO_PIN(5))

/* XSPI2 pins — ALIENTEK ATK-DNN647 NOR Flash (MX25UM25645G), AF9
 * (SoftwarePackage FSBL stm32n6xx_hal_msp.c)
 *   PN0 = DQS0, PN1 = NCS1, PN2-5 = IO0-3, PN6 = CLK, PN8-11 = IO4-7
 */

#define GPIO_XSPI2_DQS0 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(0))
#define GPIO_XSPI2_NCS1 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(1))
#define GPIO_XSPI2_IO0 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(2))
#define GPIO_XSPI2_IO1 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(3))
#define GPIO_XSPI2_IO2 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(4))
#define GPIO_XSPI2_IO3 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(5))
#define GPIO_XSPI2_CLK \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(6))
#define GPIO_XSPI2_IO4 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(8))
#define GPIO_XSPI2_IO5 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(9))
#define GPIO_XSPI2_IO6 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(10))
#define GPIO_XSPI2_IO7 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(9) | GPIO_PORTN | GPIO_PIN(11))

/* LTDC pins — ALIENTEK ATK-DNN647 RGB-LCD (15_RGBLCD, AF14).
 * 16-bit parallel RGB565 interface (R3-7, G2-7, B3-7) + CLK/HSYNC/VSYNC/DE.
 * Backlight BL on PA3 (active high).
 */

/* SAI1 pins - ALIENTEK ATK-DNN647 audio codec (44_Music_Player, AF6).
 * The codec's control bus is bit-banged I2C on PE13/PE14 and is set up by the
 * board, not here; these are the four audio signals.
 */

#define GPIO_SAI1_FS_A \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(6) | GPIO_PORTB | GPIO_PIN(0))

#define GPIO_SAI1_SD_A \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(6) | GPIO_PORTB | GPIO_PIN(2))

#define GPIO_SAI1_SCK_A \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(6) | GPIO_PORTC | GPIO_PIN(2))

#define GPIO_SAI1_MCLK_A \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(6) | GPIO_PORTE | GPIO_PIN(2))

#define GPIO_LTDC_R3 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTB | GPIO_PIN(4))
#define GPIO_LTDC_R4 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTH | GPIO_PIN(4))
#define GPIO_LTDC_R5 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTA | GPIO_PIN(15))
#define GPIO_LTDC_R6 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTF | GPIO_PIN(8))
#define GPIO_LTDC_R7 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTG | GPIO_PIN(9))

#define GPIO_LTDC_G2 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTA | GPIO_PIN(1))
#define GPIO_LTDC_G3 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTA | GPIO_PIN(0))
#define GPIO_LTDC_G4 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTB | GPIO_PIN(15))
#define GPIO_LTDC_G5 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTB | GPIO_PIN(12))
#define GPIO_LTDC_G6 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTB | GPIO_PIN(11))
#define GPIO_LTDC_G7 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTB | GPIO_PIN(10))

#define GPIO_LTDC_B3 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTA | GPIO_PIN(11))
#define GPIO_LTDC_B4 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTA | GPIO_PIN(10))
#define GPIO_LTDC_B5 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTA | GPIO_PIN(9))
#define GPIO_LTDC_B6 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTA | GPIO_PIN(8))
#define GPIO_LTDC_B7 \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTA | GPIO_PIN(2))

#define GPIO_LTDC_CLK \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTA | GPIO_PIN(5))
#define GPIO_LTDC_HSYNC \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTF | GPIO_PIN(9))
#define GPIO_LTDC_VSYNC \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTG | GPIO_PIN(0))
#define GPIO_LTDC_DE \
  (GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
   GPIO_PUPD_NONE | GPIO_AF(14) | GPIO_PORTG | GPIO_PIN(13))

/* LTDC backlight (PA3, active high, push-pull output) */

#define GPIO_LTDC_BL \
  (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | GPIO_SPEED_LOW | \
   GPIO_PUPD_NONE | GPIO_OUTPUT_CLEAR | GPIO_PORTA | GPIO_PIN(3))

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int stm32n6_configgpio(uint32_t cfgset);
int stm32n6_unconfiggpio(uint32_t cfgset);
bool stm32n6_gpioread(uint32_t pinset);
void stm32n6_gpiowrite(uint32_t pinset, bool value);
int stm32n6_gpiosetevent(uint32_t pinset, bool risingedge, bool fallingedge,
                         bool either, xcpt_t func, void *arg);

#ifdef CONFIG_DEV_GPIO

/****************************************************************************
 * Name: stm32n6_gpio_lower_initialize
 *
 * Description:
 *   Configure one physical pin for the given initial pintype and register
 *   it with the GPIO character-device upper half as /dev/gpioN.  pinset
 *   need only encode the port and pin; the mode bits are supplied by the
 *   lower half from the pintype.
 *
 ****************************************************************************/

int stm32n6_gpio_lower_initialize(uint32_t pinset, int minor,
                                  enum gpio_pintype_e pintype);
#endif

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_GPIO_H */
