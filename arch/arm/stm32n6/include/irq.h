/****************************************************************************
 * arch/arm/include/stm32n6/irq.h
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
 ****************************************************************************/

#ifndef __ARCH_ARM_INCLUDE_STM32N6_IRQ_H
#define __ARCH_ARM_INCLUDE_STM32N6_IRQ_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Processor exceptions (vectors 0-15) */

#define STM32_IRQ_RESERVED       (0)
#define STM32_IRQ_RESET          (1)
#define STM32_IRQ_NMI            (2)
#define STM32_IRQ_HARDFAULT      (3)
#define STM32_IRQ_MEMFAULT       (4)
#define STM32_IRQ_BUSFAULT       (5)
#define STM32_IRQ_USAGEFAULT     (6)
#define STM32_IRQ_SECUREFAULT    (7)
#define STM32_IRQ_SVCALL        (11)
#define STM32_IRQ_DBGMONITOR    (12)
#define STM32_IRQ_PENDSV        (14)
#define STM32_IRQ_SYSTICK       (15)

#define STM32_IRQ_FIRST         (16)

/* External interrupts (vector >= 16).
 * Numbering follows ST CMSIS stm32n647xx.h IRQn_Type enum.
 */

#define STM32_IRQ_PVD_PVM          (16 + 0)
#define STM32_IRQ_DTS              (16 + 2)
#define STM32_IRQ_RCC              (16 + 3)
#define STM32_IRQ_LOCKUP           (16 + 4)
#define STM32_IRQ_CACHE_ECC        (16 + 5)
#define STM32_IRQ_TCM_ECC          (16 + 6)
#define STM32_IRQ_BKP_ECC          (16 + 7)
#define STM32_IRQ_FPU              (16 + 8)
#define STM32_IRQ_RTC_S            (16 + 10)
#define STM32_IRQ_TAMP             (16 + 11)
#define STM32_IRQ_RIFSC_TAMPER     (16 + 12)
#define STM32_IRQ_IAC              (16 + 13)
#define STM32_IRQ_RCC_S            (16 + 14)
#define STM32_IRQ_RTC              (16 + 16)
#define STM32_IRQ_IWDG             (16 + 18)
#define STM32_IRQ_WWDG             (16 + 19)
#define STM32_IRQ_EXTI0            (16 + 20)
#define STM32_IRQ_EXTI1            (16 + 21)
#define STM32_IRQ_EXTI2            (16 + 22)
#define STM32_IRQ_EXTI3            (16 + 23)
#define STM32_IRQ_EXTI4            (16 + 24)
#define STM32_IRQ_EXTI5            (16 + 25)
#define STM32_IRQ_EXTI6            (16 + 26)
#define STM32_IRQ_EXTI7            (16 + 27)
#define STM32_IRQ_EXTI8            (16 + 28)
#define STM32_IRQ_EXTI9            (16 + 29)
#define STM32_IRQ_EXTI10           (16 + 30)
#define STM32_IRQ_EXTI11           (16 + 31)
#define STM32_IRQ_EXTI12           (16 + 32)
#define STM32_IRQ_EXTI13           (16 + 33)
#define STM32_IRQ_EXTI14           (16 + 34)
#define STM32_IRQ_EXTI15           (16 + 35)
#define STM32_IRQ_PKA              (16 + 38)
#define STM32_IRQ_HASH             (16 + 39)
#define STM32_IRQ_RNG              (16 + 40)
#define STM32_IRQ_ADC1_2           (16 + 46)
#define STM32_IRQ_CSI              (16 + 47)
#define STM32_IRQ_DCMIPP           (16 + 48)
#define STM32_IRQ_PAHB_ERR         (16 + 52)
#define STM32_IRQ_NPU0             (16 + 53)
#define STM32_IRQ_NPU1             (16 + 54)
#define STM32_IRQ_NPU2             (16 + 55)
#define STM32_IRQ_NPU3             (16 + 56)
#define STM32_IRQ_CACHEAXI         (16 + 57)
#define STM32_IRQ_LTDC_LO          (16 + 58)
#define STM32_IRQ_LTDC_LO_ERR      (16 + 59)
#define STM32_IRQ_DMA2D            (16 + 60)
#define STM32_IRQ_JPEG             (16 + 61)
#define STM32_IRQ_VENC             (16 + 62)
#define STM32_IRQ_GFXMMU           (16 + 63)
#define STM32_IRQ_GFXTIM           (16 + 64)
#define STM32_IRQ_GPU2D            (16 + 65)
#define STM32_IRQ_GPU2D_ER         (16 + 66)
#define STM32_IRQ_ICACHE           (16 + 67)
#define STM32_IRQ_HPDMA1_CH0       (16 + 68)
#define STM32_IRQ_HPDMA1_CH1       (16 + 69)
#define STM32_IRQ_HPDMA1_CH2       (16 + 70)
#define STM32_IRQ_HPDMA1_CH3       (16 + 71)
#define STM32_IRQ_HPDMA1_CH4       (16 + 72)
#define STM32_IRQ_HPDMA1_CH5       (16 + 73)
#define STM32_IRQ_HPDMA1_CH6       (16 + 74)
#define STM32_IRQ_HPDMA1_CH7       (16 + 75)
#define STM32_IRQ_HPDMA1_CH8       (16 + 76)
#define STM32_IRQ_HPDMA1_CH9       (16 + 77)
#define STM32_IRQ_HPDMA1_CH10      (16 + 78)
#define STM32_IRQ_HPDMA1_CH11      (16 + 79)
#define STM32_IRQ_HPDMA1_CH12      (16 + 80)
#define STM32_IRQ_HPDMA1_CH13      (16 + 81)
#define STM32_IRQ_HPDMA1_CH14      (16 + 82)
#define STM32_IRQ_HPDMA1_CH15      (16 + 83)
#define STM32_IRQ_GPDMA1_CH0       (16 + 84)
#define STM32_IRQ_GPDMA1_CH1       (16 + 85)
#define STM32_IRQ_GPDMA1_CH2       (16 + 86)
#define STM32_IRQ_GPDMA1_CH3       (16 + 87)
#define STM32_IRQ_GPDMA1_CH4       (16 + 88)
#define STM32_IRQ_GPDMA1_CH5       (16 + 89)
#define STM32_IRQ_GPDMA1_CH6       (16 + 90)
#define STM32_IRQ_GPDMA1_CH7       (16 + 91)
#define STM32_IRQ_GPDMA1_CH8       (16 + 92)
#define STM32_IRQ_GPDMA1_CH9       (16 + 93)
#define STM32_IRQ_GPDMA1_CH10      (16 + 94)
#define STM32_IRQ_GPDMA1_CH11      (16 + 95)
#define STM32_IRQ_GPDMA1_CH12      (16 + 96)
#define STM32_IRQ_GPDMA1_CH13      (16 + 97)
#define STM32_IRQ_GPDMA1_CH14      (16 + 98)
#define STM32_IRQ_GPDMA1_CH15      (16 + 99)
#define STM32_IRQ_I2C1_EV          (16 + 100)
#define STM32_IRQ_I2C1_ER          (16 + 101)
#define STM32_IRQ_I2C2_EV          (16 + 102)
#define STM32_IRQ_I2C2_ER          (16 + 103)
#define STM32_IRQ_I2C3_EV          (16 + 104)
#define STM32_IRQ_I2C3_ER          (16 + 105)
#define STM32_IRQ_I2C4_EV          (16 + 106)
#define STM32_IRQ_I2C4_ER          (16 + 107)
#define STM32_IRQ_I3C1_EV          (16 + 108)
#define STM32_IRQ_I3C1_ER          (16 + 109)
#define STM32_IRQ_I3C2_EV          (16 + 110)
#define STM32_IRQ_I3C2_ER          (16 + 111)
#define STM32_IRQ_TIM1_BRK         (16 + 112)
#define STM32_IRQ_TIM1_UP          (16 + 113)
#define STM32_IRQ_TIM1_TRG_COM     (16 + 114)
#define STM32_IRQ_TIM1_CC          (16 + 115)
#define STM32_IRQ_TIM2             (16 + 116)
#define STM32_IRQ_TIM3             (16 + 117)
#define STM32_IRQ_TIM4             (16 + 118)
#define STM32_IRQ_TIM5             (16 + 119)
#define STM32_IRQ_TIM6             (16 + 120)
#define STM32_IRQ_TIM7             (16 + 121)
#define STM32_IRQ_TIM8_BRK         (16 + 122)
#define STM32_IRQ_TIM8_UP          (16 + 123)
#define STM32_IRQ_TIM8_TRG_COM     (16 + 124)
#define STM32_IRQ_TIM8_CC          (16 + 125)
#define STM32_IRQ_TIM9             (16 + 126)
#define STM32_IRQ_TIM10            (16 + 127)
#define STM32_IRQ_TIM11            (16 + 128)
#define STM32_IRQ_TIM12            (16 + 129)
#define STM32_IRQ_TIM13            (16 + 130)
#define STM32_IRQ_TIM14            (16 + 131)
#define STM32_IRQ_TIM15            (16 + 132)
#define STM32_IRQ_TIM16            (16 + 133)
#define STM32_IRQ_TIM17            (16 + 134)
#define STM32_IRQ_TIM18            (16 + 135)
#define STM32_IRQ_LPTIM1           (16 + 136)
#define STM32_IRQ_LPTIM2           (16 + 137)
#define STM32_IRQ_LPTIM3           (16 + 138)
#define STM32_IRQ_LPTIM4           (16 + 139)
#define STM32_IRQ_LPTIM5           (16 + 140)
#define STM32_IRQ_ADF1_FLT0        (16 + 141)
#define STM32_IRQ_MDF1_FLT0        (16 + 142)
#define STM32_IRQ_MDF1_FLT1        (16 + 143)
#define STM32_IRQ_MDF1_FLT2        (16 + 144)
#define STM32_IRQ_MDF1_FLT3        (16 + 145)
#define STM32_IRQ_MDF1_FLT4        (16 + 146)
#define STM32_IRQ_MDF1_FLT5        (16 + 147)
#define STM32_IRQ_SAI1_A           (16 + 148)
#define STM32_IRQ_SAI1_B           (16 + 149)
#define STM32_IRQ_SAI2_A           (16 + 150)
#define STM32_IRQ_SAI2_B           (16 + 151)
#define STM32_IRQ_SPDIFRX1         (16 + 152)
#define STM32_IRQ_SPI1             (16 + 153)
#define STM32_IRQ_SPI2             (16 + 154)
#define STM32_IRQ_SPI3             (16 + 155)
#define STM32_IRQ_SPI4             (16 + 156)
#define STM32_IRQ_SPI5             (16 + 157)
#define STM32_IRQ_SPI6             (16 + 158)
#define STM32_IRQ_USART1           (16 + 159)
#define STM32_IRQ_USART2           (16 + 160)
#define STM32_IRQ_USART3           (16 + 161)
#define STM32_IRQ_UART4            (16 + 162)
#define STM32_IRQ_UART5            (16 + 163)
#define STM32_IRQ_USART6           (16 + 164)
#define STM32_IRQ_UART7            (16 + 165)
#define STM32_IRQ_UART8            (16 + 166)
#define STM32_IRQ_UART9            (16 + 167)
#define STM32_IRQ_USART10          (16 + 168)
#define STM32_IRQ_LPUART1          (16 + 169)
#define STM32_IRQ_XSPI1            (16 + 170)
#define STM32_IRQ_XSPI2            (16 + 171)
#define STM32_IRQ_XSPI3            (16 + 172)
#define STM32_IRQ_FMC              (16 + 173)
#define STM32_IRQ_SDMMC1           (16 + 174)
#define STM32_IRQ_SDMMC2           (16 + 175)
#define STM32_IRQ_UCPD1            (16 + 176)
#define STM32_IRQ_USB1_OTG_HS      (16 + 177)
#define STM32_IRQ_USB2_OTG_HS      (16 + 178)
#define STM32_IRQ_ETH1             (16 + 179)
#define STM32_IRQ_FDCAN1_IT0       (16 + 180)
#define STM32_IRQ_FDCAN1_IT1       (16 + 181)
#define STM32_IRQ_FDCAN2_IT0       (16 + 182)
#define STM32_IRQ_FDCAN2_IT1       (16 + 183)
#define STM32_IRQ_FDCAN3_IT0       (16 + 184)
#define STM32_IRQ_FDCAN3_IT1       (16 + 185)
#define STM32_IRQ_FDCAN_CU         (16 + 186)
#define STM32_IRQ_MDIOS            (16 + 187)
#define STM32_IRQ_DCMI_PSSI        (16 + 188)
#define STM32_IRQ_WAKEUP_PIN       (16 + 189)
#define STM32_IRQ_CTI_INT0         (16 + 190)
#define STM32_IRQ_CTI_INT1         (16 + 191)
#define STM32_IRQ_LTDC_UP          (16 + 193)
#define STM32_IRQ_LTDC_UP_ERR      (16 + 194)

#define STM32_IRQ_NEXTINTS         196
#define NR_IRQS                    (16 + 196)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif

#endif /* __ARCH_ARM_INCLUDE_STM32N6_IRQ_H */
