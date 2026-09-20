/****************************************************************************
 * libs/ai_aton/compat/stm32n6xx.h
 *
 * Minimal stand-in for the ST CMSIS device header "stm32n6xx.h".  The
 * vendor ATON platform layer (vendor/ll_aton/ll_aton_platform.h) includes
 * that header when it is built for LL_ATON_PLAT_STM32N6, but NuttX does not
 * carry the ST Cube CMSIS device tree.  Only two things are consumed:
 *
 *   1. __STM32N6xx_HAL_VERSION - selects the NPU interrupt naming scheme
 *      (NPU0..NPU3 for HAL >= v2.0.0).  Only the vendor reference OSAL
 *      implementations use those names; our OSAL (ll_aton_osal_nuttx.c)
 *      routes ATON line n to STM32_IRQ_NPU0 + n instead.
 *
 *   2. NPU_BASE_S / NPU_BASE_NS - the Neural-ART register window, from
 *      which the platform layer derives ATON_BASE (secure alias when
 *      CPU_IN_SECURE_STATE is defined; the NuttX port always runs the
 *      CPU in secure state).
 *
 * Everything else the ATON runtime needs (interrupt control, cache
 * maintenance, mutexes/semaphores) is provided by the NuttX OSAL port, so
 * no NVIC_*, SCB_* or HAL_* symbol has to come from this header.
 *
 * NOTE: this directory must be added to the include search path *before*
 *       the vendor directories, so that the quoted includes in the vendor
 *       headers resolve here.
 *
 ****************************************************************************/

#ifndef __LIBS_AI_ATON_COMPAT_STM32N6XX_H
#define __LIBS_AI_ATON_COMPAT_STM32N6XX_H

/****************************************************************************
 * Version - keep in sync with the ST HAL release that the ATON middleware
 * was validated against (STM32Cube FW N6 V1.0.0).
 ****************************************************************************/

#define __STM32N6xx_HAL_VERSION_MAIN   (0x01U)
#define __STM32N6xx_HAL_VERSION_SUB1   (0x00U)
#define __STM32N6xx_HAL_VERSION_SUB2   (0x00U)
#define __STM32N6xx_HAL_VERSION_RC     (0x00U)

#define __STM32N6xx_HAL_VERSION \
  ((__STM32N6xx_HAL_VERSION_MAIN << 24U) | \
   (__STM32N6xx_HAL_VERSION_SUB1 << 16U) | \
   (__STM32N6xx_HAL_VERSION_SUB2 << 8U) | \
   (__STM32N6xx_HAL_VERSION_RC))

/****************************************************************************
 * Bus bases (secure alias, matching the NuttX stm32n6 chip headers)
 ****************************************************************************/

#ifndef PERIPH_BASE_S
#  define PERIPH_BASE_S               0x50000000UL
#endif

#ifndef PERIPH_BASE_NS
#  define PERIPH_BASE_NS              0x40000000UL
#endif

#ifndef AHB5PERIPH_BASE_S
#  define AHB5PERIPH_BASE_S           (PERIPH_BASE_S + 0x08020000UL)
#endif

#ifndef AHB5PERIPH_BASE_NS
#  define AHB5PERIPH_BASE_NS          (PERIPH_BASE_NS + 0x08020000UL)
#endif

/* Neural-ART NPU register window: AHB5 + 0x0C0000 (RM0486 memory map). */

#ifndef NPU_BASE_S
#  define NPU_BASE_S                  (AHB5PERIPH_BASE_S + 0x0C0000UL)
#endif

#ifndef NPU_BASE_NS
#  define NPU_BASE_NS                 (AHB5PERIPH_BASE_NS + 0x0C0000UL)
#endif

/* The NuttX port of the STM32N6 runs in (and is only supported in) the
 * secure state - the FSBL hands over to NuttX with the CPU in secure mode
 * and all peripherals are accessed through the secure aliases.
 */

#ifndef CPU_IN_SECURE_STATE
#  define CPU_IN_SECURE_STATE         1
#endif

#endif /* __LIBS_AI_ATON_COMPAT_STM32N6XX_H */
