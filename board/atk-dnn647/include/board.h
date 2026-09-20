/****************************************************************************
 * vendor/openvela/boards/atk-dnn647/include/board.h
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

#ifndef __BOARDS_ATK_DNN647_INCLUDE_BOARD_H
#define __BOARDS_ATK_DNN647_INCLUDE_BOARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#ifndef __ASSEMBLY__
#  include <stdint.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_ARCH_CHIP_STM32N6

/* Clocking *****************************************************************/

#define STM32_HSI_FREQUENCY     64000000ul

#ifdef CONFIG_EDGESIGHT_CLOCK_800MHZ

/* Full-speed clock tree for EdgeSight (requires SMPS overdrive):
 *
 *   PLL1: HSI(64MHz) / M=2 * N=25 = 800 MHz
 *     IC1  /1 = 800 MHz  -> CPU clock
 *     IC2  /2 = 400 MHz  -> AXI bus
 *     IC6  /2 = 400 MHz  -> NPU (or PLL2/1=1000MHz for full NPU speed)
 *     IC11 /2 = 400 MHz  -> AXISRAM3/4/5/6
 *   AHB prescaler /2 = 200 MHz -> HCLK
 *   APB1..5 /1 = 200 MHz -> PCLKx
 *
 *   PLL2: HSI(64MHz) / M=8 * N=125 = 1000 MHz -> NPU via IC6
 *   PLL4: HSI(64MHz) / M=32 * N=40 = 80 MHz   -> peripheral clocks
 *
 *   SDMMC: IC4 = PLL1/4 = 200 MHz
 *   DCMIPP: IC17 = PLL2/3 = 333 MHz
 *   XSPI1/2: HCLK = 200 MHz
 */

#define STM32_PLL1_M            2
#define STM32_PLL1_N            25
#define STM32_PLL1_P1           1
#define STM32_PLL1_P2           1
#define STM32_PLL1_IC1_DIV      1
#define STM32_PLL1_IC2_DIV      2
#define STM32_PLL1_IC4_DIV      4

#define STM32_PLL2_M            8
#define STM32_PLL2_N            125
#define STM32_PLL2_P1           1
#define STM32_PLL2_P2           1

#define STM32_PLL4_M            32
#define STM32_PLL4_N            40
#define STM32_PLL4_P1           1
#define STM32_PLL4_P2           1

#define STM32_CPUCLK_FREQUENCY  800000000ul
#define STM32_AXI_FREQUENCY     400000000ul
#define STM32_NPU_FREQUENCY     1000000000ul
#define STM32_HCLK_FREQUENCY    200000000ul
#define STM32_SYSCLK_FREQUENCY  400000000ul
#define STM32_PCLK1_FREQUENCY   200000000ul
#define STM32_PCLK2_FREQUENCY   200000000ul
#define STM32_PCLK4_FREQUENCY   200000000ul
#define STM32_PCLK5_FREQUENCY   200000000ul

#define STM32_SDMMC_FREQUENCY   200000000ul
#define STM32_XSPI_FREQUENCY    200000000ul
#define STM32_DCMIPP_FREQUENCY  333333333ul

#else /* Conservative 200 MHz boot (default, no SMPS overdrive needed) */

/* Clock tree (PLL1 fed from internal HSI):
 *
 *   HSI 64 MHz / M=4 * N=50 = 800 MHz VCO
 *     IC1  /4 = 200 MHz  -> CPU clock (CPUSW)
 *     IC2  /8 = 100 MHz  \
 *     IC6 /12 = 66.7 MHz  > SYSCLK components (SYSSW IC2_IC6_IC11)
 *     IC11 /8 = 100 MHz  /
 *   HPRE /2  = 50 MHz   -> HCLK
 *   PPRE1 /1 = 50 MHz   -> PCLK1
 *   PPRE2 /1 = 50 MHz   -> PCLK2
 */

#define STM32_PLL1_M            4
#define STM32_PLL1_N            50
#define STM32_PLL1_IC1_DIV      4

#define STM32_CPUCLK_FREQUENCY  200000000ul
#define STM32_SYSCLK_FREQUENCY  (STM32_CPUCLK_FREQUENCY / 2)
#define STM32_HCLK_FREQUENCY    (STM32_CPUCLK_FREQUENCY / 4)
#define STM32_PCLK1_FREQUENCY   STM32_HCLK_FREQUENCY
#define STM32_PCLK2_FREQUENCY   STM32_HCLK_FREQUENCY

#endif /* CONFIG_EDGESIGHT_CLOCK_800MHZ */

/* The frequency values above describe the clock tree PLL1 *would*
 * produce.  stm32n6_clockconfig() only programs and switches to PLL1
 * when CONFIG_STM32N6_USE_PLL1 is set; otherwise it leaves the clock
 * tree exactly as the chain-loading FSBL left it.  On the
 * atk-dnn647 board (ATK-DNN647, flash boot via custom FSBL) the
 * FSBL programs HSI->PLL1 (M=4, N=75) = 1200 MHz VCO and switches:
 *
 *   IC1 = PLL1 /2 = 600 MHz   -> CPUCLK (drives SysTick, CLKSOURCE=1)
 *   IC2 = PLL1 /3 = 400 MHz   -> SYSCLK
 *   HPRE = /2                  -> HCLK   = 200 MHz
 *   APB1/2/4/5 = /1            -> PCLKx  = 200 MHz
 *   timer kernel clock = sys_bus_ck = SYSCLK = 400 MHz (TIMPRE=00)
 *
 * This matches the stock ALIENTEK FSBL (luoji/ FSBL: PLL1 M=4,N=75,
 * IC1/2, IC2/3, AHBCLK /2) and the bare-metal Appli, which does NOT
 * call SystemClock_Config() and runs on the FSBL's 600 MHz tree, so
 * the console works at 115200 there.  These constants MUST match that
 * hand-off clock tree or every timebase (SysTick, timers, delays,
 * baud divisors) is off by ~9x and the OS dies shortly after boot.
 * The USART1 kernel clock is routed to HSI (64 MHz) by __start_c via
 * RCC_CCIPR13 so the console baud rate stays correct regardless of
 * PLL1.
 */

#ifndef CONFIG_STM32N6_USE_PLL1
#  undef STM32_CPUCLK_FREQUENCY
#  undef STM32_SYSCLK_FREQUENCY
#  undef STM32_HCLK_FREQUENCY
#  undef STM32_PCLK1_FREQUENCY
#  undef STM32_PCLK2_FREQUENCY

#  define STM32_CPUCLK_FREQUENCY  600000000ul
#  define STM32_SYSCLK_FREQUENCY  400000000ul
#  define STM32_HCLK_FREQUENCY    200000000ul
#  define STM32_PCLK1_FREQUENCY   200000000ul
#  define STM32_PCLK2_FREQUENCY   200000000ul
#endif

/* APB timer kernel clock.  On STM32N6 the timer-group clock timg_ck does
 * NOT derive from PCLKx: per RM0486 RCC_CFGR2.TIMPRE, at the reset default
 * TIMPRE=00 timg_ck = sys_bus_ck, i.e. the AXI system bus clock taken
 * ahead of the HPRE prescaler.  Since the FSBL leaves HPRE=/2, PCLKx is
 * sys_bus_ck/2 but the timers still run at the full sys_bus_ck (= SYSCLK).
 * Using PCLKx here made every timer tick at 2x the intended rate (verified
 * live: a nominal 1 kHz PWM read back as ~2 kHz through the TIM3->TIM15
 * inter-timer link).  So the timer input clock equals SYSCLK, not PCLKx.
 */

#define STM32_APB1_TIM_FREQUENCY STM32_SYSCLK_FREQUENCY
#define STM32_APB2_TIM_FREQUENCY STM32_SYSCLK_FREQUENCY

/* I/O voltage domains ******************************************************/

#define BOARD_PWR_VDDIO  (PWR_SVMCR3_VDDIO2SV    | PWR_SVMCR3_VDDIO3SV | \
                          PWR_SVMCR3_VDDIO2VRSEL | PWR_SVMCR3_VDDIO3VRSEL)

/* Alternate function pin selections ****************************************/

/* USART1: PE5=TX (AF7), PE6=RX (AF7) — Virtual COM Port via ST-Link */

#define GPIO_USART1_TX   GPIO_USART1_TX_1
#define GPIO_USART1_RX   GPIO_USART1_RX_1

/* User LEDs — ALIENTEK ATK-DNN647
 *
 *   LED0 = PG10, LED1 = PE10   (SoftwarePackage Drivers/BSP/LED/led.h)
 *   Both LEDs are active-low: driving the pin low turns the LED on.
 *   GPIO config matches 01_LED.ioc (push-pull output, pull-down, low
 *   speed, initial state high = LED off).
 */

#define GPIO_LED0 \
  (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | GPIO_SPEED_LOW | \
   GPIO_PUPD_PD | GPIO_OUTPUT_SET | GPIO_PORTG | GPIO_PIN(10))
#define GPIO_LED1 \
  (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | GPIO_SPEED_LOW | \
   GPIO_PUPD_PD | GPIO_OUTPUT_SET | GPIO_PORTE | GPIO_PIN(10))

#define BOARD_NLEDS       2

/* User buttons — ALIENTEK ATK-DNN647
 *
 *   KEY0 = PC6,  KEY1 = PD1,  KEY2 = PG11,  WKUP = PC13
 *   (SoftwarePackage Drivers/BSP/KEY/key.h)
 *   KEY0/1/2: pull-up input, pressed = low
 *   WKUP:     pull-down input, pressed = high
 */

#define BUTTON_KEY0     0
#define BUTTON_KEY1     1
#define BUTTON_KEY2     2
#define BUTTON_WKUP     3
#define NUM_BUTTONS     4

#define GPIO_BTN_KEY0 \
  (GPIO_MODE_INPUT | GPIO_PUPD_PU | GPIO_PORTC | GPIO_PIN(6))
#define GPIO_BTN_KEY1 \
  (GPIO_MODE_INPUT | GPIO_PUPD_PU | GPIO_PORTD | GPIO_PIN(1))
#define GPIO_BTN_KEY2 \
  (GPIO_MODE_INPUT | GPIO_PUPD_PU | GPIO_PORTG | GPIO_PIN(11))
#define GPIO_BTN_WKUP \
  (GPIO_MODE_INPUT | GPIO_PUPD_PD | GPIO_PORTC | GPIO_PIN(13))

/* Display Framebuffer Addresses
 * Background (camera): 800x480x2 = 768KB
 * Foreground (overlay): 800x480x2 = 768KB (x2 for double-buffering)
 * Total: ~2.3MB, placed in AXISRAM1 upper region
 */

#define BOARD_LCD_WIDTH      800
#define BOARD_LCD_HEIGHT     480

#define BOARD_LCD_BG_ADDR    0x34100000
#define BOARD_LCD_FG_ADDR0   0x341C0000
#define BOARD_LCD_FG_ADDR1   0x34280000

#else /* !CONFIG_ARCH_CHIP_STM32N6 — QEMU/MPS3 build */

/* MPS3-AN547 SysTick clock for QEMU emulation (25 MHz REFCLK) */

#define MPS_SYSTICK_CLOCK   25000000ul

#endif /* CONFIG_ARCH_CHIP_STM32N6 */

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_ARCH_CHIP_STM32N6
void stm32_board_initialize(void);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __BOARDS_ATK_DNN647_INCLUDE_BOARD_H */
