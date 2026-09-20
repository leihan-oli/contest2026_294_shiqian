/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_i2c.c
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
 * STM32N6 I2C driver for NuttX.
 * Supports I2C1-4 in master mode, interrupt-driven.
 *
 * Adapted from STM32H7 NuttX reference (stm32_i2c.c).
 * Reference: STM32N6 HAL (stm32n6xx_hal_i2c.c)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/kmalloc.h>
#include <nuttx/semaphore.h>
#include <nuttx/i2c/i2c_master.h>
#include <syslog.h>
#include <string.h>

#include "arm_internal.h"
#include "stm32n6_gpio.h"
#include "stm32n6_i2c.h"
#include "hardware/stm32_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* I2C instance count */

#define STM32N6_I2C_COUNT   4

/* I2C register base addresses (per CMSIS stm32n647xx.h) */

#define STM32N6_I2C1_BASE   0x40005400
#define STM32N6_I2C2_BASE   0x40005800
#define STM32N6_I2C3_BASE   0x40005C00
#define STM32N6_I2C4_BASE   0x46001C00

/* I2C register offsets (STM32N6 compatible with H7) */

#define I2C_CR1_OFFSET      0x00
#define I2C_CR2_OFFSET      0x04
#define I2C_OAR1_OFFSET     0x08
#define I2C_OAR2_OFFSET     0x0C
#define I2C_TIMINGR_OFFSET  0x10
#define I2C_TIMEOUTR_OFFSET 0x14
#define I2C_ISR_OFFSET      0x18
#define I2C_ICR_OFFSET      0x1C
#define I2C_PECR_OFFSET     0x20
#define I2C_RXDR_OFFSET     0x24
#define I2C_TXDR_OFFSET     0x28

/* I2C_CR1 bits */

#define I2C_CR1_PE          (1 << 0)
#define I2C_CR1_TXIE        (1 << 1)
#define I2C_CR1_RXIE        (1 << 2)
#define I2C_CR1_STOPIE      (1 << 5)
#define I2C_CR1_TCIE        (1 << 6)
#define I2C_CR1_ERRIE       (1 << 7)
#define I2C_CR1_ANFOFF      (1 << 12)
#define I2C_CR1_DNF_MASK    (0x0F << 8)
#define I2C_CR1_SWRST       (1 << 13)

/* I2C_CR2 bits */

#define I2C_CR2_SADD_MASK   (0x3FF << 0)
#define I2C_CR2_RD_WRN      (1 << 10)
#define I2C_CR2_START        (1 << 13)
#define I2C_CR2_STOP         (1 << 14)
#define I2C_CR2_NACK         (1 << 15)
#define I2C_CR2_NBYTES_MASK  (0xFF << 16)
#define I2C_CR2_RELOAD       (1 << 24)
#define I2C_CR2_AUTOEND      (1 << 25)

/* I2C_ISR bits */

#define I2C_ISR_TXE         (1 << 0)
#define I2C_ISR_TXIS        (1 << 1)
#define I2C_ISR_RXNE        (1 << 2)
#define I2C_ISR_ADDR        (1 << 3)
#define I2C_ISR_NACKF       (1 << 4)
#define I2C_ISR_STOPF       (1 << 5)
#define I2C_ISR_TC          (1 << 6)
#define I2C_ISR_TCR         (1 << 7)
#define I2C_ISR_BERR        (1 << 8)
#define I2C_ISR_ARLO        (1 << 9)
#define I2C_ISR_OVR         (1 << 10)
#define I2C_ISR_BUSY        (1 << 15)

/* I2C timing presets for different speeds */

#define I2C_TIMING_100KHZ   0x10C0F5B7  /* 100 kHz @ 64 MHz HSI */
#define I2C_TIMING_400KHZ   0x00E03D5A  /* 400 kHz @ 64 MHz HSI */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_i2c_config_s
{
  uint32_t base;          /* Peripheral register base */
  uint32_t clock;         /* Kernel clock frequency (Hz) */
  uint32_t clken_reg;     /* RCC enable register holding the clock bit */
  uint32_t clken_bit;     /* Peripheral clock-enable bit */
  uint32_t scl_cfg;       /* SCL pin GPIO cfgset (AF, open-drain) */
  uint32_t sda_cfg;       /* SDA pin GPIO cfgset (AF, open-drain) */
};

struct stm32n6_i2c_priv_s
{
  struct i2c_master_s dev;
  const struct stm32n6_i2c_config_s *config;
  sem_t   lock;
  sem_t   wait;
  uint32_t frequency;
  uint32_t status;
  int      error;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* GPIO cfgset for an I2C alternate-function pin: AF mode, open-drain (I2C is
 * a wired-AND bus), high speed, with the internal pull-up as a belt-and-
 * suspenders backup to the board's external pull-ups.
 */

#define I2C_PIN_CFG(port, pin, af) \
  (GPIO_MODE_AF | GPIO_OTYPE_OD | GPIO_SPEED_HIGH | GPIO_PUPD_PU | \
   GPIO_AF(af) | (port) | GPIO_PIN(pin))

/* I2C4 is the only bus wired on this board: PE13=SCL, PE14=SDA, AF4, clocked
 * from APB4ENR1.  These pins and AF match the ALIENTEK STM32N647 board (ST
 * SoftwarePackage BSP), which brings the onboard AP3216C sensor out on I2C4.
 * I2C1-3 keep correct clock bits but no pin config -- they are not brought
 * out on this board.
 */

static const struct stm32n6_i2c_config_s g_i2c1_config =
{
  .base      = STM32N6_I2C1_BASE,
  .clock     = 64000000,
  .clken_reg = STM32_RCC_APB1ENR1,
  .clken_bit = RCC_APB1ENR1_I2C1EN,
  .scl_cfg   = 0,
  .sda_cfg   = 0,
};

static const struct stm32n6_i2c_config_s g_i2c2_config =
{
  .base      = STM32N6_I2C2_BASE,
  .clock     = 64000000,
  .clken_reg = STM32_RCC_APB1ENR1,
  .clken_bit = RCC_APB1ENR1_I2C2EN,
  .scl_cfg   = I2C_PIN_CFG(GPIO_PORTD, 14, 4),   /* PD14=SCL (IMX335) */
  .sda_cfg   = I2C_PIN_CFG(GPIO_PORTD, 4, 4),    /* PD4=SDA (IMX335) */
};

static const struct stm32n6_i2c_config_s g_i2c3_config =
{
  .base      = STM32N6_I2C3_BASE,
  .clock     = 64000000,
  .clken_reg = STM32_RCC_APB1ENR1,
  .clken_bit = (1 << 23),          /* I2C3EN (APB1ENR1) */
  .scl_cfg   = 0,
  .sda_cfg   = 0,
};

static const struct stm32n6_i2c_config_s g_i2c4_config =
{
  .base      = STM32N6_I2C4_BASE,
  .clock     = 64000000,
  .clken_reg = STM32_RCC_APB4ENR1,
  .clken_bit = RCC_APB4ENR1_I2C4EN,
  .scl_cfg   = I2C_PIN_CFG(GPIO_PORTE, 13, 4),
  .sda_cfg   = I2C_PIN_CFG(GPIO_PORTE, 14, 4),
};

static struct stm32n6_i2c_priv_s g_i2c1_priv;
static struct stm32n6_i2c_priv_s g_i2c2_priv;
static struct stm32n6_i2c_priv_s g_i2c3_priv;
static struct stm32n6_i2c_priv_s g_i2c4_priv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline uint32_t stm32n6_i2c_getreg(
    struct stm32n6_i2c_priv_s *priv, uint32_t offset)
{
  return *(volatile uint32_t *)(priv->config->base + offset);
}

static inline void stm32n6_i2c_putreg(
    struct stm32n6_i2c_priv_s *priv, uint32_t offset,
    uint32_t value)
{
  *(volatile uint32_t *)(priv->config->base + offset) = value;
}

static int stm32n6_i2c_setfrequency(
    struct stm32n6_i2c_priv_s *priv, uint32_t frequency);

static int stm32n6_i2c_wait_isr(
    struct stm32n6_i2c_priv_s *priv, uint32_t mask,
    uint32_t *status)
{
  uint32_t timeout = 100000;

  while (timeout-- > 0)
    {
      *status = stm32n6_i2c_getreg(priv, I2C_ISR_OFFSET);
      if (*status & I2C_ISR_NACKF)
        {
          return -ENXIO;
        }

      if (*status & I2C_ISR_BERR)
        {
          priv->error = -EIO;
          return -EIO;
        }

      if (*status & I2C_ISR_ARLO)
        {
          priv->error = -EBUSY;
          return -EBUSY;
        }

      if (*status & mask)
        {
          return 0;
        }
    }

  priv->error = -ETIMEDOUT;
  return -ETIMEDOUT;
}

static int stm32n6_i2c_transfer(
    struct i2c_master_s *dev, struct i2c_msg_s *msgs,
    int count)
{
  struct stm32n6_i2c_priv_s *priv =
    (struct stm32n6_i2c_priv_s *)dev;
  int i;
  int ret;

  nxsem_wait(&priv->lock);

  for (i = 0; i < count; i++)
    {
      struct i2c_msg_s *msg = &msgs[i];
      uint32_t cr2;
      uint32_t status;
      uint32_t j;

      /* Configure address and direction */

      cr2 = (msg->addr << 1) & I2C_CR2_SADD_MASK;

      if (msg->flags & I2C_M_READ)
        {
          cr2 |= I2C_CR2_RD_WRN;
        }

      cr2 |= (msg->length << 16) & I2C_CR2_NBYTES_MASK;

      /* Auto-end if last message */

      if (i == count - 1)
        {
          cr2 |= I2C_CR2_AUTOEND;
        }

      cr2 |= I2C_CR2_START;

      stm32n6_i2c_putreg(priv, I2C_CR2_OFFSET, cr2);

      /* Transfer data */

      if (msg->flags & I2C_M_READ)
        {
          for (j = 0; j < msg->length; j++)
            {
              ret = stm32n6_i2c_wait_isr(priv,
                                          I2C_ISR_RXNE, &status);
              if (ret < 0)
                {
                  goto errout;
                }

              msg->buffer[j] =
                stm32n6_i2c_getreg(priv,
                                   I2C_RXDR_OFFSET) & 0xff;
            }
        }
      else
        {
          for (j = 0; j < msg->length; j++)
            {
              ret = stm32n6_i2c_wait_isr(priv,
                                          I2C_ISR_TXIS |
                                          I2C_ISR_TC,
                                          &status);
              if (ret < 0)
                {
                  goto errout;
                }

              stm32n6_i2c_putreg(priv, I2C_TXDR_OFFSET,
                                 msg->buffer[j]);
            }
        }

      if (i == count - 1)
        {
          /* Last message: AUTOEND generated a STOP -- wait for it. */

          ret = stm32n6_i2c_wait_isr(priv,
                                      I2C_ISR_STOPF,
                                      &status);
          if (ret < 0)
            {
              goto errout;
            }
        }
      else
        {
          /* More messages follow: with AUTOEND clear, the hardware sets TC
           * once NBYTES have moved.  A repeated START (written on the next
           * iteration) is only legal after TC, so wait for it here to avoid
           * a restart-timing race on the v2 I2C.
           */

          ret = stm32n6_i2c_wait_isr(priv, I2C_ISR_TC, &status);
          if (ret < 0)
            {
              goto errout;
            }
        }
    }

  nxsem_post(&priv->lock);
  return count;

errout:

  /* Recover the bus: clearing PE resets the peripheral state (and TIMINGR),
   * so reprogram the timing before re-enabling.
   */

  stm32n6_i2c_putreg(priv, I2C_CR1_OFFSET, 0);
  stm32n6_i2c_setfrequency(priv, priv->frequency);
  stm32n6_i2c_putreg(priv, I2C_CR1_OFFSET, I2C_CR1_PE);
  nxsem_post(&priv->lock);
  return priv->error;
}

static int stm32n6_i2c_setfrequency(
    struct stm32n6_i2c_priv_s *priv, uint32_t frequency)
{
  if (frequency <= 100000)
    {
      stm32n6_i2c_putreg(priv, I2C_TIMINGR_OFFSET,
                          I2C_TIMING_100KHZ);
    }
  else
    {
      stm32n6_i2c_putreg(priv, I2C_TIMINGR_OFFSET,
                          I2C_TIMING_400KHZ);
    }

  priv->frequency = frequency;
  return 0;
}

#ifdef CONFIG_I2C_RESET
static int stm32n6_i2c_reset(struct i2c_master_s *dev)
{
  struct stm32n6_i2c_priv_s *priv =
    (struct stm32n6_i2c_priv_s *)dev;

  /* On the v2 I2C, clearing PE is the software reset; it also clears
   * TIMINGR, so reprogram the timing (writable only while PE=0) before
   * re-enabling.
   */

  stm32n6_i2c_putreg(priv, I2C_CR1_OFFSET, 0);
  stm32n6_i2c_setfrequency(priv, priv->frequency);
  stm32n6_i2c_putreg(priv, I2C_CR1_OFFSET, I2C_CR1_PE);

  return OK;
}
#endif

static void stm32n6_i2c_init_priv(
    struct stm32n6_i2c_priv_s *priv,
    const struct stm32n6_i2c_config_s *config)
{
  priv->config = config;
  nxsem_init(&priv->lock, 0, 1);
  nxsem_init(&priv->wait, 0, 0);
  priv->dev.ops = NULL; /* Set by caller */
  priv->frequency = 100000;
  priv->error = 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

struct i2c_master_s *stm32n6_i2cbus_initialize(int bus_num)
{
  struct stm32n6_i2c_priv_s *priv;
  const struct stm32n6_i2c_config_s *config;
  static const struct i2c_ops_s g_i2c_ops =
  {
    .transfer = stm32n6_i2c_transfer,
#ifdef CONFIG_I2C_RESET
    .reset    = stm32n6_i2c_reset,
#endif
  };

  switch (bus_num)
    {
      case 1:
        priv = &g_i2c1_priv;
        config = &g_i2c1_config;
        break;
      case 2:
        priv = &g_i2c2_priv;
        config = &g_i2c2_config;
        break;
      case 3:
        priv = &g_i2c3_priv;
        config = &g_i2c3_config;
        break;
      case 4:
        priv = &g_i2c4_priv;
        config = &g_i2c4_config;
        break;
      default:
        syslog(LOG_ERR, "i2c: unsupported bus %d\n",
               bus_num);
        return NULL;
    }

  if (priv->config != NULL)
    {
      /* Already initialized */

      return &priv->dev;
    }

  stm32n6_i2c_init_priv(priv, config);
  priv->dev.ops = &g_i2c_ops;

  /* Bring-up order matters.  Pin every I2C instance to the HSI kernel
   * clock (64 MHz) so the TIMINGR presets are valid -- the reset default
   * (PCLK1 at APB1 speed) would make the SCL timing 1.5-3x too fast and
   * the bus unreliable.  Then enable the peripheral clock, mux the SCL/
   * SDA pins, and only then program TIMINGR -- which is writable only
   * while PE=0 -- before finally enabling the peripheral.
   */

  switch (config->base)
    {
      case STM32N6_I2C1_BASE:
        modifyreg32(STM32_RCC_CCIPR4, RCC_CCIPR4_I2C1SEL_MASK,
                    RCC_CCIPR4_I2C1SEL_HSI);
        break;

      case STM32N6_I2C2_BASE:
        modifyreg32(STM32_RCC_CCIPR4, RCC_CCIPR4_I2C2SEL_MASK,
                    RCC_CCIPR4_I2C2SEL_HSI);
        break;

      case STM32N6_I2C3_BASE:
        modifyreg32(STM32_RCC_CCIPR4, RCC_CCIPR4_I2C3SEL_MASK,
                    RCC_CCIPR4_I2C3SEL_HSI);
        break;

      case STM32N6_I2C4_BASE:
        modifyreg32(STM32_RCC_CCIPR4, RCC_CCIPR4_I2C4SEL_MASK,
                    RCC_CCIPR4_I2C4SEL_HSI);
        break;

      default:
        break;
    }

  modifyreg32(config->clken_reg, 0, config->clken_bit);

  if (config->scl_cfg != 0)
    {
      stm32n6_configgpio(config->scl_cfg);
      stm32n6_configgpio(config->sda_cfg);
    }

  /* PE=0 so TIMINGR is writable, program the timing, then enable. */

  stm32n6_i2c_putreg(priv, I2C_CR1_OFFSET, 0);
  stm32n6_i2c_setfrequency(priv, 400000);
  stm32n6_i2c_putreg(priv, I2C_CR1_OFFSET, I2C_CR1_PE);

  syslog(LOG_INFO, "i2c%d: initialized @ 400kHz\n", bus_num);
  return &priv->dev;
}

int stm32n6_i2cbus_uninitialize(struct i2c_master_s *dev)
{
  struct stm32n6_i2c_priv_s *priv =
    (struct stm32n6_i2c_priv_s *)dev;

  /* Disable I2C peripheral */

  stm32n6_i2c_putreg(priv, I2C_CR1_OFFSET, 0);

  priv->config = NULL;
  nxsem_destroy(&priv->lock);
  nxsem_destroy(&priv->wait);

  return OK;
}
