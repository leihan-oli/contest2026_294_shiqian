/****************************************************************************
 * apps/examples/lvgldemo/lvgldemo.c
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
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <sys/boardctl.h>

#include <lvgl/lvgl.h>
#include <lvgl/demos/lv_demos.h>
#ifdef CONFIG_LV_USE_NUTTX_LIBUV
#include <uv.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Should we perform board-specific driver initialization? There are two
 * ways that board initialization can occur:  1) automatically via
 * board_late_initialize() during bootupif CONFIG_BOARD_LATE_INITIALIZE
 * or 2).
 * via a call to boardctl() if the interface is enabled
 * (CONFIG_BOARDCTL=y).
 * If this task is running as an NSH built-in application, then that
 * initialization has probably already been performed otherwise we do it
 * here.
 */

#undef NEED_BOARDINIT

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
#  define NEED_BOARDINIT 1
#endif

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
static void lv_nuttx_uv_loop(uv_loop_t *loop, lv_nuttx_result_t *result)
{
  lv_nuttx_uv_t uv_info;
  void *data;

  uv_loop_init(loop);

  lv_memset(&uv_info, 0, sizeof(uv_info));
  uv_info.loop = loop;
  uv_info.disp = result->disp;
  uv_info.indev = result->indev;
#ifdef CONFIG_UINPUT_TOUCH
  uv_info.uindev = result->utouch_indev;
#endif

#ifdef CONFIG_LV_USE_NUTTX_MOUSE
  uv_info.mouse_indev = result->mouse_indev;
#endif

  data = lv_nuttx_uv_init(&uv_info);
  uv_run(loop, UV_RUN_DEFAULT);
  lv_nuttx_uv_deinit(&data);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main or lv_demos_main
 *
 * Description:
 *
 * Input Parameters:
 *   Standard argc and argv
 *
 * Returned Value:
 *   Zero on success; a positive, non-zero value on failure.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  uv_loop_t ui_loop;
  lv_memzero(&ui_loop, sizeof(ui_loop));
#endif

  if (lv_is_initialized())
    {
      LV_LOG_ERROR("LVGL already initialized! aborting.");
      return -1;
    }

#ifdef NEED_BOARDINIT
  /* Perform board-specific driver initialization */

  boardctl(BOARDIOC_INIT, 0);

#endif

  lv_init();

  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#endif

#ifdef CONFIG_INPUT_TOUCHSCREEN
  info.input_path = CONFIG_EXAMPLES_LVGLDEMO_INPUT_DEVPATH;
#endif

  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      LV_LOG_ERROR("lv_demos initialization failure!");
      return 1;
    }

  if (!lv_demos_create(&argv[1], argc - 1))
    {
      lv_demos_show_help();

      /* we can add custom demos here */

      goto demo_end;
    }

  /* Tear-free smoothness: drop all continuous demo animations (gauges,
   * progress bars, charts...).  On a 600 MHz Cortex-M55 with a single
   * PARTIAL framebuffer, lv_demo_widgets keeps LVGL rendering 60 fps of
   * auto-animation, so during an interactive drag the renderer can't keep
   * up with the LTDC scanline and the screen refreshes in visible blocks.
   * Removing the animations frees the CPU for the actual drag rendering.
   */

  lv_anim_delete_all();

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  lv_nuttx_uv_loop(&ui_loop, &result);
#else
  /* DWT cycle counter for high-resolution timing (systick is only 10ms).
   * DEMCR.TRCENA + DWT.CYCCNTENA, then read DWT->CYCCNT. */

  {
    volatile uint32_t *demcr = (volatile uint32_t *)0xe000edfcu;
    volatile uint32_t *dwt_ctrl = (volatile uint32_t *)0xe0001000u;
    volatile uint32_t *dwt_cnt = (volatile uint32_t *)0xe0001004u;

    *demcr |= (1u << 24);
    *dwt_cnt = 0;
    *dwt_ctrl |= 1u;

    while (1)
      {
        uint32_t t0 = *dwt_cnt;
        uint32_t idle = lv_timer_handler();
        uint32_t dt = *dwt_cnt - t0;   /* handler wall time in CPU cycles */

        /* One-per-second diagnostics: timer-handler rate, average idle and
         * the handler wall time (avg/max, ms @ 600MHz).  While dragging a
         * full-screen refresh this shows up as a 200-500ms handler; the
         * putarea diagnostics on /dev/lcd0 show how much of that is the
         * flush itself vs. LVGL rendering. */

        static uint32_t diag_handlers = 0;
        static uint32_t diag_idle_sum  = 0;
        static uint32_t diag_dt_sum    = 0;
        static uint32_t diag_dt_max    = 0;
        static uint32_t diag_last_sec  = 0;
        uint32_t now = (uint32_t)(clock() / CLOCKS_PER_SEC);

        diag_handlers++;
        diag_idle_sum += idle;
        diag_dt_sum += dt;
        if (dt > diag_dt_max)
          {
            diag_dt_max = dt;
          }

        if (now != diag_last_sec)
          {
            syslog(LOG_INFO,
                   "lvgldemo: %lu handler/s, avg_idle=%lu ms, "
                   "handler_avg=%lu ms, handler_max=%lu ms\n",
                   (unsigned long)diag_handlers,
                   (unsigned long)(diag_idle_sum / diag_handlers),
                   (unsigned long)(diag_dt_sum / diag_handlers / 600000),
                   (unsigned long)(diag_dt_max / 600000));
            diag_handlers = 0;
            diag_idle_sum = 0;
            diag_dt_sum = 0;
            diag_dt_max = 0;
            diag_last_sec = now;
          }

        /* Minimum sleep of 1ms */

        idle = idle ? idle : 1;
        usleep(idle * 1000);
      }
  }
#endif

demo_end:
  lv_nuttx_deinit(&result);
  lv_deinit();

  return 0;
}
