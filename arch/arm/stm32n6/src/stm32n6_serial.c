/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_serial.c
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

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/serial/serial.h>

#include "arm_internal.h"
#include "chip.h"
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_USART1_SERIAL_CONSOLE
#  define HAVE_CONSOLE 1
#endif

/* USART register offsets */

#define USART_CR1_OFFSET     0x00
#define USART_CR3_OFFSET     0x08
#define USART_BRR_OFFSET     0x0c
#define USART_ISR_OFFSET     0x1c
#define USART_ICR_OFFSET     0x20
#define USART_RDR_OFFSET     0x24
#define USART_TDR_OFFSET     0x28

/* USART_CR1 bits */

#define USART_CR1_UE         (1 << 0)
#define USART_CR1_RE         (1 << 2)
#define USART_CR1_TE         (1 << 3)
#define USART_CR1_RXNEIE     (1 << 5)
#define USART_CR1_TXEIE      (1 << 7)

/* USART_ISR bits.  FE/NE/ORE are sticky error flags: they must be
 * cleared via ICR (or by reading RDR for the byte that caused them) or
 * they latch and garble/stall subsequent reception.  A USB-serial
 * (CH340) re-enumeration on the console drives the RX line with a burst
 * of break/framing garbage, so these bits are routinely hit in the
 * field when the console USB is unplugged/replugged.
 */

#define USART_ISR_FE         (1 << 1)  /* Frame error */
#define USART_ISR_NE         (1 << 2)  /* Noise error */
#define USART_ISR_ORE        (1 << 3)  /* Overrun error */
#define USART_ISR_RXNE       (1 << 5)
#define USART_ISR_TXE        (1 << 7)

/* USART_ICR bits (write-1-to-clear, same bit positions as ISR) */

#define USART_ICR_FECF       (1 << 1)
#define USART_ICR_NECF       (1 << 2)
#define USART_ICR_ORECF      (1 << 3)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_uart_s
{
  uintptr_t base;
  int       irq;
  uint32_t  baud;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int  stm32n6_setup(struct uart_dev_s *dev);
static void stm32n6_shutdown(struct uart_dev_s *dev);
static int  stm32n6_attach(struct uart_dev_s *dev);
static void stm32n6_detach(struct uart_dev_s *dev);
static int  stm32n6_interrupt(int irq, void *context,
                              void *arg);
static int  stm32n6_ioctl(struct file *filep, int cmd,
                           unsigned long arg);
static int  stm32n6_receive(struct uart_dev_s *dev,
                             unsigned int *status);
static void stm32n6_rxint(struct uart_dev_s *dev, bool enable);
static bool stm32n6_rxavailable(struct uart_dev_s *dev);
static void stm32n6_send(struct uart_dev_s *dev, int ch);
static void stm32n6_txint(struct uart_dev_s *dev, bool enable);
static bool stm32n6_txready(struct uart_dev_s *dev);
static bool stm32n6_txempty(struct uart_dev_s *dev);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct uart_ops_s g_uart_ops =
{
  .setup       = stm32n6_setup,
  .shutdown    = stm32n6_shutdown,
  .attach      = stm32n6_attach,
  .detach      = stm32n6_detach,
  .ioctl       = stm32n6_ioctl,
  .receive     = stm32n6_receive,
  .rxint       = stm32n6_rxint,
  .rxavailable = stm32n6_rxavailable,
  .send        = stm32n6_send,
  .txint       = stm32n6_txint,
  .txready     = stm32n6_txready,
  .txempty     = stm32n6_txempty,
};

#ifdef CONFIG_STM32_USART1

static char g_usart1rxbuffer[64];
static char g_usart1txbuffer[64];

static struct stm32n6_uart_s g_usart1priv =
{
  .base  = STM32_USART1_BASE,
  .irq   = STM32_IRQ_USART1,
  .baud  = CONFIG_USART1_BAUD,
};

static struct uart_dev_s g_usart1port =
{
  .recv  =
    {
      .size   = sizeof(g_usart1rxbuffer),
      .buffer = g_usart1rxbuffer,
    },
  .xmit  =
    {
      .size   = sizeof(g_usart1txbuffer),
      .buffer = g_usart1txbuffer,
    },
  .ops   = &g_uart_ops,
  .priv  = &g_usart1priv,
};

#endif

#ifdef CONFIG_STM32_USART2

static char g_usart2rxbuffer[64];
static char g_usart2txbuffer[64];

static struct stm32n6_uart_s g_usart2priv =
{
  .base  = STM32_USART2_BASE,
  .irq   = STM32_IRQ_USART2,
  .baud  = CONFIG_USART2_BAUD,
};

static struct uart_dev_s g_usart2port =
{
  .recv  =
    {
      .size   = sizeof(g_usart2rxbuffer),
      .buffer = g_usart2rxbuffer,
    },
  .xmit  =
    {
      .size   = sizeof(g_usart2txbuffer),
      .buffer = g_usart2txbuffer,
    },
  .ops   = &g_uart_ops,
  .priv  = &g_usart2priv,
};

#endif

#ifdef CONFIG_STM32_USART3

static char g_usart3rxbuffer[64];
static char g_usart3txbuffer[64];

static struct stm32n6_uart_s g_usart3priv =
{
  .base  = STM32_USART3_BASE,
  .irq   = STM32_IRQ_USART3,
  .baud  = CONFIG_USART3_BAUD,
};

static struct uart_dev_s g_usart3port =
{
  .recv  =
    {
      .size   = sizeof(g_usart3rxbuffer),
      .buffer = g_usart3rxbuffer,
    },
  .xmit  =
    {
      .size   = sizeof(g_usart3txbuffer),
      .buffer = g_usart3txbuffer,
    },
  .ops   = &g_uart_ops,
  .priv  = &g_usart3priv,
};

#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32n6_setup(struct uart_dev_s *dev)
{
  /* Already configured in lowsetup */

  return OK;
}

static void stm32n6_shutdown(struct uart_dev_s *dev)
{
  struct stm32n6_uart_s *priv = dev->priv;

  putreg32(0, priv->base + USART_CR1_OFFSET);
}

static int stm32n6_attach(struct uart_dev_s *dev)
{
  struct stm32n6_uart_s *priv = dev->priv;
  int ret;

  ret = irq_attach(priv->irq, stm32n6_interrupt, dev);
  if (ret == OK)
    {
      /* Give UART IRQs a higher priority than the SysTick default so
       * RXNE servicing is not delayed long enough to overflow (ORE),
       * which drops received characters on interactive consoles.
       */

      up_prioritize_irq(priv->irq,
                        NVIC_SYSH_PRIORITY_DEFAULT - NVIC_SYSH_PRIORITY_STEP);
      up_enable_irq(priv->irq);
    }

  return ret;
}

static void stm32n6_detach(struct uart_dev_s *dev)
{
  struct stm32n6_uart_s *priv = dev->priv;

  up_disable_irq(priv->irq);
  irq_detach(priv->irq);
}

static int stm32n6_interrupt(int irq, void *context, void *arg)
{
  struct uart_dev_s *dev = (struct uart_dev_s *)arg;
  struct stm32n6_uart_s *priv = dev->priv;
  uint32_t isr;

  isr = getreg32(priv->base + USART_ISR_OFFSET);

  /* A USB-serial (CH340) re-enumeration on the console can slam the RX
   * line with a burst of break/framing garbage (FE/NE) plus a full RDR
   * (ORE).  Clear ALL sticky error flags before servicing RXNE:
   * an un-cleared FE/NE latches the error and makes subsequent reception
   * unreliable, and an un-serviced ORE blocks new RXNE.  When RXNE is
   * set alongside a frame/noise error, the RDR content is invalid --
   * drain it (read RDR, which also clears RXNE) instead of feeding the
   * garbage into the RX buffer.
   */

  if (isr & (USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE))
    {
      uint32_t errs = isr & (USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE);

      if ((isr & USART_ISR_RXNE) && (isr & (USART_ISR_FE | USART_ISR_NE)))
        {
          /* Invalid byte from a framing/noise glitch: discard it. */

          (void)getreg32(priv->base + USART_RDR_OFFSET);
        }

      putreg32(errs, priv->base + USART_ICR_OFFSET);
    }

  if (isr & USART_ISR_RXNE)
    {
      uart_recvchars(dev);
    }

  if (isr & USART_ISR_TXE)
    {
      uart_xmitchars(dev);
    }

  return OK;
}

static int stm32n6_ioctl(struct file *filep, int cmd,
                          unsigned long arg)
{
  return -ENOTTY;
}

static int stm32n6_receive(struct uart_dev_s *dev,
                            unsigned int *status)
{
  struct stm32n6_uart_s *priv = dev->priv;

  *status = getreg32(priv->base + USART_ISR_OFFSET);
  return (int)getreg32(priv->base + USART_RDR_OFFSET);
}

static void stm32n6_rxint(struct uart_dev_s *dev, bool enable)
{
  struct stm32n6_uart_s *priv = dev->priv;
  uint32_t regval;

  regval = getreg32(priv->base + USART_CR1_OFFSET);

  if (enable)
    {
      regval |= USART_CR1_RXNEIE;
    }
  else
    {
      regval &= ~USART_CR1_RXNEIE;
    }

  putreg32(regval, priv->base + USART_CR1_OFFSET);
}

static bool stm32n6_rxavailable(struct uart_dev_s *dev)
{
  struct stm32n6_uart_s *priv = dev->priv;
  uint32_t isr = getreg32(priv->base + USART_ISR_OFFSET);

  /* A latched frame/noise error means the pending RXNE byte (if any) is
   * invalid.  Report "no data" in that case so uart_recvchars() does not
   * spin reading garbage from a sustained break on RX (e.g. while the
   * console USB-serial is re-enumerating).  The ISR clears the sticky
   * error flags and drains the invalid byte.
   */

  return (isr & USART_ISR_RXNE) != 0 &&
         (isr & (USART_ISR_FE | USART_ISR_NE)) == 0;
}

static void stm32n6_send(struct uart_dev_s *dev, int ch)
{
  struct stm32n6_uart_s *priv = dev->priv;

  putreg32((uint32_t)ch, priv->base + USART_TDR_OFFSET);
}

static void stm32n6_txint(struct uart_dev_s *dev, bool enable)
{
  struct stm32n6_uart_s *priv = dev->priv;
  uint32_t regval;

  regval = getreg32(priv->base + USART_CR1_OFFSET);

  if (enable)
    {
      regval |= USART_CR1_TXEIE;
      putreg32(regval, priv->base + USART_CR1_OFFSET);
      uart_xmitchars(dev);
    }
  else
    {
      regval &= ~USART_CR1_TXEIE;
      putreg32(regval, priv->base + USART_CR1_OFFSET);
    }
}

static bool stm32n6_txready(struct uart_dev_s *dev)
{
  struct stm32n6_uart_s *priv = dev->priv;

  return (getreg32(priv->base + USART_ISR_OFFSET) &
          USART_ISR_TXE) != 0;
}

static bool stm32n6_txempty(struct uart_dev_s *dev)
{
  return stm32n6_txready(dev);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm_earlyserialinit
 *
 * Description:
 *   Early serial initialization (called from __start).
 *
 ****************************************************************************/

#ifdef USE_EARLYSERIALINIT
void arm_earlyserialinit(void)
{
#ifdef CONFIG_STM32_USART1
#  ifdef CONFIG_USART1_SERIAL_CONSOLE
  g_usart1port.isconsole = true;
#  endif
#endif
#ifdef CONFIG_STM32_USART2
#  ifdef CONFIG_USART2_SERIAL_CONSOLE
  g_usart2port.isconsole = true;
#  endif
#endif
#ifdef CONFIG_STM32_USART3
#  ifdef CONFIG_USART3_SERIAL_CONSOLE
  g_usart3port.isconsole = true;
#  endif
#endif
}
#endif

/****************************************************************************
 * Name: arm_serialinit
 *
 * Description:
 *   Register serial console and serial ports.
 *
 ****************************************************************************/

void arm_serialinit(void)
{
#ifdef CONFIG_STM32_USART1
#  ifdef CONFIG_USART1_SERIAL_CONSOLE
  uart_register("/dev/console", &g_usart1port);
#  endif
  uart_register("/dev/ttyS0", &g_usart1port);
#endif
#ifdef CONFIG_STM32_USART2
#  ifdef CONFIG_USART2_SERIAL_CONSOLE
  uart_register("/dev/console", &g_usart2port);
#  endif
  uart_register("/dev/ttyS1", &g_usart2port);
#endif
#ifdef CONFIG_STM32_USART3
#  ifdef CONFIG_USART3_SERIAL_CONSOLE
  uart_register("/dev/console", &g_usart3port);
#  endif
  uart_register("/dev/ttyS2", &g_usart3port);
#endif
}

/****************************************************************************
 * Name: up_putc
 *
 * Description:
 *   Output one character on the console.
 *
 ****************************************************************************/

static void console_dump_str(FAR const char *str)
{
  while (*str != '\0')
    {
      arm_lowputc(*str++);
    }
}

static void console_dump_hex(uint32_t value)
{
  int i;

  for (i = 28; i >= 0; i -= 4)
    {
      uint32_t nib = (value >> i) & 0xf;
      arm_lowputc((char)(nib < 10 ? '0' + nib : 'a' + nib - 10));
    }
}

/****************************************************************************
 * Name: stm32n6_console_dump
 *
 * Description:
 *   Print the console transmitter state through polled, unbuffered output.
 *
 *   The NPU watchdog calls this when the application stops printing: once the
 *   console stops draining, every path that goes through the driver blocks,
 *   so the state has to leave the chip through arm_lowputc() to be visible.
 *
 ****************************************************************************/

static uint32_t g_console_unstick_count;

/****************************************************************************
 * Name: stm32n6_console_unstick
 *
 * Description:
 *   Hand back a lost console TX wakeup.
 *
 *   uart_write() blocks on xmitsem while the TX ring is full and relies on
 *   uart_datasent() to wake it up when space appears.  That wakeup can be
 *   lost - the upper half resets the very same semaphore from uart_close()
 *   (uart_reset_sem()) - and a writer then waits forever with an empty ring
 *   and the TX interrupt disabled: the console is dead, and everything that
 *   prints through it blocks, while the rest of the system runs.
 *
 *   Detect exactly that state (ring empty *and* somebody waiting) and post the
 *   semaphore again; the writer re-checks the ring and carries on.
 *
 ****************************************************************************/

void stm32n6_console_unstick(void)
{
  FAR uart_dev_t *dev = &g_usart1port;
  int sval;

  if (dev->xmit.head != dev->xmit.tail)
    {
      return;
    }

  if (nxsem_get_value(&dev->xmitsem, &sval) != OK || sval >= 0)
    {
      return;
    }

  g_console_unstick_count++;
  nxsem_post(&dev->xmitsem);
}

uint32_t stm32n6_console_unstick_count(void)
{
  return g_console_unstick_count;
}

void stm32n6_console_dump(void)
{
  FAR uart_dev_t *dev = &g_usart1port;

  console_dump_str("\r\n[uart1] CR1=");
  console_dump_hex(getreg32(g_usart1priv.base + USART_CR1_OFFSET));
  console_dump_str(" ISR=");
  console_dump_hex(getreg32(g_usart1priv.base + USART_ISR_OFFSET));
  console_dump_str(" head=");
  console_dump_hex((uint32_t)dev->xmit.head);
  console_dump_str(" tail=");
  console_dump_hex((uint32_t)dev->xmit.tail);
  console_dump_str(" size=");
  console_dump_hex((uint32_t)dev->xmit.size);
  console_dump_str(" sem=");
  console_dump_hex((uint32_t)(uintptr_t)&dev->xmitsem);
  console_dump_str(" stuck=");
  console_dump_hex(g_console_unstick_count);
  console_dump_str("\r\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void up_putc(int ch)
{
#ifdef HAVE_CONSOLE
  if (ch == '\n')
    {
      arm_lowputc('\r');
    }

  arm_lowputc((char)ch);
#endif
}
