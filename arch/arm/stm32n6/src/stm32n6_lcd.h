/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_lcd.h
 *
 * Minimal LCD lower-half for the 7" RGB panel (/dev/lcd0).
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_LCD_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_LCD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/lcd/lcd.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* LCD lower-half instance (registered by the board as /dev/lcd0). */

extern struct lcd_dev_s g_stm32n6_lcd;

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_LCD_H */
