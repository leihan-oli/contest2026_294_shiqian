/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_sdmmc.c
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
 * STM32N6 SDMMC driver for NuttX.
 * Supports SDMMC1-2 in 4-bit SD mode with DMA.
 *
 * Adapted from STM32H7 NuttX reference (stm32_sdmmc.c).
 * Reference: STM32N6 HAL (stm32n6xx_hal_sd.c, stm32n6xx_ll_sdmmc.c)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/kmalloc.h>
#include <nuttx/semaphore.h>
#include <nuttx/fs/fs.h>
#include <nuttx/cache.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include "arm_internal.h"
#include "stm32n6_gpio.h"
#include "stm32n6_sdmmc.h"
#include "hardware/stm32_memorymap.h"
#include "hardware/stm32_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SDMMC register base addresses (per CMSIS stm32n647xx.h) */

/* STM32N6 SDMMC bases -- SECURE aliases (NuttX runs in the secure domain,
 * same as the DMA2D/NPU base usages).  Per CMSIS:
 *   SDMMC1_BASE_S = AHB5PERIPH_BASE_S(0x58020000) + 0x7000 = 0x58027000
 *   SDMMC2_BASE_S = AHB5PERIPH_BASE_S(0x58020000) + 0x6800 = 0x58026800
 * (The 0x48... non-secure aliases must NOT be used here.)
 */
#define STM32N6_SDMMC1_BASE  0x58027000
#define STM32N6_SDMMC2_BASE  0x58026800

/* RCC AHB5/CCIPR aliases and bits (RM0486 Rev 2):
 *  - RCC_AHB5ENR    @0x260  (status/shadow; bits are RIFSC SEC/PRIV
 *                            security-protected)
 *  - RCC_AHB5ENSR   @0xA60  (write-1-to-set enable alias; HW-recommended,
 *                            used by ST LL_AHB5_GRP1_EnableClock)
 *  - RCC_AHB5LPENSR @0xAA0  (write-1-to-set sleep-mode enable alias)
 *  - RCC_AHB5RSTSR  @0xA20  (write-1-to-set reset alias; triggers reset)
 *  - RCC_AHB5RSTCR  @0x1220 (write-1-to-clear reset alias; releases reset)
 *    NOTE: the earlier comment/offset 0xA24 was wrong (CMSIS: AHB5RSTSR
 *    @0x0A20, AHB5RSTCR @0x1220).
 *  - RCC_CCIPR8     @0x160  (SDMMC1SEL bits 1:0 / SDMMC2SEL bits 5:4)
 * SDMMC1/2 enable/LPEN/RST bits are bit 8 / bit 7 (same layout in all of
 * these registers, per the RM tables).
 */
#define STM32_RCC_AHB5ENSR    (STM32_RCC_BASE + 0x0A60)
#define STM32_RCC_AHB5LPENSR  (STM32_RCC_BASE + 0x0AA0)
#define STM32_RCC_AHB5RSTSR   (STM32_RCC_BASE + 0x0A20)
#define STM32_RCC_AHB5RSTCR   (STM32_RCC_BASE + 0x1220)
#define STM32_RCC_CCIPR8      (STM32_RCC_BASE + 0x0160)

#define RCC_AHB5RSTR_SDMMC1RST   (1 << 8)
#define RCC_AHB5RSTR_SDMMC2RST   (1 << 7)
#define RCC_CCIPR8_SDMMC1SEL     (0x3 << 0)
#define RCC_CCIPR8_SDMMC2SEL     (0x3 << 4)

/* SDMMC register offsets */

#define SDMMC_POWER_OFFSET     0x00
#define SDMMC_CLKCR_OFFSET    0x04
#define SDMMC_ARG_OFFSET      0x08
#define SDMMC_CMD_OFFSET      0x0C
#define SDMMC_RESPCMD_OFFSET  0x10
#define SDMMC_RESP1_OFFSET    0x14
#define SDMMC_RESP2_OFFSET    0x18
#define SDMMC_RESP3_OFFSET    0x1C
#define SDMMC_RESP4_OFFSET    0x20
#define SDMMC_DTIMER_OFFSET   0x24
#define SDMMC_DLEN_OFFSET     0x28
#define SDMMC_DCTRL_OFFSET    0x2C
#define SDMMC_DCOUNT_OFFSET   0x30
#define SDMMC_STA_OFFSET      0x34
#define SDMMC_ICR_OFFSET      0x38
#define SDMMC_MASK_OFFSET     0x3C
#define SDMMC_IDMACTRL_OFFSET 0x50
#define SDMMC_IDMABS_OFFSET   0x54
#define SDMMC_FIFO_OFFSET     0x80

/* SDMMC_POWER bits */

#define SDMMC_POWER_PWRCTRL_MASK  0x03
#define SDMMC_POWER_PWRCTRL_ON    0x03

/* SDMMC_CLKCR bits */

/* STM32N6 SDMMC CLKCR (per CMSIS stm32n647xx.h):
 *   CLKDIV  = bits 9:0
 *   PWRSAV  = bit 12
 *   WIDBUS  = bits 15:14 (0b10 = 4-bit bus)
 *   NEGEDGE = bit 16  -- DE-PHASING selection, NOT a clock enable!
 * There is NO CLKEN bit on STM32N6; the SD clock is gated by the
 * SDMMC_POWER.PWRCTRL field (set below to 0b11 = power on).
 */
#define SDMMC_CLKCR_CLKDIV_MASK   0x3FF
#define SDMMC_CLKCR_WIDBUS_4BIT   (1 << 14)   /* SDMMC_BUS_WIDE_4B = WIDBUS_0 (bit14), per LL */
#define SDMMC_CLKCR_HWFC_EN       (1 << 17)   /* HWFC_EN bit17: RM0486 data xfer step 2 */

/* SDMMC_CMD bits */

#define SDMMC_CMD_CMDINDEX_MASK   0x3F
#define SDMMC_CMD_WAITRESP_MASK   (3 << 8)   /* CMSIS: bits 9:8 */
#define SDMMC_CMD_WAITRESP_NONE   (0 << 8)
#define SDMMC_CMD_WAITRESP_SHORT  (1 << 8)
#define SDMMC_CMD_WAITRESP_LONG   (3 << 8)
#define SDMMC_CMD_CPSMEN          (1 << 12)  /* CMSIS: bit 12 */
#define SDMMC_CMD_CMDTRANS        (1 << 6)   /* CMSIS: bit 6 (was 21!) */

/* SDMMC_STA bits */

#define SDMMC_STA_CCRCFAIL       (1 << 0)
#define SDMMC_STA_DCRCFAIL       (1 << 1)
#define SDMMC_STA_CTIMEOUT       (1 << 2)
#define SDMMC_STA_DTIMEOUT       (1 << 3)
#define SDMMC_STA_TXUNDERR       (1 << 4)
#define SDMMC_STA_RXOVERR        (1 << 5)
#define SDMMC_STA_CMDREND        (1 << 6)
#define SDMMC_STA_CMDSENT        (1 << 7)
#define SDMMC_STA_DATAEND        (1 << 8)
#define SDMMC_STA_DTO            (1 << 9)
#define SDMMC_STA_TXFIFOHE       (1 << 14)
#define SDMMC_STA_RXFIFOHF       (1 << 15)
#define SDMMC_STA_RXFIFOE        (1 << 19)   /* Receive FIFO empty */
#define SDMMC_STA_BUSYD0         (1 << 24)
#define SDMMC_STA_CPSMACT        (1 << 13)   /* Command path state machine
                                               * active (busy), per RM0486 */

/* Static command flags (== ST LL SDMMC_STATIC_CMD_FLAGS): cleared after
 * every command, exactly like HAL_SD_GetCmdError / GetCmdResp7. */
#define SDMMC_STATIC_CMD_FLAGS  (SDMMC_STA_CCRCFAIL | SDMMC_STA_CTIMEOUT | \
                                 SDMMC_STA_CMDREND | SDMMC_STA_CMDSENT)

/* SDMMC_DCTRL bits */

#define SDMMC_DCTRL_DTEN         (1 << 0)
#define SDMMC_DCTRL_DTDIR_READ   (1 << 1)
#define SDMMC_DCTRL_DTMODE_BLOCK (0 << 2)
#define SDMMC_DCTRL_DBLOCKSIZE_MASK (0xF << 4)

/* SD commands */

#define SD_CMD_GO_IDLE_STATE      0
#define SD_CMD_SEND_IF_COND       8
#define SD_CMD_SEND_CSD           9
#define SD_CMD_SEND_CID           10
#define SD_CMD_STOP_TRANSMISSION  12
#define SD_CMD_SEND_STATUS        13
#define SD_CMD_SET_BLOCKLEN       16
#define SD_CMD_READ_SINGLE_BLOCK  17
#define SD_CMD_READ_MULT_BLOCK    18
#define SD_CMD_WRITE_SINGLE_BLOCK 24
#define SD_CMD_WRITE_MULT_BLOCK   25
#define SD_CMD_APP_CMD            55
#define SD_CMD_SD_SEND_OP_COND    41
#define SD_CMD_SWITCH             6   /* ACMD6 (after CMD55): SET_BUS_WIDTH */
#define SD_CMD_ALL_SEND_CID       2
#define SD_CMD_SEND_RELATIVE_ADDR 3

/* Timeouts */

#define SDMMC_TIMEOUT_MS   1000

#ifndef CONFIG_STM32N6_SDMMC_DEVPATH
#  define CONFIG_STM32N6_SDMMC_DEVPATH "/dev/mmcsd0"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_sdmmc_priv_s
{
  uint32_t base;
  uint32_t clock;
  int      irq;
  sem_t    lock;
  sem_t    wait;
  uint32_t rca;          /* Relative Card Address */
  uint32_t block_size;
  bool     initialized;
  uint32_t card_type;
  uint64_t block_count;  /* Derived from CSD */
  bool     block_registered;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* SDMMC1 pin table — TF card on the base board (per ALIENTEK 38_SD_Card
 * example, stm32n6xx_hal_msp.c): PC8-11 = D0-D3, PC12 = CK, PH2 = CMD,
 * all AF10.  SDMMC2 (SD NAND on the core board) is not wired up here.
 */

/* NOTE: each entry MUST include GPIO_MODE_AF | GPIO_OTYPE_PP (like
 * GPIO_USART1_TX / GPIO_XSPI1_IO0).  Without GPIO_MODE_AF the pin stays
 * MODER=00 (input) and stm32n6_configgpio() skips the AFR write entirely
 * (it only writes AF when mode == 2) -- verified on real HW via the
 * GPIOC/GPIOH MODER/AFR readback (AFRL/AFRH were 0x00000000).  That left
 * the SDMMC clock/CMD pins unconnected and the command state machine
 * never completed (CPSMACT stuck).  THIS WAS THE ROOT CAUSE.
 */

static const uint32_t g_sdmmc1_pins[] =
{
  GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_AF(10) | GPIO_SPEED_HIGH |
    GPIO_PUPD_NONE | GPIO_PORTC | GPIO_PIN(8),
  GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_AF(10) | GPIO_SPEED_HIGH |
    GPIO_PUPD_NONE | GPIO_PORTC | GPIO_PIN(9),
  GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_AF(10) | GPIO_SPEED_HIGH |
    GPIO_PUPD_NONE | GPIO_PORTC | GPIO_PIN(10),
  GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_AF(10) | GPIO_SPEED_HIGH |
    GPIO_PUPD_NONE | GPIO_PORTC | GPIO_PIN(11),
  GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_AF(10) | GPIO_SPEED_HIGH |
    GPIO_PUPD_NONE | GPIO_PORTC | GPIO_PIN(12),
  GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_AF(10) | GPIO_SPEED_HIGH |
    GPIO_PUPD_NONE | GPIO_PORTH | GPIO_PIN(2),
};

#define STM32N6_SDMMC1_NPINS   (sizeof(g_sdmmc1_pins) / sizeof(g_sdmmc1_pins[0]))

static struct stm32n6_sdmmc_priv_s g_sdmmc1_priv;
static struct stm32n6_sdmmc_priv_s g_sdmmc2_priv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static const struct block_operations g_stm32n6_sdmmc_bops;

static inline uint32_t sdmmc_getreg(
    struct stm32n6_sdmmc_priv_s *priv, uint32_t offset)
{
  return *(volatile uint32_t *)(priv->base + offset);
}

static inline void sdmmc_putreg(
    struct stm32n6_sdmmc_priv_s *priv, uint32_t offset,
    uint32_t value)
{
  *(volatile uint32_t *)(priv->base + offset) = value;
}

static int sdmmc_wait_status(struct stm32n6_sdmmc_priv_s *priv,
                              uint32_t mask)
{
  uint32_t timeout = 5000000;

  while (timeout-- > 0)
    {
      uint32_t status = sdmmc_getreg(priv, SDMMC_STA_OFFSET);
      if (status & SDMMC_STA_CTIMEOUT)
        {
          sdmmc_putreg(priv, SDMMC_ICR_OFFSET,
                       SDMMC_STATIC_CMD_FLAGS);
          return -ETIMEDOUT;
        }

      /* ST HAL semantics (SDMMC_GetCmdResp7): a command is complete only
       * when the target flag is set AND CMDACT (CPSMACT) is cleared.
       * Waiting on CMDREND alone lets the next command be written while
       * the command state machine is still busy -> it freezes with
       * CPSMACT stuck (observed on real HW before this fix).
       * The response value must be read AFTER CPSMACT clears. */
      if ((status & mask) && !(status & SDMMC_STA_CPSMACT))
        {
          sdmmc_putreg(priv, SDMMC_ICR_OFFSET, mask);
          return 0;
        }
    }

  return -ETIMEDOUT;
}

/* Wait for CMDSENT (no-response command, e.g. CMD0).  Mirrors ST HAL
 * SDMMC_GetCmdError(): waits for CMDSENT, then clears ALL static command
 * flags.  Important: a no-response command does NOT raise CMDREND, so
 * the next command must not be queued before CMDSENT, otherwise the
 * command state machine overlaps and freezes (CPSMACT stuck). */

static int sdmmc_wait_cmdsent(struct stm32n6_sdmmc_priv_s *priv)
{
  uint32_t timeout = 5000000;

  while (timeout-- > 0)
    {
      uint32_t status = sdmmc_getreg(priv, SDMMC_STA_OFFSET);
      if (status & SDMMC_STA_CMDSENT)
        {
          sdmmc_putreg(priv, SDMMC_ICR_OFFSET,
                       SDMMC_STATIC_CMD_FLAGS);
          return 0;
        }

      if (status & SDMMC_STA_CTIMEOUT)
        {
          sdmmc_putreg(priv, SDMMC_ICR_OFFSET,
                       SDMMC_STATIC_CMD_FLAGS);
          return -ETIMEDOUT;
        }
    }

  syslog(LOG_ERR, "sdmmc: CMDSENT timeout (STA=0x%08lx)\n",
         (unsigned long)sdmmc_getreg(priv, SDMMC_STA_OFFSET));

  return -ETIMEDOUT;
}

/* Wait for the command path state machine to be idle (CPSMACT == 0).
 * NOTE: the previous code waited for CMDSENT/CMDREND here, which NEVER
 * asserts on the first command (there is nothing outstanding yet), so the
 * loop burned a full timeout before every CMD0.  Idle = CPSMACT clear.
 */

static int sdmmc_wait_idle(struct stm32n6_sdmmc_priv_s *priv)
{
  uint32_t timeout = 1000000;

  while (timeout-- > 0)
    {
      if (!(sdmmc_getreg(priv, SDMMC_STA_OFFSET) & SDMMC_STA_CPSMACT))
        {
          return 0;
        }
    }

  syslog(LOG_ERR, "sdmmc: wait_idle timeout (STA=0x%08lx)\n",
         (unsigned long)sdmmc_getreg(priv, SDMMC_STA_OFFSET));

  return -ETIMEDOUT;
}

static int sdmmc_send_cmd(struct stm32n6_sdmmc_priv_s *priv,
                           uint32_t cmd, uint32_t arg,
                           uint32_t *resp)
{
  uint32_t cmdreg;
  int ret;

  /* ST HAL semantics: SDMMC_SendCommand() does NOT wait for the command
   * path to be idle before writing the CMD register (it only writes).
   * Keep it identical here -- waiting on CPSMACT before CMD8 deadlocks
   * when a previous command left CPSMACT set on N6.
   */

  /* Set argument */

  sdmmc_putreg(priv, SDMMC_ARG_OFFSET, arg);

  /* Configure command */

  cmdreg = cmd & SDMMC_CMD_CMDINDEX_MASK;
  cmdreg |= SDMMC_CMD_CPSMEN;

  if (resp != NULL)
    {
      cmdreg |= SDMMC_CMD_WAITRESP_SHORT;
    }

  /* Data-transfer commands (read/write block) must set CMDTRANS (bit 6)
   * per CMSIS stm32n647xx.h -- without it the data phase never starts. */

  if (cmd == SD_CMD_READ_SINGLE_BLOCK ||
      cmd == SD_CMD_READ_MULT_BLOCK ||
      cmd == SD_CMD_WRITE_SINGLE_BLOCK ||
      cmd == SD_CMD_WRITE_MULT_BLOCK)
    {
      cmdreg |= SDMMC_CMD_CMDTRANS;
    }

  /* Send command */

  sdmmc_putreg(priv, SDMMC_CMD_OFFSET, cmdreg);

  /* Wait for response.
   * NOTE (ST HAL semantics): SDMMC_SendCommand() never waits for a
   * response; the HAL waits in GetCmdResp1/7 (done in sdmmc_wait_status)
   * and clears the static command flags afterwards. */

  if (resp != NULL)
    {
      ret = sdmmc_wait_status(priv,
                               SDMMC_STA_CMDREND |
                               SDMMC_STA_CCRCFAIL);
      if (ret < 0)
        {
          return ret;
        }

      *resp = sdmmc_getreg(priv, SDMMC_RESP1_OFFSET);
    }
  else
    {
      /* No response expected (CMD0): ST HAL waits CMDSENT then clears
       * the static command flags (SDMMC_GetCmdError).  Without this the
       * next command is queued while CMD0 is still in flight and the
       * command state machine freezes (CPSMACT stuck, observed on real
       * HW). */
      ret = sdmmc_wait_cmdsent(priv);
      if (ret < 0)
        {
          return ret;
        }
    }

  return 0;
}

/* Send a command expecting a LONG (CSD/CID, 4x32bit) response.
 * CMD2 (CID) and CMD9 (CSD) use RESP4-1 registers.
 */

static int sdmmc_send_cmd_long(struct stm32n6_sdmmc_priv_s *priv,
                               uint32_t cmd, uint32_t arg,
                               uint32_t resp[4])
{
  uint32_t cmdreg;
  int ret;

  /* Same ST semantics as sdmmc_send_cmd: no pre-write idle wait. */

  sdmmc_putreg(priv, SDMMC_ARG_OFFSET, arg);

  cmdreg = (cmd & SDMMC_CMD_CMDINDEX_MASK) |
           SDMMC_CMD_WAITRESP_LONG |
           SDMMC_CMD_CPSMEN;
  sdmmc_putreg(priv, SDMMC_CMD_OFFSET, cmdreg);

  ret = sdmmc_wait_status(priv, SDMMC_STA_CMDREND |
                                 SDMMC_STA_CCRCFAIL);
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "sdmmc: CMD%lu long response timeout (STA=0x%08lx)\n",
             (unsigned long)(cmd & SDMMC_CMD_CMDINDEX_MASK),
             (unsigned long)sdmmc_getreg(priv, SDMMC_STA_OFFSET));
      return ret;
    }

  resp[0] = sdmmc_getreg(priv, SDMMC_RESP1_OFFSET);
  resp[1] = sdmmc_getreg(priv, SDMMC_RESP2_OFFSET);
  resp[2] = sdmmc_getreg(priv, SDMMC_RESP3_OFFSET);
  resp[3] = sdmmc_getreg(priv, SDMMC_RESP4_OFFSET);

  return 0;
}

static int sdmmc_read_block(struct stm32n6_sdmmc_priv_s *priv,
                             uint32_t sector, void *buf)
{
  uint32_t *dst = (uint32_t *)buf;
  uint32_t status;
  uint32_t timeout;
  int ret;
  uint32_t i;
  uint32_t resp;

  /* Data path setup, ST HAL order (SDMMC_ConfigData with DPSM=DISABLE):
   * DCTRL must have direction/mode/block-size but NOT DTEN=1 before the
   * command -- RM0486: "When DTEN = 1, no command is transferred".  The
   * CMDTRANS bit set by sdmmc_send_cmd makes the CPSM issue DataEnable
   * to the DPSM at the end of the command, which starts the data phase.
   * (Writing DCTRL=0 leaves no direction/size -> data phase never starts;
   * observed as wait_status(DATAEND) timeout with STA=TXFIFOHE|TXFIFOE.) */
  sdmmc_putreg(priv, SDMMC_DCTRL_OFFSET,
               SDMMC_DCTRL_DTDIR_READ |
               SDMMC_DCTRL_DTMODE_BLOCK |
               (9 << 4));  /* 2^9 = 512, no DTEN */
  sdmmc_putreg(priv, SDMMC_DTIMER_OFFSET, 0x03FFFFFF);
  sdmmc_putreg(priv, SDMMC_DLEN_OFFSET, 512);

  /* Send read command (CMD17 expects R1 -- mirror ST HAL
   * SDMMC_CmdReadSingleBlock + GetCmdResp1) */

  ret = sdmmc_send_cmd(priv, SD_CMD_READ_SINGLE_BLOCK,
                        sector, &resp);
  if (ret < 0)
    {
      return ret;
    }

  /* Read data from FIFO.
   * HAL_SD_ReadBlocks style: on RXFIFOHF the WHOLE remaining block is
   * read in one sweep (HAL reads SDMMC_FIFO_SIZE/4 = 128 words when the
   * flag asserts), then the loop keeps polling for DATAEND/errors.  With
   * HWFC_EN=1 the receive data transfer is stalled whenever the RX FIFO
   * is not "half empty", so a timid 4-word-per-flag read freezes the card
   * in the middle of the block (observed: DCOUNT stuck at 0x1F0). */

  timeout = 1000000;
  i = 0;

  while (i < 128 && timeout-- > 0)  /* 512/4 = 128 words */
    {
      status = sdmmc_getreg(priv, SDMMC_STA_OFFSET);

      if (status & SDMMC_STA_RXOVERR)
        {
          return -EIO;
        }

      if (status & SDMMC_STA_RXFIFOHF)
        {
          /* Drain the FIFO completely (until RXFIFOE) so the HWFC flow
           * control sees a half-empty receive FIFO and restarts SDMMC_CK.
           * Fixed-size chunks left data behind and the transfer stayed
           * frozen (observed: DCOUNT stuck, no DATAEND). */
          while (i < 128)
            {
              status = sdmmc_getreg(priv, SDMMC_STA_OFFSET);
              if (status & SDMMC_STA_RXFIFOE)
                {
                  break;
                }

              dst[i++] = sdmmc_getreg(priv, SDMMC_FIFO_OFFSET);
            }

          if (i >= 128)
            {
              break;
            }
        }
    }

  /* Wait for data end */

  ret = sdmmc_wait_status(priv, SDMMC_STA_DATAEND);
  if (ret < 0)
    {
      return ret;
    }

  return 0;
}

static int sdmmc_write_block(struct stm32n6_sdmmc_priv_s *priv,
                              uint32_t sector, const void *buf)
{
  const uint32_t *src = (const uint32_t *)buf;
  uint32_t status;
  uint32_t timeout;
  int ret;
  uint32_t i;
  uint32_t resp;

  /* Same data-path order as read block: DCTRL (mode/size, NO DTEN),
   * DTIMER, DLEN, then the write command (CMD24 expects R1). */

  sdmmc_putreg(priv, SDMMC_DCTRL_OFFSET,
               SDMMC_DCTRL_DTMODE_BLOCK |
               (9 << 4));  /* 2^9 = 512, no DTEN, no direction (tx) */
  sdmmc_putreg(priv, SDMMC_DTIMER_OFFSET, 0x03FFFFFF);
  sdmmc_putreg(priv, SDMMC_DLEN_OFFSET, 512);

  /* Send write command (CMD24 expects R1) */

  ret = sdmmc_send_cmd(priv, SD_CMD_WRITE_SINGLE_BLOCK,
                        sector, &resp);
  if (ret < 0)
    {
      return ret;
    }

  /* Write data to FIFO */

  timeout = 1000000;
  i = 0;

  while (i < 128 && timeout-- > 0)
    {
      status = sdmmc_getreg(priv, SDMMC_STA_OFFSET);

      if (status & SDMMC_STA_TXUNDERR)
        {
          return -EIO;
        }

      if (status & SDMMC_STA_TXFIFOHE)
        {
          sdmmc_putreg(priv, SDMMC_FIFO_OFFSET, src[i++]);
          sdmmc_putreg(priv, SDMMC_FIFO_OFFSET, src[i++]);
          sdmmc_putreg(priv, SDMMC_FIFO_OFFSET, src[i++]);
          sdmmc_putreg(priv, SDMMC_FIFO_OFFSET, src[i++]);
        }
    }

  /* Wait for data end */

  ret = sdmmc_wait_status(priv, SDMMC_STA_DATAEND);
  if (ret < 0)
    {
      return ret;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_sdmmc_initialize(int bus_num)
{
  struct stm32n6_sdmmc_priv_s *priv;
  uint32_t resp;
  int ret;

  switch (bus_num)
    {
      case 1:
        priv = &g_sdmmc1_priv;
        priv->base = STM32N6_SDMMC1_BASE;
        priv->clock = 200000000;  /* HCLK = 200MHz (FSBL PLL1 path);
                                   * SDMMCCLK = HCLK via CCIPR8.SDMMC1SEL=00 */
        break;
      case 2:
        priv = &g_sdmmc2_priv;
        priv->base = STM32N6_SDMMC2_BASE;
        priv->clock = 200000000;
        break;
      default:
        syslog(LOG_ERR, "sdmmc: unsupported bus %d\n",
               bus_num);
        return -EINVAL;
    }

  if (priv->initialized)
    {
      return 0;
    }

  /* D-Cache OFF -- same as the verified bare-metal tfcard environment
   * (our own golden reference runs with SCTLR.C=0, the FSBL leaves
   * D-Cache disabled) and the same policy as the verified N6 DCMIPP
   * driver (stm32n6_dcmipp_init() calls up_disable_dcache()).  With
   * SCTLR.C=1 and MPU disabled (default map), writes to the SDMMC
   * register/clock domain can be held in the cache -> the command state
   * machine never sees the programming sequence and freezes (CPSMACT
   * stuck, observed on real HW). */

  up_disable_dcache();

  /* NOTE: ARMv8-M (CM55) D-Cache enable/disable is controlled by
   * NVIC_CFGCON.DC (NuttX arm_cache.c and CMSIS agree); SCTLR.C is a
   * reserved bit (reads as 1) and does NOT gate the data cache.  The
   * bare-metal golden reference also never uses SCTLR for cache. */

  /* RIF grant for SDMMC1 is applied AFTER the full card initialization,
   * exactly like the bare-metal 38_SD_Card reference (HAL_SD_Init first,
   * then SystemIsolation_Config()) -- see the RIF block further below.
   */

  /* Enable the SDMMC peripheral clock (RCC AHB5ENSR.SDMMCxEN -- the
   * write-1-to-set alias; AHB5ENR @0x260 is read-only status!) BEFORE
   * touching any SDMMC register -- without the clock the register reads
   * return 0 and the cmd/data status loops would hang the boot.
   * Verified against the bare-metal 38_SD_Card reference (2026-09-08):
   * its HAL MspInit uses __HAL_RCC_SDMMC1_CLK_ENABLE() = LL_AHB5_GRP1_
   * EnableClock -> writes AHB5ENSR.  The previous modifyreg32() on
   * AHB5ENR (status reg) did nothing if the bit was already 0 in
   * hardware.
   */

  /* SDMMC kernel clock source = HCLK (CCIPR8.SDMMCxSEL = 0b00), same as
   * the bare-metal HAL_RCCEx_PeriphCLKConfig(RCC_PERIPHCLK_SDMMC1,
   * RCC_SDMMC1CLKSOURCE_HCLK) in the 38_SD_Card MspInit.  Explicitly
   * program it (instead of trusting the reset default) to match the
   * verified bare-metal clock tree (HCLK = 200MHz from the FSBL PLL1
   * path).
   */

  if (bus_num == 1)
    {
      modifyreg32(STM32_RCC_CCIPR8, RCC_CCIPR8_SDMMC1SEL, 0U); /* HCLK */
      putreg32(RCC_AHB5ENR_SDMMC1EN, STM32_RCC_AHB5ENSR);
    }
  else if (bus_num == 2)
    {
      modifyreg32(STM32_RCC_CCIPR8, RCC_CCIPR8_SDMMC2SEL, 0U); /* HCLK */
      putreg32(RCC_AHB5ENR_SDMMC2EN, STM32_RCC_AHB5ENSR);
    }

  /* Fault-tolerant SDMMC peripheral reset (RCC AHB5RSTSR set / AHB5RSTCR
   * clear aliases).  Guarantees a clean command state machine even if an
   * earlier diagnostic probe (or a crashed driver) left CPSMACT frozen:
   * without this the first CMD0 inherits a stuck state machine and never
   * reports CMDSENT (observed: STA=0x2000/CMD=0x1000 left over by the
   * [E3] probe in stm32n6_start.c). */
  if (bus_num == 1)
    {
      putreg32(RCC_AHB5RSTR_SDMMC1RST, STM32_RCC_AHB5RSTSR);
    }
  else
    {
      putreg32(RCC_AHB5RSTR_SDMMC2RST, STM32_RCC_AHB5RSTSR);
    }

  {
    volatile uint32_t rst_i;
    for (rst_i = 0; rst_i < 1000; rst_i++) { }
  }

  if (bus_num == 1)
    {
      putreg32(RCC_AHB5RSTR_SDMMC1RST, STM32_RCC_AHB5RSTCR);
    }
  else
    {
      putreg32(RCC_AHB5RSTR_SDMMC2RST, STM32_RCC_AHB5RSTCR);
    }

  /* Configure SDMMC1 GPIO (TF card, AF10).  Needed before powering
   * the SDMMC block; done once per bus here.
   */

  if (bus_num == 1)
    {
      int i;

      for (i = 0; i < STM32N6_SDMMC1_NPINS; i++)
        {
          stm32n6_configgpio(g_sdmmc1_pins[i]);
        }
    }

  nxsem_init(&priv->lock, 0, 1);
  nxsem_init(&priv->wait, 0, 0);
  priv->block_size = 512;

  /* Configure clock FIRST (same order as ST HAL_SD_Init -> SDMMC_Init):
   * Nominal HCLK = 200MHz -> ST uses ClockDiv = 200M/(2*400k) = 250 for
   * SDMMC_CK = 400kHz.  Use the exact bare-metal value (the verified
   * tfcard golden reference ran its card init with CLKDIV=250):
   */

  sdmmc_putreg(priv, SDMMC_CLKCR_OFFSET,
               (250 & SDMMC_CLKCR_CLKDIV_MASK));

  /* Power on SDMMC (PWRCTRL=11).  Per RM0486 the SDMMC is DISABLED for
   * the first 74 SDMMC_CK cycles after power-on ("First 74 SDMMC_CK
   * cycles the SDMMC is still disabled"), and ST's HAL_SD_InitCard()
   * waits 1 + 74*1000/sdmmc_clk ms before issuing the first command.
   * Without this wait the CMD0 lands in the disabled window and the
   * command state machine never completes (CPSMACT stuck).
   */

  sdmmc_putreg(priv, SDMMC_POWER_OFFSET,
               SDMMC_POWER_PWRCTRL_ON);

  /* DIAG (2026-09-09): the bare-metal tfcard [P] probe uses a plain
   * for-loop delay (1500000 iterations @600MHz ~ 2.5ms) and succeeds;
   * NuttX used up_mdelay(5) which depends on SystemCoreClock/tick and
   * may return immediately if the clock estimate is wrong -- then CMD0
   * lands inside the ST-mandated 74-SDMMC_CK disabled window (850us
   * @400kHz) and the command state machine freezes (CPSMACT stuck).
   * Use the exact same loop as the verified golden reference. */
  {
    volatile uint32_t diag_i;
    for (diag_i = 0; diag_i < 1500000; diag_i++) { }
  }

  /* DLYB (SDMMC delay block @0x58028000): the bare-metal 38_SD_Card
   * reference does NOT configure DLYB at all (HAL_SD leaves it at the
   * reset default) and the SD card initializes fine.  Keep the reset
   * default here too -- the previous experiment that forced BYP_EN
   * (bit16) diverged from the verified bare-metal behavior, so it is
   * removed.  Only the diagnostic readback below is kept.
   */

  /* All bring-up diagnostics (register snapshots, MPU/CCR dumps)
   * were removed on 2026-09-10 after the SDMMC1/TF-card bring-up was
   * completed.  The essential initialization (peripheral reset, GPIO,
   * clocks, RIF, card init, ACMD6, HWFC, FIFO drain) stays below. */

  /* Send CMD0: GO_IDLE_STATE */

  ret = sdmmc_send_cmd(priv, SD_CMD_GO_IDLE_STATE, 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "sdmmc%d: GO_IDLE failed\n", bus_num);
      return ret;
    }

  /* Send CMD8: SEND_IF_COND (check voltage) */

  ret = sdmmc_send_cmd(priv, SD_CMD_SEND_IF_COND,
                        0x1aa, &resp);
  if (ret < 0)
    {
      syslog(LOG_ERR, "sdmmc%d: SEND_IF_COND failed\n",
             bus_num);
      return ret;
    }

  /* ACMD41: SD_SEND_OP_COND (wait for ready).
   * NOTE: CMD55 (APP_CMD) expects an R1 response.  Sending it as a
   * no-response command (RESP NULL) leaves the command path busy
   * (CPSMACT), and the immediately following ACMD41 gets dropped /
   * frozen -- observed on real HW as "card not ready" after 100
   * retries.  Mirror ST HAL SDMMC_CmdAppCommand(): Response=SHORT +
   * GetCmdResp1.
   */

  int retry = 100;

  while (retry-- > 0)
    {
      ret = sdmmc_send_cmd(priv, SD_CMD_APP_CMD, 0, &resp);
      if (ret < 0)
        {
          syslog(LOG_INFO, "sdmmc%d: CMD55 failed (%d), retry...\n",
                 bus_num, ret);
          continue;
        }

      ret = sdmmc_send_cmd(priv, SD_CMD_SD_SEND_OP_COND,
                            0x40ff8000, &resp);
      if (ret < 0)
        {
          syslog(LOG_INFO, "sdmmc%d: ACMD41 failed (%d), retry...\n",
                 bus_num, ret);
          continue;
        }

      if (resp & 0x80000000)
        {
          break;
        }
    }

  if (retry <= 0)
    {
      syslog(LOG_ERR, "sdmmc%d: card not ready\n", bus_num);
      return -ETIMEDOUT;
    }

  /* CMD2: ALL_SEND_CID (136-bit LONG response) */

  {
    uint32_t cid[4];
    ret = sdmmc_send_cmd_long(priv, SD_CMD_ALL_SEND_CID, 0, cid);
    if (ret < 0)
      {
        syslog(LOG_ERR, "sdmmc%d: ALL_SEND_CID failed\n", bus_num);
        return ret;
      }
  }

  /* CMD3: SEND_RELATIVE_ADDR */

  ret = sdmmc_send_cmd(priv, SD_CMD_SEND_RELATIVE_ADDR,
                        0, &resp);
  if (ret < 0)
    {
      syslog(LOG_ERR, "sdmmc%d: SEND_RCA failed\n", bus_num);
      return ret;
    }

  priv->rca = resp & 0xffff0000;

  /* Read CSD (CMD9) to derive the card capacity.  Standard SD 2.0
   * CSD v1.0: C_SIZE bits [73:62], C_SIZE_MULT [55:47], READ_BL_LEN
   * [83:80] -> (C_SIZE+1) * 2^(C_SIZE_MULT+2) * 2^READ_BL_LEN blocks.
   * CSD v2.0 (SDHC): CSD[7:0] word = resp[2], C_SIZE = 0x0FFFFF &
   * ((resp[1] & 0x3F) << 16 | resp[2] >> 16).
   *
   * NOTE: CMD9 is issued at 400kHz/1-bit (before the 4-bit/25MHz switch)
   * to match ST HAL_SD_InitCard order: a freshly ready card can answer
   * CMD9 slower than the 25MHz command-response timeout allows
   * (observed: CTIMEOUT/-110 when CMD9 was sent after the speed switch).
   */

  {
    uint32_t csd[4];
    uint64_t csize;
    uint64_t cmult;
    uint32_t read_bl_len;
    uint32_t csd_v2;

    ret = sdmmc_send_cmd_long(priv, SD_CMD_SEND_CSD, priv->rca, csd);
    if (ret < 0)
      {
        syslog(LOG_ERR, "sdmmc%d: CMD9 CSD failed: %d\n", bus_num, ret);
        return ret;
      }

    csd_v2 = (csd[0] >> 30) & 0x3;   /* CSD structure bits 127:126 */

    if (csd_v2 == 0x1)
      {
        /* SDHC (CSD v2.0): per ST HAL,
         * DeviceSize = ((CSD[1] & 0x3F) << 16) |
         *               ((CSD[2] & 0xFFFF0000) >> 16)
         * BlockNbr = (DeviceSize + 1) * 1024 (512-byte blocks).
         */

        csize = ((uint64_t)(csd[1] & 0x3f) << 16) |
                ((uint64_t)(csd[2] & 0xffff0000) >> 16);
        priv->block_count = (csize + 1) * 1024ULL;
      }
    else
      {
        /* SDSC (CSD v1.0): per ST HAL,
         * DeviceSize = ((CSD[1] & 0x3FF) << 2) |
         *               ((CSD[2] & 0xC0000000) >> 30)
         * DeviceSizeMul = (CSD[2] & 0x38000) >> 15
         * BlockNbr = (DeviceSize+1) * 2^(DeviceSizeMul+2)
         * LogBlockNbr = BlockNbr * BlockSize / 512
         */

        read_bl_len = (csd[1] >> 16) & 0xf;   /* bits 83:80 */
        csize  = ((uint64_t)(csd[1] & 0x3ff) << 2) |
                 ((uint64_t)(csd[2] & 0xc0000000) >> 30);
        cmult  = ((uint64_t)(csd[2] & 0x38000) >> 15);
        priv->block_count =
          (csize + 1) * (1ULL << (cmult + 2)) * (1ULL << read_bl_len) /
          512ULL;
      }

    syslog(LOG_INFO,
           "sdmmc%d: card initialized, RCA=%08lx, %llu blocks (CSD v%lu)\n",
           bus_num, (unsigned long)priv->rca,
           (unsigned long long)priv->block_count, csd_v2);
  }

  /* CMD7: SELECT_CARD (expects R1) */

  ret = sdmmc_send_cmd(priv, 7, priv->rca, &resp);
  if (ret < 0)
    {
      syslog(LOG_ERR, "sdmmc%d: SELECT_CARD failed\n", bus_num);
      return ret;
    }

  /* CMD16: SET_BLOCKLEN to 512 (expects R1) */

  ret = sdmmc_send_cmd(priv, SD_CMD_SET_BLOCKLEN, 512, &resp);
  if (ret < 0)
    {
      syslog(LOG_ERR, "sdmmc%d: SET_BLOCKLEN failed\n", bus_num);
      return ret;
    }

  /* Switch to 4-bit bus and higher clock (done AFTER the CSD read, per
   * ST HAL order: HAL_SD_InitCard runs at 400kHz/1-bit and only then
   * does HAL_SD_ConfigWideBusOperation switch the bus width/speed).
   *
   * CRITICAL (SD spec + ST HAL SD_WideBus_Enable): the CARD must be told
   * to switch to 4-bit with CMD55->ACMD6(SET_BUS_WIDTH, arg=2) BEFORE the
   * controller WIDBUS bits are changed.  Without it the card still drives
   * DAT0 only (1-bit), no start bit is seen on the 4-bit bus and the data
   * phase times out (observed: DCOUNT frozen at 0x200, DTIMEOUT). */

  ret = sdmmc_send_cmd(priv, SD_CMD_APP_CMD, priv->rca, &resp);
  if (ret < 0)
    {
      syslog(LOG_ERR, "sdmmc%d: CMD55(APP_CMD) failed for wide-bus\n",
             bus_num);
      return ret;
    }

  ret = sdmmc_send_cmd(priv, SD_CMD_SWITCH, 2, &resp);  /* 2 = 4-bit */
  if (ret < 0)
    {
      syslog(LOG_ERR, "sdmmc%d: ACMD6(SET_BUS_WIDTH) failed\n", bus_num);
      return ret;
    }

  syslog(LOG_INFO, "sdmmc%d: card switched to 4-bit (ACMD6 ok)\n", bus_num);

  sdmmc_putreg(priv, SDMMC_CLKCR_OFFSET,
               (4 & SDMMC_CLKCR_CLKDIV_MASK) |   /* SDMMC_CK = 200M/(2*4)=25MHz */
               SDMMC_CLKCR_WIDBUS_4BIT |
               SDMMC_CLKCR_HWFC_EN);  /* RM0486 data xfer step 2: HWFC_EN (bare-metal 0x24004) */

  priv->initialized = true;

  /* ---- RIF grant for SDMMC1 (applied AFTER card init, per bare-metal ----
   * ---- 38_SD_Card order: HAL_SD_Init first, then RIF config)      ----
   * RISC: SDMMC1 = RIF_PERIPH_REG1 | SEC21 -> RISC_SECCFGRx[1] bit21
   *   (RIFSC+0x14), RISC_PRIVCFGRx[1] bit21 (RIFSC+0x34).
   * RIMC: SDMMC1 internal IDMA master (index 2) = RIF_CID_1(0x2) | SEC |
   *   PRIV -> RIMC_ATTRx[2] (RIFSC+0xC18).  The reference configures it
   *   with MasterCID=RIF_CID_1, so mirror that exactly.
   */

  putreg32(RCC_AHB3ENR_RIFSCEN, STM32_RCC_AHB3ENSR);  /* RIFSC clock on */

  /* Bare-metal verified value (tfcard dump): RIMC_ATTR[SDMMC1]=0x310
   * = MCID=1 | MSEC | MPRIV. */
  modifyreg32(STM32_RIFSC_BASE + 0xC10 + (2u << 2), 0,
               (1u << 4) | (1u << 8) | (1u << 9));  /* MCID=1|SEC|PRIV */
  modifyreg32(STM32_RIFSC_BASE + 0x14, 0, (1u << 21));  /* SEC21  */
  modifyreg32(STM32_RIFSC_BASE + 0x34, 0, (1u << 21));  /* PRIV21 */

  syslog(LOG_INFO,
         "sdmmc%d: RIF RIMC[SDMMC1]=0x%08lx RIFSEC=0x%08lx RIFPRIV=0x%08lx\n",
         bus_num,
         (unsigned long)getreg32(STM32_RIFSC_BASE + 0xC10 + (2u << 2)),
         (unsigned long)getreg32(STM32_RIFSC_BASE + 0x14),
         (unsigned long)getreg32(STM32_RIFSC_BASE + 0x34));

  /* Register block device so the VFS can mount FATFS on it. */

  ret = register_blockdriver(CONFIG_STM32N6_SDMMC_DEVPATH,
                             &g_stm32n6_sdmmc_bops, 0666, priv);
  if (ret < 0)
    {
      syslog(LOG_ERR, "sdmmc%d: register_blockdriver failed: %d\n",
             bus_num, ret);
      return ret;
    }

  priv->block_registered = true;

  return 0;
}

int stm32n6_sdmmc_deinitialize(int bus_num)
{
  struct stm32n6_sdmmc_priv_s *priv;

  switch (bus_num)
    {
      case 1:
        priv = &g_sdmmc1_priv;
        break;
      case 2:
        priv = &g_sdmmc2_priv;
        break;
      default:
        return -EINVAL;
    }

  if (!priv->initialized)
    {
      return 0;
    }

  /* Power off */

  sdmmc_putreg(priv, SDMMC_POWER_OFFSET, 0);

  priv->initialized = false;
  nxsem_destroy(&priv->lock);
  nxsem_destroy(&priv->wait);

  syslog(LOG_INFO, "sdmmc%d: deinitialized\n", bus_num);
  return 0;
}

/****************************************************************************
 * Block Device Operations
 *
 * Wraps the SDMMC raw block read/write behind the NuttX block driver
 * interface so FATFS can mount /dev/mmcsd0.  This is a pragmatic
 * alternative to a full struct sdio_dev_s port (which the upstream
 * stm32_sdmmc.c implements for other STM32 families); the SDMMC1
 * registers and the FIFO transfer helpers below are already verified
 * for STM32N6.
 ****************************************************************************/

static int sdmmc_block_open(FAR struct inode *inode)
{
  return OK;
}

static int sdmmc_block_close(FAR struct inode *inode)
{
  return OK;
}

static ssize_t sdmmc_block_read(FAR struct inode *inode,
                                FAR unsigned char *buffer,
                                blkcnt_t start_sector,
                                unsigned int nsectors)
{
  FAR struct stm32n6_sdmmc_priv_s *priv = inode->i_private;
  unsigned int i;
  int ret;

  for (i = 0; i < nsectors; i++)
    {
      ret = sdmmc_read_block(priv, (uint32_t)(start_sector + i),
                             buffer + (i * priv->block_size));
      if (ret < 0)
        {
          return -EIO;
        }
    }

  /* NuttX block driver contract: return the number of SECTORS read
   * (not bytes).  fat_hwread() compares the return value against
   * nsectors; returning bytes (512) made every read look short and
   * fatal_mount() returned -ENODEV. */

  return (ssize_t)nsectors;
}

static ssize_t sdmmc_block_write(FAR struct inode *inode,
                                 FAR const unsigned char *buffer,
                                 blkcnt_t start_sector,
                                 unsigned int nsectors)
{
  FAR struct stm32n6_sdmmc_priv_s *priv = inode->i_private;
  unsigned int i;
  int ret;

  for (i = 0; i < nsectors; i++)
    {
      ret = sdmmc_write_block(priv, (uint32_t)(start_sector + i),
                              buffer + (i * priv->block_size));
      if (ret < 0)
        {
          return -EIO;
        }
    }

  /* NuttX block driver contract: return the number of SECTORS written. */

  return (ssize_t)nsectors;
}

static int sdmmc_block_geometry(FAR struct inode *inode,
                                FAR struct geometry *geometry)
{
  FAR struct stm32n6_sdmmc_priv_s *priv = inode->i_private;

  if (geometry == NULL)
    {
      return -EINVAL;
    }

  geometry->geo_available = true;
  geometry->geo_mediachanged = false;
  geometry->geo_writeenabled = true;
  geometry->geo_sectorsize = priv->block_size;
  geometry->geo_nsectors = priv->block_count;

  return OK;
}

static const struct block_operations g_stm32n6_sdmmc_bops =
{
  .open     = sdmmc_block_open,
  .close    = sdmmc_block_close,
  .read     = sdmmc_block_read,
  .write    = sdmmc_block_write,
  .geometry = sdmmc_block_geometry,
};
