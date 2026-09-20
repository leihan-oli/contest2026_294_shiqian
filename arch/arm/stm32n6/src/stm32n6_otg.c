/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_otg.c
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
 * STM32N6 USB OTG HS driver for NuttX.
 * Supports device mode for UVC (webcam) and mass storage.
 *
 * Adapted from STM32H7 NuttX reference (stm32_otgdev.c).
 * Reference: STM32N6 HAL (stm32n6xx_hal_pcd.c, stm32n6xx_hcd.c)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <syslog.h>
#include <string.h>

#include "stm32n6_otg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* USB OTG HS register base */

/* per CMSIS stm32n647xx.h: USB1_OTG_HS_BASE_NS */

#define STM32N6_OTG_HS_BASE  0x48040000

/* Core global registers */

#define OTG_GOTGCTL_OFFSET   0x000
#define OTG_GOTGINT_OFFSET   0x004
#define OTG_GAHBCFG_OFFSET   0x008
#define OTG_GUSBCFG_OFFSET   0x00C
#define OTG_GRSTCTL_OFFSET   0x010
#define OTG_GINTSTS_OFFSET   0x014
#define OTG_GINTMSK_OFFSET   0x018
#define OTG_GHWCFG1_OFFSET   0x044
#define OTG_GHWCFG2_OFFSET   0x048
#define OTG_GHWCFG3_OFFSET   0x04C
#define OTG_GHWCFG4_OFFSET   0x050
#define OTG_GSNPSID_OFFSET   0x040

/* Device registers */

#define OTG_DCFG_OFFSET      0x800
#define OTG_DCTL_OFFSET      0x804
#define OTG_DSTS_OFFSET      0x808
#define OTG_DIEPMSK_OFFSET   0x810
#define OTG_DOEPMSK_OFFSET   0x814
#define OTG_DAINT_OFFSET     0x818
#define OTG_DAINTMSK_OFFSET  0x81C

/* Power/clock gating */

#define OTG_PCGCCTL_OFFSET   0xE00

/* Endpoint registers (indexed) */

#define OTG_DIEPCTL0_OFFSET  0x900
#define OTG_DIEPINT0_OFFSET  0x908
#define OTG_DIEPTSIZ0_OFFSET 0x910
#define OTG_DIEPDMA0_OFFSET  0x914

/* FIFO registers */

#define OTG_DIEPTXF0_OFFSET  0x100

/* GAHBCFG bits */

#define OTG_GAHBCFG_GINT     (1 << 0)  /* Global interrupt enable */
#define OTG_GAHBCFG_TXFELVL  (1 << 7)  /* TxFIFO empty level */
#define OTG_GAHBCFG_PTXFELVL (1 << 8)  /* Periodic TxFIFO empty level */

/* GUSBCFG bits */

#define OTG_GUSBCFG_TOCAL_MASK (7 << 0)
#define OTG_GUSBCFG_PHYSEL     (1 << 6)  /* USB 2.0 HS PHY */
#define OTG_GUSBCFG_TRDT_MASK  (0xF << 10)
#define OTG_GUSBCFG_TRDT_16    (0x9 << 10)

/* GRSTCTL bits */

#define OTG_GRSTCTL_CSRST      (1 << 0)  /* Core soft reset */
#define OTG_GRSTCTL_HSRST      (1 << 1)  /* HCLK soft reset */
#define OTG_GRSTCTL_TXFNUM_MASK (0x1F << 6)
#define OTG_GRSTCTL_RXFFLSH    (1 << 4)  /* RxFIFO flush */
#define OTG_GRSTCTL_TXFFLSH    (1 << 5)  /* TxFIFO flush */

/* GINTSTS bits */

#define OTG_GINTSTS_CMOD       (1 << 0)  /* Current mode: 0=device */
#define OTG_GINTSTS_MMIS       (1 << 1)  /* Mode mismatch */
#define OTG_GINTSTS_SOF        (1 << 3)  /* Start of frame */
#define OTG_GINTSTS_RXFLVL     (1 << 4)  /* RxFIFO non-empty */
#define OTG_GINTSTS_GINAKEFF   (1 << 6)
#define OTG_GINTSTS_GONAKEFF   (1 << 7)
#define OTG_GINTSTS_USBRST     (1 << 12) /* USB reset */
#define OTG_GINTSTS_ENUMDNE    (1 << 13) /* Enumeration done */
#define OTG_GINTSTS_USBSUSP    (1 << 11) /* USB suspend */
#define OTG_GINTSTS_WKUPINT    (1 << 31) /* Resume/remote wakeup */

/* DCFG bits */

#define OTG_DCFG_DSPD_MASK    (3 << 0)  /* Device speed */
#define OTG_DCFG_DSPD_HS      (0 << 0)
#define OTG_DCFG_DSPD_FS      (1 << 0)
#define OTG_DCFG_DAD_MASK     (0x7f << 4)
#define OTG_DCFG_NZLSOHSK     (1 << 2)

/* DCTL bits */

#define OTG_DCTL_RWUSIG       (1 << 0)  /* Remote wakeup signaling */
#define OTG_DCTL_SDIS         (1 << 1)  /* Soft disconnect */
#define OTG_DCTL_GINSTS       (1 << 2)  /* Global non-periodic IN NAK */
#define OTG_DCTL_GONSTS       (1 << 3)  /* Global OUT NAK status */

/* DIEPCTL0 bits */

#define OTG_DIEPCTL0_MPSIZ_MASK (3 << 0)  /* Max packet size */
#define OTG_DIEPCTL0_MPSIZ_64  (0 << 0)
#define OTG_DIEPCTL0_TXFNUM_MASK (0xF << 22)
#define OTG_DIEPCTL0_EPENA     (1 << 31) /* Endpoint enable */
#define OTG_DIEPCTL0_EPDIS     (1 << 30) /* Endpoint disable */
#define OTG_DIEPCTL0_SNAK      (1 << 27) /* Set NAK */
#define OTG_DIEPCTL0_CNAK      (1 << 26) /* Clear NAK */
#define OTG_DIEPCTL0_STALL     (1 << 21) /* STALL handshake */
#define OTG_DIEPCTL0_EPTYP_MASK (3 << 18)
#define OTG_DIEPCTL0_EPTYP_CTRL (0 << 18)
#define OTG_DIEPCTL0_EPTYP_ISO  (1 << 18)
#define OTG_DIEPCTL0_EPTYP_BULK (2 << 18)
#define OTG_DIEPCTL0_EPTYP_INT  (3 << 18)

/* DIEPTSIZ0 bits */

#define OTG_DIEPTSIZ0_XFRSIZ_MASK (0x7F << 0)
#define OTG_DIEPTSIZ0_PKTCNT_MASK (3 << 19)

/* FIFO depth (words) */

#define OTG_TX0_FIFO_DEPTH   256  /* 1KB for EP0 */

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_otg_initialized = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void otg_putreg(uint32_t offset, uint32_t value)
{
  *(volatile uint32_t *)(STM32N6_OTG_HS_BASE + offset) = value;
}

static inline uint32_t otg_getreg(uint32_t offset)
{
  return *(volatile uint32_t *)(STM32N6_OTG_HS_BASE + offset);
}

static int otg_core_reset(void)
{
  uint32_t timeout = 100000;

  otg_putreg(OTG_GRSTCTL_OFFSET, OTG_GRSTCTL_CSRST);

  while (timeout-- > 0)
    {
      if (!(otg_getreg(OTG_GRSTCTL_OFFSET) &
            OTG_GRSTCTL_CSRST))
        {
          return 0;
        }
    }

  return -ETIMEDOUT;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_otg_device_init(enum usb_otg_speed_e speed)
{
  uint32_t gahbcfg;
  uint32_t gusbcfg;
  uint32_t dcfg;
  int ret;

  if (g_otg_initialized)
    {
      return 0;
    }

  /* Select HS PHY */

  otg_putreg(OTG_GUSBCFG_OFFSET, OTG_GUSBCFG_PHYSEL);

  /* Core soft reset */

  ret = otg_core_reset();
  if (ret < 0)
    {
      syslog(LOG_ERR, "otg: core reset timeout\n");
      return ret;
    }

  /* Configure AHB: enable global interrupt, TxFIFO empty level */

  gahbcfg = OTG_GAHBCFG_GINT | OTG_GAHBCFG_TXFELVL;
  otg_putreg(OTG_GAHBCFG_OFFSET, gahbcfg);

  /* Configure USB: HS PHY, 16-bit turnaround time */

  gusbcfg = OTG_GUSBCFG_PHYSEL | OTG_GUSBCFG_TRDT_16;
  otg_putreg(OTG_GUSBCFG_OFFSET, gusbcfg);

  /* Configure device mode: speed */

  dcfg = (speed == USB_OTG_SPEED_HIGH) ?
         OTG_DCFG_DSPD_HS : OTG_DCFG_DSPD_FS;
  otg_putreg(OTG_DCFG_OFFSET, dcfg);

  /* Flush all FIFOs (both Rx and Tx, per CMSIS GRSTCTL bits
   * RXFFLSH[4] and TXFFLSH[5]/TXFNUM[10:6]=0x10 for "all Tx FIFOs")
   */

  otg_putreg(OTG_GRSTCTL_OFFSET,
             OTG_GRSTCTL_RXFFLSH |
             OTG_GRSTCTL_TXFFLSH |
             (0x10 << 6));  /* Flush all Tx FIFOs */

  /* Clear all pending interrupts */

  otg_putreg(OTG_GINTSTS_OFFSET, 0xffffffff);

  /* Enable key interrupts */

  otg_putreg(OTG_GINTMSK_OFFSET,
             OTG_GINTSTS_USBRST |
             OTG_GINTSTS_ENUMDNE |
             OTG_GINTSTS_RXFLVL |
             OTG_GINTSTS_USBSUSP |
             OTG_GINTSTS_WKUPINT);

  g_otg_initialized = true;

  syslog(LOG_INFO, "otg: device mode initialized (%s speed)\n",
         speed == USB_OTG_SPEED_HIGH ? "high" : "full");
  return 0;
}

int stm32n6_otg_host_init(void)
{
  /* TODO: Implement host mode initialization */

  syslog(LOG_ERR, "otg: host mode not implemented\n");
  return -ENOSYS;
}

int stm32n6_otg_deinit(void)
{
  if (!g_otg_initialized)
    {
      return 0;
    }

  /* Soft disconnect */

  otg_putreg(OTG_DCTL_OFFSET, OTG_DCTL_SDIS);

  /* Disable interrupts */

  otg_putreg(OTG_GINTMSK_OFFSET, 0);

  g_otg_initialized = false;

  syslog(LOG_INFO, "otg: deinitialized\n");
  return 0;
}
