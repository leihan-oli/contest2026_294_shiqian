/****************************************************************************
 * apps/examples/cam/cam_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * IMX335 camera live-view demo (Phase 6, ALIENTEK STM32N647 board).
 *
 * Replicates the bare-metal 352_IMX335 example: IMX335 -> MIPI CSI-2
 * (2 lanes) -> DCMIPP PIPE1 (RAW10 -> RGB565 800x480, hardware ISP) ->
 * LTDC framebuffer (/dev/fb0), displayed live on the 7" RGB-LCD.
 *
 * Usage (NSH):
 *   cam start   - power the IMX335, init DCMIPP, start continuous capture
 *   cam stop    - stop the capture
 *   cam status  - show the captured frame count
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>

#include <nuttx/video/fb.h>
#include <nuttx/arch.h>
#include <nuttx/leds/userled.h>

/****************************************************************************
 * Public Function Prototypes
 *
 * Arch-level camera drivers (stm32n6_dcmipp.c / stm32n6_imx335.c).
 ****************************************************************************/

int stm32n6_dcmipp_init(void);
int stm32n6_dcmipp_start_capture(uint32_t dstaddr);
int stm32n6_dcmipp_wait_first_frame(void);
int stm32n6_dcmipp_stop(void);
uint32_t stm32n6_dcmipp_get_frame_count(void);
uint32_t stm32n6_dcmipp_get_csi_sr0(void);
uint32_t stm32n6_dcmipp_get_csi_sr1(void);
uint32_t stm32n6_dcmipp_get_p1fctcr(void);
uint32_t stm32n6_dcmipp_get_cmsr1(void);
uint32_t stm32n6_dcmipp_get_cmsr2(void);
void stm32n6_dcmipp_dump_status(void);
uint32_t stm32n6_dcmipp_get_error_code(void);
uint32_t stm32n6_dcmipp_get_overrun_count(void);
void stm32n6_dcmipp_clear_csi_flags(uint32_t sr0_mask, uint32_t sr1_mask);
int stm32n6_dcmipp_recover_vc0(void);

int stm32n6_imx335_power_on(void);
int stm32n6_gt9xxx_scan(FAR int *x, FAR int *y, FAR bool *pressed);
int stm32n6_imx335_configure(void);
int stm32n6_imx335_start_stream(void);
int stm32n6_imx335_read_reg(uint16_t reg, FAR uint8_t *val);
int stm32n6_imx335_write_reg(uint16_t reg, uint8_t val);
void stm32n6_imx335_set_hs_mode(int mode);
int stm32n6_imx335_get_hs_mode(void);
void stm32n6_dcmipp_set_rx_mode(int mode);
int stm32n6_dcmipp_get_rx_mode(void);
int stm32n6_dcmipp_set_phy_bitrate(int mbps);

/* Diagnostic helpers (this file) */

static void cam_led_all(uint32_t mask);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cam_get_framebuffer
 *
 * Description:
 *   Return the /dev/fb0 framebuffer base address (where DCMIPP will write
 *   the live RGB565 frames) via FBIOGET_PLANEINFO.
 *
 ****************************************************************************/

static int cam_get_framebuffer(FAR uint32_t *fbaddr)
{
  struct fb_planeinfo_s pinfo;
  int fd;
  int ret;

  fd = open("/dev/fb0", O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "cam: open /dev/fb0 failed: %d\n", errno);
      return -errno;
    }

  memset(&pinfo, 0, sizeof(pinfo));
  ret = ioctl(fd, FBIOGET_PLANEINFO, (unsigned long)(uintptr_t)&pinfo);
  close(fd);

  if (ret < 0)
    {
      fprintf(stderr, "cam: FBIOGET_PLANEINFO failed: %d\n", errno);
      return -errno;
    }

  *fbaddr = (uint32_t)(uintptr_t)pinfo.fbmem;
  return OK;
}

/****************************************************************************
 * Name: cam_start
 *
 * Description:
 *   Full camera start sequence (mirrors the bare-metal 352_IMX335 main):
 *     1. IMX335 power-up (PWDN/RST)
 *     2. DCMIPP + CSI-2 configuration (PIPE1 RAW10 -> RGB565 800x480)
 *     3. IMX335 register programming + streaming
 *     4. Start PIPE1 continuous capture into the live framebuffer
 *
 ****************************************************************************/

/****************************************************************************
 * Name: cam_led_all
 *
 * Description:
 *   Drive the two board LEDs (/dev/userleds, LED0=PG10 bit0, LED1=PE10
 *   bit1).  Best effort: if the node is missing we silently skip.
 *
 ****************************************************************************/

static void cam_led_all(uint32_t mask)
{
  static int fd = -1;

  if (fd < 0)
    {
      fd = open("/dev/userleds", O_WRONLY);
    }

  if (fd >= 0)
    {
      (void)ioctl(fd, ULEDIOC_SETALL, (unsigned long)mask);
    }
}

static int cam_start(void)
{
  uint32_t fbaddr;
  int ret;

  ret = cam_get_framebuffer(&fbaddr);
  if (ret < 0)
    {
      return ret;
    }

  printf("cam: framebuffer @ 0x%08lx\n", (unsigned long)fbaddr);

  /* 1. IMX335 power-up (PWDN/RST GPIO) */

  cam_led_all(0x01);                 /* LED0 on: power-on stage */
  printf("cam: [LED stage] power-on\n");
  ret = stm32n6_imx335_power_on();
  if (ret < 0)
    {
      fprintf(stderr, "cam: IMX335 power-on failed: %d\n", -ret);
      cam_led_all(0x03);             /* both on = error */
      return ret;
    }

  /* 2. DCMIPP + CSI-2 configuration (D-PHY, PIPE1) BEFORE the sensor is
   *    I2C-configured - matching the bare-metal diag9f order
   *    (imx335_dcmipp_init runs before IMX335_Init). */

  cam_led_all(0x02);                 /* LED1 on: DCMIPP init stage */
  printf("cam: [LED stage] dcmipp init\n");
  ret = stm32n6_dcmipp_init();
  if (ret < 0)
    {
      fprintf(stderr, "cam: DCMIPP init failed: %d\n", -ret);
      cam_led_all(0x03);
      return ret;
    }

  /* 3. IMX335 I2C configuration (register tables, no streaming yet) */

  cam_led_all(0x03);                 /* LED0+LED1: configure stage */
  printf("cam: [LED stage] configure (sensor now configured)\n");
  ret = stm32n6_imx335_configure();
  if (ret < 0)
    {
      fprintf(stderr, "cam: IMX335 configure failed: %d\n", -ret);
      cam_led_all(0x03);
      return ret;
    }

  /* 4. EXPERIMENT I: CAPTURE-FIRST - arm VC0 + PIPE1 BEFORE the sensor
   *    streams.  The earlier P0 experiment with this order first moved
   *    data (frames=1); with the INCK 37 MHz config now correct, this
   *    lets the D-PHY lock onto the signal as it arrives instead of
   *    sitting idle for a long time and then failing to sync. */

  cam_led_all(0x03);                 /* both on: capture stage */
  printf("cam: [LED stage] capture-first (arm VC0 + PIPE1)\n");
  ret = stm32n6_dcmipp_start_capture(fbaddr);
  if (ret < 0)
    {
      fprintf(stderr, "cam: capture start failed: %d\n", -ret);
      cam_led_all(0x03);
      return ret;
    }

  /* 5. Start the sensor streaming (0x3000=0 + INCK 37 MHz PLL). */

  printf("cam: [LED stage] start stream (sensor streaming)\n");
  ret = stm32n6_imx335_start_stream();
  if (ret < 0)
    {
      fprintf(stderr, "cam: IMX335 start stream failed: %d\n", -ret);
      cam_led_all(0x03);
      return ret;
    }

  /* 6. Wait for VC0 active + first SOF. */

  ret = stm32n6_dcmipp_wait_first_frame();
  if (ret < 0)
    {
      fprintf(stderr, "cam: first frame lock failed: %d\n", -ret);
      cam_led_all(0x03);
      return ret;
    }

  printf("cam: live camera view started (800x480 RGB565)\n");

  return OK;
}

/****************************************************************************
 * Name: cam_stop
 *
 ****************************************************************************/

static int cam_stop(void)
{
  int ret;

  ret = stm32n6_dcmipp_stop();
  if (ret < 0)
    {
      fprintf(stderr, "cam: DCMIPP stop failed: %d\n", -ret);
      return ret;
    }

  printf("cam: capture stopped\n");

  return OK;
}

/****************************************************************************
 * Name: cam_sample_csi
 *
 * Description:
 *   Sample the CSI-2 SR0 status register 30 times (1 ms apart) and count
 *   how many times SOF0F / EOF0F / VC0STATEF are set.  This tells whether
 *   the sensor is streaming continuously without touching I2C (which hangs
 *   once the sensor has been streaming).
 *
 ****************************************************************************/

static void cam_sample_csi(void)
{
  int n;
  int sofs = 0;
  int vc0s = 0;
  int eofs = 0;
  int sync0 = 0;
  int sync1 = 0;
  int act0 = 0;
  int act1 = 0;

  for (n = 0; n < 30; n++)
    {
      uint32_t sr0 = stm32n6_dcmipp_get_csi_sr0();
      uint32_t sr1 = stm32n6_dcmipp_get_csi_sr1();

      if ((sr0 & 0x00000100) != 0)   /* CSI_SR0_SOF0F */
        {
          sofs++;
        }

      if ((sr0 & 0x00020000) != 0)   /* CSI_SR0_VC0STATEF */
        {
          vc0s++;
        }

      if ((sr0 & 0x00001000) != 0)   /* CSI_SR0_EOF0F */
        {
          eofs++;
        }

      if ((sr1 & 0x00020000) != 0)   /* CSI_SR1_SYNCDL0F */
        {
          sync0++;
        }

      if ((sr1 & 0x00800000) != 0)   /* CSI_SR1_SYNCDL1F */
        {
          sync1++;
        }

      if ((sr1 & 0x00010000) != 0)   /* CSI_SR1_ACTDL0F */
        {
          act0++;
        }

      if ((sr1 & 0x00400000) != 0)   /* CSI_SR1_ACTDL1F */
        {
          act1++;
        }

      up_udelay(1000);
    }

  printf("cam: CSI 30x: SOF=%d EOF=%d VC0=%d | "
         "SYNC0=%d SYNC1=%d ACT0=%d ACT1=%d\n",
         sofs, eofs, vc0s, sync0, sync1, act0, act1);
}

/****************************************************************************
 * Name: cam_status
 *
 ****************************************************************************/

static void cam_status(void)
{
  uint32_t fbaddr;
  FAR uint16_t *fb;

  printf("cam: frames captured: %lu\n",
         (unsigned long)stm32n6_dcmipp_get_frame_count());
  printf("cam: pipe1 overruns: %lu\n",
         (unsigned long)stm32n6_dcmipp_get_overrun_count());

  /* Read the first framebuffer pixels to verify PIPE1 actually wrote.
   * 0xf800 = the fbcolor red fill would still be present if PIPE1 never
   * wrote to the live buffer.
   */

  if (cam_get_framebuffer(&fbaddr) == 0)
    {
      int row;
      int col;
      int nz;

      fb = (FAR uint16_t *)(uintptr_t)fbaddr;

      /* Multi-row / multi-column sample: how much of the framebuffer did
       * PIPE1 actually write?  A "one-line streak" on the LCD means only a
       * few rows are non-zero (partial write); a full 800x480 capture
       * shows non-zero across all sampled rows. */

      printf("cam: fb[0x%08lx] sample (row,col: px):\n", (unsigned long)fbaddr);
      for (row = 0; row < 480; row += 100)
        {
          printf("cam:   row %3d col0-7:", row);
          nz = 0;
          for (col = 0; col < 8; col++)
            {
              printf(" %04x", fb[row * 800 + col]);
              if (fb[row * 800 + col] != 0) nz++;
            }

          printf(" | x200:%04x x400:%04x x600:%04x (nz=%d)\n",
                 fb[row * 800 + 200], fb[row * 800 + 400],
                 fb[row * 800 + 600], nz);
        }

      /* Row 479 (last) as well */

      printf("cam:   row 479:");
      for (col = 0; col < 800; col += 200)
        {
          printf(" %04x", fb[479 * 800 + col]);
        }

      printf("\n");
    }

  /* Sample CSI SR0 30 times: tells whether the sensor keeps streaming
   * (SOF0F = frame start packets seen, VC0STATEF = VC0 active).
   */

  cam_sample_csi();

  /* NOTE: do not read IMX335 registers here - the I2C read hangs once the
   * sensor has been streaming for a while.  Only DCMIPP/CSI (memory-mapped)
   * status is shown.
   */

  stm32n6_dcmipp_dump_status();
}

/****************************************************************************
 * Name: cam_diag
 *
 * Description:
 *   Dump the CSI-2 / DCMIPP status registers and read back a few IMX335
 *   registers to confirm the sensor is streaming.
 *
 ****************************************************************************/

static void cam_diag(void)
{
  /* NOTE: every IMX335 I2C register access is intentionally disabled here.
   * Once the sensor has been streaming for a while an I2C read hangs the
   * bus (which is why cam_status never touches IMX335 either), and the
   * write-readback test used to put the sensor back into standby, killing
   * the stream.  This command only reports memory-mapped DCMIPP / CSI-2
   * status.
   */

  printf("cam: IMX335 I2C access disabled (I2C hangs after streaming)\n");
  stm32n6_dcmipp_dump_status();

  /* RCC clock-tree diagnostics: confirm SYSCLK (CFGR1) and AHB/ACLK
   * (CFGR2 HPRE) match the FSBL configuration (SYSCLK=IC2~400MHz,
   * HPRE=/2 -> ACLK~200MHz).  A wrong HPRE starves the DCMIPP AXI write
   * path and produces the persistent P1OVRF we observe. */

  {
    volatile uint32_t *rcc = (volatile uint32_t *)0x56028000ul;
    uint32_t cfgr1 = rcc[0x20 / 4];
    uint32_t cfgr2 = rcc[0x24 / 4];

    printf("cam:   RCC CFGR1=0x%08lx CFGR2=0x%08lx (HPRE=%lu)\n",
           (unsigned long)cfgr1, (unsigned long)cfgr2,
           (unsigned long)((cfgr2 >> 20) & 0x7u));
  }

  /* LTDC (display) side diagnostics: is the display engine running and
   * is Layer 1 pointing at the same framebuffer DCMIPP writes to? */

  {
    volatile uint32_t *ltdc = (volatile uint32_t *)0x58001000ul;

    printf("cam: --- LTDC display status ---\n");
    printf("cam:   GCR=0x%08lx (LTDCEN=%lu)\n",
           (unsigned long)ltdc[0x18 / 4],
           (unsigned long)((ltdc[0x18 / 4] >> 0) & 1u));
    printf("cam:   SRCR=0x%08lx ISR=0x%08lx\n",
           (unsigned long)ltdc[0x24 / 4],
           (unsigned long)ltdc[0x38 / 4]);
    printf("cam:   L1CR=0x%08lx (LEN=%lu)\n",
           (unsigned long)ltdc[0x10c / 4],
           (unsigned long)((ltdc[0x10c / 4] >> 0) & 1u));
    printf("cam:   L1CFBAR=0x%08lx\n",
           (unsigned long)ltdc[0x134 / 4]);
    printf("cam:   L1WHPCR=0x%08lx L1PFCR=0x%08lx (PF=%lu)\n",
           (unsigned long)ltdc[0x110 / 4],
           (unsigned long)ltdc[0x11c / 4],
           (unsigned long)(ltdc[0x11c / 4] & 0x7u));
  }
}

/****************************************************************************
 * Name: cam_diag_alive
 *
 * Description:
 *   Sample the CSI-2 PHY lane state 100 times over ~1 s and count how
 *   often the clock/data lanes are in HS activity.  A healthy 30 fps
 *   stream is in HS bursts nearly 100% of the time, so high counts mean
 *   the sensor is still streaming (and the DCMIPP receiver is wedged);
 *   counts near zero with STOP set mean the sensor has stopped.
 *
 ****************************************************************************/

static void cam_diag_alive(void)
{
  int n;
  int clkact = 0;
  int sync0  = 0;
  int sync1  = 0;
  int act0   = 0;
  int act1   = 0;
  int stop0  = 0;
  int stop1  = 0;
  int vc0    = 0;
  int sofs   = 0;
  int eofs   = 0;
  uint32_t sr0;
  uint32_t sr1;

  printf("cam: sampling CSI-2 activity for 1s (100 x 10ms)...\n");

  for (n = 0; n < 100; n++)
    {
      sr0 = stm32n6_dcmipp_get_csi_sr0();
      sr1 = stm32n6_dcmipp_get_csi_sr1();

      if ((sr1 & 0x80000000u) != 0u) clkact++;  /* ACTCLF    */
      if ((sr1 & 0x00020000u) != 0u) sync0++;   /* SYNCDL0F  */
      if ((sr1 & 0x00800000u) != 0u) sync1++;   /* SYNCDL1F  */
      if ((sr1 & 0x00010000u) != 0u) act0++;    /* ACTDL0F   */
      if ((sr1 & 0x00400000u) != 0u) act1++;    /* ACTDL1F   */
      if ((sr1 & 0x00080000u) != 0u) stop0++;   /* STOPDL0F  */
      if ((sr1 & 0x02000000u) != 0u) stop1++;   /* STOPDL1F  */
      if ((sr0 & 0x00020000u) != 0u) vc0++;     /* VC0STATEF */
      if ((sr0 & 0x00000100u) != 0u) sofs++;    /* SOF0F     */
      if ((sr0 & 0x00001000u) != 0u) eofs++;    /* EOF0F     */

      up_mdelay(10);
    }

  printf("cam: 1s: CLKACT=%d SYNC0=%d SYNC1=%d ACT0=%d ACT1=%d "
         "STOP0=%d STOP1=%d VC0=%d SOF=%d EOF=%d\n",
         clkact, sync0, sync1, act0, act1, stop0, stop1, vc0, sofs, eofs);
  printf("cam:   (high ACT/SYNC => sensor streaming, receiver wedged; "
         "all STOP => sensor stopped)\n");
  printf("cam:   sticky err code: 0x%08lx\n",
         (unsigned long)stm32n6_dcmipp_get_error_code());
}

/****************************************************************************
 * Name: cam_diag_recover
 *
 * Description:
 *   Clear CSI flags, re-arm error interrupts and re-assert VC0START.
 *   If the sensor is still streaming the receiver resumes and the frame
 *   counter keeps growing; otherwise VC0STATEF times out.
 *
 ****************************************************************************/

/****************************************************************************
 * Name: cam_diag_sync
 *
 * Description:
 *   Fine-grained CSI-2 sync sampling: read SR0/SR1 every ~1 ms for 100 ms
 *   and report how often the data lanes are synced / active and whether
 *   SOT/sync errors are present.
 *
 ****************************************************************************/

static void cam_diag_sync(void)
{
  uint32_t sr0;
  uint32_t sr1;
  int sync0 = 0, sync1 = 0, act0 = 0, act1 = 0, clkact = 0;
  int sot0 = 0, sot1 = 0, syncerr = 0, ccfifo = 0, vc0 = 0;
  int i;

  printf("cam: fine sync sampling (100 x ~1ms)...\n");

  for (i = 0; i < 100; i++)
    {
      sr0 = stm32n6_dcmipp_get_csi_sr0();
      sr1 = stm32n6_dcmipp_get_csi_sr1();

      if (sr1 & 0x80000000u) clkact++;    /* ACTCLF */
      if (sr1 & 0x00020000u) sync0++;     /* SYNCDL0F */
      if (sr1 & 0x00800000u) sync1++;     /* SYNCDL1F */
      if (sr1 & 0x00010000u) act0++;      /* ACTDL0F */
      if (sr1 & 0x00400000u) act1++;      /* ACTDL1F */
      if (sr1 & 0x00000003u) sot0++;      /* ESOTDL0F|ESOTSYNCDL0F */
      if (sr1 & 0x00000300u) sot1++;      /* ESOTDL1F|ESOTSYNCDL1F */
      if (sr0 & 0x40000000u) syncerr++;   /* SYNCERRF */
      if (sr0 & 0x00200000u) ccfifo++;    /* CCFIFOFF */
      if (sr0 & 0x00020000u) vc0++;       /* VC0STATEF */

      up_udelay(1000);
    }

  printf("cam: sync: CLK=%d SYNC0=%d SYNC1=%d ACT0=%d ACT1=%d "
         "SOT0=%d SOT1=%d SYNCERR=%d CCFIFO=%d VC0=%d /100\n",
         clkact, sync0, sync1, act0, act1, sot0, sot1,
         syncerr, ccfifo, vc0);
}

/****************************************************************************
 * Name: cam_diag_long
 *
 * Description:
 *   Long-duration cumulative monitor.  Samples SR0/SR1 every ~1 ms for
 *   'seconds' (default 10).  The sticky SOF0F/EOF0F flags are cleared
 *   after each catch so every received frame is counted.  This gives a
 *   RELIABLE sync/frame rate per firmware (the short 1msx100 'sync'
 *   sample misses rare transient sync).
 *
 ****************************************************************************/

static void cam_diag_long(int seconds)
{
  uint32_t sr0;
  uint32_t sr1;
  long clk = 0, sync0 = 0, sync1 = 0, act0 = 0, act1 = 0;
  long sot0 = 0, sot1 = 0, syncerr = 0, ccfifo = 0, vc0 = 0;
  long sof_events = 0, eof_events = 0, ccfifo_clears = 0;
  int n;
  int i;

  if (seconds <= 0)
    {
      seconds = 10;
    }

  n = seconds * 1000;
  printf("cam: long monitor %ds (%d x 1ms), clearing flags first...\n",
         seconds, n);

  /* Start clean: clear all CSI status flags */

  stm32n6_dcmipp_clear_csi_flags(0xffffffffu, 0xffffffffu);

  for (i = 0; i < n; i++)
    {
      sr0 = stm32n6_dcmipp_get_csi_sr0();
      sr1 = stm32n6_dcmipp_get_csi_sr1();

      if (sr1 & 0x80000000u) clk++;        /* ACTCLF */
      if (sr1 & 0x00020000u) sync0++;      /* SYNCDL0F */
      if (sr1 & 0x00800000u) sync1++;      /* SYNCDL1F */
      if (sr1 & 0x00010000u) act0++;       /* ACTDL0F */
      if (sr1 & 0x00400000u) act1++;       /* ACTDL1F */
      if (sr1 & 0x00000003u) sot0++;       /* ESOTDL0F|ESOTSYNCDL0F */
      if (sr1 & 0x00000300u) sot1++;       /* ESOTDL1F|ESOTSYNCDL1F */
      if (sr0 & 0x40000000u) syncerr++;    /* SYNCERRF */
      if (sr0 & 0x00200000u) ccfifo++;     /* CCFIFOFF */
      if (sr0 & 0x00020000u) vc0++;        /* VC0STATEF */

      /* Count SOF/EOF events (sticky): clear after each catch */
      if (sr0 & 0x00000100u)               /* SOF0F */
        {
          sof_events++;
          stm32n6_dcmipp_clear_csi_flags(0x00000100u, 0);
        }

      if (sr0 & 0x00001000u)               /* EOF0F */
        {
          eof_events++;
          stm32n6_dcmipp_clear_csi_flags(0x00001000u, 0);
        }

      /* Every 100ms: clear CCFIFOFF + re-assert VC0 to break any
       * CCFIFO-full backpressure wedge (hypothesis: it blocks the PHY
       * sync handshake).  Count how many times it re-sets.
       */

      if ((i % 100) == 99)
        {
          if (sr0 & 0x00200000u)     /* CCFIFOFF was set */
            {
              ccfifo_clears++;
            }

          stm32n6_dcmipp_clear_csi_flags(0x00200000u, 0);
        }

      up_udelay(1000);
    }

  printf("cam: long %ds: CLK=%ld SYNC0=%ld SYNC1=%ld ACT0=%ld ACT1=%ld "
         "SOT0=%ld SOT1=%ld SYNCERR=%ld CCFIFO=%ld VC0=%ld | "
         "SOF_events=%ld EOF_events=%ld CCFIFO_clears=%ld\n",
         seconds, clk, sync0, sync1, act0, act1, sot0, sot1,
         syncerr, ccfifo, vc0, sof_events, eof_events, ccfifo_clears);
}

static void cam_diag_recover(void)
{
  int ret;

  printf("cam: attempting VC0 recovery...\n");
  ret = stm32n6_dcmipp_recover_vc0();
  if (ret == 0)
    {
      printf("cam: recovery OK - VC0STATEF re-asserted, frames=%lu\n",
             (unsigned long)stm32n6_dcmipp_get_frame_count());
    }
  else
    {
      printf("cam: recovery FAILED (VC0STATEF timeout - sensor stopped)\n");
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static int cam_touch(void)
{
  int x = 0;
  int y = 0;
  int n = 0;
  bool pressed = false;
  bool was_pressed = false;
  struct pollfd fds;

  printf("cam: touch test - touch the 7\" panel (q or Ctrl-C to stop)\n");

  for (; ; )
    {
      /* Stop on 'q' / Ctrl-C typed on the console */

      fds.fd = 0;
      fds.events = POLLIN;
      fds.revents = 0;

      if (poll(&fds, 1, 0) > 0)
        {
          int ch = getchar();

          if (ch == 'q' || ch == 'Q' || ch == 0x03)
            {
              printf("cam: touch test stopped (%d samples)\n", n);
              break;
            }
        }

      if (stm32n6_gt9xxx_scan(&x, &y, &pressed) > 0 && pressed)
        {
          if (!was_pressed)
            {
              printf("cam: touch down   x=%4d y=%4d\n", x, y);
            }
          else
            {
              printf("cam: touch move   x=%4d y=%4d\n", x, y);
            }

          n++;
          was_pressed = true;
        }
      else
        {
          if (was_pressed)
            {
              printf("cam: touch up     (%d samples)\n", n);
            }

          was_pressed = false;
          up_mdelay(50);
        }
    }

  return 0;
}

int main(int argc, FAR char *argv[])
{
  if (argc < 2)
    {
      printf("Usage: cam <start|stop|status|hs [0|1|2|3]|rx [0|1|2|3|4]|phy [1000|1200|1600]|touch|diag [alive|sync|long [sec]|recover]>\n");
      return -1;
    }

  if (strcmp(argv[1], "hs") == 0)
    {
      if (argc < 3)
        {
          printf("cam: HS timing mode = %d\n",
                 stm32n6_imx335_get_hs_mode());
          printf("  modes: 0=off (default) 1=TCLK 2=THS 3=full\n");
          return 0;
        }

      stm32n6_imx335_set_hs_mode(atoi(argv[2]));
      printf("cam: HS timing mode set to %d\n",
             stm32n6_imx335_get_hs_mode());
      return 0;
    }
  else if (strcmp(argv[1], "rx") == 0)
    {
      if (argc < 3)
        {
          printf("cam: RX mode = %d\n", stm32n6_dcmipp_get_rx_mode());
          printf("  modes: 0=baseline 1=deskew0 2=deskew3f "
                 "3=osc285 4=osc305\n");
          return 0;
        }

      stm32n6_dcmipp_set_rx_mode(atoi(argv[2]));
      printf("cam: RX mode set to %d\n", stm32n6_dcmipp_get_rx_mode());
      return 0;
    }
  else if (strcmp(argv[1], "phy") == 0)
    {
      int bps;

      if (argc < 3)
        {
          printf("Usage: cam phy <1000|1200|1600>\n");
          return -1;
        }

      bps = atoi(argv[2]);
      if (stm32n6_dcmipp_set_phy_bitrate(bps) < 0)
        {
          printf("cam: invalid PHY bitrate %d\n", bps);
          return -1;
        }

      {
        uint32_t s1 = stm32n6_dcmipp_get_csi_sr1();
        printf("cam: PHY=%dM SR1=0x%08lx"
               " (SYNCDL0=%d SYNCDL1=%d ACTDL0=%d ACTDL1=%d)\n",
               bps, (unsigned long)s1,
               (int)((s1 >> 17) & 1u), (int)((s1 >> 23) & 1u),
               (int)((s1 >> 16) & 1u), (int)((s1 >> 22) & 1u));
      }
      return 0;
    }
  else if (strcmp(argv[1], "start") == 0)
    {
      return cam_start() < 0 ? -1 : 0;
    }
  else if (strcmp(argv[1], "stop") == 0)
    {
      return cam_stop() < 0 ? -1 : 0;
    }
  else if (strcmp(argv[1], "touch") == 0)
    {
      return cam_touch();
    }
  else if (strcmp(argv[1], "status") == 0)
    {
      cam_status();
      return 0;
    }
  else if (strcmp(argv[1], "diag") == 0)
    {
      if (argc > 2 && strcmp(argv[2], "alive") == 0)
        {
          cam_diag_alive();
          return 0;
        }
      else if (argc > 2 && strcmp(argv[2], "recover") == 0)
        {
          cam_diag_recover();
          return 0;
        }
      else if (argc > 2 && strcmp(argv[2], "sync") == 0)
        {
          cam_diag_sync();
          return 0;
        }
      else if (argc > 2 && strcmp(argv[2], "long") == 0)
        {
          cam_diag_long(argc > 3 ? atoi(argv[3]) : 10);
          return 0;
        }

      cam_diag();
      return 0;
    }

  printf("Usage: cam <start|stop|status|hs [0|1|2|3]|rx [0|1|2|3|4]|diag [alive|sync|long [sec]|recover]>\n");
  return -1;
}
