/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_imx335.c
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
 * STM32N6 IMX335 (Sony 5MP MIPI CSI-2) sensor driver.
 *
 * Ported from the ST BSP (SoftwarePackage/Drivers/BSP/IMX335/STM32_IMX335)
 * to NuttX:
 *   - Power-up sequence: PWDN=PG6, RST=PG4
 *   - I2C2 (PD4/PD14) with 16-bit register addressing, 7-bit addr 0x34
 *   - 2592x1944 RAW10 register tables + 37 MHz input clock + streaming
 *
 * The IMX335 is a streaming slave: once configured it continuously emits
 * RAW10 frames on MIPI CSI-2 (2 data lanes) without further host action.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/i2c/i2c_master.h>
#include <nuttx/arch.h>
#include <nuttx/clock.h>

#include "stm32n6_gpio.h"
#include "stm32n6_i2c.h"
#include "stm32n6_imx335.h"

#ifdef CONFIG_STM32_IMX335

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* IMX335 power/reset pins (ST BSP): PWDN=PG6, RST=PG4 */

#define IMX335_PWDN_GPIO   (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | \
                            GPIO_SPEED_HIGH | GPIO_PUPD_NONE | \
                            GPIO_PORTG | GPIO_PIN(6))
#define IMX335_RST_GPIO    (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | \
                            GPIO_SPEED_HIGH | GPIO_PUPD_NONE | \
                            GPIO_PORTG | GPIO_PIN(4))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct regval
{
  uint16_t addr;
  uint8_t  val;
};

/* 16-bit value registers (e.g. IMX335 HS-timing TCLK_ZERO @0x3a1e) */

struct regval16
{
  uint16_t addr;
  uint16_t val;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* IMX335 2592x1944 resolution register table (ST BSP res_2592_1944_regs) */

static const struct regval g_res_2592_1944[] =
{
  {0x3000, 0x01},
  {0x3002, 0x00},
  {0x300c, 0x3b},
  {0x300d, 0x2a},
  {0x3018, 0x04},
  {0x302c, 0x3c},
  {0x302e, 0x20},
  {0x3056, 0x98},
  {0x3074, 0xc8},
  {0x3076, 0x30},
  {0x304c, 0x00},
  {0x314c, 0xc6},
  {0x315a, 0x02},
  {0x3168, 0xa0},
  {0x316a, 0x7e},
  {0x31a1, 0x00},
  {0x3288, 0x21},
  {0x328a, 0x02},
  {0x3414, 0x05},
  {0x3416, 0x18},
  {0x3648, 0x01},
  {0x364a, 0x04},
  {0x364c, 0x04},
  {0x3678, 0x01},
  {0x367c, 0x31},
  {0x367e, 0x31},
  {0x3706, 0x10},
  {0x3708, 0x03},
  {0x3714, 0x02},
  {0x3715, 0x02},
  {0x3716, 0x01},
  {0x3717, 0x03},
  {0x371c, 0x3d},
  {0x371d, 0x3f},
  {0x372c, 0x00},
  {0x372d, 0x00},
  {0x372e, 0x46},
  {0x372f, 0x00},
  {0x3730, 0x89},
  {0x3731, 0x00},
  {0x3732, 0x08},
  {0x3733, 0x01},
  {0x3734, 0xfe},
  {0x3735, 0x05},
  {0x3740, 0x02},
  {0x375d, 0x00},
  {0x375e, 0x00},
  {0x375f, 0x11},
  {0x3760, 0x01},
  {0x3768, 0x1b},
  {0x3769, 0x1b},
  {0x376a, 0x1b},
  {0x376b, 0x1b},
  {0x376c, 0x1a},
  {0x376d, 0x17},
  {0x376e, 0x0f},
  {0x3776, 0x00},
  {0x3777, 0x00},
  {0x3778, 0x46},
  {0x3779, 0x00},
  {0x377a, 0x89},
  {0x377b, 0x00},
  {0x377c, 0x08},
  {0x377d, 0x01},
  {0x377e, 0x23},
  {0x377f, 0x02},
  {0x3780, 0xd9},
  {0x3781, 0x03},
  {0x3782, 0xf5},
  {0x3783, 0x06},
  {0x3784, 0xa5},
  {0x3788, 0x0f},
  {0x378a, 0xd9},
  {0x378b, 0x03},
  {0x378c, 0xeb},
  {0x378d, 0x05},
  {0x378e, 0x87},
  {0x378f, 0x06},
  {0x3790, 0xf5},
  {0x3792, 0x43},
  {0x3794, 0x7a},
  {0x3796, 0xa1},
  {0x37b0, 0x36},
  {0x3a00, 0x01},
};

/* IMX335 2-lane 10-bit mode register table (ST BSP mode_2l_10b_regs) */

static const struct regval g_mode_2l_10b[] =
{
  {0x3050, 0x00},
  {0x319d, 0x00},
  {0x341c, 0xff},
  {0x341d, 0x01},
  {0x3a01, 0x01},
};

/* IMX335 37 MHz input clock register table (ST BSP inck_37Mhz_regs) */

static const struct regval g_inck_37mhz[] =
{
  {0x300c, 0x5b},
  {0x300d, 0x40},
  {0x314c, 0x80},
  {0x314d, 0x00},
  {0x315a, 0x02},
  {0x3168, 0x68},
  {0x316a, 0x7e},
};

/* IMX335 MIPI D-PHY HS transmit-timing registers (0x3a18-0x3a28) for
 * 1188 Mbps per-lane data rate.  Values taken from the Linux mainline
 * imx335 driver (drivers/media/i2c/imx335.c, mipi_data_rate_1188Mbps).
 * These timings depend only on the per-lane bit rate (1188 Mbps, from the
 * 37.125 MHz INCK PLL), not on the host OS.  NOTE: the rest of the Linux
 * table (BCWAIT/CPWAIT/INCKSEL1-4) corresponds to a 24 MHz INCK and MUST
 * NOT be copied - the ST BSP INCK37 table above is authoritative here.
 */

static const struct regval16 g_hs_timing_tclk[] =
{
  {0x3a18, 0x008f},   /* TCLK-POST */
  {0x3a1a, 0x004f},   /* TCLK-PREPARE */
  {0x3a1c, 0x0047},   /* TCLK-TRAIL */
  {0x3a1e, 0x0137},   /* TCLK-ZERO (16-bit) */
};

static const struct regval16 g_hs_timing_ths[] =
{
  {0x3a20, 0x004f},   /* THS-PREPARE */
  {0x3a22, 0x0087},   /* THS-ZERO */
  {0x3a24, 0x004f},   /* THS-TRAIL */
  {0x3a26, 0x007f},   /* THS-EXIT */
  {0x3a28, 0x003f},   /* T-LPX */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int imx335_write_reg(FAR struct i2c_master_s *i2c, uint16_t reg,
                            uint8_t val);
static int imx335_write_table(FAR struct i2c_master_s *i2c,
                              FAR const struct regval *regs,
                              uint32_t size);
static int imx335_write_reg16(FAR struct i2c_master_s *i2c, uint16_t reg,
                             uint16_t val);
static int imx335_write_table16(FAR struct i2c_master_s *i2c,
                                FAR const struct regval16 *regs,
                                uint32_t size);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct i2c_master_s *g_i2c;

/* Runtime tuning overrides set through the helpers exported below.
 *
 * Every capture session pulses PWDN/RST and re-programs the register
 * tables, which resets whatever the operator tuned with imx335_tune.
 * stm32n6_imx335_configure() therefore re-applies any non-zero override
 * at its end, so the settings survive cam_save runs.  Zero means "keep
 * whatever the register tables programmed".
 */

static uint32_t g_tune_gain;   /* GAIN register value, 0.3 dB per step */
static uint32_t g_tune_shr0;   /* SHR0 integration length, in lines    */
static uint32_t g_tune_vmax;   /* VMAX frame length, in lines          */

/* configure() calls this to write the overrides above to the sensor */

int stm32n6_imx335_apply_tuning(void);

/* Runtime-selectable MIPI HS transmit-timing mode (see
 * stm32n6_imx335_set_hs_mode): 0=off, 1=TCLK only, 2=THS only, 3=full.
 */

/* MIPI HS transmit-timing mode for start_stream():
 *   0 = do not write HS timing regs (sensor defaults - matches bare-metal)
 *   1 = clock-lane group, 2 = data-lane group, 3 = both
 * Default 0: the HS timing values came from the Linux imx335 driver and
 * the bare-metal ST BSP (IMX335_SetFrequency) never programs them;
 * writing them disturbs the MIPI output timing (CSI ECC/SOT errors).
 */

static int g_imx335_hs_mode = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: imx335_write_reg
 *
 * Description:
 *   Write one byte to an IMX335 register (16-bit register address).
 *
 ****************************************************************************/

static int imx335_write_reg(FAR struct i2c_master_s *i2c, uint16_t reg,
                            uint8_t val)
{
  struct i2c_msg_s msg;
  uint8_t buf[3];

  buf[0] = (reg >> 8) & 0xff;
  buf[1] = reg & 0xff;
  buf[2] = val;

  msg.frequency = 100000;
  msg.addr      = IMX335_I2C_ADDR;
  msg.flags     = 0;
  msg.buffer    = buf;
  msg.length    = 3;

  return I2C_TRANSFER(i2c, &msg, 1);
}

/****************************************************************************
 * Name: imx335_write_table
 *
 * Description:
 *   Write a register value table to the IMX335.
 *
 ****************************************************************************/

static int imx335_write_table(FAR struct i2c_master_s *i2c,
                              FAR const struct regval *regs,
                              uint32_t size)
{
  uint32_t i;
  int ret;

  for (i = 0; i < size; i++)
    {
      ret = imx335_write_reg(i2c, regs[i].addr, regs[i].val);
      if (ret < 0)
        {
          _err("imx335: write reg 0x%04x failed: %d\n", regs[i].addr, ret);
          return ret;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: imx335_write_reg16
 *
 * Description:
 *   Write a 16-bit value to an IMX335 register (16-bit register address,
 *   little-endian 2-byte data, as used by the Sony MIPI HS-timing block).
 *
 ****************************************************************************/

static int imx335_write_reg16(FAR struct i2c_master_s *i2c, uint16_t reg,
                              uint16_t val)
{
  struct i2c_msg_s msg;
  uint8_t buf[4];

  buf[0] = (reg >> 8) & 0xff;
  buf[1] = reg & 0xff;
  buf[2] = val & 0xff;
  buf[3] = (val >> 8) & 0xff;

  msg.frequency = 100000;
  msg.addr      = IMX335_I2C_ADDR;
  msg.flags     = 0;
  msg.buffer    = buf;
  msg.length    = 4;

  return I2C_TRANSFER(i2c, &msg, 1);
}

/****************************************************************************
 * Name: imx335_write_table16
 *
 * Description:
 *   Write a 16-bit register value table to the IMX335.
 *
 ****************************************************************************/

static int imx335_write_table16(FAR struct i2c_master_s *i2c,
                                FAR const struct regval16 *regs,
                                uint32_t size)
{
  uint32_t i;
  int ret;

  for (i = 0; i < size; i++)
    {
      ret = imx335_write_reg16(i2c, regs[i].addr, regs[i].val);
      if (ret < 0)
        {
          _err("imx335: write16 reg 0x%04x failed: %d\n", regs[i].addr, ret);
          return ret;
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_imx335_power_on
 *
 ****************************************************************************/

/* TEMPORARY PROBE (perf 2026): the camera start sequence costs 2620 ms per
 * frame and this function alone accounts for 1620 ms, yet its body asks for
 * only 200 + 3 ms.  Print every step with both the tick time and the DWT
 * cycle count; cycles/ms/1000 gives the real CPU frequency, which in turn
 * tells us whether CONFIG_BOARD_LOOPSPERMSEC is calibrated for this clock.
 * Remove once the delay source is identified. */

#define IMX335_DWT_CYCCNT 0xe0001004u
#define imx335_cyccnt()   (*(FAR volatile uint32_t *)IMX335_DWT_CYCCNT)

static void imx335_probe(FAR const char *name, FAR clock_t *t0,
                         FAR uint32_t *c0)
{
  clock_t  now = clock_systime_ticks();
  uint32_t c1  = imx335_cyccnt();
  uint32_t ms  = (uint32_t)TICK2MSEC(now - *t0);
  uint32_t cyc = c1 - *c0;

  _info("imx335 step: %-12s %5lu ms %10lu cyc %s%lu MHz\n", name,
        (unsigned long)ms, (unsigned long)cyc, ms > 0 ? "" : "  ",
        ms > 0 ? (unsigned long)(cyc / ms / 1000) : 0);

  *t0 = now;
  *c0 = c1;
}

int stm32n6_imx335_power_on(void)
{
  /* PWDN high, RST low, wait 200 ms, then RST high */

  clock_t  t0 = clock_systime_ticks();
  uint32_t c0 = imx335_cyccnt();

  stm32n6_configgpio(IMX335_RST_GPIO);
  imx335_probe("cfg RST", &t0, &c0);

  stm32n6_configgpio(IMX335_PWDN_GPIO);
  imx335_probe("cfg PWDN", &t0, &c0);

  stm32n6_gpiowrite(IMX335_PWDN_GPIO, true);
  imx335_probe("wr PWDN=1", &t0, &c0);

  stm32n6_gpiowrite(IMX335_RST_GPIO, false);
  imx335_probe("wr RST=0", &t0, &c0);

  up_mdelay(200);
  imx335_probe("mdelay200", &t0, &c0);

  stm32n6_gpiowrite(IMX335_RST_GPIO, true);
  imx335_probe("wr RST=1", &t0, &c0);

  up_mdelay(3);
  imx335_probe("mdelay3", &t0, &c0);

  _info("imx335: power-on done (PWDN=PG6 RST=PG4)\n");

  return OK;
}

/****************************************************************************
 * Name: stm32n6_imx335_init
 *
 ****************************************************************************/

int stm32n6_imx335_configure(void)
{
  uint8_t id;
  int ret;

  /* Open I2C2 (PD4=SDA / PD14=SCL, AF4) */

  g_i2c = stm32n6_i2cbus_initialize(2);
  if (g_i2c == NULL)
    {
      _err("imx335: I2C2 init failed\n");
      return -ENODEV;
    }

  /* Check the sensor ID */

  ret = stm32n6_imx335_read_reg(IMX335_REG_ID, &id);
  if (ret < 0)
    {
      _err("imx335: read ID failed: %d\n", ret);
      return ret;
    }

  if (id != IMX335_ID)
    {
      _err("imx335: unexpected ID 0x%02x (expected 0x%02x)\n",
           id, IMX335_ID);
      return -ENODEV;
    }

  _info("imx335: ID=0x%02x OK\n", id);

  /* Program the resolution table */

  ret = imx335_write_table(g_i2c, g_res_2592_1944,
                           sizeof(g_res_2592_1944) /
                           sizeof(g_res_2592_1944[0]));
  if (ret < 0)
    {
      return ret;
    }

  /* Program the 2-lane 10-bit mode */

  ret = imx335_write_table(g_i2c, g_mode_2l_10b,
                           sizeof(g_mode_2l_10b) /
                           sizeof(g_mode_2l_10b[0]));
  if (ret < 0)
    {
      return ret;
    }

  /* --- Readback verification: confirm the I2C writes took effect ---
   * (before streaming -- I2C hangs once the sensor streams for a while)
   */

  static const uint16_t verify_regs[] =
  {
    0x300c, 0x3a00, 0x3a01, 0x3912
  };

  static const uint8_t expect[] =
  {
    0x3b, 0x01, 0x01, 0x00
  };

  uint8_t v;
  int i;

  for (i = 0; i < (int)(sizeof(verify_regs) / sizeof(verify_regs[0])); i++)
    {
      ret = stm32n6_imx335_read_reg(verify_regs[i], &v);
      if (ret < 0)
        {
          _err("imx335: readback reg 0x%04x failed: %d\n",
               verify_regs[i], ret);
        }
      else
        {
          _info("imx335: readback 0x%04x=0x%02x (expect 0x%02x)%s\n",
                verify_regs[i], v, expect[i],
                (v == expect[i]) ? " OK" : "  <<< MISMATCH");
        }
    }

  /* Default analogue gain.  The ST BSP register tables never program the
   * gain, so the sensor comes out of reset at 0 dB and every capture was
   * severely underexposed (measured mean luma 26/255).  24 dB was measured
   * as the best setting for the eye-tracking captures (mean 103/255 with no
   * highlight clipping).  imx335_tune can still override this at run time -
   * the override is re-applied immediately below.
   */

  ret = stm32n6_imx335_write_reg(0x30e8, 80);   /* 24 dB = 80 x 0.3 dB */

  if (ret < 0)
    {
      _err("imx335: default gain write failed: %d\n", ret);
    }

  /* Re-apply any runtime exposure/gain override (see g_tune_* above).
   *
   * The operator tunes these with imx335_tune and they would otherwise be
   * lost every time a capture session pulses PWDN/RST and reloads the
   * register tables.
   */

  if (stm32n6_imx335_apply_tuning() < 0)
    {
      _err("imx335: applying tuning overrides failed\n");
    }

  _info("imx335: configured (2592x1944 RAW10, 2-lane)\n");
  return OK;
}

/****************************************************************************
 * Name: stm32n6_imx335_set_gain / set_exposure / set_vmax / apply_tuning
 *
 * Description:
 *   Tuning helpers used by apps/examples/imx335_tune.  The setters only
 *   record the requested value; apply_tuning() writes the recorded values
 *   to the sensor and is called from stm32n6_imx335_configure(), so the
 *   settings survive the reset that each capture run performs.
 *
 ****************************************************************************/

void stm32n6_imx335_set_gain(uint32_t gain)
{
  g_tune_gain = gain;
}

void stm32n6_imx335_set_exposure(uint32_t shr0)
{
  g_tune_shr0 = shr0;
}

void stm32n6_imx335_set_vmax(uint32_t vmax)
{
  g_tune_vmax = vmax;
}

int stm32n6_imx335_apply_tuning(void)
{
  static const uint16_t gain_reg[2] =
  {
    0x30e8, 0x30e9
  };

  static const uint16_t shr0_reg[3] =
  {
    0x3058, 0x3059, 0x305a
  };

  static const uint16_t vmax_reg[3] =
  {
    0x3030, 0x3031, 0x3032
  };

  uint32_t val;
  int i;
  int ret;

  if (g_i2c == NULL)
    {
      return -ENODEV;
    }

  if (g_tune_vmax != 0)
    {
      for (i = 0, val = g_tune_vmax; i < 3; i++, val >>= 8)
        {
          ret = stm32n6_imx335_write_reg(vmax_reg[i],
                                          (uint8_t)(val & 0xff));
          if (ret < 0)
            {
              return ret;
            }
        }
    }

  if (g_tune_shr0 != 0)
    {
      for (i = 0, val = g_tune_shr0; i < 3; i++, val >>= 8)
        {
          ret = stm32n6_imx335_write_reg(shr0_reg[i],
                                          (uint8_t)(val & 0xff));
          if (ret < 0)
            {
              return ret;
            }
        }
    }

  if (g_tune_gain != 0)
    {
      /* IMX335 keeps the whole 0..72 dB range in the single byte at
       * gain_reg[0] (0x30e8, 0.3 dB per step - see IMX335_REG_GAIN in the
       * ST BSP).  The previous two-byte sequence split the value across the
       * wrong registers, so every configure() reset the gain back to 0 dB.
       */

      ret = stm32n6_imx335_write_reg(gain_reg[0], (uint8_t)g_tune_gain);
      if (ret < 0)
        {
          return ret;
        }
    }

  if (g_tune_gain != 0 || g_tune_shr0 != 0 || g_tune_vmax != 0)
    {
      _info("imx335: tuning reapplied (gain=0x%03lx shr0=%lu vmax=%lu)\n",
            (unsigned long)g_tune_gain, (unsigned long)g_tune_shr0,
            (unsigned long)g_tune_vmax);
    }

  return OK;
}

int stm32n6_imx335_start_stream(void)
{
  int ret;

  /* Start streaming (mode select = streaming) - the ALIENTEK IMX335_Init
   * writes the mode select BEFORE the input-clock (INCK) table; the INCK
   * PLL programming only takes effect once the sensor is out of standby.
   */

  ret = imx335_write_reg(g_i2c, IMX335_REG_MODE_SELECT,
                            IMX335_MODE_STREAMING);
  if (ret < 0)
    {
      return ret;
    }

  up_mdelay(20);

  /* Write the input-clock (INCK) PLL table.  The bare-metal ALIENTEK
   * demo calls IMX335_SetFrequency(IMX335_INCK_37MHZ) after the mode
   * select, programming the sensor PLL for a 37.125 MHz input clock
   * (0x300c=0x5B, 0x300d=0x40).  A previous appli.hex disassembly
   * mis-read SetFrequency(5) as an empty 6 MHz table; 5 is actually
   * IMX335_INCK_37MHZ (INCK_6MHZ is 0), so the INCK table MUST be
   * written for the MIPI output bitrate to match the DCMIPP PHY.
   */

  ret = imx335_write_table(g_i2c, g_inck_37mhz,
                           sizeof(g_inck_37mhz) /
                           sizeof(g_inck_37mhz[0]));
  if (ret < 0)
    {
      return ret;
    }

  if (g_imx335_hs_mode == 1 || g_imx335_hs_mode == 3)
    {
      ret = imx335_write_table16(g_i2c, g_hs_timing_tclk,
                                 sizeof(g_hs_timing_tclk) /
                                 sizeof(g_hs_timing_tclk[0]));
      if (ret < 0)
        {
          return ret;
        }
    }

  if (g_imx335_hs_mode == 2 || g_imx335_hs_mode == 3)
    {
      ret = imx335_write_table16(g_i2c, g_hs_timing_ths,
                                 sizeof(g_hs_timing_ths) /
                                 sizeof(g_hs_timing_ths[0]));
      if (ret < 0)
        {
          return ret;
        }
    }

  up_mdelay(20);

  /* DIAG (2026-08-25): verify the INCK PLL table actually took effect.
   * If the sensor PLL is not programmed, the MIPI output rate will not
   * match the DCMIPP PHY and the link never syncs (CRC errors).
   */

  uint8_t v;

  if (stm32n6_imx335_read_reg(0x300c, &v) == 0)
    {
      _info("imx335: INCK readback 0x300c=0x%02x (expect 0x5b)\n", v);
    }

  if (stm32n6_imx335_read_reg(0x300d, &v) == 0)
    {
      _info("imx335: INCK readback 0x300d=0x%02x (expect 0x40)\n", v);
    }

  if (stm32n6_imx335_read_reg(0x314c, &v) == 0)
    {
      _info("imx335: INCK readback 0x314c=0x%02x (expect 0x80)\n", v);
    }

  _info("imx335: streaming started"
        " (INCK 37MHz + HS timing mode %d written)\n", g_imx335_hs_mode);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_imx335_set_hs_mode
 *
 * Description:
 *   Select the MIPI D-PHY HS transmit-timing mode for the next
 *   stm32n6_imx335_start_stream():
 *     0 = do not write any HS timing registers (sensor defaults,
 *         matches bare-metal diag9f; this is the default)
 *     1 = write clock-lane group only (0x3a18-0x3a1e)
 *     2 = write data-lane group only (0x3a20-0x3a28)
 *     3 = write both groups
 *
 ****************************************************************************/

void stm32n6_imx335_set_hs_mode(int mode)
{
  g_imx335_hs_mode = mode;
}

int stm32n6_imx335_get_hs_mode(void)
{
  return g_imx335_hs_mode;
}

/****************************************************************************
 * Name: stm32n6_imx335_read_reg
 *
 * Description:
 *   Read one byte from an IMX335 register (16-bit register address).
 *   Requires the sensor to have been initialized (I2C bus open).
 *
 ****************************************************************************/

int stm32n6_imx335_read_reg(uint16_t reg, FAR uint8_t *val)
{
  struct i2c_msg_s msg;
  uint8_t regbuf[2];
  int ret;

  if (g_i2c == NULL)
    {
      return -ENODEV;
    }

  regbuf[0] = (reg >> 8) & 0xff;
  regbuf[1] = reg & 0xff;

  /* Write the register address with a STOP (separate transfer), then read
   * the data with a fresh START.  The IMX335 on this board does not
   * tolerate a repeated-START read (NACK / garbage), so split the two
   * phases into independent transfers.
   */

  msg.frequency = 100000;
  msg.addr      = IMX335_I2C_ADDR;
  msg.flags     = 0;
  msg.buffer    = regbuf;
  msg.length    = 2;

  ret = I2C_TRANSFER(g_i2c, &msg, 1);
  if (ret < 0)
    {
      _err("imx335: read reg 0x%04x addr wr failed: %d\n", reg, ret);
      return ret;
    }

  msg.flags     = I2C_M_READ;
  msg.buffer    = val;
  msg.length    = 1;

  ret = I2C_TRANSFER(g_i2c, &msg, 1);
  if (ret < 0)
    {
      _err("imx335: read reg 0x%04x data rd failed: %d\n", reg, ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_imx335_write_reg
 *
 * Description:
 *   Write one byte to an IMX335 register (16-bit register address).
 *   Requires the sensor to have been initialized (I2C bus open).
 *
 ****************************************************************************/

int stm32n6_imx335_write_reg(uint16_t reg, uint8_t val)
{
  struct i2c_msg_s msg;
  uint8_t buf[3];
  int ret;

  if (g_i2c == NULL)
    {
      return -ENODEV;
    }

  buf[0] = (reg >> 8) & 0xff;
  buf[1] = reg & 0xff;
  buf[2] = val;

  msg.frequency = 100000;
  msg.addr      = IMX335_I2C_ADDR;
  msg.flags     = 0;
  msg.buffer    = buf;
  msg.length    = 3;

  ret = I2C_TRANSFER(g_i2c, &msg, 1);
  if (ret < 0)
    {
      _err("imx335: write reg 0x%04x failed: %d\n", reg, ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_imx335_read_reg_8bit
 *
 * Description:
 *   Diagnostic: read one byte using 8-bit register addressing (single
 *   address byte).  Used to determine whether the IMX335 on this board
 *   uses 8-bit or 16-bit register addressing.
 *
 ****************************************************************************/

int stm32n6_imx335_read_reg_8bit(uint8_t reg, FAR uint8_t *val)
{
  struct i2c_msg_s msg[2];
  uint8_t regbuf[1];
  int ret;

  if (g_i2c == NULL)
    {
      return -ENODEV;
    }

  regbuf[0] = reg;

  msg[0].frequency = 100000;
  msg[0].addr      = IMX335_I2C_ADDR;
  msg[0].flags     = 0;
  msg[0].buffer    = regbuf;
  msg[0].length    = 1;

  msg[1].frequency = 100000;
  msg[1].addr      = IMX335_I2C_ADDR;
  msg[1].flags     = I2C_M_READ;
  msg[1].buffer    = val;
  msg[1].length    = 1;

  ret = I2C_TRANSFER(g_i2c, msg, 2);
  if (ret < 0)
    {
      _err("imx335: 8-bit read reg 0x%02x failed: %d\n", reg, ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_imx335_write_reg_8bit
 *
 * Description:
 *   Diagnostic: write one byte using 8-bit register addressing (single
 *   address byte + data byte).
 *
 ****************************************************************************/

int stm32n6_imx335_write_reg_8bit(uint8_t reg, uint8_t val)
{
  struct i2c_msg_s msg;
  uint8_t buf[2];
  int ret;

  if (g_i2c == NULL)
    {
      return -ENODEV;
    }

  buf[0] = reg;
  buf[1] = val;

  msg.frequency = 100000;
  msg.addr      = IMX335_I2C_ADDR;
  msg.flags     = 0;
  msg.buffer    = buf;
  msg.length    = 2;

  ret = I2C_TRANSFER(g_i2c, &msg, 1);
  if (ret < 0)
    {
      _err("imx335: 8-bit write reg 0x%02x failed: %d\n", reg, ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_imx335_i2c_scan
 *
 * Description:
 *   Diagnostic: scan the I2C2 bus (7-bit addresses 0x08..0x77) and report
 *   which devices ACK.  Determines whether the IMX335 (0x34) is really
 *   connected to the bus.
 *
 ****************************************************************************/

int stm32n6_imx335_i2c_scan(void)
{
  struct i2c_msg_s msg;
  uint8_t dummy;
  int addr;
  int found;

  if (g_i2c == NULL)
    {
      return -ENODEV;
    }

  found = 0;

  for (addr = 0x08; addr < 0x78; addr++)
    {
      dummy = 0;

      msg.frequency = 100000;
      msg.addr      = addr;
      msg.flags     = 0;
      msg.buffer    = &dummy;
      msg.length    = 1;

      if (I2C_TRANSFER(g_i2c, &msg, 1) == 1)
        {
          _info("imx335: I2C device found at 0x%02x\n", addr);
          found++;
        }
    }

  _info("imx335: I2C scan done, %d device(s) found\n", found);

  return found;
}

/****************************************************************************
 * Name: stm32n6_imx335_read_reg_at
 *
 * Description:
 *   Diagnostic: read one byte from an IMX335-style register using an
 *   arbitrary 7-bit I2C address.  Used to probe which address the IMX335
 *   really answers on (0x34 vs 0x1a, per the I2C bus scan).
 *
 ****************************************************************************/

int stm32n6_imx335_read_reg_at(uint16_t i2c_addr, uint16_t reg,
                               FAR uint8_t *val)
{
  struct i2c_msg_s msg[2];
  uint8_t regbuf[2];
  int ret;

  if (g_i2c == NULL)
    {
      return -ENODEV;
    }

  regbuf[0] = (reg >> 8) & 0xff;
  regbuf[1] = reg & 0xff;

  msg[0].frequency = 100000;
  msg[0].addr      = i2c_addr;
  msg[0].flags     = 0;
  msg[0].buffer    = regbuf;
  msg[0].length    = 2;

  msg[1].frequency = 100000;
  msg[1].addr      = i2c_addr;
  msg[1].flags     = I2C_M_READ;
  msg[1].buffer    = val;
  msg[1].length    = 1;

  ret = I2C_TRANSFER(g_i2c, msg, 2);
  return ret;
}

#endif /* CONFIG_STM32_IMX335 */
