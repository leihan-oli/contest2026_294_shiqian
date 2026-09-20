/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_gt9xxx.h
 *
 * GT9xxx capacitive touch controller driver.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_GT9XXX_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_GT9XXX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdbool.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_gt9xxx_initialize
 *
 * Description:
 *   Configure pins, reset the GT9xxx, verify product ID.
 *
 ****************************************************************************/

int stm32n6_gt9xxx_initialize(void);

/****************************************************************************
 * Name: stm32n6_gt9xxx_scan
 *
 * Description:
 *   Poll for a touch.  Returns 1 + fills *x / *y / *pressed on a touch,
 *   otherwise 0 with *pressed=false.
 *
 ****************************************************************************/

int stm32n6_gt9xxx_scan(FAR int *x, FAR int *y, FAR bool *pressed);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_GT9XXX_H */
