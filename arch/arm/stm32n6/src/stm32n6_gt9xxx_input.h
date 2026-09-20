/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_gt9xxx_input.h
 *
 * GT9xxx touchscreen lower-half driver (registers /dev/input0).
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_GT9XXX_INPUT_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_GT9XXX_INPUT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_gt9xxx_input_register
 *
 * Description:
 *   Register the GT9xxx as the standard touchscreen device /dev/input0
 *   and start the polling thread.
 *
 ****************************************************************************/

int stm32n6_gt9xxx_input_register(void);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_GT9XXX_INPUT_H */
