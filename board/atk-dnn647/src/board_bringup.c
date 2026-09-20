/****************************************************************************
 * vendor/openvela/boards/atk-dnn647/src/board_bringup.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/mount.h>
#include <syslog.h>

#include <nuttx/board.h>
#include <nuttx/fs/fs.h>

#ifdef CONFIG_USERLED_LOWER
#  include <nuttx/leds/userled.h>
#endif

#ifdef CONFIG_INPUT_BUTTONS_LOWER
#  include <nuttx/input/buttons.h>
#endif

#ifdef CONFIG_RTC_DRIVER
#  include <nuttx/timers/rtc.h>
#endif

#ifdef CONFIG_ARCH_CHIP_STM32N6
#  include "arm_internal.h"
#  include "stm32n6_dcmipp.h"
#  ifdef CONFIG_STM32N6_VIDEO
#    include "stm32n6_video.h"
#  endif
#  ifdef CONFIG_STM32N6_ATON_AIE
#    include <nuttx/aie/stm32n6_aton_aie.h>
#  endif
#  ifdef CONFIG_STM32_SDMMC1
#    include "stm32n6_sdmmc.h"
#  endif
#  ifdef CONFIG_STM32_GT9XXX
#    include "stm32n6_gt9xxx.h"
#  endif
#  ifdef CONFIG_STM32N6_GT9XXX_INPUT
#    include "stm32n6_gt9xxx_input.h"
#  endif
#  include "stm32n6_ltdc.h"
#  ifdef CONFIG_STM32N6_LCD
#    include "stm32n6_lcd.h"
#  endif
#  include "stm32n6_rtc.h"
#  ifdef CONFIG_DEV_GPIO
#    include "stm32n6_gpio.h"
#  endif
#  if defined(CONFIG_STM32_TIM2) || defined(CONFIG_STM32_TIM3_PWM) || \
      defined(CONFIG_STM32_TIM15_CAP)
#    include "stm32n6_tim.h"
#  endif
#  ifdef CONFIG_STM32_TIM5
#    include "stm32n6_oneshot.h"
#  endif
#  if defined(CONFIG_STM32_LPTIM1) || defined(CONFIG_STM32_LPTIM2) || \
      defined(CONFIG_STM32_LPTIM3) || defined(CONFIG_STM32_LPTIM4) || \
      defined(CONFIG_STM32_LPTIM5) || defined(CONFIG_STM32_LPTIM2_PWM)
#    include "stm32n6_lptim.h"
#  endif
#  ifdef CONFIG_STM32_ADC1
#    include "stm32n6_adc.h"
#  endif
#  ifdef CONFIG_STM32_DTS
#    include "stm32n6_dts.h"
#  endif
#  ifdef CONFIG_STM32_I2C4
#    include <nuttx/i2c/i2c_master.h>
#    include "stm32n6_i2c.h"
#  endif
#  ifdef CONFIG_STM32_IWDG
#    include "stm32n6_iwdg.h"
#  endif
#  ifdef CONFIG_STM32_WWDG
#    include "stm32n6_wwdg.h"
#  endif
#  if defined(CONFIG_STM32_XSPI1) || defined(CONFIG_STM32_XSPI2)
#    include "stm32n6_xspi.h"
#  endif
#endif

#include <arch/board/board.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_bringup (STM32N6) / board_bringup (generic)
 *
 * Description:
 *   Perform architecture-specific initialization — mount filesystems, etc.
 *
 ****************************************************************************/

/* LTDC framebuffer (800x480 RGB565 = 768 KB in internal SRAM).  Global so
 * the camera 'cam' application can direct DCMIPP capture into the live
 * display buffer via stm32n6_board_get_framebuffer().
 */

#ifdef CONFIG_VIDEO_FB
uint16_t g_ltdc_fb[800 * 480] __attribute__((aligned(64)));
#endif

static int board_bringup(void)
{
  int ret = OK;

#ifdef CONFIG_FS_PROCFS
  ret = nx_mount(NULL, "/proc", "procfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: Failed to mount procfs at /proc: %d\n", ret);
    }
#endif

#ifdef CONFIG_FS_TMPFS
  ret = nx_mount(NULL, CONFIG_LIBC_TMPDIR, "tmpfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: Failed to mount tmpfs at %s: %d\n",
             CONFIG_LIBC_TMPDIR, ret);
    }
#endif

#ifdef CONFIG_ARCH_CHIP_STM32N6
#  ifdef CONFIG_STM32_RTC
  /* Initialize the RTC peripheral (clock, prescaler, calendar) */

  ret = stm32n6_rtc_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: RTC init failed: %d\n", ret);
    }
#    ifdef CONFIG_RTC_DRIVER
  else
    {
      /* Register the RTC upper-half character driver at /dev/rtc0 */

      ret = rtc_initialize(0, stm32n6_rtc_lowerhalf());
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: rtc_initialize failed: %d\n", ret);
        }
    }
#    endif
#  endif

#  ifdef CONFIG_USERLED_LOWER
  /* Register the user LED driver at /dev/userleds.  The ALIENTEK
   * ATK-DNN647 has two user LEDs: LED0=PG10, LED1=PE10 (active-low),
   * implemented by board_userled*() in stm32n6_userleds.c.
   */

  ret = userled_lower_initialize("/dev/userleds");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: userled_lower_initialize failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_INPUT_BUTTONS_LOWER
  /* Register the button driver at /dev/buttons.  The ALIENTEK ATK-DNN647
   * has four buttons: KEY0=PC6, KEY1=PD1, KEY2=PG11 (active-low) and
   * WKUP=PC13 (active-high), implemented by board_button*() in
   * stm32n6_userbuttons.c.
   */

  ret = btn_lower_initialize("/dev/buttons");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: btn_lower_initialize failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_SDMMC1
  /* Bring up SDMMC1 (TF card on the base board) and mount FATFS.
   * The capture app writes dataset frames under /mnt/sdcard; the card
   * is then removed and read on the host PC.
   */

  ret = stm32n6_sdmmc_initialize(1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: SDMMC1 init failed: %d\n", ret);
    }
  else
    {
      ret = nx_mount(CONFIG_STM32N6_SDMMC_DEVPATH,
                     CONFIG_STM32N6_SDMMC_MOUNTPOINT,
                     "vfat", 0, NULL);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: FATFS mount %s failed: %d\n",
                 CONFIG_STM32N6_SDMMC_MOUNTPOINT, ret);
        }
    }
#  endif

#  ifdef CONFIG_STM32_SDMMC1
  /* Restore the D-Cache now that the card is up.
   *
   * stm32n6_sdmmc_initialize() disables the D-Cache while it programs the
   * card: with the cache on, writes into the SDMMC clock/register domain are
   * not seen by the command state machine and CPSMACT freezes (seen on real
   * HW).  That workaround is needed, but it left the cache off for the rest
   * of the mission, which cost 5-10x on every memory access -- the ATON
   * face-model init took 4.4 s, the eye crop 340 ms, and the per-frame
   * capture restart (DCache-less busy-wait calibration) 2.6 s.
   *
   * The card is fully initialised at this point, so put the cache back.  Any
   * later driver that needs it off (e.g. a re-init) must re-enable it itself.
   */

  up_enable_dcache();

  /* The register printed below is NVIC_CFGCON = ARM_NVIC_BASE + 0x0d14
   * = 0xe000ed14, the ARMv8-M SCB->CCR (bit 16 = D-Cache, bit 17 =
   * I-Cache).  0xe000e00c is AIRCR: it reads back as zero, which made
   * this line report "cache off" while the cache was on.
   */

  syslog(LOG_INFO, "board: D-Cache restored after SDMMC init "
                   "(NVIC_CFGCON=0x%08lx)\n",
         (unsigned long)*(volatile uint32_t *)0xe000ed14u);
#  endif

#  ifdef CONFIG_STM32_XSPI2
  /* Bring up XSPI2 (NOR Flash MX25UM25645G, mmap @ 0x70000000).
   * P0 stage: pins + clocks + device/IO-manager config only.  The
   * memory-mapped command sequence / PHY config (P1) is not programmed
   * yet, so stm32n6_xspi_read() returns undefined data until then.
   */

  ret = stm32n6_xspi_initialize(2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: XSPI2 init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_XSPI1
  /* Bring up XSPI1 (HyperRAM W958D8NBYA5I, mmap @ 0x90000000). */

  ret = stm32n6_xspi_initialize(1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: XSPI1 init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_VIDEO_FB
  /* Bring up the LTDC (7" RGB-LCD ATK-MD0700R-800480, RGB565).
   * The framebuffer lives in internal SRAM: 800x480x2 = 768 KB.
   * Registers /dev/fb0; LVGL etc. can draw into it directly.  The
   * buffer is a global so the camera 'cam' application can capture
   * straight into the live display (stm32n6_board_get_framebuffer).
   */

  ret = stm32n6_ltdc_init(800, 480, g_ltdc_fb, NULL, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LTDC init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32N6_LCD
  /* Register the shared framebuffer as the LCD device /dev/lcd0 so
   * LVGL uses its LCD path (PARTIAL draw buffer + PUTAREA) and the
   * DIRECT-mode tearing disappears.
   */

  ret = lcddev_register(0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: /dev/lcd0 register failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_DEV_GPIO
  /* Register GPIO character devices for the cmocka drivertest_gpio suite.
   * Pins follow the ALIENTEK STM32N647 board (ST SoftwarePackage BSP):
   *   /dev/gpio1 = LED0 (PG10) push-pull output  -> write/readback tests
   *   /dev/gpio2 = KEY0 (PC6)  EXTI input         -> interrupt test
   */

  ret = stm32n6_gpio_lower_initialize(GPIO_PORTG | GPIO_PIN(10), 1,
                                      GPIO_OUTPUT_PIN);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: gpio1 (PG10) init failed: %d\n", ret);
    }

  ret = stm32n6_gpio_lower_initialize(GPIO_PORTC | GPIO_PIN(6), 2,
                                      GPIO_INTERRUPT_PIN);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: gpio2 (PC6) init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_TIM2
  /* Register TIM2 as the timer character device /dev/timer0 for the
   * cmocka drivertest_timer suite (periodic 1 us-resolution timeouts).
   */

  ret = stm32n6_timer_initialize("/dev/timer0", 2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: TIM2 init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_TIM5
  /* Register TIM5 as the one-shot timer /dev/oneshot for the cmocka
   * drivertest_oneshot suite.
   */

    {
      struct oneshot_lowerhalf_s *os = stm32n6_oneshot_initialize(5);
      if (os == NULL)
        {
          syslog(LOG_ERR, "ERROR: TIM5 oneshot init failed\n");
        }
      else
        {
          ret = oneshot_register("/dev/oneshot", os);
          if (ret < 0)
            {
              syslog(LOG_ERR, "ERROR: oneshot_register failed: %d\n", ret);
            }
        }
    }
#  endif

#  ifdef CONFIG_STM32_LPTIM1
  /* Register LPTIM1 as the timer character device /dev/timer1 for the
   * cmocka drivertest_timer suite (LSI-clocked periodic timeouts).
   */

  ret = stm32n6_lptim_initialize("/dev/timer1", 1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LPTIM1 init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_LPTIM2
  /* Register LPTIM2 (APB4, LSI-clocked) as /dev/timer2. */

  ret = stm32n6_lptim_initialize("/dev/timer2", 2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LPTIM2 init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_LPTIM3
  /* Register LPTIM3 (APB4, LSI-clocked) as /dev/timer3. */

  ret = stm32n6_lptim_initialize("/dev/timer3", 3);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LPTIM3 init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_LPTIM4
  /* Register LPTIM4 (APB4, LSI-clocked) as /dev/timer4. */

  ret = stm32n6_lptim_initialize("/dev/timer4", 4);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LPTIM4 init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_LPTIM5
  /* Register LPTIM5 (APB4, LSI-clocked) as /dev/timer5. */

  ret = stm32n6_lptim_initialize("/dev/timer5", 5);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LPTIM5 init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_LPTIM2_PWM
  /* Register LPTIM2 as the low-power PWM output /dev/pwm0 (LPTIM2_CH1 on
   * PF1, LSI-clocked) for the cmocka drivertest_pwm suite.
   */

  ret = stm32n6_lppwm_initialize("/dev/pwm0");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LPTIM2 PWM init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_TIM3_PWM
  /* Register TIM3 as the PWM output /dev/pwm1 (TIM3_CH1, PWM mode 1).  Its
   * waveform is routed on-chip into TIM15 TI1 (TISEL) for the wire-free
   * capture loopback used by the tim_cap self-test.
   */

  ret = stm32n6_tim_pwm_initialize("/dev/pwm1");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: TIM3 PWM init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_TIM15_CAP
  /* Register TIM15 as the input-capture device /dev/cap0 (PWM-input mode,
   * TI1 sourced internally from TIM3 CH1 via TISEL) for the tim_cap
   * loopback self-test.
   */

  ret = stm32n6_tim_cap_initialize("/dev/cap0");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: TIM15 capture init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_ADC1
  /* Register ADC1 as /dev/adc0 for the cmocka drivertest_adc suite.  It
   * samples the chip-internal VREFINT source (no external wiring).
   */

  ret = stm32n6_adc_initialize("/dev/adc0", 1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: ADC1 init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_ADC2
  /* Register ADC2 as /dev/adc1: a VBAT + VDDCORE regular scan moved by GPDMA
   * with the analog watchdog armed (ADR-029 scan/DMA/AWD gaps).
   */

  ret = stm32n6_adc_initialize("/dev/adc1", 2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: ADC2 init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_DTS
  /* Register the digital temperature sensor as /dev/temp0.  Fully internal
   * die junction sensor (no external wiring); read() returns a b16_t
   * Celsius value (ADR-030).
   */

  ret = stm32n6_dts_initialize("/dev/temp0");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: DTS init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_I2C4
  /* Bring up I2C4 (PE13=SCL, PE14=SDA, AF4) and expose it as /dev/i2c4 for
   * the onboard AP3216C ambient-light/proximity/IR sensor at address 0x1e
   * (ADR-014).  A real external slave, so it validates the bus end to end.
   */

  do
    {
      FAR struct i2c_master_s *i2c4 = stm32n6_i2cbus_initialize(4);

      if (i2c4 == NULL)
        {
          syslog(LOG_ERR, "ERROR: I2C4 init failed\n");
        }
      else
        {
          ret = i2c_register(i2c4, 4);
          if (ret < 0)
            {
              syslog(LOG_ERR, "ERROR: i2c_register(4) failed: %d\n", ret);
            }
        }
    }
  while (0);
#  endif

#  ifdef CONFIG_STM32_IWDG
  /* Register the IWDG as /dev/watchdog0 for the cmocka drivertest_watchdog
   * suite.  LSI-clocked, left stopped until WDIOC_START.
   */

  ret = stm32n6_iwdg_initialize("/dev/watchdog0");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: IWDG init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_WWDG
  /* Register the WWDG as /dev/watchdog1.  Its early-wakeup interrupt backs
   * the watchdog capture() op, which the drivertest_watchdog API case uses.
   */

  ret = stm32n6_wwdg_initialize("/dev/watchdog1");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: WWDG init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_DCMIPP
  /* Initialize DCMIPP camera (clock/RIF/CSI/PIPE1, no capture yet).
   * The IMX335 sensor and capture start are controlled by the 'cam'
   * NSH application or the V4L2 /dev/video0 driver.
   */

  ret = stm32n6_dcmipp_init();
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: DCMIPP init failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32N6_VIDEO
  /* Register the V4L2 camera capture device (/dev/video0). */

  ret = stm32n6_video_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: V4L2 video register failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32N6_ATON_AIE
  /* Register the Neural-ART NPU AI Engine device (/dev/aie0). */

  ret = stm32n6_aton_aie_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: NPU AIE register failed: %d\n", ret);
    }
#  endif

#  ifdef CONFIG_STM32_GT9XXX
  /* Initialize the GT9xxx capacitive touch controller (7" panel). */

  ret = stm32n6_gt9xxx_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: GT9xxx touch init failed: %d\n", ret);
    }
#    ifdef CONFIG_STM32N6_GT9XXX_INPUT
  /* Register /dev/input0 for LVGL. */

  ret = stm32n6_gt9xxx_input_register();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: GT9xxx input register failed: %d\n", ret);
    }
#    endif
#  endif
#endif

  return ret;
}

/****************************************************************************
 * Name: board_lcd_getdev
 *
 * Description:
 *   Return a reference to the LCD object for the specified LCD.
 *
 ****************************************************************************/

/* g_stm32n6_lcd is provided by the chip LCD driver (stm32n6_lcd.c), which is
 * only built when CONFIG_STM32N6_LCD is selected.  CONFIG_LCD alone selects
 * the LCD upper half, so both have to be checked here.
 */

#if defined(CONFIG_LCD) && defined(CONFIG_STM32N6_LCD)
struct lcd_dev_s *board_lcd_getdev(int lcddev)
{
  if (lcddev == 0)
    {
      return &g_stm32n6_lcd;
    }

  return NULL;
}
#endif

/****************************************************************************
 * Name: stm32n6_board_get_framebuffer
 *
 * Description:
 *   Return the LTDC framebuffer base address (800x480 RGB565, internal
 *   SRAM).  Exposed so the camera 'cam' application can direct DCMIPP
 *   capture into the live display buffer.
 *
 ****************************************************************************/

#ifdef CONFIG_VIDEO_FB
uint32_t stm32n6_board_get_framebuffer(void)
{
  extern uint16_t g_ltdc_fb[];
  return (uint32_t)(uintptr_t)g_ltdc_fb;
}
#endif

/****************************************************************************
 * Name: stm32_boardinitialize
 *
 * Description:
 *   STM32N6 arch layer calls this early in boot.  For QEMU/MPS3 builds
 *   this symbol is not referenced by the arch layer.
 *
 ****************************************************************************/

#ifdef CONFIG_ARCH_CHIP_STM32N6
void stm32_boardinitialize(void)
{
}
#endif

/****************************************************************************
 * Name: board_late_initialize
 *
 * Description:
 *   Called after up_initialize(), before the initial app starts.
 *
 ****************************************************************************/

#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void)
{
  board_bringup();
}
#endif

/****************************************************************************
 * Name: board_app_initialize
 *
 * Description:
 *   Perform application specific initialization.
 *
 ****************************************************************************/

int board_app_initialize(uintptr_t arg)
{
  UNUSED(arg);

#ifndef CONFIG_BOARD_LATE_INITIALIZE
  return board_bringup();
#else
  return OK;
#endif
}
