/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_ethernet.c
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
 * STM32N6 Ethernet GMAC driver for NuttX.
 * Supports 10/100Mbps via RMII interface with LAN8742 PHY.
 *
 * Adapted from STM32H7 NuttX reference (stm32_ethernet.c).
 * Reference: STM32N6 HAL (stm32n6xx_hal_eth.c)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/net/mii.h>
#include <nuttx/net/ethernet.h>
#include <nuttx/net/netdev.h>
#include <nuttx/kmalloc.h>
#include <syslog.h>
#include <string.h>

#include "stm32n6_ethernet.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* GMAC register base (per CMSIS stm32n647xx.h: ETH1_BASE_NS) */

#define STM32N6_ETH_BASE    0x48036000

/* GMAC register offsets */

#define ETH_MACCR_OFFSET    0x0000
#define ETH_MACECR_OFFSET   0x0004
#define ETH_MACPFR_OFFSET   0x0008
#define ETH_MACWTR_OFFSET   0x000C
#define ETH_MACHT0R_OFFSET  0x0010
#define ETH_MACHT1R_OFFSET  0x0014
#define ETH_MACISR_OFFSET   0x00B0
#define ETH_MACIER_OFFSET   0x00B4
#define ETH_MACMDIOAR_OFFSET 0x0200
#define ETH_MACMDIODR_OFFSET 0x0204
#define ETH_MACA0HR_OFFSET  0x0300  /* CMSIS: MAC Address 0 high */
#define ETH_MACA0LR_OFFSET  0x0304  /* CMSIS: MAC Address 0 low */

/* DMA registers */

#define ETH_DMAMR_OFFSET    0x1000
#define ETH_DMASBMR_OFFSET  0x1004
#define ETH_DMASR_OFFSET    0x1008
#define ETH_DMAIER_OFFSET   0x100C
#define ETH_DMAMFBOCR_OFFSET 0x1010
#define ETH_DMARSWTR_OFFSET 0x1018
#define ETH_DMACCR_OFFSET       0x1100  /* CMSIS: Channel x control */
#define ETH_DMACTXDLAR_OFFSET   0x1114  /* CMSIS: Ch x Tx desc list addr */
#define ETH_DMACRXDLAR_OFFSET   0x111C  /* CMSIS: Ch x Rx desc list addr */

/* MAC configuration register bits */

#define ETH_MACCR_RE        (1 << 0)   /* Receiver enable */
#define ETH_MACCR_TE        (1 << 1)   /* Transmitter enable */
#define ETH_MACCR_DM        (1 << 13)  /* Duplex mode */
#define ETH_MACCR_FES       (1 << 14)  /* Speed: 1=100M, 0=10M */
#define ETH_MACCR_BL_MASK   (3 << 17)  /* Back-off limit */
#define ETH_MACCR_BL_10     (1 << 17)
#define ETH_MACCR_APCS      (1 << 20)  /* Auto pad/CRC stripping */

/* DMA configuration */

#define ETH_DMAMR_SWR       (1 << 0)   /* Software reset */
#define ETH_DMAMR_TAA_MASK  (7 << 12)  /* Tx arbitration algorithm */
#define ETH_DMAMR_RAA_MASK  (7 << 2)   /* Rx arbitration algorithm */

/* DMA status */

#define ETH_DMASR_TS        (1 << 0)   /* Transmit status */
#define ETH_DMASR_RS        (1 << 6)   /* Receive status */

/* DMA channel 0 Rx control (CMSIS DMACRXCR @ 0x1108) */

#define ETH_DMACRXCR_OFFSET  0x1108
#define ETH_DMACRXCR_SR      (1 << 0)   /* Start/stop receive */

/* DMA channel 0 Tx control (CMSIS DMACTXCR @ 0x1104) */

#define ETH_DMACTXCR_OFFSET  0x1104
#define ETH_DMACTXCR_ST      (1 << 0)   /* Start/stop transmit */

/* MAC MDIO address register */

#define ETH_MACMDIOAR_GB         (1 << 0)
#define ETH_MACMDIOAR_GOC_READ   (3 << 2)
#define ETH_MACMDIOAR_GOC_WRITE  (1 << 2)
#define ETH_MACMDIOAR_CR_MASK    (0xf << 8)
#define ETH_MACMDIOAR_CR_60_100  (4 << 8)
#define ETH_MACMDIOAR_PA_MASK    (0x1f << 21)
#define ETH_MACMDIOAR_RDA_MASK   (0x1f << 16)

/* Rx/Tx descriptor counts */

#define ETH_RX_DESC_COUNT   4
#define ETH_TX_DESC_COUNT   4
#define ETH_BUF_SIZE        1536

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* DMA descriptor (enhanced, 32-byte aligned) */

struct eth_dma_desc_s
{
  volatile uint32_t des0;  /* Status/control */
  volatile uint32_t des1;  /* Buffer size / control */
  volatile uint32_t des2;  /* Buffer address */
  volatile uint32_t des3;  /* Extended status / next desc */
};

/* Driver private data */

struct stm32n6_eth_s
{
  struct net_driver_s dev;       /* NuttX network device */
  struct eth_dma_desc_s *rx_desc;
  struct eth_dma_desc_s *tx_desc;
  uint8_t *rx_buf[ETH_RX_DESC_COUNT];
  uint8_t *tx_buf[ETH_TX_DESC_COUNT];
  int      tx_idx;
  int      rx_idx;
  uint32_t base;
  uint8_t  mac_addr[6];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32n6_eth_s g_eth;

/* Aligned descriptor arrays */

static struct eth_dma_desc_s g_rx_desc[ETH_RX_DESC_COUNT]
  __attribute__((aligned(32)));
static struct eth_dma_desc_s g_tx_desc[ETH_TX_DESC_COUNT]
  __attribute__((aligned(32)));

static uint8_t g_rx_buf[ETH_RX_DESC_COUNT][ETH_BUF_SIZE]
  __attribute__((aligned(4)));
static uint8_t g_tx_buf[ETH_TX_DESC_COUNT][ETH_BUF_SIZE]
  __attribute__((aligned(4)));

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline uint32_t eth_getreg(struct stm32n6_eth_s *priv,
                                   uint32_t offset)
{
  return *(volatile uint32_t *)(priv->base + offset);
}

static inline void eth_putreg(struct stm32n6_eth_s *priv,
                               uint32_t offset, uint32_t value)
{
  *(volatile uint32_t *)(priv->base + offset) = value;
}

static int eth_mdio_read(struct stm32n6_eth_s *priv,
                          uint8_t phy_addr, uint8_t reg_addr)
{
  uint32_t mdioar;
  uint32_t timeout = 100000;

  mdioar = (phy_addr << 21) & ETH_MACMDIOAR_PA_MASK;
  mdioar |= (reg_addr << 16) & ETH_MACMDIOAR_RDA_MASK;
  mdioar |= ETH_MACMDIOAR_GOC_READ;
  mdioar |= ETH_MACMDIOAR_CR_60_100;
  mdioar |= ETH_MACMDIOAR_GB;

  eth_putreg(priv, ETH_MACMDIOAR_OFFSET, mdioar);

  while (timeout-- > 0)
    {
      if (!(eth_getreg(priv, ETH_MACMDIOAR_OFFSET) &
            ETH_MACMDIOAR_GB))
        {
          return eth_getreg(priv, ETH_MACMDIODR_OFFSET);
        }
    }

  return -ETIMEDOUT;
}

static void eth_mdio_write(struct stm32n6_eth_s *priv,
                            uint8_t phy_addr, uint8_t reg_addr,
                            uint16_t value)
{
  uint32_t mdioar;
  uint32_t timeout = 100000;

  eth_putreg(priv, ETH_MACMDIODR_OFFSET, value);

  mdioar = (phy_addr << 21) & ETH_MACMDIOAR_PA_MASK;
  mdioar |= (reg_addr << 16) & ETH_MACMDIOAR_RDA_MASK;
  mdioar |= ETH_MACMDIOAR_GOC_WRITE;
  mdioar |= ETH_MACMDIOAR_CR_60_100;
  mdioar |= ETH_MACMDIOAR_GB;

  eth_putreg(priv, ETH_MACMDIOAR_OFFSET, mdioar);

  while (timeout-- > 0)
    {
      if (!(eth_getreg(priv, ETH_MACMDIOAR_OFFSET) &
            ETH_MACMDIOAR_GB))
        {
          return;
        }
    }
}

static void eth_init_descs(struct stm32n6_eth_s *priv)
{
  int i;

  /* Rx descriptors */

  priv->rx_desc = g_rx_desc;
  memset(priv->rx_desc, 0,
         sizeof(struct eth_dma_desc_s) * ETH_RX_DESC_COUNT);

  for (i = 0; i < ETH_RX_DESC_COUNT; i++)
    {
      priv->rx_buf[i] = g_rx_buf[i];
      priv->rx_desc[i].des2 = (uint32_t)(uintptr_t)
                               priv->rx_buf[i];
      priv->rx_desc[i].des0 = 0x80000000;  /* Own by DMA */
    }

  /* Chain Rx descriptors (ring) */

  for (i = 0; i < ETH_RX_DESC_COUNT - 1; i++)
    {
      priv->rx_desc[i].des3 =
        (uint32_t)(uintptr_t)&priv->rx_desc[i + 1];
    }

  priv->rx_desc[ETH_RX_DESC_COUNT - 1].des3 =
    (uint32_t)(uintptr_t)&priv->rx_desc[0];

  /* Tx descriptors */

  priv->tx_desc = g_tx_desc;
  memset(priv->tx_desc, 0,
         sizeof(struct eth_dma_desc_s) * ETH_TX_DESC_COUNT);

  for (i = 0; i < ETH_TX_DESC_COUNT; i++)
    {
      priv->tx_buf[i] = g_tx_buf[i];
      priv->tx_desc[i].des2 = (uint32_t)(uintptr_t)
                               priv->tx_buf[i];
    }

  /* Chain Tx descriptors (ring) */

  for (i = 0; i < ETH_TX_DESC_COUNT - 1; i++)
    {
      priv->tx_desc[i].des3 =
        (uint32_t)(uintptr_t)&priv->tx_desc[i + 1];
    }

  priv->tx_desc[ETH_TX_DESC_COUNT - 1].des3 =
    (uint32_t)(uintptr_t)&priv->tx_desc[0];

  priv->rx_idx = 0;
  priv->tx_idx = 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_ethernet_init(uint8_t *mac_addr)
{
  struct stm32n6_eth_s *priv = &g_eth;
  uint32_t maccr;

  priv->base = STM32N6_ETH_BASE;

  if (mac_addr != NULL)
    {
      memcpy(priv->mac_addr, mac_addr, 6);
    }
  else
    {
      /* Default MAC address */

      priv->mac_addr[0] = 0x00;
      priv->mac_addr[1] = 0x80;
      priv->mac_addr[2] = 0xe1;
      priv->mac_addr[3] = 0x00;
      priv->mac_addr[4] = 0x00;
      priv->mac_addr[5] = 0x01;
    }

  /* Software reset DMA */

  eth_putreg(priv, ETH_DMAMR_OFFSET, ETH_DMAMR_SWR);

  int timeout = 100000;

  while (timeout-- > 0)
    {
      if (!(eth_getreg(priv, ETH_DMAMR_OFFSET) &
            ETH_DMAMR_SWR))
        {
          break;
        }
    }

  /* Configure MAC address (CMSIS MACA0HR @ 0x0300, MACA0LR @ 0x0304) */

  eth_putreg(priv, ETH_MACA0HR_OFFSET,
             (priv->mac_addr[5] << 8) |
             priv->mac_addr[4]);
  eth_putreg(priv, ETH_MACA0LR_OFFSET,
             (priv->mac_addr[3] << 24) |
             (priv->mac_addr[2] << 16) |
             (priv->mac_addr[1] << 8)  |
             priv->mac_addr[0]);

  /* Initialize descriptors */

  eth_init_descs(priv);

  /* Configure DMA: Rx/Tx burst length = 4 */

  eth_putreg(priv, ETH_DMASBMR_OFFSET,
             (4 << 16) | (4 << 8));

  /* Configure DMA channel 0 Rx descriptor list address
   * (CMSIS DMACRXDLAR @ 0x111C)
   */

  eth_putreg(priv, ETH_DMACRXDLAR_OFFSET,
             (uint32_t)(uintptr_t)&priv->rx_desc[0]);

  /* Configure DMA channel 0 Tx descriptor list address
   * (CMSIS DMACTXDLAR @ 0x1114)
   */

  eth_putreg(priv, ETH_DMACTXDLAR_OFFSET,
             (uint32_t)(uintptr_t)&priv->tx_desc[0]);

  /* Start DMA channels (CMSIS DMACRXCR @ 0x1108, DMACTXCR @ 0x1104) */

  eth_putreg(priv, ETH_DMACRXCR_OFFSET,
             eth_getreg(priv, ETH_DMACRXCR_OFFSET) |
             ETH_DMACRXCR_SR);
  eth_putreg(priv, ETH_DMACTXCR_OFFSET,
             eth_getreg(priv, ETH_DMACTXCR_OFFSET) |
             ETH_DMACTXCR_ST);

  /* Configure MAC: 100Mbps full duplex, enable Rx/Tx */

  maccr = ETH_MACCR_DM | ETH_MACCR_FES | ETH_MACCR_BL_10;
  maccr |= ETH_MACCR_RE | ETH_MACCR_TE;
  maccr |= ETH_MACCR_APCS;
  eth_putreg(priv, ETH_MACCR_OFFSET, maccr);

  syslog(LOG_INFO, "eth: initialized %02x:%02x:%02x:%02x:%02x:%02x\n",
         priv->mac_addr[0], priv->mac_addr[1],
         priv->mac_addr[2], priv->mac_addr[3],
         priv->mac_addr[4], priv->mac_addr[5]);

  return 0;
}
