/****************************************************************************
 * apps/examples/imx335_tune/imx335_tune_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * IMX335 exposure / gain tuning helper for dataset capture.
 *
 * Background: the IMX335 has no built-in auto exposure, and the register
 * tables used by the NuttX driver are the ST BSP tables, which do NOT
 * program GAIN at all.  The power-on default is therefore 0 dB with SHR0
 * at its minimum (i.e. the longest exposure the current VMAX allows).
 * Captured frames came out badly under-exposed (mean luma ~ 12/255,
 * 89% of pixels below 16), which makes them useless as training data.
 *
 * This command lets the operator sweep gain and exposure from NSH instead
 * of re-flashing the board for every register experiment.
 *
 * Register map (IMX335 datasheet):
 *   0x3058  SHR0[7:0]      integration time = 1 frame period - SHR0 * 1H
 *   0x3059  SHR0[15:8]     smaller SHR0 = longer exposure (minimum 9)
 *   0x305a  SHR0[19:16]
 *   0x3030  VMAX[7:0]      number of lines per frame; a larger VMAX lowers
 *   0x3031  VMAX[15:8]     the frame rate and thereby extends the maximum
 *   0x3032  VMAX[19:16]    exposure time (default 0x1194 = 4500 lines)
 *   0x30e8  GAIN[7:0]      analog (then digital) gain, 0.3 dB per step
 *   0x30e9  GAIN[10:8]     register value = dB / 0.3 (0x00 = 0 dB)
 *
 * Usage:
 *   imx335_tune dump                 read back gain/exposure/VMAX
 *   imx335_tune gain <db>            set gain, 0..72 dB
 *   imx335_tune expos <lines>        set SHR0 directly (9 .. VMAX-1)
 *   imx335_tune fps <fps>            set VMAX for the wanted frame rate
 *   imx335_tune vmax <lines>         set VMAX directly
 *   imx335_tune read <addr>          read one byte  (debug)
 *   imx335_tune write <addr> <val>   write one byte (debug)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IMX335_REG_VMAX_L    0x3030
#define IMX335_REG_VMAX_M    0x3031
#define IMX335_REG_VMAX_H    0x3032
#define IMX335_REG_SHR0_L    0x3058
#define IMX335_REG_SHR0_M    0x3059
#define IMX335_REG_SHR0_H    0x305a
#define IMX335_REG_GAIN_L    0x30e8
#define IMX335_REG_GAIN_H    0x30e9

#define IMX335_SHR0_MIN      9
#define IMX335_GAIN_MAX_DB   72

/* Nominal 1H period at the 37.125 MHz INCK used by this board:
 * frame period = VMAX * 1H, so VMAX = 134972 / fps (4500 lines @ 30 fps).
 */

#define IMX335_VMAX_PER_FPS  134972

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* The sensor is reached through the IMX335 driver, which keeps the I2C
 * master handle private.  Its register accessors are exported and are used
 * here directly; this app lives in the same flat address space.
 */

extern int stm32n6_imx335_write_reg(uint16_t reg, uint8_t val);
extern int stm32n6_imx335_read_reg(uint16_t reg, FAR uint8_t *val);
extern int stm32n6_imx335_power_on(void);
extern int stm32n6_imx335_configure(void);
extern void stm32n6_imx335_set_gain(uint32_t gain);
extern void stm32n6_imx335_set_exposure(uint32_t shr0);
extern void stm32n6_imx335_set_vmax(uint32_t vmax);

static int tune_read24(uint16_t reg_l, uint32_t *val)
{
  uint8_t b[3];
  int ret;

  ret = stm32n6_imx335_read_reg(reg_l, &b[0]);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_imx335_read_reg(reg_l + 1, &b[1]);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_imx335_read_reg(reg_l + 2, &b[2]);
  if (ret < 0)
    {
      return ret;
    }

  *val = (uint32_t)b[0] | ((uint32_t)b[1] << 8) |
         (((uint32_t)b[2] & 0x0f) << 16);
  return 0;
}

static int tune_write24(uint16_t reg_l, uint32_t val)
{
  int ret;

  ret = stm32n6_imx335_write_reg(reg_l, val & 0xff);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_imx335_write_reg(reg_l + 1, (val >> 8) & 0xff);
  if (ret < 0)
    {
      return ret;
    }

  return stm32n6_imx335_write_reg(reg_l + 2, (val >> 16) & 0x0f);
}

/* Make sure the sensor answers on I2C, powering it up if needed. */

static int tune_probe(void)
{
  uint8_t v;
  int ret;

  ret = stm32n6_imx335_read_reg(IMX335_REG_GAIN_L, &v);
  if (ret == -ENODEV)
    {
      printf("imx335_tune: sensor not up, powering on...\n");
      ret = stm32n6_imx335_power_on();
      if (ret < 0)
        {
          printf("imx335_tune: power-on failed: %d\n", ret);
          return ret;
        }

      /* configure() opens I2C2 and programs the sensor.  Without it the
       * register accessors have no bus handle and return -ENODEV, which is
       * what made the first version of this tool unusable.
       */

      ret = stm32n6_imx335_configure();
      if (ret < 0)
        {
          printf("imx335_tune: configure failed: %d\n", ret);
          return ret;
        }

      ret = stm32n6_imx335_read_reg(IMX335_REG_GAIN_L, &v);
    }

  if (ret < 0)
    {
      printf("imx335_tune: sensor does not answer (err=%d)\n", ret);
      printf("imx335_tune: hint - run 'cam_save 1 probe' once to bring "
             "the camera up\n");
    }

  return ret;
}

static void tune_dump(void)
{
  uint32_t gain;
  uint32_t shr0;
  uint32_t vmax;
  uint32_t lines;
  uint32_t ms10;
  uint32_t fps10;
  uint32_t db10;

  if (tune_read24(IMX335_REG_GAIN_L, &gain) < 0 ||
      tune_read24(IMX335_REG_SHR0_L, &shr0) < 0 ||
      tune_read24(IMX335_REG_VMAX_L, &vmax) < 0)
    {
      printf("imx335_tune: register read failed\n");
      return;
    }

  if (vmax == 0)
    {
      vmax = 1;
    }

  db10 = gain * 3u;

  printf("imx335: GAIN = 0x%03lx -> %lu.%lu dB\n",
         (unsigned long)gain,
         (unsigned long)(db10 / 10u), (unsigned long)(db10 % 10u));
  lines = vmax - shr0;
  ms10  = (lines * 741u + 5000u) / 10000u;

  printf("imx335: SHR0 = 0x%05lx -> integration %lu lines "
         "(%lu.%lu ms nominal)\n",
         (unsigned long)shr0, (unsigned long)lines,
         (unsigned long)(ms10 / 10u), (unsigned long)(ms10 % 10u));

  fps10 = (1349720u + vmax / 2u) / vmax;

  printf("imx335: VMAX = 0x%05lx -> %lu lines, %lu.%lu fps nominal\n",
         (unsigned long)vmax, (unsigned long)vmax,
         (unsigned long)(fps10 / 10u), (unsigned long)(fps10 % 10u));
  printf("imx335: note: nominal values assume 1H = 7.41us (37.125 MHz "
         "INCK)\n");
}

static int tune_set_gain(int db)
{
  uint32_t val;
  int ret;

  if (db < 0 || db > IMX335_GAIN_MAX_DB)
    {
      printf("imx335_tune: gain must be 0..%d dB\n", IMX335_GAIN_MAX_DB);
      return -EINVAL;
    }

  /* GAIN[10:0]: value = dB / 0.3 dB.  Write the high byte first so that
   * the 11-bit value is latched when the low byte lands.
   */

  val = ((uint32_t)db * 10u) / 3u;

  /* The whole 0..72 dB range lives in the single byte at 0x30e8 */

  ret = stm32n6_imx335_write_reg(IMX335_REG_GAIN_L, val & 0xff);

  if (ret < 0)
    {
      printf("imx335_tune: gain write failed: %d\n", ret);
      return ret;
    }

  /* Record it so that every later capture session re-applies it: cam_save
   * pulses PWDN/RST and reloads the register tables on each run.
   */

  stm32n6_imx335_set_gain(val);

  printf("imx335_tune: gain = %d dB (reg 0x%03lx) - takes effect on the "
         "next frame, re-applied after each power-on\n",
         db, (unsigned long)val);
  return 0;
}

static int tune_set_shr0(uint32_t lines)
{
  uint32_t vmax;
  uint32_t expo;
  uint32_t ms10;
  int ret;

  if (tune_read24(IMX335_REG_VMAX_L, &vmax) < 0)
    {
      printf("imx335_tune: cannot read VMAX\n");
      return -EIO;
    }

  if (lines < IMX335_SHR0_MIN || lines >= vmax)
    {
      printf("imx335_tune: SHR0 must be %d..%ld (VMAX-1) lines\n",
             IMX335_SHR0_MIN, (unsigned long)(vmax - 1));
      return -EINVAL;
    }

  ret = tune_write24(IMX335_REG_SHR0_L, lines);
  if (ret < 0)
    {
      printf("imx335_tune: SHR0 write failed: %d\n", ret);
      return ret;
    }

  stm32n6_imx335_set_exposure(lines);

  expo = vmax - lines;
  ms10 = (expo * 741u + 5000u) / 10000u;

  printf("imx335_tune: SHR0 = %lu lines -> integration %lu lines "
         "(%lu.%lu ms nominal), re-applied after each power-on\n",
         (unsigned long)lines, (unsigned long)expo,
         (unsigned long)(ms10 / 10u), (unsigned long)(ms10 % 10u));
  return 0;
}

static int tune_set_vmax(uint32_t lines)
{
  uint32_t shr0;
  uint32_t fps10;
  uint32_t ms10;
  int ret;

  if (lines < 64 || lines > 0xfffff)
    {
      printf("imx335_tune: VMAX out of range (64..1048575)\n");
      return -EINVAL;
    }

  if (tune_read24(IMX335_REG_SHR0_L, &shr0) == 0 && shr0 >= lines)
    {
      /* Keep the constraint SHR0 <= VMAX - 1 valid, shrink SHR0 first. */

      printf("imx335_tune: clamping SHR0 to %lu (VMAX-1)\n",
             (unsigned long)(lines - 1));
      if (tune_write24(IMX335_REG_SHR0_L, lines - 1) < 0)
        {
          printf("imx335_tune: SHR0 write failed\n");
          return -EIO;
        }
    }

  ret = tune_write24(IMX335_REG_VMAX_L, lines);
  if (ret < 0)
    {
      printf("imx335_tune: VMAX write failed: %d\n", ret);
      return ret;
    }

  stm32n6_imx335_set_vmax(lines);

  fps10 = (1349720u + lines / 2u) / lines;
  ms10  = (lines * 741u + 5000u) / 10000u;

  printf("imx335_tune: VMAX = %lu lines -> %lu.%lu fps nominal, "
         "max exposure %lu.%lu ms, re-applied after each power-on\n",
         (unsigned long)lines,
         (unsigned long)(fps10 / 10u), (unsigned long)(fps10 % 10u),
         (unsigned long)(ms10 / 10u), (unsigned long)(ms10 % 10u));
  return 0;
}

static void tune_usage(void)
{
  printf("Usage: imx335_tune <command> [args]\n"
         "  dump                 read back gain / SHR0 / VMAX\n"
         "  gain <db>            set analog+digital gain (0..72 dB)\n"
         "  expos <lines>        set SHR0 directly (9 .. VMAX-1)\n"
         "  fps <fps>            set VMAX for the wanted frame rate\n"
         "  vmax <lines>         set VMAX directly\n"
         "  read <addr>          read one byte\n"
         "  write <addr> <val>   write one byte\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc < 2)
    {
      tune_usage();
      return EXIT_FAILURE;
    }

  if (tune_probe() < 0)
    {
      return EXIT_FAILURE;
    }

  if (strcmp(argv[1], "dump") == 0)
    {
      tune_dump();
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "gain") == 0 && argc >= 3)
    {
      return tune_set_gain(atoi(argv[2])) < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "expos") == 0 && argc >= 3)
    {
      return tune_set_shr0((uint32_t)strtoul(argv[2], NULL, 0)) < 0 ?
             EXIT_FAILURE : EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "fps") == 0 && argc >= 3)
    {
      uint32_t fps = (uint32_t)strtoul(argv[2], NULL, 0);

      if (fps == 0 || fps > 120)
        {
          printf("imx335_tune: fps must be 1..120\n");
          return EXIT_FAILURE;
        }

      return tune_set_vmax(IMX335_VMAX_PER_FPS / fps) < 0 ?
             EXIT_FAILURE : EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "vmax") == 0 && argc >= 3)
    {
      return tune_set_vmax((uint32_t)strtoul(argv[2], NULL, 0)) < 0 ?
             EXIT_FAILURE : EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "read") == 0 && argc >= 3)
    {
      uint16_t reg = (uint16_t)strtoul(argv[2], NULL, 16);
      uint8_t val = 0;

      if (stm32n6_imx335_read_reg(reg, &val) < 0)
        {
          printf("imx335_tune: read 0x%04x failed\n", reg);
          return EXIT_FAILURE;
        }

      printf("imx335: [0x%04x] = 0x%02x\n", reg, val);
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "write") == 0 && argc >= 4)
    {
      uint16_t reg = (uint16_t)strtoul(argv[2], NULL, 16);
      uint8_t val = (uint8_t)strtoul(argv[3], NULL, 0);

      if (stm32n6_imx335_write_reg(reg, val) < 0)
        {
          printf("imx335_tune: write 0x%04x failed\n", reg);
          return EXIT_FAILURE;
        }

      printf("imx335: [0x%04x] <- 0x%02x\n", reg, val);
      return EXIT_SUCCESS;
    }

  tune_usage();
  return EXIT_FAILURE;
}
