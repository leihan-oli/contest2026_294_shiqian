/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_gt9xxx.c
 *
 * GT9xxx capacitive touch controller driver (ALIENTEK ATK-DNN647 7" panel).
 * GPIO bit-bang I2C:  SCL=PD14, SDA=PD4 (open-drain + pull-up).
 * Reset=PD10, INT=PB3.
 *
 * Ported from the ALIENTEK BSP (Drivers/BSP/TOUCH/gt9xxx.c + ctiic.c) to
 * NuttX GPIO APIs.
 *
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdbool.h>
#include <string.h>
#include <debug.h>
#include <errno.h>
#include <syslog.h>

#include "arm_internal.h"
#include "stm32n6_gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* GT9xxx GPIO pins (ALIENTEK 7" panel) */

#define GT9XXX_SCL_GPIO   (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
                           GPIO_PUPD_NONE | GPIO_PORTD | GPIO_PIN(14))
#define GT9XXX_SDA_GPIO   (GPIO_MODE_OUTPUT | GPIO_OTYPE_OD | GPIO_SPEED_HIGH | \
                           GPIO_PUPD_PU  | GPIO_PORTD | GPIO_PIN(4))
#define GT9XXX_RST_GPIO   (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | GPIO_SPEED_HIGH | \
                           GPIO_PUPD_NONE | GPIO_PORTD | GPIO_PIN(10))
#define GT9XXX_INT_GPIO   (GPIO_MODE_INPUT  | GPIO_PUPD_PU | \
                           GPIO_PORTB | GPIO_PIN(3))

/* GT9xxx I2C command / registers */

#define GT9XXX_CMD_WR     0x28
#define GT9XXX_CMD_RD     0x29

#define GT9XXX_CTRL_REG   0x8040
#define GT9XXX_PID_REG    0x8140
#define GT9XXX_GSTID_REG  0x814e
#define GT9XXX_TP1_REG    0x8150

#define GT9XXX_MAX_TOUCH  5

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_gt_tnum = GT9XXX_MAX_TOUCH;

/****************************************************************************
 * Private Functions - bit-bang I2C
 ****************************************************************************/

static void gt9xxx_delay(void)
{
  up_udelay(2);
}

static void gt9xxx_scl(int high)
{
  stm32n6_gpiowrite(GT9XXX_SCL_GPIO, high);
}

static void gt9xxx_sda(int high)
{
  stm32n6_gpiowrite(GT9XXX_SDA_GPIO, high);
}

static int gt9xxx_sda_read(void)
{
  return stm32n6_gpioread(GT9XXX_SDA_GPIO);
}

static void gt9xxx_i2c_start(void)
{
  gt9xxx_sda(1);
  gt9xxx_scl(1);
  gt9xxx_delay();
  gt9xxx_sda(0);
  gt9xxx_delay();
  gt9xxx_scl(0);
  gt9xxx_delay();
}

static void gt9xxx_i2c_stop(void)
{
  gt9xxx_sda(0);
  gt9xxx_delay();
  gt9xxx_scl(1);
  gt9xxx_delay();
  gt9xxx_sda(1);
  gt9xxx_delay();
}

static int gt9xxx_i2c_wait_ack(void)
{
  int waittime = 0;

  gt9xxx_sda(1);
  gt9xxx_delay();
  gt9xxx_scl(1);
  gt9xxx_delay();

  while (gt9xxx_sda_read())
    {
      waittime++;
      if (waittime > 250)
        {
          gt9xxx_i2c_stop();
          return 1;
        }
      gt9xxx_delay();
    }

  gt9xxx_scl(0);
  gt9xxx_delay();
  return 0;
}

static void gt9xxx_i2c_ack(void)
{
  gt9xxx_sda(0);
  gt9xxx_delay();
  gt9xxx_scl(1);
  gt9xxx_delay();
  gt9xxx_scl(0);
  gt9xxx_delay();
  gt9xxx_sda(1);
  gt9xxx_delay();
}

static void gt9xxx_i2c_nack(void)
{
  gt9xxx_sda(1);
  gt9xxx_delay();
  gt9xxx_scl(1);
  gt9xxx_delay();
  gt9xxx_scl(0);
  gt9xxx_delay();
}

static void gt9xxx_i2c_send_byte(uint8_t data)
{
  int t;

  for (t = 0; t < 8; t++)
    {
      gt9xxx_sda((data & 0x80) >> 7);
      gt9xxx_delay();
      gt9xxx_scl(1);
      gt9xxx_delay();
      gt9xxx_scl(0);
      data <<= 1;
    }

  gt9xxx_sda(1);
}

static uint8_t gt9xxx_i2c_read_byte(bool ack)
{
  uint8_t receive = 0;
  int i;

  for (i = 0; i < 8; i++)
    {
      receive <<= 1;
      gt9xxx_scl(1);
      gt9xxx_delay();

      if (gt9xxx_sda_read())
        {
          receive++;
        }

      gt9xxx_scl(0);
      gt9xxx_delay();
    }

  if (ack)
    {
      gt9xxx_i2c_ack();
    }
  else
    {
      gt9xxx_i2c_nack();
    }

  return receive;
}

/****************************************************************************
 * Private Functions - GT9xxx register access
 ****************************************************************************/

static void gt9xxx_wr_reg(uint16_t reg, FAR const uint8_t *buf, uint8_t len)
{
  uint8_t i;

  gt9xxx_i2c_start();
  gt9xxx_i2c_send_byte(GT9XXX_CMD_WR);
  gt9xxx_i2c_wait_ack();
  gt9xxx_i2c_send_byte(reg >> 8);
  gt9xxx_i2c_wait_ack();
  gt9xxx_i2c_send_byte(reg & 0xff);
  gt9xxx_i2c_wait_ack();

  for (i = 0; i < len; i++)
    {
      gt9xxx_i2c_send_byte(buf[i]);
      if (gt9xxx_i2c_wait_ack())
        {
          break;
        }
    }

  gt9xxx_i2c_stop();
}

static void gt9xxx_rd_reg(uint16_t reg, FAR uint8_t *buf, uint8_t len)
{
  uint8_t i;

  gt9xxx_i2c_start();
  gt9xxx_i2c_send_byte(GT9XXX_CMD_WR);
  gt9xxx_i2c_wait_ack();
  gt9xxx_i2c_send_byte(reg >> 8);
  gt9xxx_i2c_wait_ack();
  gt9xxx_i2c_send_byte(reg & 0xff);
  gt9xxx_i2c_wait_ack();
  gt9xxx_i2c_start();
  gt9xxx_i2c_send_byte(GT9XXX_CMD_RD);
  gt9xxx_i2c_wait_ack();

  for (i = 0; i < len; i++)
    {
      buf[i] = gt9xxx_i2c_read_byte(i == (len - 1) ? false : true);
    }

  gt9xxx_i2c_stop();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_gt9xxx_initialize
 *
 * Description:
 *   Configure pins, reset the GT9xxx, verify its product ID and leave it
 *   in coordinate-report mode.
 *
 ****************************************************************************/

int stm32n6_gt9xxx_initialize(void)
{
  uint8_t temp[5];
  int ret = -ENODEV;

  stm32n6_configgpio(GT9XXX_SCL_GPIO);
  stm32n6_configgpio(GT9XXX_SDA_GPIO);
  stm32n6_configgpio(GT9XXX_RST_GPIO);
  stm32n6_configgpio(GT9XXX_INT_GPIO);

  /* Reset pulse */

  stm32n6_gpiowrite(GT9XXX_RST_GPIO, false);
  up_mdelay(10);
  stm32n6_gpiowrite(GT9XXX_RST_GPIO, true);
  up_mdelay(10);

  /* Read product ID: "911", "9147", "1158" or "9271" */

  temp[4] = 0;
  gt9xxx_rd_reg(GT9XXX_PID_REG, temp, 4);

  syslog(LOG_INFO, "gt9xxx: PID bytes = %02x %02x %02x %02x\n",
         temp[0], temp[1], temp[2], temp[3]);

  if (strcmp((char *)temp, "911")  == 0 ||
      strcmp((char *)temp, "9147") == 0 ||
      strcmp((char *)temp, "1158") == 0 ||
      strcmp((char *)temp, "9271") == 0)
    {
      if (strcmp((char *)temp, "9271") == 0)
        {
          g_gt_tnum = 10;
        }
      else
        {
          g_gt_tnum = GT9XXX_MAX_TOUCH;
        }

      syslog(LOG_INFO, "gt9xxx: touch IC found, ID=%s, tnum=%d\n",
             (char *)temp, g_gt_tnum);

      /* Soft reset: set then clear the control register bit0 */

      temp[0] = 0x02;
      gt9xxx_wr_reg(GT9XXX_CTRL_REG, temp, 1);
      up_mdelay(10);

      temp[0] = 0x00;
      gt9xxx_wr_reg(GT9XXX_CTRL_REG, temp, 1);

      ret = OK;
    }
  else
    {
      syslog(LOG_ERR, "gt9xxx: no supported touch IC (ID=%.4s)\n",
             (char *)temp);
    }

  return ret;
}

/****************************************************************************
 * Name: stm32n6_gt9xxx_scan
 *
 * Description:
 *   Poll the GT9xxx for a touch.  On a touch returns 1 and fills *x / *y
 *   with the first contact's coordinates (raw 0..1023 / 0..511 range,
 *   matching the 7" panel); otherwise returns 0 and *pressed=false.
 *
 ****************************************************************************/

int stm32n6_gt9xxx_scan(FAR int *x, FAR int *y, FAR bool *pressed)
{
  uint8_t status;
  uint8_t buf[4];

  *pressed = false;

  gt9xxx_rd_reg(GT9XXX_GSTID_REG, &status, 1);

  if ((status & 0x0f) == 0 || (status & 0x0f) > g_gt_tnum)
    {
      /* No valid contact.  If the IC raised the buffer-status bit (0x80)
       * it is waiting for the host to consume the (empty) report; clear
       * it or the status sticks at 0x80 forever (observed on the panel).
       */

      if (status & 0x80)
        {
          status = 0;
          gt9xxx_wr_reg(GT9XXX_GSTID_REG, &status, 1);
        }

      return 0;
    }

  /* Read the first touch point: 4 bytes of raw coordinate data */

  gt9xxx_rd_reg(GT9XXX_TP1_REG, buf, 4);

#ifdef CONFIG_STM32_GT9XXX_DEBUG
  /* Dump the raw bytes so the register layout can be verified. */

  syslog(LOG_INFO, "gt9xxx: raw=%02x %02x %02x %02x\n",
         buf[0], buf[1], buf[2], buf[3]);
#endif

  /* Consume the report: clear the buffer-status flag */

  status = 0;
  gt9xxx_wr_reg(GT9XXX_GSTID_REG, &status, 1);

  /* ALIENTEK reference driver layout for the 7" RGB panel:
   *   x = (buf[1] << 8) | buf[0]
   *   y = (buf[3] << 8) | buf[2]
   */

  *x = ((int)buf[1] << 8) | buf[0];
  *y = ((int)buf[3] << 8) | buf[2];

  /* Reject out-of-range samples */

  if (*x > 4095 || *y > 4095)
    {
      return 0;
    }

  *pressed = true;

  return 1;
}
