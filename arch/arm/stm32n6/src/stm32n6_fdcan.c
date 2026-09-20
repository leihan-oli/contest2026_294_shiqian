/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_fdcan.c
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
 * STM32N6 FDCAN driver for NuttX.
 * Supports FDCAN1-3 with CAN-FD protocol.
 *
 * Adapted from STM32H7 NuttX reference (stm32_fdcan_sock.c).
 * Reference: STM32N6 HAL (stm32n6xx_hal_fdcan.c)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/kmalloc.h>
#include <nuttx/semaphore.h>
#include <syslog.h>
#include <string.h>

#include "stm32n6_fdcan.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* FDCAN register base addresses */

#define STM32N6_FDCAN1_BASE  0x4000A000
#define STM32N6_FDCAN2_BASE  0x4000B000
#define STM32N6_FDCAN3_BASE  0x4000C000

/* FDCAN message RAM base */

#define FDCAN1_MSG_RAM_BASE  0x4000AC00
#define FDCAN2_MSG_RAM_BASE  0x4000BC00

/* FDCAN register offsets */

#define FDCAN_CCCR_OFFSET    0x018  /* CMSIS CCCR (CC Control) */
#define FDCAN_BTP_OFFSET     0x01C  /* CMSIS NBTP (Nominal Bit Timing) */
#define FDCAN_TDCR_OFFSET    0x048  /* CMSIS TDCR (Tx Delay Comp) */
#define FDCAN_IR_OFFSET      0x050  /* CMSIS IR (Interrupt) */
#define FDCAN_IE_OFFSET      0x054  /* CMSIS IE (Interrupt Enable) */
#define FDCAN_ILS_OFFSET     0x058  /* CMSIS ILS (Interrupt Line Select) */
#define FDCAN_ILE_OFFSET     0x05C  /* CMSIS ILE (Interrupt Line Enable) */
#define FDCAN_GFC_OFFSET     0x080  /* CMSIS GFC (Global Filter Config) */
#define FDCAN_TXBAR_OFFSET   0x0D0  /* CMSIS TXBAR (Tx Buffer Add Req) */
#define FDCAN_TXBCR_OFFSET   0x0D4  /* CMSIS TXBCR (Tx Buffer Cancel Req) */
#define FDCAN_TXBTO_OFFSET   0x0D8  /* CMSIS TXBTO (Tx Buffer Tx Occurred) */
#define FDCAN_RXF0S_OFFSET   0x0A4  /* CMSIS RXF0S (Rx FIFO0 Status) */
#define FDCAN_RXF0A_OFFSET   0x0A8  /* CMSIS RXF0A (Rx FIFO0 Ack) */

/* CCCR register bits */

#define FDCAN_CCCR_INIT       (1 << 0)
#define FDCAN_CCCR_CCE        (1 << 1)
#define FDCAN_CCCR_ASM        (1 << 2)
#define FDCAN_CCCR_CSA        (1 << 3)
#define FDCAN_CCCR_CSR        (1 << 4)
#define FDCAN_CCCR_MON        (1 << 5)
#define FDCAN_CCCR_DAR        (1 << 6)
#define FDCAN_CCCR_TEST       (1 << 7)
#define FDCAN_CCCR_FDOE       (1 << 8)
#define FDCAN_CCCR_BRSE       (1 << 9)
#define FDCAN_CCCR_TXP        (1 << 14)

/* IR register bits */

#define FDCAN_IR_RF0N         (1 << 0)  /* Rx FIFO 0 new message */
#define FDCAN_IR_RF0W         (1 << 1)  /* Rx FIFO 0 watermark */
#define FDCAN_IR_RF0F         (1 << 2)  /* Rx FIFO 0 full */
#define FDCAN_IR_RF0L         (1 << 3)  /* Rx FIFO 0 lost */
#define FDCAN_IR_TC           (1 << 4)  /* Transmission completed */
#define FDCAN_IR_TCF          (1 << 5)  /* Transmission cancellation */
#define FDCAN_IR_TFE          (1 << 6)  /* Tx FIFO empty */
#define FDCAN_IR_TEFL         (1 << 12) /* Tx event FIFO lost */
#define FDCAN_IR_BO           (1 << 25) /* Bus-off */
#define FDCAN_IR_EW           (1 << 26) /* Error warning */
#define FDCAN_IR_EP           (1 << 27) /* Error passive */

/* Bit timing presets (500 kbit/s @ 80 MHz FDCAN clock) */

#define FDCAN_BTP_500K       0x00030009  /* 500 kbit/s */
#define FDCAN_BTP_1M         0x00010009  /* 1 Mbit/s */

/* Standard CAN frame format */

#define FDCAN_STANDARD_ID    0
#define FDCAN_EXTENDED_ID    1

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_fdcan_priv_s
{
  uint32_t base;
  uint32_t msg_ram_base;
  sem_t    lock;
  sem_t    txwait;
  sem_t    rxwait;
  bool     initialized;
  uint32_t bitrate;
  uint32_t rx_count;
  uint32_t tx_count;
  uint32_t error_count;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32n6_fdcan_priv_s g_fdcan1_priv;
static struct stm32n6_fdcan_priv_s g_fdcan2_priv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline uint32_t fdcan_getreg(
    struct stm32n6_fdcan_priv_s *priv, uint32_t offset)
{
  return *(volatile uint32_t *)(priv->base + offset);
}

static inline void fdcan_putreg(
    struct stm32n6_fdcan_priv_s *priv, uint32_t offset,
    uint32_t value)
{
  *(volatile uint32_t *)(priv->base + offset) = value;
}

static int fdcan_enter_init(struct stm32n6_fdcan_priv_s *priv)
{
  uint32_t timeout = 100000;

  /* Request init mode */

  fdcan_putreg(priv, FDCAN_CCCR_OFFSET,
               fdcan_getreg(priv, FDCAN_CCCR_OFFSET) |
               FDCAN_CCCR_INIT);

  while (timeout-- > 0)
    {
      if (fdcan_getreg(priv, FDCAN_CCCR_OFFSET) &
          FDCAN_CCCR_INIT)
        {
          return 0;
        }
    }

  return -ETIMEDOUT;
}

static void fdcan_leave_init(struct stm32n6_fdcan_priv_s *priv)
{
  fdcan_putreg(priv, FDCAN_CCCR_OFFSET,
               fdcan_getreg(priv, FDCAN_CCCR_OFFSET) &
               ~(FDCAN_CCCR_INIT | FDCAN_CCCR_CCE));
}

static int fdcan_set_bitrate(struct stm32n6_fdcan_priv_s *priv,
                              uint32_t bitrate)
{
  uint32_t btp;

  switch (bitrate)
    {
      case 500000:
        btp = FDCAN_BTP_500K;
        break;
      case 1000000:
        btp = FDCAN_BTP_1M;
        break;
      default:
        syslog(LOG_ERR, "fdcan: unsupported bitrate %lu\n",
               (unsigned long)bitrate);
        return -EINVAL;
    }

  fdcan_putreg(priv, FDCAN_BTP_OFFSET, btp);
  priv->bitrate = bitrate;
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_fdcan_initialize(int bus_num, uint32_t bitrate)
{
  struct stm32n6_fdcan_priv_s *priv;
  int ret;

  switch (bus_num)
    {
      case 1:
        priv = &g_fdcan1_priv;
        priv->base = STM32N6_FDCAN1_BASE;
        priv->msg_ram_base = FDCAN1_MSG_RAM_BASE;
        break;
      case 2:
        priv = &g_fdcan2_priv;
        priv->base = STM32N6_FDCAN2_BASE;
        priv->msg_ram_base = FDCAN2_MSG_RAM_BASE;
        break;
      default:
        syslog(LOG_ERR, "fdcan: unsupported bus %d\n", bus_num);
        return -EINVAL;
    }

  if (priv->initialized)
    {
      return 0;
    }

  nxsem_init(&priv->lock, 0, 1);
  nxsem_init(&priv->txwait, 0, 0);
  nxsem_init(&priv->rxwait, 0, 0);

  /* Enter init mode */

  ret = fdcan_enter_init(priv);
  if (ret < 0)
    {
      syslog(LOG_ERR, "fdcan%d: init mode timeout\n", bus_num);
      return ret;
    }

  /* Enable configuration change */

  fdcan_putreg(priv, FDCAN_CCCR_OFFSET,
               fdcan_getreg(priv, FDCAN_CCCR_OFFSET) |
               FDCAN_CCCR_CCE);

  /* Set bitrate */

  ret = fdcan_set_bitrate(priv, bitrate);
  if (ret < 0)
    {
      return ret;
    }

  /* Enable CAN-FD mode */

  fdcan_putreg(priv, FDCAN_CCCR_OFFSET,
               fdcan_getreg(priv, FDCAN_CCCR_OFFSET) |
               FDCAN_CCCR_FDOE);

  /* Configure acceptance filter: accept all */

  fdcan_putreg(priv, FDCAN_GFC_OFFSET, 0);

  /* Leave init mode */

  fdcan_leave_init(priv);

  priv->initialized = true;

  syslog(LOG_INFO, "fdcan%d: initialized @ %lu bps\n",
         bus_num, (unsigned long)bitrate);
  return 0;
}

int stm32n6_fdcan_send(int bus_num, uint32_t id,
                        const uint8_t *data, uint8_t len)
{
  struct stm32n6_fdcan_priv_s *priv;
  uint32_t timeout = 100000;

  switch (bus_num)
    {
      case 1:
        priv = &g_fdcan1_priv;
        break;
      case 2:
        priv = &g_fdcan2_priv;
        break;
      default:
        return -EINVAL;
    }

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  /* Wait for Tx buffer available */

  while (timeout-- > 0)
    {
      if (fdcan_getreg(priv, FDCAN_TXBAR_OFFSET) & 1)
        {
          break;
        }
    }

  if (timeout == 0)
    {
      return -EBUSY;
    }

  /* Write Tx buffer element */

  uint32_t *tx_buf = (uint32_t *)priv->msg_ram_base;

  /* T0: ID + RTR + XTD */

  tx_buf[0] = (id << 18) | (FDCAN_STANDARD_ID << 30);

  /* T1: DLC + BRS + FDF */

  tx_buf[1] = (len & 0x0f) | (1 << 20) | (1 << 21);

  /* Data bytes */

  memcpy(&tx_buf[2], data, len);

  /* Add request */

  fdcan_putreg(priv, FDCAN_TXBAR_OFFSET, 1);

  priv->tx_count++;
  return 0;
}

int stm32n6_fdcan_receive(int bus_num, uint32_t *id,
                           uint8_t *data, uint8_t *len,
                           int timeout_ms)
{
  struct stm32n6_fdcan_priv_s *priv;

  switch (bus_num)
    {
      case 1:
        priv = &g_fdcan1_priv;
        break;
      case 2:
        priv = &g_fdcan2_priv;
        break;
      default:
        return -EINVAL;
    }

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  /* Check Rx FIFO 0 status */

  uint32_t rxf0s = fdcan_getreg(priv, FDCAN_RXF0S_OFFSET);
  uint32_t fill_level = rxf0s & 0x7f;

  if (fill_level == 0)
    {
      return -EAGAIN;
    }

  /* Read Rx FIFO 0 element */

  uint32_t *rx_buf = (uint32_t *)(priv->msg_ram_base + 0x100);

  /* R0: ID */

  *id = (rx_buf[0] >> 18) & 0x7ff;

  /* R1: DLC */

  *len = rx_buf[1] & 0x0f;

  /* Data */

  memcpy(data, &rx_buf[2], *len);

  /* Acknowledge read */

  fdcan_putreg(priv, FDCAN_RXF0A_OFFSET, 0);

  priv->rx_count++;
  return 0;
}

void stm32n6_fdcan_deinit(int bus_num)
{
  struct stm32n6_fdcan_priv_s *priv;

  switch (bus_num)
    {
      case 1:
        priv = &g_fdcan1_priv;
        break;
      case 2:
        priv = &g_fdcan2_priv;
        break;
      default:
        return;
    }

  if (!priv->initialized)
    {
      return;
    }

  /* Enter init mode to stop */

  fdcan_enter_init(priv);

  priv->initialized = false;
  nxsem_destroy(&priv->lock);
  nxsem_destroy(&priv->txwait);
  nxsem_destroy(&priv->rxwait);

  syslog(LOG_INFO, "fdcan%d: deinitialized\n", bus_num);
}
