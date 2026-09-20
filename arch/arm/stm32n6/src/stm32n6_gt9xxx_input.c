/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_gt9xxx_input.c
 *
 * GT9xxx capacitive touchscreen lower-half driver for the NuttX
 * touchscreen upper-half (/dev/input0).
 *
 * Reuses the bit-bang I2C scan from stm32n6_gt9xxx.c and pushes touch
 * events through the standard NuttX touchscreen framework
 * (touch_register / touch_event), so LVGL can read them from
 * /dev/input0.
 *
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>

#include <nuttx/arch.h>
#include <nuttx/kmalloc.h>
#include <nuttx/signal.h>
#include <nuttx/input/touchscreen.h>

#include "arm_internal.h"
#include "stm32n6_gt9xxx.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GT9XXX_INPUT_DEVNAME  "/dev/input0"
#define GT9XXX_POLL_USEC      (10 * 1000)  /* Poll every 10 ms */
#define GT9XXX_HOLD_MS        60           /* Hold pressed this long after the
                                            * last coordinate update before
                                            * reporting TOUCH_UP (hides the
                                            * GT9xxx idle gaps between report
                                            * frames). */
#define GT9XXX_THREAD_STACK   2048

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct gt9xxx_input_s
{
  struct touch_lowerhalf_s lower;  /* Touchscreen lower half (must be first) */
  pthread_t thread;                /* Polling thread */
  bool running;                    /* True while the polling thread runs */
  bool pressed;                    /* Current touch-down state */
  uint64_t last_update_us;         /* Uptime of the last reported touch */
  uint16_t last_x;                 /* Last reported X */
  uint16_t last_y;                 /* Last reported Y */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static FAR void *gt9xxx_input_thread(FAR void *arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: gt9xxx_input_uptime_us
 *
 * Description:
 *   Return the system uptime in microseconds.
 *
 ****************************************************************************/

static uint64_t gt9xxx_input_uptime_us(void)
{
  struct timespec ts;

  clock_systime_timespec(&ts);
  return (uint64_t)ts.tv_sec * 1000000ull + ts.tv_nsec / 1000;
}

/****************************************************************************
 * Name: gt9xxx_input_report
 *
 * Description:
 *   Push a single-point touch sample to the touchscreen upper half.
 *
 ****************************************************************************/

static void gt9xxx_input_report(FAR struct gt9xxx_input_s *priv,
                                uint8_t flags, int16_t x, int16_t y)
{
  struct touch_sample_s sample;

  memset(&sample, 0, sizeof(sample));
  sample.npoints              = 1;
  sample.point[0].id          = 0;
  sample.point[0].flags       = flags | TOUCH_ID_VALID | TOUCH_POS_VALID;
  sample.point[0].x           = x;
  sample.point[0].y           = y;

  touch_event(priv->lower.priv, &sample);
}

/****************************************************************************
 * Name: gt9xxx_input_thread
 *
 * Description:
 *   Poll the GT9xxx and report touch events.  While a finger is held
 *   still the GT9xxx may go idle between coordinate frames; the hold
 *   time keeps the contact reported as pressed so LVGL sees a stable
 *   touch instead of rapid down/up chatter.
 *
 ****************************************************************************/

static FAR void *gt9xxx_input_thread(FAR void *arg)
{
  FAR struct gt9xxx_input_s *priv =
    (FAR struct gt9xxx_input_s *)arg;
  int x;
  int y;
  bool pressed;

  while (priv->running)
    {
      if (stm32n6_gt9xxx_scan(&x, &y, &pressed) > 0)
        {
          /* New touch frame from the GT9xxx */

          priv->last_x = (uint16_t)x;
          priv->last_y = (uint16_t)y;
          priv->last_update_us = gt9xxx_input_uptime_us();

          gt9xxx_input_report(priv,
                              priv->pressed ? TOUCH_MOVE : TOUCH_DOWN,
                              x, y);
          priv->pressed = true;
        }
      else if (priv->pressed)
        {
          /* No new frame.  The finger may still be held still while the
           * IC is between reports; only report UP after the hold time.
           */

          if (gt9xxx_input_uptime_us() - priv->last_update_us >
              (uint64_t)GT9XXX_HOLD_MS * 1000)
            {
              gt9xxx_input_report(priv, TOUCH_UP,
                                  priv->last_x, priv->last_y);
              priv->pressed = false;
            }
        }

      nxsig_usleep(GT9XXX_POLL_USEC);
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_gt9xxx_input_register
 *
 * Description:
 *   Register the GT9xxx as the standard touchscreen device /dev/input0
 *   and start the polling thread.
 *
 ****************************************************************************/

int stm32n6_gt9xxx_input_register(void)
{
  FAR struct gt9xxx_input_s *priv;
  pthread_attr_t attr;
  int ret;

  priv = (FAR struct gt9xxx_input_s *)
         kmm_zalloc(sizeof(struct gt9xxx_input_s));
  if (priv == NULL)
    {
      return -ENOMEM;
    }

  priv->lower.maxpoint = 1;
  priv->running        = true;

  ret = touch_register(&priv->lower, GT9XXX_INPUT_DEVNAME, 1);
  if (ret < 0)
    {
      kmm_free(priv);
      return ret;
    }

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, GT9XXX_THREAD_STACK);

  ret = pthread_create(&priv->thread, &attr, gt9xxx_input_thread, priv);
  if (ret < 0)
    {
      touch_unregister(&priv->lower, GT9XXX_INPUT_DEVNAME);
      kmm_free(priv);
      return ret;
    }

  return OK;
}
