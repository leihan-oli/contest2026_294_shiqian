/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_lowputc.c
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

#include "arm_internal.h"
#include "stm32n6_lowputc.h"
#include "stm32n6_gpio.h"
#include "hardware/stm32_memorymap.h"
#include "hardware/stm32_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* USART register offsets */

#define USART_CR1_OFFSET     0x00
#define USART_CR2_OFFSET     0x04
#define USART_CR3_OFFSET     0x08
#define USART_BRR_OFFSET     0x0c
#define USART_ISR_OFFSET     0x1c
#define USART_TDR_OFFSET     0x28

/* USART_CR1 bits */

#define USART_CR1_UE         (1 << 0)
#define USART_CR1_RE         (1 << 2)
#define USART_CR1_TE         (1 << 3)
#define USART_CR1_FIFOEN     (1 << 29)
#define USART_CR1_RE         (1 << 2)
#define USART_CR1_TE         (1 << 3)
#define USART_CR1_PS         (1 << 9)
#define USART_CR1_PCE        (1 << 10)
#define USART_CR1_M0         (1 << 12)
#define USART_CR1_OVER8      (1 << 15)
#define USART_CR1_M1         (1 << 28)
#define USART_CR1_FIFOEN     (1 << 29)

/* USART_CR2 bits */

#define USART_CR2_STOP_SHIFT (12)
#define USART_CR2_STOP2      (2 << USART_CR2_STOP_SHIFT)

/* USART_ISR bits */

#define USART_ISR_TXE        (1 << 7)

/* Console UART selection.  Only USART1 is supported in this port
 * (this matches this repo's board.h pin assignment: PE5/PE6=AF7).
 */

#ifdef CONFIG_USART1_SERIAL_CONSOLE
#  define CONSOLE_BASE       STM32_USART1_BASE
#  define CONSOLE_APBEN      RCC_APB2ENR_USART1EN
#  define CONSOLE_APBENSR    STM32_RCC_APB2ENSR
#  define CONSOLE_BAUD       CONFIG_USART1_BAUD
#  define CONSOLE_BITS       CONFIG_USART1_BITS
#  define CONSOLE_PARITY     CONFIG_USART1_PARITY
#  define CONSOLE_2STOP      CONFIG_USART1_2STOP
#endif

/* HSI clock = 64 MHz.  USART1 is clocked from APB2 (also HSI-derived
 * at this stage of clock configuration; see stm32n6_rcc.c).
 */

#define STM32_HSI_FREQUENCY  64000000
#define CONSOLE_CLOCK        STM32_HSI_FREQUENCY

#ifdef CONSOLE_BASE

/* CR1 data-bit-count field (M1:M0), ported from apache/nuttx
 * upstream stm32_lowputc.c: STM32 USART M1:M0 = 00 selects 8 data
 * bits, 01 selects 9, 10 selects 7.
 */

#  if CONSOLE_BITS == 9
#    define USART_CR1_M0_VALUE USART_CR1_M0
#    define USART_CR1_M1_VALUE 0
#  elif CONSOLE_BITS == 7
#    define USART_CR1_M0_VALUE 0
#    define USART_CR1_M1_VALUE USART_CR1_M1
#  else /* 8 bits (default) */
#    define USART_CR1_M0_VALUE 0
#    define USART_CR1_M1_VALUE 0
#  endif

#  if CONSOLE_PARITY == 1 /* odd parity */
#    define USART_CR1_PARITY_VALUE (USART_CR1_PCE | USART_CR1_PS)
#  elif CONSOLE_PARITY == 2 /* even parity */
#    define USART_CR1_PARITY_VALUE USART_CR1_PCE
#  else /* no parity (default) */
#    define USART_CR1_PARITY_VALUE 0
#  endif

#  define USART_CR1_SETBITS \
    (USART_CR1_M0_VALUE | USART_CR1_M1_VALUE | USART_CR1_PARITY_VALUE)

#  if CONSOLE_2STOP != 0
#    define USART_CR2_SETBITS USART_CR2_STOP2
#  else
#    define USART_CR2_SETBITS 0
#  endif

/* Calculate the USART BAUD rate divider.
 *
 * Oversampling by 16 (the default):
 *   UARTDIV = fCK / baud
 *
 * Oversampling by 8 (used automatically when the by-16 divisor
 * would be too coarse for the requested baud rate):
 *   UARTDIV = 2 * fCK / baud, folded into BRR's compressed
 *   4-bit fraction format (DIV_FRACTION[0] dropped).
 *
 * Ported from apache/nuttx upstream stm32_lowputc.c.  The switch
 * threshold (2000) matches upstream's heuristic: below it, losing
 * the low fraction bit when packing the OVER8 BRR value costs at
 * most a small fraction of a bit period; above it (i.e. slow baud
 * rates relative to the input clock) plain 16x oversampling is
 * already precise enough and OVER8 gives no benefit.
 */

#  define STM32N6_USARTDIV8 \
      (((CONSOLE_CLOCK << 1) + (CONSOLE_BAUD >> 1)) / CONSOLE_BAUD)
#  define STM32N6_USARTDIV16 \
      ((CONSOLE_CLOCK + (CONSOLE_BAUD >> 1)) / CONSOLE_BAUD)

#  if STM32N6_USARTDIV8 > 2000
#    define STM32N6_BRR_VALUE STM32N6_USARTDIV16
#    undef USE_OVER8
#  else
#    define USE_OVER8 1
#    define STM32N6_BRR_VALUE \
      ((STM32N6_USARTDIV8 & 0xfff0) | ((STM32N6_USARTDIV8 & 0x000f) >> 1))
#  endif

#endif /* CONSOLE_BASE */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_lowsetup
 *
 * Description:
 *   Configure the console USART.  Called at the very beginning of
 *   __start, before .data/.bss are necessarily fully set up, so this
 *   function must not rely on any initialized global state.
 *
 ****************************************************************************/

void stm32n6_lowsetup(void)
{
#ifdef CONSOLE_BASE
  uint32_t cr;

  /* Enable the USART1 peripheral clock using the write-1-to-set
   * ENSR alias rather than a read-modify-write on ENR, so this
   * does not race any other producer of the same clock-enable
   * bitmap (ported from apache/nuttx upstream stm32_lowsetup(),
   * which uses the equivalent STM32N6_CONSOLE_APBREG/APBEN pair).
   */

  putreg32(CONSOLE_APBEN, CONSOLE_APBENSR);

  /* Configure USART1 pins: PE5=TX AF7, PE6=RX AF7 */

  stm32n6_configgpio(GPIO_USART1_TX);
  stm32n6_configgpio(GPIO_USART1_RX);

  /* Disable USART before configuring */

  putreg32(0, CONSOLE_BASE + USART_CR1_OFFSET);
  putreg32(0, CONSOLE_BASE + USART_CR3_OFFSET);

  /* CR2: stop bits */

  putreg32(USART_CR2_SETBITS, CONSOLE_BASE + USART_CR2_OFFSET);

  /* CR1: data bits, parity, RX FIFO (UE/TE/RE and OVER8 are applied
   * below, after BRR is programmed, matching upstream's ordering).
   * Enabling the RX FIFO lets the hardware buffer incoming bytes so
   * that a transient interrupt latency does not overflow (ORE) and
   * drop characters on interactive consoles.
   */

  putreg32(USART_CR1_SETBITS | USART_CR1_FIFOEN,
           CONSOLE_BASE + USART_CR1_OFFSET);

  /* Configure baud rate */

  putreg32(STM32N6_BRR_VALUE, CONSOLE_BASE + USART_BRR_OFFSET);

  cr = getreg32(CONSOLE_BASE + USART_CR1_OFFSET);
#  ifdef USE_OVER8
  cr |= USART_CR1_OVER8;
  putreg32(cr, CONSOLE_BASE + USART_CR1_OFFSET);
#  endif

  /* Enable USART: TX + RX + UE */

  cr |= (USART_CR1_UE | USART_CR1_TE | USART_CR1_RE);
  putreg32(cr, CONSOLE_BASE + USART_CR1_OFFSET);
#endif /* CONSOLE_BASE */
}

/****************************************************************************
 * Name: arm_lowputc
 *
 * Description:
 *   Output one byte on the serial console.
 *
 ****************************************************************************/

void arm_lowputc(char ch)
{
#ifdef CONSOLE_BASE
  while ((getreg32(CONSOLE_BASE + USART_ISR_OFFSET) &
          USART_ISR_TXE) == 0)
    {
    }

  putreg32((uint32_t)ch, CONSOLE_BASE + USART_TDR_OFFSET);
#endif
}
