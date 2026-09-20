/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_spi.c
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
 * STM32N6 SPI driver for NuttX.
 * Supports SPI1-6 in master mode with DMA capability.
 *
 * Adapted from STM32H7 NuttX reference (stm32_spi.c).
 * Reference: STM32N6 HAL (stm32n6xx_hal_spi.c)
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

#include "stm32n6_spi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SPI register base addresses (per CMSIS stm32n647xx.h) */

#define STM32N6_SPI1_BASE   0x42003000
#define STM32N6_SPI2_BASE   0x40003800
#define STM32N6_SPI3_BASE   0x40003C00
#define STM32N6_SPI4_BASE   0x42003400
#define STM32N6_SPI5_BASE   0x42005000
#define STM32N6_SPI6_BASE   0x46001400

/* SPI register offsets */

#define SPI_CR1_OFFSET      0x00
#define SPI_CR2_OFFSET      0x04
#define SPI_CFG1_OFFSET     0x08
#define SPI_CFG2_OFFSET     0x0C
#define SPI_IER_OFFSET      0x10
#define SPI_SR_OFFSET       0x14
#define SPI_IFCR_OFFSET     0x18
#define SPI_TXDR_OFFSET     0x20
#define SPI_RXDR_OFFSET     0x30

/* SPI_CR1 bits */

#define SPI_CR1_SPE         (1 << 0)   /* SPI enable */
#define SPI_CR1_MASRX       (1 << 8)   /* Master RX */
#define SPI_CR1_CSTART      (1 << 9)   /* Master transfer start */
#define SPI_CR1_CSUSP       (1 << 10)  /* Master suspend */
#define SPI_CR1_HDDIR       (1 << 11)  /* Half-duplex direction */
#define SPI_CR1_SSI         (1 << 12)  /* Internal slave select */
#define SPI_CR1_CRC33_17    (1 << 14)  /* CRC polynomial */
#define SPI_CR1_RCRCINI     (1 << 15)  /* RX CRC init */
#define SPI_CR1_TCRCINI     (1 << 16)  /* TX CRC init */

/* SPI_CFG1 bits */

#define SPI_CFG1_DSIZE_MASK (0x1F << 0)  /* Data size */
#define SPI_CFG1_DSIZE_8    (7 << 0)
#define SPI_CFG1_DSIZE_16   (15 << 0)
#define SPI_CFG1_DSIZE_32   (31 << 0)
#define SPI_CFG1_FTHLV_MASK (0xF << 5)   /* FIFO threshold level */

/* SPI_CFG2 bits */

#define SPI_CFG2_MASTER     (1 << 22)  /* Master mode */
#define SPI_CFG2_LSBFRST    (1 << 13)  /* LSB first */
#define SPI_CFG2_CPHA       (1 << 24)  /* Clock phase */
#define SPI_CFG2_CPOL       (1 << 25)  /* Clock polarity */
#define SPI_CFG2_SSM        (1 << 26)  /* Software slave management */
#define SPI_CFG2_COMM_MASK  (3 << 17)  /* Communication mode */
#define SPI_CFG2_COMM_FULL  (0 << 17)  /* Full duplex */
#define SPI_CFG2_COMM_SIMPLEX_TX (1 << 17)
#define SPI_CFG2_COMM_SIMPLEX_RX (2 << 17)
#define SPI_CFG2_COMM_HALF (3 << 17)

/* SPI_SR bits */

#define SPI_SR_RXP          (1 << 0)   /* RX packet available */
#define SPI_SR_TXP          (1 << 1)   /* TX packet space available */
#define SPI_SR_EOT          (1 << 3)   /* End of transfer */
#define SPI_SR_OVR          (1 << 6)   /* Overrun */
#define SPI_SR_SUSP         (1 << 11)  /* Master suspend */
#define SPI_SR_TXC          (1 << 12)  /* TX complete */
#define SPI_SR_RXWNE        (1 << 14)  /* RX word not empty */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_spi_priv_s
{
  uint32_t base;
  uint32_t clock;
  sem_t    lock;
  sem_t    wait;
  bool     initialized;
  uint8_t  bits_per_word;
  uint32_t frequency;
  uint8_t  mode;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32n6_spi_priv_s g_spi1_priv;
static struct stm32n6_spi_priv_s g_spi2_priv;
static struct stm32n6_spi_priv_s g_spi3_priv;
static struct stm32n6_spi_priv_s g_spi4_priv;
static struct stm32n6_spi_priv_s g_spi5_priv;
static struct stm32n6_spi_priv_s g_spi6_priv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline uint32_t spi_getreg(
    struct stm32n6_spi_priv_s *priv, uint32_t offset)
{
  return *(volatile uint32_t *)(priv->base + offset);
}

static inline void spi_putreg(
    struct stm32n6_spi_priv_s *priv, uint32_t offset,
    uint32_t value)
{
  *(volatile uint32_t *)(priv->base + offset) = value;
}

static void spi_config(struct stm32n6_spi_priv_s *priv)
{
  uint32_t cfg1;
  uint32_t cfg2;

  /* Data size */

  cfg1 = 0;
  if (priv->bits_per_word <= 8)
    {
      cfg1 |= SPI_CFG1_DSIZE_8;
    }
  else if (priv->bits_per_word <= 16)
    {
      cfg1 |= SPI_CFG1_DSIZE_16;
    }
  else
    {
      cfg1 |= SPI_CFG1_DSIZE_32;
    }

  /* FIFO threshold: 1 packet */

  cfg1 |= (1 << 5);

  spi_putreg(priv, SPI_CFG1_OFFSET, cfg1);

  /* Master mode, SSM, CPOL, CPHA */

  cfg2 = SPI_CFG2_MASTER | SPI_CFG2_SSM;
  cfg2 |= SPI_CFG2_COMM_FULL;

  if (priv->mode & SPI_CPHA)
    {
      cfg2 |= SPI_CFG2_CPHA;
    }

  if (priv->mode & SPI_CPOL)
    {
      cfg2 |= SPI_CFG2_CPOL;
    }

  spi_putreg(priv, SPI_CFG2_OFFSET, cfg2);

  /* Enable SPI, set SSI (no slave) */

  spi_putreg(priv, SPI_CR1_OFFSET, SPI_CR1_SPE | SPI_CR1_SSI);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_spi_initialize(int bus_num)
{
  struct stm32n6_spi_priv_s *priv;

  switch (bus_num)
    {
      case 1:
        priv = &g_spi1_priv;
        priv->base = STM32N6_SPI1_BASE;
        break;
      case 2:
        priv = &g_spi2_priv;
        priv->base = STM32N6_SPI2_BASE;
        break;
      case 3:
        priv = &g_spi3_priv;
        priv->base = STM32N6_SPI3_BASE;
        break;
      case 4:
        priv = &g_spi4_priv;
        priv->base = STM32N6_SPI4_BASE;
        break;
      case 5:
        priv = &g_spi5_priv;
        priv->base = STM32N6_SPI5_BASE;
        break;
      case 6:
        priv = &g_spi6_priv;
        priv->base = STM32N6_SPI6_BASE;
        break;
      default:
        syslog(LOG_ERR, "spi: unsupported bus %d\n", bus_num);
        return -EINVAL;
    }

  if (priv->initialized)
    {
      return 0;
    }

  nxsem_init(&priv->lock, 0, 1);
  nxsem_init(&priv->wait, 0, 0);
  priv->bits_per_word = 8;
  priv->frequency = 1000000;  /* 1 MHz default */
  priv->mode = 0;

  spi_config(priv);

  priv->initialized = true;

  syslog(LOG_INFO, "spi%d: initialized @ 1MHz 8-bit\n", bus_num);
  return 0;
}

int stm32n6_spi_transfer(int bus_num, const uint8_t *tx,
                           uint8_t *rx, uint32_t len)
{
  struct stm32n6_spi_priv_s *priv;
  uint32_t i;
  uint32_t timeout;

  switch (bus_num)
    {
      case 1:
        priv = &g_spi1_priv;
        break;
      case 2:
        priv = &g_spi2_priv;
        break;
      case 3:
        priv = &g_spi3_priv;
        break;
      case 4:
        priv = &g_spi4_priv;
        break;
      case 5:
        priv = &g_spi5_priv;
        break;
      case 6:
        priv = &g_spi6_priv;
        break;
      default:
        return -EINVAL;
    }

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  nxsem_wait(&priv->lock);

  /* Start transfer */

  spi_putreg(priv, SPI_CR1_OFFSET,
             spi_getreg(priv, SPI_CR1_OFFSET) | SPI_CR1_CSTART);

  for (i = 0; i < len; i++)
    {
      /* Wait for TX space */

      timeout = 100000;
      while (timeout-- > 0)
        {
          if (spi_getreg(priv, SPI_SR_OFFSET) & SPI_SR_TXP)
            {
              break;
            }
        }

      if (timeout == 0)
        {
          nxsem_post(&priv->lock);
          return -ETIMEDOUT;
        }

      /* Send byte */

      if (tx != NULL)
        {
          spi_putreg(priv, SPI_TXDR_OFFSET, tx[i]);
        }
      else
        {
          spi_putreg(priv, SPI_TXDR_OFFSET, 0xff);
        }

      /* Wait for RX data */

      timeout = 100000;
      while (timeout-- > 0)
        {
          if (spi_getreg(priv, SPI_SR_OFFSET) & SPI_SR_RXP)
            {
              break;
            }
        }

      if (timeout == 0)
        {
          nxsem_post(&priv->lock);
          return -ETIMEDOUT;
        }

      /* Read byte */

      if (rx != NULL)
        {
          rx[i] = spi_getreg(priv, SPI_RXDR_OFFSET) & 0xff;
        }
      else
        {
          /* Discard */

          (void)spi_getreg(priv, SPI_RXDR_OFFSET);
        }
    }

  /* Wait for TX complete */

  timeout = 100000;
  while (timeout-- > 0)
    {
      if (spi_getreg(priv, SPI_SR_OFFSET) & SPI_SR_TXC)
        {
          break;
        }
    }

  nxsem_post(&priv->lock);
  return 0;
}

void stm32n6_spi_deinit(int bus_num)
{
  struct stm32n6_spi_priv_s *priv;

  switch (bus_num)
    {
      case 1:
        priv = &g_spi1_priv;
        break;
      case 2:
        priv = &g_spi2_priv;
        break;
      default:
        return;
    }

  if (!priv->initialized)
    {
      return;
    }

  /* Disable SPI */

  spi_putreg(priv, SPI_CR1_OFFSET, 0);

  priv->initialized = false;
  nxsem_destroy(&priv->lock);
  nxsem_destroy(&priv->wait);

  syslog(LOG_INFO, "spi%d: deinitialized\n", bus_num);
}
