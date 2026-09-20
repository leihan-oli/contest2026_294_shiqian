/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_gpio_lower.c
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
 * NuttX GPIO character-device lower-half for STM32N6.  Bridges the
 * generic /dev/gpioN upper-half (drivers/ioexpander/gpio.c) onto the
 * arch pin primitives (stm32n6_configgpio / gpioread / gpiowrite) and
 * the EXTI interrupt engine (stm32n6_gpiosetevent).
 *
 * A single unified operations table implements all seven ops so that a
 * device may be switched between output/input/interrupt pintypes at
 * run time via GPIOC_SETPINTYPE (as the cmocka drivertest_gpio suite
 * does); the upper half DEBUGASSERTs every op pointer it needs for the
 * new pintype, so none may be NULL.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <debug.h>

#include <nuttx/kmalloc.h>
#include <nuttx/ioexpander/gpio.h>

#include "stm32n6_gpio.h"
#include "stm32n6_exti.h"

#ifdef CONFIG_DEV_GPIO

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_gpiolower_s
{
  struct gpio_dev_s gpio;      /* Upper-half device (must be first)     */
  uint32_t pinset;             /* Port|pin encoding (no mode bits)      */
  pin_interrupt_t callback;    /* Registered interrupt callback         */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32n6_go_read(FAR struct gpio_dev_s *dev, FAR bool *value);
static int stm32n6_go_write(FAR struct gpio_dev_s *dev, bool value);
static int stm32n6_go_attach(FAR struct gpio_dev_s *dev,
                             pin_interrupt_t callback);
static int stm32n6_go_enable(FAR struct gpio_dev_s *dev, bool enable);
static int stm32n6_go_setpintype(FAR struct gpio_dev_s *dev,
                                 enum gpio_pintype_e pintype);
static int stm32n6_go_setdebounce(FAR struct gpio_dev_s *dev,
                                  unsigned long duration);
static int stm32n6_go_setmask(FAR struct gpio_dev_s *dev, bool enable);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct gpio_operations_s g_stm32n6_gpio_ops =
{
  .go_read       = stm32n6_go_read,
  .go_write      = stm32n6_go_write,
  .go_attach     = stm32n6_go_attach,
  .go_enable     = stm32n6_go_enable,
  .go_setpintype = stm32n6_go_setpintype,
  .go_setdebounce = stm32n6_go_setdebounce,
  .go_setmask    = stm32n6_go_setmask,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_gpio_isr
 *
 * Description:
 *   EXTI dispatch trampoline.  Forwards the arch xcpt_t interrupt to the
 *   upper-half pin_interrupt_t callback stored on the device.
 *
 ****************************************************************************/

static int stm32n6_gpio_isr(int irq, FAR void *context, FAR void *arg)
{
  FAR struct stm32n6_gpiolower_s *priv =
    (FAR struct stm32n6_gpiolower_s *)arg;

  DEBUGASSERT(priv != NULL && priv->callback != NULL);

  priv->callback(&priv->gpio,
                 (uint8_t)((priv->pinset & GPIO_PIN_MASK) >>
                           GPIO_PIN_SHIFT));
  return OK;
}

/****************************************************************************
 * Name: stm32n6_pintype_cfgset
 *
 * Description:
 *   Translate a generic gpio_pintype_e into the arch pin-configuration
 *   mode bits, OR'd onto the device's fixed port|pin base.
 *
 ****************************************************************************/

static uint32_t stm32n6_pintype_cfgset(uint32_t base,
                                       enum gpio_pintype_e pintype)
{
  switch (pintype)
    {
      case GPIO_INPUT_PIN:
        return base | GPIO_MODE_INPUT | GPIO_PUPD_NONE;

      case GPIO_INPUT_PIN_PULLUP:
        return base | GPIO_MODE_INPUT | GPIO_PUPD_PU;

      case GPIO_INPUT_PIN_PULLDOWN:
        return base | GPIO_MODE_INPUT | GPIO_PUPD_PD;

      case GPIO_OUTPUT_PIN:
        return base | GPIO_MODE_OUTPUT | GPIO_OTYPE_PP |
               GPIO_SPEED_HIGH;

      case GPIO_OUTPUT_PIN_OPENDRAIN:
        return base | GPIO_MODE_OUTPUT | GPIO_OTYPE_OD |
               GPIO_SPEED_HIGH;

      default:

        /* All interrupt pintypes are configured as plain inputs here;
         * the edge trigger is programmed separately via EXTI in
         * stm32n6_go_enable().  A pull is chosen to give the line a
         * defined idle level when left floating.
         */

        if (pintype == GPIO_INTERRUPT_RISING_PIN ||
            pintype == GPIO_INTERRUPT_HIGH_PIN)
          {
            return base | GPIO_MODE_INPUT | GPIO_PUPD_PD;
          }

        return base | GPIO_MODE_INPUT | GPIO_PUPD_PU;
    }
}

/****************************************************************************
 * Name: stm32n6_go_read
 ****************************************************************************/

static int stm32n6_go_read(FAR struct gpio_dev_s *dev, FAR bool *value)
{
  FAR struct stm32n6_gpiolower_s *priv =
    (FAR struct stm32n6_gpiolower_s *)dev;

  DEBUGASSERT(priv != NULL && value != NULL);

  *value = stm32n6_gpioread(priv->pinset);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_go_write
 ****************************************************************************/

static int stm32n6_go_write(FAR struct gpio_dev_s *dev, bool value)
{
  FAR struct stm32n6_gpiolower_s *priv =
    (FAR struct stm32n6_gpiolower_s *)dev;

  DEBUGASSERT(priv != NULL);

  stm32n6_gpiowrite(priv->pinset, value);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_go_attach
 ****************************************************************************/

static int stm32n6_go_attach(FAR struct gpio_dev_s *dev,
                             pin_interrupt_t callback)
{
  FAR struct stm32n6_gpiolower_s *priv =
    (FAR struct stm32n6_gpiolower_s *)dev;

  DEBUGASSERT(priv != NULL);

  /* Detach any prior EXTI wiring before (re)storing the callback */

  stm32n6_gpiosetevent(priv->pinset, false, false, false, NULL, NULL);
  priv->callback = callback;
  return OK;
}

/****************************************************************************
 * Name: stm32n6_go_enable
 ****************************************************************************/

static int stm32n6_go_enable(FAR struct gpio_dev_s *dev, bool enable)
{
  FAR struct stm32n6_gpiolower_s *priv =
    (FAR struct stm32n6_gpiolower_s *)dev;
  bool rising;
  bool falling;

  DEBUGASSERT(priv != NULL);

  if (!enable || priv->callback == NULL)
    {
      stm32n6_gpiosetevent(priv->pinset, false, false, false, NULL, NULL);
      return OK;
    }

  /* Derive the edge trigger from the pintype set by the upper half */

  switch (priv->gpio.gp_pintype)
    {
      case GPIO_INTERRUPT_RISING_PIN:
      case GPIO_INTERRUPT_HIGH_PIN:
        rising  = true;
        falling = false;
        break;

      case GPIO_INTERRUPT_FALLING_PIN:
      case GPIO_INTERRUPT_LOW_PIN:
        rising  = false;
        falling = true;
        break;

      default:

        /* GPIO_INTERRUPT_PIN / _BOTH_PIN: trigger on both edges */

        rising  = true;
        falling = true;
        break;
    }

  return stm32n6_gpiosetevent(priv->pinset, rising, falling, false,
                              stm32n6_gpio_isr, priv);
}

/****************************************************************************
 * Name: stm32n6_go_setpintype
 ****************************************************************************/

static int stm32n6_go_setpintype(FAR struct gpio_dev_s *dev,
                                 enum gpio_pintype_e pintype)
{
  FAR struct stm32n6_gpiolower_s *priv =
    (FAR struct stm32n6_gpiolower_s *)dev;
  int ret;

  DEBUGASSERT(priv != NULL && pintype < GPIO_NPINTYPES);

  /* Tear down any active EXTI trigger before re-purposing the pin */

  stm32n6_gpiosetevent(priv->pinset, false, false, false, NULL, NULL);

  ret = stm32n6_configgpio(stm32n6_pintype_cfgset(priv->pinset, pintype));
  if (ret < 0)
    {
      return ret;
    }

  /* The upper half asserts dev->gp_pintype == pintype on return */

  priv->gpio.gp_pintype = pintype;
  return OK;
}

/****************************************************************************
 * Name: stm32n6_go_setdebounce
 ****************************************************************************/

static int stm32n6_go_setdebounce(FAR struct gpio_dev_s *dev,
                                  unsigned long duration)
{
  /* No hardware debounce filter is wired on this port */

  UNUSED(dev);
  UNUSED(duration);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_go_setmask
 ****************************************************************************/

static int stm32n6_go_setmask(FAR struct gpio_dev_s *dev, bool enable)
{
  FAR struct stm32n6_gpiolower_s *priv =
    (FAR struct stm32n6_gpiolower_s *)dev;

  DEBUGASSERT(priv != NULL);

  /* Masking an interrupt is equivalent to disabling its EXTI trigger */

  if (enable)
    {
      stm32n6_gpiosetevent(priv->pinset, false, false, false, NULL, NULL);
    }
  else
    {
      stm32n6_go_enable(dev, true);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_gpio_lower_initialize
 *
 * Description:
 *   Configure a single physical pin for the requested initial pintype and
 *   register it with the GPIO upper half as /dev/gpioN.
 *
 * Input Parameters:
 *   pinset  - Port|pin base encoding (mode bits are supplied internally)
 *   minor   - Minor number N for the /dev/gpioN node
 *   pintype - Initial pin type (see enum gpio_pintype_e)
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_gpio_lower_initialize(uint32_t pinset, int minor,
                                  enum gpio_pintype_e pintype)
{
  FAR struct stm32n6_gpiolower_s *priv;
  int ret;

  priv = kmm_zalloc(sizeof(struct stm32n6_gpiolower_s));
  if (priv == NULL)
    {
      return -ENOMEM;
    }

  priv->pinset          = pinset & (GPIO_PORT_MASK | GPIO_PIN_MASK);
  priv->callback        = NULL;
  priv->gpio.gp_pintype = pintype;
  priv->gpio.gp_ops     = &g_stm32n6_gpio_ops;

  /* Apply the initial electrical configuration */

  ret = stm32n6_configgpio(stm32n6_pintype_cfgset(priv->pinset, pintype));
  if (ret < 0)
    {
      kmm_free(priv);
      return ret;
    }

  ret = gpio_pin_register(&priv->gpio, minor);
  if (ret < 0)
    {
      kmm_free(priv);
      return ret;
    }

  return OK;
}

#endif /* CONFIG_DEV_GPIO */
