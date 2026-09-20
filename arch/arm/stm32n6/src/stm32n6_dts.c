/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_dts.c
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
#include <stdbool.h>
#include <errno.h>
#include <debug.h>

#include <fixedmath.h>

#include <nuttx/kmalloc.h>
#include <nuttx/mutex.h>
#include <nuttx/fs/fs.h>
#include <nuttx/arch.h>
#include <nuttx/clock.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "stm32n6_dts.h"
#include "hardware/stm32_dts.h"
#include "hardware/stm32_rcc.h"

#ifdef CONFIG_STM32_DTS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Temperature conversion (DS14791 / ST HAL formula):
 *
 *   Temp(C) = G + H * (sample/Cal5 - 0.5) + J * Fclk_ts
 *           = 60 + 200 * (sample/4094 - 0.5) + (-0.1) * 4
 *           = 200 * sample / 4094 - 40.4
 *
 * G/H/J/Cal5 are fixed characterization constants (no OTP calibration).
 * The kernel is built without an FPU (CONFIG_ARCH_FPU unset), so the
 * result is produced in b16_t (Q16.16) fixed point with 64-bit integer
 * math instead of float:
 *
 *   temp_b16 = 13107200 * sample / 4094 - 2647654
 *
 * where 13107200 = 200 * 65536 and 2647654 ~= 40.4 * 65536.
 */

#define DTS_CONV_NUM    13107200LL  /* 200 * 65536 */
#define DTS_CONV_CAL5   4094LL      /* Cal5 parameter */
#define DTS_CONV_OFFSET 2647654LL   /* 40.4 * 65536 (rounded) */

/* SDIF programming and sample-ready polling timeouts (microseconds).  The
 * SDA interface and a continuous-mode conversion both complete well within
 * a millisecond at the 4 MHz TS clock; 1 s mirrors the ST HAL guard.
 */

#define DTS_SDIF_TIMEOUT_US  1000000
#define DTS_SAMPLE_TIMEOUT_US 1000000

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_dts_dev_s
{
  mutex_t lock;      /* Serializes access to the SDIF and sample registers */
  bool    started;   /* True once the sensor is configured and running */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int  dts_program_sda(uint32_t reg, uint32_t value);
static int  dts_readtemp(FAR struct stm32n6_dts_dev_s *priv,
                         FAR b16_t *temp);
static ssize_t dts_read(FAR struct file *filep, FAR char *buffer,
                        size_t buflen);
static ssize_t dts_write(FAR struct file *filep, FAR const char *buffer,
                         size_t buflen);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_dts_fops =
{
  NULL,        /* open */
  NULL,        /* close */
  dts_read,    /* read */
  dts_write,   /* write */
  NULL,        /* seek */
  NULL,        /* ioctl */
};

static struct stm32n6_dts_dev_s g_dts_dev =
{
  NXMUTEX_INITIALIZER,
  false,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dts_program_sda
 *
 * Description:
 *   Program one of the sensor's serial-data (SDA) registers through the
 *   SDIF interface.  Waits for the interface to be idle, inhibits the
 *   other sensor, then issues a write request.  Sensor 0 is the only
 *   sensor used by this port.
 *
 ****************************************************************************/

static int dts_program_sda(uint32_t reg, uint32_t value)
{
  clock_t start;

  /* Wait until the serial data interface is no longer busy. */

  start = clock_systime_ticks();
  while ((getreg32(STM32_DTS_TSCSDIF_SR) & DTS_TSCSDIF_SR_SDIF_BUSY) != 0)
    {
      if (TICK2USEC(clock_systime_ticks() - start) > DTS_SDIF_TIMEOUT_US)
        {
          snerr("ERROR: SDIF busy timeout\n");
          return -ETIMEDOUT;
        }
    }

  /* Inhibit serial programming of sensor 1 so this request targets
   * sensor 0 only.
   */

  putreg32(DTS_TSCSDIF_CFGR_INHIBIT_1, STM32_DTS_TSCSDIF_CFGR);

  /* Issue the write request: program + write-no-read + register + value. */

  putreg32(DTS_TSCSDIF_CR_SDIF_PROG | DTS_TSCSDIF_CR_SDIF_WRN | reg | value,
           STM32_DTS_TSCSDIF_CR);

  return OK;
}

/****************************************************************************
 * Name: dts_readtemp
 *
 * Description:
 *   Poll sensor 0 for a completed conversion, read the raw sample, reject
 *   invalid/fault samples, and convert to a b16_t temperature in Celsius.
 *
 ****************************************************************************/

static int dts_readtemp(FAR struct stm32n6_dts_dev_s *priv, FAR b16_t *temp)
{
  clock_t start;
  uint32_t sample;
  int64_t value;

  /* Wait for a completed conversion. */

  start = clock_systime_ticks();
  while ((getreg32(STM32_DTS_S0_DONER) &
          DTS_DONER_SMPL_DONE) == 0)
    {
      if (TICK2USEC(clock_systime_ticks() - start) > DTS_SAMPLE_TIMEOUT_US)
        {
          snerr("ERROR: sample-ready timeout\n");
          return -ETIMEDOUT;
        }
    }

  sample = getreg32(STM32_DTS_S0_DATAR);

  /* Reject samples flagged as the wrong type or as faulted. */

  if ((sample & (DTS_DATAR_SAMPLE_TYPE |
                 DTS_DATAR_SAMPLE_FAULT)) != 0)
    {
      snerr("ERROR: invalid/fault sample 0x%08" PRIx32 "\n", sample);
      return -EIO;
    }

  sample &= DTS_DATAR_SAMPLE_DATA_MASK;

  /* temp_b16 = 13107200 * sample / 4094 - 2647654 (see header comment). */

  value = (DTS_CONV_NUM * (int64_t)sample) / DTS_CONV_CAL5 -
          DTS_CONV_OFFSET;

  *temp = (b16_t)value;
  return OK;
}

/****************************************************************************
 * Name: dts_read
 ****************************************************************************/

static ssize_t dts_read(FAR struct file *filep, FAR char *buffer,
                        size_t buflen)
{
  FAR struct inode *inode = filep->f_inode;
  FAR struct stm32n6_dts_dev_s *priv = inode->i_private;
  FAR b16_t *ptr;
  ssize_t nsamples;
  int i;
  int ret;

  nsamples = buflen / sizeof(b16_t);
  ptr      = (FAR b16_t *)buffer;

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  for (i = 0; i < nsamples; i++)
    {
      b16_t temp = 0;

      ret = dts_readtemp(priv, &temp);
      if (ret < 0)
        {
          nxmutex_unlock(&priv->lock);
          return (ssize_t)ret;
        }

      *ptr++ = temp;
    }

  nxmutex_unlock(&priv->lock);
  return nsamples * sizeof(b16_t);
}

/****************************************************************************
 * Name: dts_write
 ****************************************************************************/

static ssize_t dts_write(FAR struct file *filep, FAR const char *buffer,
                         size_t buflen)
{
  return -ENOSYS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_dts_initialize
 *
 * Description:
 *   Enable the DTS clock, program the TS clock synthesizer to 4 MHz,
 *   configure sensor 0 for continuous 12-bit acquisition, start it, and
 *   register the character device at the given path (e.g. "/dev/temp0").
 *   Reading a b16_t from the device returns the die junction temperature
 *   in Celsius.
 *
 ****************************************************************************/

int stm32n6_dts_initialize(FAR const char *devpath)
{
  FAR struct stm32n6_dts_dev_s *priv = &g_dts_dev;
  int ret;

  /* Enable the DTS peripheral clock (APB4ENR2 DTSEN). */

  modifyreg32(STM32_RCC_APB4ENR2, 0, RCC_APB4ENR2_DTSEN);

  /* Configure the TS clock synthesizer.  The DTS kernel clock is
   * hsi_div8_ck = HSI/8 = 8 MHz; with CLK_SYNTH_HI = CLK_SYNTH_LO = 0 the
   * TS clock is 4 MHz, the frequency the temperature formula assumes.
   */

  putreg32(DTS_TSCCLKSYNTHR_CLK_SYNTH_EN | DTS_TSCCLKSYNTHR_CLK_SYNTH_HOLD,
           STM32_DTS_TSCCLKSYNTHR);

  /* Program the typical sensor power-up delay for sensor 0. */

  ret = dts_program_sda(DTS_SDA_TIMERR_REG, DTS_SDA_POWER_UP_DELAY);
  if (ret < 0)
    {
      goto err;
    }

  /* Enable the TS interrupt source in the PVT block (required for the
   * sample-done flag to update even in polled use).
   */

  putreg32(DTS_PVT_IER_TS_IRQ_ENABLE, STM32_DTS_PVT_IER);

  /* Undo any sensor-0 SDIF disable left from reset. */

  modifyreg32(STM32_DTS_TSCSDIFDIS, DTS_TSCSDIFDIS_TS0_DISABLE, 0);

  /* Configure sensor 0 for continuous acquisition at 12-bit resolution. */

  ret = dts_program_sda(DTS_SDA_CFGR_REG, DTS_SDA_RESOLUTION_12BITS);
  if (ret < 0)
    {
      goto err;
    }

  /* Reset the min/max sample trackers before starting. */

  putreg32(DTS_HILORESETR_SMPL_LO_SET | DTS_HILORESETR_SMPL_HI_CLR,
           STM32_DTS_S0_HILORESETR);

  /* Start sensor 0 in continuous mode. */

  ret = dts_program_sda(DTS_SDA_CR_REG, DTS_SDA_MODE_CONTINUOUS);
  if (ret < 0)
    {
      goto err;
    }

  priv->started = true;

  ret = register_driver(devpath, &g_dts_fops, 0444, priv);
  if (ret < 0)
    {
      snerr("ERROR: register_driver %s failed: %d\n", devpath, ret);
      goto err;
    }

  sninfo("DTS registered at %s\n", devpath);
  return OK;

err:

  /* Power the sensor back down and gate the clock on any failure. */

  dts_program_sda(DTS_SDA_CR_REG, DTS_SDA_SENSOR_POWER_DOWN);
  modifyreg32(STM32_RCC_APB4ENR2, RCC_APB4ENR2_DTSEN, 0);
  priv->started = false;
  return ret;
}

#endif /* CONFIG_STM32_DTS */
