/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_dcmipp.h
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
 * STM32N6 DCMIPP / CSI-2 register definitions.
 * Extracted from STM32Cube_FW_N6 stm32n647xx.h (CSI + DCMIPP).
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_DCMIPP_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_DCMIPP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

#include "stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* CMSIS __IO compatibility for the register structs below */

#ifndef __IO
#  define __IO volatile
#endif

/* DCMIPP / CSI-2 host live on APB5.  This port uses the secure alias
 * (STM32_PERIPH_BASE = 0x50000000) as for all other peripherals.
 */

#define STM32_DCMIPP_BASE      (STM32_APB5_BASE + 0x2000u)
#define STM32_CSI_BASE         (STM32_APB5_BASE + 0x6000u)

/* DCMIPP instance macro (NuttX style) */

#define DCMIPP                 ((DCMIPP_TypeDef *)STM32_DCMIPP_BASE)

/* CSI-2 host instance macro */

#define CSI                    ((CSI_TypeDef *)STM32_CSI_BASE)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/*
 * @brief  (CSI)
 */
typedef struct
{
  __IO uint32_t CR;               /*!< CSI-2 Host control register                           Address offset: 0x0000 */
  __IO uint32_t PCR;              /*!< CSI-2 Host DPHY_RX control register                   Address offset: 0x0004 */
       uint32_t RESERVED0[2];     /*!< Reserved                                              Address offset: 0x0008-0x000C */
  __IO uint32_t VC0CFGR1;         /*!< CSI-2 Host virtual channel 0 configuration register 1 Address offset: 0x0010 */
  __IO uint32_t VC0CFGR2;         /*!< CSI-2 Host virtual channel 0 configuration register 2 Address offset: 0x0014 */
  __IO uint32_t VC0CFGR3;         /*!< CSI-2 Host virtual channel 0 configuration register 3 Address offset: 0x0018 */
  __IO uint32_t VC0CFGR4;         /*!< CSI-2 Host virtual channel 0 configuration register 4 Address offset: 0x001C */
  __IO uint32_t VC1CFGR1;         /*!< CSI-2 Host virtual channel 1 configuration register 1 Address offset: 0x0020 */
  __IO uint32_t VC1CFGR2;         /*!< CSI-2 Host virtual channel 1 configuration register 2 Address offset: 0x0024 */
  __IO uint32_t VC1CFGR3;         /*!< CSI-2 Host virtual channel 1 configuration register 3 Address offset: 0x0028 */
  __IO uint32_t VC1CFGR4;         /*!< CSI-2 Host virtual channel 1 configuration register 4 Address offset: 0x002C */
  __IO uint32_t VC2CFGR1;         /*!< CSI-2 Host virtual channel 2 configuration register 1 Address offset: 0x0030 */
  __IO uint32_t VC2CFGR2;         /*!< CSI-2 Host virtual channel 2 configuration register 2 Address offset: 0x0034 */
  __IO uint32_t VC2CFGR3;         /*!< CSI-2 Host virtual channel 2 configuration register 3 Address offset: 0x0038 */
  __IO uint32_t VC2CFGR4;         /*!< CSI-2 Host virtual channel 2 configuration register 4 Address offset: 0x003C */
  __IO uint32_t VC3CFGR1;         /*!< CSI-2 Host virtual channel 3 configuration register 1 Address offset: 0x0040 */
  __IO uint32_t VC3CFGR2;         /*!< CSI-2 Host virtual channel 3 configuration register 2 Address offset: 0x0044 */
  __IO uint32_t VC3CFGR3;         /*!< CSI-2 Host virtual channel 3 configuration register 3 Address offset: 0x0048 */
  __IO uint32_t VC3CFGR4;         /*!< CSI-2 Host virtual channel 3 configuration register 4 Address offset: 0x004C */
  __IO uint32_t LB0CFGR;          /*!< CSI-2 Host line byte 0 configuration register         Address offset: 0x0050 */
  __IO uint32_t LB1CFGR;          /*!< CSI-2 Host line byte 1 configuration register         Address offset: 0x0054 */
  __IO uint32_t LB2CFGR;          /*!< CSI-2 Host line byte 2 configuration register         Address offset: 0x0058 */
  __IO uint32_t LB3CFGR;          /*!< CSI-2 Host line byte 3 configuration register         Address offset: 0x005C */
  __IO uint32_t TIM0CFGR;         /*!< CSI-2 Host timer 0 configuration register             Address offset: 0x0060 */
  __IO uint32_t TIM1CFGR;         /*!< CSI-2 Host timer 1 configuration register             Address offset: 0x0064 */
  __IO uint32_t TIM2CFGR;         /*!< CSI-2 Host timer 2 configuration register             Address offset: 0x0068 */
  __IO uint32_t TIM3CFGR;         /*!< CSI-2 Host timer 3 configuration register             Address offset: 0x006C */
  __IO uint32_t LMCFGR;           /*!< CSI-2 Host lane merger configuration register         Address offset: 0x0070 */
  __IO uint32_t PRGITR;           /*!< CSI-2 Host program interrupt register                 Address offset: 0x0074 */
  __IO uint32_t WDR;              /*!< CSI-2 Host watchdog register                          Address offset: 0x0078 */
       uint32_t RESERVED1;        /*!< Reserved                                              Address offset: 0x007C */
  __IO uint32_t IER0;             /*!< CSI-2 Host Interrupt enable register 0                Address offset: 0x0080 */
  __IO uint32_t IER1;             /*!< CSI-2 Host Interrupt enable register 1                Address offset: 0x0084 */
       uint32_t RESERVED2[2];     /*!< Reserved                                              Address offset: 0x0088-0x008C */
  __IO uint32_t SR0;              /*!< CSI-2 Host status register 0                          Address offset: 0x0090 */
  __IO uint32_t SR1;              /*!< CSI-2 Host status register 1                          Address offset: 0x0094 */
       uint32_t RESERVED3[26];    /*!< Reserved                                              Address offset: 0x0098-0x00FC */
  __IO uint32_t FCR0;             /*!< CSI-2 Host Flag clear register 0                      Address offset: 0x0100 */
  __IO uint32_t FCR1;             /*!< CSI-2 Host Flag clear register 1                      Address offset: 0x0104 */
       uint32_t RESERVED4[2];     /*!< Reserved                                              Address offset: 0x0108-0x010C */
  __IO uint32_t SPDFR;            /*!< CSI-2 Host short packet data field register           Address offset: 0x0110 */
  __IO uint32_t ERR1;             /*!< CSI-2 Host error register 1                           Address offset: 0x0114 */
  __IO uint32_t ERR2;             /*!< CSI-2 Host error register 2                           Address offset: 0x0118 */
       uint32_t RESERVED5[953];   /*!< Reserved                                              Address offset: 0x011C-0x0FFC */
  __IO uint32_t PRCR;             /*!< CSI PHY reset control register                        Address offset: 0x1000 */
  __IO uint32_t PMCR;             /*!< CSI PHY mode control register                         Address offset: 0x1004 */
  __IO uint32_t PFCR;             /*!< CSI PHY frequency control register                    Address offset: 0x1008 */
       uint32_t RESERVED6;        /*!< Reserved                                              Address offset: 0x100C */
  __IO uint32_t PTCR0;            /*!< CSI PHY test control register 0                       Address offset: 0x1010 */
  __IO uint32_t PTCR1;            /*!< CSI PHY test control register 1                       Address offset: 0x1014 */
  __IO uint32_t PTSR;             /*!< CSI PHY test status register                          Address offset: 0x1018 */
       uint32_t RESERVED7[1017];  /*!< Reserved                                              Address offset: 0x101C-0x1FFC */
} CSI_TypeDef;

/**
  * @brief Debug MCU
  */
typedef struct
{
  __IO uint32_t IDCODE;        /*!< MCU device ID code,                            Address offset: 0x00  */
  __IO uint32_t CR;            /*!< Debug MCU configuration register,              Address offset: 0x04  */
  uint32_t RESERVED1[2];       /*!< Reserved,                                  Address offset: 0x08-0x0C */
  __IO uint32_t APB1LFZ1;      /*!< Debug MCU APB1LFZ1 freeze register,            Address offset: 0x10  */
  __IO uint32_t APB1HFZ1;      /*!< Debug MCU APB1HFZ1 freeze register,            Address offset: 0x14  */
  __IO uint32_t APB2FZ1;       /*!< Debug MCU APB2FZ1 freeze register,             Address offset: 0x18  */
  __IO uint32_t APB4FZ1;       /*!< Debug MCU APB4FZ1 freeze register,             Address offset: 0x1C  */
  __IO uint32_t APB5FZ1;       /*!< Debug MCU APB5FZ1 freeze register,             Address offset: 0x20  */
  __IO uint32_t AHB1FZ1;       /*!< Debug MCU AHB1FZ1 freeze register,             Address offset: 0x24  */
  __IO uint32_t AHB5FZ1;       /*!< Debug MCU AHB5FZ1 freeze register,             Address offset: 0x28  */
  uint32_t RESERVED2[52];      /*!< Reserved,                                  Address offset: 0x2C-0xF8 */
  __IO uint32_t SR;            /*!< Debug MCU status register,                     Address offset: 0xFC  */
  __IO uint32_t DBG_AUTH_HOST; /*!< Debug MCU authentication host register,        Address offset: 0x100 */
  __IO uint32_t DBG_AUTH_DEV;  /*!< Debug MCU authentication device register,      Address offset: 0x104 */
  __IO uint32_t DBG_AUTH_ACK;  /*!< Debug MCU acknowledge authentication register, Address offset: 0x104 */
} DBGMCU_TypeDef;

/**
  * @brief DCMI
  */
typedef struct
{
  __IO uint32_t CR;       /*!< DCMI control register 1,                       Address offset: 0x00 */
  __IO uint32_t SR;       /*!< DCMI status register,                          Address offset: 0x04 */
  __IO uint32_t RISR;     /*!< DCMI raw interrupt status register,            Address offset: 0x08 */
  __IO uint32_t IER;      /*!< DCMI interrupt enable register,                Address offset: 0x0C */
  __IO uint32_t MISR;     /*!< DCMI masked interrupt status register,         Address offset: 0x10 */
  __IO uint32_t ICR;      /*!< DCMI interrupt clear register,                 Address offset: 0x14 */
  __IO uint32_t ESCR;     /*!< DCMI embedded synchronization code register,   Address offset: 0x18 */
  __IO uint32_t ESUR;     /*!< DCMI embedded synchronization unmask register, Address offset: 0x1C */
  __IO uint32_t CWSTRTR;  /*!< DCMI crop window start,                        Address offset: 0x20 */
  __IO uint32_t CWSIZER;  /*!< DCMI crop window size,                         Address offset: 0x24 */
  __IO uint32_t DR;       /*!< DCMI data register,                            Address offset: 0x28 */
} DCMI_TypeDef;

#define DCMIPP_NUM_OF_PIPES 0x03U

typedef struct
{
  uint32_t PxRIxCR1;      /*! DCMIPP Pipex ROIx configuration register 1  Address offset: 0x924 + (x - 1) * 0x400, (x = 1 to 2)  */
  uint32_t PxRIxCR2;      /*! DCMIPP Pipex ROIx configuration register 2  Address offset: 0x928 + (x - 1) * 0x400, (x = 1 to 2)  */
} DCMIPP_Region_TypeDef;

/*
 * @brief Digital camera interface pixel pipeline DCMIPP
 */
typedef struct
{
  __IO uint32_t IPGR1;           /*!< DCMIPP IPPLUG global register 1                                    Address offset: 0x000 */
  __IO uint32_t IPGR2;           /*!< DCMIPP IPPLUG global register 2                                    Address offset: 0x004 */
  __IO uint32_t IPGR3;           /*!< DCMIPP IPPLUG global register 3                                    Address offset: 0x008 */
       uint32_t RESERVED0[4];    /*!< Reserved                                                           Address offset: 0x00C-0x018 */
  __IO uint32_t IPGR8;           /*!< DCMIPP IPPLUG identification register                              Address offset: 0x01C */
  __IO uint32_t IPC1R1;          /*!< DCMIPP IPPLUG Clientx register 1                                   Address offset: 0x020 + 0x10 * (x - 1), (x = 1 to 5) */
  __IO uint32_t IPC1R2;          /*!< DCMIPP IPPLUG Clientx register 2                                   Address offset: 0x024 + 0x10 * (x - 1), (x = 1 to 5) */
  __IO uint32_t IPC1R3;          /*!< DCMIPP IPPLUG Clientx register 3                                   Address offset: 0x028 + 0x10 * (x - 1), (x = 1 to 5) */
       uint32_t RESERVED1;       /*!< Reserved                                                           Address offset: 0x02C */
  __IO uint32_t IPC2R1;          /*!< DCMIPP IPPLUG Clientx register 1                                   Address offset: 0x030 */
  __IO uint32_t IPC2R2;          /*!< DCMIPP IPPLUG Clientx register 2                                   Address offset: 0x034 */
  __IO uint32_t IPC2R3;          /*!< DCMIPP IPPLUG Clientx register 3                                   Address offset: 0x038 */
       uint32_t RESERVED2;       /*!< Reserved                                                           Address offset: 0x03C */
  __IO uint32_t IPC3R1;          /*!< DCMIPP IPPLUG Clientx register 1                                   Address offset: 0x040 */
  __IO uint32_t IPC3R2;          /*!< DCMIPP IPPLUG Clientx register 2                                   Address offset: 0x044 */
  __IO uint32_t IPC3R3;          /*!< DCMIPP IPPLUG Clientx register 3                                   Address offset: 0x048 */
       uint32_t RESERVED3;       /*!< Reserved                                                           Address offset: 0x04C */
  __IO uint32_t IPC4R1;          /*!< DCMIPP IPPLUG Clientx register 1                                   Address offset: 0x050 */
  __IO uint32_t IPC4R2;          /*!< DCMIPP IPPLUG Clientx register 2                                   Address offset: 0x054 */
  __IO uint32_t IPC4R3;          /*!< DCMIPP IPPLUG Clientx register 3                                   Address offset: 0x058 */
       uint32_t RESERVED4;       /*!< Reserved                                                           Address offset: 0x05C */
  __IO uint32_t IPC5R1;          /*!< DCMIPP IPPLUG Clientx register 1                                   Address offset: 0x060 */
  __IO uint32_t IPC5R2;          /*!< DCMIPP IPPLUG Clientx register 2                                   Address offset: 0x064 */
  __IO uint32_t IPC5R3;          /*!< DCMIPP IPPLUG Clientx register 3                                   Address offset: 0x068 */
       uint32_t RESERVED5[38];   /*!< Reserved                                                           Address offset: 0x06C-0x100 */
  __IO uint32_t PRCR;            /*!< DCMIPP parallel interface control register                         Address offset: 0x104 */
  __IO uint32_t PRESCR;          /*!< DCMIPP parallel interface embedded synchronization code register   Address offset: 0x108 */
  __IO uint32_t PRESUR;          /*!< DCMIPP parallel interface embedded synchronization unmask register Address offset: 0x10C */
       uint32_t RESERVED6[57];   /*!< Reserved                                                           Address offset: 0x110-0x1F0 */
  __IO uint32_t PRIER;           /*!< DCMIPP parallel interface interrupt enable register                Address offset: 0x1F4 */
  __IO uint32_t PRSR;            /*!< DCMIPP parallel interface status register                          Address offset: 0x1F8 */
  __IO uint32_t PRFCR;           /*!< DCMIPP parallel interface interrupt clear register                 Address offset: 0x1FC */
       uint32_t RESERVED7;       /*!< Reserved                                                           Address offset: 0x200 */
  __IO uint32_t CMCR;            /*!< DCMIPP common configuration register                               Address offset: 0x204 */
  __IO uint32_t CMFRCR;          /*!< DCMIPP common frame counter register                               Address offset: 0x208 */
       uint32_t RESERVED8[121];  /*!< Reserved                                                           Address offset: 0x20C-0x3EC */
  __IO uint32_t CMIER;           /*!< DCMIPP common interrupt enable register                            Address offset: 0x3F0 */
  __IO uint32_t CMSR1;           /*!< DCMIPP common status register 1                                    Address offset: 0x3F4 */
  __IO uint32_t CMSR2;           /*!< DCMIPP common status register 2                                    Address offset: 0x3F8 */
  __IO uint32_t CMFCR;           /*!< DCMIPP common interrupt clear register                             Address offset: 0x3FC */
       uint32_t RESERVED9;            /*!< Reserved                                                      Address offset: 0x400 */
  __IO uint32_t P0FSCR;          /*!< DCMIPP Pipe0 flow selection configuration register                 Address offset: 0x404 */
       uint32_t RESERVED10[62];  /*!< Reserved                                                           Address offset: 0x408-0x4FC */
  __IO uint32_t P0FCTCR;         /*!< DCMIPP Pipe0 flow control configuration register                   Address offset: 0x500 */
  __IO uint32_t P0SCSTR;         /*!< DCMIPP Pipe0 stat/crop start register                              Address offset: 0x504 */
  __IO uint32_t P0SCSZR;         /*!< DCMIPP Pipe0 stat/crop size register                               Address offset: 0x508 */
       uint32_t RESERVED11[41];  /*!< Reserved                                                           Address offset: 0x50C-0x5AC */
  __IO uint32_t P0DCCNTR;        /*!< DCMIPP Pipe0 dump counter register                                 Address offset: 0x5B0 */
  __IO uint32_t P0DCLMTR;        /*!< DCMIPP Pipe0 dump limit register                                   Address offset: 0x5B4 */
       uint32_t RESERVED12[2];   /*!< Reserved                                                           Address offset: 0x5B8-0x5BC */
  __IO uint32_t P0PPCR;          /*!< DCMIPP Pipe0 pixel packer configuration register                   Address offset: 0x5C0 */
  __IO uint32_t P0PPM0AR1;       /*!< DCMIPP Pipe0 pixel packer Memory0 address register 1               Address offset: 0x5C4 */
  __IO uint32_t P0PPM0AR2;       /*!< DCMIPP Pipe0 pixel packer Memory0 address register 2               Address offset: 0x5C8 */
       uint32_t RESERVED13;      /*!< Reserved                                                           Address offset: 0x5C8-0x5CC */
  __IO uint32_t P0STM0AR;        /*!< DCMIPP Pipe0 status Memory0 address register                       Address offset: 0x5D0 */
       uint32_t RESERVED14[8];   /*!< Reserved                                                           Address offset: 0x5D4-0x5F0 */
  __IO uint32_t P0IER;           /*!< DCMIPP Pipe0 interrupt enable register                             Address offset: 0x5F4 */
  __IO uint32_t P0SR;            /*!< DCMIPP Pipe0 status register                                       Address offset: 0x5F8 */
  __IO uint32_t P0FCR;           /*!< DCMIPP Pipe0 interrupt clear register                              Address offset: 0x5FC */
       uint32_t RESERVED15;      /*!< Reserved                                                           Address offset: 0x600 */
  __IO uint32_t P0CFSCR;         /*!< DCMIPP Pipe0 current flow selection configuration register         Address offset: 0x604 */
       uint32_t RESERVED17[62];  /*!< Reserved                                                           Address offset: 0x608-0x6FC */
  __IO uint32_t P0CFCTCR;        /*!< DCMIPP Pipe0 current flow control configuration register           Address offset: 0x700 */
  __IO uint32_t P0CSCSTR;        /*!< DCMIPP Pipe0 current stat/crop start register                      Address offset: 0x704 */
  __IO uint32_t P0CSCSZR;        /*!< DCMIPP Pipe0 current stat/crop size register                       Address offset: 0x708 */
       uint32_t RESERVED18[45];  /*!< Reserved                                                           Address offset: 0x70C-0x7BC */
  __IO uint32_t P0CPPCR;         /*!< DCMIPP Pipe0 current pixel packer configuration register           Address offset: 0x7C0 */
  __IO uint32_t P0CPPM0AR1;      /*!< DCMIPP Pipe0 current pixel packer Memory0 address register 1       Address offset: 0x7C4 */
  __IO uint32_t P0CPPM0AR2;      /*!< DCMIPP Pipe0 current pixel packer Memory0 address register 2       Address offset: */
       uint32_t RESERVED19[14];  /*!< Reserved                                                           Address offset: 0x7C8-0x7FC */
  __IO uint32_t P1FSCR;          /*!< DCMIPP Pipe1 flow selection configuration register                 Address offset: 0x804 */
       uint32_t RESERVED20[6];   /*!< Reserved                                                           Address offset: 0x808-0x81C */
  __IO uint32_t P1SRCR;          /*!< DCMIPP Pipe1 stat removal configuration register                   Address offset: 0x820 */
  __IO uint32_t P1BPRCR;         /*!< DCMIPP Pipe1 bad pixel removal control register                    Address offset: 0x824 */
  __IO uint32_t P1BPRSR;         /*!< DCMIPP Pipe1 bad pixel removal status register                     Address offset: 0x828 */
       uint32_t RESERVED21;      /*!< Reserved                                                           Address offset: 0x82C */
  __IO uint32_t P1DECR;          /*!< DCMIPP Pipe1 decimation register                                   Address offset: 0x830 */
       uint32_t RESERVED22[3];   /*!< Reserved                                                           Address offset: 0x834-0x83C */
  __IO uint32_t P1BLCCR;         /*!< DCMIPP Pipe1 black level calibration control register              Address offset: 0x840 */
  __IO uint32_t P1EXCR1;         /*!< DCMIPP Pipe1 exposure control register 1                           Address offset: 0x844 */
  __IO uint32_t P1EXCR2;         /*!< DCMIPP Pipe1 exposure control register 2                           Address offset: 0x848 */
       uint32_t RESERVED23;      /*!< Reserved                                                           Address offset: 0x84C */
  __IO uint32_t P1ST1CR;         /*!< DCMIPP Pipe1 statistics 1 control register                         Address offset: 0x850 */
  __IO uint32_t P1ST2CR;         /*!< DCMIPP Pipe1 statistics 2 control register                         Address offset: 0x854 */
  __IO uint32_t P1ST3CR;         /*!< DCMIPP Pipe1 statistics 3 control register                         Address offset: 0x858 */
  __IO uint32_t P1STSTR;         /*!< DCMIPP Pipe1 statistics window start register                      Address offset: 0x85C */
  __IO uint32_t P1STSZR;         /*!< DCMIPP Pipe1 statistics window size register                       Address offset: 0x860 */
  __IO uint32_t P1ST1SR;         /*!< DCMIPP Pipe1 statistics 1 status register                          Address offset: 0x864 */
  __IO uint32_t P1ST2SR;         /*!< DCMIPP Pipe1 statistics 2 status register                          Address offset: 0x868 */
  __IO uint32_t P1ST3SR;         /*!< DCMIPP Pipe1 statistics 3 status register                          Address offset: 0x86C */
  __IO uint32_t P1DMCR;          /*!< DCMIPP Pipe1 demosaicing configuration register                    Address offset: 0x870 */
       uint32_t RESERVED24[3];   /*!< Reserved                                                           Address offset: 0x874-0x87C */
  __IO uint32_t P1CCCR;          /*!< DCMIPP Pipe1 ColorConv configuration register                      Address offset: 0x880 */
  __IO uint32_t P1CCRR1;         /*!< DCMIPP Pipe1 ColorConv red coefficient register 1                  Address offset: 0x884 */
  __IO uint32_t P1CCRR2;         /*!< DCMIPP Pipe1 ColorConv red coefficient register 2                  Address offset: 0x888 */
  __IO uint32_t P1CCGR1;         /*!< DCMIPP Pipe1 ColorConv green coefficient register 1                Address offset: 0x88C */
  __IO uint32_t P1CCGR2;         /*!< DCMIPP Pipe1 ColorConv green coefficient register 2                Address offset: 0x890 */
  __IO uint32_t P1CCBR1;         /*!< DCMIPP Pipe1 ColorConv blue coefficient register 1                 Address offset: 0x894 */
  __IO uint32_t P1CCBR2;         /*!< DCMIPP Pipe1 ColorConv blue coefficient register 2                 Address offset: 0x898 */
       uint32_t RESERVED25;      /*!< Reserved                                                           Address offset: 0x89C */
  __IO uint32_t P1CTCR1;         /*!< DCMIPP Pipe1 contrast control register 1                           Address offset: 0x8A0 */
  __IO uint32_t P1CTCR2;         /*!< DCMIPP Pipe1 contrast control register 2                           Address offset: 0x8A4 */
  __IO uint32_t P1CTCR3;         /*!< DCMIPP Pipe1 contrast control register 3                           Address offset: 0x8A8 */
       uint32_t RESERVED26[21];  /*!< Reserved                                                           Address offset: 0x8AC-0x8FC */
  __IO uint32_t P1FCTCR;         /*!< DCMIPP Pipe1 flow control configuration register                   Address offset: 0x900 */
  __IO uint32_t P1CRSTR;         /*!< DCMIPP Pipe1 crop window start register                            Address offset: 0x904 */
  __IO uint32_t P1CRSZR;         /*!< DCMIPP Pipe1 crop window size register                             Address offset: 0x908 */
  __IO uint32_t P1DCCR;          /*!< DCMIPP Pipe1 decimation register                                   Address offset: 0x90C */
  __IO uint32_t P1DSCR;          /*!< DCMIPP Pipe1 downsize configuration register                       Address offset: 0x910 */
  __IO uint32_t P1DSRTIOR;       /*!< DCMIPP Pipe1 downsize ratio register                               Address offset: 0x914 */
  __IO uint32_t P1DSSZR;         /*!< DCMIPP Pipe1 downsize destination size register                    Address offset: 0x918 */
       uint32_t RESERVED28;      /*!< Reserved                                                           Address offset:  */
  __IO uint32_t P1CMRICR;        /*!< DCMIPP Pipe1 common ROI configuration register                     Address offset: 0x920 */
  __IO uint32_t P1RIxCR1;        /*!< DCMIPP Pipe1 ROIx configuration register 1                         Address offset: 0x924 + (x - 1) * 0x8, (x = 1 to 8) */
  __IO uint32_t P1RIxCR2;        /*!< DCMIPP Pipe1 ROIx configuration register 2                         Address offset: 0x928 + (x - 1) * 0x8, (x = 1 to 8) */
       uint32_t RESERVED29[17];      /*!< Reserved                                                       Address offset:  */
  __IO uint32_t P1GMCR;          /*!< DCMIPP Pipe1 gamma configuration register                          Address offset: 0x970 */
       uint32_t RESERVED30[3];   /*!< Reserved                                                           Address offset: 0x974-0x97C */
  __IO uint32_t P1YUVCR;         /*!< DCMIPP Pipe1 YUVConv configuration register                        Address offset: 0x980 */
  __IO uint32_t P1YUVRR1;        /*!< DCMIPP Pipe1 YUVConv red coefficient register 1                    Address offset: 0x984 */
  __IO uint32_t P1YUVRR2;        /*!< DCMIPP Pipe1 YUVConv red coefficient register 2                    Address offset: 0x988 */
  __IO uint32_t P1YUVGR1;        /*!< DCMIPP Pipe1 YUVConv green coefficient register 1                  Address offset: 0x98C */
  __IO uint32_t P1YUVGR2;        /*!< DCMIPP Pipe1 YUVConv green coefficient register 2                  Address offset: 0x990 */
  __IO uint32_t P1YUVBR1;        /*!< DCMIPP Pipe1 YUVConv blue coefficient register 1                   Address offset: 0x994 */
  __IO uint32_t P1YUVBR2;        /*!< DCMIPP Pipe1 YUV blue coefficient register 2                       Address offset: 0x998 */
       uint32_t RESERVED31[9];   /*!< Reserved                                                           Address offset: 0x99C-0x9BC */
  __IO uint32_t P1PPCR;          /*!< DCMIPP Pipe1 pixel packer configuration register                   Address offset: 0x9C0 */
  __IO uint32_t P1PPM0AR1;       /*!< DCMIPP Pipe1 pixel packer Memory0 address register 1               Address offset: 0x9C4 */
  __IO uint32_t P1PPM0AR2;       /*!< DCMIPP Pipe1 pixel packer Memory0 address register 2               Address offset: 0x9C8 */
  __IO uint32_t P1PPM0PR;        /*!< DCMIPP Pipe1 pixel packer Memory0 pitch register                   Address offset: 0x9CC */
  __IO uint32_t P1STM0AR;        /*!< DCMIPP Pipe1 status Memory0 address register                       Address offset: 0x9D0 */
  __IO uint32_t P1PPM1AR1;       /*!< DCMIPP Pipe1 pixel packer Memory1 address register 1               Address offset: 0x9D4 */
  __IO uint32_t P1PPM1AR2;       /*!< DCMIPP Pipe1 pixel packer Memory1 address register 2               Address offset: 0x9D8 */
  __IO uint32_t P1PPM1PR;        /*!< DCMIPP Pipe1 pixel packer Memory1 pitch register                   Address offset: 0x9DC */
  __IO uint32_t P1STM1AR;        /*!< DCMIPP Pipe1 status Memory1 address register                       Address offset: 0x9E0 */
  __IO uint32_t P1PPM2AR1;       /*!< DCMIPP Pipe1 pixel packer memory2 address register 1               Address offset: 0x9E4 */
  __IO uint32_t P1PPM2AR2;       /*!< DCMIPP Pipe1 pixel packer memory2 address register 2               Address offset: 0x9E8 */
  __IO uint32_t RESERVED34;      /*!< Reserved                                                           Address offset: 0x9EC */
  __IO uint32_t P1STM2AR;        /*!< DCMIPP Pipe1 status Memory2 address register                       Address offset: 0x9F0 */
  __IO uint32_t P1IER;           /*!< DCMIPP Pipe1 interrupt enable register                             Address offset: 0x9F4 */
  __IO uint32_t P1SR;            /*!< DCMIPP Pipe1 status register                                       Address offset: 0x9F8 */
  __IO uint32_t P1FCR;           /*!< DCMIPP Pipe1 interrupt clear register                              Address offset: 0x9FC */
       uint32_t RESERVED35;      /*!< Reserved                                                           Address offset: 0xA00 */
  __IO uint32_t P1CFSCR;         /*!< DCMIPP Pipe1 current flow selection configuration register         Address offset: 0xA04 */
       uint32_t RESERVED36[7];   /*!< Reserved                                                           Address offset: 0xA08-0xA20 */
  __IO uint32_t P1CBPRCR;        /*!< DCMIPP Pipe1 current bad pixel removal register                    Address offset: 0xA24 */
       uint32_t RESERVED37[6];   /*!< Reserved                                                           Address offset: 0xA28-0xA3C */
  __IO uint32_t P1CBLCCR;        /*!< DCMIPP Pipe1 current black level calibration control register      Address offset: 0xA40 */
  __IO uint32_t P1CEXCR1;        /*!< DCMIPP Pipe1 current exposure control register 1                   Address offset: 0xA44 */
  __IO uint32_t P1CEXCR2;        /*!< DCMIPP Pipe1 current exposure control register 2                   Address offset: 0xA48 */
       uint32_t RESERVED38;      /*!< Reserved                                                           Address offset: 0xA4C */
  __IO uint32_t P1CST1CR;        /*!< DCMIPP Pipe1 current statistics 1 control register                 Address offset: 0xA50 */
  __IO uint32_t P1CST2CR;        /*!< DCMIPP Pipe1 current statistics 2 control register                 Address offset: 0xA54 */
  __IO uint32_t P1CST3CR;        /*!< DCMIPP Pipe1 current statistics 3 control register                 Address offset: 0xA58 */
  __IO uint32_t P1CSTSTR;        /*!< DCMIPP Pipe1 current statistics window start register              Address offset: 0xA5C */
  __IO uint32_t P1CSTSZR;        /*!< DCMIPP Pipe1 current statistics window size register               Address offset: 0xA60 */
       uint32_t RESERVED39[7];   /*!< Reserved                                                           Address offset: 0xA64-0xA7C */
  __IO uint32_t P1CCCCR;         /*!< DCMIPP Pipe1 current ColorConv configuration register              Address offset: 0xA80 */
  __IO uint32_t P1CCCRR1;        /*!< DCMIPP Pipe1 current ColorConv red coefficient register 1          Address offset: 0xA84 */
  __IO uint32_t P1CCCRR2;        /*!< DCMIPP Pipe1 current ColorConv red coefficient register 2          Address offset: 0xA88 */
  __IO uint32_t P1CCCGR1;        /*!< DCMIPP Pipe1 current ColorConv green coefficient register 1        Address offset: 0xA8C */
  __IO uint32_t P1CCCGR2;        /*!< DCMIPP Pipe1 current ColorConv green coefficient register 2        Address offset: 0xA90 */
  __IO uint32_t P1CCCBR1;        /*!< DCMIPP Pipe1 current ColorConv blue coefficient register 1         Address offset: 0xA94 */
  __IO uint32_t P1CCCBR2;        /*!< DCMIPP Pipe1 current ColorConv blue coefficient register 2         Address offset: 0xA98 */
       uint32_t RESERVED40;      /*!< Reserved                                                           Address offset: 0xA9C */
  __IO uint32_t P1CCTCR1;        /*!< DCMIPP Pipe1 current contrast control register 1                   Address offset: 0xAA0 */
  __IO uint32_t P1CCTCR2;        /*!< DCMIPP Pipe1 current contrast control register 2                   Address offset: 0xAA4 */
  __IO uint32_t P1CCTCR3;        /*!< DCMIPP Pipe1 current contrast control register 3                   Address offset: 0xAA8 */
       uint32_t RESERVED41[21];  /*!< Reserved                                                           Address offset: 0xAAC-0xAFC */
  __IO uint32_t P1CFCTCR;        /*!< DCMIPP Pipe1 current flow control configuration register           Address offset: 0xB00 */
  __IO uint32_t P1CCRSTR;        /*!< DCMIPP Pipe1 current crop window start register                    Address offset: 0xB04 */
  __IO uint32_t P1CCRSZR;        /*!< DCMIPP Pipe1 current crop window size register                     Address offset: 0xB08 */
  __IO uint32_t P1CDCCR;         /*!< DCMIPP Pipe1 current decimation register                           Address offset: 0xB0C */
  __IO uint32_t P1CDSCR;         /*!< DCMIPP Pipe1 current downsize configuration register               Address offset: 0xB10 */
  __IO uint32_t P1CDSRTIOR;      /*!< DCMIPP Pipe1 current downsize ratio register                       Address offset: 0xB14 */
  __IO uint32_t P1CDSSZR;        /*!< DCMIPP Pipe1 current downsize destination size register            Address offset: 0xB18 */
       uint32_t RESERVED43;      /*!< Reserved                                                           Address offset: 0xB1C */
       uint32_t P1CCMRICR;       /*!< DCMIPP Pipe1 current common ROI configuration register             Address offset: 0xB20 */
  __IO uint32_t P1CRIxCR1;       /*!< DCMIPP Pipe1 current ROIx configuration register 1                 Address offset: 0xB24 + 0x8 * (x - 1), (x = 1 to 8) */
  __IO uint32_t P1CRIxCR2;       /*!< DCMIPP Pipe1 current ROIx configuration register 2                 Address offset: 0xB28 + 0x8 * (x - 1), (x = 1 to 8) */
  uint32_t RESERVED44[37];       /*!< Reserved                                                           Address offset: 0xB64-0xBBC */
  __IO uint32_t P1CPPCR;         /*!< DCMIPP Pipe1 current pixel packer configuration register           Address offset: 0xBC0 */
  __IO uint32_t P1CPPM0AR1;      /*!< DCMIPP Pipe1 current pixel packer Memory0 address register 1       Address offset: 0xBC4 */
  __IO uint32_t P1CPPM0AR2;      /*!< DCMIPP Pipe1 current pixel packer Memory0 address register 1       Address offset: 0xBC8 */
  __IO uint32_t P1CPPM0PR;       /*!< DCMIPP Pipe1 current pixel packer Memory0 pitch register           Address offset: 0xBCC */
       uint32_t RESERVED45;      /*!< Reserved                                                           Address offset: 0xBD0 */
  __IO uint32_t P1CPPM1AR1;      /*!< DCMIPP Pipe1 current pixel packer Memory1 address register 1       Address offset: 0xBD4 */
  __IO uint32_t P1CPPM1AR2;      /*!< DCMIPP Pipe1 current pixel packer Memory1 address register 2       Address offset: 0xBD8 */
  __IO uint32_t P1CPPM1PR;       /*!< DCMIPP Pipe1 current pixel packer Memory1 pitch register           Address offset: 0xBDC */
       uint32_t RESERVED47;      /*!< Reserved                                                           Address offset: 0xBE0 */
  __IO uint32_t P1CPPM2AR1;      /*!< DCMIPP Pipe1 current pixel packer memory2 address register 1       Address offset: 0xBE4 */
  __IO uint32_t P1CPPM2AR2;      /*!< DCMIPP Pipe1 current pixel packer Memory2 address register 2       Address offset: 0xBE8 */
       uint32_t RESERVED48[6];   /*!< Reserved                                                           Address offset: 0xBE8-0xBFC */
  __IO uint32_t P2FSCR;          /*!< DCMIPP Pipe2 flow selection configuration register                 Address offset: 0xC04 */
       uint32_t RESERVED49[62];  /*!< Reserved                                                           Address offset: 0xC08-0xCFC */
  __IO uint32_t P2FCTCR;         /*!< DCMIPP Pipe2 flow control configuration register                   Address offset: 0xD00 */
  __IO uint32_t P2CRSTR;         /*!< DCMIPP Pipe2 crop window start register                            Address offset: 0xD04 */
  __IO uint32_t P2CRSZR;         /*!< DCMIPP Pipe2 crop window size register                             Address offset: 0xD08 */
  __IO uint32_t P2DCCR;          /*!< DCMIPP Pipe2 decimation register                                   Address offset: 0xD0C */
  __IO uint32_t P2DSCR;          /*!< DCMIPP Pipe2 downsize configuration register                       Address offset: 0xD10 */
  __IO uint32_t P2DSRTIOR;       /*!< DCMIPP Pipe2 downsize ratio register                               Address offset: 0xD14 */
  __IO uint32_t P2DSSZR;         /*!< DCMIPP Pipe2 downsize destination size register                    Address offset: 0xD18 */
       uint32_t RESERVED51;      /*!< Reserved                                                           Address offset:  0xD1C */
  __IO uint32_t P2CMRICR;        /*!< DCMIPP Pipe2 common ROI configuration register                     Address offset:  0xD20 */
  __IO uint32_t P2RIxCR1;        /*!< DCMIPP Pipe2 ROIx configuration register 1                         Address offset: 0xD24 + (x - 1) * 0x8, (x = 1 to 8) */
  __IO uint32_t P2RIxCR2;        /*!< DCMIPP Pipe2 ROIx configuration register 2                         Address offset: 0xD28 + (x - 1) * 0x8, (x = 1 to 8) */
       uint32_t RESERVED53[17];  /*!< Reserved                                                           Address offset: */
  __IO uint32_t P2GMCR;          /*!< DCMIPP Pipe2 gamma configuration register                          Address offset: 0xD70 */
       uint32_t RESERVED54[19];  /*!< Reserved                                                           Address offset: 0xD74-0xDBC */
  __IO uint32_t P2PPCR;          /*!< DCMIPP Pipe2 pixel packer configuration register                   Address offset: 0xDC0 */
  __IO uint32_t P2PPM0AR1;       /*!< DCMIPP Pipe2 pixel packer Memory0 address register 1               Address offset: 0xDC4 */
  __IO uint32_t P2PPM0AR2;       /*!< DCMIPP Pipe2 pixel packer Memory0 address register 2               Address offset: 0xDC8 */
  __IO uint32_t P2PPM0PR;        /*!< DCMIPP Pipe2 pixel packer Memory0 pitch register                   Address offset: 0xDCC */
  __IO uint32_t P2STM0AR;        /*!< DCMIPP Pipe2 status Memory0 address register                       Address offset: 0xDD0 */
       uint32_t RESERVED55[8];   /*!< Reserved                                                           Address offset: 0xDD4-0xDF0 */
  __IO uint32_t P2IER;           /*!< DCMIPP Pipe2 interrupt enable register                             Address offset: 0xDF4 */
  __IO uint32_t P2SR;            /*!< DCMIPP Pipe2 status register                                       Address offset: 0xDF8 */
  __IO uint32_t P2FCR;           /*!< DCMIPP Pipe2 interrupt clear register                              Address offset: 0xDFC */
       uint32_t RESERVED56;      /*!< Reserved                                                           Address offset: 0xE00 */
  __IO uint32_t P2CFSCR;         /*!< DCMIPP Pipe2 current flow selection configuration register         Address offset: 0xE04 */
       uint32_t RESERVED57[62];  /*!< Reserved                                                           Address offset: 0xE08-0xEFC */
  __IO uint32_t P2CFCTCR;        /*!< DCMIPP Pipe2 current flow control configuration register           Address offset: 0xF00 */
  __IO uint32_t P2CCRSTR;        /*!< DCMIPP Pipe2 current crop window start register                    Address offset: 0xF04 */
  __IO uint32_t P2CCRSZR;        /*!< DCMIPP Pipe2 current crop window size register                     Address offset: 0xF08 */
  __IO uint32_t P2CDCCR;         /*!< DCMIPP Pipe2 current decimation register                           Address offset: 0xF0C */
  __IO uint32_t P2CDSCR;         /*!< DCMIPP Pipe2 current downsize configuration register               Address offset: 0xF10 */
  __IO uint32_t P2CDSRTIOR;      /*!< DCMIPP Pipe2 current downsize ratio register                       Address offset: 0xF14 */
  __IO uint32_t P2CDSSZR;        /*!< DCMIPP Pipe2 current downsize destination size register            Address offset: 0xF18 */
  __IO uint32_t RESERVED59[2];   /*!< Reserved                                                           Address offset: 0xF1C-0xF20 */
  __IO uint32_t P2CRIxCR1;       /*!< Pipe2 current ROIx configuration register 1                        Address offset: 0xF24 + (x - 1) * 0x8, (x = 1 to 8)*/
  __IO uint32_t P2CRIxCR2;       /*!< Pipe2 current ROIx configuration register 2                        Address offset: 0xF28 + (x - 1) * 0x8, (x = 1 to 8)*/
       uint32_t RESERVED60[37];  /*!< Reserved                                                           Address offset: 0xF64-0xFBC */
  __IO uint32_t P2CPPCR;         /*!< DCMIPP Pipe2 current pixel packer configuration register           Address offset: 0xFC0 */
  __IO uint32_t P2CPPM0AR1;      /*!< DCMIPP Pipe2 current pixel packer Memory0 address register 1       Address offset: 0xFC4 */
  __IO uint32_t P2CPPM0AR2;      /*!< DCMIPP Pipe2 current pixel packer Memory0 address register 2       Address offset: 0xFC8 */
  __IO uint32_t P2CPPM0PR;       /*!< DCMIPP Pipe2 current pixel packer Memory0 pitch register           Address offset: 0xFCC */
       uint32_t RESERVED61[7];   /*!< Reserved                                                           Address offset: 0xFD0-0xFE8 */
  __IO uint32_t HWCFGR2;         /*!< DCMIPP hardware configuration register 2                           Address offset: 0xFEC */
  __IO uint32_t HWCFGR1;         /*!< DCMIPP hardware configuration register 1                           Address offset: 0xFF0 */
  __IO uint32_t VERR;            /*!< DCMIPP version register                                            Address offset: 0xFF4 */
  __IO uint32_t IPIDR;           /*!< DCMIPP identification register                                     Address offset: 0xFF8 */
  __IO uint32_t SIDR;            /*!< DCMIPP size identification register                                Address offset: 0xFFC */
} DCMIPP_TypeDef;

/********************  Bit definition for CSI_CR register  ********************/
#define CSI_CR_CSIEN_Pos                (0U)
#define CSI_CR_CSIEN_Msk                (0x1UL << CSI_CR_CSIEN_Pos)               /*!< 0x00000001 */
#define CSI_CR_CSIEN                    CSI_CR_CSIEN_Msk                         /*!< CSI-2 enable */
#define CSI_CR_VC0START_Pos             (2U)
#define CSI_CR_VC0START_Msk             (0x1UL << CSI_CR_VC0START_Pos)            /*!< 0x00000004 */
#define CSI_CR_VC0START                 CSI_CR_VC0START_Msk                      /*!< Virtual channel 0 start */
#define CSI_CR_VC0STOP_Pos              (3U)
#define CSI_CR_VC0STOP_Msk              (0x1UL << CSI_CR_VC0STOP_Pos)             /*!< 0x00000008 */
#define CSI_CR_VC0STOP                  CSI_CR_VC0STOP_Msk                       /*!< Virtual channel 0 stop */
#define CSI_CR_VC1START_Pos             (6U)
#define CSI_CR_VC1START_Msk             (0x1UL << CSI_CR_VC1START_Pos)            /*!< 0x00000040 */
#define CSI_CR_VC1START                 CSI_CR_VC1START_Msk                      /*!< Virtual channel 1 start */
#define CSI_CR_VC1STOP_Pos              (7U)
#define CSI_CR_VC1STOP_Msk              (0x1UL << CSI_CR_VC1STOP_Pos)             /*!< 0x00000080 */
#define CSI_CR_VC1STOP                  CSI_CR_VC1STOP_Msk                       /*!< Virtual channel 1 stop */
#define CSI_CR_VC2START_Pos             (10U)
#define CSI_CR_VC2START_Msk             (0x1UL << CSI_CR_VC2START_Pos)            /*!< 0x00000400 */
#define CSI_CR_VC2START                 CSI_CR_VC2START_Msk                      /*!< Virtual channel 2 start */
#define CSI_CR_VC2STOP_Pos              (11U)
#define CSI_CR_VC2STOP_Msk              (0x1UL << CSI_CR_VC2STOP_Pos)             /*!< 0x00000800 */
#define CSI_CR_VC2STOP                  CSI_CR_VC2STOP_Msk                       /*!< Virtual channel 2 stop */
#define CSI_CR_VC3START_Pos             (14U)
#define CSI_CR_VC3START_Msk             (0x1UL << CSI_CR_VC3START_Pos)            /*!< 0x00004000 */
#define CSI_CR_VC3START                 CSI_CR_VC3START_Msk                      /*!< Virtual channel 3 start */
#define CSI_CR_VC3STOP_Pos              (15U)
#define CSI_CR_VC3STOP_Msk              (0x1UL << CSI_CR_VC3STOP_Pos)             /*!< 0x00008000 */
#define CSI_CR_VC3STOP                  CSI_CR_VC3STOP_Msk                       /*!< Virtual channel 3 stop */

/*******************  Bit definition for CSI_PCR register  ********************/
#define CSI_PCR_PWRDOWN_Pos             (0U)
#define CSI_PCR_PWRDOWN_Msk             (0x1UL << CSI_PCR_PWRDOWN_Pos)            /*!< 0x00000001 */
#define CSI_PCR_PWRDOWN                 CSI_PCR_PWRDOWN_Msk                      /*!< Virtual channel 3 start */
#define CSI_PCR_CLEN_Pos                (1U)
#define CSI_PCR_CLEN_Msk                (0x1UL << CSI_PCR_CLEN_Pos)               /*!< 0x00000002 */
#define CSI_PCR_CLEN                    CSI_PCR_CLEN_Msk                         /*!< Clock lane enable */
#define CSI_PCR_DL0EN_Pos               (2U)
#define CSI_PCR_DL0EN_Msk               (0x1UL << CSI_PCR_DL0EN_Pos)              /*!< 0x00000004 */
#define CSI_PCR_DL0EN                   CSI_PCR_DL0EN_Msk                        /*!< D-PHY_RX data lane 0 enable */
#define CSI_PCR_DL1EN_Pos               (3U)
#define CSI_PCR_DL1EN_Msk               (0x1UL << CSI_PCR_DL1EN_Pos)              /*!< 0x00000008 */
#define CSI_PCR_DL1EN                   CSI_PCR_DL1EN_Msk                        /*!< D-PHY_RX data lane 1 enable */

/*****************  Bit definition for CSI_VC0CFGR1 register  *****************/
#define CSI_VC0CFGR1_ALLDT_Pos          (0U)
#define CSI_VC0CFGR1_ALLDT_Msk          (0x1UL << CSI_VC0CFGR1_ALLDT_Pos)         /*!< 0x00000001 */
#define CSI_VC0CFGR1_ALLDT              CSI_VC0CFGR1_ALLDT_Msk                   /*!< All data types enable for the virtual channel x */
#define CSI_VC0CFGR1_DT0EN_Pos          (1U)
#define CSI_VC0CFGR1_DT0EN_Msk          (0x1UL << CSI_VC0CFGR1_DT0EN_Pos)         /*!< 0x00000002 */
#define CSI_VC0CFGR1_DT0EN              CSI_VC0CFGR1_DT0EN_Msk                   /*!< Data type 0 enable */
#define CSI_VC0CFGR1_DT1EN_Pos          (2U)
#define CSI_VC0CFGR1_DT1EN_Msk          (0x1UL << CSI_VC0CFGR1_DT1EN_Pos)         /*!< 0x00000004 */
#define CSI_VC0CFGR1_DT1EN              CSI_VC0CFGR1_DT1EN_Msk                   /*!< Data type 1 enable */
#define CSI_VC0CFGR1_DT2EN_Pos          (3U)
#define CSI_VC0CFGR1_DT2EN_Msk          (0x1UL << CSI_VC0CFGR1_DT2EN_Pos)         /*!< 0x00000008 */
#define CSI_VC0CFGR1_DT2EN              CSI_VC0CFGR1_DT2EN_Msk                   /*!< Data type 2 enable */
#define CSI_VC0CFGR1_DT3EN_Pos          (4U)
#define CSI_VC0CFGR1_DT3EN_Msk          (0x1UL << CSI_VC0CFGR1_DT3EN_Pos)         /*!< 0x00000010 */
#define CSI_VC0CFGR1_DT3EN              CSI_VC0CFGR1_DT3EN_Msk                   /*!< Data type 3 enable */
#define CSI_VC0CFGR1_DT4EN_Pos          (5U)
#define CSI_VC0CFGR1_DT4EN_Msk          (0x1UL << CSI_VC0CFGR1_DT4EN_Pos)         /*!< 0x00000020 */
#define CSI_VC0CFGR1_DT4EN              CSI_VC0CFGR1_DT4EN_Msk                   /*!< Data type 4 enable */
#define CSI_VC0CFGR1_DT5EN_Pos          (6U)
#define CSI_VC0CFGR1_DT5EN_Msk          (0x1UL << CSI_VC0CFGR1_DT5EN_Pos)         /*!< 0x00000040 */
#define CSI_VC0CFGR1_DT5EN              CSI_VC0CFGR1_DT5EN_Msk                   /*!< Data type 5 enable */
#define CSI_VC0CFGR1_DT6EN_Pos          (7U)
#define CSI_VC0CFGR1_DT6EN_Msk          (0x1UL << CSI_VC0CFGR1_DT6EN_Pos)         /*!< 0x00000080 */
#define CSI_VC0CFGR1_DT6EN              CSI_VC0CFGR1_DT6EN_Msk                   /*!< Data type 6 enable */
#define CSI_VC0CFGR1_CDTFT_Pos          (8U)
#define CSI_VC0CFGR1_CDTFT_Msk          (0x1FUL << CSI_VC0CFGR1_CDTFT_Pos)        /*!< 0x00001F00 */
#define CSI_VC0CFGR1_CDTFT              CSI_VC0CFGR1_CDTFT_Msk                   /*!< Common format for all data types */
#define CSI_VC0CFGR1_DT0_Pos            (16U)
#define CSI_VC0CFGR1_DT0_Msk            (0x3FUL << CSI_VC0CFGR1_DT0_Pos)          /*!< 0x003F0000 */
#define CSI_VC0CFGR1_DT0                CSI_VC0CFGR1_DT0_Msk                     /*!< Data type 0 class selection for virtual channel x */
#define CSI_VC0CFGR1_DT0FT_Pos          (24U)
#define CSI_VC0CFGR1_DT0FT_Msk          (0x1FUL << CSI_VC0CFGR1_DT0FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC0CFGR1_DT0FT              CSI_VC0CFGR1_DT0FT_Msk                   /*!< Data type 0 format */

/*****************  Bit definition for CSI_VC0CFGR2 register  *****************/
#define CSI_VC0CFGR2_DT1_Pos            (0U)
#define CSI_VC0CFGR2_DT1_Msk            (0x3FUL << CSI_VC0CFGR2_DT1_Pos)          /*!< 0x0000003F */
#define CSI_VC0CFGR2_DT1                CSI_VC0CFGR2_DT1_Msk                     /*!< Data type 1 class selection for virtual channel x */
#define CSI_VC0CFGR2_DT1FT_Pos          (8U)
#define CSI_VC0CFGR2_DT1FT_Msk          (0x1FUL << CSI_VC0CFGR2_DT1FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC0CFGR2_DT1FT              CSI_VC0CFGR2_DT1FT_Msk                   /*!< Data type 1 format */
#define CSI_VC0CFGR2_DT2_Pos            (16U)
#define CSI_VC0CFGR2_DT2_Msk            (0x3FUL << CSI_VC0CFGR2_DT2_Pos)          /*!< 0x003F0000 */
#define CSI_VC0CFGR2_DT2                CSI_VC0CFGR2_DT2_Msk                     /*!< Data type 2 class selection for virtual channel x */
#define CSI_VC0CFGR2_DT2FT_Pos          (24U)
#define CSI_VC0CFGR2_DT2FT_Msk          (0x1FUL << CSI_VC0CFGR2_DT2FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC0CFGR2_DT2FT              CSI_VC0CFGR2_DT2FT_Msk                   /*!< Data type 2 format */

/*****************  Bit definition for CSI_VC0CFGR3 register  *****************/
#define CSI_VC0CFGR3_DT3_Pos            (0U)
#define CSI_VC0CFGR3_DT3_Msk            (0x3FUL << CSI_VC0CFGR3_DT3_Pos)          /*!< 0x0000003F */
#define CSI_VC0CFGR3_DT3                CSI_VC0CFGR3_DT3_Msk                     /*!< Data type 3 class selection for virtual channel x */
#define CSI_VC0CFGR3_DT3FT_Pos          (8U)
#define CSI_VC0CFGR3_DT3FT_Msk          (0x1FUL << CSI_VC0CFGR3_DT3FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC0CFGR3_DT3FT              CSI_VC0CFGR3_DT3FT_Msk                   /*!< Data type 3 format */
#define CSI_VC0CFGR3_DT4_Pos            (16U)
#define CSI_VC0CFGR3_DT4_Msk            (0x3FUL << CSI_VC0CFGR3_DT4_Pos)          /*!< 0x003F0000 */
#define CSI_VC0CFGR3_DT4                CSI_VC0CFGR3_DT4_Msk                     /*!< Data type 4 class selection for virtual channel x */
#define CSI_VC0CFGR3_DT4FT_Pos          (24U)
#define CSI_VC0CFGR3_DT4FT_Msk          (0x1FUL << CSI_VC0CFGR3_DT4FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC0CFGR3_DT4FT              CSI_VC0CFGR3_DT4FT_Msk                   /*!< Data type 4 format */

/*****************  Bit definition for CSI_VC0CFGR4 register  *****************/
#define CSI_VC0CFGR4_DT5_Pos            (0U)
#define CSI_VC0CFGR4_DT5_Msk            (0x3FUL << CSI_VC0CFGR4_DT5_Pos)          /*!< 0x0000003F */
#define CSI_VC0CFGR4_DT5                CSI_VC0CFGR4_DT5_Msk                     /*!< Data type 5 class selection for virtual channel x */
#define CSI_VC0CFGR4_DT5FT_Pos          (8U)
#define CSI_VC0CFGR4_DT5FT_Msk          (0x1FUL << CSI_VC0CFGR4_DT5FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC0CFGR4_DT5FT              CSI_VC0CFGR4_DT5FT_Msk                   /*!< Data type 5 format */
#define CSI_VC0CFGR4_DT6_Pos            (16U)
#define CSI_VC0CFGR4_DT6_Msk            (0x3FUL << CSI_VC0CFGR4_DT6_Pos)          /*!< 0x003F0000 */
#define CSI_VC0CFGR4_DT6                CSI_VC0CFGR4_DT6_Msk                     /*!< Data type 6 class selection for virtual channel x */
#define CSI_VC0CFGR4_DT6FT_Pos          (24U)
#define CSI_VC0CFGR4_DT6FT_Msk          (0x1FUL << CSI_VC0CFGR4_DT6FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC0CFGR4_DT6FT              CSI_VC0CFGR4_DT6FT_Msk                   /*!< Data type 6 format */

/*****************  Bit definition for CSI_VC1CFGR1 register  *****************/
#define CSI_VC1CFGR1_ALLDT_Pos          (0U)
#define CSI_VC1CFGR1_ALLDT_Msk          (0x1UL << CSI_VC1CFGR1_ALLDT_Pos)         /*!< 0x00000001 */
#define CSI_VC1CFGR1_ALLDT              CSI_VC1CFGR1_ALLDT_Msk                   /*!< All data types enable for the virtual channel x */
#define CSI_VC1CFGR1_DT0EN_Pos          (1U)
#define CSI_VC1CFGR1_DT0EN_Msk          (0x1UL << CSI_VC1CFGR1_DT0EN_Pos)         /*!< 0x00000002 */
#define CSI_VC1CFGR1_DT0EN              CSI_VC1CFGR1_DT0EN_Msk                   /*!< Data type 0 enable */
#define CSI_VC1CFGR1_DT1EN_Pos          (2U)
#define CSI_VC1CFGR1_DT1EN_Msk          (0x1UL << CSI_VC1CFGR1_DT1EN_Pos)         /*!< 0x00000004 */
#define CSI_VC1CFGR1_DT1EN              CSI_VC1CFGR1_DT1EN_Msk                   /*!< Data type 1 enable */
#define CSI_VC1CFGR1_DT2EN_Pos          (3U)
#define CSI_VC1CFGR1_DT2EN_Msk          (0x1UL << CSI_VC1CFGR1_DT2EN_Pos)         /*!< 0x00000008 */
#define CSI_VC1CFGR1_DT2EN              CSI_VC1CFGR1_DT2EN_Msk                   /*!< Data type 2 enable */
#define CSI_VC1CFGR1_DT3EN_Pos          (4U)
#define CSI_VC1CFGR1_DT3EN_Msk          (0x1UL << CSI_VC1CFGR1_DT3EN_Pos)         /*!< 0x00000010 */
#define CSI_VC1CFGR1_DT3EN              CSI_VC1CFGR1_DT3EN_Msk                   /*!< Data type 3 enable */
#define CSI_VC1CFGR1_DT4EN_Pos          (5U)
#define CSI_VC1CFGR1_DT4EN_Msk          (0x1UL << CSI_VC1CFGR1_DT4EN_Pos)         /*!< 0x00000020 */
#define CSI_VC1CFGR1_DT4EN              CSI_VC1CFGR1_DT4EN_Msk                   /*!< Data type 4 enable */
#define CSI_VC1CFGR1_DT5EN_Pos          (6U)
#define CSI_VC1CFGR1_DT5EN_Msk          (0x1UL << CSI_VC1CFGR1_DT5EN_Pos)         /*!< 0x00000040 */
#define CSI_VC1CFGR1_DT5EN              CSI_VC1CFGR1_DT5EN_Msk                   /*!< Data type 5 enable */
#define CSI_VC1CFGR1_DT6EN_Pos          (7U)
#define CSI_VC1CFGR1_DT6EN_Msk          (0x1UL << CSI_VC1CFGR1_DT6EN_Pos)         /*!< 0x00000080 */
#define CSI_VC1CFGR1_DT6EN              CSI_VC1CFGR1_DT6EN_Msk                   /*!< Data type 6 enable */
#define CSI_VC1CFGR1_CDTFT_Pos          (8U)
#define CSI_VC1CFGR1_CDTFT_Msk          (0x1FUL << CSI_VC1CFGR1_CDTFT_Pos)        /*!< 0x00001F00 */
#define CSI_VC1CFGR1_CDTFT              CSI_VC1CFGR1_CDTFT_Msk                   /*!< Common format for all data types */
#define CSI_VC1CFGR1_DT0_Pos            (16U)
#define CSI_VC1CFGR1_DT0_Msk            (0x3FUL << CSI_VC1CFGR1_DT0_Pos)          /*!< 0x003F0000 */
#define CSI_VC1CFGR1_DT0                CSI_VC1CFGR1_DT0_Msk                     /*!< Data type 0 class selection for virtual channel x */
#define CSI_VC1CFGR1_DT0FT_Pos          (24U)
#define CSI_VC1CFGR1_DT0FT_Msk          (0x1FUL << CSI_VC1CFGR1_DT0FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC1CFGR1_DT0FT              CSI_VC1CFGR1_DT0FT_Msk                   /*!< Data type 0 format */

/*****************  Bit definition for CSI_VC1CFGR2 register  *****************/
#define CSI_VC1CFGR2_DT1_Pos            (0U)
#define CSI_VC1CFGR2_DT1_Msk            (0x3FUL << CSI_VC1CFGR2_DT1_Pos)          /*!< 0x0000003F */
#define CSI_VC1CFGR2_DT1                CSI_VC1CFGR2_DT1_Msk                     /*!< Data type 1 class selection for virtual channel x */
#define CSI_VC1CFGR2_DT1FT_Pos          (8U)
#define CSI_VC1CFGR2_DT1FT_Msk          (0x1FUL << CSI_VC1CFGR2_DT1FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC1CFGR2_DT1FT              CSI_VC1CFGR2_DT1FT_Msk                   /*!< Data type 1 format */
#define CSI_VC1CFGR2_DT2_Pos            (16U)
#define CSI_VC1CFGR2_DT2_Msk            (0x3FUL << CSI_VC1CFGR2_DT2_Pos)          /*!< 0x003F0000 */
#define CSI_VC1CFGR2_DT2                CSI_VC1CFGR2_DT2_Msk                     /*!< Data type 2 class selection for virtual channel x */
#define CSI_VC1CFGR2_DT2FT_Pos          (24U)
#define CSI_VC1CFGR2_DT2FT_Msk          (0x1FUL << CSI_VC1CFGR2_DT2FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC1CFGR2_DT2FT              CSI_VC1CFGR2_DT2FT_Msk                   /*!< Data type 2 format */

/*****************  Bit definition for CSI_VC1CFGR3 register  *****************/
#define CSI_VC1CFGR3_DT3_Pos            (0U)
#define CSI_VC1CFGR3_DT3_Msk            (0x3FUL << CSI_VC1CFGR3_DT3_Pos)          /*!< 0x0000003F */
#define CSI_VC1CFGR3_DT3                CSI_VC1CFGR3_DT3_Msk                     /*!< Data type 3 class selection for virtual channel x */
#define CSI_VC1CFGR3_DT3FT_Pos          (8U)
#define CSI_VC1CFGR3_DT3FT_Msk          (0x1FUL << CSI_VC1CFGR3_DT3FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC1CFGR3_DT3FT              CSI_VC1CFGR3_DT3FT_Msk                   /*!< Data type 3 format */
#define CSI_VC1CFGR3_DT4_Pos            (16U)
#define CSI_VC1CFGR3_DT4_Msk            (0x3FUL << CSI_VC1CFGR3_DT4_Pos)          /*!< 0x003F0000 */
#define CSI_VC1CFGR3_DT4                CSI_VC1CFGR3_DT4_Msk                     /*!< Data type 4 class selection for virtual channel x */
#define CSI_VC1CFGR3_DT4FT_Pos          (24U)
#define CSI_VC1CFGR3_DT4FT_Msk          (0x1FUL << CSI_VC1CFGR3_DT4FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC1CFGR3_DT4FT              CSI_VC1CFGR3_DT4FT_Msk                   /*!< Data type 4 format */

/*****************  Bit definition for CSI_VC1CFGR4 register  *****************/
#define CSI_VC1CFGR4_DT5_Pos            (0U)
#define CSI_VC1CFGR4_DT5_Msk            (0x3FUL << CSI_VC1CFGR4_DT5_Pos)          /*!< 0x0000003F */
#define CSI_VC1CFGR4_DT5                CSI_VC1CFGR4_DT5_Msk                     /*!< Data type 5 class selection for virtual channel x */
#define CSI_VC1CFGR4_DT5FT_Pos          (8U)
#define CSI_VC1CFGR4_DT5FT_Msk          (0x1FUL << CSI_VC1CFGR4_DT5FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC1CFGR4_DT5FT              CSI_VC1CFGR4_DT5FT_Msk                   /*!< Data type 5 format */
#define CSI_VC1CFGR4_DT6_Pos            (16U)
#define CSI_VC1CFGR4_DT6_Msk            (0x3FUL << CSI_VC1CFGR4_DT6_Pos)          /*!< 0x003F0000 */
#define CSI_VC1CFGR4_DT6                CSI_VC1CFGR4_DT6_Msk                     /*!< Data type 6 class selection for virtual channel x */
#define CSI_VC1CFGR4_DT6FT_Pos          (24U)
#define CSI_VC1CFGR4_DT6FT_Msk          (0x1FUL << CSI_VC1CFGR4_DT6FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC1CFGR4_DT6FT              CSI_VC1CFGR4_DT6FT_Msk                   /*!< Data type 6 format */

/*****************  Bit definition for CSI_VC2CFGR1 register  *****************/
#define CSI_VC2CFGR1_ALLDT_Pos          (0U)
#define CSI_VC2CFGR1_ALLDT_Msk          (0x1UL << CSI_VC2CFGR1_ALLDT_Pos)         /*!< 0x00000001 */
#define CSI_VC2CFGR1_ALLDT              CSI_VC2CFGR1_ALLDT_Msk                   /*!< All data types enable for the virtual channel x */
#define CSI_VC2CFGR1_DT0EN_Pos          (1U)
#define CSI_VC2CFGR1_DT0EN_Msk          (0x1UL << CSI_VC2CFGR1_DT0EN_Pos)         /*!< 0x00000002 */
#define CSI_VC2CFGR1_DT0EN              CSI_VC2CFGR1_DT0EN_Msk                   /*!< Data type 0 enable */
#define CSI_VC2CFGR1_DT1EN_Pos          (2U)
#define CSI_VC2CFGR1_DT1EN_Msk          (0x1UL << CSI_VC2CFGR1_DT1EN_Pos)         /*!< 0x00000004 */
#define CSI_VC2CFGR1_DT1EN              CSI_VC2CFGR1_DT1EN_Msk                   /*!< Data type 1 enable */
#define CSI_VC2CFGR1_DT2EN_Pos          (3U)
#define CSI_VC2CFGR1_DT2EN_Msk          (0x1UL << CSI_VC2CFGR1_DT2EN_Pos)         /*!< 0x00000008 */
#define CSI_VC2CFGR1_DT2EN              CSI_VC2CFGR1_DT2EN_Msk                   /*!< Data type 2 enable */
#define CSI_VC2CFGR1_DT3EN_Pos          (4U)
#define CSI_VC2CFGR1_DT3EN_Msk          (0x1UL << CSI_VC2CFGR1_DT3EN_Pos)         /*!< 0x00000010 */
#define CSI_VC2CFGR1_DT3EN              CSI_VC2CFGR1_DT3EN_Msk                   /*!< Data type 3 enable */
#define CSI_VC2CFGR1_DT4EN_Pos          (5U)
#define CSI_VC2CFGR1_DT4EN_Msk          (0x1UL << CSI_VC2CFGR1_DT4EN_Pos)         /*!< 0x00000020 */
#define CSI_VC2CFGR1_DT4EN              CSI_VC2CFGR1_DT4EN_Msk                   /*!< Data type 4 enable */
#define CSI_VC2CFGR1_DT5EN_Pos          (6U)
#define CSI_VC2CFGR1_DT5EN_Msk          (0x1UL << CSI_VC2CFGR1_DT5EN_Pos)         /*!< 0x00000040 */
#define CSI_VC2CFGR1_DT5EN              CSI_VC2CFGR1_DT5EN_Msk                   /*!< Data type 5 enable */
#define CSI_VC2CFGR1_DT6EN_Pos          (7U)
#define CSI_VC2CFGR1_DT6EN_Msk          (0x1UL << CSI_VC2CFGR1_DT6EN_Pos)         /*!< 0x00000080 */
#define CSI_VC2CFGR1_DT6EN              CSI_VC2CFGR1_DT6EN_Msk                   /*!< Data type 6 enable */
#define CSI_VC2CFGR1_CDTFT_Pos          (8U)
#define CSI_VC2CFGR1_CDTFT_Msk          (0x1FUL << CSI_VC2CFGR1_CDTFT_Pos)        /*!< 0x00001F00 */
#define CSI_VC2CFGR1_CDTFT              CSI_VC2CFGR1_CDTFT_Msk                   /*!< Common format for all data types */
#define CSI_VC2CFGR1_DT0_Pos            (16U)
#define CSI_VC2CFGR1_DT0_Msk            (0x3FUL << CSI_VC2CFGR1_DT0_Pos)          /*!< 0x003F0000 */
#define CSI_VC2CFGR1_DT0                CSI_VC2CFGR1_DT0_Msk                     /*!< Data type 0 class selection for virtual channel x */
#define CSI_VC2CFGR1_DT0FT_Pos          (24U)
#define CSI_VC2CFGR1_DT0FT_Msk          (0x1FUL << CSI_VC2CFGR1_DT0FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC2CFGR1_DT0FT              CSI_VC2CFGR1_DT0FT_Msk                   /*!< Data type 0 format */

/*****************  Bit definition for CSI_VC2CFGR2 register  *****************/
#define CSI_VC2CFGR2_DT1_Pos            (0U)
#define CSI_VC2CFGR2_DT1_Msk            (0x3FUL << CSI_VC2CFGR2_DT1_Pos)          /*!< 0x0000003F */
#define CSI_VC2CFGR2_DT1                CSI_VC2CFGR2_DT1_Msk                     /*!< Data type 1 class selection for virtual channel x */
#define CSI_VC2CFGR2_DT1FT_Pos          (8U)
#define CSI_VC2CFGR2_DT1FT_Msk          (0x1FUL << CSI_VC2CFGR2_DT1FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC2CFGR2_DT1FT              CSI_VC2CFGR2_DT1FT_Msk                   /*!< Data type 1 format */
#define CSI_VC2CFGR2_DT2_Pos            (16U)
#define CSI_VC2CFGR2_DT2_Msk            (0x3FUL << CSI_VC2CFGR2_DT2_Pos)          /*!< 0x003F0000 */
#define CSI_VC2CFGR2_DT2                CSI_VC2CFGR2_DT2_Msk                     /*!< Data type 2 class selection for virtual channel x */
#define CSI_VC2CFGR2_DT2FT_Pos          (24U)
#define CSI_VC2CFGR2_DT2FT_Msk          (0x1FUL << CSI_VC2CFGR2_DT2FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC2CFGR2_DT2FT              CSI_VC2CFGR2_DT2FT_Msk                   /*!< Data type 2 format */

/*****************  Bit definition for CSI_VC2CFGR3 register  *****************/
#define CSI_VC2CFGR3_DT3_Pos            (0U)
#define CSI_VC2CFGR3_DT3_Msk            (0x3FUL << CSI_VC2CFGR3_DT3_Pos)          /*!< 0x0000003F */
#define CSI_VC2CFGR3_DT3                CSI_VC2CFGR3_DT3_Msk                     /*!< Data type 3 class selection for virtual channel x */
#define CSI_VC2CFGR3_DT3FT_Pos          (8U)
#define CSI_VC2CFGR3_DT3FT_Msk          (0x1FUL << CSI_VC2CFGR3_DT3FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC2CFGR3_DT3FT              CSI_VC2CFGR3_DT3FT_Msk                   /*!< Data type 3 format */
#define CSI_VC2CFGR3_DT4_Pos            (16U)
#define CSI_VC2CFGR3_DT4_Msk            (0x3FUL << CSI_VC2CFGR3_DT4_Pos)          /*!< 0x003F0000 */
#define CSI_VC2CFGR3_DT4                CSI_VC2CFGR3_DT4_Msk                     /*!< Data type 4 class selection for virtual channel x */
#define CSI_VC2CFGR3_DT4FT_Pos          (24U)
#define CSI_VC2CFGR3_DT4FT_Msk          (0x1FUL << CSI_VC2CFGR3_DT4FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC2CFGR3_DT4FT              CSI_VC2CFGR3_DT4FT_Msk                   /*!< Data type 4 format */

/*****************  Bit definition for CSI_VC2CFGR4 register  *****************/
#define CSI_VC2CFGR4_DT5_Pos            (0U)
#define CSI_VC2CFGR4_DT5_Msk            (0x3FUL << CSI_VC2CFGR4_DT5_Pos)          /*!< 0x0000003F */
#define CSI_VC2CFGR4_DT5                CSI_VC2CFGR4_DT5_Msk                     /*!< Data type 5 class selection for virtual channel x */
#define CSI_VC2CFGR4_DT5FT_Pos          (8U)
#define CSI_VC2CFGR4_DT5FT_Msk          (0x1FUL << CSI_VC2CFGR4_DT5FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC2CFGR4_DT5FT              CSI_VC2CFGR4_DT5FT_Msk                   /*!< Data type 5 format */
#define CSI_VC2CFGR4_DT6_Pos            (16U)
#define CSI_VC2CFGR4_DT6_Msk            (0x3FUL << CSI_VC2CFGR4_DT6_Pos)          /*!< 0x003F0000 */
#define CSI_VC2CFGR4_DT6                CSI_VC2CFGR4_DT6_Msk                     /*!< Data type 6 class selection for virtual channel x */
#define CSI_VC2CFGR4_DT6FT_Pos          (24U)
#define CSI_VC2CFGR4_DT6FT_Msk          (0x1FUL << CSI_VC2CFGR4_DT6FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC2CFGR4_DT6FT              CSI_VC2CFGR4_DT6FT_Msk                   /*!< Data type 6 format */

/*****************  Bit definition for CSI_VC3CFGR1 register  *****************/
#define CSI_VC3CFGR1_ALLDT_Pos          (0U)
#define CSI_VC3CFGR1_ALLDT_Msk          (0x1UL << CSI_VC3CFGR1_ALLDT_Pos)         /*!< 0x00000001 */
#define CSI_VC3CFGR1_ALLDT              CSI_VC3CFGR1_ALLDT_Msk                   /*!< All data types enable for the virtual channel x */
#define CSI_VC3CFGR1_DT0EN_Pos          (1U)
#define CSI_VC3CFGR1_DT0EN_Msk          (0x1UL << CSI_VC3CFGR1_DT0EN_Pos)         /*!< 0x00000002 */
#define CSI_VC3CFGR1_DT0EN              CSI_VC3CFGR1_DT0EN_Msk                   /*!< Data type 0 enable */
#define CSI_VC3CFGR1_DT1EN_Pos          (2U)
#define CSI_VC3CFGR1_DT1EN_Msk          (0x1UL << CSI_VC3CFGR1_DT1EN_Pos)         /*!< 0x00000004 */
#define CSI_VC3CFGR1_DT1EN              CSI_VC3CFGR1_DT1EN_Msk                   /*!< Data type 1 enable */
#define CSI_VC3CFGR1_DT2EN_Pos          (3U)
#define CSI_VC3CFGR1_DT2EN_Msk          (0x1UL << CSI_VC3CFGR1_DT2EN_Pos)         /*!< 0x00000008 */
#define CSI_VC3CFGR1_DT2EN              CSI_VC3CFGR1_DT2EN_Msk                   /*!< Data type 2 enable */
#define CSI_VC3CFGR1_DT3EN_Pos          (4U)
#define CSI_VC3CFGR1_DT3EN_Msk          (0x1UL << CSI_VC3CFGR1_DT3EN_Pos)         /*!< 0x00000010 */
#define CSI_VC3CFGR1_DT3EN              CSI_VC3CFGR1_DT3EN_Msk                   /*!< Data type 3 enable */
#define CSI_VC3CFGR1_DT4EN_Pos          (5U)
#define CSI_VC3CFGR1_DT4EN_Msk          (0x1UL << CSI_VC3CFGR1_DT4EN_Pos)         /*!< 0x00000020 */
#define CSI_VC3CFGR1_DT4EN              CSI_VC3CFGR1_DT4EN_Msk                   /*!< Data type 4 enable */
#define CSI_VC3CFGR1_DT5EN_Pos          (6U)
#define CSI_VC3CFGR1_DT5EN_Msk          (0x1UL << CSI_VC3CFGR1_DT5EN_Pos)         /*!< 0x00000040 */
#define CSI_VC3CFGR1_DT5EN              CSI_VC3CFGR1_DT5EN_Msk                   /*!< Data type 5 enable */
#define CSI_VC3CFGR1_DT6EN_Pos          (7U)
#define CSI_VC3CFGR1_DT6EN_Msk          (0x1UL << CSI_VC3CFGR1_DT6EN_Pos)         /*!< 0x00000080 */
#define CSI_VC3CFGR1_DT6EN              CSI_VC3CFGR1_DT6EN_Msk                   /*!< Data type 6 enable */
#define CSI_VC3CFGR1_CDTFT_Pos          (8U)
#define CSI_VC3CFGR1_CDTFT_Msk          (0x1FUL << CSI_VC3CFGR1_CDTFT_Pos)        /*!< 0x00001F00 */
#define CSI_VC3CFGR1_CDTFT              CSI_VC3CFGR1_CDTFT_Msk                   /*!< Common format for all data types */
#define CSI_VC3CFGR1_DT0_Pos            (16U)
#define CSI_VC3CFGR1_DT0_Msk            (0x3FUL << CSI_VC3CFGR1_DT0_Pos)          /*!< 0x003F0000 */
#define CSI_VC3CFGR1_DT0                CSI_VC3CFGR1_DT0_Msk                     /*!< Data type 0 class selection for virtual channel x */
#define CSI_VC3CFGR1_DT0FT_Pos          (24U)
#define CSI_VC3CFGR1_DT0FT_Msk          (0x1FUL << CSI_VC3CFGR1_DT0FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC3CFGR1_DT0FT              CSI_VC3CFGR1_DT0FT_Msk                   /*!< Data type 0 format */

/*****************  Bit definition for CSI_VC3CFGR2 register  *****************/
#define CSI_VC3CFGR2_DT1_Pos            (0U)
#define CSI_VC3CFGR2_DT1_Msk            (0x3FUL << CSI_VC3CFGR2_DT1_Pos)          /*!< 0x0000003F */
#define CSI_VC3CFGR2_DT1                CSI_VC3CFGR2_DT1_Msk                     /*!< Data type 1 class selection for virtual channel x */
#define CSI_VC3CFGR2_DT1FT_Pos          (8U)
#define CSI_VC3CFGR2_DT1FT_Msk          (0x1FUL << CSI_VC3CFGR2_DT1FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC3CFGR2_DT1FT              CSI_VC3CFGR2_DT1FT_Msk                   /*!< Data type 1 format */
#define CSI_VC3CFGR2_DT2_Pos            (16U)
#define CSI_VC3CFGR2_DT2_Msk            (0x3FUL << CSI_VC3CFGR2_DT2_Pos)          /*!< 0x003F0000 */
#define CSI_VC3CFGR2_DT2                CSI_VC3CFGR2_DT2_Msk                     /*!< Data type 2 class selection for virtual channel x */
#define CSI_VC3CFGR2_DT2FT_Pos          (24U)
#define CSI_VC3CFGR2_DT2FT_Msk          (0x1FUL << CSI_VC3CFGR2_DT2FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC3CFGR2_DT2FT              CSI_VC3CFGR2_DT2FT_Msk                   /*!< Data type 2 format */

/*****************  Bit definition for CSI_VC3CFGR3 register  *****************/
#define CSI_VC3CFGR3_DT3_Pos            (0U)
#define CSI_VC3CFGR3_DT3_Msk            (0x3FUL << CSI_VC3CFGR3_DT3_Pos)          /*!< 0x0000003F */
#define CSI_VC3CFGR3_DT3                CSI_VC3CFGR3_DT3_Msk                     /*!< Data type 3 class selection for virtual channel x */
#define CSI_VC3CFGR3_DT3FT_Pos          (8U)
#define CSI_VC3CFGR3_DT3FT_Msk          (0x1FUL << CSI_VC3CFGR3_DT3FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC3CFGR3_DT3FT              CSI_VC3CFGR3_DT3FT_Msk                   /*!< Data type 3 format */
#define CSI_VC3CFGR3_DT4_Pos            (16U)
#define CSI_VC3CFGR3_DT4_Msk            (0x3FUL << CSI_VC3CFGR3_DT4_Pos)          /*!< 0x003F0000 */
#define CSI_VC3CFGR3_DT4                CSI_VC3CFGR3_DT4_Msk                     /*!< Data type 4 class selection for virtual channel x */
#define CSI_VC3CFGR3_DT4FT_Pos          (24U)
#define CSI_VC3CFGR3_DT4FT_Msk          (0x1FUL << CSI_VC3CFGR3_DT4FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC3CFGR3_DT4FT              CSI_VC3CFGR3_DT4FT_Msk                   /*!< Data type 4 format */

/*****************  Bit definition for CSI_VC3CFGR4 register  *****************/
#define CSI_VC3CFGR4_DT5_Pos            (0U)
#define CSI_VC3CFGR4_DT5_Msk            (0x3FUL << CSI_VC3CFGR4_DT5_Pos)          /*!< 0x0000003F */
#define CSI_VC3CFGR4_DT5                CSI_VC3CFGR4_DT5_Msk                     /*!< Data type 5 class selection for virtual channel x */
#define CSI_VC3CFGR4_DT5FT_Pos          (8U)
#define CSI_VC3CFGR4_DT5FT_Msk          (0x1FUL << CSI_VC3CFGR4_DT5FT_Pos)        /*!< 0x00001F00 */
#define CSI_VC3CFGR4_DT5FT              CSI_VC3CFGR4_DT5FT_Msk                   /*!< Data type 5 format */
#define CSI_VC3CFGR4_DT6_Pos            (16U)
#define CSI_VC3CFGR4_DT6_Msk            (0x3FUL << CSI_VC3CFGR4_DT6_Pos)          /*!< 0x003F0000 */
#define CSI_VC3CFGR4_DT6                CSI_VC3CFGR4_DT6_Msk                     /*!< Data type 6 class selection for virtual channel x */
#define CSI_VC3CFGR4_DT6FT_Pos          (24U)
#define CSI_VC3CFGR4_DT6FT_Msk          (0x1FUL << CSI_VC3CFGR4_DT6FT_Pos)        /*!< 0x1F000000 */
#define CSI_VC3CFGR4_DT6FT              CSI_VC3CFGR4_DT6FT_Msk                   /*!< Data type 6 format */

/*****************  Bit definition for CSI_LB0CFGR register  ******************/
#define CSI_LB0CFGR_BYTECNT_Pos         (0U)
#define CSI_LB0CFGR_BYTECNT_Msk         (0xFFFFUL << CSI_LB0CFGR_BYTECNT_Pos)     /*!< 0x0000FFFF */
#define CSI_LB0CFGR_BYTECNT             CSI_LB0CFGR_BYTECNT_Msk                  /*!< Byte counter */
#define CSI_LB0CFGR_LINECNT_Pos         (16U)
#define CSI_LB0CFGR_LINECNT_Msk         (0xFFFFUL << CSI_LB0CFGR_LINECNT_Pos)     /*!< 0xFFFF0000 */
#define CSI_LB0CFGR_LINECNT             CSI_LB0CFGR_LINECNT_Msk                  /*!< Line counter */

/*****************  Bit definition for CSI_LB1CFGR register  ******************/
#define CSI_LB1CFGR_BYTECNT_Pos         (0U)
#define CSI_LB1CFGR_BYTECNT_Msk         (0xFFFFUL << CSI_LB1CFGR_BYTECNT_Pos)     /*!< 0x0000FFFF */
#define CSI_LB1CFGR_BYTECNT             CSI_LB1CFGR_BYTECNT_Msk                  /*!< Byte counter */
#define CSI_LB1CFGR_LINECNT_Pos         (16U)
#define CSI_LB1CFGR_LINECNT_Msk         (0xFFFFUL << CSI_LB1CFGR_LINECNT_Pos)     /*!< 0xFFFF0000 */
#define CSI_LB1CFGR_LINECNT             CSI_LB1CFGR_LINECNT_Msk                  /*!< Line counter */

/*****************  Bit definition for CSI_LB2CFGR register  ******************/
#define CSI_LB2CFGR_BYTECNT_Pos         (0U)
#define CSI_LB2CFGR_BYTECNT_Msk         (0xFFFFUL << CSI_LB2CFGR_BYTECNT_Pos)     /*!< 0x0000FFFF */
#define CSI_LB2CFGR_BYTECNT             CSI_LB2CFGR_BYTECNT_Msk                  /*!< Byte counter */
#define CSI_LB2CFGR_LINECNT_Pos         (16U)
#define CSI_LB2CFGR_LINECNT_Msk         (0xFFFFUL << CSI_LB2CFGR_LINECNT_Pos)     /*!< 0xFFFF0000 */
#define CSI_LB2CFGR_LINECNT             CSI_LB2CFGR_LINECNT_Msk                  /*!< Line counter */

/*****************  Bit definition for CSI_LB3CFGR register  ******************/
#define CSI_LB3CFGR_BYTECNT_Pos         (0U)
#define CSI_LB3CFGR_BYTECNT_Msk         (0xFFFFUL << CSI_LB3CFGR_BYTECNT_Pos)     /*!< 0x0000FFFF */
#define CSI_LB3CFGR_BYTECNT             CSI_LB3CFGR_BYTECNT_Msk                  /*!< Byte counter */
#define CSI_LB3CFGR_LINECNT_Pos         (16U)
#define CSI_LB3CFGR_LINECNT_Msk         (0xFFFFUL << CSI_LB3CFGR_LINECNT_Pos)     /*!< 0xFFFF0000 */
#define CSI_LB3CFGR_LINECNT             CSI_LB3CFGR_LINECNT_Msk                  /*!< Line counter */

/*****************  Bit definition for CSI_TIM0CFGR register  *****************/
#define CSI_TIM0CFGR_COUNT_Pos          (0U)
#define CSI_TIM0CFGR_COUNT_Msk          (0x1FFFFFFUL << CSI_TIM0CFGR_COUNT_Pos)   /*!< 0x01FFFFFF */
#define CSI_TIM0CFGR_COUNT              CSI_TIM0CFGR_COUNT_Msk                   /*!< Clock cycle counter */

/*****************  Bit definition for CSI_TIM1CFGR register  *****************/
#define CSI_TIM1CFGR_COUNT_Pos          (0U)
#define CSI_TIM1CFGR_COUNT_Msk          (0x1FFFFFFUL << CSI_TIM1CFGR_COUNT_Pos)   /*!< 0x01FFFFFF */
#define CSI_TIM1CFGR_COUNT              CSI_TIM1CFGR_COUNT_Msk                   /*!< Clock cycle counter */

/*****************  Bit definition for CSI_TIM2CFGR register  *****************/
#define CSI_TIM2CFGR_COUNT_Pos          (0U)
#define CSI_TIM2CFGR_COUNT_Msk          (0x1FFFFFFUL << CSI_TIM2CFGR_COUNT_Pos)   /*!< 0x01FFFFFF */
#define CSI_TIM2CFGR_COUNT              CSI_TIM2CFGR_COUNT_Msk                   /*!< Clock cycle counter */

/*****************  Bit definition for CSI_TIM3CFGR register  *****************/
#define CSI_TIM3CFGR_COUNT_Pos          (0U)
#define CSI_TIM3CFGR_COUNT_Msk          (0x1FFFFFFUL << CSI_TIM3CFGR_COUNT_Pos)   /*!< 0x01FFFFFF */
#define CSI_TIM3CFGR_COUNT              CSI_TIM3CFGR_COUNT_Msk                   /*!< Clock cycle counter */

/******************  Bit definition for CSI_LMCFGR register  ******************/
#define CSI_LMCFGR_LANENB_Pos           (8U)
#define CSI_LMCFGR_LANENB_Msk           (0x7UL << CSI_LMCFGR_LANENB_Pos)          /*!< 0x00000700 */
#define CSI_LMCFGR_LANENB               CSI_LMCFGR_LANENB_Msk                    /*!< Number of lanes */
#define CSI_LMCFGR_DL0MAP_Pos           (16U)
#define CSI_LMCFGR_DL0MAP_Msk           (0x7UL << CSI_LMCFGR_DL0MAP_Pos)          /*!< 0x00070000 */
#define CSI_LMCFGR_DL0MAP               CSI_LMCFGR_DL0MAP_Msk                    /*!< Physical mapping of logical data lane 0 */
#define CSI_LMCFGR_DL1MAP_Pos           (20U)
#define CSI_LMCFGR_DL1MAP_Msk           (0x7UL << CSI_LMCFGR_DL1MAP_Pos)          /*!< 0x00700000 */
#define CSI_LMCFGR_DL1MAP               CSI_LMCFGR_DL1MAP_Msk                    /*!< Physical mapping of logical data lane 1 */

/******************  Bit definition for CSI_PRGITR register  ******************/
#define CSI_PRGITR_LB0VC_Pos            (0U)
#define CSI_PRGITR_LB0VC_Msk            (0x3UL << CSI_PRGITR_LB0VC_Pos)           /*!< 0x00000003 */
#define CSI_PRGITR_LB0VC                CSI_PRGITR_LB0VC_Msk                     /*!< Line/Byte counter 0 linked to a virtual channel */
#define CSI_PRGITR_LB0EN_Pos            (3U)
#define CSI_PRGITR_LB0EN_Msk            (0x1UL << CSI_PRGITR_LB0EN_Pos)           /*!< 0x00000008 */
#define CSI_PRGITR_LB0EN                CSI_PRGITR_LB0EN_Msk                     /*!< Line/Byte 0counter enable */
#define CSI_PRGITR_LB1VC_Pos            (4U)
#define CSI_PRGITR_LB1VC_Msk            (0x3UL << CSI_PRGITR_LB1VC_Pos)           /*!< 0x00000030 */
#define CSI_PRGITR_LB1VC                CSI_PRGITR_LB1VC_Msk                     /*!< Line/Byte counter 1 linked to a virtual channel */
#define CSI_PRGITR_LB1EN_Pos            (7U)
#define CSI_PRGITR_LB1EN_Msk            (0x1UL << CSI_PRGITR_LB1EN_Pos)           /*!< 0x00000080 */
#define CSI_PRGITR_LB1EN                CSI_PRGITR_LB1EN_Msk                     /*!< Line/Byte 1 counter enable */
#define CSI_PRGITR_LB2VC_Pos            (8U)
#define CSI_PRGITR_LB2VC_Msk            (0x3UL << CSI_PRGITR_LB2VC_Pos)           /*!< 0x00000300 */
#define CSI_PRGITR_LB2VC                CSI_PRGITR_LB2VC_Msk                     /*!< Line/Byte counter 2 linked to a virtual channel */
#define CSI_PRGITR_LB2EN_Pos            (11U)
#define CSI_PRGITR_LB2EN_Msk            (0x1UL << CSI_PRGITR_LB2EN_Pos)           /*!< 0x00000800 */
#define CSI_PRGITR_LB2EN                CSI_PRGITR_LB2EN_Msk                     /*!< Line/Byte 2 counter enable */
#define CSI_PRGITR_LB3VC_Pos            (12U)
#define CSI_PRGITR_LB3VC_Msk            (0x3UL << CSI_PRGITR_LB3VC_Pos)           /*!< 0x00003000 */
#define CSI_PRGITR_LB3VC                CSI_PRGITR_LB3VC_Msk                     /*!< Line/Byte counter 3 linked to a virtual channel */
#define CSI_PRGITR_LB3EN_Pos            (15U)
#define CSI_PRGITR_LB3EN_Msk            (0x1UL << CSI_PRGITR_LB3EN_Pos)           /*!< 0x00008000 */
#define CSI_PRGITR_LB3EN                CSI_PRGITR_LB3EN_Msk                     /*!< Line/Byte 3 counter enable */
#define CSI_PRGITR_TIM0VC_Pos           (16U)
#define CSI_PRGITR_TIM0VC_Msk           (0x3UL << CSI_PRGITR_TIM0VC_Pos)          /*!< 0x00030000 */
#define CSI_PRGITR_TIM0VC               CSI_PRGITR_TIM0VC_Msk                    /*!< TIM0 base time linked to a virtual channel */
#define CSI_PRGITR_TIM0EOF_Pos          (18U)
#define CSI_PRGITR_TIM0EOF_Msk          (0x1UL << CSI_PRGITR_TIM0EOF_Pos)         /*!< 0x00040000 */
#define CSI_PRGITR_TIM0EOF              CSI_PRGITR_TIM0EOF_Msk                   /*!< TIM0 base time starting from the end of frame */
#define CSI_PRGITR_TIM0EN_Pos           (19U)
#define CSI_PRGITR_TIM0EN_Msk           (0x1UL << CSI_PRGITR_TIM0EN_Pos)          /*!< 0x00080000 */
#define CSI_PRGITR_TIM0EN               CSI_PRGITR_TIM0EN_Msk                    /*!< TIM0 base time enable */
#define CSI_PRGITR_TIM1VC_Pos           (20U)
#define CSI_PRGITR_TIM1VC_Msk           (0x3UL << CSI_PRGITR_TIM1VC_Pos)          /*!< 0x00300000 */
#define CSI_PRGITR_TIM1VC               CSI_PRGITR_TIM1VC_Msk                    /*!< TIM1 base time linked to a virtual channel */
#define CSI_PRGITR_TIM1EOF_Pos          (22U)
#define CSI_PRGITR_TIM1EOF_Msk          (0x1UL << CSI_PRGITR_TIM1EOF_Pos)         /*!< 0x00400000 */
#define CSI_PRGITR_TIM1EOF              CSI_PRGITR_TIM1EOF_Msk                   /*!< TIM1 base time starting from the end of frame */
#define CSI_PRGITR_TIM1EN_Pos           (23U)
#define CSI_PRGITR_TIM1EN_Msk           (0x1UL << CSI_PRGITR_TIM1EN_Pos)          /*!< 0x00800000 */
#define CSI_PRGITR_TIM1EN               CSI_PRGITR_TIM1EN_Msk                    /*!< TIM1 base time enable */
#define CSI_PRGITR_TIM2VC_Pos           (24U)
#define CSI_PRGITR_TIM2VC_Msk           (0x3UL << CSI_PRGITR_TIM2VC_Pos)          /*!< 0x03000000 */
#define CSI_PRGITR_TIM2VC               CSI_PRGITR_TIM2VC_Msk                    /*!< TIM2 base time linked to a virtual channel */
#define CSI_PRGITR_TIM2EOF_Pos          (26U)
#define CSI_PRGITR_TIM2EOF_Msk          (0x1UL << CSI_PRGITR_TIM2EOF_Pos)         /*!< 0x04000000 */
#define CSI_PRGITR_TIM2EOF              CSI_PRGITR_TIM2EOF_Msk                   /*!< TIM2 base time starting from the end of frame */
#define CSI_PRGITR_TIM2EN_Pos           (27U)
#define CSI_PRGITR_TIM2EN_Msk           (0x1UL << CSI_PRGITR_TIM2EN_Pos)          /*!< 0x08000000 */
#define CSI_PRGITR_TIM2EN               CSI_PRGITR_TIM2EN_Msk                    /*!< TIM2 base time enable */
#define CSI_PRGITR_TIM3VC_Pos           (28U)
#define CSI_PRGITR_TIM3VC_Msk           (0x3UL << CSI_PRGITR_TIM3VC_Pos)          /*!< 0x30000000 */
#define CSI_PRGITR_TIM3VC               CSI_PRGITR_TIM3VC_Msk                    /*!< TIM3 base time linked to a virtual channel */
#define CSI_PRGITR_TIM3EOF_Pos          (30U)
#define CSI_PRGITR_TIM3EOF_Msk          (0x1UL << CSI_PRGITR_TIM3EOF_Pos)         /*!< 0x40000000 */
#define CSI_PRGITR_TIM3EOF              CSI_PRGITR_TIM3EOF_Msk                   /*!< TIM3 base time starting from the end of frame */
#define CSI_PRGITR_TIM3EN_Pos           (31U)
#define CSI_PRGITR_TIM3EN_Msk           (0x1UL << CSI_PRGITR_TIM3EN_Pos)          /*!< 0x80000000 */
#define CSI_PRGITR_TIM3EN               CSI_PRGITR_TIM3EN_Msk                    /*!< TIM3 base time enable */

/*******************  Bit definition for CSI_WDR register  ********************/
#define CSI_WDR_CNT_Pos                 (0U)
#define CSI_WDR_CNT_Msk                 (0xFFFFFFFFUL << CSI_WDR_CNT_Pos)         /*!< 0xFFFFFFFF */
#define CSI_WDR_CNT                     CSI_WDR_CNT_Msk                          /*!< Watchdog counter */

/*******************  Bit definition for CSI_IER0 register  *******************/
#define CSI_IER0_LB0IE_Pos              (0U)
#define CSI_IER0_LB0IE_Msk              (0x1UL << CSI_IER0_LB0IE_Pos)             /*!< 0x00000001 */
#define CSI_IER0_LB0IE                  CSI_IER0_LB0IE_Msk                       /*!< Line byte counter 0 interrupt enable */
#define CSI_IER0_LB1IE_Pos              (1U)
#define CSI_IER0_LB1IE_Msk              (0x1UL << CSI_IER0_LB1IE_Pos)             /*!< 0x00000002 */
#define CSI_IER0_LB1IE                  CSI_IER0_LB1IE_Msk                       /*!< Line byte counter 1 interrupt enable */
#define CSI_IER0_LB2IE_Pos              (2U)
#define CSI_IER0_LB2IE_Msk              (0x1UL << CSI_IER0_LB2IE_Pos)             /*!< 0x00000004 */
#define CSI_IER0_LB2IE                  CSI_IER0_LB2IE_Msk                       /*!< Line byte counter 2 interrupt enable */
#define CSI_IER0_LB3IE_Pos              (3U)
#define CSI_IER0_LB3IE_Msk              (0x1UL << CSI_IER0_LB3IE_Pos)             /*!< 0x00000008 */
#define CSI_IER0_LB3IE                  CSI_IER0_LB3IE_Msk                       /*!< Line byte counter 3 interrupt enable */
#define CSI_IER0_TIM0IE_Pos             (4U)
#define CSI_IER0_TIM0IE_Msk             (0x1UL << CSI_IER0_TIM0IE_Pos)            /*!< 0x00000010 */
#define CSI_IER0_TIM0IE                 CSI_IER0_TIM0IE_Msk                      /*!< Timer 0 interrupt enable */
#define CSI_IER0_TIM1IE_Pos             (5U)
#define CSI_IER0_TIM1IE_Msk             (0x1UL << CSI_IER0_TIM1IE_Pos)            /*!< 0x00000020 */
#define CSI_IER0_TIM1IE                 CSI_IER0_TIM1IE_Msk                      /*!< Timer 1 interrupt enable */
#define CSI_IER0_TIM2IE_Pos             (6U)
#define CSI_IER0_TIM2IE_Msk             (0x1UL << CSI_IER0_TIM2IE_Pos)            /*!< 0x00000040 */
#define CSI_IER0_TIM2IE                 CSI_IER0_TIM2IE_Msk                      /*!< Timer 2 interrupt enable */
#define CSI_IER0_TIM3IE_Pos             (7U)
#define CSI_IER0_TIM3IE_Msk             (0x1UL << CSI_IER0_TIM3IE_Pos)            /*!< 0x00000080 */
#define CSI_IER0_TIM3IE                 CSI_IER0_TIM3IE_Msk                      /*!< Timer 3 interrupt enable */
#define CSI_IER0_SOF0IE_Pos             (8U)
#define CSI_IER0_SOF0IE_Msk             (0x1UL << CSI_IER0_SOF0IE_Pos)            /*!< 0x00000100 */
#define CSI_IER0_SOF0IE                 CSI_IER0_SOF0IE_Msk                      /*!< Start of frame for virtual channel 0 interrupt enable */
#define CSI_IER0_SOF1IE_Pos             (9U)
#define CSI_IER0_SOF1IE_Msk             (0x1UL << CSI_IER0_SOF1IE_Pos)            /*!< 0x00000200 */
#define CSI_IER0_SOF1IE                 CSI_IER0_SOF1IE_Msk                      /*!< Start of frame for virtual channel 1 interrupt enable */
#define CSI_IER0_SOF2IE_Pos             (10U)
#define CSI_IER0_SOF2IE_Msk             (0x1UL << CSI_IER0_SOF2IE_Pos)            /*!< 0x00000400 */
#define CSI_IER0_SOF2IE                 CSI_IER0_SOF2IE_Msk                      /*!< Start of frame for virtual channel 2 interrupt enable */
#define CSI_IER0_SOF3IE_Pos             (11U)
#define CSI_IER0_SOF3IE_Msk             (0x1UL << CSI_IER0_SOF3IE_Pos)            /*!< 0x00000800 */
#define CSI_IER0_SOF3IE                 CSI_IER0_SOF3IE_Msk                      /*!< Start of frame for virtual channel 3 interrupt enable */
#define CSI_IER0_EOF0IE_Pos             (12U)
#define CSI_IER0_EOF0IE_Msk             (0x1UL << CSI_IER0_EOF0IE_Pos)            /*!< 0x00001000 */
#define CSI_IER0_EOF0IE                 CSI_IER0_EOF0IE_Msk                      /*!< End of frame for virtual channel 0 interrupt enable */
#define CSI_IER0_EOF1IE_Pos             (13U)
#define CSI_IER0_EOF1IE_Msk             (0x1UL << CSI_IER0_EOF1IE_Pos)            /*!< 0x00002000 */
#define CSI_IER0_EOF1IE                 CSI_IER0_EOF1IE_Msk                      /*!< End of frame for virtual channel 1 interrupt enable */
#define CSI_IER0_EOF2IE_Pos             (14U)
#define CSI_IER0_EOF2IE_Msk             (0x1UL << CSI_IER0_EOF2IE_Pos)            /*!< 0x00004000 */
#define CSI_IER0_EOF2IE                 CSI_IER0_EOF2IE_Msk                      /*!< End of frame for virtual channel 2 interrupt enable */
#define CSI_IER0_EOF3IE_Pos             (15U)
#define CSI_IER0_EOF3IE_Msk             (0x1UL << CSI_IER0_EOF3IE_Pos)            /*!< 0x00008000 */
#define CSI_IER0_EOF3IE                 CSI_IER0_EOF3IE_Msk                      /*!< End of frame for virtual channel 3 interrupt enable */
#define CSI_IER0_SPKTIE_Pos             (16U)
#define CSI_IER0_SPKTIE_Msk             (0x1UL << CSI_IER0_SPKTIE_Pos)            /*!< 0x00010000 */
#define CSI_IER0_SPKTIE                 CSI_IER0_SPKTIE_Msk                      /*!< Short packet interrupt enable */
#define CSI_IER0_CCFIFOFIE_Pos          (21U)
#define CSI_IER0_CCFIFOFIE_Msk          (0x1UL << CSI_IER0_CCFIFOFIE_Pos)         /*!< 0x00200000 */
#define CSI_IER0_CCFIFOFIE              CSI_IER0_CCFIFOFIE_Msk                   /*!< Clock changer FIFO full interrupt enable */
#define CSI_IER0_CRCERRIE_Pos           (24U)
#define CSI_IER0_CRCERRIE_Msk           (0x1UL << CSI_IER0_CRCERRIE_Pos)          /*!< 0x01000000 */
#define CSI_IER0_CRCERRIE               CSI_IER0_CRCERRIE_Msk                    /*!< CRC error interrupt enable */
#define CSI_IER0_ECCERRIE_Pos           (25U)
#define CSI_IER0_ECCERRIE_Msk           (0x1UL << CSI_IER0_ECCERRIE_Pos)          /*!< 0x02000000 */
#define CSI_IER0_ECCERRIE               CSI_IER0_ECCERRIE_Msk                    /*!< ECC error interrupt enable */
#define CSI_IER0_CECCERRIE_Pos          (26U)
#define CSI_IER0_CECCERRIE_Msk          (0x1UL << CSI_IER0_CECCERRIE_Pos)         /*!< 0x04000000 */
#define CSI_IER0_CECCERRIE              CSI_IER0_CECCERRIE_Msk                   /*!< Corrected ECC error interrupt enable */
#define CSI_IER0_IDERRIE_Pos            (27U)
#define CSI_IER0_IDERRIE_Msk            (0x1UL << CSI_IER0_IDERRIE_Pos)           /*!< 0x08000000 */
#define CSI_IER0_IDERRIE                CSI_IER0_IDERRIE_Msk                     /*!< Data type ID error interrupt enable */
#define CSI_IER0_SPKTERRIE_Pos          (28U)
#define CSI_IER0_SPKTERRIE_Msk          (0x1UL << CSI_IER0_SPKTERRIE_Pos)         /*!< 0x10000000 */
#define CSI_IER0_SPKTERRIE              CSI_IER0_SPKTERRIE_Msk                   /*!< Short packet error interrupt enable */
#define CSI_IER0_WDERRIE_Pos            (29U)
#define CSI_IER0_WDERRIE_Msk            (0x1UL << CSI_IER0_WDERRIE_Pos)           /*!< 0x20000000 */
#define CSI_IER0_WDERRIE                CSI_IER0_WDERRIE_Msk                     /*!< Watchdog error interrupt enable */
#define CSI_IER0_SYNCERRIE_Pos          (30U)
#define CSI_IER0_SYNCERRIE_Msk          (0x1UL << CSI_IER0_SYNCERRIE_Pos)         /*!< 0x40000000 */
#define CSI_IER0_SYNCERRIE              CSI_IER0_SYNCERRIE_Msk                   /*!< Invalid synchronization error interrupt enable */

/*******************  Bit definition for CSI_IER1 register  *******************/
#define CSI_IER1_ESOTDL0IE_Pos          (0U)
#define CSI_IER1_ESOTDL0IE_Msk          (0x1UL << CSI_IER1_ESOTDL0IE_Pos)         /*!< 0x00000001 */
#define CSI_IER1_ESOTDL0IE              CSI_IER1_ESOTDL0IE_Msk                   /*!< Start of transmission error interrupt enable on lane 0 */
#define CSI_IER1_ESOTSYNCDL0IE_Pos      (1U)
#define CSI_IER1_ESOTSYNCDL0IE_Msk      (0x1UL << CSI_IER1_ESOTSYNCDL0IE_Pos)     /*!< 0x00000002 */
#define CSI_IER1_ESOTSYNCDL0IE          CSI_IER1_ESOTSYNCDL0IE_Msk               /*!< Start of transmission synchronization interrupt error enable on lane 0 */
#define CSI_IER1_EESCDL0IE_Pos          (2U)
#define CSI_IER1_EESCDL0IE_Msk          (0x1UL << CSI_IER1_EESCDL0IE_Pos)         /*!< 0x00000004 */
#define CSI_IER1_EESCDL0IE              CSI_IER1_EESCDL0IE_Msk                   /*!< D-PHY_RX lane 0 escape entry error interrupt enable */
#define CSI_IER1_ESYNCESCDL0IE_Pos      (3U)
#define CSI_IER1_ESYNCESCDL0IE_Msk      (0x1UL << CSI_IER1_ESYNCESCDL0IE_Pos)     /*!< 0x00000008 */
#define CSI_IER1_ESYNCESCDL0IE          CSI_IER1_ESYNCESCDL0IE_Msk               /*!< D-PHY_RX lane 0 low power data transmission synchronization error interrupt enable */
#define CSI_IER1_ECTRLDL0IE_Pos         (4U)
#define CSI_IER1_ECTRLDL0IE_Msk         (0x1UL << CSI_IER1_ECTRLDL0IE_Pos)        /*!< 0x00000010 */
#define CSI_IER1_ECTRLDL0IE             CSI_IER1_ECTRLDL0IE_Msk                  /*!< D-PHY_RX lane 0 control error interrupt enable */
#define CSI_IER1_ESOTDL1IE_Pos          (8U)
#define CSI_IER1_ESOTDL1IE_Msk          (0x1UL << CSI_IER1_ESOTDL1IE_Pos)         /*!< 0x00000100 */
#define CSI_IER1_ESOTDL1IE              CSI_IER1_ESOTDL1IE_Msk                   /*!< Start of transmission error interrupt enable on lane 1 */
#define CSI_IER1_ESOTSYNCDL1IE_Pos      (9U)
#define CSI_IER1_ESOTSYNCDL1IE_Msk      (0x1UL << CSI_IER1_ESOTSYNCDL1IE_Pos)     /*!< 0x00000200 */
#define CSI_IER1_ESOTSYNCDL1IE          CSI_IER1_ESOTSYNCDL1IE_Msk               /*!< Start of transmission synchronization interrupt error enable on lane 1 */
#define CSI_IER1_EESCDL1IE_Pos          (10U)
#define CSI_IER1_EESCDL1IE_Msk          (0x1UL << CSI_IER1_EESCDL1IE_Pos)         /*!< 0x00000400 */
#define CSI_IER1_EESCDL1IE              CSI_IER1_EESCDL1IE_Msk                   /*!< D-PHY_RX lane 1 escape entry error interrupt enable */
#define CSI_IER1_ESYNCESCDL1IE_Pos      (11U)
#define CSI_IER1_ESYNCESCDL1IE_Msk      (0x1UL << CSI_IER1_ESYNCESCDL1IE_Pos)     /*!< 0x00000800 */
#define CSI_IER1_ESYNCESCDL1IE          CSI_IER1_ESYNCESCDL1IE_Msk               /*!< D-PHY_RX lane 1 low power data transmission synchronization error interrupt enable */
#define CSI_IER1_ECTRLDL1IE_Pos         (12U)
#define CSI_IER1_ECTRLDL1IE_Msk         (0x1UL << CSI_IER1_ECTRLDL1IE_Pos)        /*!< 0x00001000 */
#define CSI_IER1_ECTRLDL1IE             CSI_IER1_ECTRLDL1IE_Msk                  /*!< D-PHY_RX lane 1 control error interrupt enable */

/*******************  Bit definition for CSI_SR0 register  ********************/
#define CSI_SR0_LB0F_Pos                (0U)
#define CSI_SR0_LB0F_Msk                (0x1UL << CSI_SR0_LB0F_Pos)               /*!< 0x00000001 */
#define CSI_SR0_LB0F                    CSI_SR0_LB0F_Msk                         /*!< Line byte counter 0 flag */
#define CSI_SR0_LB1F_Pos                (1U)
#define CSI_SR0_LB1F_Msk                (0x1UL << CSI_SR0_LB1F_Pos)               /*!< 0x00000002 */
#define CSI_SR0_LB1F                    CSI_SR0_LB1F_Msk                         /*!< Line byte counter 1 flag */
#define CSI_SR0_LB2F_Pos                (2U)
#define CSI_SR0_LB2F_Msk                (0x1UL << CSI_SR0_LB2F_Pos)               /*!< 0x00000004 */
#define CSI_SR0_LB2F                    CSI_SR0_LB2F_Msk                         /*!< Line byte counter 2 flag */
#define CSI_SR0_LB3F_Pos                (3U)
#define CSI_SR0_LB3F_Msk                (0x1UL << CSI_SR0_LB3F_Pos)               /*!< 0x00000008 */
#define CSI_SR0_LB3F                    CSI_SR0_LB3F_Msk                         /*!< Line byte counter 3 flag */
#define CSI_SR0_TIM0F_Pos               (4U)
#define CSI_SR0_TIM0F_Msk               (0x1UL << CSI_SR0_TIM0F_Pos)              /*!< 0x00000010 */
#define CSI_SR0_TIM0F                   CSI_SR0_TIM0F_Msk                        /*!< Timer 0 flag */
#define CSI_SR0_TIM1F_Pos               (5U)
#define CSI_SR0_TIM1F_Msk               (0x1UL << CSI_SR0_TIM1F_Pos)              /*!< 0x00000020 */
#define CSI_SR0_TIM1F                   CSI_SR0_TIM1F_Msk                        /*!< Timer 1 flag */
#define CSI_SR0_TIM2F_Pos               (6U)
#define CSI_SR0_TIM2F_Msk               (0x1UL << CSI_SR0_TIM2F_Pos)              /*!< 0x00000040 */
#define CSI_SR0_TIM2F                   CSI_SR0_TIM2F_Msk                        /*!< Timer 2 flag */
#define CSI_SR0_TIM3F_Pos               (7U)
#define CSI_SR0_TIM3F_Msk               (0x1UL << CSI_SR0_TIM3F_Pos)              /*!< 0x00000080 */
#define CSI_SR0_TIM3F                   CSI_SR0_TIM3F_Msk                        /*!< Timer 3 flag */
#define CSI_SR0_SOF0F_Pos               (8U)
#define CSI_SR0_SOF0F_Msk               (0x1UL << CSI_SR0_SOF0F_Pos)              /*!< 0x00000100 */
#define CSI_SR0_SOF0F                   CSI_SR0_SOF0F_Msk                        /*!< Start of frame flag for virtual channel 0 */
#define CSI_SR0_SOF1F_Pos               (9U)
#define CSI_SR0_SOF1F_Msk               (0x1UL << CSI_SR0_SOF1F_Pos)              /*!< 0x00000200 */
#define CSI_SR0_SOF1F                   CSI_SR0_SOF1F_Msk                        /*!< Start of frame flag for virtual channel 1 */
#define CSI_SR0_SOF2F_Pos               (10U)
#define CSI_SR0_SOF2F_Msk               (0x1UL << CSI_SR0_SOF2F_Pos)              /*!< 0x00000400 */
#define CSI_SR0_SOF2F                   CSI_SR0_SOF2F_Msk                        /*!< Start of frame flag for virtual channel 2 */
#define CSI_SR0_SOF3F_Pos               (11U)
#define CSI_SR0_SOF3F_Msk               (0x1UL << CSI_SR0_SOF3F_Pos)              /*!< 0x00000800 */
#define CSI_SR0_SOF3F                   CSI_SR0_SOF3F_Msk                        /*!< Start of frame flag for virtual channel 3 */
#define CSI_SR0_EOF0F_Pos               (12U)
#define CSI_SR0_EOF0F_Msk               (0x1UL << CSI_SR0_EOF0F_Pos)              /*!< 0x00001000 */
#define CSI_SR0_EOF0F                   CSI_SR0_EOF0F_Msk                        /*!< End of frame flag for virtual channel 0 */
#define CSI_SR0_EOF1F_Pos               (13U)
#define CSI_SR0_EOF1F_Msk               (0x1UL << CSI_SR0_EOF1F_Pos)              /*!< 0x00002000 */
#define CSI_SR0_EOF1F                   CSI_SR0_EOF1F_Msk                        /*!< End of frame flag for virtual channel 1 */
#define CSI_SR0_EOF2F_Pos               (14U)
#define CSI_SR0_EOF2F_Msk               (0x1UL << CSI_SR0_EOF2F_Pos)              /*!< 0x00004000 */
#define CSI_SR0_EOF2F                   CSI_SR0_EOF2F_Msk                        /*!< End of frame flag for virtual channel 2 */
#define CSI_SR0_EOF3F_Pos               (15U)
#define CSI_SR0_EOF3F_Msk               (0x1UL << CSI_SR0_EOF3F_Pos)              /*!< 0x00008000 */
#define CSI_SR0_EOF3F                   CSI_SR0_EOF3F_Msk                        /*!< End of frame flag for virtual channel 3 */
#define CSI_SR0_SPKTF_Pos               (16U)
#define CSI_SR0_SPKTF_Msk               (0x1UL << CSI_SR0_SPKTF_Pos)              /*!< 0x00010000 */
#define CSI_SR0_SPKTF                   CSI_SR0_SPKTF_Msk                        /*!< Short packet flag */
#define CSI_SR0_VC0STATEF_Pos           (17U)
#define CSI_SR0_VC0STATEF_Msk           (0x1UL << CSI_SR0_VC0STATEF_Pos)          /*!< 0x00020000 */
#define CSI_SR0_VC0STATEF               CSI_SR0_VC0STATEF_Msk                    /*!< Virtual channel 0 state flag */
#define CSI_SR0_VC1STATEF_Pos           (18U)
#define CSI_SR0_VC1STATEF_Msk           (0x1UL << CSI_SR0_VC1STATEF_Pos)          /*!< 0x00040000 */
#define CSI_SR0_VC1STATEF               CSI_SR0_VC1STATEF_Msk                    /*!< Virtual channel 1 state flag */
#define CSI_SR0_VC2STATEF_Pos           (19U)
#define CSI_SR0_VC2STATEF_Msk           (0x1UL << CSI_SR0_VC2STATEF_Pos)          /*!< 0x00080000 */
#define CSI_SR0_VC2STATEF               CSI_SR0_VC2STATEF_Msk                    /*!< Virtual channel 2 state flag */
#define CSI_SR0_VC3STATEF_Pos           (20U)
#define CSI_SR0_VC3STATEF_Msk           (0x1UL << CSI_SR0_VC3STATEF_Pos)          /*!< 0x00100000 */
#define CSI_SR0_VC3STATEF               CSI_SR0_VC3STATEF_Msk                    /*!< Virtual channel 3 state flag */
#define CSI_SR0_CCFIFOFF_Pos            (21U)
#define CSI_SR0_CCFIFOFF_Msk            (0x1UL << CSI_SR0_CCFIFOFF_Pos)           /*!< 0x00200000 */
#define CSI_SR0_CCFIFOFF                CSI_SR0_CCFIFOFF_Msk                     /*!< Clock changer FIFO full flag */
#define CSI_SR0_CRCERRF_Pos             (24U)
#define CSI_SR0_CRCERRF_Msk             (0x1UL << CSI_SR0_CRCERRF_Pos)            /*!< 0x01000000 */
#define CSI_SR0_CRCERRF                 CSI_SR0_CRCERRF_Msk                      /*!< CRC error flag */
#define CSI_SR0_ECCERRF_Pos             (25U)
#define CSI_SR0_ECCERRF_Msk             (0x1UL << CSI_SR0_ECCERRF_Pos)            /*!< 0x02000000 */
#define CSI_SR0_ECCERRF                 CSI_SR0_ECCERRF_Msk                      /*!< ECC error flag */
#define CSI_SR0_CECCERRF_Pos            (26U)
#define CSI_SR0_CECCERRF_Msk            (0x1UL << CSI_SR0_CECCERRF_Pos)           /*!< 0x04000000 */
#define CSI_SR0_CECCERRF                CSI_SR0_CECCERRF_Msk                     /*!< Corrected ECC error flag */
#define CSI_SR0_IDERRF_Pos              (27U)
#define CSI_SR0_IDERRF_Msk              (0x1UL << CSI_SR0_IDERRF_Pos)             /*!< 0x08000000 */
#define CSI_SR0_IDERRF                  CSI_SR0_IDERRF_Msk                       /*!< Data type ID error flag */
#define CSI_SR0_SPKTERRF_Pos            (28U)
#define CSI_SR0_SPKTERRF_Msk            (0x1UL << CSI_SR0_SPKTERRF_Pos)           /*!< 0x10000000 */
#define CSI_SR0_SPKTERRF                CSI_SR0_SPKTERRF_Msk                     /*!< Short packet error flag */
#define CSI_SR0_WDERRF_Pos              (29U)
#define CSI_SR0_WDERRF_Msk              (0x1UL << CSI_SR0_WDERRF_Pos)             /*!< 0x20000000 */
#define CSI_SR0_WDERRF                  CSI_SR0_WDERRF_Msk                       /*!< Watchdog error flag */
#define CSI_SR0_SYNCERRF_Pos            (30U)
#define CSI_SR0_SYNCERRF_Msk            (0x1UL << CSI_SR0_SYNCERRF_Pos)           /*!< 0x40000000 */
#define CSI_SR0_SYNCERRF                CSI_SR0_SYNCERRF_Msk                     /*!< Invalid synchronization error flag */

/*******************  Bit definition for CSI_SR1 register  ********************/
#define CSI_SR1_ESOTDL0F_Pos            (0U)
#define CSI_SR1_ESOTDL0F_Msk            (0x1UL << CSI_SR1_ESOTDL0F_Pos)           /*!< 0x00000001 */
#define CSI_SR1_ESOTDL0F                CSI_SR1_ESOTDL0F_Msk                     /*!< Start of transmission error flag on lane 0 */
#define CSI_SR1_ESOTSYNCDL0F_Pos        (1U)
#define CSI_SR1_ESOTSYNCDL0F_Msk        (0x1UL << CSI_SR1_ESOTSYNCDL0F_Pos)       /*!< 0x00000002 */
#define CSI_SR1_ESOTSYNCDL0F            CSI_SR1_ESOTSYNCDL0F_Msk                 /*!< Start of transmission synchronization error flag on lane 0 */
#define CSI_SR1_EESCDL0F_Pos            (2U)
#define CSI_SR1_EESCDL0F_Msk            (0x1UL << CSI_SR1_EESCDL0F_Pos)           /*!< 0x00000004 */
#define CSI_SR1_EESCDL0F                CSI_SR1_EESCDL0F_Msk                     /*!< D-PHY_RX lane 0 escape entry error flag */
#define CSI_SR1_ESYNCESCDL0F_Pos        (3U)
#define CSI_SR1_ESYNCESCDL0F_Msk        (0x1UL << CSI_SR1_ESYNCESCDL0F_Pos)       /*!< 0x00000008 */
#define CSI_SR1_ESYNCESCDL0F            CSI_SR1_ESYNCESCDL0F_Msk                 /*!< D-PHY_RX lane 0 low power data transmission synchronization error flag */
#define CSI_SR1_ECTRLDL0F_Pos           (4U)
#define CSI_SR1_ECTRLDL0F_Msk           (0x1UL << CSI_SR1_ECTRLDL0F_Pos)          /*!< 0x00000010 */
#define CSI_SR1_ECTRLDL0F               CSI_SR1_ECTRLDL0F_Msk                    /*!< D-PHY_RX lane 0 control error flag */
#define CSI_SR1_ESOTDL1F_Pos            (8U)
#define CSI_SR1_ESOTDL1F_Msk            (0x1UL << CSI_SR1_ESOTDL1F_Pos)           /*!< 0x00000100 */
#define CSI_SR1_ESOTDL1F                CSI_SR1_ESOTDL1F_Msk                     /*!< Start of transmission error flag on lane 1 */
#define CSI_SR1_ESOTSYNCDL1F_Pos        (9U)
#define CSI_SR1_ESOTSYNCDL1F_Msk        (0x1UL << CSI_SR1_ESOTSYNCDL1F_Pos)       /*!< 0x00000200 */
#define CSI_SR1_ESOTSYNCDL1F            CSI_SR1_ESOTSYNCDL1F_Msk                 /*!< Start of transmission synchronization error flag on lane 1 */
#define CSI_SR1_EESCDL1F_Pos            (10U)
#define CSI_SR1_EESCDL1F_Msk            (0x1UL << CSI_SR1_EESCDL1F_Pos)           /*!< 0x00000400 */
#define CSI_SR1_EESCDL1F                CSI_SR1_EESCDL1F_Msk                     /*!< D-PHY_RX lane 1 escape entry error flag */
#define CSI_SR1_ESYNCESCDL1F_Pos        (11U)
#define CSI_SR1_ESYNCESCDL1F_Msk        (0x1UL << CSI_SR1_ESYNCESCDL1F_Pos)       /*!< 0x00000800 */
#define CSI_SR1_ESYNCESCDL1F            CSI_SR1_ESYNCESCDL1F_Msk                 /*!< D-PHY_RX lane 1 low power data transmission synchronization error flag */
#define CSI_SR1_ECTRLDL1F_Pos           (12U)
#define CSI_SR1_ECTRLDL1F_Msk           (0x1UL << CSI_SR1_ECTRLDL1F_Pos)          /*!< 0x00001000 */
#define CSI_SR1_ECTRLDL1F               CSI_SR1_ECTRLDL1F_Msk                    /*!< D-PHY_RX lane 1 control error flag */
#define CSI_SR1_ACTDL0F_Pos             (16U)
#define CSI_SR1_ACTDL0F_Msk             (0x1UL << CSI_SR1_ACTDL0F_Pos)            /*!< 0x00010000 */
#define CSI_SR1_ACTDL0F                 CSI_SR1_ACTDL0F_Msk                      /*!< D-PHY_RX lane 0 High speed reception active */
#define CSI_SR1_SYNCDL0F_Pos            (17U)
#define CSI_SR1_SYNCDL0F_Msk            (0x1UL << CSI_SR1_SYNCDL0F_Pos)           /*!< 0x00020000 */
#define CSI_SR1_SYNCDL0F                CSI_SR1_SYNCDL0F_Msk                     /*!< D-PHY_RX lane 0 receiver synchronization observed */
#define CSI_SR1_SKCALDL0F_Pos           (18U)
#define CSI_SR1_SKCALDL0F_Msk           (0x1UL << CSI_SR1_SKCALDL0F_Pos)          /*!< 0x00040000 */
#define CSI_SR1_SKCALDL0F               CSI_SR1_SKCALDL0F_Msk                    /*!< D-PHY_RX lane 0 High speed skew calibration */
#define CSI_SR1_STOPDL0F_Pos            (19U)
#define CSI_SR1_STOPDL0F_Msk            (0x1UL << CSI_SR1_STOPDL0F_Pos)           /*!< 0x00080000 */
#define CSI_SR1_STOPDL0F                CSI_SR1_STOPDL0F_Msk                     /*!< D-PHY_RX receiver data lane 0 in stop state */
#define CSI_SR1_ULPNDL0F_Pos            (20U)
#define CSI_SR1_ULPNDL0F_Msk            (0x1UL << CSI_SR1_ULPNDL0F_Pos)           /*!< 0x00100000 */
#define CSI_SR1_ULPNDL0F                CSI_SR1_ULPNDL0F_Msk                     /*!< D-PHY_RX receiver Ultra low power state (not) Active on data lane 00 */
#define CSI_SR1_ACTDL1F_Pos             (22U)
#define CSI_SR1_ACTDL1F_Msk             (0x1UL << CSI_SR1_ACTDL1F_Pos)            /*!< 0x00400000 */
#define CSI_SR1_ACTDL1F                 CSI_SR1_ACTDL1F_Msk                      /*!< D-PHY_RX lane 1 High speed reception active */
#define CSI_SR1_SYNCDL1F_Pos            (23U)
#define CSI_SR1_SYNCDL1F_Msk            (0x1UL << CSI_SR1_SYNCDL1F_Pos)           /*!< 0x00800000 */
#define CSI_SR1_SYNCDL1F                CSI_SR1_SYNCDL1F_Msk                     /*!< D-PHY_RX lane 1 receiver synchronization observed */
#define CSI_SR1_SKCALDL1F_Pos           (24U)
#define CSI_SR1_SKCALDL1F_Msk           (0x1UL << CSI_SR1_SKCALDL1F_Pos)          /*!< 0x01000000 */
#define CSI_SR1_SKCALDL1F               CSI_SR1_SKCALDL1F_Msk                    /*!< D-PHY_RX lane 1 High speed skew calibration */
#define CSI_SR1_STOPDL1F_Pos            (25U)
#define CSI_SR1_STOPDL1F_Msk            (0x1UL << CSI_SR1_STOPDL1F_Pos)           /*!< 0x02000000 */
#define CSI_SR1_STOPDL1F                CSI_SR1_STOPDL1F_Msk                     /*!< D-PHY_RX receiver data lane 1 in stop state */
#define CSI_SR1_ULPNDL1F_Pos            (26U)
#define CSI_SR1_ULPNDL1F_Msk            (0x1UL << CSI_SR1_ULPNDL1F_Pos)           /*!< 0x04000000 */
#define CSI_SR1_ULPNDL1F                CSI_SR1_ULPNDL1F_Msk                     /*!< D-PHY_RX receiver Ultra low power state (not) Active on data lane 1 */
#define CSI_SR1_STOPCLF_Pos             (28U)
#define CSI_SR1_STOPCLF_Msk             (0x1UL << CSI_SR1_STOPCLF_Pos)            /*!< 0x10000000 */
#define CSI_SR1_STOPCLF                 CSI_SR1_STOPCLF_Msk                      /*!< D-PHY_RX receiver in stop state for the clock lane */
#define CSI_SR1_ULPNACTF_Pos            (29U)
#define CSI_SR1_ULPNACTF_Msk            (0x1UL << CSI_SR1_ULPNACTF_Pos)           /*!< 0x20000000 */
#define CSI_SR1_ULPNACTF                CSI_SR1_ULPNACTF_Msk                     /*!< D-PHY_RX receiver ULP state (not) active */
#define CSI_SR1_ULPNCLF_Pos             (30U)
#define CSI_SR1_ULPNCLF_Msk             (0x1UL << CSI_SR1_ULPNCLF_Pos)            /*!< 0x40000000 */
#define CSI_SR1_ULPNCLF                 CSI_SR1_ULPNCLF_Msk                      /*!< D-PHY_RX receiver Ultra-Low power state (not) on clock lane */
#define CSI_SR1_ACTCLF_Pos              (31U)
#define CSI_SR1_ACTCLF_Msk              (0x1UL << CSI_SR1_ACTCLF_Pos)             /*!< 0x80000000 */
#define CSI_SR1_ACTCLF                  CSI_SR1_ACTCLF_Msk                       /*!< D-PHY_RX receiver clock active flag */

/*******************  Bit definition for CSI_FCR0 register  *******************/
#define CSI_FCR0_CLB0F_Pos              (0U)
#define CSI_FCR0_CLB0F_Msk              (0x1UL << CSI_FCR0_CLB0F_Pos)             /*!< 0x00000001 */
#define CSI_FCR0_CLB0F                  CSI_FCR0_CLB0F_Msk                       /*!< Clear Line byte counter 0 flag */
#define CSI_FCR0_CLB1F_Pos              (1U)
#define CSI_FCR0_CLB1F_Msk              (0x1UL << CSI_FCR0_CLB1F_Pos)             /*!< 0x00000002 */
#define CSI_FCR0_CLB1F                  CSI_FCR0_CLB1F_Msk                       /*!< Clear Line byte counter 1 flag */
#define CSI_FCR0_CLB2F_Pos              (2U)
#define CSI_FCR0_CLB2F_Msk              (0x1UL << CSI_FCR0_CLB2F_Pos)             /*!< 0x00000004 */
#define CSI_FCR0_CLB2F                  CSI_FCR0_CLB2F_Msk                       /*!< Clear Line byte counter 2 flag */
#define CSI_FCR0_CLB3F_Pos              (3U)
#define CSI_FCR0_CLB3F_Msk              (0x1UL << CSI_FCR0_CLB3F_Pos)             /*!< 0x00000008 */
#define CSI_FCR0_CLB3F                  CSI_FCR0_CLB3F_Msk                       /*!< Clear Line byte counter 3 flag */
#define CSI_FCR0_CTIM0F_Pos             (4U)
#define CSI_FCR0_CTIM0F_Msk             (0x1UL << CSI_FCR0_CTIM0F_Pos)            /*!< 0x00000010 */
#define CSI_FCR0_CTIM0F                 CSI_FCR0_CTIM0F_Msk                      /*!< Clear Timer 0 flag */
#define CSI_FCR0_CTIM1F_Pos             (5U)
#define CSI_FCR0_CTIM1F_Msk             (0x1UL << CSI_FCR0_CTIM1F_Pos)            /*!< 0x00000020 */
#define CSI_FCR0_CTIM1F                 CSI_FCR0_CTIM1F_Msk                      /*!< Clear Timer 1 flag */
#define CSI_FCR0_CTIM2F_Pos             (6U)
#define CSI_FCR0_CTIM2F_Msk             (0x1UL << CSI_FCR0_CTIM2F_Pos)            /*!< 0x00000040 */
#define CSI_FCR0_CTIM2F                 CSI_FCR0_CTIM2F_Msk                      /*!< Clear Timer 2 flag */
#define CSI_FCR0_CTIM3F_Pos             (7U)
#define CSI_FCR0_CTIM3F_Msk             (0x1UL << CSI_FCR0_CTIM3F_Pos)            /*!< 0x00000080 */
#define CSI_FCR0_CTIM3F                 CSI_FCR0_CTIM3F_Msk                      /*!< Clear Timer 3 flag */
#define CSI_FCR0_CSOF0F_Pos             (8U)
#define CSI_FCR0_CSOF0F_Msk             (0x1UL << CSI_FCR0_CSOF0F_Pos)            /*!< 0x00000100 */
#define CSI_FCR0_CSOF0F                 CSI_FCR0_CSOF0F_Msk                      /*!< Clear Start of frame flag for virtual channel 0 */
#define CSI_FCR0_CSOF1F_Pos             (9U)
#define CSI_FCR0_CSOF1F_Msk             (0x1UL << CSI_FCR0_CSOF1F_Pos)            /*!< 0x00000200 */
#define CSI_FCR0_CSOF1F                 CSI_FCR0_CSOF1F_Msk                      /*!< Clear Start of frame flag for virtual channel 1 */
#define CSI_FCR0_CSOF2F_Pos             (10U)
#define CSI_FCR0_CSOF2F_Msk             (0x1UL << CSI_FCR0_CSOF2F_Pos)            /*!< 0x00000400 */
#define CSI_FCR0_CSOF2F                 CSI_FCR0_CSOF2F_Msk                      /*!< Clear Start of frame flag for virtual channel 2 */
#define CSI_FCR0_CSOF3F_Pos             (11U)
#define CSI_FCR0_CSOF3F_Msk             (0x1UL << CSI_FCR0_CSOF3F_Pos)            /*!< 0x00000800 */
#define CSI_FCR0_CSOF3F                 CSI_FCR0_CSOF3F_Msk                      /*!< Clear Start of frame flag for virtual channel 3 */
#define CSI_FCR0_CEOF0F_Pos             (12U)
#define CSI_FCR0_CEOF0F_Msk             (0x1UL << CSI_FCR0_CEOF0F_Pos)            /*!< 0x00001000 */
#define CSI_FCR0_CEOF0F                 CSI_FCR0_CEOF0F_Msk                      /*!< Clear End of frame flag for virtual channel 0 */
#define CSI_FCR0_CEOF1F_Pos             (13U)
#define CSI_FCR0_CEOF1F_Msk             (0x1UL << CSI_FCR0_CEOF1F_Pos)            /*!< 0x00002000 */
#define CSI_FCR0_CEOF1F                 CSI_FCR0_CEOF1F_Msk                      /*!< Clear End of frame flag for virtual channel 1 */
#define CSI_FCR0_CEOF2F_Pos             (14U)
#define CSI_FCR0_CEOF2F_Msk             (0x1UL << CSI_FCR0_CEOF2F_Pos)            /*!< 0x00004000 */
#define CSI_FCR0_CEOF2F                 CSI_FCR0_CEOF2F_Msk                      /*!< Clear End of frame flag for virtual channel 2 */
#define CSI_FCR0_CEOF3F_Pos             (15U)
#define CSI_FCR0_CEOF3F_Msk             (0x1UL << CSI_FCR0_CEOF3F_Pos)            /*!< 0x00008000 */
#define CSI_FCR0_CEOF3F                 CSI_FCR0_CEOF3F_Msk                      /*!< Clear End of frame flag for virtual channel 3 */
#define CSI_FCR0_CSPKTF_Pos             (16U)
#define CSI_FCR0_CSPKTF_Msk             (0x1UL << CSI_FCR0_CSPKTF_Pos)            /*!< 0x00010000 */
#define CSI_FCR0_CSPKTF                 CSI_FCR0_CSPKTF_Msk                      /*!< Clear Short packet flag */
#define CSI_FCR0_CCCFIFOFF_Pos          (21U)
#define CSI_FCR0_CCCFIFOFF_Msk          (0x1UL << CSI_FCR0_CCCFIFOFF_Pos)         /*!< 0x00200000 */
#define CSI_FCR0_CCCFIFOFF              CSI_FCR0_CCCFIFOFF_Msk                   /*!< Clear Clock changer FIFO full flag */
#define CSI_FCR0_CCRCERRF_Pos           (24U)
#define CSI_FCR0_CCRCERRF_Msk           (0x1UL << CSI_FCR0_CCRCERRF_Pos)          /*!< 0x01000000 */
#define CSI_FCR0_CCRCERRF               CSI_FCR0_CCRCERRF_Msk                    /*!< Clear CRC error flag */
#define CSI_FCR0_CECCERRF_Pos           (25U)
#define CSI_FCR0_CECCERRF_Msk           (0x1UL << CSI_FCR0_CECCERRF_Pos)          /*!< 0x02000000 */
#define CSI_FCR0_CECCERRF               CSI_FCR0_CECCERRF_Msk                    /*!< Clear ECC error flag */
#define CSI_FCR0_CCECCERRF_Pos          (26U)
#define CSI_FCR0_CCECCERRF_Msk          (0x1UL << CSI_FCR0_CCECCERRF_Pos)         /*!< 0x04000000 */
#define CSI_FCR0_CCECCERRF              CSI_FCR0_CCECCERRF_Msk                   /*!< Clear Corrected ECC error flag */
#define CSI_FCR0_CIDERRF_Pos            (27U)
#define CSI_FCR0_CIDERRF_Msk            (0x1UL << CSI_FCR0_CIDERRF_Pos)           /*!< 0x08000000 */
#define CSI_FCR0_CIDERRF                CSI_FCR0_CIDERRF_Msk                     /*!< Clear Data type ID error flag */
#define CSI_FCR0_CSPKTERRF_Pos          (28U)
#define CSI_FCR0_CSPKTERRF_Msk          (0x1UL << CSI_FCR0_CSPKTERRF_Pos)         /*!< 0x10000000 */
#define CSI_FCR0_CSPKTERRF              CSI_FCR0_CSPKTERRF_Msk                   /*!< Clear Short packet error flag */
#define CSI_FCR0_CWDERRF_Pos            (29U)
#define CSI_FCR0_CWDERRF_Msk            (0x1UL << CSI_FCR0_CWDERRF_Pos)           /*!< 0x20000000 */
#define CSI_FCR0_CWDERRF                CSI_FCR0_CWDERRF_Msk                     /*!< Clear Watchdog error flag */
#define CSI_FCR0_CSYNCERRF_Pos          (30U)
#define CSI_FCR0_CSYNCERRF_Msk          (0x1UL << CSI_FCR0_CSYNCERRF_Pos)         /*!< 0x40000000 */
#define CSI_FCR0_CSYNCERRF              CSI_FCR0_CSYNCERRF_Msk                   /*!< Clear Invalid synchronization error flag */

/*******************  Bit definition for CSI_FCR1 register  *******************/
#define CSI_FCR1_CESOTDL0F_Pos          (0U)
#define CSI_FCR1_CESOTDL0F_Msk          (0x1UL << CSI_FCR1_CESOTDL0F_Pos)         /*!< 0x00000001 */
#define CSI_FCR1_CESOTDL0F              CSI_FCR1_CESOTDL0F_Msk                   /*!< Clear Start of transmission error flag on lane 0 */
#define CSI_FCR1_CESOTSYNCDL0F_Pos      (1U)
#define CSI_FCR1_CESOTSYNCDL0F_Msk      (0x1UL << CSI_FCR1_CESOTSYNCDL0F_Pos)     /*!< 0x00000002 */
#define CSI_FCR1_CESOTSYNCDL0F          CSI_FCR1_CESOTSYNCDL0F_Msk               /*!< Clear Start of transmission synchronization error flag on lane 0 */
#define CSI_FCR1_CEESCDL0F_Pos          (2U)
#define CSI_FCR1_CEESCDL0F_Msk          (0x1UL << CSI_FCR1_CEESCDL0F_Pos)         /*!< 0x00000004 */
#define CSI_FCR1_CEESCDL0F              CSI_FCR1_CEESCDL0F_Msk                   /*!< Clear D-PHY_RX lane 0 escape entry error flag */
#define CSI_FCR1_CESYNCESCDL0F_Pos      (3U)
#define CSI_FCR1_CESYNCESCDL0F_Msk      (0x1UL << CSI_FCR1_CESYNCESCDL0F_Pos)     /*!< 0x00000008 */
#define CSI_FCR1_CESYNCESCDL0F          CSI_FCR1_CESYNCESCDL0F_Msk               /*!< Clear D-PHY_RX lane 0 low power data transmission synchronization error flag */
#define CSI_FCR1_CECTRLDL0F_Pos         (4U)
#define CSI_FCR1_CECTRLDL0F_Msk         (0x1UL << CSI_FCR1_CECTRLDL0F_Pos)        /*!< 0x00000010 */
#define CSI_FCR1_CECTRLDL0F             CSI_FCR1_CECTRLDL0F_Msk                  /*!< Clear D-PHY_RX lane 0 control error flag */
#define CSI_FCR1_CESOTDL1F_Pos          (8U)
#define CSI_FCR1_CESOTDL1F_Msk          (0x1UL << CSI_FCR1_CESOTDL1F_Pos)         /*!< 0x00000100 */
#define CSI_FCR1_CESOTDL1F              CSI_FCR1_CESOTDL1F_Msk                   /*!< Clear Start of transmission error flag on lane 1 */
#define CSI_FCR1_CESOTSYNCDL1F_Pos      (9U)
#define CSI_FCR1_CESOTSYNCDL1F_Msk      (0x1UL << CSI_FCR1_CESOTSYNCDL1F_Pos)     /*!< 0x00000200 */
#define CSI_FCR1_CESOTSYNCDL1F          CSI_FCR1_CESOTSYNCDL1F_Msk               /*!< Clear Start of transmission synchronization error flag on lane 1 */
#define CSI_FCR1_CEESCDL1F_Pos          (10U)
#define CSI_FCR1_CEESCDL1F_Msk          (0x1UL << CSI_FCR1_CEESCDL1F_Pos)         /*!< 0x00000400 */
#define CSI_FCR1_CEESCDL1F              CSI_FCR1_CEESCDL1F_Msk                   /*!< Clear D-PHY_RX lane 1 escape entry error flag */
#define CSI_FCR1_CESYNCESCDL1F_Pos      (11U)
#define CSI_FCR1_CESYNCESCDL1F_Msk      (0x1UL << CSI_FCR1_CESYNCESCDL1F_Pos)     /*!< 0x00000800 */
#define CSI_FCR1_CESYNCESCDL1F          CSI_FCR1_CESYNCESCDL1F_Msk               /*!< Clear D-PHY_RX lane 1 low power data transmission synchronization error flag */
#define CSI_FCR1_CECTRLDL1F_Pos         (12U)
#define CSI_FCR1_CECTRLDL1F_Msk         (0x1UL << CSI_FCR1_CECTRLDL1F_Pos)        /*!< 0x00001000 */
#define CSI_FCR1_CECTRLDL1F             CSI_FCR1_CECTRLDL1F_Msk                  /*!< Clear D-PHY_RX lane 1 control error flag */

/******************  Bit definition for CSI_SPDFR register  *******************/
#define CSI_SPDFR_DATAFIELD_Pos         (0U)
#define CSI_SPDFR_DATAFIELD_Msk         (0xFFFFUL << CSI_SPDFR_DATAFIELD_Pos)     /*!< 0x0000FFFF */
#define CSI_SPDFR_DATAFIELD             CSI_SPDFR_DATAFIELD_Msk                  /*!< Data field */
#define CSI_SPDFR_DATATYPE_Pos          (16U)
#define CSI_SPDFR_DATATYPE_Msk          (0x3FUL << CSI_SPDFR_DATATYPE_Pos)        /*!< 0x003F0000 */
#define CSI_SPDFR_DATATYPE              CSI_SPDFR_DATATYPE_Msk                   /*!< Data type class */
#define CSI_SPDFR_VCHANNEL_Pos          (22U)
#define CSI_SPDFR_VCHANNEL_Msk          (0x3UL << CSI_SPDFR_VCHANNEL_Pos)         /*!< 0x00C00000 */
#define CSI_SPDFR_VCHANNEL              CSI_SPDFR_VCHANNEL_Msk                   /*!< Virtual channel */

/*******************  Bit definition for CSI_ERR1 register  *******************/
#define CSI_ERR1_CRCDTERR_Pos           (0U)
#define CSI_ERR1_CRCDTERR_Msk           (0x3FUL << CSI_ERR1_CRCDTERR_Pos)         /*!< 0x0000003F */
#define CSI_ERR1_CRCDTERR               CSI_ERR1_CRCDTERR_Msk                    /*!< Data type having a CRC error */
#define CSI_ERR1_CRCVCERR_Pos           (6U)
#define CSI_ERR1_CRCVCERR_Msk           (0x3UL << CSI_ERR1_CRCVCERR_Pos)          /*!< 0x000000C0 */
#define CSI_ERR1_CRCVCERR               CSI_ERR1_CRCVCERR_Msk                    /*!< Virtual channel having a CRC error */
#define CSI_ERR1_CECCDTERR_Pos          (8U)
#define CSI_ERR1_CECCDTERR_Msk          (0x3FUL << CSI_ERR1_CECCDTERR_Pos)        /*!< 0x00003F00 */
#define CSI_ERR1_CECCDTERR              CSI_ERR1_CECCDTERR_Msk                   /*!< Data type having a corrected ECC error */
#define CSI_ERR1_CECCVCERR_Pos          (14U)
#define CSI_ERR1_CECCVCERR_Msk          (0x3UL << CSI_ERR1_CECCVCERR_Pos)         /*!< 0x0000C000 */
#define CSI_ERR1_CECCVCERR              CSI_ERR1_CECCVCERR_Msk                   /*!< Virtual channel having a corrected ECC error */
#define CSI_ERR1_IDDTERR_Pos            (16U)
#define CSI_ERR1_IDDTERR_Msk            (0x3FUL << CSI_ERR1_IDDTERR_Pos)          /*!< 0x003F0000 */
#define CSI_ERR1_IDDTERR                CSI_ERR1_IDDTERR_Msk                     /*!< Data type in error */
#define CSI_ERR1_IDVCERR_Pos            (22U)
#define CSI_ERR1_IDVCERR_Msk            (0x3UL << CSI_ERR1_IDVCERR_Pos)           /*!< 0x00C00000 */
#define CSI_ERR1_IDVCERR                CSI_ERR1_IDVCERR_Msk                     /*!< Virtual channel having ID error */

/*******************  Bit definition for CSI_ERR2 register  *******************/
#define CSI_ERR2_SPKTDTERR_Pos          (0U)
#define CSI_ERR2_SPKTDTERR_Msk          (0x3FUL << CSI_ERR2_SPKTDTERR_Pos)        /*!< 0x0000003F */
#define CSI_ERR2_SPKTDTERR              CSI_ERR2_SPKTDTERR_Msk                   /*!< Data type having a short packet error */
#define CSI_ERR2_SPKTVCERR_Pos          (6U)
#define CSI_ERR2_SPKTVCERR_Msk          (0x3UL << CSI_ERR2_SPKTVCERR_Pos)         /*!< 0x000000C0 */
#define CSI_ERR2_SPKTVCERR              CSI_ERR2_SPKTVCERR_Msk                   /*!< Virtual channel having a short packet error */
#define CSI_ERR2_WDVCERR_Pos            (16U)
#define CSI_ERR2_WDVCERR_Msk            (0x3UL << CSI_ERR2_WDVCERR_Pos)           /*!< 0x00030000 */
#define CSI_ERR2_WDVCERR                CSI_ERR2_WDVCERR_Msk                     /*!< Virtual channel having a watchdog error */
#define CSI_ERR2_SYNCVCERR_Pos          (18U)
#define CSI_ERR2_SYNCVCERR_Msk          (0x3UL << CSI_ERR2_SYNCVCERR_Pos)         /*!< 0x000C0000 */
#define CSI_ERR2_SYNCVCERR              CSI_ERR2_SYNCVCERR_Msk                   /*!< Virtual channel having synchronization error */

/*******************  Bit definition for CSI_PRCR register  *******************/
#define CSI_PRCR_PEN_Pos                (1U)
#define CSI_PRCR_PEN_Msk                (0x1UL << CSI_PRCR_PEN_Pos)               /*!< 0x00000002 */
#define CSI_PRCR_PEN                    CSI_PRCR_PEN_Msk                         /*!< When set to 0, this bit places the digital section of the D-PHY in the reset state */

/*******************  Bit definition for CSI_PMCR register  *******************/
#define CSI_PMCR_FRXMDL0_Pos            (0U)
#define CSI_PMCR_FRXMDL0_Msk            (0x1UL << CSI_PMCR_FRXMDL0_Pos)           /*!< 0x00000001 */
#define CSI_PMCR_FRXMDL0                CSI_PMCR_FRXMDL0_Msk                     /*!< Force to Rx Mode the Data Lane 0 */
#define CSI_PMCR_FRXMDL1_Pos            (1U)
#define CSI_PMCR_FRXMDL1_Msk            (0x1UL << CSI_PMCR_FRXMDL1_Pos)           /*!< 0x00000002 */
#define CSI_PMCR_FRXMDL1                CSI_PMCR_FRXMDL1_Msk                     /*!< Force to Rx Mode the Data Lane 1 */
#define CSI_PMCR_FTXSMDL0_Pos           (2U)
#define CSI_PMCR_FTXSMDL0_Msk           (0x1UL << CSI_PMCR_FTXSMDL0_Pos)          /*!< 0x00000004 */
#define CSI_PMCR_FTXSMDL0               CSI_PMCR_FTXSMDL0_Msk                    /*!< Force to Tx Stop Mode the Data Lane 0 */
#define CSI_PMCR_DTDL_Pos               (4U)
#define CSI_PMCR_DTDL_Msk               (0x1UL << CSI_PMCR_DTDL_Pos)              /*!< 0x00000010 */
#define CSI_PMCR_DTDL                   CSI_PMCR_DTDL_Msk                        /*!< Disable Turn-around Data Lane 0 */
#define CSI_PMCR_RTDL0_Pos              (8U)
#define CSI_PMCR_RTDL0_Msk              (0x1UL << CSI_PMCR_RTDL0_Pos)             /*!< 0x00000100 */
#define CSI_PMCR_RTDL0                  CSI_PMCR_RTDL0_Msk                       /*!< Turn-around Request Data Lane 0 */
#define CSI_PMCR_TUESDL0_Pos            (12U)
#define CSI_PMCR_TUESDL0_Msk            (0x1UL << CSI_PMCR_TUESDL0_Pos)           /*!< 0x00001000 */
#define CSI_PMCR_TUESDL0                CSI_PMCR_TUESDL0_Msk                     /*!< Tx ULP Escape-mode Data Lane 0 */
#define CSI_PMCR_TUEXDL0_Pos            (16U)
#define CSI_PMCR_TUEXDL0_Msk            (0x1UL << CSI_PMCR_TUEXDL0_Pos)           /*!< 0x00010000 */
#define CSI_PMCR_TUEXDL0                CSI_PMCR_TUEXDL0_Msk                     /*!< Tx ULP Exit-sequence Data Lane 0 */

/*******************  Bit definition for CSI_PFCR register  *******************/
#define CSI_PFCR_CCFR_Pos               (0U)
#define CSI_PFCR_CCFR_Msk               (0x3FUL << CSI_PFCR_CCFR_Pos)             /*!< 0x0000003F */
#define CSI_PFCR_CCFR                   CSI_PFCR_CCFR_Msk                        /*!< Configuration Clock Frequency Range selection */
#define CSI_PFCR_HSFR_Pos               (8U)
#define CSI_PFCR_HSFR_Msk               (0x7FUL << CSI_PFCR_HSFR_Pos)             /*!< 0x00007F00 */
#define CSI_PFCR_HSFR                   CSI_PFCR_HSFR_Msk                        /*!< PHY-high-speed Frequency Range selection */
#define CSI_PFCR_DLD_Pos                (16U)
#define CSI_PFCR_DLD_Msk                (0x1UL << CSI_PFCR_DLD_Pos)               /*!< 0x00010000 */
#define CSI_PFCR_DLD                    CSI_PFCR_DLD_Msk                         /*!< Data Lane Direction of lane0 */

/******************  Bit definition for CSI_PTCR0 register  *******************/
#define CSI_PTCR0_TCKEN_Pos             (0U)
#define CSI_PTCR0_TCKEN_Msk             (0x1UL << CSI_PTCR0_TCKEN_Pos)            /*!< 0x00000001 */
#define CSI_PTCR0_TCKEN                 CSI_PTCR0_TCKEN_Msk                      /*!< Test-interface Clock Enable for the TDI bus into the PHY */
#define CSI_PTCR0_TRSEN_Pos             (1U)
#define CSI_PTCR0_TRSEN_Msk             (0x1UL << CSI_PTCR0_TRSEN_Pos)            /*!< 0x00000002 */
#define CSI_PTCR0_TRSEN                 CSI_PTCR0_TRSEN_Msk                      /*!< Test-interface Reset Enable for the TDI bus into the PHY */

/******************  Bit definition for CSI_PTCR1 register  *******************/
#define CSI_PTCR1_TDI_Pos               (0U)
#define CSI_PTCR1_TDI_Msk               (0xFFUL << CSI_PTCR1_TDI_Pos)             /*!< 0x000000FF */
#define CSI_PTCR1_TDI                   CSI_PTCR1_TDI_Msk                        /*!< Test-interface Data In */
#define CSI_PTCR1_TWM_Pos               (16U)
#define CSI_PTCR1_TWM_Msk               (0x1UL << CSI_PTCR1_TWM_Pos)              /*!< 0x00010000 */
#define CSI_PTCR1_TWM                   CSI_PTCR1_TWM_Msk                        /*!< Test-interface Write Mode selector */

/*******************  Bit definition for CSI_PTSR register  *******************/
#define CSI_PTSR_TDO_Pos                (0U)
#define CSI_PTSR_TDO_Msk                (0xFFUL << CSI_PTSR_TDO_Pos)              /*!< 0x000000FF */
#define CSI_PTSR_TDO                    CSI_PTSR_TDO_Msk                         /*!< CSI PHY test interface data output bus for read-back and internal probing functionalities */


/*********************************************************************************/
/*                                                                               */
/*                                DBGMCU                                         */
/*                                                                               */
/*********************************************************************************/

/*****************  Bit definition for DCMIPP_IPGR1 register  *****************/
#define DCMIPP_IPGR1_MEMORYPAGE_Pos         (0U)
#define DCMIPP_IPGR1_MEMORYPAGE_Msk         (0x7UL << DCMIPP_IPGR1_MEMORYPAGE_Pos)           /*!< 0x00000007 */
#define DCMIPP_IPGR1_MEMORYPAGE             DCMIPP_IPGR1_MEMORYPAGE_Msk                     /*!< Memory page size, as power of 2 of 64-byte units: */
#define DCMIPP_IPGR1_QOS_MODE_Pos           (24U)
#define DCMIPP_IPGR1_QOS_MODE_Msk           (0x1UL << DCMIPP_IPGR1_QOS_MODE_Pos)             /*!< 0x01000000 */
#define DCMIPP_IPGR1_QOS_MODE               DCMIPP_IPGR1_QOS_MODE_Msk                       /*!< Quality of service */

/*****************  Bit definition for DCMIPP_IPGR2 register  *****************/
#define DCMIPP_IPGR2_PSTART_Pos             (0U)
#define DCMIPP_IPGR2_PSTART_Msk             (0x1UL << DCMIPP_IPGR2_PSTART_Pos)               /*!< 0x00000001 */
#define DCMIPP_IPGR2_PSTART                 DCMIPP_IPGR2_PSTART_Msk                         /*!< Request to lock the IP-Plug, to allow reconfiguration */

/*****************  Bit definition for DCMIPP_IPGR3 register  *****************/
#define DCMIPP_IPGR3_IDLE_Pos               (0U)
#define DCMIPP_IPGR3_IDLE_Msk               (0x1UL << DCMIPP_IPGR3_IDLE_Pos)                 /*!< 0x00000001 */
#define DCMIPP_IPGR3_IDLE                   DCMIPP_IPGR3_IDLE_Msk                           /*!< Status of IP-Plug */

/*****************  Bit definition for DCMIPP_IPGR8 register  *****************/
#define DCMIPP_IPGR8_DID_Pos                (0U)
#define DCMIPP_IPGR8_DID_Msk                (0x3FUL << DCMIPP_IPGR8_DID_Pos)                 /*!< 0x0000003F */
#define DCMIPP_IPGR8_DID                    DCMIPP_IPGR8_DID_Msk                            /*!< Division identifier (0x14) */
#define DCMIPP_IPGR8_REVID_Pos              (8U)
#define DCMIPP_IPGR8_REVID_Msk              (0x1FUL << DCMIPP_IPGR8_REVID_Pos)               /*!< 0x00001F00 */
#define DCMIPP_IPGR8_REVID                  DCMIPP_IPGR8_REVID_Msk                          /*!< Revision identifier (0x03) */
#define DCMIPP_IPGR8_ARCHIID_Pos            (16U)
#define DCMIPP_IPGR8_ARCHIID_Msk            (0x1FUL << DCMIPP_IPGR8_ARCHIID_Pos)             /*!< 0x001F0000 */
#define DCMIPP_IPGR8_ARCHIID                DCMIPP_IPGR8_ARCHIID_Msk                        /*!< Architecture identifier (0x04) */
#define DCMIPP_IPGR8_IPPID_Pos              (24U)
#define DCMIPP_IPGR8_IPPID_Msk              (0xFFUL << DCMIPP_IPGR8_IPPID_Pos)               /*!< 0xFF000000 */
#define DCMIPP_IPGR8_IPPID                  DCMIPP_IPGR8_IPPID_Msk                          /*!< IP identifier (0xAA) */

/****************  Bit definition for DCMIPP_IPC1R1 register  *****************/
#define DCMIPP_IPC1R1_TRAFFIC_Pos           (0U)
#define DCMIPP_IPC1R1_TRAFFIC_Msk           (0x7UL << DCMIPP_IPC1R1_TRAFFIC_Pos)             /*!< 0x00000007 */
#define DCMIPP_IPC1R1_TRAFFIC               DCMIPP_IPC1R1_TRAFFIC_Msk                       /*!< Burst size as power of 2 of 8 bytes units */
#define DCMIPP_IPC1R1_OTR_Pos               (8U)
#define DCMIPP_IPC1R1_OTR_Msk               (0xFUL << DCMIPP_IPC1R1_OTR_Pos)                 /*!< 0x00000F00 */
#define DCMIPP_IPC1R1_OTR                   DCMIPP_IPC1R1_OTR_Msk                           /*!< max outstanding transactions: */

/****************  Bit definition for DCMIPP_IPC1R2 register  *****************/
#define DCMIPP_IPC1R2_WLRU_Pos              (16U)
#define DCMIPP_IPC1R2_WLRU_Msk              (0xFUL << DCMIPP_IPC1R2_WLRU_Pos)                /*!< 0x000F0000 */
#define DCMIPP_IPC1R2_WLRU                  DCMIPP_IPC1R2_WLRU_Msk                          /*!< Ratio for WLRU[3:0] arbitration: */

/****************  Bit definition for DCMIPP_IPC1R3 register  *****************/
#define DCMIPP_IPC1R3_DPREGSTART_Pos        (0U)
#define DCMIPP_IPC1R3_DPREGSTART_Msk        (0x3FFUL << DCMIPP_IPC1R3_DPREGSTART_Pos)        /*!< 0x000003FF */
#define DCMIPP_IPC1R3_DPREGSTART            DCMIPP_IPC1R3_DPREGSTART_Msk                    /*!< Start word (AXI width = 64 bits) of the FIFO of this client */
#define DCMIPP_IPC1R3_DPREGEND_Pos          (16U)
#define DCMIPP_IPC1R3_DPREGEND_Msk          (0x3FFUL << DCMIPP_IPC1R3_DPREGEND_Pos)          /*!< 0x03FF0000 */
#define DCMIPP_IPC1R3_DPREGEND              DCMIPP_IPC1R3_DPREGEND_Msk                      /*!< End word (AXI width = 64 bits) of the FIFO of this client */

/****************  Bit definition for DCMIPP_IPC2R1 register  *****************/
#define DCMIPP_IPC2R1_TRAFFIC_Pos           (0U)
#define DCMIPP_IPC2R1_TRAFFIC_Msk           (0x7UL << DCMIPP_IPC2R1_TRAFFIC_Pos)             /*!< 0x00000007 */
#define DCMIPP_IPC2R1_TRAFFIC               DCMIPP_IPC2R1_TRAFFIC_Msk                       /*!< Burst size as power of 2 of 8 bytes units */
#define DCMIPP_IPC2R1_OTR_Pos               (8U)
#define DCMIPP_IPC2R1_OTR_Msk               (0xFUL << DCMIPP_IPC2R1_OTR_Pos)                 /*!< 0x00000F00 */
#define DCMIPP_IPC2R1_OTR                   DCMIPP_IPC2R1_OTR_Msk                           /*!< max outstanding transactions: */

/****************  Bit definition for DCMIPP_IPC2R2 register  *****************/
#define DCMIPP_IPC2R2_WLRU_Pos              (16U)
#define DCMIPP_IPC2R2_WLRU_Msk              (0xFUL << DCMIPP_IPC2R2_WLRU_Pos)                /*!< 0x000F0000 */
#define DCMIPP_IPC2R2_WLRU                  DCMIPP_IPC2R2_WLRU_Msk                          /*!< Ratio for WLRU[3:0] arbitration: */

/****************  Bit definition for DCMIPP_IPC2R3 register  *****************/
#define DCMIPP_IPC2R3_DPREGSTART_Pos        (0U)
#define DCMIPP_IPC2R3_DPREGSTART_Msk        (0x3FFUL << DCMIPP_IPC2R3_DPREGSTART_Pos)        /*!< 0x000003FF */
#define DCMIPP_IPC2R3_DPREGSTART            DCMIPP_IPC2R3_DPREGSTART_Msk                    /*!< Start word (AXI width = 64 bits) of the FIFO of this client */
#define DCMIPP_IPC2R3_DPREGEND_Pos          (16U)
#define DCMIPP_IPC2R3_DPREGEND_Msk          (0x3FFUL << DCMIPP_IPC2R3_DPREGEND_Pos)          /*!< 0x03FF0000 */
#define DCMIPP_IPC2R3_DPREGEND              DCMIPP_IPC2R3_DPREGEND_Msk                      /*!< End word (AXI width = 64 bits) of the FIFO of this client */

/****************  Bit definition for DCMIPP_IPC3R1 register  *****************/
#define DCMIPP_IPC3R1_TRAFFIC_Pos           (0U)
#define DCMIPP_IPC3R1_TRAFFIC_Msk           (0x7UL << DCMIPP_IPC3R1_TRAFFIC_Pos)             /*!< 0x00000007 */
#define DCMIPP_IPC3R1_TRAFFIC               DCMIPP_IPC3R1_TRAFFIC_Msk                       /*!< Burst size as power of 2 of 8 bytes units */
#define DCMIPP_IPC3R1_OTR_Pos               (8U)
#define DCMIPP_IPC3R1_OTR_Msk               (0xFUL << DCMIPP_IPC3R1_OTR_Pos)                 /*!< 0x00000F00 */
#define DCMIPP_IPC3R1_OTR                   DCMIPP_IPC3R1_OTR_Msk                           /*!< max outstanding transactions: */

/****************  Bit definition for DCMIPP_IPC3R2 register  *****************/
#define DCMIPP_IPC3R2_WLRU_Pos              (16U)
#define DCMIPP_IPC3R2_WLRU_Msk              (0xFUL << DCMIPP_IPC3R2_WLRU_Pos)                /*!< 0x000F0000 */
#define DCMIPP_IPC3R2_WLRU                  DCMIPP_IPC3R2_WLRU_Msk                          /*!< Ratio for WLRU[3:0] arbitration: */

/****************  Bit definition for DCMIPP_IPC3R3 register  *****************/
#define DCMIPP_IPC3R3_DPREGSTART_Pos        (0U)
#define DCMIPP_IPC3R3_DPREGSTART_Msk        (0x3FFUL << DCMIPP_IPC3R3_DPREGSTART_Pos)        /*!< 0x000003FF */
#define DCMIPP_IPC3R3_DPREGSTART            DCMIPP_IPC3R3_DPREGSTART_Msk                    /*!< Start word (AXI width = 64 bits) of the FIFO of this client */
#define DCMIPP_IPC3R3_DPREGEND_Pos          (16U)
#define DCMIPP_IPC3R3_DPREGEND_Msk          (0x3FFUL << DCMIPP_IPC3R3_DPREGEND_Pos)          /*!< 0x03FF0000 */
#define DCMIPP_IPC3R3_DPREGEND              DCMIPP_IPC3R3_DPREGEND_Msk                      /*!< End word (AXI width = 64 bits) of the FIFO of this client */

/****************  Bit definition for DCMIPP_IPC4R1 register  *****************/
#define DCMIPP_IPC4R1_TRAFFIC_Pos           (0U)
#define DCMIPP_IPC4R1_TRAFFIC_Msk           (0x7UL << DCMIPP_IPC4R1_TRAFFIC_Pos)             /*!< 0x00000007 */
#define DCMIPP_IPC4R1_TRAFFIC               DCMIPP_IPC4R1_TRAFFIC_Msk                       /*!< Burst size as power of 2 of 8 bytes units */
#define DCMIPP_IPC4R1_OTR_Pos               (8U)
#define DCMIPP_IPC4R1_OTR_Msk               (0xFUL << DCMIPP_IPC4R1_OTR_Pos)                 /*!< 0x00000F00 */
#define DCMIPP_IPC4R1_OTR                   DCMIPP_IPC4R1_OTR_Msk                           /*!< max outstanding transactions: */

/****************  Bit definition for DCMIPP_IPC4R2 register  *****************/
#define DCMIPP_IPC4R2_WLRU_Pos              (16U)
#define DCMIPP_IPC4R2_WLRU_Msk              (0xFUL << DCMIPP_IPC4R2_WLRU_Pos)                /*!< 0x000F0000 */
#define DCMIPP_IPC4R2_WLRU                  DCMIPP_IPC4R2_WLRU_Msk                          /*!< Ratio for WLRU[3:0] arbitration: */

/****************  Bit definition for DCMIPP_IPC4R3 register  *****************/
#define DCMIPP_IPC4R3_DPREGSTART_Pos        (0U)
#define DCMIPP_IPC4R3_DPREGSTART_Msk        (0x3FFUL << DCMIPP_IPC4R3_DPREGSTART_Pos)        /*!< 0x000003FF */
#define DCMIPP_IPC4R3_DPREGSTART            DCMIPP_IPC4R3_DPREGSTART_Msk                    /*!< Start word (AXI width = 64 bits) of the FIFO of this client */
#define DCMIPP_IPC4R3_DPREGEND_Pos          (16U)
#define DCMIPP_IPC4R3_DPREGEND_Msk          (0x3FFUL << DCMIPP_IPC4R3_DPREGEND_Pos)          /*!< 0x03FF0000 */
#define DCMIPP_IPC4R3_DPREGEND              DCMIPP_IPC4R3_DPREGEND_Msk                      /*!< End word (AXI width = 64 bits) of the FIFO of this client */

/****************  Bit definition for DCMIPP_IPC5R1 register  *****************/
#define DCMIPP_IPC5R1_TRAFFIC_Pos           (0U)
#define DCMIPP_IPC5R1_TRAFFIC_Msk           (0x7UL << DCMIPP_IPC5R1_TRAFFIC_Pos)             /*!< 0x00000007 */
#define DCMIPP_IPC5R1_TRAFFIC               DCMIPP_IPC5R1_TRAFFIC_Msk                       /*!< Burst size as power of 2 of 8 bytes units */
#define DCMIPP_IPC5R1_OTR_Pos               (8U)
#define DCMIPP_IPC5R1_OTR_Msk               (0xFUL << DCMIPP_IPC5R1_OTR_Pos)                 /*!< 0x00000F00 */
#define DCMIPP_IPC5R1_OTR                   DCMIPP_IPC5R1_OTR_Msk                           /*!< max outstanding transactions: */

/****************  Bit definition for DCMIPP_IPC5R2 register  *****************/
#define DCMIPP_IPC5R2_WLRU_Pos              (16U)
#define DCMIPP_IPC5R2_WLRU_Msk              (0xFUL << DCMIPP_IPC5R2_WLRU_Pos)                /*!< 0x000F0000 */
#define DCMIPP_IPC5R2_WLRU                  DCMIPP_IPC5R2_WLRU_Msk                          /*!< Ratio for WLRU[3:0] arbitration: */

/****************  Bit definition for DCMIPP_IPC5R3 register  *****************/
#define DCMIPP_IPC5R3_DPREGSTART_Pos        (0U)
#define DCMIPP_IPC5R3_DPREGSTART_Msk        (0x3FFUL << DCMIPP_IPC5R3_DPREGSTART_Pos)        /*!< 0x000003FF */
#define DCMIPP_IPC5R3_DPREGSTART            DCMIPP_IPC5R3_DPREGSTART_Msk                    /*!< Start word (AXI width = 64 bits) of the FIFO of this client */
#define DCMIPP_IPC5R3_DPREGEND_Pos          (16U)
#define DCMIPP_IPC5R3_DPREGEND_Msk          (0x3FFUL << DCMIPP_IPC5R3_DPREGEND_Pos)          /*!< 0x03FF0000 */
#define DCMIPP_IPC5R3_DPREGEND              DCMIPP_IPC5R3_DPREGEND_Msk                      /*!< End word (AXI width = 64 bits) of the FIFO of this client */

/***************  Bit definition for DCMIPP_PRHWCFGR register  ****************/

/*****************  Bit definition for DCMIPP_PRCR register  ******************/
#define DCMIPP_PRCR_ESS_Pos                 (4U)
#define DCMIPP_PRCR_ESS_Msk                 (0x1UL << DCMIPP_PRCR_ESS_Pos)                   /*!< 0x00000010 */
#define DCMIPP_PRCR_ESS                     DCMIPP_PRCR_ESS_Msk                             /*!< Embedded synchronization select */
#define DCMIPP_PRCR_PCKPOL_Pos              (5U)
#define DCMIPP_PRCR_PCKPOL_Msk              (0x1UL << DCMIPP_PRCR_PCKPOL_Pos)                /*!< 0x00000020 */
#define DCMIPP_PRCR_PCKPOL                  DCMIPP_PRCR_PCKPOL_Msk                          /*!< Pixel clock polarity */
#define DCMIPP_PRCR_HSPOL_Pos               (6U)
#define DCMIPP_PRCR_HSPOL_Msk               (0x1UL << DCMIPP_PRCR_HSPOL_Pos)                 /*!< 0x00000040 */
#define DCMIPP_PRCR_HSPOL                   DCMIPP_PRCR_HSPOL_Msk                           /*!< Horizontal synchronization polarity */
#define DCMIPP_PRCR_VSPOL_Pos               (7U)
#define DCMIPP_PRCR_VSPOL_Msk               (0x1UL << DCMIPP_PRCR_VSPOL_Pos)                 /*!< 0x00000080 */
#define DCMIPP_PRCR_VSPOL                   DCMIPP_PRCR_VSPOL_Msk                           /*!< Vertical synchronization polarity */
#define DCMIPP_PRCR_EDM_Pos                 (10U)
#define DCMIPP_PRCR_EDM_Msk                 (0x7UL << DCMIPP_PRCR_EDM_Pos)                   /*!< 0x00001C00 */
#define DCMIPP_PRCR_EDM                     DCMIPP_PRCR_EDM_Msk                             /*!< Extended data mode */
#define DCMIPP_PRCR_ENABLE_Pos              (14U)
#define DCMIPP_PRCR_ENABLE_Msk              (0x1UL << DCMIPP_PRCR_ENABLE_Pos)                /*!< 0x00004000 */
#define DCMIPP_PRCR_ENABLE                  DCMIPP_PRCR_ENABLE_Msk                          /*!< Parallel interface enable */
#define DCMIPP_PRCR_FORMAT_Pos              (16U)
#define DCMIPP_PRCR_FORMAT_Msk              (0xFFUL << DCMIPP_PRCR_FORMAT_Pos)               /*!< 0x00FF0000 */
#define DCMIPP_PRCR_FORMAT                  DCMIPP_PRCR_FORMAT_Msk                          /*!< Other values: Data is captured and output as-is through the data/dump pipeline only (e */
#define DCMIPP_PRCR_SWAPCYCLES_Pos          (25U)
#define DCMIPP_PRCR_SWAPCYCLES_Msk          (0x1UL << DCMIPP_PRCR_SWAPCYCLES_Pos)            /*!< 0x02000000 */
#define DCMIPP_PRCR_SWAPCYCLES              DCMIPP_PRCR_SWAPCYCLES_Msk                      /*!< Swap data from cycle 0 vs */
#define DCMIPP_PRCR_SWAPBITS_Pos            (26U)
#define DCMIPP_PRCR_SWAPBITS_Msk            (0x1UL << DCMIPP_PRCR_SWAPBITS_Pos)              /*!< 0x04000000 */
#define DCMIPP_PRCR_SWAPBITS                DCMIPP_PRCR_SWAPBITS_Msk                        /*!< Swap LSB vs */

/****************  Bit definition for DCMIPP_PRESCR register  *****************/
#define DCMIPP_PRESCR_FSC_Pos               (0U)
#define DCMIPP_PRESCR_FSC_Msk               (0xFFUL << DCMIPP_PRESCR_FSC_Pos)                /*!< 0x000000FF */
#define DCMIPP_PRESCR_FSC                   DCMIPP_PRESCR_FSC_Msk                           /*!< Frame start delimiter code */
#define DCMIPP_PRESCR_LSC_Pos               (8U)
#define DCMIPP_PRESCR_LSC_Msk               (0xFFUL << DCMIPP_PRESCR_LSC_Pos)                /*!< 0x0000FF00 */
#define DCMIPP_PRESCR_LSC                   DCMIPP_PRESCR_LSC_Msk                           /*!< Line start delimiter code */
#define DCMIPP_PRESCR_LEC_Pos               (16U)
#define DCMIPP_PRESCR_LEC_Msk               (0xFFUL << DCMIPP_PRESCR_LEC_Pos)                /*!< 0x00FF0000 */
#define DCMIPP_PRESCR_LEC                   DCMIPP_PRESCR_LEC_Msk                           /*!< Line end delimiter code */
#define DCMIPP_PRESCR_FEC_Pos               (24U)
#define DCMIPP_PRESCR_FEC_Msk               (0xFFUL << DCMIPP_PRESCR_FEC_Pos)                /*!< 0xFF000000 */
#define DCMIPP_PRESCR_FEC                   DCMIPP_PRESCR_FEC_Msk                           /*!< Frame end delimiter code */

/****************  Bit definition for DCMIPP_PRESUR register  *****************/
#define DCMIPP_PRESUR_FSU_Pos               (0U)
#define DCMIPP_PRESUR_FSU_Msk               (0xFFUL << DCMIPP_PRESUR_FSU_Pos)                /*!< 0x000000FF */
#define DCMIPP_PRESUR_FSU                   DCMIPP_PRESUR_FSU_Msk                           /*!< Frame start delimiter unmask */
#define DCMIPP_PRESUR_LSU_Pos               (8U)
#define DCMIPP_PRESUR_LSU_Msk               (0xFFUL << DCMIPP_PRESUR_LSU_Pos)                /*!< 0x0000FF00 */
#define DCMIPP_PRESUR_LSU                   DCMIPP_PRESUR_LSU_Msk                           /*!< Line start delimiter unmask */
#define DCMIPP_PRESUR_LEU_Pos               (16U)
#define DCMIPP_PRESUR_LEU_Msk               (0xFFUL << DCMIPP_PRESUR_LEU_Pos)                /*!< 0x00FF0000 */
#define DCMIPP_PRESUR_LEU                   DCMIPP_PRESUR_LEU_Msk                           /*!< Line end delimiter unmask */
#define DCMIPP_PRESUR_FEU_Pos               (24U)
#define DCMIPP_PRESUR_FEU_Msk               (0xFFUL << DCMIPP_PRESUR_FEU_Pos)                /*!< 0xFF000000 */
#define DCMIPP_PRESUR_FEU                   DCMIPP_PRESUR_FEU_Msk                           /*!< Frame end delimiter unmask */

/*****************  Bit definition for DCMIPP_PRIER register  *****************/
#define DCMIPP_PRIER_ERRIE_Pos              (6U)
#define DCMIPP_PRIER_ERRIE_Msk              (0x1UL << DCMIPP_PRIER_ERRIE_Pos)                /*!< 0x00000040 */
#define DCMIPP_PRIER_ERRIE                  DCMIPP_PRIER_ERRIE_Msk                          /*!< Synchronization error interrupt enable */

/*****************  Bit definition for DCMIPP_PRSR register  ******************/
#define DCMIPP_PRSR_ERRF_Pos                (6U)
#define DCMIPP_PRSR_ERRF_Msk                (0x1UL << DCMIPP_PRSR_ERRF_Pos)                  /*!< 0x00000040 */
#define DCMIPP_PRSR_ERRF                    DCMIPP_PRSR_ERRF_Msk                            /*!< Synchronization error raw interrupt status */
#define DCMIPP_PRSR_HSYNC_Pos               (16U)
#define DCMIPP_PRSR_HSYNC_Msk               (0x1UL << DCMIPP_PRSR_HSYNC_Pos)                 /*!< 0x00010000 */
#define DCMIPP_PRSR_HSYNC                   DCMIPP_PRSR_HSYNC_Msk                           /*!< This bit gives the state of the HSYNC pin with the correct programmed polarity if the ENABLE bit is */
#define DCMIPP_PRSR_VSYNC_Pos               (17U)
#define DCMIPP_PRSR_VSYNC_Msk               (0x1UL << DCMIPP_PRSR_VSYNC_Pos)                 /*!< 0x00020000 */
#define DCMIPP_PRSR_VSYNC                   DCMIPP_PRSR_VSYNC_Msk                           /*!< This bit gives the state of the VSYNC pin with the correct programmed polarity if the ENABLE bit is */

/*****************  Bit definition for DCMIPP_PRFCR register  *****************/
#define DCMIPP_PRFCR_CERRF_Pos              (6U)
#define DCMIPP_PRFCR_CERRF_Msk              (0x1UL << DCMIPP_PRFCR_CERRF_Pos)                /*!< 0x00000040 */
#define DCMIPP_PRFCR_CERRF                  DCMIPP_PRFCR_CERRF_Msk                          /*!< Synchronization error interrupt status clear */

/*****************  Bit definition for DCMIPP_CMCR register  ******************/
#define DCMIPP_CMCR_INSEL_Pos               (0U)
#define DCMIPP_CMCR_INSEL_Msk               (0x1UL << DCMIPP_CMCR_INSEL_Pos)                 /*!< 0x00000001 */
#define DCMIPP_CMCR_INSEL                   DCMIPP_CMCR_INSEL_Msk                           /*!< input selection */
#define DCMIPP_CMCR_PSFC_Pos                (1U)
#define DCMIPP_CMCR_PSFC_Msk                (0x3UL << DCMIPP_CMCR_PSFC_Pos)                  /*!< 0x00000006 */
#define DCMIPP_CMCR_PSFC                    DCMIPP_CMCR_PSFC_Msk                            /*!< Pipe selection for the frame counter */
#define DCMIPP_CMCR_CFC_Pos                 (4U)
#define DCMIPP_CMCR_CFC_Msk                 (0x1UL << DCMIPP_CMCR_CFC_Pos)                   /*!< 0x00000010 */
#define DCMIPP_CMCR_CFC                     DCMIPP_CMCR_CFC_Msk                             /*!< Clear frame counter */
#define DCMIPP_CMCR_SWAPRB_Pos              (7U)
#define DCMIPP_CMCR_SWAPRB_Msk              (0x1UL << DCMIPP_CMCR_SWAPRB_Pos)                /*!< 0x00000080 */
#define DCMIPP_CMCR_SWAPRB                  DCMIPP_CMCR_SWAPRB_Msk                          /*!< Swap R/U and B/V */

/****************  Bit definition for DCMIPP_CMFRCR register  *****************/
#define DCMIPP_CMFRCR_FRMCNT_Pos            (0U)
#define DCMIPP_CMFRCR_FRMCNT_Msk            (0xFFFFFFFFUL << DCMIPP_CMFRCR_FRMCNT_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_CMFRCR_FRMCNT                DCMIPP_CMFRCR_FRMCNT_Msk                        /*!< Frame counter, read-only, loops around */

/*****************  Bit definition for DCMIPP_CMIER register  *****************/
#define DCMIPP_CMIER_ATXERRIE_Pos           (5U)
#define DCMIPP_CMIER_ATXERRIE_Msk           (0x1UL << DCMIPP_CMIER_ATXERRIE_Pos)             /*!< 0x00000020 */
#define DCMIPP_CMIER_ATXERRIE               DCMIPP_CMIER_ATXERRIE_Msk                       /*!< AXI Transfer error interrupt enable for IPPLUG */
#define DCMIPP_CMIER_PRERRIE_Pos            (6U)
#define DCMIPP_CMIER_PRERRIE_Msk            (0x1UL << DCMIPP_CMIER_PRERRIE_Pos)              /*!< 0x00000040 */
#define DCMIPP_CMIER_PRERRIE                DCMIPP_CMIER_PRERRIE_Msk                        /*!< limit interrupt enable for the Parallel Interface */
#define DCMIPP_CMIER_P0LINEIE_Pos           (8U)
#define DCMIPP_CMIER_P0LINEIE_Msk           (0x1UL << DCMIPP_CMIER_P0LINEIE_Pos)             /*!< 0x00000100 */
#define DCMIPP_CMIER_P0LINEIE               DCMIPP_CMIER_P0LINEIE_Msk                       /*!< multi-Line Capture complete interrupt enable for the Pipe0 */
#define DCMIPP_CMIER_P0FRAMEIE_Pos          (9U)
#define DCMIPP_CMIER_P0FRAMEIE_Msk          (0x1UL << DCMIPP_CMIER_P0FRAMEIE_Pos)            /*!< 0x00000200 */
#define DCMIPP_CMIER_P0FRAMEIE              DCMIPP_CMIER_P0FRAMEIE_Msk                      /*!< Frame Capture complete interrupt enable for the Pipe0 */
#define DCMIPP_CMIER_P0VSYNCIE_Pos          (10U)
#define DCMIPP_CMIER_P0VSYNCIE_Msk          (0x1UL << DCMIPP_CMIER_P0VSYNCIE_Pos)            /*!< 0x00000400 */
#define DCMIPP_CMIER_P0VSYNCIE              DCMIPP_CMIER_P0VSYNCIE_Msk                      /*!< Vertical sync interrupt enable for the Pipe0 */
#define DCMIPP_CMIER_P0LIMITIE_Pos          (14U)
#define DCMIPP_CMIER_P0LIMITIE_Msk          (0x1UL << DCMIPP_CMIER_P0LIMITIE_Pos)            /*!< 0x00004000 */
#define DCMIPP_CMIER_P0LIMITIE              DCMIPP_CMIER_P0LIMITIE_Msk                      /*!< limit interrupt enable for the Pipe0 */
#define DCMIPP_CMIER_P0OVRIE_Pos            (15U)
#define DCMIPP_CMIER_P0OVRIE_Msk            (0x1UL << DCMIPP_CMIER_P0OVRIE_Pos)              /*!< 0x00008000 */
#define DCMIPP_CMIER_P0OVRIE                DCMIPP_CMIER_P0OVRIE_Msk                        /*!< Overrun interrupt enable for the Pipe0 */
#define DCMIPP_CMIER_P1LINEIE_Pos           (16U)
#define DCMIPP_CMIER_P1LINEIE_Msk           (0x1UL << DCMIPP_CMIER_P1LINEIE_Pos)             /*!< 0x00010000 */
#define DCMIPP_CMIER_P1LINEIE               DCMIPP_CMIER_P1LINEIE_Msk                       /*!< multi-Line Capture complete interrupt status clear for the Pipe1 */
#define DCMIPP_CMIER_P1FRAMEIE_Pos          (17U)
#define DCMIPP_CMIER_P1FRAMEIE_Msk          (0x1UL << DCMIPP_CMIER_P1FRAMEIE_Pos)            /*!< 0x00020000 */
#define DCMIPP_CMIER_P1FRAMEIE              DCMIPP_CMIER_P1FRAMEIE_Msk                      /*!< Frame Capture complete interrupt enable for the Pipe1 */
#define DCMIPP_CMIER_P1VSYNCIE_Pos          (18U)
#define DCMIPP_CMIER_P1VSYNCIE_Msk          (0x1UL << DCMIPP_CMIER_P1VSYNCIE_Pos)            /*!< 0x00040000 */
#define DCMIPP_CMIER_P1VSYNCIE              DCMIPP_CMIER_P1VSYNCIE_Msk                      /*!< Vertical sync interrupt enable for the Pipe1 */
#define DCMIPP_CMIER_P1OVRIE_Pos            (23U)
#define DCMIPP_CMIER_P1OVRIE_Msk            (0x1UL << DCMIPP_CMIER_P1OVRIE_Pos)              /*!< 0x00800000 */
#define DCMIPP_CMIER_P1OVRIE                DCMIPP_CMIER_P1OVRIE_Msk                        /*!< Overrun interrupt enable for the Pipe1 */
#define DCMIPP_CMIER_P2LINEIE_Pos           (24U)
#define DCMIPP_CMIER_P2LINEIE_Msk           (0x1UL << DCMIPP_CMIER_P2LINEIE_Pos)             /*!< 0x01000000 */
#define DCMIPP_CMIER_P2LINEIE               DCMIPP_CMIER_P2LINEIE_Msk                       /*!< multi-Line Capture complete interrupt enable for the Pipe2 */
#define DCMIPP_CMIER_P2FRAMEIE_Pos          (25U)
#define DCMIPP_CMIER_P2FRAMEIE_Msk          (0x1UL << DCMIPP_CMIER_P2FRAMEIE_Pos)            /*!< 0x02000000 */
#define DCMIPP_CMIER_P2FRAMEIE              DCMIPP_CMIER_P2FRAMEIE_Msk                      /*!< Frame Capture complete interrupt enable for the Pipe2 */
#define DCMIPP_CMIER_P2VSYNCIE_Pos          (26U)
#define DCMIPP_CMIER_P2VSYNCIE_Msk          (0x1UL << DCMIPP_CMIER_P2VSYNCIE_Pos)            /*!< 0x04000000 */
#define DCMIPP_CMIER_P2VSYNCIE              DCMIPP_CMIER_P2VSYNCIE_Msk                      /*!< Vertical sync interrupt enable for the Pipe2 */
#define DCMIPP_CMIER_P2OVRIE_Pos            (31U)
#define DCMIPP_CMIER_P2OVRIE_Msk            (0x1UL << DCMIPP_CMIER_P2OVRIE_Pos)              /*!< 0x80000000 */
#define DCMIPP_CMIER_P2OVRIE                DCMIPP_CMIER_P2OVRIE_Msk                        /*!< Overrun interrupt status enable for the Pipe2 */

/*****************  Bit definition for DCMIPP_CMSR1 register  *****************/
#define DCMIPP_CMSR1_PRHSYNC_Pos            (0U)
#define DCMIPP_CMSR1_PRHSYNC_Msk            (0x1UL << DCMIPP_CMSR1_PRHSYNC_Pos)              /*!< 0x00000001 */
#define DCMIPP_CMSR1_PRHSYNC                DCMIPP_CMSR1_PRHSYNC_Msk                        /*!< This bit gives the state of the HSYNC pin with the correct programmed polarity on the parallel inte */
#define DCMIPP_CMSR1_PRVSYNC_Pos            (1U)
#define DCMIPP_CMSR1_PRVSYNC_Msk            (0x1UL << DCMIPP_CMSR1_PRVSYNC_Pos)              /*!< 0x00000002 */
#define DCMIPP_CMSR1_PRVSYNC                DCMIPP_CMSR1_PRVSYNC_Msk                        /*!< This bit gives the state of the VSYNC pin with the correct programmed polarity on the parallel inte */
#define DCMIPP_CMSR1_P0LSTLINE_Pos          (8U)
#define DCMIPP_CMSR1_P0LSTLINE_Msk          (0x1UL << DCMIPP_CMSR1_P0LSTLINE_Pos)            /*!< 0x00000100 */
#define DCMIPP_CMSR1_P0LSTLINE              DCMIPP_CMSR1_P0LSTLINE_Msk                      /*!< Last Line LSB bit, sampled at Frame capture complete event for Pipe0 */
#define DCMIPP_CMSR1_P0LSTFRM_Pos           (9U)
#define DCMIPP_CMSR1_P0LSTFRM_Msk           (0x1UL << DCMIPP_CMSR1_P0LSTFRM_Pos)             /*!< 0x00000200 */
#define DCMIPP_CMSR1_P0LSTFRM               DCMIPP_CMSR1_P0LSTFRM_Msk                       /*!< Last frame LSB bit, sampled at Frame capture complete event for Pipe0 */
#define DCMIPP_CMSR1_P0CPTACT_Pos           (15U)
#define DCMIPP_CMSR1_P0CPTACT_Msk           (0x1UL << DCMIPP_CMSR1_P0CPTACT_Pos)             /*!< 0x00008000 */
#define DCMIPP_CMSR1_P0CPTACT               DCMIPP_CMSR1_P0CPTACT_Msk                       /*!< Active frame capture (active from start-of-frame to frame complete) for Pipe0 */
#define DCMIPP_CMSR1_P1LSTLINE_Pos          (16U)
#define DCMIPP_CMSR1_P1LSTLINE_Msk          (0x1UL << DCMIPP_CMSR1_P1LSTLINE_Pos)            /*!< 0x00010000 */
#define DCMIPP_CMSR1_P1LSTLINE              DCMIPP_CMSR1_P1LSTLINE_Msk                      /*!< Last Line LSB bit, sampled at Frame capture complete event for Pipe1 */
#define DCMIPP_CMSR1_P1LSTFRM_Pos           (17U)
#define DCMIPP_CMSR1_P1LSTFRM_Msk           (0x1UL << DCMIPP_CMSR1_P1LSTFRM_Pos)             /*!< 0x00020000 */
#define DCMIPP_CMSR1_P1LSTFRM               DCMIPP_CMSR1_P1LSTFRM_Msk                       /*!< Last frame LSB bit, sampled at frame capture complete event for Pipe1 */
#define DCMIPP_CMSR1_P1CPTACT_Pos           (23U)
#define DCMIPP_CMSR1_P1CPTACT_Msk           (0x1UL << DCMIPP_CMSR1_P1CPTACT_Pos)             /*!< 0x00800000 */
#define DCMIPP_CMSR1_P1CPTACT               DCMIPP_CMSR1_P1CPTACT_Msk                       /*!< Active frame capture (active from start-of-frame to frame complete) for Pipe1 */
#define DCMIPP_CMSR1_P2LSTLINE_Pos          (24U)
#define DCMIPP_CMSR1_P2LSTLINE_Msk          (0x1UL << DCMIPP_CMSR1_P2LSTLINE_Pos)            /*!< 0x01000000 */
#define DCMIPP_CMSR1_P2LSTLINE              DCMIPP_CMSR1_P2LSTLINE_Msk                      /*!< Last line LSB bit, sampled at Frame capture complete event for Pipe2 */
#define DCMIPP_CMSR1_P2LSTFRM_Pos           (25U)
#define DCMIPP_CMSR1_P2LSTFRM_Msk           (0x1UL << DCMIPP_CMSR1_P2LSTFRM_Pos)             /*!< 0x02000000 */
#define DCMIPP_CMSR1_P2LSTFRM               DCMIPP_CMSR1_P2LSTFRM_Msk                       /*!< Last frame LSB bit, sampled at frame capture complete event for Pipe2 */
#define DCMIPP_CMSR1_P2CPTACT_Pos           (31U)
#define DCMIPP_CMSR1_P2CPTACT_Msk           (0x1UL << DCMIPP_CMSR1_P2CPTACT_Pos)             /*!< 0x80000000 */
#define DCMIPP_CMSR1_P2CPTACT               DCMIPP_CMSR1_P2CPTACT_Msk                       /*!< Active frame capture (active from start-of-frame to frame complete) for Pipe2 */

/*****************  Bit definition for DCMIPP_CMSR2 register  *****************/
#define DCMIPP_CMSR2_ATXERRF_Pos            (5U)
#define DCMIPP_CMSR2_ATXERRF_Msk            (0x1UL << DCMIPP_CMSR2_ATXERRF_Pos)              /*!< 0x00000020 */
#define DCMIPP_CMSR2_ATXERRF                DCMIPP_CMSR2_ATXERRF_Msk                        /*!< AXI transfer error interrupt status flag for the IPPLUG */
#define DCMIPP_CMSR2_PRERRF_Pos             (6U)
#define DCMIPP_CMSR2_PRERRF_Msk             (0x1UL << DCMIPP_CMSR2_PRERRF_Pos)               /*!< 0x00000040 */
#define DCMIPP_CMSR2_PRERRF                 DCMIPP_CMSR2_PRERRF_Msk                         /*!< Synchronization error raw interrupt status for the parallel interface */
#define DCMIPP_CMSR2_P0LINEF_Pos            (8U)
#define DCMIPP_CMSR2_P0LINEF_Msk            (0x1UL << DCMIPP_CMSR2_P0LINEF_Pos)              /*!< 0x00000100 */
#define DCMIPP_CMSR2_P0LINEF                DCMIPP_CMSR2_P0LINEF_Msk                        /*!< Multi-line capture completed raw interrupt status for Pipe0 */
#define DCMIPP_CMSR2_P0FRAMEF_Pos           (9U)
#define DCMIPP_CMSR2_P0FRAMEF_Msk           (0x1UL << DCMIPP_CMSR2_P0FRAMEF_Pos)             /*!< 0x00000200 */
#define DCMIPP_CMSR2_P0FRAMEF               DCMIPP_CMSR2_P0FRAMEF_Msk                       /*!< Frame capture completed raw interrupt status for Pipe0 */
#define DCMIPP_CMSR2_P0VSYNCF_Pos           (10U)
#define DCMIPP_CMSR2_P0VSYNCF_Msk           (0x1UL << DCMIPP_CMSR2_P0VSYNCF_Pos)             /*!< 0x00000400 */
#define DCMIPP_CMSR2_P0VSYNCF               DCMIPP_CMSR2_P0VSYNCF_Msk                       /*!< VSYNC raw interrupt status for Pipe0 */
#define DCMIPP_CMSR2_P0LIMITF_Pos           (14U)
#define DCMIPP_CMSR2_P0LIMITF_Msk           (0x1UL << DCMIPP_CMSR2_P0LIMITF_Pos)             /*!< 0x00004000 */
#define DCMIPP_CMSR2_P0LIMITF               DCMIPP_CMSR2_P0LIMITF_Msk                       /*!< Limit raw interrupt status for Pipe0 */
#define DCMIPP_CMSR2_P0OVRF_Pos             (15U)
#define DCMIPP_CMSR2_P0OVRF_Msk             (0x1UL << DCMIPP_CMSR2_P0OVRF_Pos)               /*!< 0x00008000 */
#define DCMIPP_CMSR2_P0OVRF                 DCMIPP_CMSR2_P0OVRF_Msk                         /*!< Overrun raw interrupt status for Pipe0 */
#define DCMIPP_CMSR2_P1LINEF_Pos            (16U)
#define DCMIPP_CMSR2_P1LINEF_Msk            (0x1UL << DCMIPP_CMSR2_P1LINEF_Pos)              /*!< 0x00010000 */
#define DCMIPP_CMSR2_P1LINEF                DCMIPP_CMSR2_P1LINEF_Msk                        /*!< Multi-line capture completed raw interrupt status for Pipe1 */
#define DCMIPP_CMSR2_P1FRAMEF_Pos           (17U)
#define DCMIPP_CMSR2_P1FRAMEF_Msk           (0x1UL << DCMIPP_CMSR2_P1FRAMEF_Pos)             /*!< 0x00020000 */
#define DCMIPP_CMSR2_P1FRAMEF               DCMIPP_CMSR2_P1FRAMEF_Msk                       /*!< Frame capture completed raw interrupt status for Pipe1 */
#define DCMIPP_CMSR2_P1VSYNCF_Pos           (18U)
#define DCMIPP_CMSR2_P1VSYNCF_Msk           (0x1UL << DCMIPP_CMSR2_P1VSYNCF_Pos)             /*!< 0x00040000 */
#define DCMIPP_CMSR2_P1VSYNCF               DCMIPP_CMSR2_P1VSYNCF_Msk                       /*!< VSYNC raw interrupt status for Pipe1 */
#define DCMIPP_CMSR2_P1OVRF_Pos             (23U)
#define DCMIPP_CMSR2_P1OVRF_Msk             (0x1UL << DCMIPP_CMSR2_P1OVRF_Pos)               /*!< 0x00800000 */
#define DCMIPP_CMSR2_P1OVRF                 DCMIPP_CMSR2_P1OVRF_Msk                         /*!< Overrun raw interrupt status for Pipe1 */
#define DCMIPP_CMSR2_P2LINEF_Pos            (24U)
#define DCMIPP_CMSR2_P2LINEF_Msk            (0x1UL << DCMIPP_CMSR2_P2LINEF_Pos)              /*!< 0x01000000 */
#define DCMIPP_CMSR2_P2LINEF                DCMIPP_CMSR2_P2LINEF_Msk                        /*!< Multi-line capture completed raw interrupt status for Pipe2 */
#define DCMIPP_CMSR2_P2FRAMEF_Pos           (25U)
#define DCMIPP_CMSR2_P2FRAMEF_Msk           (0x1UL << DCMIPP_CMSR2_P2FRAMEF_Pos)             /*!< 0x02000000 */
#define DCMIPP_CMSR2_P2FRAMEF               DCMIPP_CMSR2_P2FRAMEF_Msk                       /*!< Frame capture completed raw interrupt status for Pipe2 */
#define DCMIPP_CMSR2_P2VSYNCF_Pos           (26U)
#define DCMIPP_CMSR2_P2VSYNCF_Msk           (0x1UL << DCMIPP_CMSR2_P2VSYNCF_Pos)             /*!< 0x04000000 */
#define DCMIPP_CMSR2_P2VSYNCF               DCMIPP_CMSR2_P2VSYNCF_Msk                       /*!< VSYNC raw interrupt status for Pipe2 */
#define DCMIPP_CMSR2_P2OVRF_Pos             (31U)
#define DCMIPP_CMSR2_P2OVRF_Msk             (0x1UL << DCMIPP_CMSR2_P2OVRF_Pos)               /*!< 0x80000000 */
#define DCMIPP_CMSR2_P2OVRF                 DCMIPP_CMSR2_P2OVRF_Msk                         /*!< Overrun raw interrupt status for Pipe2 */

/*****************  Bit definition for DCMIPP_CMFCR register  *****************/
#define DCMIPP_CMFCR_CATXERRF_Pos           (5U)
#define DCMIPP_CMFCR_CATXERRF_Msk           (0x1UL << DCMIPP_CMFCR_CATXERRF_Pos)             /*!< 0x00000020 */
#define DCMIPP_CMFCR_CATXERRF               DCMIPP_CMFCR_CATXERRF_Msk                       /*!< AXI Transfer error interrupt status clear */
#define DCMIPP_CMFCR_CPRERRF_Pos            (6U)
#define DCMIPP_CMFCR_CPRERRF_Msk            (0x1UL << DCMIPP_CMFCR_CPRERRF_Pos)              /*!< 0x00000040 */
#define DCMIPP_CMFCR_CPRERRF                DCMIPP_CMFCR_CPRERRF_Msk                        /*!< Synchronization error interrupt status clear */
#define DCMIPP_CMFCR_CP0LINEF_Pos           (8U)
#define DCMIPP_CMFCR_CP0LINEF_Msk           (0x1UL << DCMIPP_CMFCR_CP0LINEF_Pos)             /*!< 0x00000100 */
#define DCMIPP_CMFCR_CP0LINEF               DCMIPP_CMFCR_CP0LINEF_Msk                       /*!< Multi-line capture complete interrupt status clear */
#define DCMIPP_CMFCR_CP0FRAMEF_Pos          (9U)
#define DCMIPP_CMFCR_CP0FRAMEF_Msk          (0x1UL << DCMIPP_CMFCR_CP0FRAMEF_Pos)            /*!< 0x00000200 */
#define DCMIPP_CMFCR_CP0FRAMEF              DCMIPP_CMFCR_CP0FRAMEF_Msk                      /*!< Frame capture complete interrupt status clear */
#define DCMIPP_CMFCR_CP0VSYNCF_Pos          (10U)
#define DCMIPP_CMFCR_CP0VSYNCF_Msk          (0x1UL << DCMIPP_CMFCR_CP0VSYNCF_Pos)            /*!< 0x00000400 */
#define DCMIPP_CMFCR_CP0VSYNCF              DCMIPP_CMFCR_CP0VSYNCF_Msk                      /*!< Vertical synchronization interrupt status clear */
#define DCMIPP_CMFCR_CP0LIMITF_Pos          (14U)
#define DCMIPP_CMFCR_CP0LIMITF_Msk          (0x1UL << DCMIPP_CMFCR_CP0LIMITF_Pos)            /*!< 0x00004000 */
#define DCMIPP_CMFCR_CP0LIMITF              DCMIPP_CMFCR_CP0LIMITF_Msk                      /*!< limit interrupt status clear */
#define DCMIPP_CMFCR_CP0OVRF_Pos            (15U)
#define DCMIPP_CMFCR_CP0OVRF_Msk            (0x1UL << DCMIPP_CMFCR_CP0OVRF_Pos)              /*!< 0x00008000 */
#define DCMIPP_CMFCR_CP0OVRF                DCMIPP_CMFCR_CP0OVRF_Msk                        /*!< Overrun interrupt status clear */
#define DCMIPP_CMFCR_CP1LINEF_Pos           (16U)
#define DCMIPP_CMFCR_CP1LINEF_Msk           (0x1UL << DCMIPP_CMFCR_CP1LINEF_Pos)             /*!< 0x00010000 */
#define DCMIPP_CMFCR_CP1LINEF               DCMIPP_CMFCR_CP1LINEF_Msk                       /*!< Multi-line capture complete interrupt status clear */
#define DCMIPP_CMFCR_CP1FRAMEF_Pos          (17U)
#define DCMIPP_CMFCR_CP1FRAMEF_Msk          (0x1UL << DCMIPP_CMFCR_CP1FRAMEF_Pos)            /*!< 0x00020000 */
#define DCMIPP_CMFCR_CP1FRAMEF              DCMIPP_CMFCR_CP1FRAMEF_Msk                      /*!< Frame capture complete interrupt status clear */
#define DCMIPP_CMFCR_CP1VSYNCF_Pos          (18U)
#define DCMIPP_CMFCR_CP1VSYNCF_Msk          (0x1UL << DCMIPP_CMFCR_CP1VSYNCF_Pos)            /*!< 0x00040000 */
#define DCMIPP_CMFCR_CP1VSYNCF              DCMIPP_CMFCR_CP1VSYNCF_Msk                      /*!< Vertical synchronization interrupt status clear */
#define DCMIPP_CMFCR_CP1OVRF_Pos            (23U)
#define DCMIPP_CMFCR_CP1OVRF_Msk            (0x1UL << DCMIPP_CMFCR_CP1OVRF_Pos)              /*!< 0x00800000 */
#define DCMIPP_CMFCR_CP1OVRF                DCMIPP_CMFCR_CP1OVRF_Msk                        /*!< Overrun interrupt status clear */
#define DCMIPP_CMFCR_CP2LINEF_Pos           (24U)
#define DCMIPP_CMFCR_CP2LINEF_Msk           (0x1UL << DCMIPP_CMFCR_CP2LINEF_Pos)             /*!< 0x01000000 */
#define DCMIPP_CMFCR_CP2LINEF               DCMIPP_CMFCR_CP2LINEF_Msk                       /*!< Multi-line capture complete interrupt status clear */
#define DCMIPP_CMFCR_CP2FRAMEF_Pos          (25U)
#define DCMIPP_CMFCR_CP2FRAMEF_Msk          (0x1UL << DCMIPP_CMFCR_CP2FRAMEF_Pos)            /*!< 0x02000000 */
#define DCMIPP_CMFCR_CP2FRAMEF              DCMIPP_CMFCR_CP2FRAMEF_Msk                      /*!< Frame capture complete interrupt status clear */
#define DCMIPP_CMFCR_CP2VSYNCF_Pos          (26U)
#define DCMIPP_CMFCR_CP2VSYNCF_Msk          (0x1UL << DCMIPP_CMFCR_CP2VSYNCF_Pos)            /*!< 0x04000000 */
#define DCMIPP_CMFCR_CP2VSYNCF              DCMIPP_CMFCR_CP2VSYNCF_Msk                      /*!< Vertical synchronization interrupt status clear */
#define DCMIPP_CMFCR_CP2OVRF_Pos            (31U)
#define DCMIPP_CMFCR_CP2OVRF_Msk            (0x1UL << DCMIPP_CMFCR_CP2OVRF_Pos)              /*!< 0x80000000 */
#define DCMIPP_CMFCR_CP2OVRF                DCMIPP_CMFCR_CP2OVRF_Msk                        /*!< Overrun interrupt status clear */

/****************  Bit definition for DCMIPP_P0FSCR register  *****************/
#define DCMIPP_P0FSCR_DTIDA_Pos             (0U)
#define DCMIPP_P0FSCR_DTIDA_Msk             (0x3FUL << DCMIPP_P0FSCR_DTIDA_Pos)              /*!< 0x0000003F */
#define DCMIPP_P0FSCR_DTIDA                 DCMIPP_P0FSCR_DTIDA_Msk                         /*!< Data type selection ID A */
#define DCMIPP_P0FSCR_DTIDB_Pos             (8U)
#define DCMIPP_P0FSCR_DTIDB_Msk             (0x3FUL << DCMIPP_P0FSCR_DTIDB_Pos)              /*!< 0x00003F00 */
#define DCMIPP_P0FSCR_DTIDB                 DCMIPP_P0FSCR_DTIDB_Msk                         /*!< Data type selection ID B */
#define DCMIPP_P0FSCR_DTMODE_Pos            (16U)
#define DCMIPP_P0FSCR_DTMODE_Msk            (0x3UL << DCMIPP_P0FSCR_DTMODE_Pos)              /*!< 0x00030000 */
#define DCMIPP_P0FSCR_DTMODE                DCMIPP_P0FSCR_DTMODE_Msk                        /*!< Flow selection mode */
#define DCMIPP_P0FSCR_VC_Pos                (19U)
#define DCMIPP_P0FSCR_VC_Msk                (0x3UL << DCMIPP_P0FSCR_VC_Pos)                  /*!< 0x00180000 */
#define DCMIPP_P0FSCR_VC                    DCMIPP_P0FSCR_VC_Msk                            /*!< Flow selection mode */
#define DCMIPP_P0FSCR_PIPEN_Pos             (31U)
#define DCMIPP_P0FSCR_PIPEN_Msk             (0x1UL << DCMIPP_P0FSCR_PIPEN_Pos)               /*!< 0x80000000 */
#define DCMIPP_P0FSCR_PIPEN                 DCMIPP_P0FSCR_PIPEN_Msk                         /*!< Activation of PipeN */

/****************  Bit definition for DCMIPP_P0FCTCR register  ****************/
#define DCMIPP_P0FCTCR_FRATE_Pos            (0U)
#define DCMIPP_P0FCTCR_FRATE_Msk            (0x3UL << DCMIPP_P0FCTCR_FRATE_Pos)              /*!< 0x00000003 */
#define DCMIPP_P0FCTCR_FRATE                DCMIPP_P0FCTCR_FRATE_Msk                        /*!< Frame capture rate control */
#define DCMIPP_P0FCTCR_CPTMODE_Pos          (2U)
#define DCMIPP_P0FCTCR_CPTMODE_Msk          (0x1UL << DCMIPP_P0FCTCR_CPTMODE_Pos)            /*!< 0x00000004 */
#define DCMIPP_P0FCTCR_CPTMODE              DCMIPP_P0FCTCR_CPTMODE_Msk                      /*!< Capture mode */
#define DCMIPP_P0FCTCR_CPTREQ_Pos           (3U)
#define DCMIPP_P0FCTCR_CPTREQ_Msk           (0x1UL << DCMIPP_P0FCTCR_CPTREQ_Pos)             /*!< 0x00000008 */
#define DCMIPP_P0FCTCR_CPTREQ               DCMIPP_P0FCTCR_CPTREQ_Msk                       /*!< Capture requested */

/****************  Bit definition for DCMIPP_P0SCSTR register  ****************/
#define DCMIPP_P0SCSTR_HSTART_Pos           (0U)
#define DCMIPP_P0SCSTR_HSTART_Msk           (0xFFFUL << DCMIPP_P0SCSTR_HSTART_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P0SCSTR_HSTART               DCMIPP_P0SCSTR_HSTART_Msk                       /*!< Horizontal start, from 0 to 4094 words wide */
#define DCMIPP_P0SCSTR_VSTART_Pos           (16U)
#define DCMIPP_P0SCSTR_VSTART_Msk           (0xFFFUL << DCMIPP_P0SCSTR_VSTART_Pos)           /*!< 0x0FFF0000 */
#define DCMIPP_P0SCSTR_VSTART               DCMIPP_P0SCSTR_VSTART_Msk                       /*!< Vertical start, from 0 to 4094 pixels high */

/****************  Bit definition for DCMIPP_P0SCSZR register  ****************/
#define DCMIPP_P0SCSZR_HSIZE_Pos            (0U)
#define DCMIPP_P0SCSZR_HSIZE_Msk            (0xFFFUL << DCMIPP_P0SCSZR_HSIZE_Pos)            /*!< 0x00000FFF */
#define DCMIPP_P0SCSZR_HSIZE                DCMIPP_P0SCSZR_HSIZE_Msk                        /*!< Horizontal size, from 0 to 4094 word wide (data 32-bit) */
#define DCMIPP_P0SCSZR_VSIZE_Pos            (16U)
#define DCMIPP_P0SCSZR_VSIZE_Msk            (0xFFFUL << DCMIPP_P0SCSZR_VSIZE_Pos)            /*!< 0x0FFF0000 */
#define DCMIPP_P0SCSZR_VSIZE                DCMIPP_P0SCSZR_VSIZE_Msk                        /*!< Vertical size, from 0 to 4094 pixels high */
#define DCMIPP_P0SCSZR_POSNEG_Pos           (30U)
#define DCMIPP_P0SCSZR_POSNEG_Msk           (0x1UL << DCMIPP_P0SCSZR_POSNEG_Pos)             /*!< 0x40000000 */
#define DCMIPP_P0SCSZR_POSNEG               DCMIPP_P0SCSZR_POSNEG_Msk                       /*!< This bit is set and cleared by software */
#define DCMIPP_P0SCSZR_ENABLE_Pos           (31U)
#define DCMIPP_P0SCSZR_ENABLE_Msk           (0x1UL << DCMIPP_P0SCSZR_ENABLE_Pos)             /*!< 0x80000000 */
#define DCMIPP_P0SCSZR_ENABLE               DCMIPP_P0SCSZR_ENABLE_Msk                       /*!< This bit is set and cleared by software */

/***************  Bit definition for DCMIPP_P0DCCNTR register  ****************/
#define DCMIPP_P0DCCNTR_CNT_Pos             (0U)
#define DCMIPP_P0DCCNTR_CNT_Msk             (0x3FFFFFFUL << DCMIPP_P0DCCNTR_CNT_Pos)         /*!< 0x03FFFFFF */
#define DCMIPP_P0DCCNTR_CNT                 DCMIPP_P0DCCNTR_CNT_Msk                         /*!< Number of data dumped during the frame */

/***************  Bit definition for DCMIPP_P0DCLMTR register  ****************/
#define DCMIPP_P0DCLMTR_LIMIT_Pos           (0U)
#define DCMIPP_P0DCLMTR_LIMIT_Msk           (0xFFFFFFUL << DCMIPP_P0DCLMTR_LIMIT_Pos)        /*!< 0x00FFFFFF */
#define DCMIPP_P0DCLMTR_LIMIT               DCMIPP_P0DCLMTR_LIMIT_Msk                       /*!< Maximum number of 32-bit data that can be dumped during a frame, after the crop 2D operation */
#define DCMIPP_P0DCLMTR_ENABLE_Pos          (31U)
#define DCMIPP_P0DCLMTR_ENABLE_Msk          (0x1UL << DCMIPP_P0DCLMTR_ENABLE_Pos)            /*!< 0x80000000 */
#define DCMIPP_P0DCLMTR_ENABLE              DCMIPP_P0DCLMTR_ENABLE_Msk                      /*!<  */

/****************  Bit definition for DCMIPP_P0PPCR register  *****************/
#define DCMIPP_P0PPCR_SWAPYUV_Pos           (0U)
#define DCMIPP_P0PPCR_SWAPYUV_Msk           (0x1UL << DCMIPP_P0PPCR_SWAPYUV_Pos)             /*!< 0x00000001 */
#define DCMIPP_P0PPCR_SWAPYUV               DCMIPP_P0PPCR_SWAPYUV_Msk                       /*!< SwapY vs UV bits, when the YUV mode is active */
#define DCMIPP_P0PPCR_PAD_Pos               (5U)
#define DCMIPP_P0PPCR_PAD_Msk               (0x1UL << DCMIPP_P0PPCR_PAD_Pos)                 /*!< 0x00000020 */
#define DCMIPP_P0PPCR_PAD                   DCMIPP_P0PPCR_PAD_Msk                           /*!< Pad mode for monochrome and raw Bayer 10/12/14 bpp: MSB vs */
#define DCMIPP_P0PPCR_HEADEREN_Pos          (6U)
#define DCMIPP_P0PPCR_HEADEREN_Msk          (0x1UL << DCMIPP_P0PPCR_HEADEREN_Pos)            /*!< 0x00000040 */
#define DCMIPP_P0PPCR_HEADEREN              DCMIPP_P0PPCR_HEADEREN_Msk                      /*!< CSI header dump enable */
#define DCMIPP_P0PPCR_BSM_Pos               (7U)
#define DCMIPP_P0PPCR_BSM_Msk               (0x3UL << DCMIPP_P0PPCR_BSM_Pos)                 /*!< 0x00000180 */
#define DCMIPP_P0PPCR_BSM                   DCMIPP_P0PPCR_BSM_Msk                           /*!< Byte select mode */
#define DCMIPP_P0PPCR_OEBS_Pos              (9U)
#define DCMIPP_P0PPCR_OEBS_Msk              (0x1UL << DCMIPP_P0PPCR_OEBS_Pos)                /*!< 0x00000200 */
#define DCMIPP_P0PPCR_OEBS                  DCMIPP_P0PPCR_OEBS_Msk                          /*!< Odd/even byte select (byte select start) */
#define DCMIPP_P0PPCR_LSM_Pos               (10U)
#define DCMIPP_P0PPCR_LSM_Msk               (0x1UL << DCMIPP_P0PPCR_LSM_Pos)                 /*!< 0x00000400 */
#define DCMIPP_P0PPCR_LSM                   DCMIPP_P0PPCR_LSM_Msk                           /*!< Line select mode */
#define DCMIPP_P0PPCR_OELS_Pos              (11U)
#define DCMIPP_P0PPCR_OELS_Msk              (0x1UL << DCMIPP_P0PPCR_OELS_Pos)                /*!< 0x00000800 */
#define DCMIPP_P0PPCR_OELS                  DCMIPP_P0PPCR_OELS_Msk                          /*!< Odd/even line select (line select start) */
#define DCMIPP_P0PPCR_LINEMULT_Pos          (13U)
#define DCMIPP_P0PPCR_LINEMULT_Msk          (0x7UL << DCMIPP_P0PPCR_LINEMULT_Pos)            /*!< 0x0000E000 */
#define DCMIPP_P0PPCR_LINEMULT              DCMIPP_P0PPCR_LINEMULT_Msk                      /*!< Amount of capture completed lines for LINE Event and Interrupt */
#define DCMIPP_P0PPCR_DBM_Pos               (16U)
#define DCMIPP_P0PPCR_DBM_Msk               (0x1UL << DCMIPP_P0PPCR_DBM_Pos)                 /*!< 0x00010000 */
#define DCMIPP_P0PPCR_DBM                   DCMIPP_P0PPCR_DBM_Msk                           /*!< Double buffer mode */

/***************  Bit definition for DCMIPP_P0PPM0AR1 register  ***************/
#define DCMIPP_P0PPM0AR1_M0A_Pos            (0U)
#define DCMIPP_P0PPM0AR1_M0A_Msk            (0xFFFFFFFFUL << DCMIPP_P0PPM0AR1_M0A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P0PPM0AR1_M0A                DCMIPP_P0PPM0AR1_M0A_Msk                        /*!< Memory0 address */

/***************  Bit definition for DCMIPP_P0PPM0AR2 register  ***************/
#define DCMIPP_P0PPM0AR2_M0A_Pos            (0U)
#define DCMIPP_P0PPM0AR2_M0A_Msk            (0xFFFFFFFFUL << DCMIPP_P0PPM0AR2_M0A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P0PPM0AR2_M0A                DCMIPP_P0PPM0AR2_M0A_Msk                        /*!< Memory0 address */

/*****************  Bit definition for DCMIPP_P0IER register  *****************/
#define DCMIPP_P0IER_LINEIE_Pos             (0U)
#define DCMIPP_P0IER_LINEIE_Msk             (0x1UL << DCMIPP_P0IER_LINEIE_Pos)               /*!< 0x00000001 */
#define DCMIPP_P0IER_LINEIE                 DCMIPP_P0IER_LINEIE_Msk                         /*!< Multi-line capture completed interrupt enable */
#define DCMIPP_P0IER_FRAMEIE_Pos            (1U)
#define DCMIPP_P0IER_FRAMEIE_Msk            (0x1UL << DCMIPP_P0IER_FRAMEIE_Pos)              /*!< 0x00000002 */
#define DCMIPP_P0IER_FRAMEIE                DCMIPP_P0IER_FRAMEIE_Msk                        /*!< Frame capture completed interrupt enable */
#define DCMIPP_P0IER_VSYNCIE_Pos            (2U)
#define DCMIPP_P0IER_VSYNCIE_Msk            (0x1UL << DCMIPP_P0IER_VSYNCIE_Pos)              /*!< 0x00000004 */
#define DCMIPP_P0IER_VSYNCIE                DCMIPP_P0IER_VSYNCIE_Msk                        /*!< VSYNC interrupt enable */
#define DCMIPP_P0IER_LIMITIE_Pos            (6U)
#define DCMIPP_P0IER_LIMITIE_Msk            (0x1UL << DCMIPP_P0IER_LIMITIE_Pos)              /*!< 0x00000040 */
#define DCMIPP_P0IER_LIMITIE                DCMIPP_P0IER_LIMITIE_Msk                        /*!< Limit interrupt enable */
#define DCMIPP_P0IER_OVRIE_Pos              (7U)
#define DCMIPP_P0IER_OVRIE_Msk              (0x1UL << DCMIPP_P0IER_OVRIE_Pos)                /*!< 0x00000080 */
#define DCMIPP_P0IER_OVRIE                  DCMIPP_P0IER_OVRIE_Msk                          /*!< Overrun interrupt enable */

/*****************  Bit definition for DCMIPP_P0SR register  ******************/
#define DCMIPP_P0SR_LINEF_Pos               (0U)
#define DCMIPP_P0SR_LINEF_Msk               (0x1UL << DCMIPP_P0SR_LINEF_Pos)                 /*!< 0x00000001 */
#define DCMIPP_P0SR_LINEF                   DCMIPP_P0SR_LINEF_Msk                           /*!< Multi-line capture completed raw interrupt status */
#define DCMIPP_P0SR_FRAMEF_Pos              (1U)
#define DCMIPP_P0SR_FRAMEF_Msk              (0x1UL << DCMIPP_P0SR_FRAMEF_Pos)                /*!< 0x00000002 */
#define DCMIPP_P0SR_FRAMEF                  DCMIPP_P0SR_FRAMEF_Msk                          /*!< Frame capture completed raw interrupt status */
#define DCMIPP_P0SR_VSYNCF_Pos              (2U)
#define DCMIPP_P0SR_VSYNCF_Msk              (0x1UL << DCMIPP_P0SR_VSYNCF_Pos)                /*!< 0x00000004 */
#define DCMIPP_P0SR_VSYNCF                  DCMIPP_P0SR_VSYNCF_Msk                          /*!< VSYNC raw interrupt status */
#define DCMIPP_P0SR_LIMITF_Pos              (6U)
#define DCMIPP_P0SR_LIMITF_Msk              (0x1UL << DCMIPP_P0SR_LIMITF_Pos)                /*!< 0x00000040 */
#define DCMIPP_P0SR_LIMITF                  DCMIPP_P0SR_LIMITF_Msk                          /*!< Limit raw interrupt status */
#define DCMIPP_P0SR_OVRF_Pos                (7U)
#define DCMIPP_P0SR_OVRF_Msk                (0x1UL << DCMIPP_P0SR_OVRF_Pos)                  /*!< 0x00000080 */
#define DCMIPP_P0SR_OVRF                    DCMIPP_P0SR_OVRF_Msk                            /*!< Overrun raw interrupt status */
#define DCMIPP_P0SR_LSTLINE_Pos             (16U)
#define DCMIPP_P0SR_LSTLINE_Msk             (0x1UL << DCMIPP_P0SR_LSTLINE_Pos)               /*!< 0x00010000 */
#define DCMIPP_P0SR_LSTLINE                 DCMIPP_P0SR_LSTLINE_Msk                         /*!< Last line LSB bit, sampled at frame capture complete event */
#define DCMIPP_P0SR_LSTFRM_Pos              (17U)
#define DCMIPP_P0SR_LSTFRM_Msk              (0x1UL << DCMIPP_P0SR_LSTFRM_Pos)                /*!< 0x00020000 */
#define DCMIPP_P0SR_LSTFRM                  DCMIPP_P0SR_LSTFRM_Msk                          /*!< Last frame LSB bit, sampled at frame capture complete event */
#define DCMIPP_P0SR_CPTACT_Pos              (23U)
#define DCMIPP_P0SR_CPTACT_Msk              (0x1UL << DCMIPP_P0SR_CPTACT_Pos)                /*!< 0x00800000 */
#define DCMIPP_P0SR_CPTACT                  DCMIPP_P0SR_CPTACT_Msk                          /*!< Capture immediate status */

/*****************  Bit definition for DCMIPP_P0FCR register  *****************/
#define DCMIPP_P0FCR_CLINEF_Pos             (0U)
#define DCMIPP_P0FCR_CLINEF_Msk             (0x1UL << DCMIPP_P0FCR_CLINEF_Pos)               /*!< 0x00000001 */
#define DCMIPP_P0FCR_CLINEF                 DCMIPP_P0FCR_CLINEF_Msk                         /*!< Multi-line capture complete interrupt status clear */
#define DCMIPP_P0FCR_CFRAMEF_Pos            (1U)
#define DCMIPP_P0FCR_CFRAMEF_Msk            (0x1UL << DCMIPP_P0FCR_CFRAMEF_Pos)              /*!< 0x00000002 */
#define DCMIPP_P0FCR_CFRAMEF                DCMIPP_P0FCR_CFRAMEF_Msk                        /*!< Frame capture complete interrupt status clear */
#define DCMIPP_P0FCR_CVSYNCF_Pos            (2U)
#define DCMIPP_P0FCR_CVSYNCF_Msk            (0x1UL << DCMIPP_P0FCR_CVSYNCF_Pos)              /*!< 0x00000004 */
#define DCMIPP_P0FCR_CVSYNCF                DCMIPP_P0FCR_CVSYNCF_Msk                        /*!< Vertical synchronization interrupt status clear */
#define DCMIPP_P0FCR_CLIMITF_Pos            (6U)
#define DCMIPP_P0FCR_CLIMITF_Msk            (0x1UL << DCMIPP_P0FCR_CLIMITF_Pos)              /*!< 0x00000040 */
#define DCMIPP_P0FCR_CLIMITF                DCMIPP_P0FCR_CLIMITF_Msk                        /*!< limit interrupt status clear */
#define DCMIPP_P0FCR_COVRF_Pos              (7U)
#define DCMIPP_P0FCR_COVRF_Msk              (0x1UL << DCMIPP_P0FCR_COVRF_Pos)                /*!< 0x00000080 */
#define DCMIPP_P0FCR_COVRF                  DCMIPP_P0FCR_COVRF_Msk                          /*!< Overrun interrupt status clear */

/****************  Bit definition for DCMIPP_P0CFSCR register  ****************/
#define DCMIPP_P0CFSCR_DTIDA_Pos            (0U)
#define DCMIPP_P0CFSCR_DTIDA_Msk            (0x3FUL << DCMIPP_P0CFSCR_DTIDA_Pos)             /*!< 0x0000003F */
#define DCMIPP_P0CFSCR_DTIDA                DCMIPP_P0CFSCR_DTIDA_Msk                        /*!< Current Data type selection ID A */
#define DCMIPP_P0CFSCR_DTIDB_Pos            (8U)
#define DCMIPP_P0CFSCR_DTIDB_Msk            (0x3FUL << DCMIPP_P0CFSCR_DTIDB_Pos)             /*!< 0x00003F00 */
#define DCMIPP_P0CFSCR_DTIDB                DCMIPP_P0CFSCR_DTIDB_Msk                        /*!< Current Data type selection ID B */
#define DCMIPP_P0CFSCR_DTMODE_Pos           (16U)
#define DCMIPP_P0CFSCR_DTMODE_Msk           (0x3UL << DCMIPP_P0CFSCR_DTMODE_Pos)             /*!< 0x00030000 */
#define DCMIPP_P0CFSCR_DTMODE               DCMIPP_P0CFSCR_DTMODE_Msk                       /*!< Flow selection mode */
#define DCMIPP_P0CFSCR_VC_Pos               (19U)
#define DCMIPP_P0CFSCR_VC_Msk               (0x3UL << DCMIPP_P0CFSCR_VC_Pos)                 /*!< 0x00180000 */
#define DCMIPP_P0CFSCR_VC                   DCMIPP_P0CFSCR_VC_Msk                           /*!< Current flow selection mode */
#define DCMIPP_P0CFSCR_PIPEN_Pos            (31U)
#define DCMIPP_P0CFSCR_PIPEN_Msk            (0x1UL << DCMIPP_P0CFSCR_PIPEN_Pos)              /*!< 0x80000000 */
#define DCMIPP_P0CFSCR_PIPEN                DCMIPP_P0CFSCR_PIPEN_Msk                        /*!< Current activation of PipeN */

/***************  Bit definition for DCMIPP_P0CFCTCR register  ****************/
#define DCMIPP_P0CFCTCR_FRATE_Pos           (0U)
#define DCMIPP_P0CFCTCR_FRATE_Msk           (0x3UL << DCMIPP_P0CFCTCR_FRATE_Pos)             /*!< 0x00000003 */
#define DCMIPP_P0CFCTCR_FRATE               DCMIPP_P0CFCTCR_FRATE_Msk                       /*!< Frame capture rate control */
#define DCMIPP_P0CFCTCR_CPTMODE_Pos         (2U)
#define DCMIPP_P0CFCTCR_CPTMODE_Msk         (0x1UL << DCMIPP_P0CFCTCR_CPTMODE_Pos)           /*!< 0x00000004 */
#define DCMIPP_P0CFCTCR_CPTMODE             DCMIPP_P0CFCTCR_CPTMODE_Msk                     /*!< Capture mode */
#define DCMIPP_P0CFCTCR_CPTREQ_Pos          (3U)
#define DCMIPP_P0CFCTCR_CPTREQ_Msk          (0x1UL << DCMIPP_P0CFCTCR_CPTREQ_Pos)            /*!< 0x00000008 */
#define DCMIPP_P0CFCTCR_CPTREQ              DCMIPP_P0CFCTCR_CPTREQ_Msk                      /*!< Capture requested */

/***************  Bit definition for DCMIPP_P0CSCSTR register  ****************/
#define DCMIPP_P0CSCSTR_HSTART_Pos          (0U)
#define DCMIPP_P0CSCSTR_HSTART_Msk          (0xFFFUL << DCMIPP_P0CSCSTR_HSTART_Pos)          /*!< 0x00000FFF */
#define DCMIPP_P0CSCSTR_HSTART              DCMIPP_P0CSCSTR_HSTART_Msk                      /*!< Current horizontal start, from 0 to 4094 words wide */
#define DCMIPP_P0CSCSTR_VSTART_Pos          (16U)
#define DCMIPP_P0CSCSTR_VSTART_Msk          (0xFFFUL << DCMIPP_P0CSCSTR_VSTART_Pos)          /*!< 0x0FFF0000 */
#define DCMIPP_P0CSCSTR_VSTART              DCMIPP_P0CSCSTR_VSTART_Msk                      /*!< Current vertical start, from 0 to 4094 pixels high */

/***************  Bit definition for DCMIPP_P0CSCSZR register  ****************/
#define DCMIPP_P0CSCSZR_HSIZE_Pos           (0U)
#define DCMIPP_P0CSCSZR_HSIZE_Msk           (0xFFFUL << DCMIPP_P0CSCSZR_HSIZE_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P0CSCSZR_HSIZE               DCMIPP_P0CSCSZR_HSIZE_Msk                       /*!< Current horizontal size, from 0 to 4094 word wide (data 32-bit) */
#define DCMIPP_P0CSCSZR_VSIZE_Pos           (16U)
#define DCMIPP_P0CSCSZR_VSIZE_Msk           (0xFFFUL << DCMIPP_P0CSCSZR_VSIZE_Pos)           /*!< 0x0FFF0000 */
#define DCMIPP_P0CSCSZR_VSIZE               DCMIPP_P0CSCSZR_VSIZE_Msk                       /*!< Current vertical size, from 0 to 4094 pixels high */
#define DCMIPP_P0CSCSZR_POSNEG_Pos          (30U)
#define DCMIPP_P0CSCSZR_POSNEG_Msk          (0x1UL << DCMIPP_P0CSCSZR_POSNEG_Pos)            /*!< 0x40000000 */
#define DCMIPP_P0CSCSZR_POSNEG              DCMIPP_P0CSCSZR_POSNEG_Msk                      /*!< Current value of the POSNEG bit */
#define DCMIPP_P0CSCSZR_ENABLE_Pos          (31U)
#define DCMIPP_P0CSCSZR_ENABLE_Msk          (0x1UL << DCMIPP_P0CSCSZR_ENABLE_Pos)            /*!< 0x80000000 */
#define DCMIPP_P0CSCSZR_ENABLE              DCMIPP_P0CSCSZR_ENABLE_Msk                      /*!< Current value of the ENABLE bit */

/****************  Bit definition for DCMIPP_P0CPPCR register  ****************/
#define DCMIPP_P0CPPCR_SWAPYUV_Pos          (0U)
#define DCMIPP_P0CPPCR_SWAPYUV_Msk          (0x1UL << DCMIPP_P0CPPCR_SWAPYUV_Pos)            /*!< 0x00000001 */
#define DCMIPP_P0CPPCR_SWAPYUV              DCMIPP_P0CPPCR_SWAPYUV_Msk                      /*!< SwapY vs UV bits, when the YUV mode is active */
#define DCMIPP_P0CPPCR_PAD_Pos              (5U)
#define DCMIPP_P0CPPCR_PAD_Msk              (0x1UL << DCMIPP_P0CPPCR_PAD_Pos)                /*!< 0x00000020 */
#define DCMIPP_P0CPPCR_PAD                  DCMIPP_P0CPPCR_PAD_Msk                          /*!< Current Pad mode for monochrome and raw Bayer 10/12/14 bpp: MSB vs */
#define DCMIPP_P0CPPCR_HEADEREN_Pos         (6U)
#define DCMIPP_P0CPPCR_HEADEREN_Msk         (0x1UL << DCMIPP_P0CPPCR_HEADEREN_Pos)           /*!< 0x00000040 */
#define DCMIPP_P0CPPCR_HEADEREN             DCMIPP_P0CPPCR_HEADEREN_Msk                     /*!< Current CSI header dump enable */
#define DCMIPP_P0CPPCR_BSM_Pos              (7U)
#define DCMIPP_P0CPPCR_BSM_Msk              (0x3UL << DCMIPP_P0CPPCR_BSM_Pos)                /*!< 0x00000180 */
#define DCMIPP_P0CPPCR_BSM                  DCMIPP_P0CPPCR_BSM_Msk                          /*!< Current Byte select mode */
#define DCMIPP_P0CPPCR_OEBS_Pos             (9U)
#define DCMIPP_P0CPPCR_OEBS_Msk             (0x1UL << DCMIPP_P0CPPCR_OEBS_Pos)               /*!< 0x00000200 */
#define DCMIPP_P0CPPCR_OEBS                 DCMIPP_P0CPPCR_OEBS_Msk                         /*!< Current odd/even byte select (Byte select start) */
#define DCMIPP_P0CPPCR_LSM_Pos              (10U)
#define DCMIPP_P0CPPCR_LSM_Msk              (0x1UL << DCMIPP_P0CPPCR_LSM_Pos)                /*!< 0x00000400 */
#define DCMIPP_P0CPPCR_LSM                  DCMIPP_P0CPPCR_LSM_Msk                          /*!< Current Line select mode */
#define DCMIPP_P0CPPCR_OELS_Pos             (11U)
#define DCMIPP_P0CPPCR_OELS_Msk             (0x1UL << DCMIPP_P0CPPCR_OELS_Pos)               /*!< 0x00000800 */
#define DCMIPP_P0CPPCR_OELS                 DCMIPP_P0CPPCR_OELS_Msk                         /*!< Current odd/even line select (Line select start) */
#define DCMIPP_P0CPPCR_LINEMULT_Pos         (13U)
#define DCMIPP_P0CPPCR_LINEMULT_Msk         (0x7UL << DCMIPP_P0CPPCR_LINEMULT_Pos)           /*!< 0x0000E000 */
#define DCMIPP_P0CPPCR_LINEMULT             DCMIPP_P0CPPCR_LINEMULT_Msk                     /*!< Current amount of capture completed lines for LINE Event and Interrupt */
#define DCMIPP_P0CPPCR_DBM_Pos              (16U)
#define DCMIPP_P0CPPCR_DBM_Msk              (0x1UL << DCMIPP_P0CPPCR_LINEMULT_Pos)           /*!< 0x00010000 */
#define DCMIPP_P0CPPCR_DBM                  DCMIPP_P0CPPCR_LINEMULT_Msk                     /*!< Double buffer mode */

/**************  Bit definition for DCMIPP_P0CPPM0AR1 register  ***************/
#define DCMIPP_P0CPPM0AR1_M0A_Pos           (0U)
#define DCMIPP_P0CPPM0AR1_M0A_Msk           (0xFFFFFFFFUL << DCMIPP_P0CPPM0AR1_M0A_Pos)      /*!< 0xFFFFFFFF */
#define DCMIPP_P0CPPM0AR1_M0A               DCMIPP_P0CPPM0AR1_M0A_Msk                       /*!< Memory0 address */

/****************  Bit definition for DCMIPP_P1FSCR register  *****************/
#define DCMIPP_P1FSCR_DTIDA_Pos             (0U)
#define DCMIPP_P1FSCR_DTIDA_Msk             (0x3FUL << DCMIPP_P1FSCR_DTIDA_Pos)              /*!< 0x0000003F */
#define DCMIPP_P1FSCR_DTIDA                 DCMIPP_P1FSCR_DTIDA_Msk                         /*!< Data type ID A */
#define DCMIPP_P1FSCR_DTIDB_Pos             (8U)
#define DCMIPP_P1FSCR_DTIDB_Msk             (0x3FUL << DCMIPP_P1FSCR_DTIDB_Pos)              /*!< 0x00003F00 */
#define DCMIPP_P1FSCR_DTIDB                 DCMIPP_P1FSCR_DTIDB_Msk                         /*!< Data type ID B */
#define DCMIPP_P1FSCR_DTMODE_Pos            (16U)
#define DCMIPP_P1FSCR_DTMODE_Msk            (0x3UL << DCMIPP_P1FSCR_DTMODE_Pos)              /*!< 0x00030000 */
#define DCMIPP_P1FSCR_DTMODE                DCMIPP_P1FSCR_DTMODE_Msk                        /*!< Flow selection mode */
#define DCMIPP_P1FSCR_PIPEDIFF_Pos          (18U)
#define DCMIPP_P1FSCR_PIPEDIFF_Msk          (0x1UL << DCMIPP_P1FSCR_PIPEDIFF_Pos)            /*!< 0x00040000 */
#define DCMIPP_P1FSCR_PIPEDIFF              DCMIPP_P1FSCR_PIPEDIFF_Msk                      /*!< Differentiates Pipe2 vs */
#define DCMIPP_P1FSCR_VC_Pos                (19U)
#define DCMIPP_P1FSCR_VC_Msk                (0x3UL << DCMIPP_P1FSCR_VC_Pos)                  /*!< 0x00180000 */
#define DCMIPP_P1FSCR_VC                    DCMIPP_P1FSCR_VC_Msk                            /*!< Flow selection mode */
#define DCMIPP_P1FSCR_FDTF_Pos              (24U)
#define DCMIPP_P1FSCR_FDTF_Msk              (0x3FUL << DCMIPP_P1FSCR_FDTF_Pos)               /*!< 0x3F000000 */
#define DCMIPP_P1FSCR_FDTF                  DCMIPP_P1FSCR_FDTF_Msk                          /*!< Force Data type format */
#define DCMIPP_P1FSCR_FDTFEN_Pos            (30U)
#define DCMIPP_P1FSCR_FDTFEN_Msk            (0x1UL << DCMIPP_P1FSCR_FDTFEN_Pos)              /*!< 0x40000000 */
#define DCMIPP_P1FSCR_FDTFEN                DCMIPP_P1FSCR_FDTFEN_Msk                        /*!< Force Data type format enable */
#define DCMIPP_P1FSCR_PIPEN_Pos             (31U)
#define DCMIPP_P1FSCR_PIPEN_Msk             (0x1UL << DCMIPP_P1FSCR_PIPEN_Pos)               /*!< 0x80000000 */
#define DCMIPP_P1FSCR_PIPEN                 DCMIPP_P1FSCR_PIPEN_Msk                         /*!< Activation of PipeN */

/****************  Bit definition for DCMIPP_P1SRCR register  *****************/
#define DCMIPP_P1SRCR_LASTLINE_Pos          (0U)
#define DCMIPP_P1SRCR_LASTLINE_Msk          (0xFFFUL << DCMIPP_P1SRCR_LASTLINE_Pos)          /*!< 0x00000FFF */
#define DCMIPP_P1SRCR_LASTLINE              DCMIPP_P1SRCR_LASTLINE_Msk                      /*!< Number of the last line to be kept when CROPEN = 1 */
#define DCMIPP_P1SRCR_FIRSTLINEDEL_Pos      (12U)
#define DCMIPP_P1SRCR_FIRSTLINEDEL_Msk      (0x7UL << DCMIPP_P1SRCR_FIRSTLINEDEL_Pos)        /*!< 0x00007000 */
#define DCMIPP_P1SRCR_FIRSTLINEDEL          DCMIPP_P1SRCR_FIRSTLINEDEL_Msk                  /*!< Number of lines to be deleted when CROPEN = 1 */
#define DCMIPP_P1SRCR_CROPEN_Pos            (15U)
#define DCMIPP_P1SRCR_CROPEN_Msk            (0x1UL << DCMIPP_P1SRCR_CROPEN_Pos)              /*!< 0x00008000 */
#define DCMIPP_P1SRCR_CROPEN                DCMIPP_P1SRCR_CROPEN_Msk                        /*!< Crop line enable */

/****************  Bit definition for DCMIPP_P1BPRCR register  ****************/
#define DCMIPP_P1BPRCR_ENABLE_Pos           (0U)
#define DCMIPP_P1BPRCR_ENABLE_Msk           (0x1UL << DCMIPP_P1BPRCR_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P1BPRCR_ENABLE               DCMIPP_P1BPRCR_ENABLE_Msk                       /*!< Bad pixel detection must be enabled only for raw Bayer flows, as it corrupts RGB flows */
#define DCMIPP_P1BPRCR_STRENGTH_Pos         (1U)
#define DCMIPP_P1BPRCR_STRENGTH_Msk         (0x7UL << DCMIPP_P1BPRCR_STRENGTH_Pos)           /*!< 0x0000000E */
#define DCMIPP_P1BPRCR_STRENGTH             DCMIPP_P1BPRCR_STRENGTH_Msk                     /*!< Strength (aggressivity) of the bad pixel detection: */

/****************  Bit definition for DCMIPP_P1BPRSR register  ****************/
#define DCMIPP_P1BPRSR_BADCNT_Pos           (0U)
#define DCMIPP_P1BPRSR_BADCNT_Msk           (0xFFFUL << DCMIPP_P1BPRSR_BADCNT_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P1BPRSR_BADCNT               DCMIPP_P1BPRSR_BADCNT_Msk                       /*!< Amount of detected bad pixels */

/****************  Bit definition for DCMIPP_P1DECR register  *****************/
#define DCMIPP_P1DECR_ENABLE_Pos            (0U)
#define DCMIPP_P1DECR_ENABLE_Msk            (0x1UL << DCMIPP_P1DECR_ENABLE_Pos)              /*!< 0x00000001 */
#define DCMIPP_P1DECR_ENABLE                DCMIPP_P1DECR_ENABLE_Msk                        /*!<  */
#define DCMIPP_P1DECR_HDEC_Pos              (1U)
#define DCMIPP_P1DECR_HDEC_Msk              (0x3UL << DCMIPP_P1DECR_HDEC_Pos)                /*!< 0x00000006 */
#define DCMIPP_P1DECR_HDEC                  DCMIPP_P1DECR_HDEC_Msk                          /*!< Horizontal decimation ratio */
#define DCMIPP_P1DECR_VDEC_Pos              (3U)
#define DCMIPP_P1DECR_VDEC_Msk              (0x3UL << DCMIPP_P1DECR_VDEC_Pos)                /*!< 0x00000018 */
#define DCMIPP_P1DECR_VDEC                  DCMIPP_P1DECR_VDEC_Msk                          /*!< Vertical decimation ratio */

/****************  Bit definition for DCMIPP_P1BLCCR register  ****************/
#define DCMIPP_P1BLCCR_ENABLE_Pos           (0U)
#define DCMIPP_P1BLCCR_ENABLE_Msk           (0x1UL << DCMIPP_P1BLCCR_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P1BLCCR_ENABLE               DCMIPP_P1BLCCR_ENABLE_Msk                       /*!< Black level calibration */
#define DCMIPP_P1BLCCR_BLCB_Pos             (8U)
#define DCMIPP_P1BLCCR_BLCB_Msk             (0xFFUL << DCMIPP_P1BLCCR_BLCB_Pos)              /*!< 0x0000FF00 */
#define DCMIPP_P1BLCCR_BLCB                 DCMIPP_P1BLCCR_BLCB_Msk                         /*!< Black level calibration - Blue */
#define DCMIPP_P1BLCCR_BLCG_Pos             (16U)
#define DCMIPP_P1BLCCR_BLCG_Msk             (0xFFUL << DCMIPP_P1BLCCR_BLCG_Pos)              /*!< 0x00FF0000 */
#define DCMIPP_P1BLCCR_BLCG                 DCMIPP_P1BLCCR_BLCG_Msk                         /*!< Black level calibration - Green */
#define DCMIPP_P1BLCCR_BLCR_Pos             (24U)
#define DCMIPP_P1BLCCR_BLCR_Msk             (0xFFUL << DCMIPP_P1BLCCR_BLCR_Pos)              /*!< 0xFF000000 */
#define DCMIPP_P1BLCCR_BLCR                 DCMIPP_P1BLCCR_BLCR_Msk                         /*!< Black level calibration - Red */

/****************  Bit definition for DCMIPP_P1EXCR1 register  ****************/
#define DCMIPP_P1EXCR1_ENABLE_Pos           (0U)
#define DCMIPP_P1EXCR1_ENABLE_Msk           (0x1UL << DCMIPP_P1EXCR1_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P1EXCR1_ENABLE               DCMIPP_P1EXCR1_ENABLE_Msk                       /*!< Exposure control (multiplication and shift) of all red, green and blue */
#define DCMIPP_P1EXCR1_MULTR_Pos            (20U)
#define DCMIPP_P1EXCR1_MULTR_Msk            (0xFFUL << DCMIPP_P1EXCR1_MULTR_Pos)             /*!< 0x0FF00000 */
#define DCMIPP_P1EXCR1_MULTR                DCMIPP_P1EXCR1_MULTR_Msk                        /*!< Exposure multiplier - Red */
#define DCMIPP_P1EXCR1_SHFR_Pos             (28U)
#define DCMIPP_P1EXCR1_SHFR_Msk             (0x7UL << DCMIPP_P1EXCR1_SHFR_Pos)               /*!< 0x70000000 */
#define DCMIPP_P1EXCR1_SHFR                 DCMIPP_P1EXCR1_SHFR_Msk                         /*!< Exposure shift - Red */

/****************  Bit definition for DCMIPP_P1EXCR2 register  ****************/
#define DCMIPP_P1EXCR2_MULTB_Pos            (4U)
#define DCMIPP_P1EXCR2_MULTB_Msk            (0xFFUL << DCMIPP_P1EXCR2_MULTB_Pos)             /*!< 0x00000FF0 */
#define DCMIPP_P1EXCR2_MULTB                DCMIPP_P1EXCR2_MULTB_Msk                        /*!< Exposure multiplier - Blue */
#define DCMIPP_P1EXCR2_SHFB_Pos             (12U)
#define DCMIPP_P1EXCR2_SHFB_Msk             (0x7UL << DCMIPP_P1EXCR2_SHFB_Pos)               /*!< 0x00007000 */
#define DCMIPP_P1EXCR2_SHFB                 DCMIPP_P1EXCR2_SHFB_Msk                         /*!< Exposure shift - Blue */
#define DCMIPP_P1EXCR2_MULTG_Pos            (20U)
#define DCMIPP_P1EXCR2_MULTG_Msk            (0xFFUL << DCMIPP_P1EXCR2_MULTG_Pos)             /*!< 0x0FF00000 */
#define DCMIPP_P1EXCR2_MULTG                DCMIPP_P1EXCR2_MULTG_Msk                        /*!< Exposure multiplier - Green */
#define DCMIPP_P1EXCR2_SHFG_Pos             (28U)
#define DCMIPP_P1EXCR2_SHFG_Msk             (0x7UL << DCMIPP_P1EXCR2_SHFG_Pos)               /*!< 0x70000000 */
#define DCMIPP_P1EXCR2_SHFG                 DCMIPP_P1EXCR2_SHFG_Msk                         /*!< Exposure shift - Green */

/****************  Bit definition for DCMIPP_P1ST1CR register  ****************/
#define DCMIPP_P1ST1CR_ENABLE_Pos           (0U)
#define DCMIPP_P1ST1CR_ENABLE_Msk           (0x1UL << DCMIPP_P1ST1CR_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P1ST1CR_ENABLE               DCMIPP_P1ST1CR_ENABLE_Msk                       /*!<  */
#define DCMIPP_P1ST1CR_BINS_Pos             (2U)
#define DCMIPP_P1ST1CR_BINS_Msk             (0x3UL << DCMIPP_P1ST1CR_BINS_Pos)               /*!< 0x0000000C */
#define DCMIPP_P1ST1CR_BINS                 DCMIPP_P1ST1CR_BINS_Msk                         /*!< Bin definition */
#define DCMIPP_P1ST1CR_SRC_Pos              (4U)
#define DCMIPP_P1ST1CR_SRC_Msk              (0x7UL << DCMIPP_P1ST1CR_SRC_Pos)                /*!< 0x00000070 */
#define DCMIPP_P1ST1CR_SRC                  DCMIPP_P1ST1CR_SRC_Msk                          /*!< Statistics source */
#define DCMIPP_P1ST1CR_MODE_Pos             (7U)
#define DCMIPP_P1ST1CR_MODE_Msk             (0x1UL << DCMIPP_P1ST1CR_MODE_Pos)               /*!< 0x00000080 */
#define DCMIPP_P1ST1CR_MODE                 DCMIPP_P1ST1CR_MODE_Msk                         /*!< Statistics mode */

/****************  Bit definition for DCMIPP_P1ST2CR register  ****************/
#define DCMIPP_P1ST2CR_ENABLE_Pos           (0U)
#define DCMIPP_P1ST2CR_ENABLE_Msk           (0x1UL << DCMIPP_P1ST2CR_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P1ST2CR_ENABLE               DCMIPP_P1ST2CR_ENABLE_Msk                       /*!<  */
#define DCMIPP_P1ST2CR_BINS_Pos             (2U)
#define DCMIPP_P1ST2CR_BINS_Msk             (0x3UL << DCMIPP_P1ST2CR_BINS_Pos)               /*!< 0x0000000C */
#define DCMIPP_P1ST2CR_BINS                 DCMIPP_P1ST2CR_BINS_Msk                         /*!< Bin definition */
#define DCMIPP_P1ST2CR_SRC_Pos              (4U)
#define DCMIPP_P1ST2CR_SRC_Msk              (0x7UL << DCMIPP_P1ST2CR_SRC_Pos)                /*!< 0x00000070 */
#define DCMIPP_P1ST2CR_SRC                  DCMIPP_P1ST2CR_SRC_Msk                          /*!< Statistics source */
#define DCMIPP_P1ST2CR_MODE_Pos             (7U)
#define DCMIPP_P1ST2CR_MODE_Msk             (0x1UL << DCMIPP_P1ST2CR_MODE_Pos)               /*!< 0x00000080 */
#define DCMIPP_P1ST2CR_MODE                 DCMIPP_P1ST2CR_MODE_Msk                         /*!< Statistics mode */

/****************  Bit definition for DCMIPP_P1ST3CR register  ****************/
#define DCMIPP_P1ST3CR_ENABLE_Pos           (0U)
#define DCMIPP_P1ST3CR_ENABLE_Msk           (0x1UL << DCMIPP_P1ST3CR_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P1ST3CR_ENABLE               DCMIPP_P1ST3CR_ENABLE_Msk                       /*!<  */
#define DCMIPP_P1ST3CR_BINS_Pos             (2U)
#define DCMIPP_P1ST3CR_BINS_Msk             (0x3UL << DCMIPP_P1ST3CR_BINS_Pos)               /*!< 0x0000000C */
#define DCMIPP_P1ST3CR_BINS                 DCMIPP_P1ST3CR_BINS_Msk                         /*!< Bin definition */
#define DCMIPP_P1ST3CR_SRC_Pos              (4U)
#define DCMIPP_P1ST3CR_SRC_Msk              (0x7UL << DCMIPP_P1ST3CR_SRC_Pos)                /*!< 0x00000070 */
#define DCMIPP_P1ST3CR_SRC                  DCMIPP_P1ST3CR_SRC_Msk                          /*!< Statistics source */
#define DCMIPP_P1ST3CR_MODE_Pos             (7U)
#define DCMIPP_P1ST3CR_MODE_Msk             (0x1UL << DCMIPP_P1ST3CR_MODE_Pos)               /*!< 0x00000080 */
#define DCMIPP_P1ST3CR_MODE                 DCMIPP_P1ST3CR_MODE_Msk                         /*!< Statistics mode */

/****************  Bit definition for DCMIPP_P1STSTR register  ****************/
#define DCMIPP_P1STSTR_HSTART_Pos           (0U)
#define DCMIPP_P1STSTR_HSTART_Msk           (0xFFFUL << DCMIPP_P1STSTR_HSTART_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P1STSTR_HSTART               DCMIPP_P1STSTR_HSTART_Msk                       /*!< Horizontal start, from 0 to 4094 pixels wide */
#define DCMIPP_P1STSTR_VSTART_Pos           (16U)
#define DCMIPP_P1STSTR_VSTART_Msk           (0xFFFUL << DCMIPP_P1STSTR_VSTART_Pos)           /*!< 0x0FFF0000 */
#define DCMIPP_P1STSTR_VSTART               DCMIPP_P1STSTR_VSTART_Msk                       /*!< Vertical start, from 0 to 4094 pixels high */

/****************  Bit definition for DCMIPP_P1STSZR register  ****************/
#define DCMIPP_P1STSZR_HSIZE_Pos            (0U)
#define DCMIPP_P1STSZR_HSIZE_Msk            (0xFFFUL << DCMIPP_P1STSZR_HSIZE_Pos)            /*!< 0x00000FFF */
#define DCMIPP_P1STSZR_HSIZE                DCMIPP_P1STSZR_HSIZE_Msk                        /*!< Horizontal size, from 0 to 4094 pixels wide */
#define DCMIPP_P1STSZR_VSIZE_Pos            (16U)
#define DCMIPP_P1STSZR_VSIZE_Msk            (0xFFFUL << DCMIPP_P1STSZR_VSIZE_Pos)            /*!< 0x0FFF0000 */
#define DCMIPP_P1STSZR_VSIZE                DCMIPP_P1STSZR_VSIZE_Msk                        /*!< Vertical size, from 0 to 4094 pixels high */
#define DCMIPP_P1STSZR_CROPEN_Pos           (31U)
#define DCMIPP_P1STSZR_CROPEN_Msk           (0x1UL << DCMIPP_P1STSZR_CROPEN_Pos)             /*!< 0x80000000 */
#define DCMIPP_P1STSZR_CROPEN               DCMIPP_P1STSZR_CROPEN_Msk                       /*!<  */

/****************  Bit definition for DCMIPP_P1ST1SR register  ****************/
#define DCMIPP_P1ST1SR_ACCU_Pos             (0U)
#define DCMIPP_P1ST1SR_ACCU_Msk             (0xFFFFFFUL << DCMIPP_P1ST1SR_ACCU_Pos)          /*!< 0x00FFFFFF */
#define DCMIPP_P1ST1SR_ACCU                 DCMIPP_P1ST1SR_ACCU_Msk                         /*!< Accumulation result, divided by 256 */

/****************  Bit definition for DCMIPP_P1ST2SR register  ****************/
#define DCMIPP_P1ST2SR_ACCU_Pos             (0U)
#define DCMIPP_P1ST2SR_ACCU_Msk             (0xFFFFFFUL << DCMIPP_P1ST2SR_ACCU_Pos)          /*!< 0x00FFFFFF */
#define DCMIPP_P1ST2SR_ACCU                 DCMIPP_P1ST2SR_ACCU_Msk                         /*!< accumulation result, divided by 256 */

/****************  Bit definition for DCMIPP_P1ST3SR register  ****************/
#define DCMIPP_P1ST3SR_ACCU_Pos             (0U)
#define DCMIPP_P1ST3SR_ACCU_Msk             (0xFFFFFFUL << DCMIPP_P1ST3SR_ACCU_Pos)          /*!< 0x00FFFFFF */
#define DCMIPP_P1ST3SR_ACCU                 DCMIPP_P1ST3SR_ACCU_Msk                         /*!< accumulation result, divided by 256 */

/****************  Bit definition for DCMIPP_P1DMCR register  *****************/
#define DCMIPP_P1DMCR_ENABLE_Pos            (0U)
#define DCMIPP_P1DMCR_ENABLE_Msk            (0x1UL << DCMIPP_P1DMCR_ENABLE_Pos)              /*!< 0x00000001 */
#define DCMIPP_P1DMCR_ENABLE                DCMIPP_P1DMCR_ENABLE_Msk                        /*!<  */
#define DCMIPP_P1DMCR_TYPE_Pos              (1U)
#define DCMIPP_P1DMCR_TYPE_Msk              (0x3UL << DCMIPP_P1DMCR_TYPE_Pos)                /*!< 0x00000006 */
#define DCMIPP_P1DMCR_TYPE                  DCMIPP_P1DMCR_TYPE_Msk                          /*!< Raw Bayer type */
#define DCMIPP_P1DMCR_PEAK_Pos              (16U)
#define DCMIPP_P1DMCR_PEAK_Msk              (0x7UL << DCMIPP_P1DMCR_PEAK_Pos)                /*!< 0x00070000 */
#define DCMIPP_P1DMCR_PEAK                  DCMIPP_P1DMCR_PEAK_Msk                          /*!< Strength of the peak detection */
#define DCMIPP_P1DMCR_LINEV_Pos             (20U)
#define DCMIPP_P1DMCR_LINEV_Msk             (0x7UL << DCMIPP_P1DMCR_LINEV_Pos)               /*!< 0x00700000 */
#define DCMIPP_P1DMCR_LINEV                 DCMIPP_P1DMCR_LINEV_Msk                         /*!< Strength of the vertical line detection */
#define DCMIPP_P1DMCR_LINEH_Pos             (24U)
#define DCMIPP_P1DMCR_LINEH_Msk             (0x7UL << DCMIPP_P1DMCR_LINEH_Pos)               /*!< 0x07000000 */
#define DCMIPP_P1DMCR_LINEH                 DCMIPP_P1DMCR_LINEH_Msk                         /*!< Strength of the horizontal line detection */
#define DCMIPP_P1DMCR_EDGE_Pos              (28U)
#define DCMIPP_P1DMCR_EDGE_Msk              (0x7UL << DCMIPP_P1DMCR_EDGE_Pos)                /*!< 0x70000000 */
#define DCMIPP_P1DMCR_EDGE                  DCMIPP_P1DMCR_EDGE_Msk                          /*!< Strength of the edge detection */

/****************  Bit definition for DCMIPP_P1CCCR register  *****************/
#define DCMIPP_P1CCCR_ENABLE_Pos            (0U)
#define DCMIPP_P1CCCR_ENABLE_Msk            (0x1UL << DCMIPP_P1CCCR_ENABLE_Pos)              /*!< 0x00000001 */
#define DCMIPP_P1CCCR_ENABLE                DCMIPP_P1CCCR_ENABLE_Msk                        /*!<  */
#define DCMIPP_P1CCCR_TYPE_Pos              (1U)
#define DCMIPP_P1CCCR_TYPE_Msk              (0x1UL << DCMIPP_P1CCCR_TYPE_Pos)                /*!< 0x00000002 */
#define DCMIPP_P1CCCR_TYPE                  DCMIPP_P1CCCR_TYPE_Msk                          /*!< output samples type used while CLAMP is activated */
#define DCMIPP_P1CCCR_CLAMP_Pos             (2U)
#define DCMIPP_P1CCCR_CLAMP_Msk             (0x1UL << DCMIPP_P1CCCR_CLAMP_Pos)               /*!< 0x00000004 */
#define DCMIPP_P1CCCR_CLAMP                 DCMIPP_P1CCCR_CLAMP_Msk                         /*!< Clamp the output samples */

/****************  Bit definition for DCMIPP_P1CCRR1 register  ****************/
#define DCMIPP_P1CCRR1_RR_Pos               (0U)
#define DCMIPP_P1CCRR1_RR_Msk               (0x7FFUL << DCMIPP_P1CCRR1_RR_Pos)               /*!< 0x000007FF */
#define DCMIPP_P1CCRR1_RR                   DCMIPP_P1CCRR1_RR_Msk                           /*!< Coefficient row 1 column 1 of the matrix */
#define DCMIPP_P1CCRR1_RG_Pos               (16U)
#define DCMIPP_P1CCRR1_RG_Msk               (0x7FFUL << DCMIPP_P1CCRR1_RG_Pos)               /*!< 0x07FF0000 */
#define DCMIPP_P1CCRR1_RG                   DCMIPP_P1CCRR1_RG_Msk                           /*!< Coefficient row 1 column 2 of the matrix */

/****************  Bit definition for DCMIPP_P1CCRR2 register  ****************/
#define DCMIPP_P1CCRR2_RB_Pos               (0U)
#define DCMIPP_P1CCRR2_RB_Msk               (0x7FFUL << DCMIPP_P1CCRR2_RB_Pos)               /*!< 0x000007FF */
#define DCMIPP_P1CCRR2_RB                   DCMIPP_P1CCRR2_RB_Msk                           /*!< Coefficient row 1 column 3 of the matrix */
#define DCMIPP_P1CCRR2_RA_Pos               (16U)
#define DCMIPP_P1CCRR2_RA_Msk               (0x3FFUL << DCMIPP_P1CCRR2_RA_Pos)               /*!< 0x03FF0000 */
#define DCMIPP_P1CCRR2_RA                   DCMIPP_P1CCRR2_RA_Msk                           /*!< Coefficient row 1 of the added column (signed integer value) */

/****************  Bit definition for DCMIPP_P1CCGR1 register  ****************/
#define DCMIPP_P1CCGR1_GR_Pos               (0U)
#define DCMIPP_P1CCGR1_GR_Msk               (0x7FFUL << DCMIPP_P1CCGR1_GR_Pos)               /*!< 0x000007FF */
#define DCMIPP_P1CCGR1_GR                   DCMIPP_P1CCGR1_GR_Msk                           /*!< Coefficient row 2 column 1 of the matrix */
#define DCMIPP_P1CCGR1_GG_Pos               (16U)
#define DCMIPP_P1CCGR1_GG_Msk               (0x7FFUL << DCMIPP_P1CCGR1_GG_Pos)               /*!< 0x07FF0000 */
#define DCMIPP_P1CCGR1_GG                   DCMIPP_P1CCGR1_GG_Msk                           /*!< Coefficient row 2 column 2 of the matrix */

/****************  Bit definition for DCMIPP_P1CCGR2 register  ****************/
#define DCMIPP_P1CCGR2_GB_Pos               (0U)
#define DCMIPP_P1CCGR2_GB_Msk               (0x7FFUL << DCMIPP_P1CCGR2_GB_Pos)               /*!< 0x000007FF */
#define DCMIPP_P1CCGR2_GB                   DCMIPP_P1CCGR2_GB_Msk                           /*!< Coefficient row 2 column 3 of the matrix */
#define DCMIPP_P1CCGR2_GA_Pos               (16U)
#define DCMIPP_P1CCGR2_GA_Msk               (0x3FFUL << DCMIPP_P1CCGR2_GA_Pos)               /*!< 0x03FF0000 */
#define DCMIPP_P1CCGR2_GA                   DCMIPP_P1CCGR2_GA_Msk                           /*!< Coefficient row 2 of the added column (signed integer value) */

/****************  Bit definition for DCMIPP_P1CCBR1 register  ****************/
#define DCMIPP_P1CCBR1_BR_Pos               (0U)
#define DCMIPP_P1CCBR1_BR_Msk               (0x7FFUL << DCMIPP_P1CCBR1_BR_Pos)               /*!< 0x000007FF */
#define DCMIPP_P1CCBR1_BR                   DCMIPP_P1CCBR1_BR_Msk                           /*!< Coefficient row 3 column 1 of the matrix */
#define DCMIPP_P1CCBR1_BG_Pos               (16U)
#define DCMIPP_P1CCBR1_BG_Msk               (0x7FFUL << DCMIPP_P1CCBR1_BG_Pos)               /*!< 0x07FF0000 */
#define DCMIPP_P1CCBR1_BG                   DCMIPP_P1CCBR1_BG_Msk                           /*!< Coefficient row 3 column 2 of the matrix */

/****************  Bit definition for DCMIPP_P1CCBR2 register  ****************/
#define DCMIPP_P1CCBR2_BB_Pos               (0U)
#define DCMIPP_P1CCBR2_BB_Msk               (0x7FFUL << DCMIPP_P1CCBR2_BB_Pos)               /*!< 0x000007FF */
#define DCMIPP_P1CCBR2_BB                   DCMIPP_P1CCBR2_BB_Msk                           /*!< Coefficient row 3 column 3 of the matrix */
#define DCMIPP_P1CCBR2_BA_Pos               (16U)
#define DCMIPP_P1CCBR2_BA_Msk               (0x3FFUL << DCMIPP_P1CCBR2_BA_Pos)               /*!< 0x03FF0000 */
#define DCMIPP_P1CCBR2_BA                   DCMIPP_P1CCBR2_BA_Msk                           /*!< Coefficient row 3 of the added column (signed integer value) */

/****************  Bit definition for DCMIPP_P1CTCR1 register  ****************/
#define DCMIPP_P1CTCR1_ENABLE_Pos           (0U)
#define DCMIPP_P1CTCR1_ENABLE_Msk           (0x1UL << DCMIPP_P1CTCR1_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P1CTCR1_ENABLE               DCMIPP_P1CTCR1_ENABLE_Msk                       /*!<  */
#define DCMIPP_P1CTCR1_LUM0_Pos             (9U)
#define DCMIPP_P1CTCR1_LUM0_Msk             (0x3FUL << DCMIPP_P1CTCR1_LUM0_Pos)              /*!< 0x00007E00 */
#define DCMIPP_P1CTCR1_LUM0                 DCMIPP_P1CTCR1_LUM0_Msk                         /*!< Luminance increase for input luminance of 0 (increase is idle with LUMx = 16) */

/****************  Bit definition for DCMIPP_P1CTCR2 register  ****************/
#define DCMIPP_P1CTCR2_LUM4_Pos             (1U)
#define DCMIPP_P1CTCR2_LUM4_Msk             (0x3FUL << DCMIPP_P1CTCR2_LUM4_Pos)              /*!< 0x0000007E */
#define DCMIPP_P1CTCR2_LUM4                 DCMIPP_P1CTCR2_LUM4_Msk                         /*!< Luminance increase for input luminance of 128 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CTCR2_LUM3_Pos             (9U)
#define DCMIPP_P1CTCR2_LUM3_Msk             (0x3FUL << DCMIPP_P1CTCR2_LUM3_Pos)              /*!< 0x00007E00 */
#define DCMIPP_P1CTCR2_LUM3                 DCMIPP_P1CTCR2_LUM3_Msk                         /*!< Luminance increase for input luminance of 96 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CTCR2_LUM2_Pos             (17U)
#define DCMIPP_P1CTCR2_LUM2_Msk             (0x3FUL << DCMIPP_P1CTCR2_LUM2_Pos)              /*!< 0x007E0000 */
#define DCMIPP_P1CTCR2_LUM2                 DCMIPP_P1CTCR2_LUM2_Msk                         /*!< Luminance increase for input luminance of 64 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CTCR2_LUM1_Pos             (25U)
#define DCMIPP_P1CTCR2_LUM1_Msk             (0x3FUL << DCMIPP_P1CTCR2_LUM1_Pos)              /*!< 0x7E000000 */
#define DCMIPP_P1CTCR2_LUM1                 DCMIPP_P1CTCR2_LUM1_Msk                         /*!< Luminance increase for input luminance of 32 (increase is idle with LUMx = 16) */

/****************  Bit definition for DCMIPP_P1CTCR3 register  ****************/
#define DCMIPP_P1CTCR3_LUM8_Pos             (1U)
#define DCMIPP_P1CTCR3_LUM8_Msk             (0x3FUL << DCMIPP_P1CTCR3_LUM8_Pos)              /*!< 0x0000007E */
#define DCMIPP_P1CTCR3_LUM8                 DCMIPP_P1CTCR3_LUM8_Msk                         /*!< Luminance increase for input luminance of 256 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CTCR3_LUM7_Pos             (9U)
#define DCMIPP_P1CTCR3_LUM7_Msk             (0x3FUL << DCMIPP_P1CTCR3_LUM7_Pos)              /*!< 0x00007E00 */
#define DCMIPP_P1CTCR3_LUM7                 DCMIPP_P1CTCR3_LUM7_Msk                         /*!< Luminance increase for input luminance of 224 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CTCR3_LUM6_Pos             (17U)
#define DCMIPP_P1CTCR3_LUM6_Msk             (0x3FUL << DCMIPP_P1CTCR3_LUM6_Pos)              /*!< 0x007E0000 */
#define DCMIPP_P1CTCR3_LUM6                 DCMIPP_P1CTCR3_LUM6_Msk                         /*!< Luminance increase for input luminance of 192 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CTCR3_LUM5_Pos             (25U)
#define DCMIPP_P1CTCR3_LUM5_Msk             (0x3FUL << DCMIPP_P1CTCR3_LUM5_Pos)              /*!< 0x7E000000 */
#define DCMIPP_P1CTCR3_LUM5                 DCMIPP_P1CTCR3_LUM5_Msk                         /*!< Luminance increase for input luminance of 160 (increase is idle with LUMx = 16) */

/****************  Bit definition for DCMIPP_P1FCTCR register  ****************/
#define DCMIPP_P1FCTCR_FRATE_Pos            (0U)
#define DCMIPP_P1FCTCR_FRATE_Msk            (0x3UL << DCMIPP_P1FCTCR_FRATE_Pos)              /*!< 0x00000003 */
#define DCMIPP_P1FCTCR_FRATE                DCMIPP_P1FCTCR_FRATE_Msk                        /*!< Frame capture rate control */
#define DCMIPP_P1FCTCR_CPTMODE_Pos          (2U)
#define DCMIPP_P1FCTCR_CPTMODE_Msk          (0x1UL << DCMIPP_P1FCTCR_CPTMODE_Pos)            /*!< 0x00000004 */
#define DCMIPP_P1FCTCR_CPTMODE              DCMIPP_P1FCTCR_CPTMODE_Msk                      /*!< Capture mode */
#define DCMIPP_P1FCTCR_CPTREQ_Pos           (3U)
#define DCMIPP_P1FCTCR_CPTREQ_Msk           (0x1UL << DCMIPP_P1FCTCR_CPTREQ_Pos)             /*!< 0x00000008 */
#define DCMIPP_P1FCTCR_CPTREQ               DCMIPP_P1FCTCR_CPTREQ_Msk                       /*!< Capture requested */

/****************  Bit definition for DCMIPP_P1CRSTR register  ****************/
#define DCMIPP_P1CRSTR_HSTART_Pos           (0U)
#define DCMIPP_P1CRSTR_HSTART_Msk           (0xFFFUL << DCMIPP_P1CRSTR_HSTART_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P1CRSTR_HSTART               DCMIPP_P1CRSTR_HSTART_Msk                       /*!< Horizontal start, from 0 to 4094 pixels wide */
#define DCMIPP_P1CRSTR_VSTART_Pos           (16U)
#define DCMIPP_P1CRSTR_VSTART_Msk           (0xFFFUL << DCMIPP_P1CRSTR_VSTART_Pos)           /*!< 0x0FFF0000 */
#define DCMIPP_P1CRSTR_VSTART               DCMIPP_P1CRSTR_VSTART_Msk                       /*!< Vertical start, from 0 to 4094 pixels high */

/****************  Bit definition for DCMIPP_P1CRSZR register  ****************/
#define DCMIPP_P1CRSZR_HSIZE_Pos            (0U)
#define DCMIPP_P1CRSZR_HSIZE_Msk            (0xFFFUL << DCMIPP_P1CRSZR_HSIZE_Pos)            /*!< 0x00000FFF */
#define DCMIPP_P1CRSZR_HSIZE                DCMIPP_P1CRSZR_HSIZE_Msk                        /*!< Horizontal size, from 0 to 4094 pixels wide */
#define DCMIPP_P1CRSZR_VSIZE_Pos            (16U)
#define DCMIPP_P1CRSZR_VSIZE_Msk            (0xFFFUL << DCMIPP_P1CRSZR_VSIZE_Pos)            /*!< 0x0FFF0000 */
#define DCMIPP_P1CRSZR_VSIZE                DCMIPP_P1CRSZR_VSIZE_Msk                        /*!< Vertical size, from 0 to 4094 pixels high */
#define DCMIPP_P1CRSZR_ENABLE_Pos           (31U)
#define DCMIPP_P1CRSZR_ENABLE_Msk           (0x1UL << DCMIPP_P1CRSZR_ENABLE_Pos)             /*!< 0x80000000 */
#define DCMIPP_P1CRSZR_ENABLE               DCMIPP_P1CRSZR_ENABLE_Msk                       /*!<  */

/****************  Bit definition for DCMIPP_P1DCCR register  *****************/
#define DCMIPP_P1DCCR_ENABLE_Pos            (0U)
#define DCMIPP_P1DCCR_ENABLE_Msk            (0x1UL << DCMIPP_P1DCCR_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P1DCCR_ENABLE                DCMIPP_P1DCCR_ENABLE_Msk                        /*!< Decimation enable */
#define DCMIPP_P1DCCR_HDEC_Pos              (1U)
#define DCMIPP_P1DCCR_HDEC_Msk              (0x3UL << DCMIPP_P1DCCR_HDEC_Pos)               /*!< 0x00000006 */
#define DCMIPP_P1DCCR_HDEC                  DCMIPP_P1DCCR_HDEC_Msk                          /*!< Horizontal decimation ratio */
#define DCMIPP_P1DCCR_VDEC_Pos              (3U)
#define DCMIPP_P1DCCR_VDEC_Msk              (0x3UL << DCMIPP_P1DCCR_VDEC_Pos)               /*!< 0x00000018 */
#define DCMIPP_P1DCCR_VDEC                  DCMIPP_P1DCCR_VDEC_Msk                          /*!< Vertical decimation ratio */

/****************  Bit definition for DCMIPP_P1DSCR register  *****************/
#define DCMIPP_P1DSCR_HDIV_Pos              (0U)
#define DCMIPP_P1DSCR_HDIV_Msk              (0x3FFUL << DCMIPP_P1DSCR_HDIV_Pos)              /*!< 0x000003FF */
#define DCMIPP_P1DSCR_HDIV                  DCMIPP_P1DSCR_HDIV_Msk                          /*!< Horizontal division factor, from 128 (8x) to 1023 (1x) */
#define DCMIPP_P1DSCR_VDIV_Pos              (16U)
#define DCMIPP_P1DSCR_VDIV_Msk              (0x3FFUL << DCMIPP_P1DSCR_VDIV_Pos)              /*!< 0x03FF0000 */
#define DCMIPP_P1DSCR_VDIV                  DCMIPP_P1DSCR_VDIV_Msk                          /*!< Vertical division factor, from 128 (8x) to 1023 (1x) */
#define DCMIPP_P1DSCR_ENABLE_Pos            (31U)
#define DCMIPP_P1DSCR_ENABLE_Msk            (0x1UL << DCMIPP_P1DSCR_ENABLE_Pos)              /*!< 0x80000000 */
#define DCMIPP_P1DSCR_ENABLE                DCMIPP_P1DSCR_ENABLE_Msk                        /*!<  Downscaler Enable */

/***************  Bit definition for DCMIPP_P1DSRTIOR register  ***************/
#define DCMIPP_P1DSRTIOR_HRATIO_Pos         (0U)
#define DCMIPP_P1DSRTIOR_HRATIO_Msk         (0xFFFFUL << DCMIPP_P1DSRTIOR_HRATIO_Pos)        /*!< 0x0000FFFF */
#define DCMIPP_P1DSRTIOR_HRATIO             DCMIPP_P1DSRTIOR_HRATIO_Msk                     /*!< Horizontal ratio, from 8192 (1x) to 65535 (8x) */
#define DCMIPP_P1DSRTIOR_VRATIO_Pos         (16U)
#define DCMIPP_P1DSRTIOR_VRATIO_Msk         (0xFFFFUL << DCMIPP_P1DSRTIOR_VRATIO_Pos)        /*!< 0xFFFF0000 */
#define DCMIPP_P1DSRTIOR_VRATIO             DCMIPP_P1DSRTIOR_VRATIO_Msk                     /*!< Vertical ratio, from 8192 (1x) to 65535 (8x) */

/****************  Bit definition for DCMIPP_P1DSSZR register  ****************/
#define DCMIPP_P1DSSZR_HSIZE_Pos            (0U)
#define DCMIPP_P1DSSZR_HSIZE_Msk            (0xFFFUL << DCMIPP_P1DSSZR_HSIZE_Pos)            /*!< 0x00000FFF */
#define DCMIPP_P1DSSZR_HSIZE                DCMIPP_P1DSSZR_HSIZE_Msk                        /*!< Horizontal size, from 0 to 4094 pixels wide */
#define DCMIPP_P1DSSZR_VSIZE_Pos            (16U)
#define DCMIPP_P1DSSZR_VSIZE_Msk            (0xFFFUL << DCMIPP_P1DSSZR_VSIZE_Pos)            /*!< 0x0FFF0000 */
#define DCMIPP_P1DSSZR_VSIZE                DCMIPP_P1DSSZR_VSIZE_Msk                        /*!< Vertical size, from 0 to 4094 pixels high */

/***************  Bit definition for DCMIPP_P1CMRICR register  ***************/
#define DCMIPP_P1CMRICR_ROILSZ_Pos          (0U)
#define DCMIPP_P1CMRICR_ROILSZ_Msk          (0x3UL << DCMIPP_P1CMRICR_ROILSZ_Pos)           /*!< 0x00000003 */
#define DCMIPP_P1CMRICR_ROILSZ              DCMIPP_P1CMRICR_ROILSZ_Msk                      /*!< Region of interest line size width */
#define DCMIPP_P1CMRICR_ROI1EN_Pos          (16U)
#define DCMIPP_P1CMRICR_ROI1EN_Msk          (0x1UL << DCMIPP_P1CMRICR_ROI1EN_Pos)           /*!< 0x00010000 */
#define DCMIPP_P1CMRICR_ROI1EN              DCMIPP_P1CMRICR_ROI1EN_Msk                      /*!< Region Of Interest 1 Enable */
#define DCMIPP_P1CMRICR_ROI2EN_Pos          (17U)
#define DCMIPP_P1CMRICR_ROI2EN_Msk          (0x1UL << DCMIPP_P1CMRICR_ROI2EN_Pos)           /*!< 0x00020000 */
#define DCMIPP_P1CMRICR_ROI2EN              DCMIPP_P1CMRICR_ROI2EN_Msk                      /*!< Region Of Interest 2 Enable */
#define DCMIPP_P1CMRICR_ROI3EN_Pos          (18U)
#define DCMIPP_P1CMRICR_ROI3EN_Msk          (0x1UL << DCMIPP_P1CMRICR_ROI3EN_Pos)           /*!< 0x00040000 */
#define DCMIPP_P1CMRICR_ROI3EN              DCMIPP_P1CMRICR_ROI3EN_Msk                      /*!< Region Of Interest 3 Enable */
#define DCMIPP_P1CMRICR_ROI4EN_Pos          (19U)
#define DCMIPP_P1CMRICR_ROI4EN_Msk          (0x1UL << DCMIPP_P1CMRICR_ROI4EN_Pos)           /*!< 0x00080000 */
#define DCMIPP_P1CMRICR_ROI4EN              DCMIPP_P1CMRICR_ROI4EN_Msk                      /*!< Region Of Interest 4 Enable */
#define DCMIPP_P1CMRICR_ROI5EN_Pos          (20U)
#define DCMIPP_P1CMRICR_ROI5EN_Msk          (0x1UL << DCMIPP_P1CMRICR_ROI5EN_Pos)           /*!< 0x00100000 */
#define DCMIPP_P1CMRICR_ROI5EN              DCMIPP_P1CMRICR_ROI5EN_Msk                      /*!< Region Of Interest 5 Enable */
#define DCMIPP_P1CMRICR_ROI6EN_Pos          (21U)
#define DCMIPP_P1CMRICR_ROI6EN_Msk          (0x1UL << DCMIPP_P1CMRICR_ROI6EN_Pos)           /*!< 0x00200000 */
#define DCMIPP_P1CMRICR_ROI6EN              DCMIPP_P1CMRICR_ROI6EN_Msk                      /*!< Region Of Interest 6 Enable */
#define DCMIPP_P1CMRICR_ROI7EN_Pos          (22U)
#define DCMIPP_P1CMRICR_ROI7EN_Msk          (0x1UL << DCMIPP_P1CMRICR_ROI7EN_Pos)           /*!< 0x00400000 */
#define DCMIPP_P1CMRICR_ROI7EN              DCMIPP_P1CMRICR_ROI7EN_Msk                      /*!< Region Of Interest 7 Enable */
#define DCMIPP_P1CMRICR_ROI8EN_Pos          (23U)
#define DCMIPP_P1CMRICR_ROI8EN_Msk          (0x1UL << DCMIPP_P1CMRICR_ROI8EN_Pos)           /*!< 0x00800000 */
#define DCMIPP_P1CMRICR_ROI8EN              DCMIPP_P1CMRICR_ROI8EN_Msk                      /*!< Region Of Interest 8 Enable */

/***************  Bit definition for DCMIPP_P1RIxCR1 register  ***************/
#define DCMIPP_P1RIxCR1_HSTART_Pos          (0U)
#define DCMIPP_P1RIxCR1_HSTART_Msk          (0xFFFUL << DCMIPP_P1RIxCR1_HSTART_Pos)        /*!< 0x00000FFF */
#define DCMIPP_P1RIxCR1_HSTART              DCMIPP_P1RIxCR1_HSTART_Msk                     /*!< Horizontal start */
#define DCMIPP_P1RIxCR1_CLB_Pos             (12U)
#define DCMIPP_P1RIxCR1_CLB_Msk             (0x3UL << DCMIPP_P1RIxCR1_CLB_Pos)             /*!< 0x00003000 */
#define DCMIPP_P1RIxCR1_CLB                 DCMIPP_P1RIxCR1_CLB_Msk                        /*!< Color line blue */
#define DCMIPP_P1RIxCR1_CLG_Pos             (14U)
#define DCMIPP_P1RIxCR1_CLG_Msk             (0x3UL << DCMIPP_P1RIxCR1_CLG_Pos)             /*!< 0x0000C000 */
#define DCMIPP_P1RIxCR1_CLG                 DCMIPP_P1RIxCR1_CLG_Msk                        /*!< Color line green */
#define DCMIPP_P1RIxCR1_VSTART_Pos          (16U)
#define DCMIPP_P1RIxCR1_VSTART_Msk          (0xFFFUL << DCMIPP_P1RIxCR1_VSTART_Pos)        /*!< 0x0FFF0000 */
#define DCMIPP_P1RIxCR1_VSTART              DCMIPP_P1RIxCR1_VSTART_Msk                     /*!< Vertical start */
#define DCMIPP_P1RIxCR1_CLR_Pos             (28U)
#define DCMIPP_P1RIxCR1_CLR_Msk             (0x3UL << DCMIPP_P1RIxCR1_CLR_Pos)             /*!< 0x30000000 */
#define DCMIPP_P1RIxCR1_CLR                 DCMIPP_P1RIxCR1_CLR_Msk                        /*!< Color line red */

/***************  Bit definition for DCMIPP_P1RIxCR2 register  ***************/
#define DCMIPP_P1RIxCR2_VSIZE_Pos           (0U)
#define DCMIPP_P1RIxCR2_VSIZE_Msk           (0xFFFUL << DCMIPP_P1RIxCR2_VSIZE_Pos)        /*!<  0x00000FFF */
#define DCMIPP_P1RIxCR2_VSIZE               DCMIPP_P1RIxCR2_VSIZE_Msk                     /*!< Vertical Size */
#define DCMIPP_P1RIxCR2_HSIZE_Pos           (16U)
#define DCMIPP_P1RIxCR2_HSIZE_Msk           (0xFFFUL << DCMIPP_P1RIxCR2_HSIZE_Pos)        /*!<  0x07FF8000 */
#define DCMIPP_P1RIxCR2_HSIZE               DCMIPP_P1RIxCR2_HSIZE_Msk                     /*!< Horizontal Size */

/****************  Bit definition for DCMIPP_P1GMCR register  *****************/
#define DCMIPP_P1GMCR_ENABLE_Pos            (0U)
#define DCMIPP_P1GMCR_ENABLE_Msk            (0x1UL << DCMIPP_P1GMCR_ENABLE_Pos)              /*!< 0x00000001   */
#define DCMIPP_P1GMCR_ENABLE                DCMIPP_P1GMCR_ENABLE_Msk                        /*!<  Gamma  enable*/

/****************  Bit definition for DCMIPP_P1YUVCR register  ****************/
#define DCMIPP_P1YUVCR_ENABLE_Pos           (0U)
#define DCMIPP_P1YUVCR_ENABLE_Msk           (0x1UL << DCMIPP_P1YUVCR_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P1YUVCR_ENABLE               DCMIPP_P1YUVCR_ENABLE_Msk                       /*!<  */
#define DCMIPP_P1YUVCR_TYPE_Pos             (1U)
#define DCMIPP_P1YUVCR_TYPE_Msk             (0x1UL << DCMIPP_P1YUVCR_TYPE_Pos)               /*!< 0x00000002 */
#define DCMIPP_P1YUVCR_TYPE                 DCMIPP_P1YUVCR_TYPE_Msk                         /*!< Output samples type used while CLAMP is activated */
#define DCMIPP_P1YUVCR_CLAMP_Pos            (2U)
#define DCMIPP_P1YUVCR_CLAMP_Msk            (0x1UL << DCMIPP_P1YUVCR_CLAMP_Pos)              /*!< 0x00000004 */
#define DCMIPP_P1YUVCR_CLAMP                DCMIPP_P1YUVCR_CLAMP_Msk                        /*!< Clamp the output samples */

/***************  Bit definition for DCMIPP_P1YUVRR1 register  ****************/
#define DCMIPP_P1YUVRR1_RR_Pos              (0U)
#define DCMIPP_P1YUVRR1_RR_Msk              (0x7FFUL << DCMIPP_P1YUVRR1_RR_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1YUVRR1_RR                  DCMIPP_P1YUVRR1_RR_Msk                          /*!< Coefficient row 1 column 1 of the matrix */
#define DCMIPP_P1YUVRR1_RG_Pos              (16U)
#define DCMIPP_P1YUVRR1_RG_Msk              (0x7FFUL << DCMIPP_P1YUVRR1_RG_Pos)              /*!< 0x07FF0000 */
#define DCMIPP_P1YUVRR1_RG                  DCMIPP_P1YUVRR1_RG_Msk                          /*!< Coefficient row 1 column 2 of the matrix */

/***************  Bit definition for DCMIPP_P1YUVRR2 register  ****************/
#define DCMIPP_P1YUVRR2_RB_Pos              (0U)
#define DCMIPP_P1YUVRR2_RB_Msk              (0x7FFUL << DCMIPP_P1YUVRR2_RB_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1YUVRR2_RB                  DCMIPP_P1YUVRR2_RB_Msk                          /*!< Coefficient row 1 column 3 of the matrix */
#define DCMIPP_P1YUVRR2_RA_Pos              (16U)
#define DCMIPP_P1YUVRR2_RA_Msk              (0x3FFUL << DCMIPP_P1YUVRR2_RA_Pos)              /*!< 0x03FF0000 */
#define DCMIPP_P1YUVRR2_RA                  DCMIPP_P1YUVRR2_RA_Msk                          /*!< Coefficient row 1 of the added column (signed integer value) */

/***************  Bit definition for DCMIPP_P1YUVGR1 register  ****************/
#define DCMIPP_P1YUVGR1_GR_Pos              (0U)
#define DCMIPP_P1YUVGR1_GR_Msk              (0x7FFUL << DCMIPP_P1YUVGR1_GR_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1YUVGR1_GR                  DCMIPP_P1YUVGR1_GR_Msk                          /*!< Coefficient row 2 column 1 of the matrix */
#define DCMIPP_P1YUVGR1_GG_Pos              (16U)
#define DCMIPP_P1YUVGR1_GG_Msk              (0x7FFUL << DCMIPP_P1YUVGR1_GG_Pos)              /*!< 0x07FF0000 */
#define DCMIPP_P1YUVGR1_GG                  DCMIPP_P1YUVGR1_GG_Msk                          /*!< Coefficient row 2 column 2 of the matrix */

/***************  Bit definition for DCMIPP_P1YUVGR2 register  ****************/
#define DCMIPP_P1YUVGR2_GB_Pos              (0U)
#define DCMIPP_P1YUVGR2_GB_Msk              (0x7FFUL << DCMIPP_P1YUVGR2_GB_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1YUVGR2_GB                  DCMIPP_P1YUVGR2_GB_Msk                          /*!< Coefficient row 2 column 3 of the matrix */
#define DCMIPP_P1YUVGR2_GA_Pos              (16U)
#define DCMIPP_P1YUVGR2_GA_Msk              (0x3FFUL << DCMIPP_P1YUVGR2_GA_Pos)              /*!< 0x03FF0000 */
#define DCMIPP_P1YUVGR2_GA                  DCMIPP_P1YUVGR2_GA_Msk                          /*!< Coefficient row 2 of the added column (signed integer value) */

/***************  Bit definition for DCMIPP_P1YUVBR1 register  ****************/
#define DCMIPP_P1YUVBR1_BR_Pos              (0U)
#define DCMIPP_P1YUVBR1_BR_Msk              (0x7FFUL << DCMIPP_P1YUVBR1_BR_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1YUVBR1_BR                  DCMIPP_P1YUVBR1_BR_Msk                          /*!< Coefficient row 3 column 1 of the matrix */
#define DCMIPP_P1YUVBR1_BG_Pos              (16U)
#define DCMIPP_P1YUVBR1_BG_Msk              (0x7FFUL << DCMIPP_P1YUVBR1_BG_Pos)              /*!< 0x07FF0000 */
#define DCMIPP_P1YUVBR1_BG                  DCMIPP_P1YUVBR1_BG_Msk                          /*!< Coefficient row 3 column 2 of the matrix */

/***************  Bit definition for DCMIPP_P1YUVBR2 register  ****************/
#define DCMIPP_P1YUVBR2_BB_Pos              (0U)
#define DCMIPP_P1YUVBR2_BB_Msk              (0x7FFUL << DCMIPP_P1YUVBR2_BB_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1YUVBR2_BB                  DCMIPP_P1YUVBR2_BB_Msk                          /*!< Coefficient row 3 column 3 of the matrix */
#define DCMIPP_P1YUVBR2_BA_Pos              (16U)
#define DCMIPP_P1YUVBR2_BA_Msk              (0x3FFUL << DCMIPP_P1YUVBR2_BA_Pos)              /*!< 0x03FF0000 */
#define DCMIPP_P1YUVBR2_BA                  DCMIPP_P1YUVBR2_BA_Msk                          /*!< Coefficient row 3 of the added column (signed integer value) */

/****************  Bit definition for DCMIPP_P1PPCR register  *****************/
#define DCMIPP_P1PPCR_FORMAT_Pos            (0U)
#define DCMIPP_P1PPCR_FORMAT_Msk            (0xFUL << DCMIPP_P1PPCR_FORMAT_Pos)              /*!< 0x0000000F */
#define DCMIPP_P1PPCR_FORMAT                DCMIPP_P1PPCR_FORMAT_Msk                        /*!< Memory format */
#define DCMIPP_P1PPCR_SWAPRB_Pos            (4U)
#define DCMIPP_P1PPCR_SWAPRB_Msk            (0x1UL << DCMIPP_P1PPCR_SWAPRB_Pos)              /*!< 0x00000010 */
#define DCMIPP_P1PPCR_SWAPRB                DCMIPP_P1PPCR_SWAPRB_Msk                        /*!< Swaps R-vs-B components if RGB, and U-vs-V components if YUV */
#define DCMIPP_P1PPCR_LINEMULT_Pos          (13U)
#define DCMIPP_P1PPCR_LINEMULT_Msk          (0x7UL << DCMIPP_P1PPCR_LINEMULT_Pos)            /*!< 0x0000E000 */
#define DCMIPP_P1PPCR_LINEMULT              DCMIPP_P1PPCR_LINEMULT_Msk                      /*!< Amount of capture completed lines for LINE Event and Interrupt */
#define DCMIPP_P1PPCR_DBM_Pos               (16U)
#define DCMIPP_P1PPCR_DBM_Msk               (0x1UL << DCMIPP_P1PPCR_DBM_Pos)                 /*!< 0x00010000 */
#define DCMIPP_P1PPCR_DBM                   DCMIPP_P1PPCR_DBM_Msk                           /*!< Double buffer mode */
#define DCMIPP_P1PPCR_LMAWM_Pos             (17U)
#define DCMIPP_P1PPCR_LMAWM_Msk             (0x7UL << DCMIPP_P1PPCR_LMAWM_Pos)                /*!< 0x000E0000 */
#define DCMIPP_P1PPCR_LMAWM                 DCMIPP_P1PPCR_LMAWM_Msk                          /*!< Line multi address wrapping modulo */
#define DCMIPP_P1PPCR_LMAWE_Pos             (20U)
#define DCMIPP_P1PPCR_LMAWE_Msk             (0x1UL << DCMIPP_P1PPCR_LMAWE_Pos)                /*!< 0x00100000 */
#define DCMIPP_P1PPCR_LMAWE                 DCMIPP_P1PPCR_LMAWE_Msk                           /*!< Line multi address wrapping enable */

/***************  Bit definition for DCMIPP_P1PPM0AR1 register  ***************/
#define DCMIPP_P1PPM0AR1_M0A_Pos            (0U)
#define DCMIPP_P1PPM0AR1_M0A_Msk            (0xFFFFFFFFUL << DCMIPP_P1PPM0AR1_M0A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P1PPM0AR1_M0A                DCMIPP_P1PPM0AR1_M0A_Msk                        /*!< Memory0 address register 1*/

/***************  Bit definition for DCMIPP_P1PPM0AR2 register  ***************/
#define DCMIPP_P1PPM0AR2_M0A_Pos            (0U)
#define DCMIPP_P1PPM0AR2_M0A_Msk            (0xFFFFFFFFUL << DCMIPP_P1PPM0AR2_M0A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P1PPM0AR2_M0A                DCMIPP_P1PPM0AR2_M0A_Msk                        /*!< Memory0 address register 2 */

/***************  Bit definition for DCMIPP_P1PPM0PR register  ****************/
#define DCMIPP_P1PPM0PR_PITCH_Pos           (0U)
#define DCMIPP_P1PPM0PR_PITCH_Msk           (0x7FFFUL << DCMIPP_P1PPM0PR_PITCH_Pos)          /*!< 0x00007FFF */
#define DCMIPP_P1PPM0PR_PITCH               DCMIPP_P1PPM0PR_PITCH_Msk                       /*!< Number of bytes between the address of two consecutive lines */

/***************  Bit definition for DCMIPP_P1PPM1AR1 register  ***************/
#define DCMIPP_P1PPM1AR1_M1A_Pos            (0U)
#define DCMIPP_P1PPM1AR1_M1A_Msk            (0xFFFFFFFFUL << DCMIPP_P1PPM1AR1_M1A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P1PPM1AR1_M1A                DCMIPP_P1PPM1AR1_M1A_Msk                        /*!< Memory1 address */

/***************  Bit definition for DCMIPP_P1PPM1AR2 register  ***************/
#define DCMIPP_P1PPM1AR2_M1A_Pos            (0U)
#define DCMIPP_P1PPM1AR2_M1A_Msk            (0xFFFFFFFFUL << DCMIPP_P1PPM1AR2_M1A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P1PPM1AR2_M1A                DCMIPP_P1PPM1AR2_M1A_Msk                        /*!< Memory1 address */

/***************  Bit definition for DCMIPP_P1PPM1PR register  ****************/
#define DCMIPP_P1PPM1PR_PITCH_Pos           (0U)
#define DCMIPP_P1PPM1PR_PITCH_Msk           (0x7FFFUL << DCMIPP_P1PPM1PR_PITCH_Pos)          /*!< 0x00007FFF */
#define DCMIPP_P1PPM1PR_PITCH               DCMIPP_P1PPM1PR_PITCH_Msk                       /*!< Number of bytes between the address of two consecutive lines */

/***************  Bit definition for DCMIPP_P1STM1AR register  ****************/
#define DCMIPP_P1STM1AR_M1A_Pos             (0U)
#define DCMIPP_P1STM1AR_M1A_Msk             (0x7FFFUL << DCMIPP_P1STM1AR_M1A_Pos)           /*!< 0xFFFFFFFF */
#define DCMIPP_P1STM1AR_M1A                 DCMIPP_P1STM1AR_M1A_Msk                         /*!< status Memory1 address register */

/***************  Bit definition for DCMIPP_P1PPM2AR1 register  ***************/
#define DCMIPP_P1PPM2AR1_M2A_Pos            (0U)
#define DCMIPP_P1PPM2AR1_M2A_Msk            (0xFFFFFFFFUL << DCMIPP_P1PPM2AR1_M2A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P1PPM2AR1_M2A                DCMIPP_P1PPM2AR1_M2A_Msk                        /*!< Memory2 address register 1*/

/***************  Bit definition for DCMIPP_P1PPM2AR2 register  ***************/
#define DCMIPP_P1PPM2AR2_M2A_Pos            (0U)
#define DCMIPP_P1PPM2AR2_M2A_Msk            (0xFFFFFFFFUL << DCMIPP_P1PPM2AR2_M2A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P1PPM2AR2_M2A                DCMIPP_P1PPM2AR2_M2A_Msk                        /*!< Memory2 address register 2 */

/***************  Bit definition for DCMIPP_P1STM2AR register  ****************/
#define DCMIPP_P1STM2AR_M2A_Pos             (0U)
#define DCMIPP_P1STM2AR_M2A_Msk             (0xFFFFFFFFUL << DCMIPP_P1STM2AR_M2A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P1STM2AR_M2A                 DCMIPP_P1STM2AR_M2A_Msk                         /*!< status Memory2 address register */

/*****************  Bit definition for DCMIPP_P1IER register  *****************/
#define DCMIPP_P1IER_LINEIE_Pos             (0U)
#define DCMIPP_P1IER_LINEIE_Msk             (0x1UL << DCMIPP_P1IER_LINEIE_Pos)               /*!< 0x00000001 */
#define DCMIPP_P1IER_LINEIE                 DCMIPP_P1IER_LINEIE_Msk                         /*!< Multi-line capture completed interrupt enable */
#define DCMIPP_P1IER_FRAMEIE_Pos            (1U)
#define DCMIPP_P1IER_FRAMEIE_Msk            (0x1UL << DCMIPP_P1IER_FRAMEIE_Pos)              /*!< 0x00000002 */
#define DCMIPP_P1IER_FRAMEIE                DCMIPP_P1IER_FRAMEIE_Msk                        /*!< Frame capture completed interrupt enable */
#define DCMIPP_P1IER_VSYNCIE_Pos            (2U)
#define DCMIPP_P1IER_VSYNCIE_Msk            (0x1UL << DCMIPP_P1IER_VSYNCIE_Pos)              /*!< 0x00000004 */
#define DCMIPP_P1IER_VSYNCIE                DCMIPP_P1IER_VSYNCIE_Msk                        /*!< VSYNC interrupt enable */
#define DCMIPP_P1IER_OVRIE_Pos              (7U)
#define DCMIPP_P1IER_OVRIE_Msk              (0x1UL << DCMIPP_P1IER_OVRIE_Pos)                /*!< 0x00000080 */
#define DCMIPP_P1IER_OVRIE                  DCMIPP_P1IER_OVRIE_Msk                          /*!< Overrun interrupt enable */

/*****************  Bit definition for DCMIPP_P1SR register  ******************/
#define DCMIPP_P1SR_LINEF_Pos               (0U)
#define DCMIPP_P1SR_LINEF_Msk               (0x1UL << DCMIPP_P1SR_LINEF_Pos)                 /*!< 0x00000001 */
#define DCMIPP_P1SR_LINEF                   DCMIPP_P1SR_LINEF_Msk                           /*!< Multi-line capture completed raw interrupt status */
#define DCMIPP_P1SR_FRAMEF_Pos              (1U)
#define DCMIPP_P1SR_FRAMEF_Msk              (0x1UL << DCMIPP_P1SR_FRAMEF_Pos)                /*!< 0x00000002 */
#define DCMIPP_P1SR_FRAMEF                  DCMIPP_P1SR_FRAMEF_Msk                          /*!< Frame capture completed raw interrupt status */
#define DCMIPP_P1SR_VSYNCF_Pos              (2U)
#define DCMIPP_P1SR_VSYNCF_Msk              (0x1UL << DCMIPP_P1SR_VSYNCF_Pos)                /*!< 0x00000004 */
#define DCMIPP_P1SR_VSYNCF                  DCMIPP_P1SR_VSYNCF_Msk                          /*!< VSYNC raw interrupt status */
#define DCMIPP_P1SR_OVRF_Pos                (7U)
#define DCMIPP_P1SR_OVRF_Msk                (0x1UL << DCMIPP_P1SR_OVRF_Pos)                  /*!< 0x00000080 */
#define DCMIPP_P1SR_OVRF                    DCMIPP_P1SR_OVRF_Msk                            /*!< Overrun raw interrupt status */
#define DCMIPP_P1SR_LSTLINE_Pos             (16U)
#define DCMIPP_P1SR_LSTLINE_Msk             (0x1UL << DCMIPP_P1SR_LSTLINE_Pos)               /*!< 0x00010000 */
#define DCMIPP_P1SR_LSTLINE                 DCMIPP_P1SR_LSTLINE_Msk                         /*!< Last line LSB bit, sampled at frame capture complete event */
#define DCMIPP_P1SR_LSTFRM_Pos              (17U)
#define DCMIPP_P1SR_LSTFRM_Msk              (0x1UL << DCMIPP_P1SR_LSTFRM_Pos)                /*!< 0x00020000 */
#define DCMIPP_P1SR_LSTFRM                  DCMIPP_P1SR_LSTFRM_Msk                          /*!< Last frame LSB bit, sampled at frame capture complete event */
#define DCMIPP_P1SR_CPTACT_Pos              (23U)
#define DCMIPP_P1SR_CPTACT_Msk              (0x1UL << DCMIPP_P1SR_CPTACT_Pos)                /*!< 0x00800000 */
#define DCMIPP_P1SR_CPTACT                  DCMIPP_P1SR_CPTACT_Msk                          /*!< Capture immediate status */

/*****************  Bit definition for DCMIPP_P1FCR register  *****************/
#define DCMIPP_P1FCR_CLINEF_Pos             (0U)
#define DCMIPP_P1FCR_CLINEF_Msk             (0x1UL << DCMIPP_P1FCR_CLINEF_Pos)               /*!< 0x00000001 */
#define DCMIPP_P1FCR_CLINEF                 DCMIPP_P1FCR_CLINEF_Msk                         /*!< Multi-line capture complete interrupt status clear */
#define DCMIPP_P1FCR_CFRAMEF_Pos            (1U)
#define DCMIPP_P1FCR_CFRAMEF_Msk            (0x1UL << DCMIPP_P1FCR_CFRAMEF_Pos)              /*!< 0x00000002 */
#define DCMIPP_P1FCR_CFRAMEF                DCMIPP_P1FCR_CFRAMEF_Msk                        /*!< Frame capture complete interrupt status clear */
#define DCMIPP_P1FCR_CVSYNCF_Pos            (2U)
#define DCMIPP_P1FCR_CVSYNCF_Msk            (0x1UL << DCMIPP_P1FCR_CVSYNCF_Pos)              /*!< 0x00000004 */
#define DCMIPP_P1FCR_CVSYNCF                DCMIPP_P1FCR_CVSYNCF_Msk                        /*!< Vertical synchronization interrupt status clear */
#define DCMIPP_P1FCR_COVRF_Pos              (7U)
#define DCMIPP_P1FCR_COVRF_Msk              (0x1UL << DCMIPP_P1FCR_COVRF_Pos)                /*!< 0x00000080 */
#define DCMIPP_P1FCR_COVRF                  DCMIPP_P1FCR_COVRF_Msk                          /*!< Overrun interrupt status clear */

/****************  Bit definition for DCMIPP_P1CFSCR register  ****************/
#define DCMIPP_P1CFSCR_DTIDA_Pos            (0U)
#define DCMIPP_P1CFSCR_DTIDA_Msk            (0x3FUL << DCMIPP_P1CFSCR_DTIDA_Pos)             /*!< 0x0000003F */
#define DCMIPP_P1CFSCR_DTIDA                DCMIPP_P1CFSCR_DTIDA_Msk                        /*!< Current Data type ID A */
#define DCMIPP_P1CFSCR_DTIDB_Pos            (8U)
#define DCMIPP_P1CFSCR_DTIDB_Msk            (0x3FUL << DCMIPP_P1CFSCR_DTIDB_Pos)             /*!< 0x00003F00 */
#define DCMIPP_P1CFSCR_DTIDB                DCMIPP_P1CFSCR_DTIDB_Msk                        /*!< Current Data type ID B */
#define DCMIPP_P1CFSCR_DTMODE_Pos           (16U)
#define DCMIPP_P1CFSCR_DTMODE_Msk           (0x3UL << DCMIPP_P1CFSCR_DTMODE_Pos)             /*!< 0x00030000 */
#define DCMIPP_P1CFSCR_DTMODE               DCMIPP_P1CFSCR_DTMODE_Msk                       /*!< Flow selection mode */
#define DCMIPP_P1CFSCR_PIPEDIFF_Pos         (18U)
#define DCMIPP_P1CFSCR_PIPEDIFF_Msk         (0x1UL << DCMIPP_P1CFSCR_PIPEDIFF_Pos)           /*!< 0x00040000 */
#define DCMIPP_P1CFSCR_PIPEDIFF             DCMIPP_P1CFSCR_PIPEDIFF_Msk                     /*!< Current differentiates Pipe2 vs */
#define DCMIPP_P1CFSCR_VC_Pos               (19U)
#define DCMIPP_P1CFSCR_VC_Msk               (0x3UL << DCMIPP_P1CFSCR_VC_Pos)                 /*!< 0x00180000 */
#define DCMIPP_P1CFSCR_VC                   DCMIPP_P1CFSCR_VC_Msk                           /*!< Current flow selection mode */
#define DCMIPP_P1CFSCR_FDTF_Pos             (24U)
#define DCMIPP_P1CFSCR_FDTF_Msk             (0x3FUL << DCMIPP_P1CFSCR_FDTF_Pos)              /*!< 0x3F000000 */
#define DCMIPP_P1CFSCR_FDTF                 DCMIPP_P1CFSCR_FDTF_Msk                         /*!< Current force Data type format */
#define DCMIPP_P1CFSCR_FDTFEN_Pos           (30U)
#define DCMIPP_P1CFSCR_FDTFEN_Msk           (0x1UL << DCMIPP_P1CFSCR_FDTFEN_Pos)             /*!< 0x40000000 */
#define DCMIPP_P1CFSCR_FDTFEN               DCMIPP_P1CFSCR_FDTFEN_Msk                       /*!< Current force Data type format enable */
#define DCMIPP_P1CFSCR_PIPEN_Pos            (31U)
#define DCMIPP_P1CFSCR_PIPEN_Msk            (0x1UL << DCMIPP_P1CFSCR_PIPEN_Pos)              /*!< 0x80000000 */
#define DCMIPP_P1CFSCR_PIPEN                DCMIPP_P1CFSCR_PIPEN_Msk                        /*!< Current activation of PipeN */

/***************  Bit definition for DCMIPP_P1CBPRCR register  ****************/
#define DCMIPP_P1CBPRCR_ENABLE_Pos          (0U)
#define DCMIPP_P1CBPRCR_ENABLE_Msk          (0x1UL << DCMIPP_P1CBPRCR_ENABLE_Pos)            /*!< 0x00000001 */
#define DCMIPP_P1CBPRCR_ENABLE              DCMIPP_P1CBPRCR_ENABLE_Msk                      /*!< Current status of enable bit */
#define DCMIPP_P1CBPRCR_STRENGTH_Pos        (1U)
#define DCMIPP_P1CBPRCR_STRENGTH_Msk        (0x7UL << DCMIPP_P1CBPRCR_STRENGTH_Pos)          /*!< 0x0000000E */
#define DCMIPP_P1CBPRCR_STRENGTH            DCMIPP_P1CBPRCR_STRENGTH_Msk                    /*!< Current strength (aggressivity) of the bad pixel detection: */

/***************  Bit definition for DCMIPP_P1CBLCCR register  ****************/
#define DCMIPP_P1CBLCCR_ENABLE_Pos          (0U)
#define DCMIPP_P1CBLCCR_ENABLE_Msk          (0x1UL << DCMIPP_P1CBLCCR_ENABLE_Pos)            /*!< 0x00000001 */
#define DCMIPP_P1CBLCCR_ENABLE              DCMIPP_P1CBLCCR_ENABLE_Msk                      /*!< For current black level calibration */
#define DCMIPP_P1CBLCCR_BLCB_Pos            (8U)
#define DCMIPP_P1CBLCCR_BLCB_Msk            (0xFFUL << DCMIPP_P1CBLCCR_BLCB_Pos)             /*!< 0x0000FF00 */
#define DCMIPP_P1CBLCCR_BLCB                DCMIPP_P1CBLCCR_BLCB_Msk                        /*!< Current black level calibration - Blue */
#define DCMIPP_P1CBLCCR_BLCG_Pos            (16U)
#define DCMIPP_P1CBLCCR_BLCG_Msk            (0xFFUL << DCMIPP_P1CBLCCR_BLCG_Pos)             /*!< 0x00FF0000 */
#define DCMIPP_P1CBLCCR_BLCG                DCMIPP_P1CBLCCR_BLCG_Msk                        /*!< Current black level calibration - Green */
#define DCMIPP_P1CBLCCR_BLCR_Pos            (24U)
#define DCMIPP_P1CBLCCR_BLCR_Msk            (0xFFUL << DCMIPP_P1CBLCCR_BLCR_Pos)             /*!< 0xFF000000 */
#define DCMIPP_P1CBLCCR_BLCR                DCMIPP_P1CBLCCR_BLCR_Msk                        /*!< Current black level calibration - Red */

/***************  Bit definition for DCMIPP_P1CEXCR1 register  ****************/
#define DCMIPP_P1CEXCR1_ENABLE_Pos          (0U)
#define DCMIPP_P1CEXCR1_ENABLE_Msk          (0x1UL << DCMIPP_P1CEXCR1_ENABLE_Pos)            /*!< 0x00000001 */
#define DCMIPP_P1CEXCR1_ENABLE              DCMIPP_P1CEXCR1_ENABLE_Msk                      /*!< for exposure control (multiplication and shift) */
#define DCMIPP_P1CEXCR1_MULTR_Pos           (20U)
#define DCMIPP_P1CEXCR1_MULTR_Msk           (0xFFUL << DCMIPP_P1CEXCR1_MULTR_Pos)            /*!< 0x0FF00000 */
#define DCMIPP_P1CEXCR1_MULTR               DCMIPP_P1CEXCR1_MULTR_Msk                       /*!< Current exposure multiplier - Red */
#define DCMIPP_P1CEXCR1_SHFR_Pos            (28U)
#define DCMIPP_P1CEXCR1_SHFR_Msk            (0x7UL << DCMIPP_P1CEXCR1_SHFR_Pos)              /*!< 0x70000000 */
#define DCMIPP_P1CEXCR1_SHFR                DCMIPP_P1CEXCR1_SHFR_Msk                        /*!< Current exposure shift - Red */

/***************  Bit definition for DCMIPP_P1CEXCR2 register  ****************/
#define DCMIPP_P1CEXCR2_MULTB_Pos           (4U)
#define DCMIPP_P1CEXCR2_MULTB_Msk           (0xFFUL << DCMIPP_P1CEXCR2_MULTB_Pos)            /*!< 0x00000FF0 */
#define DCMIPP_P1CEXCR2_MULTB               DCMIPP_P1CEXCR2_MULTB_Msk                       /*!< Current exposure multiplier - Blue */
#define DCMIPP_P1CEXCR2_SHFB_Pos            (12U)
#define DCMIPP_P1CEXCR2_SHFB_Msk            (0x7UL << DCMIPP_P1CEXCR2_SHFB_Pos)              /*!< 0x00007000 */
#define DCMIPP_P1CEXCR2_SHFB                DCMIPP_P1CEXCR2_SHFB_Msk                        /*!< Current exposure shift - Blue */
#define DCMIPP_P1CEXCR2_MULTG_Pos           (20U)
#define DCMIPP_P1CEXCR2_MULTG_Msk           (0xFFUL << DCMIPP_P1CEXCR2_MULTG_Pos)            /*!< 0x0FF00000 */
#define DCMIPP_P1CEXCR2_MULTG               DCMIPP_P1CEXCR2_MULTG_Msk                       /*!< Current exposure multiplier - Green */
#define DCMIPP_P1CEXCR2_SHFG_Pos            (28U)
#define DCMIPP_P1CEXCR2_SHFG_Msk            (0x7UL << DCMIPP_P1CEXCR2_SHFG_Pos)              /*!< 0x70000000 */
#define DCMIPP_P1CEXCR2_SHFG                DCMIPP_P1CEXCR2_SHFG_Msk                        /*!< Current exposure shift - Green */

/***************  Bit definition for DCMIPP_P1CST1CR register  ****************/
#define DCMIPP_P1CST1CR_ENABLE_Pos          (0U)
#define DCMIPP_P1CST1CR_ENABLE_Msk          (0x1UL << DCMIPP_P1CST1CR_ENABLE_Pos)            /*!< 0x00000001 */
#define DCMIPP_P1CST1CR_ENABLE              DCMIPP_P1CST1CR_ENABLE_Msk                      /*!< Current enable bit value */
#define DCMIPP_P1CST1CR_BINS_Pos            (2U)
#define DCMIPP_P1CST1CR_BINS_Msk            (0x3UL << DCMIPP_P1CST1CR_BINS_Pos)              /*!< 0x0000000C */
#define DCMIPP_P1CST1CR_BINS                DCMIPP_P1CST1CR_BINS_Msk                        /*!< Current bin definition */
#define DCMIPP_P1CST1CR_SRC_Pos             (4U)
#define DCMIPP_P1CST1CR_SRC_Msk             (0x7UL << DCMIPP_P1CST1CR_SRC_Pos)               /*!< 0x00000070 */
#define DCMIPP_P1CST1CR_SRC                 DCMIPP_P1CST1CR_SRC_Msk                         /*!< Current source of statistics */
#define DCMIPP_P1CST1CR_MODE_Pos            (7U)
#define DCMIPP_P1CST1CR_MODE_Msk            (0x1UL << DCMIPP_P1CST1CR_MODE_Pos)              /*!< 0x00000080 */
#define DCMIPP_P1CST1CR_MODE                DCMIPP_P1CST1CR_MODE_Msk                        /*!< Current statistics mode */
#define DCMIPP_P1CST1CR_ACCU_Pos            (8U)
#define DCMIPP_P1CST1CR_ACCU_Msk            (0xFFFFFFUL << DCMIPP_P1CST1CR_ACCU_Pos)         /*!< 0xFFFFFF00 */
#define DCMIPP_P1CST1CR_ACCU                DCMIPP_P1CST1CR_ACCU_Msk                        /*!< Current accumulation result, divided by 256 */

/***************  Bit definition for DCMIPP_P1CST2CR register  ****************/
#define DCMIPP_P1CST2CR_ENABLE_Pos          (0U)
#define DCMIPP_P1CST2CR_ENABLE_Msk          (0x1UL << DCMIPP_P1CST2CR_ENABLE_Pos)            /*!< 0x00000001 */
#define DCMIPP_P1CST2CR_ENABLE              DCMIPP_P1CST2CR_ENABLE_Msk                      /*!<  */
#define DCMIPP_P1CST2CR_BINS_Pos            (2U)
#define DCMIPP_P1CST2CR_BINS_Msk            (0x3UL << DCMIPP_P1CST2CR_BINS_Pos)              /*!< 0x0000000C */
#define DCMIPP_P1CST2CR_BINS                DCMIPP_P1CST2CR_BINS_Msk                        /*!< Bin definition */
#define DCMIPP_P1CST2CR_SRC_Pos             (4U)
#define DCMIPP_P1CST2CR_SRC_Msk             (0x7UL << DCMIPP_P1CST2CR_SRC_Pos)               /*!< 0x00000070 */
#define DCMIPP_P1CST2CR_SRC                 DCMIPP_P1CST2CR_SRC_Msk                         /*!< source of stat */
#define DCMIPP_P1CST2CR_MODE_Pos            (7U)
#define DCMIPP_P1CST2CR_MODE_Msk            (0x1UL << DCMIPP_P1CST2CR_MODE_Pos)              /*!< 0x00000080 */
#define DCMIPP_P1CST2CR_MODE                DCMIPP_P1CST2CR_MODE_Msk                        /*!< statistics mode */
#define DCMIPP_P1CST2CR_ACCU_Pos            (8U)
#define DCMIPP_P1CST2CR_ACCU_Msk            (0xFFFFFFUL << DCMIPP_P1CST2CR_ACCU_Pos)         /*!< 0xFFFFFF00 */
#define DCMIPP_P1CST2CR_ACCU                DCMIPP_P1CST2CR_ACCU_Msk                        /*!< Accumulation result, divided by 256 */

/***************  Bit definition for DCMIPP_P1CST3CR register  ****************/
#define DCMIPP_P1CST3CR_ENABLE_Pos          (0U)
#define DCMIPP_P1CST3CR_ENABLE_Msk          (0x1UL << DCMIPP_P1CST3CR_ENABLE_Pos)            /*!< 0x00000001 */
#define DCMIPP_P1CST3CR_ENABLE              DCMIPP_P1CST3CR_ENABLE_Msk                      /*!<  */
#define DCMIPP_P1CST3CR_BINS_Pos            (2U)
#define DCMIPP_P1CST3CR_BINS_Msk            (0x3UL << DCMIPP_P1CST3CR_BINS_Pos)              /*!< 0x0000000C */
#define DCMIPP_P1CST3CR_BINS                DCMIPP_P1CST3CR_BINS_Msk                        /*!< Bin definition */
#define DCMIPP_P1CST3CR_SRC_Pos             (4U)
#define DCMIPP_P1CST3CR_SRC_Msk             (0x7UL << DCMIPP_P1CST3CR_SRC_Pos)               /*!< 0x00000070 */
#define DCMIPP_P1CST3CR_SRC                 DCMIPP_P1CST3CR_SRC_Msk                         /*!< Statistics source */
#define DCMIPP_P1CST3CR_MODE_Pos            (7U)
#define DCMIPP_P1CST3CR_MODE_Msk            (0x1UL << DCMIPP_P1CST3CR_MODE_Pos)              /*!< 0x00000080 */
#define DCMIPP_P1CST3CR_MODE                DCMIPP_P1CST3CR_MODE_Msk                        /*!< Statistics mode */
#define DCMIPP_P1CST3CR_ACCU_Pos            (8U)
#define DCMIPP_P1CST3CR_ACCU_Msk            (0xFFFFFFUL << DCMIPP_P1CST3CR_ACCU_Pos)         /*!< 0xFFFFFF00 */
#define DCMIPP_P1CST3CR_ACCU                DCMIPP_P1CST3CR_ACCU_Msk                        /*!< Accumulation result, divided by 256 */

/***************  Bit definition for DCMIPP_P1CSTSTR register  ****************/
#define DCMIPP_P1CSTSTR_HSTART_Pos          (0U)
#define DCMIPP_P1CSTSTR_HSTART_Msk          (0xFFFUL << DCMIPP_P1CSTSTR_HSTART_Pos)          /*!< 0x00000FFF */
#define DCMIPP_P1CSTSTR_HSTART              DCMIPP_P1CSTSTR_HSTART_Msk                      /*!< Current horizontal start, from 0 to 4094 pixels wide */
#define DCMIPP_P1CSTSTR_VSTART_Pos          (16U)
#define DCMIPP_P1CSTSTR_VSTART_Msk          (0xFFFUL << DCMIPP_P1CSTSTR_VSTART_Pos)          /*!< 0x0FFF0000 */
#define DCMIPP_P1CSTSTR_VSTART              DCMIPP_P1CSTSTR_VSTART_Msk                      /*!< Current vertical start, from 0 to 4094 pixels high */

/***************  Bit definition for DCMIPP_P1CSTSZR register  ****************/
#define DCMIPP_P1CSTSZR_HSIZE_Pos           (0U)
#define DCMIPP_P1CSTSZR_HSIZE_Msk           (0xFFFUL << DCMIPP_P1CSTSZR_HSIZE_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P1CSTSZR_HSIZE               DCMIPP_P1CSTSZR_HSIZE_Msk                       /*!< Current horizontal size, from 0 to 4094 pixels wide */
#define DCMIPP_P1CSTSZR_VSIZE_Pos           (16U)
#define DCMIPP_P1CSTSZR_VSIZE_Msk           (0xFFFUL << DCMIPP_P1CSTSZR_VSIZE_Pos)           /*!< 0x0FFF0000 */
#define DCMIPP_P1CSTSZR_VSIZE               DCMIPP_P1CSTSZR_VSIZE_Msk                       /*!< Current vertical size, from 0 to 4094 pixels high */
#define DCMIPP_P1CSTSZR_CROPEN_Pos          (31U)
#define DCMIPP_P1CSTSZR_CROPEN_Msk          (0x1UL << DCMIPP_P1CSTSZR_CROPEN_Pos)            /*!< 0x80000000 */
#define DCMIPP_P1CSTSZR_CROPEN              DCMIPP_P1CSTSZR_CROPEN_Msk                      /*!< Current CROPEN bit value */

/****************  Bit definition for DCMIPP_P1CCCCR register  ****************/
#define DCMIPP_P1CCCCR_ENABLE_Pos           (0U)
#define DCMIPP_P1CCCCR_ENABLE_Msk           (0x1UL << DCMIPP_P1CCCCR_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P1CCCCR_ENABLE               DCMIPP_P1CCCCR_ENABLE_Msk                       /*!< This bit indicates the current value applied */
#define DCMIPP_P1CCCCR_TYPE_Pos             (1U)
#define DCMIPP_P1CCCCR_TYPE_Msk             (0x1UL << DCMIPP_P1CCCCR_TYPE_Pos)               /*!< 0x00000002 */
#define DCMIPP_P1CCCCR_TYPE                 DCMIPP_P1CCCCR_TYPE_Msk                         /*!< output samples type used while CLAMP is activated */
#define DCMIPP_P1CCCCR_CLAMP_Pos            (2U)
#define DCMIPP_P1CCCCR_CLAMP_Msk            (0x1UL << DCMIPP_P1CCCCR_CLAMP_Pos)              /*!< 0x00000004 */
#define DCMIPP_P1CCCCR_CLAMP                DCMIPP_P1CCCCR_CLAMP_Msk                        /*!< Clamp the output samples */

/***************  Bit definition for DCMIPP_P1CCCRR1 register  ****************/
#define DCMIPP_P1CCCRR1_RR_Pos              (0U)
#define DCMIPP_P1CCCRR1_RR_Msk              (0x7FFUL << DCMIPP_P1CCCRR1_RR_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1CCCRR1_RR                  DCMIPP_P1CCCRR1_RR_Msk                          /*!< Current coefficient row 1 column 1 of the matrix */
#define DCMIPP_P1CCCRR1_RG_Pos              (16U)
#define DCMIPP_P1CCCRR1_RG_Msk              (0x7FFUL << DCMIPP_P1CCCRR1_RG_Pos)              /*!< 0x07FF0000 */
#define DCMIPP_P1CCCRR1_RG                  DCMIPP_P1CCCRR1_RG_Msk                          /*!< Current coefficient row 1 column 2 of the matrix */

/***************  Bit definition for DCMIPP_P1CCCRR2 register  ****************/
#define DCMIPP_P1CCCRR2_RB_Pos              (0U)
#define DCMIPP_P1CCCRR2_RB_Msk              (0x7FFUL << DCMIPP_P1CCCRR2_RB_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1CCCRR2_RB                  DCMIPP_P1CCCRR2_RB_Msk                          /*!< Current coefficient row 1 column 3 of the matrix */
#define DCMIPP_P1CCCRR2_RA_Pos              (16U)
#define DCMIPP_P1CCCRR2_RA_Msk              (0x3FFUL << DCMIPP_P1CCCRR2_RA_Pos)              /*!< 0x03FF0000 */
#define DCMIPP_P1CCCRR2_RA                  DCMIPP_P1CCCRR2_RA_Msk                          /*!< Current coefficient row 1 of the added column (signed integer value) */

/***************  Bit definition for DCMIPP_P1CCCGR1 register  ****************/
#define DCMIPP_P1CCCGR1_GR_Pos              (0U)
#define DCMIPP_P1CCCGR1_GR_Msk              (0x7FFUL << DCMIPP_P1CCCGR1_GR_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1CCCGR1_GR                  DCMIPP_P1CCCGR1_GR_Msk                          /*!< Current coefficient row 2 column 1 of the matrix */
#define DCMIPP_P1CCCGR1_GG_Pos              (16U)
#define DCMIPP_P1CCCGR1_GG_Msk              (0x7FFUL << DCMIPP_P1CCCGR1_GG_Pos)              /*!< 0x07FF0000 */
#define DCMIPP_P1CCCGR1_GG                  DCMIPP_P1CCCGR1_GG_Msk                          /*!< Current coefficient row 2 column 2 of the matrix */

/***************  Bit definition for DCMIPP_P1CCCGR2 register  ****************/
#define DCMIPP_P1CCCGR2_GB_Pos              (0U)
#define DCMIPP_P1CCCGR2_GB_Msk              (0x7FFUL << DCMIPP_P1CCCGR2_GB_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1CCCGR2_GB                  DCMIPP_P1CCCGR2_GB_Msk                          /*!< Current coefficient row 2 column 3 of the matrix */
#define DCMIPP_P1CCCGR2_GA_Pos              (16U)
#define DCMIPP_P1CCCGR2_GA_Msk              (0x3FFUL << DCMIPP_P1CCCGR2_GA_Pos)              /*!< 0x03FF0000 */
#define DCMIPP_P1CCCGR2_GA                  DCMIPP_P1CCCGR2_GA_Msk                          /*!< Current coefficient row 2 of the added column (signed integer value) */

/***************  Bit definition for DCMIPP_P1CCCBR1 register  ****************/
#define DCMIPP_P1CCCBR1_BR_Pos              (0U)
#define DCMIPP_P1CCCBR1_BR_Msk              (0x7FFUL << DCMIPP_P1CCCBR1_BR_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1CCCBR1_BR                  DCMIPP_P1CCCBR1_BR_Msk                          /*!< Current coefficient row 3 column 1 of the matrix */
#define DCMIPP_P1CCCBR1_BG_Pos              (16U)
#define DCMIPP_P1CCCBR1_BG_Msk              (0x7FFUL << DCMIPP_P1CCCBR1_BG_Pos)              /*!< 0x07FF0000 */
#define DCMIPP_P1CCCBR1_BG                  DCMIPP_P1CCCBR1_BG_Msk                          /*!< Current coefficient row 3 column 2 of the matrix */

/***************  Bit definition for DCMIPP_P1CCCBR2 register  ****************/
#define DCMIPP_P1CCCBR2_BB_Pos              (0U)
#define DCMIPP_P1CCCBR2_BB_Msk              (0x7FFUL << DCMIPP_P1CCCBR2_BB_Pos)              /*!< 0x000007FF */
#define DCMIPP_P1CCCBR2_BB                  DCMIPP_P1CCCBR2_BB_Msk                          /*!< Current coefficient row 3 column 3 of the matrix */
#define DCMIPP_P1CCCBR2_BA_Pos              (16U)
#define DCMIPP_P1CCCBR2_BA_Msk              (0x3FFUL << DCMIPP_P1CCCBR2_BA_Pos)              /*!< 0x03FF0000 */
#define DCMIPP_P1CCCBR2_BA                  DCMIPP_P1CCCBR2_BA_Msk                          /*!< Current coefficient row 3 of the added column (signed integer value) */

/***************  Bit definition for DCMIPP_P1CCTCR1 register  ****************/
#define DCMIPP_P1CCTCR1_ENABLE_Pos          (0U)
#define DCMIPP_P1CCTCR1_ENABLE_Msk          (0x1UL << DCMIPP_P1CCTCR1_ENABLE_Pos)            /*!< 0x00000001 */
#define DCMIPP_P1CCTCR1_ENABLE              DCMIPP_P1CCTCR1_ENABLE_Msk                      /*!< Current ENABLE bit value */
#define DCMIPP_P1CCTCR1_LUM0_Pos            (9U)
#define DCMIPP_P1CCTCR1_LUM0_Msk            (0x3FUL << DCMIPP_P1CCTCR1_LUM0_Pos)             /*!< 0x00007E00 */
#define DCMIPP_P1CCTCR1_LUM0                DCMIPP_P1CCTCR1_LUM0_Msk                        /*!< Current luminance increase for input luminance of 0 (increase is idle with LUMx = 16) */

/***************  Bit definition for DCMIPP_P1CCTCR2 register  ****************/
#define DCMIPP_P1CCTCR2_LUM4_Pos            (1U)
#define DCMIPP_P1CCTCR2_LUM4_Msk            (0x3FUL << DCMIPP_P1CCTCR2_LUM4_Pos)             /*!< 0x0000007E */
#define DCMIPP_P1CCTCR2_LUM4                DCMIPP_P1CCTCR2_LUM4_Msk                        /*!< Current luminance increase for input luminance of 128 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CCTCR2_LUM3_Pos            (9U)
#define DCMIPP_P1CCTCR2_LUM3_Msk            (0x3FUL << DCMIPP_P1CCTCR2_LUM3_Pos)             /*!< 0x00007E00 */
#define DCMIPP_P1CCTCR2_LUM3                DCMIPP_P1CCTCR2_LUM3_Msk                        /*!< Current luminance increase for input luminance of 96 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CCTCR2_LUM2_Pos            (17U)
#define DCMIPP_P1CCTCR2_LUM2_Msk            (0x3FUL << DCMIPP_P1CCTCR2_LUM2_Pos)             /*!< 0x007E0000 */
#define DCMIPP_P1CCTCR2_LUM2                DCMIPP_P1CCTCR2_LUM2_Msk                        /*!< Current luminance increase for input luminance of 64 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CCTCR2_LUM1_Pos            (25U)
#define DCMIPP_P1CCTCR2_LUM1_Msk            (0x3FUL << DCMIPP_P1CCTCR2_LUM1_Pos)             /*!< 0x7E000000 */
#define DCMIPP_P1CCTCR2_LUM1                DCMIPP_P1CCTCR2_LUM1_Msk                        /*!< Current luminance increase for input luminance of 32 (increase is idle with LUMx = 16) */

/***************  Bit definition for DCMIPP_P1CCTCR3 register  ****************/
#define DCMIPP_P1CCTCR3_LUM8_Pos            (1U)
#define DCMIPP_P1CCTCR3_LUM8_Msk            (0x3FUL << DCMIPP_P1CCTCR3_LUM8_Pos)             /*!< 0x0000007E */
#define DCMIPP_P1CCTCR3_LUM8                DCMIPP_P1CCTCR3_LUM8_Msk                        /*!< Luminance increase for input luminance of 256 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CCTCR3_LUM7_Pos            (9U)
#define DCMIPP_P1CCTCR3_LUM7_Msk            (0x3FUL << DCMIPP_P1CCTCR3_LUM7_Pos)             /*!< 0x00007E00 */
#define DCMIPP_P1CCTCR3_LUM7                DCMIPP_P1CCTCR3_LUM7_Msk                        /*!< Luminance increase for input luminance of 224 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CCTCR3_LUM6_Pos            (17U)
#define DCMIPP_P1CCTCR3_LUM6_Msk            (0x3FUL << DCMIPP_P1CCTCR3_LUM6_Pos)             /*!< 0x007E0000 */
#define DCMIPP_P1CCTCR3_LUM6                DCMIPP_P1CCTCR3_LUM6_Msk                        /*!< Luminance increase for input luminance of 192 (increase is idle with LUMx = 16) */
#define DCMIPP_P1CCTCR3_LUM5_Pos            (25U)
#define DCMIPP_P1CCTCR3_LUM5_Msk            (0x3FUL << DCMIPP_P1CCTCR3_LUM5_Pos)             /*!< 0x7E000000 */
#define DCMIPP_P1CCTCR3_LUM5                DCMIPP_P1CCTCR3_LUM5_Msk                        /*!< Luminance increase for input luminance of 160 (increase is idle with LUMx = 16) */

/***************  Bit definition for DCMIPP_P1CFCTCR register  ****************/
#define DCMIPP_P1CFCTCR_FRATE_Pos           (0U)
#define DCMIPP_P1CFCTCR_FRATE_Msk           (0x3UL << DCMIPP_P1CFCTCR_FRATE_Pos)             /*!< 0x00000003 */
#define DCMIPP_P1CFCTCR_FRATE               DCMIPP_P1CFCTCR_FRATE_Msk                       /*!< Frame capture rate control */
#define DCMIPP_P1CFCTCR_CPTMODE_Pos         (2U)
#define DCMIPP_P1CFCTCR_CPTMODE_Msk         (0x1UL << DCMIPP_P1CFCTCR_CPTMODE_Pos)           /*!< 0x00000004 */
#define DCMIPP_P1CFCTCR_CPTMODE             DCMIPP_P1CFCTCR_CPTMODE_Msk                     /*!< Capture mode */
#define DCMIPP_P1CFCTCR_CPTREQ_Pos          (3U)
#define DCMIPP_P1CFCTCR_CPTREQ_Msk          (0x1UL << DCMIPP_P1CFCTCR_CPTREQ_Pos)            /*!< 0x00000008 */
#define DCMIPP_P1CFCTCR_CPTREQ              DCMIPP_P1CFCTCR_CPTREQ_Msk                      /*!< Capture requested */

/***************  Bit definition for DCMIPP_P1CCRSTR register  ****************/
#define DCMIPP_P1CCRSTR_HSTART_Pos          (0U)
#define DCMIPP_P1CCRSTR_HSTART_Msk          (0xFFFUL << DCMIPP_P1CCRSTR_HSTART_Pos)          /*!< 0x00000FFF */
#define DCMIPP_P1CCRSTR_HSTART              DCMIPP_P1CCRSTR_HSTART_Msk                      /*!< Current horizontal start, from 0 to 4094 pixels wide */
#define DCMIPP_P1CCRSTR_VSTART_Pos          (16U)
#define DCMIPP_P1CCRSTR_VSTART_Msk          (0xFFFUL << DCMIPP_P1CCRSTR_VSTART_Pos)          /*!< 0x0FFF0000 */
#define DCMIPP_P1CCRSTR_VSTART              DCMIPP_P1CCRSTR_VSTART_Msk                      /*!< Current vertical start, from 0 to 4094 pixels high */

/***************  Bit definition for DCMIPP_P1CCRSZR register  ****************/
#define DCMIPP_P1CCRSZR_HSIZE_Pos           (0U)
#define DCMIPP_P1CCRSZR_HSIZE_Msk           (0xFFFUL << DCMIPP_P1CCRSZR_HSIZE_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P1CCRSZR_HSIZE               DCMIPP_P1CCRSZR_HSIZE_Msk                       /*!< Current horizontal size, from 0 to 4094 pixels wide */
#define DCMIPP_P1CCRSZR_VSIZE_Pos           (16U)
#define DCMIPP_P1CCRSZR_VSIZE_Msk           (0xFFFUL << DCMIPP_P1CCRSZR_VSIZE_Pos)           /*!< 0x0FFF0000 */
#define DCMIPP_P1CCRSZR_VSIZE               DCMIPP_P1CCRSZR_VSIZE_Msk                       /*!< Current vertical size, from 0 to 4094 pixels high */
#define DCMIPP_P1CCRSZR_ENABLE_Pos          (31U)
#define DCMIPP_P1CCRSZR_ENABLE_Msk          (0x1UL << DCMIPP_P1CCRSZR_ENABLE_Pos)            /*!< 0x80000000 */
#define DCMIPP_P1CCRSZR_ENABLE              DCMIPP_P1CCRSZR_ENABLE_Msk                      /*!< Current ENABLE bit value */

/****************  Bit definition for DCMIPP_P1CDCCR register  *****************/
#define DCMIPP_P1CDCCR_ENABLE_Pos           (0U)
#define DCMIPP_P1CDCCR_ENABLE_Msk           (0x1UL << DCMIPP_P1CDCCR_ENABLE_Pos)            /*!< 0x00000001 */
#define DCMIPP_P1CDCCR_ENABLE               DCMIPP_P1CDCCR_ENABLE_Msk                       /*!< Decimation enable */
#define DCMIPP_P1CDCCR_HDEC_Pos             (1U)
#define DCMIPP_P1CDCCR_HDEC_Msk             (0x3UL << DCMIPP_P1CDCCR_HDEC_Pos)               /*!< 0x00000006 */
#define DCMIPP_P1CDCCR_HDEC                 DCMIPP_P1CDCCR_HDEC_Msk                         /*!< Horizontal decimation ratio */
#define DCMIPP_P1CDCCR_VDEC_Pos             (3U)
#define DCMIPP_P1CDCCR_VDEC_Msk             (0x3UL << DCMIPP_P1CDCCR_VDEC_Pos)               /*!< 0x00000018 */
#define DCMIPP_P1CDCCR_VDEC                 DCMIPP_P1CDCCR_VDEC_Msk                         /*!< Vertical decimation ratio */

/****************  Bit definition for DCMIPP_P1CDSCR register  ****************/
#define DCMIPP_P1CDSCR_HDIV_Pos             (0U)
#define DCMIPP_P1CDSCR_HDIV_Msk             (0x3FFUL << DCMIPP_P1CDSCR_HDIV_Pos)             /*!< 0x000003FF */
#define DCMIPP_P1CDSCR_HDIV                 DCMIPP_P1CDSCR_HDIV_Msk                         /*!< Current horizontal division factor, from 128 (8x) to 1023 (1x) */
#define DCMIPP_P1CDSCR_VDIV_Pos             (16U)
#define DCMIPP_P1CDSCR_VDIV_Msk             (0x3FFUL << DCMIPP_P1CDSCR_VDIV_Pos)             /*!< 0x03FF0000 */
#define DCMIPP_P1CDSCR_VDIV                 DCMIPP_P1CDSCR_VDIV_Msk                         /*!< Current vertical division factor, from 128 (8x) to 1023 (1x) */
#define DCMIPP_P1CDSCR_ENABLE_Pos           (31U)
#define DCMIPP_P1CDSCR_ENABLE_Msk           (0x1UL << DCMIPP_P1CDSCR_ENABLE_Pos)             /*!< 0x80000000 */
#define DCMIPP_P1CDSCR_ENABLE               DCMIPP_P1CDSCR_ENABLE_Msk                       /*!< Current value of the bit ENABLE */

/**************  Bit definition for DCMIPP_P1CDSRTIOR register  ***************/
#define DCMIPP_P1CDSRTIOR_HRATIO_Pos        (0U)
#define DCMIPP_P1CDSRTIOR_HRATIO_Msk        (0xFFFFUL << DCMIPP_P1CDSRTIOR_HRATIO_Pos)       /*!< 0x0000FFFF */
#define DCMIPP_P1CDSRTIOR_HRATIO            DCMIPP_P1CDSRTIOR_HRATIO_Msk                    /*!< Current horizontal ratio, from 8192 (1x) to 65535 (8x) */
#define DCMIPP_P1CDSRTIOR_VRATIO_Pos        (16U)
#define DCMIPP_P1CDSRTIOR_VRATIO_Msk        (0xFFFFUL << DCMIPP_P1CDSRTIOR_VRATIO_Pos)       /*!< 0xFFFF0000 */
#define DCMIPP_P1CDSRTIOR_VRATIO            DCMIPP_P1CDSRTIOR_VRATIO_Msk                    /*!< Current vertical ratio, from 8192 (1x) to 65535 (8x) */

/***************  Bit definition for DCMIPP_P1CDSSZR register  ****************/
#define DCMIPP_P1CDSSZR_HSIZE_Pos           (0U)
#define DCMIPP_P1CDSSZR_HSIZE_Msk           (0xFFFUL << DCMIPP_P1CDSSZR_HSIZE_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P1CDSSZR_HSIZE               DCMIPP_P1CDSSZR_HSIZE_Msk                       /*!< Current horizontal size, from 0 to 4094 pixels wide */
#define DCMIPP_P1CDSSZR_VSIZE_Pos           (16U)
#define DCMIPP_P1CDSSZR_VSIZE_Msk           (0xFFFUL << DCMIPP_P1CDSSZR_VSIZE_Pos)           /*!< 0x0FFF0000 */
#define DCMIPP_P1CDSSZR_VSIZE               DCMIPP_P1CDSSZR_VSIZE_Msk                       /*!< Current vertical size, from 0 to 4094 pixels high */

/****************  Bit definition for DCMIPP_P1CPPCR register  ****************/
#define DCMIPP_P1CPPCR_FORMAT_Pos           (0U)
#define DCMIPP_P1CPPCR_FORMAT_Msk           (0xFUL << DCMIPP_P1CPPCR_FORMAT_Pos)             /*!< 0x0000000F */
#define DCMIPP_P1CPPCR_FORMAT               DCMIPP_P1CPPCR_FORMAT_Msk                       /*!< Memory format */
#define DCMIPP_P1CPPCR_SWAPRB_Pos           (4U)
#define DCMIPP_P1CPPCR_SWAPRB_Msk           (0x1UL << DCMIPP_P1CPPCR_SWAPRB_Pos)             /*!< 0x00000010 */
#define DCMIPP_P1CPPCR_SWAPRB               DCMIPP_P1CPPCR_SWAPRB_Msk                       /*!< Swaps R-vs-B components if RGB, and U-vs-V components if YUV */
#define DCMIPP_P1CPPCR_LINEMULT_Pos         (13U)
#define DCMIPP_P1CPPCR_LINEMULT_Msk         (0x7UL << DCMIPP_P1CPPCR_LINEMULT_Pos)           /*!< 0x0000E000 */
#define DCMIPP_P1CPPCR_LINEMULT             DCMIPP_P1CPPCR_LINEMULT_Msk                     /*!< Amount of capture completed lines for LINE Event and Interrupt */

/**************  Bit definition for DCMIPP_P1CPPM0AR1 register  ***************/
#define DCMIPP_P1CPPM0AR1_M0A_Pos           (0U)
#define DCMIPP_P1CPPM0AR1_M0A_Msk           (0xFFFFFFFFUL << DCMIPP_P1CPPM0AR1_M0A_Pos)      /*!< 0xFFFFFFFF */
#define DCMIPP_P1CPPM0AR1_M0A               DCMIPP_P1CPPM0AR1_M0A_Msk                       /*!< Memory0 address */

/***************  Bit definition for DCMIPP_P1CPPM0PR register  ***************/
#define DCMIPP_P1CPPM0PR_PITCH_Pos          (0U)
#define DCMIPP_P1CPPM0PR_PITCH_Msk          (0x7FFFUL << DCMIPP_P1CPPM0PR_PITCH_Pos)         /*!< 0x00007FFF */
#define DCMIPP_P1CPPM0PR_PITCH              DCMIPP_P1CPPM0PR_PITCH_Msk                      /*!< Number of bytes between the address of two consecutive lines */

/**************  Bit definition for DCMIPP_P1CPPM1AR1 register  ***************/
#define DCMIPP_P1CPPM1AR1_M1A_Pos           (0U)
#define DCMIPP_P1CPPM1AR1_M1A_Msk           (0xFFFFFFFFUL << DCMIPP_P1CPPM1AR1_M1A_Pos)      /*!< 0xFFFFFFFF */
#define DCMIPP_P1CPPM1AR1_M1A               DCMIPP_P1CPPM1AR1_M1A_Msk                       /*!< Memory1 address */

/***************  Bit definition for DCMIPP_P1CPPM1PR register  ***************/
#define DCMIPP_P1CPPM1PR_PITCH_Pos          (0U)
#define DCMIPP_P1CPPM1PR_PITCH_Msk          (0x7FFFUL << DCMIPP_P1CPPM1PR_PITCH_Pos)         /*!< 0x00007FFF */
#define DCMIPP_P1CPPM1PR_PITCH              DCMIPP_P1CPPM1PR_PITCH_Msk                      /*!< Number of bytes between the address of two consecutive lines */

/**************  Bit definition for DCMIPP_P1CPPM2AR1 register  ***************/
#define DCMIPP_P1CPPM2AR1_M2A_Pos           (0U)
#define DCMIPP_P1CPPM2AR1_M2A_Msk           (0xFFFFFFFFUL << DCMIPP_P1CPPM2AR1_M2A_Pos)      /*!< 0xFFFFFFFF */
#define DCMIPP_P1CPPM2AR1_M2A               DCMIPP_P1CPPM2AR1_M2A_Msk                       /*!< Memory 2 address */

/****************  Bit definition for DCMIPP_P2FSCR register  *****************/
#define DCMIPP_P2FSCR_DTIDA_Pos             (0U)
#define DCMIPP_P2FSCR_DTIDA_Msk             (0x3FUL << DCMIPP_P2FSCR_DTIDA_Pos)              /*!< 0x0000003F */
#define DCMIPP_P2FSCR_DTIDA                 DCMIPP_P2FSCR_DTIDA_Msk                         /*!< Data type ID */
#define DCMIPP_P2FSCR_VC_Pos                (19U)
#define DCMIPP_P2FSCR_VC_Msk                (0x3UL << DCMIPP_P2FSCR_VC_Pos)                  /*!< 0x00180000 */
#define DCMIPP_P2FSCR_VC                    DCMIPP_P2FSCR_VC_Msk                            /*!< Flow selection mode */
#define DCMIPP_P2FSCR_FDTF_Pos              (24U)
#define DCMIPP_P2FSCR_FDTF_Msk              (0x3FUL << DCMIPP_P2FSCR_FDTF_Pos)               /*!< 0x3F000000 */
#define DCMIPP_P2FSCR_FDTF                  DCMIPP_P2FSCR_FDTF_Msk                          /*!< Force Data type format */
#define DCMIPP_P2FSCR_FDTFEN_Pos            (30U)
#define DCMIPP_P2FSCR_FDTFEN_Msk            (0x1UL << DCMIPP_P2FSCR_FDTFEN_Pos)              /*!< 0x40000000 */
#define DCMIPP_P2FSCR_FDTFEN                DCMIPP_P2FSCR_FDTFEN_Msk                        /*!< Force Data type format enable */
#define DCMIPP_P2FSCR_PIPEN_Pos             (31U)
#define DCMIPP_P2FSCR_PIPEN_Msk             (0x1UL << DCMIPP_P2FSCR_PIPEN_Pos)               /*!< 0x80000000 */
#define DCMIPP_P2FSCR_PIPEN                 DCMIPP_P2FSCR_PIPEN_Msk                         /*!< Activation of PipeN */

/****************  Bit definition for DCMIPP_P2FCTCR register  ****************/
#define DCMIPP_P2FCTCR_FRATE_Pos            (0U)
#define DCMIPP_P2FCTCR_FRATE_Msk            (0x3UL << DCMIPP_P2FCTCR_FRATE_Pos)              /*!< 0x00000003 */
#define DCMIPP_P2FCTCR_FRATE                DCMIPP_P2FCTCR_FRATE_Msk                        /*!< Frame capture rate control */
#define DCMIPP_P2FCTCR_CPTMODE_Pos          (2U)
#define DCMIPP_P2FCTCR_CPTMODE_Msk          (0x1UL << DCMIPP_P2FCTCR_CPTMODE_Pos)            /*!< 0x00000004 */
#define DCMIPP_P2FCTCR_CPTMODE              DCMIPP_P2FCTCR_CPTMODE_Msk                      /*!< Capture mode */
#define DCMIPP_P2FCTCR_CPTREQ_Pos           (3U)
#define DCMIPP_P2FCTCR_CPTREQ_Msk           (0x1UL << DCMIPP_P2FCTCR_CPTREQ_Pos)             /*!< 0x00000008 */
#define DCMIPP_P2FCTCR_CPTREQ               DCMIPP_P2FCTCR_CPTREQ_Msk                       /*!< Capture requested */

/****************  Bit definition for DCMIPP_P2CRSTR register  ****************/
#define DCMIPP_P2CRSTR_HSTART_Pos           (0U)
#define DCMIPP_P2CRSTR_HSTART_Msk           (0xFFFUL << DCMIPP_P2CRSTR_HSTART_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P2CRSTR_HSTART               DCMIPP_P2CRSTR_HSTART_Msk                       /*!< Horizontal start, from 0 to 4094 pixels wide */
#define DCMIPP_P2CRSTR_VSTART_Pos           (16U)
#define DCMIPP_P2CRSTR_VSTART_Msk           (0xFFFUL << DCMIPP_P2CRSTR_VSTART_Pos)           /*!< 0x0FFF0000 */
#define DCMIPP_P2CRSTR_VSTART               DCMIPP_P2CRSTR_VSTART_Msk                       /*!< Vertical start, from 0 to 4094 pixels high */

/****************  Bit definition for DCMIPP_P2CRSZR register  ****************/
#define DCMIPP_P2CRSZR_HSIZE_Pos            (0U)
#define DCMIPP_P2CRSZR_HSIZE_Msk            (0xFFFUL << DCMIPP_P2CRSZR_HSIZE_Pos)            /*!< 0x00000FFF */
#define DCMIPP_P2CRSZR_HSIZE                DCMIPP_P2CRSZR_HSIZE_Msk                        /*!< Horizontal size, from 0 to 4094 pixels wide */
#define DCMIPP_P2CRSZR_VSIZE_Pos            (16U)
#define DCMIPP_P2CRSZR_VSIZE_Msk            (0xFFFUL << DCMIPP_P2CRSZR_VSIZE_Pos)            /*!< 0x0FFF0000 */
#define DCMIPP_P2CRSZR_VSIZE                DCMIPP_P2CRSZR_VSIZE_Msk                        /*!< Vertical size, from 0 to 4094 pixels high */
#define DCMIPP_P2CRSZR_ENABLE_Pos           (31U)
#define DCMIPP_P2CRSZR_ENABLE_Msk           (0x1UL << DCMIPP_P2CRSZR_ENABLE_Pos)             /*!< 0x80000000 */
#define DCMIPP_P2CRSZR_ENABLE               DCMIPP_P2CRSZR_ENABLE_Msk                       /*!<  */

/****************  Bit definition for DCMIPP_P2DCCR register  *****************/
#define DCMIPP_P2DCCR_ENABLE_Pos            (0U)
#define DCMIPP_P2DCCR_ENABLE_Msk            (0x1UL << DCMIPP_P2DCCR_ENABLE_Pos)             /*!< 0x00000001 */
#define DCMIPP_P2DCCR_ENABLE                DCMIPP_P2DCCR_ENABLE_Msk                        /*!< Decimation enable */
#define DCMIPP_P2DCCR_HDEC_Pos              (1U)
#define DCMIPP_P2DCCR_HDEC_Msk              (0x3UL << DCMIPP_P2DCCR_HDEC_Pos)               /*!< 0x00000006 */
#define DCMIPP_P2DCCR_HDEC                  DCMIPP_P2DCCR_HDEC_Msk                          /*!< Horizontal decimation ratio */
#define DCMIPP_P2DCCR_VDEC_Pos              (3U)
#define DCMIPP_P2DCCR_VDEC_Msk              (0x3UL << DCMIPP_P2DCCR_VDEC_Pos)               /*!< 0x00000018 */
#define DCMIPP_P2DCCR_VDEC                  DCMIPP_P2DCCR_VDEC_Msk                          /*!< Vertical decimation ratio */

/****************  Bit definition for DCMIPP_P2DSCR register  *****************/
#define DCMIPP_P2DSCR_HDIV_Pos              (0U)
#define DCMIPP_P2DSCR_HDIV_Msk              (0x3FFUL << DCMIPP_P2DSCR_HDIV_Pos)              /*!< 0x000003FF */
#define DCMIPP_P2DSCR_HDIV                  DCMIPP_P2DSCR_HDIV_Msk                          /*!< Horizontal division factor, from 128 (8x) to 1023 (1x) */
#define DCMIPP_P2DSCR_VDIV_Pos              (16U)
#define DCMIPP_P2DSCR_VDIV_Msk              (0x3FFUL << DCMIPP_P2DSCR_VDIV_Pos)              /*!< 0x03FF0000 */
#define DCMIPP_P2DSCR_VDIV                  DCMIPP_P2DSCR_VDIV_Msk                          /*!< Vertical division factor, from 128 (8x) to 1023 (1x) */
#define DCMIPP_P2DSCR_ENABLE_Pos            (31U)
#define DCMIPP_P2DSCR_ENABLE_Msk            (0x1UL << DCMIPP_P2DSCR_ENABLE_Pos)              /*!< 0x80000000 */
#define DCMIPP_P2DSCR_ENABLE                DCMIPP_P2DSCR_ENABLE_Msk                        /*!<  */

/***************  Bit definition for DCMIPP_P2DSRTIOR register  ***************/
#define DCMIPP_P2DSRTIOR_HRATIO_Pos         (0U)
#define DCMIPP_P2DSRTIOR_HRATIO_Msk         (0xFFFFUL << DCMIPP_P2DSRTIOR_HRATIO_Pos)        /*!< 0x0000FFFF */
#define DCMIPP_P2DSRTIOR_HRATIO             DCMIPP_P2DSRTIOR_HRATIO_Msk                     /*!< Horizontal ratio, from 8192 (1x) to 65535 (8x) */
#define DCMIPP_P2DSRTIOR_VRATIO_Pos         (16U)
#define DCMIPP_P2DSRTIOR_VRATIO_Msk         (0xFFFFUL << DCMIPP_P2DSRTIOR_VRATIO_Pos)        /*!< 0xFFFF0000 */
#define DCMIPP_P2DSRTIOR_VRATIO             DCMIPP_P2DSRTIOR_VRATIO_Msk                     /*!< Vertical ratio, from 8192 (1x) to 65535 (8x) */

/****************  Bit definition for DCMIPP_P2DSSZR register  ****************/
#define DCMIPP_P2DSSZR_HSIZE_Pos            (0U)
#define DCMIPP_P2DSSZR_HSIZE_Msk            (0xFFFUL << DCMIPP_P2DSSZR_HSIZE_Pos)            /*!< 0x00000FFF */
#define DCMIPP_P2DSSZR_HSIZE                DCMIPP_P2DSSZR_HSIZE_Msk                        /*!< Horizontal size, from 0 to 4094 pixels wide */
#define DCMIPP_P2DSSZR_VSIZE_Pos            (16U)
#define DCMIPP_P2DSSZR_VSIZE_Msk            (0xFFFUL << DCMIPP_P2DSSZR_VSIZE_Pos)            /*!< 0x0FFF0000 */
#define DCMIPP_P2DSSZR_VSIZE                DCMIPP_P2DSSZR_VSIZE_Msk                        /*!< Vertical size, from 0 to 4094 pixels high */

/****************  Bit definition for DCMIPP_P2GMCR register  *****************/
#define DCMIPP_P2GMCR_ENABLE_Pos            (0U)
#define DCMIPP_P2GMCR_ENABLE_Msk            (0x1UL << DCMIPP_P2GMCR_ENABLE_Pos)              /*!< 0x00000001 */
#define DCMIPP_P2GMCR_ENABLE                DCMIPP_P2GMCR_ENABLE_Msk                        /*!<  */

/***************  Bit definition for DCMIPP_P2CMRICR register  ***************/
#define DCMIPP_P2CMRICR_ROILSZ_Pos          (0U)
#define DCMIPP_P2CMRICR_ROILSZ_Msk          (0x3UL << DCMIPP_P2CMRICR_ROILSZ_Pos)           /*!< 0x00000003 */
#define DCMIPP_P2CMRICR_ROILSZ              DCMIPP_P2CMRICR_ROILSZ_Msk                      /*!< Region of interest line size width */
#define DCMIPP_P2CMRICR_ROI1EN_Pos          (16U)
#define DCMIPP_P2CMRICR_ROI1EN_Msk          (0x1UL << DCMIPP_P2CMRICR_ROI1EN_Pos)           /*!< 0x00010000 */
#define DCMIPP_P2CMRICR_ROI1EN              DCMIPP_P2CMRICR_ROI1EN_Msk                      /*!< Region Of Interest 1 Enable */
#define DCMIPP_P2CMRICR_ROI2EN_Pos          (17U)
#define DCMIPP_P2CMRICR_ROI2EN_Msk          (0x1UL << DCMIPP_P2CMRICR_ROI2EN_Pos)           /*!< 0x00020000 */
#define DCMIPP_P2CMRICR_ROI2EN              DCMIPP_P2CMRICR_ROI2EN_Msk                      /*!< Region Of Interest 2 Enable */
#define DCMIPP_P2CMRICR_ROI3EN_Pos          (18U)
#define DCMIPP_P2CMRICR_ROI3EN_Msk          (0x1UL << DCMIPP_P2CMRICR_ROI3EN_Pos)           /*!< 0x00040000 */
#define DCMIPP_P2CMRICR_ROI3EN              DCMIPP_P2CMRICR_ROI3EN_Msk                      /*!< Region Of Interest 3 Enable */
#define DCMIPP_P2CMRICR_ROI4EN_Pos          (19U)
#define DCMIPP_P2CMRICR_ROI4EN_Msk          (0x1UL << DCMIPP_P2CMRICR_ROI4EN_Pos)           /*!< 0x00080000 */
#define DCMIPP_P2CMRICR_ROI4EN              DCMIPP_P2CMRICR_ROI4EN_Msk                      /*!< Region Of Interest 4 Enable */
#define DCMIPP_P2CMRICR_ROI5EN_Pos          (20U)
#define DCMIPP_P2CMRICR_ROI5EN_Msk          (0x1UL << DCMIPP_P2CMRICR_ROI5EN_Pos)           /*!< 0x00100000 */
#define DCMIPP_P2CMRICR_ROI5EN              DCMIPP_P2CMRICR_ROI5EN_Msk                      /*!< Region Of Interest 5 Enable */
#define DCMIPP_P2CMRICR_ROI6EN_Pos          (21U)
#define DCMIPP_P2CMRICR_ROI6EN_Msk          (0x1UL << DCMIPP_P2CMRICR_ROI6EN_Pos)           /*!< 0x00200000 */
#define DCMIPP_P2CMRICR_ROI6EN              DCMIPP_P2CMRICR_ROI6EN_Msk                      /*!< Region Of Interest 6 Enable */
#define DCMIPP_P2CMRICR_ROI7EN_Pos          (22U)
#define DCMIPP_P2CMRICR_ROI7EN_Msk          (0x1UL << DCMIPP_P2CMRICR_ROI7EN_Pos)           /*!< 0x00400000 */
#define DCMIPP_P2CMRICR_ROI7EN              DCMIPP_P2CMRICR_ROI7EN_Msk                      /*!< Region Of Interest 7 Enable */
#define DCMIPP_P2CMRICR_ROI8EN_Pos          (23U)
#define DCMIPP_P2CMRICR_ROI8EN_Msk          (0x1UL << DCMIPP_P2CMRICR_ROI8EN_Pos)           /*!< 0x00800000 */
#define DCMIPP_P2CMRICR_ROI8EN              DCMIPP_P2CMRICR_ROI8EN_Msk                      /*!< Region Of Interest 8 Enable */

/***************  Bit definition for DCMIPP_P2RIxCR1 register  ***************/
#define DCMIPP_P2RIxCR1_HSTART_Pos          (0U)
#define DCMIPP_P2RIxCR1_HSTART_Msk          (0xFFFUL << DCMIPP_P2RIxCR1_HSTART_Pos)        /*!< 0x00000FFF */
#define DCMIPP_P2RIxCR1_HSTART              DCMIPP_P2RIxCR1_HSTART_Msk                     /*!< Horizontal start */
#define DCMIPP_P2RIxCR1_CLB_Pos             (12U)
#define DCMIPP_P2RIxCR1_CLB_Msk             (0x3UL << DCMIPP_P2RIxCR1_CLB_Pos)             /*!< 0x00003000 */
#define DCMIPP_P2RIxCR1_CLB                 DCMIPP_P2RIxCR1_CLB_Msk                        /*!< Color line blue */
#define DCMIPP_P2RIxCR1_CLG_Pos             (14U)
#define DCMIPP_P2RIxCR1_CLG_Msk             (0x3UL << DCMIPP_P2RIxCR1_CLG_Pos)             /*!< 0x0000C000 */
#define DCMIPP_P2RIxCR1_CLG                 DCMIPP_P2RIxCR1_CLG_Msk                        /*!< Color line green */
#define DCMIPP_P2RIxCR1_VSTART_Pos          (16U)
#define DCMIPP_P2RIxCR1_VSTART_Msk          (0xFFFUL << DCMIPP_P2RIxCR1_VSTART_Pos)        /*!< 0x0FFF0000 */
#define DCMIPP_P2RIxCR1_VSTART              DCMIPP_P2RIxCR1_VSTART_Msk                     /*!< Vertical start */
#define DCMIPP_P2RIxCR1_CLR_Pos             (28U)
#define DCMIPP_P2RIxCR1_CLR_Msk             (0x3UL << DCMIPP_P2RIxCR1_CLR_Pos)             /*!< 0x30000000 */
#define DCMIPP_P2RIxCR1_CLR                 DCMIPP_P2RIxCR1_CLR_Msk                        /*!< Color line red */

/***************  Bit definition for DCMIPP_P2RIxCR2 register  ***************/
#define DCMIPP_P2RIxCR2_VSIZE_Pos           (0U)
#define DCMIPP_P2RIxCR2_VSIZE_Msk           (0xFFFUL << DCMIPP_P2RIxCR2_VSIZE_Pos)        /*!<  0x00000FFF */
#define DCMIPP_P2RIxCR2_VSIZE               DCMIPP_P2RIxCR2_VSIZE_Msk                     /*!< Vertical Size */
#define DCMIPP_P2RIxCR2_HSIZE_Pos           (16U)
#define DCMIPP_P2RIxCR2_HSIZE_Msk           (0xFFFUL << DCMIPP_P2RIxCR2_HSIZE_Pos)        /*!<  0x07FF8000 */
#define DCMIPP_P2RIxCR2_HSIZE               DCMIPP_P2RIxCR2_HSIZE_Msk                     /*!< Horizontal Size */

/****************  Bit definition for DCMIPP_P2PPCR register  *****************/
#define DCMIPP_P2PPCR_FORMAT_Pos            (0U)
#define DCMIPP_P2PPCR_FORMAT_Msk            (0xFUL << DCMIPP_P2PPCR_FORMAT_Pos)              /*!< 0x0000000F */
#define DCMIPP_P2PPCR_FORMAT                DCMIPP_P2PPCR_FORMAT_Msk                         /*!< Memory format (only coplanar formats are supported in Pipe2) */
#define DCMIPP_P2PPCR_SWAPRB_Pos            (4U)
#define DCMIPP_P2PPCR_SWAPRB_Msk            (0x1UL << DCMIPP_P2PPCR_SWAPRB_Pos)              /*!< 0x00000010 */
#define DCMIPP_P2PPCR_SWAPRB                DCMIPP_P2PPCR_SWAPRB_Msk                         /*!< Swaps R-vs-B components if RGB, and if YUV, swaps U-vs-V components */
#define DCMIPP_P2PPCR_LINEMULT_Pos          (13U)
#define DCMIPP_P2PPCR_LINEMULT_Msk          (0x7UL << DCMIPP_P2PPCR_LINEMULT_Pos)            /*!< 0x0000E000 */
#define DCMIPP_P2PPCR_LINEMULT              DCMIPP_P2PPCR_LINEMULT_Msk                       /*!< Amount of capture completed lines for LINE Event and Interrupt */
#define DCMIPP_P2PPCR_DBM_Pos               (16U)
#define DCMIPP_P2PPCR_DBM_Msk               (0x1UL << DCMIPP_P2PPCR_DBM_Pos)                 /*!< 0x00010000 */
#define DCMIPP_P2PPCR_DBM                   DCMIPP_P2PPCR_DBM_Msk                            /*!< Double buffer mode */
#define DCMIPP_P2PPCR_LMAWM_Pos             (17U)
#define DCMIPP_P2PPCR_LMAWM_Msk             (0x7UL << DCMIPP_P2PPCR_LMAWM_Pos)               /*!< 0x000E0000 */
#define DCMIPP_P2PPCR_LMAWM                 DCMIPP_P2PPCR_LMAWM_Msk                          /*!< Line multi address wrapping modulo */
#define DCMIPP_P2PPCR_LMAWE_Pos             (20U)
#define DCMIPP_P2PPCR_LMAWE_Msk             (0x7UL << DCMIPP_P2PPCR_LMAWE_Pos)               /*!< 0x00100000 */
#define DCMIPP_P2PPCR_LMAWE                 DCMIPP_P2PPCR_LMAWE_Msk                          /*!< Line multi address wrapping enable */

/***************  Bit definition for DCMIPP_P2PPM0AR1 register  ***************/
#define DCMIPP_P2PPM0AR1_M0A_Pos            (0U)
#define DCMIPP_P2PPM0AR1_M0A_Msk            (0xFFFFFFFFUL << DCMIPP_P2PPM0AR1_M0A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P2PPM0AR1_M0A                DCMIPP_P2PPM0AR1_M0A_Msk                        /*!< Memory0 address register 1 */

/***************  Bit definition for DCMIPP_P2PPM0AR2 register  ***************/
#define DCMIPP_P2PPM0AR2_M0A_Pos            (0U)
#define DCMIPP_P2PPM0AR2_M0A_Msk            (0xFFFFFFFFUL << DCMIPP_P2PPM0AR2_M0A_Pos)       /*!< 0xFFFFFFFF */
#define DCMIPP_P2PPM0AR2_M0A                DCMIPP_P2PPM0AR2_M0A_Msk                        /*!< Memory0 address register 2*/

/***************  Bit definition for DCMIPP_P2PPM0PR register  ****************/
#define DCMIPP_P2PPM0PR_PITCH_Pos           (0U)
#define DCMIPP_P2PPM0PR_PITCH_Msk           (0x7FFFUL << DCMIPP_P2PPM0PR_PITCH_Pos)          /*!< 0x00007FFF */
#define DCMIPP_P2PPM0PR_PITCH               DCMIPP_P2PPM0PR_PITCH_Msk                       /*!< Number of bytes between the address of two consecutive lines */

/***************  Bit definition for DCMIPP_P2PPM0PR register  ****************/
#define DCMIPP_P2STM0AR_Pos                 (0U)
#define DCMIPP_P2STM0AR_Msk                 (0xFFFFFFFFUL << DCMIPP_P2STM0AR_Pos)           /*!< 0xFFFFFFFF */
#define DCMIPP_P2STM0AR                     DCMIPP_P2STM0AR_Msk                             /*!< Pipe2 status Memory0 address register */

/*****************  Bit definition for DCMIPP_P2IER register  *****************/
#define DCMIPP_P2IER_LINEIE_Pos             (0U)
#define DCMIPP_P2IER_LINEIE_Msk             (0x1UL << DCMIPP_P2IER_LINEIE_Pos)               /*!< 0x00000001 */
#define DCMIPP_P2IER_LINEIE                 DCMIPP_P2IER_LINEIE_Msk                         /*!< Multi-line capture completed interrupt enable */
#define DCMIPP_P2IER_FRAMEIE_Pos            (1U)
#define DCMIPP_P2IER_FRAMEIE_Msk            (0x1UL << DCMIPP_P2IER_FRAMEIE_Pos)              /*!< 0x00000002 */
#define DCMIPP_P2IER_FRAMEIE                DCMIPP_P2IER_FRAMEIE_Msk                        /*!< Frame capture completed interrupt enable */
#define DCMIPP_P2IER_VSYNCIE_Pos            (2U)
#define DCMIPP_P2IER_VSYNCIE_Msk            (0x1UL << DCMIPP_P2IER_VSYNCIE_Pos)              /*!< 0x00000004 */
#define DCMIPP_P2IER_VSYNCIE                DCMIPP_P2IER_VSYNCIE_Msk                        /*!< VSYNC interrupt enable */
#define DCMIPP_P2IER_OVRIE_Pos              (7U)
#define DCMIPP_P2IER_OVRIE_Msk              (0x1UL << DCMIPP_P2IER_OVRIE_Pos)                /*!< 0x00000080 */
#define DCMIPP_P2IER_OVRIE                  DCMIPP_P2IER_OVRIE_Msk                          /*!< Overrun interrupt enable */

/*****************  Bit definition for DCMIPP_P2SR register  ******************/
#define DCMIPP_P2SR_LINEF_Pos               (0U)
#define DCMIPP_P2SR_LINEF_Msk               (0x1UL << DCMIPP_P2SR_LINEF_Pos)                 /*!< 0x00000001 */
#define DCMIPP_P2SR_LINEF                   DCMIPP_P2SR_LINEF_Msk                           /*!< Multi-line capture completed raw interrupt status */
#define DCMIPP_P2SR_FRAMEF_Pos              (1U)
#define DCMIPP_P2SR_FRAMEF_Msk              (0x1UL << DCMIPP_P2SR_FRAMEF_Pos)                /*!< 0x00000002 */
#define DCMIPP_P2SR_FRAMEF                  DCMIPP_P2SR_FRAMEF_Msk                          /*!< Frame capture completed raw interrupt status */
#define DCMIPP_P2SR_VSYNCF_Pos              (2U)
#define DCMIPP_P2SR_VSYNCF_Msk              (0x1UL << DCMIPP_P2SR_VSYNCF_Pos)                /*!< 0x00000004 */
#define DCMIPP_P2SR_VSYNCF                  DCMIPP_P2SR_VSYNCF_Msk                          /*!< VSYNC raw interrupt status */
#define DCMIPP_P2SR_OVRF_Pos                (7U)
#define DCMIPP_P2SR_OVRF_Msk                (0x1UL << DCMIPP_P2SR_OVRF_Pos)                  /*!< 0x00000080 */
#define DCMIPP_P2SR_OVRF                    DCMIPP_P2SR_OVRF_Msk                            /*!< Overrun raw interrupt status */
#define DCMIPP_P2SR_LSTLINE_Pos             (16U)
#define DCMIPP_P2SR_LSTLINE_Msk             (0x1UL << DCMIPP_P2SR_LSTLINE_Pos)               /*!< 0x00010000 */
#define DCMIPP_P2SR_LSTLINE                 DCMIPP_P2SR_LSTLINE_Msk                         /*!< Last line LSB bit, sampled at frame capture complete event */
#define DCMIPP_P2SR_LSTFRM_Pos              (17U)
#define DCMIPP_P2SR_LSTFRM_Msk              (0x1UL << DCMIPP_P2SR_LSTFRM_Pos)                /*!< 0x00020000 */
#define DCMIPP_P2SR_LSTFRM                  DCMIPP_P2SR_LSTFRM_Msk                          /*!< Last frame LSB bit, sampled at frame capture complete event */
#define DCMIPP_P2SR_CPTACT_Pos              (23U)
#define DCMIPP_P2SR_CPTACT_Msk              (0x1UL << DCMIPP_P2SR_CPTACT_Pos)                /*!< 0x00800000 */
#define DCMIPP_P2SR_CPTACT                  DCMIPP_P2SR_CPTACT_Msk                          /*!< Capture immediate status */

/*****************  Bit definition for DCMIPP_P2FCR register  *****************/
#define DCMIPP_P2FCR_CLINEF_Pos             (0U)
#define DCMIPP_P2FCR_CLINEF_Msk             (0x1UL << DCMIPP_P2FCR_CLINEF_Pos)               /*!< 0x00000001 */
#define DCMIPP_P2FCR_CLINEF                 DCMIPP_P2FCR_CLINEF_Msk                         /*!< Multi-line capture complete interrupt status clear */
#define DCMIPP_P2FCR_CFRAMEF_Pos            (1U)
#define DCMIPP_P2FCR_CFRAMEF_Msk            (0x1UL << DCMIPP_P2FCR_CFRAMEF_Pos)              /*!< 0x00000002 */
#define DCMIPP_P2FCR_CFRAMEF                DCMIPP_P2FCR_CFRAMEF_Msk                        /*!< Frame capture complete interrupt status clear */
#define DCMIPP_P2FCR_CVSYNCF_Pos            (2U)
#define DCMIPP_P2FCR_CVSYNCF_Msk            (0x1UL << DCMIPP_P2FCR_CVSYNCF_Pos)              /*!< 0x00000004 */
#define DCMIPP_P2FCR_CVSYNCF                DCMIPP_P2FCR_CVSYNCF_Msk                        /*!< Vertical synchronization interrupt status clear */
#define DCMIPP_P2FCR_COVRF_Pos              (7U)
#define DCMIPP_P2FCR_COVRF_Msk              (0x1UL << DCMIPP_P2FCR_COVRF_Pos)                /*!< 0x00000080 */
#define DCMIPP_P2FCR_COVRF                  DCMIPP_P2FCR_COVRF_Msk                          /*!< Overrun interrupt status clear */

/****************  Bit definition for DCMIPP_P2CFSCR register  ****************/
#define DCMIPP_P2CFSCR_DTID_Pos             (0U)
#define DCMIPP_P2CFSCR_DTID_Msk             (0x3FUL << DCMIPP_P2CFSCR_DTID_Pos)              /*!< 0x0000003F */
#define DCMIPP_P2CFSCR_DTID                 DCMIPP_P2CFSCR_DTID_Msk                         /*!< Current Data type ID */
#define DCMIPP_P2CFSCR_VC_Pos               (19U)
#define DCMIPP_P2CFSCR_VC_Msk               (0x3UL << DCMIPP_P2CFSCR_VC_Pos)                 /*!< 0x00180000 */
#define DCMIPP_P2CFSCR_VC                   DCMIPP_P2CFSCR_VC_Msk                           /*!< Current flow selection mode */
#define DCMIPP_P2CFSCR_FDTF_Pos             (24U)
#define DCMIPP_P2CFSCR_FDTF_Msk             (0x3FUL << DCMIPP_P2CFSCR_FDTF_Pos)              /*!< 0x3F000000 */
#define DCMIPP_P2CFSCR_FDTF                 DCMIPP_P2CFSCR_FDTF_Msk                         /*!< Current force Data type format */
#define DCMIPP_P2CFSCR_FDTFEN_Pos           (30U)
#define DCMIPP_P2CFSCR_FDTFEN_Msk           (0x1UL << DCMIPP_P2CFSCR_FDTFEN_Pos)             /*!< 0x40000000 */
#define DCMIPP_P2CFSCR_FDTFEN               DCMIPP_P2CFSCR_FDTFEN_Msk                       /*!< Current force Data type format enable */
#define DCMIPP_P2CFSCR_PIPEN_Pos            (31U)
#define DCMIPP_P2CFSCR_PIPEN_Msk            (0x1UL << DCMIPP_P2CFSCR_PIPEN_Pos)              /*!< 0x80000000 */
#define DCMIPP_P2CFSCR_PIPEN                DCMIPP_P2CFSCR_PIPEN_Msk                        /*!< Current activation of PipeN */

/***************  Bit definition for DCMIPP_P2CFCTCR register  ****************/
#define DCMIPP_P2CFCTCR_FRATE_Pos           (0U)
#define DCMIPP_P2CFCTCR_FRATE_Msk           (0x3UL << DCMIPP_P2CFCTCR_FRATE_Pos)             /*!< 0x00000003 */
#define DCMIPP_P2CFCTCR_FRATE               DCMIPP_P2CFCTCR_FRATE_Msk                       /*!< Frame capture rate control */
#define DCMIPP_P2CFCTCR_CPTMODE_Pos         (2U)
#define DCMIPP_P2CFCTCR_CPTMODE_Msk         (0x1UL << DCMIPP_P2CFCTCR_CPTMODE_Pos)           /*!< 0x00000004 */
#define DCMIPP_P2CFCTCR_CPTMODE             DCMIPP_P2CFCTCR_CPTMODE_Msk                     /*!< Capture mode */
#define DCMIPP_P2CFCTCR_CPTREQ_Pos          (3U)
#define DCMIPP_P2CFCTCR_CPTREQ_Msk          (0x1UL << DCMIPP_P2CFCTCR_CPTREQ_Pos)            /*!< 0x00000008 */
#define DCMIPP_P2CFCTCR_CPTREQ              DCMIPP_P2CFCTCR_CPTREQ_Msk                      /*!< Capture requested */

/***************  Bit definition for DCMIPP_P2CCRSTR register  ****************/
#define DCMIPP_P2CCRSTR_HSTART_Pos          (0U)
#define DCMIPP_P2CCRSTR_HSTART_Msk          (0xFFFUL << DCMIPP_P2CCRSTR_HSTART_Pos)          /*!< 0x00000FFF */
#define DCMIPP_P2CCRSTR_HSTART              DCMIPP_P2CCRSTR_HSTART_Msk                      /*!< Current horizontal start, from 0 to 4094 pixels wide */
#define DCMIPP_P2CCRSTR_VSTART_Pos          (16U)
#define DCMIPP_P2CCRSTR_VSTART_Msk          (0xFFFUL << DCMIPP_P2CCRSTR_VSTART_Pos)          /*!< 0x0FFF0000 */
#define DCMIPP_P2CCRSTR_VSTART              DCMIPP_P2CCRSTR_VSTART_Msk                      /*!< Current vertical start, from 0 to 4094 pixels high */

/***************  Bit definition for DCMIPP_P2CCRSZR register  ****************/
#define DCMIPP_P2CCRSZR_HSIZE_Pos           (0U)
#define DCMIPP_P2CCRSZR_HSIZE_Msk           (0xFFFUL << DCMIPP_P2CCRSZR_HSIZE_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P2CCRSZR_HSIZE               DCMIPP_P2CCRSZR_HSIZE_Msk                       /*!< Current horizontal size, from 0 to 4094 pixels wide */
#define DCMIPP_P2CCRSZR_VSIZE_Pos           (16U)
#define DCMIPP_P2CCRSZR_VSIZE_Msk           (0xFFFUL << DCMIPP_P2CCRSZR_VSIZE_Pos)           /*!< 0x0FFF0000 */
#define DCMIPP_P2CCRSZR_VSIZE               DCMIPP_P2CCRSZR_VSIZE_Msk                       /*!< Current vertical size, from 0 to 4094 pixels high */
#define DCMIPP_P2CCRSZR_ENABLE_Pos          (31U)
#define DCMIPP_P2CCRSZR_ENABLE_Msk          (0x1UL << DCMIPP_P2CCRSZR_ENABLE_Pos)            /*!< 0x80000000 */
#define DCMIPP_P2CCRSZR_ENABLE              DCMIPP_P2CCRSZR_ENABLE_Msk                      /*!< Current ENABLE bit value */

/****************  Bit definition for DCMIPP_P2CDCCR register  *****************/
#define DCMIPP_P2CDCCR_ENABLE_Pos           (0U)
#define DCMIPP_P2CDCCR_ENABLE_Msk           (0x1UL << DCMIPP_P2CDCCR_ENABLE_Pos)            /*!< 0x00000001 */
#define DCMIPP_P2CDCCR_ENABLE               DCMIPP_P2CDCCR_ENABLE_Msk                       /*!< Decimation enable */
#define DCMIPP_P2CDCCR_HDEC_Pos             (1U)
#define DCMIPP_P2CDCCR_HDEC_Msk             (0x3UL << DCMIPP_P2CDCCR_HDEC_Pos)              /*!< 0x00000006 */
#define DCMIPP_P2CDCCR_HDEC                 DCMIPP_P2CDCCR_HDEC_Msk                         /*!< Horizontal decimation ratio */
#define DCMIPP_P2CDCCR_VDEC_Pos             (3U)
#define DCMIPP_P2CDCCR_VDEC_Msk             (0x3UL << DCMIPP_P2CDCCR_VDEC_Pos)              /*!< 0x00000018 */
#define DCMIPP_P2CDCCR_VDEC                 DCMIPP_P2CDCCR_VDEC_Msk                         /*!< Vertical decimation ratio */

/****************  Bit definition for DCMIPP_P2CDSCR register  ****************/
#define DCMIPP_P2CDSCR_HDIV_Pos             (0U)
#define DCMIPP_P2CDSCR_HDIV_Msk             (0x3FFUL << DCMIPP_P2CDSCR_HDIV_Pos)             /*!< 0x000003FF */
#define DCMIPP_P2CDSCR_HDIV                 DCMIPP_P2CDSCR_HDIV_Msk                         /*!< Current horizontal division factor, from 128 (8x) to 1023 (1x) */
#define DCMIPP_P2CDSCR_VDIV_Pos             (16U)
#define DCMIPP_P2CDSCR_VDIV_Msk             (0x3FFUL << DCMIPP_P2CDSCR_VDIV_Pos)             /*!< 0x03FF0000 */
#define DCMIPP_P2CDSCR_VDIV                 DCMIPP_P2CDSCR_VDIV_Msk                         /*!< Current vertical division factor, from 128 (8x) to 1023 (1x) */
#define DCMIPP_P2CDSCR_ENABLE_Pos           (31U)
#define DCMIPP_P2CDSCR_ENABLE_Msk           (0x1UL << DCMIPP_P2CDSCR_ENABLE_Pos)             /*!< 0x80000000 */
#define DCMIPP_P2CDSCR_ENABLE               DCMIPP_P2CDSCR_ENABLE_Msk                       /*!< Current value of the bit ENABLE */

/**************  Bit definition for DCMIPP_P2CDSRTIOR register  ***************/
#define DCMIPP_P2CDSRTIOR_HRATIO_Pos        (0U)
#define DCMIPP_P2CDSRTIOR_HRATIO_Msk        (0xFFFFUL << DCMIPP_P2CDSRTIOR_HRATIO_Pos)       /*!< 0x0000FFFF */
#define DCMIPP_P2CDSRTIOR_HRATIO            DCMIPP_P2CDSRTIOR_HRATIO_Msk                    /*!< Current horizontal ratio, from 8192 (1x) to 65535 (8x) */
#define DCMIPP_P2CDSRTIOR_VRATIO_Pos        (16U)
#define DCMIPP_P2CDSRTIOR_VRATIO_Msk        (0xFFFFUL << DCMIPP_P2CDSRTIOR_VRATIO_Pos)       /*!< 0xFFFF0000 */
#define DCMIPP_P2CDSRTIOR_VRATIO            DCMIPP_P2CDSRTIOR_VRATIO_Msk                    /*!< Current vertical ratio, from 8192 (1x) to 65535 (8x) */

/***************  Bit definition for DCMIPP_P2CDSSZR register  ****************/
#define DCMIPP_P2CDSSZR_HSIZE_Pos           (0U)
#define DCMIPP_P2CDSSZR_HSIZE_Msk           (0xFFFUL << DCMIPP_P2CDSSZR_HSIZE_Pos)           /*!< 0x00000FFF */
#define DCMIPP_P2CDSSZR_HSIZE               DCMIPP_P2CDSSZR_HSIZE_Msk                       /*!< Current horizontal size, from 0 to 4094 pixels wide */
#define DCMIPP_P2CDSSZR_VSIZE_Pos           (16U)
#define DCMIPP_P2CDSSZR_VSIZE_Msk           (0xFFFUL << DCMIPP_P2CDSSZR_VSIZE_Pos)           /*!< 0x0FFF0000 */
#define DCMIPP_P2CDSSZR_VSIZE               DCMIPP_P2CDSSZR_VSIZE_Msk                       /*!< Current vertical size, from 0 to 4094 pixels high */

/****************  Bit definition for DCMIPP_P2CPPCR register  ****************/
#define DCMIPP_P2CPPCR_FORMAT_Pos           (0U)
#define DCMIPP_P2CPPCR_FORMAT_Msk           (0xFUL << DCMIPP_P2CPPCR_FORMAT_Pos)             /*!< 0x0000000F */
#define DCMIPP_P2CPPCR_FORMAT               DCMIPP_P2CPPCR_FORMAT_Msk                       /*!< Memory format (only coplanar formats are supported in Pipe2) */
#define DCMIPP_P2CPPCR_SWAPRB_Pos           (4U)
#define DCMIPP_P2CPPCR_SWAPRB_Msk           (0x1UL << DCMIPP_P2CPPCR_SWAPRB_Pos)             /*!< 0x00000010 */
#define DCMIPP_P2CPPCR_SWAPRB               DCMIPP_P2CPPCR_SWAPRB_Msk                       /*!< Swaps R-vs-B components if RGB, and if YUV, swaps U-vs-V components */
#define DCMIPP_P2CPPCR_LINEMULT_Pos         (13U)
#define DCMIPP_P2CPPCR_LINEMULT_Msk         (0x7UL << DCMIPP_P2CPPCR_LINEMULT_Pos)           /*!< 0x0000E000 */
#define DCMIPP_P2CPPCR_LINEMULT             DCMIPP_P2CPPCR_LINEMULT_Msk                     /*!< Amount of capture completed lines for LINE Event and Interrupt */
#define DCMIPP_P2CPPCR_DBM_Pos              (16U)
#define DCMIPP_P2CPPCR_DBM_Msk              (0x1UL << DCMIPP_P2CPPCR_DBM_Pos)                /*!< 0x00010000 */
#define DCMIPP_P2CPPCR_DBM                  DCMIPP_P2CPPCR_DBM_Msk                           /*!< Double buffer mode */
#define DCMIPP_P2CPPCR_LMAWM_Pos            (17U)
#define DCMIPP_P2CPPCR_LMAWM_Msk            (0x7UL << DCMIPP_P2CPPCR_LMAWM_Pos)              /*!< 0x000E0000 */
#define DCMIPP_P2CPPCR_LMAWM                DCMIPP_P2CPPCR_LMAWM_Msk                         /*!< Line multi address wrapping modulo */
#define DCMIPP_P2CPPCR_LMAWE_Pos            (20U)
#define DCMIPP_P2CPPCR_LMAWE_Msk            (0x7UL << DCMIPP_P2CPPCR_LMAWE_Pos)              /*!< 0x00100000 */
#define DCMIPP_P2CPPCR_LMAWE                DCMIPP_P2CPPCR_LMAWE_Msk                         /*!< Line multi address wrapping enable */

/**************  Bit definition for DCMIPP_P2CPPM0AR1 register  ***************/
#define DCMIPP_P2CPPM0AR1_M0A_Pos           (0U)
#define DCMIPP_P2CPPM0AR1_M0A_Msk           (0xFFFFFFFFUL << DCMIPP_P2CPPM0AR1_M0A_Pos)      /*!< 0xFFFFFFFF */
#define DCMIPP_P2CPPM0AR1_M0A               DCMIPP_P2CPPM0AR1_M0A_Msk                       /*!< Memory0 address */

/**************  Bit definition for DCMIPP_P2CPPM0AR2 register  ***************/
#define DCMIPP_P2CPPM0AR2_M0A_Pos           (0U)
#define DCMIPP_P2CPPM0AR2_M0A_Msk           (0xFFFFFFFFUL << DCMIPP_P2CPPM0AR1_M0A_Pos)      /*!< 0xFFFFFFFF */
#define DCMIPP_P2CPPM0AR2_M0A               DCMIPP_P2CPPM0AR1_M0A_Msk                       /*!< Memory0 address Register 2 */

/***************  Bit definition for DCMIPP_P2CPPM0PR register  ***************/
#define DCMIPP_P2CPPM0PR_PITCH_Pos          (0U)
#define DCMIPP_P2CPPM0PR_PITCH_Msk          (0x7FFFUL << DCMIPP_P2CPPM0PR_PITCH_Pos)         /*!< 0x00007FFF */
#define DCMIPP_P2CPPM0PR_PITCH              DCMIPP_P2CPPM0PR_PITCH_Msk                      /*!< Number of bytes between the address of two consecutive lines */

/****************  Bit definition for DCMIPP_HWCFGR2 register  ****************/
#define DCMIPP_HWCFGR2_VPFT_Pos             (0U)
#define DCMIPP_HWCFGR2_VPFT_Msk             (0x7U << DCMIPP_HWCFGR2_VPFT_Pos)               /*!< 0x00000007 */
#define DCMIPP_HWCFGR2_VPFT                 DCMIPP_HWCFGR2_VPFT_Msk                         /*!< Virtual pipe function */
#define DCMIPP_HWCFGR2_DBMFT_Pos            (4U)
#define DCMIPP_HWCFGR2_DBMFT_Msk            (0x1U << DCMIPP_HWCFGR2_DBMFT_Pos)              /*!< 0x00000010 */
#define DCMIPP_HWCFGR2_DBMFT                DCMIPP_HWCFGR2_DBMFT_Msk                        /*!< Double buffer mode featured */
#define DCMIPP_HWCFGR2_PROCCLK_Pos          (8U)
#define DCMIPP_HWCFGR2_PROCCLK_Msk          (0x1U << DCMIPP_HWCFGR2_PROCCLK_Pos)            /*!< 0x00000100 */
#define DCMIPP_HWCFGR2_PROCCLK              DCMIPP_HWCFGR2_PROCCLK_Msk                      /*!< Processing clock linked to AXI clock featured */
#define DCMIPP_HWCFGR2_ADDMOD_Pos           (12U)
#define DCMIPP_HWCFGR2_ADDMOD_Msk           (0x1U << DCMIPP_HWCFGR2_ADDMOD_Pos)             /*!< 0x00001000 */
#define DCMIPP_HWCFGR2_ADDMOD               DCMIPP_HWCFGR2_ADDMOD_Msk                       /*!< Address modulo computation to access a small buffer in streaming featured */
#define DCMIPP_HWCFGR2_DEC1_Pos             (16U)
#define DCMIPP_HWCFGR2_DEC1_Msk             (0x1U << DCMIPP_HWCFGR2_DEC1_Pos)               /*!< 0x00010000 */
#define DCMIPP_HWCFGR2_DEC1                 DCMIPP_HWCFGR2_DEC1_Msk                         /*!< Decimation on Pipe1 before downsize */
#define DCMIPP_HWCFGR2_DEC2_Pos             (17U)
#define DCMIPP_HWCFGR2_DEC2_Msk             (0x1U << DCMIPP_HWCFGR2_DEC2_Pos)               /*!< 0x00020000 */
#define DCMIPP_HWCFGR2_DEC2                 DCMIPP_HWCFGR2_DEC2_Msk                         /*!< Decimation on Pipe2 before downsize */
#define DCMIPP_HWCFGR2_MCU_Pos              (20U)
#define DCMIPP_HWCFGR2_MCU_Msk              (0x1U << DCMIPP_HWCFGR2_MCU_Pos)                /*!< 0x00100000 */
#define DCMIPP_HWCFGR2_MCU                  DCMIPP_HWCFGR2_MCU_Msk                          /*!< Macroblock unit as pixel format  */
#define DCMIPP_HWCFGR2_TPG_Pos              (24U)
#define DCMIPP_HWCFGR2_TPG_Msk              (0x1U << DCMIPP_HWCFGR2_TPG_Pos)                /*!< 0x01000000 */
#define DCMIPP_HWCFGR2_TPG                  DCMIPP_HWCFGR2_TPG_Msk                          /*!< Test Pattern Generator */
#define DCMIPP_HWCFGR2_STV_Pos              (28U)
#define DCMIPP_HWCFGR2_STV_Msk              (0x1U << DCMIPP_HWCFGR2_STV_Pos)                /*!< 0x10000000 */
#define DCMIPP_HWCFGR2_STV                  DCMIPP_HWCFGR2_STV_Msk                          /*!< Statistic Version */

/****************  Bit definition for DCMIPP_HWCFGR1 register  ****************/
#define DCMIPP_HWCFGR1_CSIFT_Pos            (0U)
#define DCMIPP_HWCFGR1_CSIFT_Msk            (0x1U << DCMIPP_HWCFGR1_CSIFT_Pos)              /*!< 0x00000001 */
#define DCMIPP_HWCFGR1_CSIFT                DCMIPP_HWCFGR1_CSIFT_Msk                        /*!< CSI2 host protocol compliant */
#define DCMIPP_HWCFGR1_PIPENB_Pos           (4U)
#define DCMIPP_HWCFGR1_PIPENB_Msk           (0x3U << DCMIPP_HWCFGR1_PIPENB_Pos)             /*!< 0x00000030 */
#define DCMIPP_HWCFGR1_PIPENB               DCMIPP_HWCFGR1_PIPENB_Msk                       /*!< Number of pipes */
#define DCMIPP_HWCFGR1_IPPLUGCFG_Pos        (8U)
#define DCMIPP_HWCFGR1_IPPLUGCFG_Msk        (0x1U << DCMIPP_HWCFGR1_IPPLUGCFG_Pos)          /*!< 0x00000100 */
#define DCMIPP_HWCFGR1_IPPLUGCFG            DCMIPP_HWCFGR1_IPPLUGCFG_Msk                    /*!< IP-Plug configuration */
#define DCMIPP_HWCFGR1_DSP1FT_Pos           (12U)
#define DCMIPP_HWCFGR1_DSP1FT_Msk           (0x1U << DCMIPP_HWCFGR1_DSP1FT_Pos)             /*!< 0x00001000 */
#define DCMIPP_HWCFGR1_DSP1FT               DCMIPP_HWCFGR1_DSP1FT_Msk                       /*!< Down-sampling feature for the pixel Pipe1 */
#define DCMIPP_HWCFGR1_DSP2FT_Pos           (13U)
#define DCMIPP_HWCFGR1_DSP2FT_Msk           (0x1U << DCMIPP_HWCFGR1_DSP2FT_Pos)             /*!< 0x00002000 */
#define DCMIPP_HWCFGR1_DSP2FT               DCMIPP_HWCFGR1_DSP2FT_Msk                       /*!< Down-sampling feature for the pixel Pipe2 */
#define DCMIPP_HWCFGR1_RB2RGB_Pos           (16U)
#define DCMIPP_HWCFGR1_RB2RGB_Msk           (0x1U << DCMIPP_HWCFGR1_RB2RGB_Pos)             /*!< 0x00010000 */
#define DCMIPP_HWCFGR1_RB2RGB               DCMIPP_HWCFGR1_RB2RGB_Msk                       /*!< Raw Bayer to RGB feature (demosaicer) */
#define DCMIPP_HWCFGR1_PLANARFT_Pos         (20U)
#define DCMIPP_HWCFGR1_PLANARFT_Msk         (0x3U << DCMIPP_HWCFGR1_PLANARFT_Pos)           /*!< 0x00300000 */
#define DCMIPP_HWCFGR1_PLANARFT             DCMIPP_HWCFGR1_PLANARFT_Msk                     /*!< Buffer features for Pipe1 */
#define DCMIPP_HWCFGR1_ROI1NB_Pos           (24U)
#define DCMIPP_HWCFGR1_ROI1NB_Msk           (0xFU << DCMIPP_HWCFGR1_ROI1NB_Pos)             /*!< 0x0F000000 */
#define DCMIPP_HWCFGR1_ROI1NB               DCMIPP_HWCFGR1_ROI1NB_Msk                       /*!< Number of ROIs for Pipe1 */
#define DCMIPP_HWCFGR1_ROI2NB_Pos           (28U)
#define DCMIPP_HWCFGR1_ROI2NB_Msk           (0xFU << DCMIPP_HWCFGR1_ROI2NB_Pos)             /*!< 0xF0000000 */
#define DCMIPP_HWCFGR1_ROI2NB               DCMIPP_HWCFGR1_ROI2NB_Msk                       /*!< Number of ROIs for Pipe2 */

/*****************  Bit definition for DCMIPP_VERR register  ******************/
#define DCMIPP_VERR_MINREV_Pos              (0U)
#define DCMIPP_VERR_MINREV_Msk              (0xFU << DCMIPP_VERR_MINREV_Pos)                /*!< 0x0000000F */
#define DCMIPP_VERR_MINREV                  DCMIPP_VERR_MINREV_Msk                          /*!< DCMIPP minor revision */
#define DCMIPP_VERR_MAJREV_Pos              (4U)
#define DCMIPP_VERR_MAJREV_Msk              (0xFU << DCMIPP_VERR_MAJREV_Pos)                /*!< 0x000000F0 */
#define DCMIPP_VERR_MAJREV                  DCMIPP_VERR_MAJREV_Msk                          /*!< DCMIPP major revision */

/*****************  Bit definition for DCMIPP_IPIDR register  *****************/
#define DCMIPP_IPIDR_IDR_Pos                (0U)
#define DCMIPP_IPIDR_IDR_Msk                (0xFFFFFFFFU << DCMIPP_IPIDR_IDR_Pos)           /*!< 0xFFFFFFFF */
#define DCMIPP_IPIDR_IDR                    DCMIPP_IPIDR_IDR_Msk                            /*!< Parallel camera interface (DCMI) and optional pixel processing (PP) */

/*****************  Bit definition for DCMIPP_SIDR register  ******************/
#define DCMIPP_SIDR_SID_Pos                 (0U)
#define DCMIPP_SIDR_SID_Msk                 (0xFFFFFFFFU << DCMIPP_SIDR_SID_Pos)            /*!< 0xFFFFFFFF */
#define DCMIPP_SIDR_SID                     DCMIPP_SIDR_SID_Msk                             /*!< 4-Kbyte decoding space */

/******************************************************************************/
/*                                                                            */
/*                        Delay Block Interface (DLYB)                        */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for DLYB_CR register  ********************/

/* NuttX-style _SHIFT aliases for the CMSIS _Pos bit-position macros */

#define CSI_CR_CSIEN_SHIFT   CSI_CR_CSIEN_Pos
#define CSI_CR_VC0START_SHIFT   CSI_CR_VC0START_Pos
#define CSI_CR_VC0STOP_SHIFT   CSI_CR_VC0STOP_Pos
#define CSI_CR_VC1START_SHIFT   CSI_CR_VC1START_Pos
#define CSI_CR_VC1STOP_SHIFT   CSI_CR_VC1STOP_Pos
#define CSI_CR_VC2START_SHIFT   CSI_CR_VC2START_Pos
#define CSI_CR_VC2STOP_SHIFT   CSI_CR_VC2STOP_Pos
#define CSI_CR_VC3START_SHIFT   CSI_CR_VC3START_Pos
#define CSI_CR_VC3STOP_SHIFT   CSI_CR_VC3STOP_Pos
#define CSI_PCR_PWRDOWN_SHIFT   CSI_PCR_PWRDOWN_Pos
#define CSI_PCR_CLEN_SHIFT   CSI_PCR_CLEN_Pos
#define CSI_PCR_DL0EN_SHIFT   CSI_PCR_DL0EN_Pos
#define CSI_PCR_DL1EN_SHIFT   CSI_PCR_DL1EN_Pos
#define CSI_VC0CFGR1_ALLDT_SHIFT   CSI_VC0CFGR1_ALLDT_Pos
#define CSI_VC0CFGR1_DT0EN_SHIFT   CSI_VC0CFGR1_DT0EN_Pos
#define CSI_VC0CFGR1_DT1EN_SHIFT   CSI_VC0CFGR1_DT1EN_Pos
#define CSI_VC0CFGR1_DT2EN_SHIFT   CSI_VC0CFGR1_DT2EN_Pos
#define CSI_VC0CFGR1_DT3EN_SHIFT   CSI_VC0CFGR1_DT3EN_Pos
#define CSI_VC0CFGR1_DT4EN_SHIFT   CSI_VC0CFGR1_DT4EN_Pos
#define CSI_VC0CFGR1_DT5EN_SHIFT   CSI_VC0CFGR1_DT5EN_Pos
#define CSI_VC0CFGR1_DT6EN_SHIFT   CSI_VC0CFGR1_DT6EN_Pos
#define CSI_VC0CFGR1_CDTFT_SHIFT   CSI_VC0CFGR1_CDTFT_Pos
#define CSI_VC0CFGR1_DT0_SHIFT   CSI_VC0CFGR1_DT0_Pos
#define CSI_VC0CFGR1_DT0FT_SHIFT   CSI_VC0CFGR1_DT0FT_Pos
#define CSI_VC0CFGR2_DT1_SHIFT   CSI_VC0CFGR2_DT1_Pos
#define CSI_VC0CFGR2_DT1FT_SHIFT   CSI_VC0CFGR2_DT1FT_Pos
#define CSI_VC0CFGR2_DT2_SHIFT   CSI_VC0CFGR2_DT2_Pos
#define CSI_VC0CFGR2_DT2FT_SHIFT   CSI_VC0CFGR2_DT2FT_Pos
#define CSI_VC0CFGR3_DT3_SHIFT   CSI_VC0CFGR3_DT3_Pos
#define CSI_VC0CFGR3_DT3FT_SHIFT   CSI_VC0CFGR3_DT3FT_Pos
#define CSI_VC0CFGR3_DT4_SHIFT   CSI_VC0CFGR3_DT4_Pos
#define CSI_VC0CFGR3_DT4FT_SHIFT   CSI_VC0CFGR3_DT4FT_Pos
#define CSI_VC0CFGR4_DT5_SHIFT   CSI_VC0CFGR4_DT5_Pos
#define CSI_VC0CFGR4_DT5FT_SHIFT   CSI_VC0CFGR4_DT5FT_Pos
#define CSI_VC0CFGR4_DT6_SHIFT   CSI_VC0CFGR4_DT6_Pos
#define CSI_VC0CFGR4_DT6FT_SHIFT   CSI_VC0CFGR4_DT6FT_Pos
#define CSI_VC1CFGR1_ALLDT_SHIFT   CSI_VC1CFGR1_ALLDT_Pos
#define CSI_VC1CFGR1_DT0EN_SHIFT   CSI_VC1CFGR1_DT0EN_Pos
#define CSI_VC1CFGR1_DT1EN_SHIFT   CSI_VC1CFGR1_DT1EN_Pos
#define CSI_VC1CFGR1_DT2EN_SHIFT   CSI_VC1CFGR1_DT2EN_Pos
#define CSI_VC1CFGR1_DT3EN_SHIFT   CSI_VC1CFGR1_DT3EN_Pos
#define CSI_VC1CFGR1_DT4EN_SHIFT   CSI_VC1CFGR1_DT4EN_Pos
#define CSI_VC1CFGR1_DT5EN_SHIFT   CSI_VC1CFGR1_DT5EN_Pos
#define CSI_VC1CFGR1_DT6EN_SHIFT   CSI_VC1CFGR1_DT6EN_Pos
#define CSI_VC1CFGR1_CDTFT_SHIFT   CSI_VC1CFGR1_CDTFT_Pos
#define CSI_VC1CFGR1_DT0_SHIFT   CSI_VC1CFGR1_DT0_Pos
#define CSI_VC1CFGR1_DT0FT_SHIFT   CSI_VC1CFGR1_DT0FT_Pos
#define CSI_VC1CFGR2_DT1_SHIFT   CSI_VC1CFGR2_DT1_Pos
#define CSI_VC1CFGR2_DT1FT_SHIFT   CSI_VC1CFGR2_DT1FT_Pos
#define CSI_VC1CFGR2_DT2_SHIFT   CSI_VC1CFGR2_DT2_Pos
#define CSI_VC1CFGR2_DT2FT_SHIFT   CSI_VC1CFGR2_DT2FT_Pos
#define CSI_VC1CFGR3_DT3_SHIFT   CSI_VC1CFGR3_DT3_Pos
#define CSI_VC1CFGR3_DT3FT_SHIFT   CSI_VC1CFGR3_DT3FT_Pos
#define CSI_VC1CFGR3_DT4_SHIFT   CSI_VC1CFGR3_DT4_Pos
#define CSI_VC1CFGR3_DT4FT_SHIFT   CSI_VC1CFGR3_DT4FT_Pos
#define CSI_VC1CFGR4_DT5_SHIFT   CSI_VC1CFGR4_DT5_Pos
#define CSI_VC1CFGR4_DT5FT_SHIFT   CSI_VC1CFGR4_DT5FT_Pos
#define CSI_VC1CFGR4_DT6_SHIFT   CSI_VC1CFGR4_DT6_Pos
#define CSI_VC1CFGR4_DT6FT_SHIFT   CSI_VC1CFGR4_DT6FT_Pos
#define CSI_VC2CFGR1_ALLDT_SHIFT   CSI_VC2CFGR1_ALLDT_Pos
#define CSI_VC2CFGR1_DT0EN_SHIFT   CSI_VC2CFGR1_DT0EN_Pos
#define CSI_VC2CFGR1_DT1EN_SHIFT   CSI_VC2CFGR1_DT1EN_Pos
#define CSI_VC2CFGR1_DT2EN_SHIFT   CSI_VC2CFGR1_DT2EN_Pos
#define CSI_VC2CFGR1_DT3EN_SHIFT   CSI_VC2CFGR1_DT3EN_Pos
#define CSI_VC2CFGR1_DT4EN_SHIFT   CSI_VC2CFGR1_DT4EN_Pos
#define CSI_VC2CFGR1_DT5EN_SHIFT   CSI_VC2CFGR1_DT5EN_Pos
#define CSI_VC2CFGR1_DT6EN_SHIFT   CSI_VC2CFGR1_DT6EN_Pos
#define CSI_VC2CFGR1_CDTFT_SHIFT   CSI_VC2CFGR1_CDTFT_Pos
#define CSI_VC2CFGR1_DT0_SHIFT   CSI_VC2CFGR1_DT0_Pos
#define CSI_VC2CFGR1_DT0FT_SHIFT   CSI_VC2CFGR1_DT0FT_Pos
#define CSI_VC2CFGR2_DT1_SHIFT   CSI_VC2CFGR2_DT1_Pos
#define CSI_VC2CFGR2_DT1FT_SHIFT   CSI_VC2CFGR2_DT1FT_Pos
#define CSI_VC2CFGR2_DT2_SHIFT   CSI_VC2CFGR2_DT2_Pos
#define CSI_VC2CFGR2_DT2FT_SHIFT   CSI_VC2CFGR2_DT2FT_Pos
#define CSI_VC2CFGR3_DT3_SHIFT   CSI_VC2CFGR3_DT3_Pos
#define CSI_VC2CFGR3_DT3FT_SHIFT   CSI_VC2CFGR3_DT3FT_Pos
#define CSI_VC2CFGR3_DT4_SHIFT   CSI_VC2CFGR3_DT4_Pos
#define CSI_VC2CFGR3_DT4FT_SHIFT   CSI_VC2CFGR3_DT4FT_Pos
#define CSI_VC2CFGR4_DT5_SHIFT   CSI_VC2CFGR4_DT5_Pos
#define CSI_VC2CFGR4_DT5FT_SHIFT   CSI_VC2CFGR4_DT5FT_Pos
#define CSI_VC2CFGR4_DT6_SHIFT   CSI_VC2CFGR4_DT6_Pos
#define CSI_VC2CFGR4_DT6FT_SHIFT   CSI_VC2CFGR4_DT6FT_Pos
#define CSI_VC3CFGR1_ALLDT_SHIFT   CSI_VC3CFGR1_ALLDT_Pos
#define CSI_VC3CFGR1_DT0EN_SHIFT   CSI_VC3CFGR1_DT0EN_Pos
#define CSI_VC3CFGR1_DT1EN_SHIFT   CSI_VC3CFGR1_DT1EN_Pos
#define CSI_VC3CFGR1_DT2EN_SHIFT   CSI_VC3CFGR1_DT2EN_Pos
#define CSI_VC3CFGR1_DT3EN_SHIFT   CSI_VC3CFGR1_DT3EN_Pos
#define CSI_VC3CFGR1_DT4EN_SHIFT   CSI_VC3CFGR1_DT4EN_Pos
#define CSI_VC3CFGR1_DT5EN_SHIFT   CSI_VC3CFGR1_DT5EN_Pos
#define CSI_VC3CFGR1_DT6EN_SHIFT   CSI_VC3CFGR1_DT6EN_Pos
#define CSI_VC3CFGR1_CDTFT_SHIFT   CSI_VC3CFGR1_CDTFT_Pos
#define CSI_VC3CFGR1_DT0_SHIFT   CSI_VC3CFGR1_DT0_Pos
#define CSI_VC3CFGR1_DT0FT_SHIFT   CSI_VC3CFGR1_DT0FT_Pos
#define CSI_VC3CFGR2_DT1_SHIFT   CSI_VC3CFGR2_DT1_Pos
#define CSI_VC3CFGR2_DT1FT_SHIFT   CSI_VC3CFGR2_DT1FT_Pos
#define CSI_VC3CFGR2_DT2_SHIFT   CSI_VC3CFGR2_DT2_Pos
#define CSI_VC3CFGR2_DT2FT_SHIFT   CSI_VC3CFGR2_DT2FT_Pos
#define CSI_VC3CFGR3_DT3_SHIFT   CSI_VC3CFGR3_DT3_Pos
#define CSI_VC3CFGR3_DT3FT_SHIFT   CSI_VC3CFGR3_DT3FT_Pos
#define CSI_VC3CFGR3_DT4_SHIFT   CSI_VC3CFGR3_DT4_Pos
#define CSI_VC3CFGR3_DT4FT_SHIFT   CSI_VC3CFGR3_DT4FT_Pos
#define CSI_VC3CFGR4_DT5_SHIFT   CSI_VC3CFGR4_DT5_Pos
#define CSI_VC3CFGR4_DT5FT_SHIFT   CSI_VC3CFGR4_DT5FT_Pos
#define CSI_VC3CFGR4_DT6_SHIFT   CSI_VC3CFGR4_DT6_Pos
#define CSI_VC3CFGR4_DT6FT_SHIFT   CSI_VC3CFGR4_DT6FT_Pos
#define CSI_LB0CFGR_BYTECNT_SHIFT   CSI_LB0CFGR_BYTECNT_Pos
#define CSI_LB0CFGR_LINECNT_SHIFT   CSI_LB0CFGR_LINECNT_Pos
#define CSI_LB1CFGR_BYTECNT_SHIFT   CSI_LB1CFGR_BYTECNT_Pos
#define CSI_LB1CFGR_LINECNT_SHIFT   CSI_LB1CFGR_LINECNT_Pos
#define CSI_LB2CFGR_BYTECNT_SHIFT   CSI_LB2CFGR_BYTECNT_Pos
#define CSI_LB2CFGR_LINECNT_SHIFT   CSI_LB2CFGR_LINECNT_Pos
#define CSI_LB3CFGR_BYTECNT_SHIFT   CSI_LB3CFGR_BYTECNT_Pos
#define CSI_LB3CFGR_LINECNT_SHIFT   CSI_LB3CFGR_LINECNT_Pos
#define CSI_TIM0CFGR_COUNT_SHIFT   CSI_TIM0CFGR_COUNT_Pos
#define CSI_TIM1CFGR_COUNT_SHIFT   CSI_TIM1CFGR_COUNT_Pos
#define CSI_TIM2CFGR_COUNT_SHIFT   CSI_TIM2CFGR_COUNT_Pos
#define CSI_TIM3CFGR_COUNT_SHIFT   CSI_TIM3CFGR_COUNT_Pos
#define CSI_LMCFGR_LANENB_SHIFT   CSI_LMCFGR_LANENB_Pos
#define CSI_LMCFGR_DL0MAP_SHIFT   CSI_LMCFGR_DL0MAP_Pos
#define CSI_LMCFGR_DL1MAP_SHIFT   CSI_LMCFGR_DL1MAP_Pos
#define CSI_PRGITR_LB0VC_SHIFT   CSI_PRGITR_LB0VC_Pos
#define CSI_PRGITR_LB0EN_SHIFT   CSI_PRGITR_LB0EN_Pos
#define CSI_PRGITR_LB1VC_SHIFT   CSI_PRGITR_LB1VC_Pos
#define CSI_PRGITR_LB1EN_SHIFT   CSI_PRGITR_LB1EN_Pos
#define CSI_PRGITR_LB2VC_SHIFT   CSI_PRGITR_LB2VC_Pos
#define CSI_PRGITR_LB2EN_SHIFT   CSI_PRGITR_LB2EN_Pos
#define CSI_PRGITR_LB3VC_SHIFT   CSI_PRGITR_LB3VC_Pos
#define CSI_PRGITR_LB3EN_SHIFT   CSI_PRGITR_LB3EN_Pos
#define CSI_PRGITR_TIM0VC_SHIFT   CSI_PRGITR_TIM0VC_Pos
#define CSI_PRGITR_TIM0EOF_SHIFT   CSI_PRGITR_TIM0EOF_Pos
#define CSI_PRGITR_TIM0EN_SHIFT   CSI_PRGITR_TIM0EN_Pos
#define CSI_PRGITR_TIM1VC_SHIFT   CSI_PRGITR_TIM1VC_Pos
#define CSI_PRGITR_TIM1EOF_SHIFT   CSI_PRGITR_TIM1EOF_Pos
#define CSI_PRGITR_TIM1EN_SHIFT   CSI_PRGITR_TIM1EN_Pos
#define CSI_PRGITR_TIM2VC_SHIFT   CSI_PRGITR_TIM2VC_Pos
#define CSI_PRGITR_TIM2EOF_SHIFT   CSI_PRGITR_TIM2EOF_Pos
#define CSI_PRGITR_TIM2EN_SHIFT   CSI_PRGITR_TIM2EN_Pos
#define CSI_PRGITR_TIM3VC_SHIFT   CSI_PRGITR_TIM3VC_Pos
#define CSI_PRGITR_TIM3EOF_SHIFT   CSI_PRGITR_TIM3EOF_Pos
#define CSI_PRGITR_TIM3EN_SHIFT   CSI_PRGITR_TIM3EN_Pos
#define CSI_WDR_CNT_SHIFT   CSI_WDR_CNT_Pos
#define CSI_IER0_LB0IE_SHIFT   CSI_IER0_LB0IE_Pos
#define CSI_IER0_LB1IE_SHIFT   CSI_IER0_LB1IE_Pos
#define CSI_IER0_LB2IE_SHIFT   CSI_IER0_LB2IE_Pos
#define CSI_IER0_LB3IE_SHIFT   CSI_IER0_LB3IE_Pos
#define CSI_IER0_TIM0IE_SHIFT   CSI_IER0_TIM0IE_Pos
#define CSI_IER0_TIM1IE_SHIFT   CSI_IER0_TIM1IE_Pos
#define CSI_IER0_TIM2IE_SHIFT   CSI_IER0_TIM2IE_Pos
#define CSI_IER0_TIM3IE_SHIFT   CSI_IER0_TIM3IE_Pos
#define CSI_IER0_SOF0IE_SHIFT   CSI_IER0_SOF0IE_Pos
#define CSI_IER0_SOF1IE_SHIFT   CSI_IER0_SOF1IE_Pos
#define CSI_IER0_SOF2IE_SHIFT   CSI_IER0_SOF2IE_Pos
#define CSI_IER0_SOF3IE_SHIFT   CSI_IER0_SOF3IE_Pos
#define CSI_IER0_EOF0IE_SHIFT   CSI_IER0_EOF0IE_Pos
#define CSI_IER0_EOF1IE_SHIFT   CSI_IER0_EOF1IE_Pos
#define CSI_IER0_EOF2IE_SHIFT   CSI_IER0_EOF2IE_Pos
#define CSI_IER0_EOF3IE_SHIFT   CSI_IER0_EOF3IE_Pos
#define CSI_IER0_SPKTIE_SHIFT   CSI_IER0_SPKTIE_Pos
#define CSI_IER0_CCFIFOFIE_SHIFT   CSI_IER0_CCFIFOFIE_Pos
#define CSI_IER0_CRCERRIE_SHIFT   CSI_IER0_CRCERRIE_Pos
#define CSI_IER0_ECCERRIE_SHIFT   CSI_IER0_ECCERRIE_Pos
#define CSI_IER0_CECCERRIE_SHIFT   CSI_IER0_CECCERRIE_Pos
#define CSI_IER0_IDERRIE_SHIFT   CSI_IER0_IDERRIE_Pos
#define CSI_IER0_SPKTERRIE_SHIFT   CSI_IER0_SPKTERRIE_Pos
#define CSI_IER0_WDERRIE_SHIFT   CSI_IER0_WDERRIE_Pos
#define CSI_IER0_SYNCERRIE_SHIFT   CSI_IER0_SYNCERRIE_Pos
#define CSI_IER1_ESOTDL0IE_SHIFT   CSI_IER1_ESOTDL0IE_Pos
#define CSI_IER1_ESOTSYNCDL0IE_SHIFT   CSI_IER1_ESOTSYNCDL0IE_Pos
#define CSI_IER1_EESCDL0IE_SHIFT   CSI_IER1_EESCDL0IE_Pos
#define CSI_IER1_ESYNCESCDL0IE_SHIFT   CSI_IER1_ESYNCESCDL0IE_Pos
#define CSI_IER1_ECTRLDL0IE_SHIFT   CSI_IER1_ECTRLDL0IE_Pos
#define CSI_IER1_ESOTDL1IE_SHIFT   CSI_IER1_ESOTDL1IE_Pos
#define CSI_IER1_ESOTSYNCDL1IE_SHIFT   CSI_IER1_ESOTSYNCDL1IE_Pos
#define CSI_IER1_EESCDL1IE_SHIFT   CSI_IER1_EESCDL1IE_Pos
#define CSI_IER1_ESYNCESCDL1IE_SHIFT   CSI_IER1_ESYNCESCDL1IE_Pos
#define CSI_IER1_ECTRLDL1IE_SHIFT   CSI_IER1_ECTRLDL1IE_Pos
#define CSI_SR0_LB0F_SHIFT   CSI_SR0_LB0F_Pos
#define CSI_SR0_LB1F_SHIFT   CSI_SR0_LB1F_Pos
#define CSI_SR0_LB2F_SHIFT   CSI_SR0_LB2F_Pos
#define CSI_SR0_LB3F_SHIFT   CSI_SR0_LB3F_Pos
#define CSI_SR0_TIM0F_SHIFT   CSI_SR0_TIM0F_Pos
#define CSI_SR0_TIM1F_SHIFT   CSI_SR0_TIM1F_Pos
#define CSI_SR0_TIM2F_SHIFT   CSI_SR0_TIM2F_Pos
#define CSI_SR0_TIM3F_SHIFT   CSI_SR0_TIM3F_Pos
#define CSI_SR0_SOF0F_SHIFT   CSI_SR0_SOF0F_Pos
#define CSI_SR0_SOF1F_SHIFT   CSI_SR0_SOF1F_Pos
#define CSI_SR0_SOF2F_SHIFT   CSI_SR0_SOF2F_Pos
#define CSI_SR0_SOF3F_SHIFT   CSI_SR0_SOF3F_Pos
#define CSI_SR0_EOF0F_SHIFT   CSI_SR0_EOF0F_Pos
#define CSI_SR0_EOF1F_SHIFT   CSI_SR0_EOF1F_Pos
#define CSI_SR0_EOF2F_SHIFT   CSI_SR0_EOF2F_Pos
#define CSI_SR0_EOF3F_SHIFT   CSI_SR0_EOF3F_Pos
#define CSI_SR0_SPKTF_SHIFT   CSI_SR0_SPKTF_Pos
#define CSI_SR0_VC0STATEF_SHIFT   CSI_SR0_VC0STATEF_Pos
#define CSI_SR0_VC1STATEF_SHIFT   CSI_SR0_VC1STATEF_Pos
#define CSI_SR0_VC2STATEF_SHIFT   CSI_SR0_VC2STATEF_Pos
#define CSI_SR0_VC3STATEF_SHIFT   CSI_SR0_VC3STATEF_Pos
#define CSI_SR0_CCFIFOFF_SHIFT   CSI_SR0_CCFIFOFF_Pos
#define CSI_SR0_CRCERRF_SHIFT   CSI_SR0_CRCERRF_Pos
#define CSI_SR0_ECCERRF_SHIFT   CSI_SR0_ECCERRF_Pos
#define CSI_SR0_CECCERRF_SHIFT   CSI_SR0_CECCERRF_Pos
#define CSI_SR0_IDERRF_SHIFT   CSI_SR0_IDERRF_Pos
#define CSI_SR0_SPKTERRF_SHIFT   CSI_SR0_SPKTERRF_Pos
#define CSI_SR0_WDERRF_SHIFT   CSI_SR0_WDERRF_Pos
#define CSI_SR0_SYNCERRF_SHIFT   CSI_SR0_SYNCERRF_Pos
#define CSI_SR1_ESOTDL0F_SHIFT   CSI_SR1_ESOTDL0F_Pos
#define CSI_SR1_ESOTSYNCDL0F_SHIFT   CSI_SR1_ESOTSYNCDL0F_Pos
#define CSI_SR1_EESCDL0F_SHIFT   CSI_SR1_EESCDL0F_Pos
#define CSI_SR1_ESYNCESCDL0F_SHIFT   CSI_SR1_ESYNCESCDL0F_Pos
#define CSI_SR1_ECTRLDL0F_SHIFT   CSI_SR1_ECTRLDL0F_Pos
#define CSI_SR1_ESOTDL1F_SHIFT   CSI_SR1_ESOTDL1F_Pos
#define CSI_SR1_ESOTSYNCDL1F_SHIFT   CSI_SR1_ESOTSYNCDL1F_Pos
#define CSI_SR1_EESCDL1F_SHIFT   CSI_SR1_EESCDL1F_Pos
#define CSI_SR1_ESYNCESCDL1F_SHIFT   CSI_SR1_ESYNCESCDL1F_Pos
#define CSI_SR1_ECTRLDL1F_SHIFT   CSI_SR1_ECTRLDL1F_Pos
#define CSI_SR1_ACTDL0F_SHIFT   CSI_SR1_ACTDL0F_Pos
#define CSI_SR1_SYNCDL0F_SHIFT   CSI_SR1_SYNCDL0F_Pos
#define CSI_SR1_SKCALDL0F_SHIFT   CSI_SR1_SKCALDL0F_Pos
#define CSI_SR1_STOPDL0F_SHIFT   CSI_SR1_STOPDL0F_Pos
#define CSI_SR1_ULPNDL0F_SHIFT   CSI_SR1_ULPNDL0F_Pos
#define CSI_SR1_ACTDL1F_SHIFT   CSI_SR1_ACTDL1F_Pos
#define CSI_SR1_SYNCDL1F_SHIFT   CSI_SR1_SYNCDL1F_Pos
#define CSI_SR1_SKCALDL1F_SHIFT   CSI_SR1_SKCALDL1F_Pos
#define CSI_SR1_STOPDL1F_SHIFT   CSI_SR1_STOPDL1F_Pos
#define CSI_SR1_ULPNDL1F_SHIFT   CSI_SR1_ULPNDL1F_Pos
#define CSI_SR1_STOPCLF_SHIFT   CSI_SR1_STOPCLF_Pos
#define CSI_SR1_ULPNACTF_SHIFT   CSI_SR1_ULPNACTF_Pos
#define CSI_SR1_ULPNCLF_SHIFT   CSI_SR1_ULPNCLF_Pos
#define CSI_SR1_ACTCLF_SHIFT   CSI_SR1_ACTCLF_Pos
#define CSI_FCR0_CLB0F_SHIFT   CSI_FCR0_CLB0F_Pos
#define CSI_FCR0_CLB1F_SHIFT   CSI_FCR0_CLB1F_Pos
#define CSI_FCR0_CLB2F_SHIFT   CSI_FCR0_CLB2F_Pos
#define CSI_FCR0_CLB3F_SHIFT   CSI_FCR0_CLB3F_Pos
#define CSI_FCR0_CTIM0F_SHIFT   CSI_FCR0_CTIM0F_Pos
#define CSI_FCR0_CTIM1F_SHIFT   CSI_FCR0_CTIM1F_Pos
#define CSI_FCR0_CTIM2F_SHIFT   CSI_FCR0_CTIM2F_Pos
#define CSI_FCR0_CTIM3F_SHIFT   CSI_FCR0_CTIM3F_Pos
#define CSI_FCR0_CSOF0F_SHIFT   CSI_FCR0_CSOF0F_Pos
#define CSI_FCR0_CSOF1F_SHIFT   CSI_FCR0_CSOF1F_Pos
#define CSI_FCR0_CSOF2F_SHIFT   CSI_FCR0_CSOF2F_Pos
#define CSI_FCR0_CSOF3F_SHIFT   CSI_FCR0_CSOF3F_Pos
#define CSI_FCR0_CEOF0F_SHIFT   CSI_FCR0_CEOF0F_Pos
#define CSI_FCR0_CEOF1F_SHIFT   CSI_FCR0_CEOF1F_Pos
#define CSI_FCR0_CEOF2F_SHIFT   CSI_FCR0_CEOF2F_Pos
#define CSI_FCR0_CEOF3F_SHIFT   CSI_FCR0_CEOF3F_Pos
#define CSI_FCR0_CSPKTF_SHIFT   CSI_FCR0_CSPKTF_Pos
#define CSI_FCR0_CCCFIFOFF_SHIFT   CSI_FCR0_CCCFIFOFF_Pos
#define CSI_FCR0_CCRCERRF_SHIFT   CSI_FCR0_CCRCERRF_Pos
#define CSI_FCR0_CECCERRF_SHIFT   CSI_FCR0_CECCERRF_Pos
#define CSI_FCR0_CCECCERRF_SHIFT   CSI_FCR0_CCECCERRF_Pos
#define CSI_FCR0_CIDERRF_SHIFT   CSI_FCR0_CIDERRF_Pos
#define CSI_FCR0_CSPKTERRF_SHIFT   CSI_FCR0_CSPKTERRF_Pos
#define CSI_FCR0_CWDERRF_SHIFT   CSI_FCR0_CWDERRF_Pos
#define CSI_FCR0_CSYNCERRF_SHIFT   CSI_FCR0_CSYNCERRF_Pos
#define CSI_FCR1_CESOTDL0F_SHIFT   CSI_FCR1_CESOTDL0F_Pos
#define CSI_FCR1_CESOTSYNCDL0F_SHIFT   CSI_FCR1_CESOTSYNCDL0F_Pos
#define CSI_FCR1_CEESCDL0F_SHIFT   CSI_FCR1_CEESCDL0F_Pos
#define CSI_FCR1_CESYNCESCDL0F_SHIFT   CSI_FCR1_CESYNCESCDL0F_Pos
#define CSI_FCR1_CECTRLDL0F_SHIFT   CSI_FCR1_CECTRLDL0F_Pos
#define CSI_FCR1_CESOTDL1F_SHIFT   CSI_FCR1_CESOTDL1F_Pos
#define CSI_FCR1_CESOTSYNCDL1F_SHIFT   CSI_FCR1_CESOTSYNCDL1F_Pos
#define CSI_FCR1_CEESCDL1F_SHIFT   CSI_FCR1_CEESCDL1F_Pos
#define CSI_FCR1_CESYNCESCDL1F_SHIFT   CSI_FCR1_CESYNCESCDL1F_Pos
#define CSI_FCR1_CECTRLDL1F_SHIFT   CSI_FCR1_CECTRLDL1F_Pos
#define CSI_SPDFR_DATAFIELD_SHIFT   CSI_SPDFR_DATAFIELD_Pos
#define CSI_SPDFR_DATATYPE_SHIFT   CSI_SPDFR_DATATYPE_Pos
#define CSI_SPDFR_VCHANNEL_SHIFT   CSI_SPDFR_VCHANNEL_Pos
#define CSI_ERR1_CRCDTERR_SHIFT   CSI_ERR1_CRCDTERR_Pos
#define CSI_ERR1_CRCVCERR_SHIFT   CSI_ERR1_CRCVCERR_Pos
#define CSI_ERR1_CECCDTERR_SHIFT   CSI_ERR1_CECCDTERR_Pos
#define CSI_ERR1_CECCVCERR_SHIFT   CSI_ERR1_CECCVCERR_Pos
#define CSI_ERR1_IDDTERR_SHIFT   CSI_ERR1_IDDTERR_Pos
#define CSI_ERR1_IDVCERR_SHIFT   CSI_ERR1_IDVCERR_Pos
#define CSI_ERR2_SPKTDTERR_SHIFT   CSI_ERR2_SPKTDTERR_Pos
#define CSI_ERR2_SPKTVCERR_SHIFT   CSI_ERR2_SPKTVCERR_Pos
#define CSI_ERR2_WDVCERR_SHIFT   CSI_ERR2_WDVCERR_Pos
#define CSI_ERR2_SYNCVCERR_SHIFT   CSI_ERR2_SYNCVCERR_Pos
#define CSI_PRCR_PEN_SHIFT   CSI_PRCR_PEN_Pos
#define CSI_PMCR_FRXMDL0_SHIFT   CSI_PMCR_FRXMDL0_Pos
#define CSI_PMCR_FRXMDL1_SHIFT   CSI_PMCR_FRXMDL1_Pos
#define CSI_PMCR_FTXSMDL0_SHIFT   CSI_PMCR_FTXSMDL0_Pos
#define CSI_PMCR_DTDL_SHIFT   CSI_PMCR_DTDL_Pos
#define CSI_PMCR_RTDL0_SHIFT   CSI_PMCR_RTDL0_Pos
#define CSI_PMCR_TUESDL0_SHIFT   CSI_PMCR_TUESDL0_Pos
#define CSI_PMCR_TUEXDL0_SHIFT   CSI_PMCR_TUEXDL0_Pos
#define CSI_PFCR_CCFR_SHIFT   CSI_PFCR_CCFR_Pos
#define CSI_PFCR_HSFR_SHIFT   CSI_PFCR_HSFR_Pos
#define CSI_PFCR_DLD_SHIFT   CSI_PFCR_DLD_Pos
#define CSI_PTCR0_TCKEN_SHIFT   CSI_PTCR0_TCKEN_Pos
#define CSI_PTCR0_TRSEN_SHIFT   CSI_PTCR0_TRSEN_Pos
#define CSI_PTCR1_TDI_SHIFT   CSI_PTCR1_TDI_Pos
#define CSI_PTCR1_TWM_SHIFT   CSI_PTCR1_TWM_Pos
#define CSI_PTSR_TDO_SHIFT   CSI_PTSR_TDO_Pos
#define DCMIPP_IPGR1_MEMORYPAGE_SHIFT   DCMIPP_IPGR1_MEMORYPAGE_Pos
#define DCMIPP_IPGR1_QOS_MODE_SHIFT   DCMIPP_IPGR1_QOS_MODE_Pos
#define DCMIPP_IPGR2_PSTART_SHIFT   DCMIPP_IPGR2_PSTART_Pos
#define DCMIPP_IPGR3_IDLE_SHIFT   DCMIPP_IPGR3_IDLE_Pos
#define DCMIPP_IPGR8_DID_SHIFT   DCMIPP_IPGR8_DID_Pos
#define DCMIPP_IPGR8_REVID_SHIFT   DCMIPP_IPGR8_REVID_Pos
#define DCMIPP_IPGR8_ARCHIID_SHIFT   DCMIPP_IPGR8_ARCHIID_Pos
#define DCMIPP_IPGR8_IPPID_SHIFT   DCMIPP_IPGR8_IPPID_Pos
#define DCMIPP_IPC1R1_TRAFFIC_SHIFT   DCMIPP_IPC1R1_TRAFFIC_Pos
#define DCMIPP_IPC1R1_OTR_SHIFT   DCMIPP_IPC1R1_OTR_Pos
#define DCMIPP_IPC1R2_WLRU_SHIFT   DCMIPP_IPC1R2_WLRU_Pos
#define DCMIPP_IPC1R3_DPREGSTART_SHIFT   DCMIPP_IPC1R3_DPREGSTART_Pos
#define DCMIPP_IPC1R3_DPREGEND_SHIFT   DCMIPP_IPC1R3_DPREGEND_Pos
#define DCMIPP_IPC2R1_TRAFFIC_SHIFT   DCMIPP_IPC2R1_TRAFFIC_Pos
#define DCMIPP_IPC2R1_OTR_SHIFT   DCMIPP_IPC2R1_OTR_Pos
#define DCMIPP_IPC2R2_WLRU_SHIFT   DCMIPP_IPC2R2_WLRU_Pos
#define DCMIPP_IPC2R3_DPREGSTART_SHIFT   DCMIPP_IPC2R3_DPREGSTART_Pos
#define DCMIPP_IPC2R3_DPREGEND_SHIFT   DCMIPP_IPC2R3_DPREGEND_Pos
#define DCMIPP_IPC3R1_TRAFFIC_SHIFT   DCMIPP_IPC3R1_TRAFFIC_Pos
#define DCMIPP_IPC3R1_OTR_SHIFT   DCMIPP_IPC3R1_OTR_Pos
#define DCMIPP_IPC3R2_WLRU_SHIFT   DCMIPP_IPC3R2_WLRU_Pos
#define DCMIPP_IPC3R3_DPREGSTART_SHIFT   DCMIPP_IPC3R3_DPREGSTART_Pos
#define DCMIPP_IPC3R3_DPREGEND_SHIFT   DCMIPP_IPC3R3_DPREGEND_Pos
#define DCMIPP_IPC4R1_TRAFFIC_SHIFT   DCMIPP_IPC4R1_TRAFFIC_Pos
#define DCMIPP_IPC4R1_OTR_SHIFT   DCMIPP_IPC4R1_OTR_Pos
#define DCMIPP_IPC4R2_WLRU_SHIFT   DCMIPP_IPC4R2_WLRU_Pos
#define DCMIPP_IPC4R3_DPREGSTART_SHIFT   DCMIPP_IPC4R3_DPREGSTART_Pos
#define DCMIPP_IPC4R3_DPREGEND_SHIFT   DCMIPP_IPC4R3_DPREGEND_Pos
#define DCMIPP_IPC5R1_TRAFFIC_SHIFT   DCMIPP_IPC5R1_TRAFFIC_Pos
#define DCMIPP_IPC5R1_OTR_SHIFT   DCMIPP_IPC5R1_OTR_Pos
#define DCMIPP_IPC5R2_WLRU_SHIFT   DCMIPP_IPC5R2_WLRU_Pos
#define DCMIPP_IPC5R3_DPREGSTART_SHIFT   DCMIPP_IPC5R3_DPREGSTART_Pos
#define DCMIPP_IPC5R3_DPREGEND_SHIFT   DCMIPP_IPC5R3_DPREGEND_Pos
#define DCMIPP_PRCR_ESS_SHIFT   DCMIPP_PRCR_ESS_Pos
#define DCMIPP_PRCR_PCKPOL_SHIFT   DCMIPP_PRCR_PCKPOL_Pos
#define DCMIPP_PRCR_HSPOL_SHIFT   DCMIPP_PRCR_HSPOL_Pos
#define DCMIPP_PRCR_VSPOL_SHIFT   DCMIPP_PRCR_VSPOL_Pos
#define DCMIPP_PRCR_EDM_SHIFT   DCMIPP_PRCR_EDM_Pos
#define DCMIPP_PRCR_ENABLE_SHIFT   DCMIPP_PRCR_ENABLE_Pos
#define DCMIPP_PRCR_FORMAT_SHIFT   DCMIPP_PRCR_FORMAT_Pos
#define DCMIPP_PRCR_SWAPCYCLES_SHIFT   DCMIPP_PRCR_SWAPCYCLES_Pos
#define DCMIPP_PRCR_SWAPBITS_SHIFT   DCMIPP_PRCR_SWAPBITS_Pos
#define DCMIPP_PRESCR_FSC_SHIFT   DCMIPP_PRESCR_FSC_Pos
#define DCMIPP_PRESCR_LSC_SHIFT   DCMIPP_PRESCR_LSC_Pos
#define DCMIPP_PRESCR_LEC_SHIFT   DCMIPP_PRESCR_LEC_Pos
#define DCMIPP_PRESCR_FEC_SHIFT   DCMIPP_PRESCR_FEC_Pos
#define DCMIPP_PRESUR_FSU_SHIFT   DCMIPP_PRESUR_FSU_Pos
#define DCMIPP_PRESUR_LSU_SHIFT   DCMIPP_PRESUR_LSU_Pos
#define DCMIPP_PRESUR_LEU_SHIFT   DCMIPP_PRESUR_LEU_Pos
#define DCMIPP_PRESUR_FEU_SHIFT   DCMIPP_PRESUR_FEU_Pos
#define DCMIPP_PRIER_ERRIE_SHIFT   DCMIPP_PRIER_ERRIE_Pos
#define DCMIPP_PRSR_ERRF_SHIFT   DCMIPP_PRSR_ERRF_Pos
#define DCMIPP_PRSR_HSYNC_SHIFT   DCMIPP_PRSR_HSYNC_Pos
#define DCMIPP_PRSR_VSYNC_SHIFT   DCMIPP_PRSR_VSYNC_Pos
#define DCMIPP_PRFCR_CERRF_SHIFT   DCMIPP_PRFCR_CERRF_Pos
#define DCMIPP_CMCR_INSEL_SHIFT   DCMIPP_CMCR_INSEL_Pos
#define DCMIPP_CMCR_PSFC_SHIFT   DCMIPP_CMCR_PSFC_Pos
#define DCMIPP_CMCR_CFC_SHIFT   DCMIPP_CMCR_CFC_Pos
#define DCMIPP_CMCR_SWAPRB_SHIFT   DCMIPP_CMCR_SWAPRB_Pos
#define DCMIPP_CMFRCR_FRMCNT_SHIFT   DCMIPP_CMFRCR_FRMCNT_Pos
#define DCMIPP_CMIER_ATXERRIE_SHIFT   DCMIPP_CMIER_ATXERRIE_Pos
#define DCMIPP_CMIER_PRERRIE_SHIFT   DCMIPP_CMIER_PRERRIE_Pos
#define DCMIPP_CMIER_P0LINEIE_SHIFT   DCMIPP_CMIER_P0LINEIE_Pos
#define DCMIPP_CMIER_P0FRAMEIE_SHIFT   DCMIPP_CMIER_P0FRAMEIE_Pos
#define DCMIPP_CMIER_P0VSYNCIE_SHIFT   DCMIPP_CMIER_P0VSYNCIE_Pos
#define DCMIPP_CMIER_P0LIMITIE_SHIFT   DCMIPP_CMIER_P0LIMITIE_Pos
#define DCMIPP_CMIER_P0OVRIE_SHIFT   DCMIPP_CMIER_P0OVRIE_Pos
#define DCMIPP_CMIER_P1LINEIE_SHIFT   DCMIPP_CMIER_P1LINEIE_Pos
#define DCMIPP_CMIER_P1FRAMEIE_SHIFT   DCMIPP_CMIER_P1FRAMEIE_Pos
#define DCMIPP_CMIER_P1VSYNCIE_SHIFT   DCMIPP_CMIER_P1VSYNCIE_Pos
#define DCMIPP_CMIER_P1OVRIE_SHIFT   DCMIPP_CMIER_P1OVRIE_Pos
#define DCMIPP_CMIER_P2LINEIE_SHIFT   DCMIPP_CMIER_P2LINEIE_Pos
#define DCMIPP_CMIER_P2FRAMEIE_SHIFT   DCMIPP_CMIER_P2FRAMEIE_Pos
#define DCMIPP_CMIER_P2VSYNCIE_SHIFT   DCMIPP_CMIER_P2VSYNCIE_Pos
#define DCMIPP_CMIER_P2OVRIE_SHIFT   DCMIPP_CMIER_P2OVRIE_Pos
#define DCMIPP_CMSR1_PRHSYNC_SHIFT   DCMIPP_CMSR1_PRHSYNC_Pos
#define DCMIPP_CMSR1_PRVSYNC_SHIFT   DCMIPP_CMSR1_PRVSYNC_Pos
#define DCMIPP_CMSR1_P0LSTLINE_SHIFT   DCMIPP_CMSR1_P0LSTLINE_Pos
#define DCMIPP_CMSR1_P0LSTFRM_SHIFT   DCMIPP_CMSR1_P0LSTFRM_Pos
#define DCMIPP_CMSR1_P0CPTACT_SHIFT   DCMIPP_CMSR1_P0CPTACT_Pos
#define DCMIPP_CMSR1_P1LSTLINE_SHIFT   DCMIPP_CMSR1_P1LSTLINE_Pos
#define DCMIPP_CMSR1_P1LSTFRM_SHIFT   DCMIPP_CMSR1_P1LSTFRM_Pos
#define DCMIPP_CMSR1_P1CPTACT_SHIFT   DCMIPP_CMSR1_P1CPTACT_Pos
#define DCMIPP_CMSR1_P2LSTLINE_SHIFT   DCMIPP_CMSR1_P2LSTLINE_Pos
#define DCMIPP_CMSR1_P2LSTFRM_SHIFT   DCMIPP_CMSR1_P2LSTFRM_Pos
#define DCMIPP_CMSR1_P2CPTACT_SHIFT   DCMIPP_CMSR1_P2CPTACT_Pos
#define DCMIPP_CMSR2_ATXERRF_SHIFT   DCMIPP_CMSR2_ATXERRF_Pos
#define DCMIPP_CMSR2_PRERRF_SHIFT   DCMIPP_CMSR2_PRERRF_Pos
#define DCMIPP_CMSR2_P0LINEF_SHIFT   DCMIPP_CMSR2_P0LINEF_Pos
#define DCMIPP_CMSR2_P0FRAMEF_SHIFT   DCMIPP_CMSR2_P0FRAMEF_Pos
#define DCMIPP_CMSR2_P0VSYNCF_SHIFT   DCMIPP_CMSR2_P0VSYNCF_Pos
#define DCMIPP_CMSR2_P0LIMITF_SHIFT   DCMIPP_CMSR2_P0LIMITF_Pos
#define DCMIPP_CMSR2_P0OVRF_SHIFT   DCMIPP_CMSR2_P0OVRF_Pos
#define DCMIPP_CMSR2_P1LINEF_SHIFT   DCMIPP_CMSR2_P1LINEF_Pos
#define DCMIPP_CMSR2_P1FRAMEF_SHIFT   DCMIPP_CMSR2_P1FRAMEF_Pos
#define DCMIPP_CMSR2_P1VSYNCF_SHIFT   DCMIPP_CMSR2_P1VSYNCF_Pos
#define DCMIPP_CMSR2_P1OVRF_SHIFT   DCMIPP_CMSR2_P1OVRF_Pos
#define DCMIPP_CMSR2_P2LINEF_SHIFT   DCMIPP_CMSR2_P2LINEF_Pos
#define DCMIPP_CMSR2_P2FRAMEF_SHIFT   DCMIPP_CMSR2_P2FRAMEF_Pos
#define DCMIPP_CMSR2_P2VSYNCF_SHIFT   DCMIPP_CMSR2_P2VSYNCF_Pos
#define DCMIPP_CMSR2_P2OVRF_SHIFT   DCMIPP_CMSR2_P2OVRF_Pos
#define DCMIPP_CMFCR_CATXERRF_SHIFT   DCMIPP_CMFCR_CATXERRF_Pos
#define DCMIPP_CMFCR_CPRERRF_SHIFT   DCMIPP_CMFCR_CPRERRF_Pos
#define DCMIPP_CMFCR_CP0LINEF_SHIFT   DCMIPP_CMFCR_CP0LINEF_Pos
#define DCMIPP_CMFCR_CP0FRAMEF_SHIFT   DCMIPP_CMFCR_CP0FRAMEF_Pos
#define DCMIPP_CMFCR_CP0VSYNCF_SHIFT   DCMIPP_CMFCR_CP0VSYNCF_Pos
#define DCMIPP_CMFCR_CP0LIMITF_SHIFT   DCMIPP_CMFCR_CP0LIMITF_Pos
#define DCMIPP_CMFCR_CP0OVRF_SHIFT   DCMIPP_CMFCR_CP0OVRF_Pos
#define DCMIPP_CMFCR_CP1LINEF_SHIFT   DCMIPP_CMFCR_CP1LINEF_Pos
#define DCMIPP_CMFCR_CP1FRAMEF_SHIFT   DCMIPP_CMFCR_CP1FRAMEF_Pos
#define DCMIPP_CMFCR_CP1VSYNCF_SHIFT   DCMIPP_CMFCR_CP1VSYNCF_Pos
#define DCMIPP_CMFCR_CP1OVRF_SHIFT   DCMIPP_CMFCR_CP1OVRF_Pos
#define DCMIPP_CMFCR_CP2LINEF_SHIFT   DCMIPP_CMFCR_CP2LINEF_Pos
#define DCMIPP_CMFCR_CP2FRAMEF_SHIFT   DCMIPP_CMFCR_CP2FRAMEF_Pos
#define DCMIPP_CMFCR_CP2VSYNCF_SHIFT   DCMIPP_CMFCR_CP2VSYNCF_Pos
#define DCMIPP_CMFCR_CP2OVRF_SHIFT   DCMIPP_CMFCR_CP2OVRF_Pos
#define DCMIPP_P0FSCR_DTIDA_SHIFT   DCMIPP_P0FSCR_DTIDA_Pos
#define DCMIPP_P0FSCR_DTIDB_SHIFT   DCMIPP_P0FSCR_DTIDB_Pos
#define DCMIPP_P0FSCR_DTMODE_SHIFT   DCMIPP_P0FSCR_DTMODE_Pos
#define DCMIPP_P0FSCR_VC_SHIFT   DCMIPP_P0FSCR_VC_Pos
#define DCMIPP_P0FSCR_PIPEN_SHIFT   DCMIPP_P0FSCR_PIPEN_Pos
#define DCMIPP_P0FCTCR_FRATE_SHIFT   DCMIPP_P0FCTCR_FRATE_Pos
#define DCMIPP_P0FCTCR_CPTMODE_SHIFT   DCMIPP_P0FCTCR_CPTMODE_Pos
#define DCMIPP_P0FCTCR_CPTREQ_SHIFT   DCMIPP_P0FCTCR_CPTREQ_Pos
#define DCMIPP_P0SCSTR_HSTART_SHIFT   DCMIPP_P0SCSTR_HSTART_Pos
#define DCMIPP_P0SCSTR_VSTART_SHIFT   DCMIPP_P0SCSTR_VSTART_Pos
#define DCMIPP_P0SCSZR_HSIZE_SHIFT   DCMIPP_P0SCSZR_HSIZE_Pos
#define DCMIPP_P0SCSZR_VSIZE_SHIFT   DCMIPP_P0SCSZR_VSIZE_Pos
#define DCMIPP_P0SCSZR_POSNEG_SHIFT   DCMIPP_P0SCSZR_POSNEG_Pos
#define DCMIPP_P0SCSZR_ENABLE_SHIFT   DCMIPP_P0SCSZR_ENABLE_Pos
#define DCMIPP_P0DCCNTR_CNT_SHIFT   DCMIPP_P0DCCNTR_CNT_Pos
#define DCMIPP_P0DCLMTR_LIMIT_SHIFT   DCMIPP_P0DCLMTR_LIMIT_Pos
#define DCMIPP_P0DCLMTR_ENABLE_SHIFT   DCMIPP_P0DCLMTR_ENABLE_Pos
#define DCMIPP_P0PPCR_SWAPYUV_SHIFT   DCMIPP_P0PPCR_SWAPYUV_Pos
#define DCMIPP_P0PPCR_PAD_SHIFT   DCMIPP_P0PPCR_PAD_Pos
#define DCMIPP_P0PPCR_HEADEREN_SHIFT   DCMIPP_P0PPCR_HEADEREN_Pos
#define DCMIPP_P0PPCR_BSM_SHIFT   DCMIPP_P0PPCR_BSM_Pos
#define DCMIPP_P0PPCR_OEBS_SHIFT   DCMIPP_P0PPCR_OEBS_Pos
#define DCMIPP_P0PPCR_LSM_SHIFT   DCMIPP_P0PPCR_LSM_Pos
#define DCMIPP_P0PPCR_OELS_SHIFT   DCMIPP_P0PPCR_OELS_Pos
#define DCMIPP_P0PPCR_LINEMULT_SHIFT   DCMIPP_P0PPCR_LINEMULT_Pos
#define DCMIPP_P0PPCR_DBM_SHIFT   DCMIPP_P0PPCR_DBM_Pos
#define DCMIPP_P0PPM0AR1_M0A_SHIFT   DCMIPP_P0PPM0AR1_M0A_Pos
#define DCMIPP_P0PPM0AR2_M0A_SHIFT   DCMIPP_P0PPM0AR2_M0A_Pos
#define DCMIPP_P0IER_LINEIE_SHIFT   DCMIPP_P0IER_LINEIE_Pos
#define DCMIPP_P0IER_FRAMEIE_SHIFT   DCMIPP_P0IER_FRAMEIE_Pos
#define DCMIPP_P0IER_VSYNCIE_SHIFT   DCMIPP_P0IER_VSYNCIE_Pos
#define DCMIPP_P0IER_LIMITIE_SHIFT   DCMIPP_P0IER_LIMITIE_Pos
#define DCMIPP_P0IER_OVRIE_SHIFT   DCMIPP_P0IER_OVRIE_Pos
#define DCMIPP_P0SR_LINEF_SHIFT   DCMIPP_P0SR_LINEF_Pos
#define DCMIPP_P0SR_FRAMEF_SHIFT   DCMIPP_P0SR_FRAMEF_Pos
#define DCMIPP_P0SR_VSYNCF_SHIFT   DCMIPP_P0SR_VSYNCF_Pos
#define DCMIPP_P0SR_LIMITF_SHIFT   DCMIPP_P0SR_LIMITF_Pos
#define DCMIPP_P0SR_OVRF_SHIFT   DCMIPP_P0SR_OVRF_Pos
#define DCMIPP_P0SR_LSTLINE_SHIFT   DCMIPP_P0SR_LSTLINE_Pos
#define DCMIPP_P0SR_LSTFRM_SHIFT   DCMIPP_P0SR_LSTFRM_Pos
#define DCMIPP_P0SR_CPTACT_SHIFT   DCMIPP_P0SR_CPTACT_Pos
#define DCMIPP_P0FCR_CLINEF_SHIFT   DCMIPP_P0FCR_CLINEF_Pos
#define DCMIPP_P0FCR_CFRAMEF_SHIFT   DCMIPP_P0FCR_CFRAMEF_Pos
#define DCMIPP_P0FCR_CVSYNCF_SHIFT   DCMIPP_P0FCR_CVSYNCF_Pos
#define DCMIPP_P0FCR_CLIMITF_SHIFT   DCMIPP_P0FCR_CLIMITF_Pos
#define DCMIPP_P0FCR_COVRF_SHIFT   DCMIPP_P0FCR_COVRF_Pos
#define DCMIPP_P0CFSCR_DTIDA_SHIFT   DCMIPP_P0CFSCR_DTIDA_Pos
#define DCMIPP_P0CFSCR_DTIDB_SHIFT   DCMIPP_P0CFSCR_DTIDB_Pos
#define DCMIPP_P0CFSCR_DTMODE_SHIFT   DCMIPP_P0CFSCR_DTMODE_Pos
#define DCMIPP_P0CFSCR_VC_SHIFT   DCMIPP_P0CFSCR_VC_Pos
#define DCMIPP_P0CFSCR_PIPEN_SHIFT   DCMIPP_P0CFSCR_PIPEN_Pos
#define DCMIPP_P0CFCTCR_FRATE_SHIFT   DCMIPP_P0CFCTCR_FRATE_Pos
#define DCMIPP_P0CFCTCR_CPTMODE_SHIFT   DCMIPP_P0CFCTCR_CPTMODE_Pos
#define DCMIPP_P0CFCTCR_CPTREQ_SHIFT   DCMIPP_P0CFCTCR_CPTREQ_Pos
#define DCMIPP_P0CSCSTR_HSTART_SHIFT   DCMIPP_P0CSCSTR_HSTART_Pos
#define DCMIPP_P0CSCSTR_VSTART_SHIFT   DCMIPP_P0CSCSTR_VSTART_Pos
#define DCMIPP_P0CSCSZR_HSIZE_SHIFT   DCMIPP_P0CSCSZR_HSIZE_Pos
#define DCMIPP_P0CSCSZR_VSIZE_SHIFT   DCMIPP_P0CSCSZR_VSIZE_Pos
#define DCMIPP_P0CSCSZR_POSNEG_SHIFT   DCMIPP_P0CSCSZR_POSNEG_Pos
#define DCMIPP_P0CSCSZR_ENABLE_SHIFT   DCMIPP_P0CSCSZR_ENABLE_Pos
#define DCMIPP_P0CPPCR_SWAPYUV_SHIFT   DCMIPP_P0CPPCR_SWAPYUV_Pos
#define DCMIPP_P0CPPCR_PAD_SHIFT   DCMIPP_P0CPPCR_PAD_Pos
#define DCMIPP_P0CPPCR_HEADEREN_SHIFT   DCMIPP_P0CPPCR_HEADEREN_Pos
#define DCMIPP_P0CPPCR_BSM_SHIFT   DCMIPP_P0CPPCR_BSM_Pos
#define DCMIPP_P0CPPCR_OEBS_SHIFT   DCMIPP_P0CPPCR_OEBS_Pos
#define DCMIPP_P0CPPCR_LSM_SHIFT   DCMIPP_P0CPPCR_LSM_Pos
#define DCMIPP_P0CPPCR_OELS_SHIFT   DCMIPP_P0CPPCR_OELS_Pos
#define DCMIPP_P0CPPCR_LINEMULT_SHIFT   DCMIPP_P0CPPCR_LINEMULT_Pos
#define DCMIPP_P0CPPCR_DBM_SHIFT   DCMIPP_P0CPPCR_DBM_Pos
#define DCMIPP_P0CPPM0AR1_M0A_SHIFT   DCMIPP_P0CPPM0AR1_M0A_Pos
#define DCMIPP_P1FSCR_DTIDA_SHIFT   DCMIPP_P1FSCR_DTIDA_Pos
#define DCMIPP_P1FSCR_DTIDB_SHIFT   DCMIPP_P1FSCR_DTIDB_Pos
#define DCMIPP_P1FSCR_DTMODE_SHIFT   DCMIPP_P1FSCR_DTMODE_Pos
#define DCMIPP_P1FSCR_PIPEDIFF_SHIFT   DCMIPP_P1FSCR_PIPEDIFF_Pos
#define DCMIPP_P1FSCR_VC_SHIFT   DCMIPP_P1FSCR_VC_Pos
#define DCMIPP_P1FSCR_FDTF_SHIFT   DCMIPP_P1FSCR_FDTF_Pos
#define DCMIPP_P1FSCR_FDTFEN_SHIFT   DCMIPP_P1FSCR_FDTFEN_Pos
#define DCMIPP_P1FSCR_PIPEN_SHIFT   DCMIPP_P1FSCR_PIPEN_Pos
#define DCMIPP_P1SRCR_LASTLINE_SHIFT   DCMIPP_P1SRCR_LASTLINE_Pos
#define DCMIPP_P1SRCR_FIRSTLINEDEL_SHIFT   DCMIPP_P1SRCR_FIRSTLINEDEL_Pos
#define DCMIPP_P1SRCR_CROPEN_SHIFT   DCMIPP_P1SRCR_CROPEN_Pos
#define DCMIPP_P1BPRCR_ENABLE_SHIFT   DCMIPP_P1BPRCR_ENABLE_Pos
#define DCMIPP_P1BPRCR_STRENGTH_SHIFT   DCMIPP_P1BPRCR_STRENGTH_Pos
#define DCMIPP_P1BPRSR_BADCNT_SHIFT   DCMIPP_P1BPRSR_BADCNT_Pos
#define DCMIPP_P1DECR_ENABLE_SHIFT   DCMIPP_P1DECR_ENABLE_Pos
#define DCMIPP_P1DECR_HDEC_SHIFT   DCMIPP_P1DECR_HDEC_Pos
#define DCMIPP_P1DECR_VDEC_SHIFT   DCMIPP_P1DECR_VDEC_Pos
#define DCMIPP_P1BLCCR_ENABLE_SHIFT   DCMIPP_P1BLCCR_ENABLE_Pos
#define DCMIPP_P1BLCCR_BLCB_SHIFT   DCMIPP_P1BLCCR_BLCB_Pos
#define DCMIPP_P1BLCCR_BLCG_SHIFT   DCMIPP_P1BLCCR_BLCG_Pos
#define DCMIPP_P1BLCCR_BLCR_SHIFT   DCMIPP_P1BLCCR_BLCR_Pos
#define DCMIPP_P1EXCR1_ENABLE_SHIFT   DCMIPP_P1EXCR1_ENABLE_Pos
#define DCMIPP_P1EXCR1_MULTR_SHIFT   DCMIPP_P1EXCR1_MULTR_Pos
#define DCMIPP_P1EXCR1_SHFR_SHIFT   DCMIPP_P1EXCR1_SHFR_Pos
#define DCMIPP_P1EXCR2_MULTB_SHIFT   DCMIPP_P1EXCR2_MULTB_Pos
#define DCMIPP_P1EXCR2_SHFB_SHIFT   DCMIPP_P1EXCR2_SHFB_Pos
#define DCMIPP_P1EXCR2_MULTG_SHIFT   DCMIPP_P1EXCR2_MULTG_Pos
#define DCMIPP_P1EXCR2_SHFG_SHIFT   DCMIPP_P1EXCR2_SHFG_Pos
#define DCMIPP_P1ST1CR_ENABLE_SHIFT   DCMIPP_P1ST1CR_ENABLE_Pos
#define DCMIPP_P1ST1CR_BINS_SHIFT   DCMIPP_P1ST1CR_BINS_Pos
#define DCMIPP_P1ST1CR_SRC_SHIFT   DCMIPP_P1ST1CR_SRC_Pos
#define DCMIPP_P1ST1CR_MODE_SHIFT   DCMIPP_P1ST1CR_MODE_Pos
#define DCMIPP_P1ST2CR_ENABLE_SHIFT   DCMIPP_P1ST2CR_ENABLE_Pos
#define DCMIPP_P1ST2CR_BINS_SHIFT   DCMIPP_P1ST2CR_BINS_Pos
#define DCMIPP_P1ST2CR_SRC_SHIFT   DCMIPP_P1ST2CR_SRC_Pos
#define DCMIPP_P1ST2CR_MODE_SHIFT   DCMIPP_P1ST2CR_MODE_Pos
#define DCMIPP_P1ST3CR_ENABLE_SHIFT   DCMIPP_P1ST3CR_ENABLE_Pos
#define DCMIPP_P1ST3CR_BINS_SHIFT   DCMIPP_P1ST3CR_BINS_Pos
#define DCMIPP_P1ST3CR_SRC_SHIFT   DCMIPP_P1ST3CR_SRC_Pos
#define DCMIPP_P1ST3CR_MODE_SHIFT   DCMIPP_P1ST3CR_MODE_Pos
#define DCMIPP_P1STSTR_HSTART_SHIFT   DCMIPP_P1STSTR_HSTART_Pos
#define DCMIPP_P1STSTR_VSTART_SHIFT   DCMIPP_P1STSTR_VSTART_Pos
#define DCMIPP_P1STSZR_HSIZE_SHIFT   DCMIPP_P1STSZR_HSIZE_Pos
#define DCMIPP_P1STSZR_VSIZE_SHIFT   DCMIPP_P1STSZR_VSIZE_Pos
#define DCMIPP_P1STSZR_CROPEN_SHIFT   DCMIPP_P1STSZR_CROPEN_Pos
#define DCMIPP_P1ST1SR_ACCU_SHIFT   DCMIPP_P1ST1SR_ACCU_Pos
#define DCMIPP_P1ST2SR_ACCU_SHIFT   DCMIPP_P1ST2SR_ACCU_Pos
#define DCMIPP_P1ST3SR_ACCU_SHIFT   DCMIPP_P1ST3SR_ACCU_Pos
#define DCMIPP_P1DMCR_ENABLE_SHIFT   DCMIPP_P1DMCR_ENABLE_Pos
#define DCMIPP_P1DMCR_TYPE_SHIFT   DCMIPP_P1DMCR_TYPE_Pos
#define DCMIPP_P1DMCR_PEAK_SHIFT   DCMIPP_P1DMCR_PEAK_Pos
#define DCMIPP_P1DMCR_LINEV_SHIFT   DCMIPP_P1DMCR_LINEV_Pos
#define DCMIPP_P1DMCR_LINEH_SHIFT   DCMIPP_P1DMCR_LINEH_Pos
#define DCMIPP_P1DMCR_EDGE_SHIFT   DCMIPP_P1DMCR_EDGE_Pos
#define DCMIPP_P1CCCR_ENABLE_SHIFT   DCMIPP_P1CCCR_ENABLE_Pos
#define DCMIPP_P1CCCR_TYPE_SHIFT   DCMIPP_P1CCCR_TYPE_Pos
#define DCMIPP_P1CCCR_CLAMP_SHIFT   DCMIPP_P1CCCR_CLAMP_Pos
#define DCMIPP_P1CCRR1_RR_SHIFT   DCMIPP_P1CCRR1_RR_Pos
#define DCMIPP_P1CCRR1_RG_SHIFT   DCMIPP_P1CCRR1_RG_Pos
#define DCMIPP_P1CCRR2_RB_SHIFT   DCMIPP_P1CCRR2_RB_Pos
#define DCMIPP_P1CCRR2_RA_SHIFT   DCMIPP_P1CCRR2_RA_Pos
#define DCMIPP_P1CCGR1_GR_SHIFT   DCMIPP_P1CCGR1_GR_Pos
#define DCMIPP_P1CCGR1_GG_SHIFT   DCMIPP_P1CCGR1_GG_Pos
#define DCMIPP_P1CCGR2_GB_SHIFT   DCMIPP_P1CCGR2_GB_Pos
#define DCMIPP_P1CCGR2_GA_SHIFT   DCMIPP_P1CCGR2_GA_Pos
#define DCMIPP_P1CCBR1_BR_SHIFT   DCMIPP_P1CCBR1_BR_Pos
#define DCMIPP_P1CCBR1_BG_SHIFT   DCMIPP_P1CCBR1_BG_Pos
#define DCMIPP_P1CCBR2_BB_SHIFT   DCMIPP_P1CCBR2_BB_Pos
#define DCMIPP_P1CCBR2_BA_SHIFT   DCMIPP_P1CCBR2_BA_Pos
#define DCMIPP_P1CTCR1_ENABLE_SHIFT   DCMIPP_P1CTCR1_ENABLE_Pos
#define DCMIPP_P1CTCR1_LUM0_SHIFT   DCMIPP_P1CTCR1_LUM0_Pos
#define DCMIPP_P1CTCR2_LUM4_SHIFT   DCMIPP_P1CTCR2_LUM4_Pos
#define DCMIPP_P1CTCR2_LUM3_SHIFT   DCMIPP_P1CTCR2_LUM3_Pos
#define DCMIPP_P1CTCR2_LUM2_SHIFT   DCMIPP_P1CTCR2_LUM2_Pos
#define DCMIPP_P1CTCR2_LUM1_SHIFT   DCMIPP_P1CTCR2_LUM1_Pos
#define DCMIPP_P1CTCR3_LUM8_SHIFT   DCMIPP_P1CTCR3_LUM8_Pos
#define DCMIPP_P1CTCR3_LUM7_SHIFT   DCMIPP_P1CTCR3_LUM7_Pos
#define DCMIPP_P1CTCR3_LUM6_SHIFT   DCMIPP_P1CTCR3_LUM6_Pos
#define DCMIPP_P1CTCR3_LUM5_SHIFT   DCMIPP_P1CTCR3_LUM5_Pos
#define DCMIPP_P1FCTCR_FRATE_SHIFT   DCMIPP_P1FCTCR_FRATE_Pos
#define DCMIPP_P1FCTCR_CPTMODE_SHIFT   DCMIPP_P1FCTCR_CPTMODE_Pos
#define DCMIPP_P1FCTCR_CPTREQ_SHIFT   DCMIPP_P1FCTCR_CPTREQ_Pos
#define DCMIPP_P1CRSTR_HSTART_SHIFT   DCMIPP_P1CRSTR_HSTART_Pos
#define DCMIPP_P1CRSTR_VSTART_SHIFT   DCMIPP_P1CRSTR_VSTART_Pos
#define DCMIPP_P1CRSZR_HSIZE_SHIFT   DCMIPP_P1CRSZR_HSIZE_Pos
#define DCMIPP_P1CRSZR_VSIZE_SHIFT   DCMIPP_P1CRSZR_VSIZE_Pos
#define DCMIPP_P1CRSZR_ENABLE_SHIFT   DCMIPP_P1CRSZR_ENABLE_Pos
#define DCMIPP_P1DCCR_ENABLE_SHIFT   DCMIPP_P1DCCR_ENABLE_Pos
#define DCMIPP_P1DCCR_HDEC_SHIFT   DCMIPP_P1DCCR_HDEC_Pos
#define DCMIPP_P1DCCR_VDEC_SHIFT   DCMIPP_P1DCCR_VDEC_Pos
#define DCMIPP_P1DSCR_HDIV_SHIFT   DCMIPP_P1DSCR_HDIV_Pos
#define DCMIPP_P1DSCR_VDIV_SHIFT   DCMIPP_P1DSCR_VDIV_Pos
#define DCMIPP_P1DSCR_ENABLE_SHIFT   DCMIPP_P1DSCR_ENABLE_Pos
#define DCMIPP_P1DSRTIOR_HRATIO_SHIFT   DCMIPP_P1DSRTIOR_HRATIO_Pos
#define DCMIPP_P1DSRTIOR_VRATIO_SHIFT   DCMIPP_P1DSRTIOR_VRATIO_Pos
#define DCMIPP_P1DSSZR_HSIZE_SHIFT   DCMIPP_P1DSSZR_HSIZE_Pos
#define DCMIPP_P1DSSZR_VSIZE_SHIFT   DCMIPP_P1DSSZR_VSIZE_Pos
#define DCMIPP_P1CMRICR_ROILSZ_SHIFT   DCMIPP_P1CMRICR_ROILSZ_Pos
#define DCMIPP_P1CMRICR_ROI1EN_SHIFT   DCMIPP_P1CMRICR_ROI1EN_Pos
#define DCMIPP_P1CMRICR_ROI2EN_SHIFT   DCMIPP_P1CMRICR_ROI2EN_Pos
#define DCMIPP_P1CMRICR_ROI3EN_SHIFT   DCMIPP_P1CMRICR_ROI3EN_Pos
#define DCMIPP_P1CMRICR_ROI4EN_SHIFT   DCMIPP_P1CMRICR_ROI4EN_Pos
#define DCMIPP_P1CMRICR_ROI5EN_SHIFT   DCMIPP_P1CMRICR_ROI5EN_Pos
#define DCMIPP_P1CMRICR_ROI6EN_SHIFT   DCMIPP_P1CMRICR_ROI6EN_Pos
#define DCMIPP_P1CMRICR_ROI7EN_SHIFT   DCMIPP_P1CMRICR_ROI7EN_Pos
#define DCMIPP_P1CMRICR_ROI8EN_SHIFT   DCMIPP_P1CMRICR_ROI8EN_Pos
#define DCMIPP_P1RIxCR1_HSTART_SHIFT   DCMIPP_P1RIxCR1_HSTART_Pos
#define DCMIPP_P1RIxCR1_CLB_SHIFT   DCMIPP_P1RIxCR1_CLB_Pos
#define DCMIPP_P1RIxCR1_CLG_SHIFT   DCMIPP_P1RIxCR1_CLG_Pos
#define DCMIPP_P1RIxCR1_VSTART_SHIFT   DCMIPP_P1RIxCR1_VSTART_Pos
#define DCMIPP_P1RIxCR1_CLR_SHIFT   DCMIPP_P1RIxCR1_CLR_Pos
#define DCMIPP_P1RIxCR2_VSIZE_SHIFT   DCMIPP_P1RIxCR2_VSIZE_Pos
#define DCMIPP_P1RIxCR2_HSIZE_SHIFT   DCMIPP_P1RIxCR2_HSIZE_Pos
#define DCMIPP_P1GMCR_ENABLE_SHIFT   DCMIPP_P1GMCR_ENABLE_Pos
#define DCMIPP_P1YUVCR_ENABLE_SHIFT   DCMIPP_P1YUVCR_ENABLE_Pos
#define DCMIPP_P1YUVCR_TYPE_SHIFT   DCMIPP_P1YUVCR_TYPE_Pos
#define DCMIPP_P1YUVCR_CLAMP_SHIFT   DCMIPP_P1YUVCR_CLAMP_Pos
#define DCMIPP_P1YUVRR1_RR_SHIFT   DCMIPP_P1YUVRR1_RR_Pos
#define DCMIPP_P1YUVRR1_RG_SHIFT   DCMIPP_P1YUVRR1_RG_Pos
#define DCMIPP_P1YUVRR2_RB_SHIFT   DCMIPP_P1YUVRR2_RB_Pos
#define DCMIPP_P1YUVRR2_RA_SHIFT   DCMIPP_P1YUVRR2_RA_Pos
#define DCMIPP_P1YUVGR1_GR_SHIFT   DCMIPP_P1YUVGR1_GR_Pos
#define DCMIPP_P1YUVGR1_GG_SHIFT   DCMIPP_P1YUVGR1_GG_Pos
#define DCMIPP_P1YUVGR2_GB_SHIFT   DCMIPP_P1YUVGR2_GB_Pos
#define DCMIPP_P1YUVGR2_GA_SHIFT   DCMIPP_P1YUVGR2_GA_Pos
#define DCMIPP_P1YUVBR1_BR_SHIFT   DCMIPP_P1YUVBR1_BR_Pos
#define DCMIPP_P1YUVBR1_BG_SHIFT   DCMIPP_P1YUVBR1_BG_Pos
#define DCMIPP_P1YUVBR2_BB_SHIFT   DCMIPP_P1YUVBR2_BB_Pos
#define DCMIPP_P1YUVBR2_BA_SHIFT   DCMIPP_P1YUVBR2_BA_Pos
#define DCMIPP_P1PPCR_FORMAT_SHIFT   DCMIPP_P1PPCR_FORMAT_Pos
#define DCMIPP_P1PPCR_SWAPRB_SHIFT   DCMIPP_P1PPCR_SWAPRB_Pos
#define DCMIPP_P1PPCR_LINEMULT_SHIFT   DCMIPP_P1PPCR_LINEMULT_Pos
#define DCMIPP_P1PPCR_DBM_SHIFT   DCMIPP_P1PPCR_DBM_Pos
#define DCMIPP_P1PPCR_LMAWM_SHIFT   DCMIPP_P1PPCR_LMAWM_Pos
#define DCMIPP_P1PPCR_LMAWE_SHIFT   DCMIPP_P1PPCR_LMAWE_Pos
#define DCMIPP_P1PPM0AR1_M0A_SHIFT   DCMIPP_P1PPM0AR1_M0A_Pos
#define DCMIPP_P1PPM0AR2_M0A_SHIFT   DCMIPP_P1PPM0AR2_M0A_Pos
#define DCMIPP_P1PPM0PR_PITCH_SHIFT   DCMIPP_P1PPM0PR_PITCH_Pos
#define DCMIPP_P1PPM1AR1_M1A_SHIFT   DCMIPP_P1PPM1AR1_M1A_Pos
#define DCMIPP_P1PPM1AR2_M1A_SHIFT   DCMIPP_P1PPM1AR2_M1A_Pos
#define DCMIPP_P1PPM1PR_PITCH_SHIFT   DCMIPP_P1PPM1PR_PITCH_Pos
#define DCMIPP_P1STM1AR_M1A_SHIFT   DCMIPP_P1STM1AR_M1A_Pos
#define DCMIPP_P1PPM2AR1_M2A_SHIFT   DCMIPP_P1PPM2AR1_M2A_Pos
#define DCMIPP_P1PPM2AR2_M2A_SHIFT   DCMIPP_P1PPM2AR2_M2A_Pos
#define DCMIPP_P1STM2AR_M2A_SHIFT   DCMIPP_P1STM2AR_M2A_Pos
#define DCMIPP_P1IER_LINEIE_SHIFT   DCMIPP_P1IER_LINEIE_Pos
#define DCMIPP_P1IER_FRAMEIE_SHIFT   DCMIPP_P1IER_FRAMEIE_Pos
#define DCMIPP_P1IER_VSYNCIE_SHIFT   DCMIPP_P1IER_VSYNCIE_Pos
#define DCMIPP_P1IER_OVRIE_SHIFT   DCMIPP_P1IER_OVRIE_Pos
#define DCMIPP_P1SR_LINEF_SHIFT   DCMIPP_P1SR_LINEF_Pos
#define DCMIPP_P1SR_FRAMEF_SHIFT   DCMIPP_P1SR_FRAMEF_Pos
#define DCMIPP_P1SR_VSYNCF_SHIFT   DCMIPP_P1SR_VSYNCF_Pos
#define DCMIPP_P1SR_OVRF_SHIFT   DCMIPP_P1SR_OVRF_Pos
#define DCMIPP_P1SR_LSTLINE_SHIFT   DCMIPP_P1SR_LSTLINE_Pos
#define DCMIPP_P1SR_LSTFRM_SHIFT   DCMIPP_P1SR_LSTFRM_Pos
#define DCMIPP_P1SR_CPTACT_SHIFT   DCMIPP_P1SR_CPTACT_Pos
#define DCMIPP_P1FCR_CLINEF_SHIFT   DCMIPP_P1FCR_CLINEF_Pos
#define DCMIPP_P1FCR_CFRAMEF_SHIFT   DCMIPP_P1FCR_CFRAMEF_Pos
#define DCMIPP_P1FCR_CVSYNCF_SHIFT   DCMIPP_P1FCR_CVSYNCF_Pos
#define DCMIPP_P1FCR_COVRF_SHIFT   DCMIPP_P1FCR_COVRF_Pos
#define DCMIPP_P1CFSCR_DTIDA_SHIFT   DCMIPP_P1CFSCR_DTIDA_Pos
#define DCMIPP_P1CFSCR_DTIDB_SHIFT   DCMIPP_P1CFSCR_DTIDB_Pos
#define DCMIPP_P1CFSCR_DTMODE_SHIFT   DCMIPP_P1CFSCR_DTMODE_Pos
#define DCMIPP_P1CFSCR_PIPEDIFF_SHIFT   DCMIPP_P1CFSCR_PIPEDIFF_Pos
#define DCMIPP_P1CFSCR_VC_SHIFT   DCMIPP_P1CFSCR_VC_Pos
#define DCMIPP_P1CFSCR_FDTF_SHIFT   DCMIPP_P1CFSCR_FDTF_Pos
#define DCMIPP_P1CFSCR_FDTFEN_SHIFT   DCMIPP_P1CFSCR_FDTFEN_Pos
#define DCMIPP_P1CFSCR_PIPEN_SHIFT   DCMIPP_P1CFSCR_PIPEN_Pos
#define DCMIPP_P1CBPRCR_ENABLE_SHIFT   DCMIPP_P1CBPRCR_ENABLE_Pos
#define DCMIPP_P1CBPRCR_STRENGTH_SHIFT   DCMIPP_P1CBPRCR_STRENGTH_Pos
#define DCMIPP_P1CBLCCR_ENABLE_SHIFT   DCMIPP_P1CBLCCR_ENABLE_Pos
#define DCMIPP_P1CBLCCR_BLCB_SHIFT   DCMIPP_P1CBLCCR_BLCB_Pos
#define DCMIPP_P1CBLCCR_BLCG_SHIFT   DCMIPP_P1CBLCCR_BLCG_Pos
#define DCMIPP_P1CBLCCR_BLCR_SHIFT   DCMIPP_P1CBLCCR_BLCR_Pos
#define DCMIPP_P1CEXCR1_ENABLE_SHIFT   DCMIPP_P1CEXCR1_ENABLE_Pos
#define DCMIPP_P1CEXCR1_MULTR_SHIFT   DCMIPP_P1CEXCR1_MULTR_Pos
#define DCMIPP_P1CEXCR1_SHFR_SHIFT   DCMIPP_P1CEXCR1_SHFR_Pos
#define DCMIPP_P1CEXCR2_MULTB_SHIFT   DCMIPP_P1CEXCR2_MULTB_Pos
#define DCMIPP_P1CEXCR2_SHFB_SHIFT   DCMIPP_P1CEXCR2_SHFB_Pos
#define DCMIPP_P1CEXCR2_MULTG_SHIFT   DCMIPP_P1CEXCR2_MULTG_Pos
#define DCMIPP_P1CEXCR2_SHFG_SHIFT   DCMIPP_P1CEXCR2_SHFG_Pos
#define DCMIPP_P1CST1CR_ENABLE_SHIFT   DCMIPP_P1CST1CR_ENABLE_Pos
#define DCMIPP_P1CST1CR_BINS_SHIFT   DCMIPP_P1CST1CR_BINS_Pos
#define DCMIPP_P1CST1CR_SRC_SHIFT   DCMIPP_P1CST1CR_SRC_Pos
#define DCMIPP_P1CST1CR_MODE_SHIFT   DCMIPP_P1CST1CR_MODE_Pos
#define DCMIPP_P1CST1CR_ACCU_SHIFT   DCMIPP_P1CST1CR_ACCU_Pos
#define DCMIPP_P1CST2CR_ENABLE_SHIFT   DCMIPP_P1CST2CR_ENABLE_Pos
#define DCMIPP_P1CST2CR_BINS_SHIFT   DCMIPP_P1CST2CR_BINS_Pos
#define DCMIPP_P1CST2CR_SRC_SHIFT   DCMIPP_P1CST2CR_SRC_Pos
#define DCMIPP_P1CST2CR_MODE_SHIFT   DCMIPP_P1CST2CR_MODE_Pos
#define DCMIPP_P1CST2CR_ACCU_SHIFT   DCMIPP_P1CST2CR_ACCU_Pos
#define DCMIPP_P1CST3CR_ENABLE_SHIFT   DCMIPP_P1CST3CR_ENABLE_Pos
#define DCMIPP_P1CST3CR_BINS_SHIFT   DCMIPP_P1CST3CR_BINS_Pos
#define DCMIPP_P1CST3CR_SRC_SHIFT   DCMIPP_P1CST3CR_SRC_Pos
#define DCMIPP_P1CST3CR_MODE_SHIFT   DCMIPP_P1CST3CR_MODE_Pos
#define DCMIPP_P1CST3CR_ACCU_SHIFT   DCMIPP_P1CST3CR_ACCU_Pos
#define DCMIPP_P1CSTSTR_HSTART_SHIFT   DCMIPP_P1CSTSTR_HSTART_Pos
#define DCMIPP_P1CSTSTR_VSTART_SHIFT   DCMIPP_P1CSTSTR_VSTART_Pos
#define DCMIPP_P1CSTSZR_HSIZE_SHIFT   DCMIPP_P1CSTSZR_HSIZE_Pos
#define DCMIPP_P1CSTSZR_VSIZE_SHIFT   DCMIPP_P1CSTSZR_VSIZE_Pos
#define DCMIPP_P1CSTSZR_CROPEN_SHIFT   DCMIPP_P1CSTSZR_CROPEN_Pos
#define DCMIPP_P1CCCCR_ENABLE_SHIFT   DCMIPP_P1CCCCR_ENABLE_Pos
#define DCMIPP_P1CCCCR_TYPE_SHIFT   DCMIPP_P1CCCCR_TYPE_Pos
#define DCMIPP_P1CCCCR_CLAMP_SHIFT   DCMIPP_P1CCCCR_CLAMP_Pos
#define DCMIPP_P1CCCRR1_RR_SHIFT   DCMIPP_P1CCCRR1_RR_Pos
#define DCMIPP_P1CCCRR1_RG_SHIFT   DCMIPP_P1CCCRR1_RG_Pos
#define DCMIPP_P1CCCRR2_RB_SHIFT   DCMIPP_P1CCCRR2_RB_Pos
#define DCMIPP_P1CCCRR2_RA_SHIFT   DCMIPP_P1CCCRR2_RA_Pos
#define DCMIPP_P1CCCGR1_GR_SHIFT   DCMIPP_P1CCCGR1_GR_Pos
#define DCMIPP_P1CCCGR1_GG_SHIFT   DCMIPP_P1CCCGR1_GG_Pos
#define DCMIPP_P1CCCGR2_GB_SHIFT   DCMIPP_P1CCCGR2_GB_Pos
#define DCMIPP_P1CCCGR2_GA_SHIFT   DCMIPP_P1CCCGR2_GA_Pos
#define DCMIPP_P1CCCBR1_BR_SHIFT   DCMIPP_P1CCCBR1_BR_Pos
#define DCMIPP_P1CCCBR1_BG_SHIFT   DCMIPP_P1CCCBR1_BG_Pos
#define DCMIPP_P1CCCBR2_BB_SHIFT   DCMIPP_P1CCCBR2_BB_Pos
#define DCMIPP_P1CCCBR2_BA_SHIFT   DCMIPP_P1CCCBR2_BA_Pos
#define DCMIPP_P1CCTCR1_ENABLE_SHIFT   DCMIPP_P1CCTCR1_ENABLE_Pos
#define DCMIPP_P1CCTCR1_LUM0_SHIFT   DCMIPP_P1CCTCR1_LUM0_Pos
#define DCMIPP_P1CCTCR2_LUM4_SHIFT   DCMIPP_P1CCTCR2_LUM4_Pos
#define DCMIPP_P1CCTCR2_LUM3_SHIFT   DCMIPP_P1CCTCR2_LUM3_Pos
#define DCMIPP_P1CCTCR2_LUM2_SHIFT   DCMIPP_P1CCTCR2_LUM2_Pos
#define DCMIPP_P1CCTCR2_LUM1_SHIFT   DCMIPP_P1CCTCR2_LUM1_Pos
#define DCMIPP_P1CCTCR3_LUM8_SHIFT   DCMIPP_P1CCTCR3_LUM8_Pos
#define DCMIPP_P1CCTCR3_LUM7_SHIFT   DCMIPP_P1CCTCR3_LUM7_Pos
#define DCMIPP_P1CCTCR3_LUM6_SHIFT   DCMIPP_P1CCTCR3_LUM6_Pos
#define DCMIPP_P1CCTCR3_LUM5_SHIFT   DCMIPP_P1CCTCR3_LUM5_Pos
#define DCMIPP_P1CFCTCR_FRATE_SHIFT   DCMIPP_P1CFCTCR_FRATE_Pos
#define DCMIPP_P1CFCTCR_CPTMODE_SHIFT   DCMIPP_P1CFCTCR_CPTMODE_Pos
#define DCMIPP_P1CFCTCR_CPTREQ_SHIFT   DCMIPP_P1CFCTCR_CPTREQ_Pos
#define DCMIPP_P1CCRSTR_HSTART_SHIFT   DCMIPP_P1CCRSTR_HSTART_Pos
#define DCMIPP_P1CCRSTR_VSTART_SHIFT   DCMIPP_P1CCRSTR_VSTART_Pos
#define DCMIPP_P1CCRSZR_HSIZE_SHIFT   DCMIPP_P1CCRSZR_HSIZE_Pos
#define DCMIPP_P1CCRSZR_VSIZE_SHIFT   DCMIPP_P1CCRSZR_VSIZE_Pos
#define DCMIPP_P1CCRSZR_ENABLE_SHIFT   DCMIPP_P1CCRSZR_ENABLE_Pos
#define DCMIPP_P1CDCCR_ENABLE_SHIFT   DCMIPP_P1CDCCR_ENABLE_Pos
#define DCMIPP_P1CDCCR_HDEC_SHIFT   DCMIPP_P1CDCCR_HDEC_Pos
#define DCMIPP_P1CDCCR_VDEC_SHIFT   DCMIPP_P1CDCCR_VDEC_Pos
#define DCMIPP_P1CDSCR_HDIV_SHIFT   DCMIPP_P1CDSCR_HDIV_Pos
#define DCMIPP_P1CDSCR_VDIV_SHIFT   DCMIPP_P1CDSCR_VDIV_Pos
#define DCMIPP_P1CDSCR_ENABLE_SHIFT   DCMIPP_P1CDSCR_ENABLE_Pos
#define DCMIPP_P1CDSRTIOR_HRATIO_SHIFT   DCMIPP_P1CDSRTIOR_HRATIO_Pos
#define DCMIPP_P1CDSRTIOR_VRATIO_SHIFT   DCMIPP_P1CDSRTIOR_VRATIO_Pos
#define DCMIPP_P1CDSSZR_HSIZE_SHIFT   DCMIPP_P1CDSSZR_HSIZE_Pos
#define DCMIPP_P1CDSSZR_VSIZE_SHIFT   DCMIPP_P1CDSSZR_VSIZE_Pos
#define DCMIPP_P1CPPCR_FORMAT_SHIFT   DCMIPP_P1CPPCR_FORMAT_Pos
#define DCMIPP_P1CPPCR_SWAPRB_SHIFT   DCMIPP_P1CPPCR_SWAPRB_Pos
#define DCMIPP_P1CPPCR_LINEMULT_SHIFT   DCMIPP_P1CPPCR_LINEMULT_Pos
#define DCMIPP_P1CPPM0AR1_M0A_SHIFT   DCMIPP_P1CPPM0AR1_M0A_Pos
#define DCMIPP_P1CPPM0PR_PITCH_SHIFT   DCMIPP_P1CPPM0PR_PITCH_Pos
#define DCMIPP_P1CPPM1AR1_M1A_SHIFT   DCMIPP_P1CPPM1AR1_M1A_Pos
#define DCMIPP_P1CPPM1PR_PITCH_SHIFT   DCMIPP_P1CPPM1PR_PITCH_Pos
#define DCMIPP_P1CPPM2AR1_M2A_SHIFT   DCMIPP_P1CPPM2AR1_M2A_Pos
#define DCMIPP_P2FSCR_DTIDA_SHIFT   DCMIPP_P2FSCR_DTIDA_Pos
#define DCMIPP_P2FSCR_VC_SHIFT   DCMIPP_P2FSCR_VC_Pos
#define DCMIPP_P2FSCR_FDTF_SHIFT   DCMIPP_P2FSCR_FDTF_Pos
#define DCMIPP_P2FSCR_FDTFEN_SHIFT   DCMIPP_P2FSCR_FDTFEN_Pos
#define DCMIPP_P2FSCR_PIPEN_SHIFT   DCMIPP_P2FSCR_PIPEN_Pos
#define DCMIPP_P2FCTCR_FRATE_SHIFT   DCMIPP_P2FCTCR_FRATE_Pos
#define DCMIPP_P2FCTCR_CPTMODE_SHIFT   DCMIPP_P2FCTCR_CPTMODE_Pos
#define DCMIPP_P2FCTCR_CPTREQ_SHIFT   DCMIPP_P2FCTCR_CPTREQ_Pos
#define DCMIPP_P2CRSTR_HSTART_SHIFT   DCMIPP_P2CRSTR_HSTART_Pos
#define DCMIPP_P2CRSTR_VSTART_SHIFT   DCMIPP_P2CRSTR_VSTART_Pos
#define DCMIPP_P2CRSZR_HSIZE_SHIFT   DCMIPP_P2CRSZR_HSIZE_Pos
#define DCMIPP_P2CRSZR_VSIZE_SHIFT   DCMIPP_P2CRSZR_VSIZE_Pos
#define DCMIPP_P2CRSZR_ENABLE_SHIFT   DCMIPP_P2CRSZR_ENABLE_Pos
#define DCMIPP_P2DCCR_ENABLE_SHIFT   DCMIPP_P2DCCR_ENABLE_Pos
#define DCMIPP_P2DCCR_HDEC_SHIFT   DCMIPP_P2DCCR_HDEC_Pos
#define DCMIPP_P2DCCR_VDEC_SHIFT   DCMIPP_P2DCCR_VDEC_Pos
#define DCMIPP_P2DSCR_HDIV_SHIFT   DCMIPP_P2DSCR_HDIV_Pos
#define DCMIPP_P2DSCR_VDIV_SHIFT   DCMIPP_P2DSCR_VDIV_Pos
#define DCMIPP_P2DSCR_ENABLE_SHIFT   DCMIPP_P2DSCR_ENABLE_Pos
#define DCMIPP_P2DSRTIOR_HRATIO_SHIFT   DCMIPP_P2DSRTIOR_HRATIO_Pos
#define DCMIPP_P2DSRTIOR_VRATIO_SHIFT   DCMIPP_P2DSRTIOR_VRATIO_Pos
#define DCMIPP_P2DSSZR_HSIZE_SHIFT   DCMIPP_P2DSSZR_HSIZE_Pos
#define DCMIPP_P2DSSZR_VSIZE_SHIFT   DCMIPP_P2DSSZR_VSIZE_Pos
#define DCMIPP_P2GMCR_ENABLE_SHIFT   DCMIPP_P2GMCR_ENABLE_Pos
#define DCMIPP_P2CMRICR_ROILSZ_SHIFT   DCMIPP_P2CMRICR_ROILSZ_Pos
#define DCMIPP_P2CMRICR_ROI1EN_SHIFT   DCMIPP_P2CMRICR_ROI1EN_Pos
#define DCMIPP_P2CMRICR_ROI2EN_SHIFT   DCMIPP_P2CMRICR_ROI2EN_Pos
#define DCMIPP_P2CMRICR_ROI3EN_SHIFT   DCMIPP_P2CMRICR_ROI3EN_Pos
#define DCMIPP_P2CMRICR_ROI4EN_SHIFT   DCMIPP_P2CMRICR_ROI4EN_Pos
#define DCMIPP_P2CMRICR_ROI5EN_SHIFT   DCMIPP_P2CMRICR_ROI5EN_Pos
#define DCMIPP_P2CMRICR_ROI6EN_SHIFT   DCMIPP_P2CMRICR_ROI6EN_Pos
#define DCMIPP_P2CMRICR_ROI7EN_SHIFT   DCMIPP_P2CMRICR_ROI7EN_Pos
#define DCMIPP_P2CMRICR_ROI8EN_SHIFT   DCMIPP_P2CMRICR_ROI8EN_Pos
#define DCMIPP_P2RIxCR1_HSTART_SHIFT   DCMIPP_P2RIxCR1_HSTART_Pos
#define DCMIPP_P2RIxCR1_CLB_SHIFT   DCMIPP_P2RIxCR1_CLB_Pos
#define DCMIPP_P2RIxCR1_CLG_SHIFT   DCMIPP_P2RIxCR1_CLG_Pos
#define DCMIPP_P2RIxCR1_VSTART_SHIFT   DCMIPP_P2RIxCR1_VSTART_Pos
#define DCMIPP_P2RIxCR1_CLR_SHIFT   DCMIPP_P2RIxCR1_CLR_Pos
#define DCMIPP_P2RIxCR2_VSIZE_SHIFT   DCMIPP_P2RIxCR2_VSIZE_Pos
#define DCMIPP_P2RIxCR2_HSIZE_SHIFT   DCMIPP_P2RIxCR2_HSIZE_Pos
#define DCMIPP_P2PPCR_FORMAT_SHIFT   DCMIPP_P2PPCR_FORMAT_Pos
#define DCMIPP_P2PPCR_SWAPRB_SHIFT   DCMIPP_P2PPCR_SWAPRB_Pos
#define DCMIPP_P2PPCR_LINEMULT_SHIFT   DCMIPP_P2PPCR_LINEMULT_Pos
#define DCMIPP_P2PPCR_DBM_SHIFT   DCMIPP_P2PPCR_DBM_Pos
#define DCMIPP_P2PPCR_LMAWM_SHIFT   DCMIPP_P2PPCR_LMAWM_Pos
#define DCMIPP_P2PPCR_LMAWE_SHIFT   DCMIPP_P2PPCR_LMAWE_Pos
#define DCMIPP_P2PPM0AR1_M0A_SHIFT   DCMIPP_P2PPM0AR1_M0A_Pos
#define DCMIPP_P2PPM0AR2_M0A_SHIFT   DCMIPP_P2PPM0AR2_M0A_Pos
#define DCMIPP_P2PPM0PR_PITCH_SHIFT   DCMIPP_P2PPM0PR_PITCH_Pos
#define DCMIPP_P2STM0AR_SHIFT   DCMIPP_P2STM0AR_Pos
#define DCMIPP_P2IER_LINEIE_SHIFT   DCMIPP_P2IER_LINEIE_Pos
#define DCMIPP_P2IER_FRAMEIE_SHIFT   DCMIPP_P2IER_FRAMEIE_Pos
#define DCMIPP_P2IER_VSYNCIE_SHIFT   DCMIPP_P2IER_VSYNCIE_Pos
#define DCMIPP_P2IER_OVRIE_SHIFT   DCMIPP_P2IER_OVRIE_Pos
#define DCMIPP_P2SR_LINEF_SHIFT   DCMIPP_P2SR_LINEF_Pos
#define DCMIPP_P2SR_FRAMEF_SHIFT   DCMIPP_P2SR_FRAMEF_Pos
#define DCMIPP_P2SR_VSYNCF_SHIFT   DCMIPP_P2SR_VSYNCF_Pos
#define DCMIPP_P2SR_OVRF_SHIFT   DCMIPP_P2SR_OVRF_Pos
#define DCMIPP_P2SR_LSTLINE_SHIFT   DCMIPP_P2SR_LSTLINE_Pos
#define DCMIPP_P2SR_LSTFRM_SHIFT   DCMIPP_P2SR_LSTFRM_Pos
#define DCMIPP_P2SR_CPTACT_SHIFT   DCMIPP_P2SR_CPTACT_Pos
#define DCMIPP_P2FCR_CLINEF_SHIFT   DCMIPP_P2FCR_CLINEF_Pos
#define DCMIPP_P2FCR_CFRAMEF_SHIFT   DCMIPP_P2FCR_CFRAMEF_Pos
#define DCMIPP_P2FCR_CVSYNCF_SHIFT   DCMIPP_P2FCR_CVSYNCF_Pos
#define DCMIPP_P2FCR_COVRF_SHIFT   DCMIPP_P2FCR_COVRF_Pos
#define DCMIPP_P2CFSCR_DTID_SHIFT   DCMIPP_P2CFSCR_DTID_Pos
#define DCMIPP_P2CFSCR_VC_SHIFT   DCMIPP_P2CFSCR_VC_Pos
#define DCMIPP_P2CFSCR_FDTF_SHIFT   DCMIPP_P2CFSCR_FDTF_Pos
#define DCMIPP_P2CFSCR_FDTFEN_SHIFT   DCMIPP_P2CFSCR_FDTFEN_Pos
#define DCMIPP_P2CFSCR_PIPEN_SHIFT   DCMIPP_P2CFSCR_PIPEN_Pos
#define DCMIPP_P2CFCTCR_FRATE_SHIFT   DCMIPP_P2CFCTCR_FRATE_Pos
#define DCMIPP_P2CFCTCR_CPTMODE_SHIFT   DCMIPP_P2CFCTCR_CPTMODE_Pos
#define DCMIPP_P2CFCTCR_CPTREQ_SHIFT   DCMIPP_P2CFCTCR_CPTREQ_Pos
#define DCMIPP_P2CCRSTR_HSTART_SHIFT   DCMIPP_P2CCRSTR_HSTART_Pos
#define DCMIPP_P2CCRSTR_VSTART_SHIFT   DCMIPP_P2CCRSTR_VSTART_Pos
#define DCMIPP_P2CCRSZR_HSIZE_SHIFT   DCMIPP_P2CCRSZR_HSIZE_Pos
#define DCMIPP_P2CCRSZR_VSIZE_SHIFT   DCMIPP_P2CCRSZR_VSIZE_Pos
#define DCMIPP_P2CCRSZR_ENABLE_SHIFT   DCMIPP_P2CCRSZR_ENABLE_Pos
#define DCMIPP_P2CDCCR_ENABLE_SHIFT   DCMIPP_P2CDCCR_ENABLE_Pos
#define DCMIPP_P2CDCCR_HDEC_SHIFT   DCMIPP_P2CDCCR_HDEC_Pos
#define DCMIPP_P2CDCCR_VDEC_SHIFT   DCMIPP_P2CDCCR_VDEC_Pos
#define DCMIPP_P2CDSCR_HDIV_SHIFT   DCMIPP_P2CDSCR_HDIV_Pos
#define DCMIPP_P2CDSCR_VDIV_SHIFT   DCMIPP_P2CDSCR_VDIV_Pos
#define DCMIPP_P2CDSCR_ENABLE_SHIFT   DCMIPP_P2CDSCR_ENABLE_Pos
#define DCMIPP_P2CDSRTIOR_HRATIO_SHIFT   DCMIPP_P2CDSRTIOR_HRATIO_Pos
#define DCMIPP_P2CDSRTIOR_VRATIO_SHIFT   DCMIPP_P2CDSRTIOR_VRATIO_Pos
#define DCMIPP_P2CDSSZR_HSIZE_SHIFT   DCMIPP_P2CDSSZR_HSIZE_Pos
#define DCMIPP_P2CDSSZR_VSIZE_SHIFT   DCMIPP_P2CDSSZR_VSIZE_Pos
#define DCMIPP_P2CPPCR_FORMAT_SHIFT   DCMIPP_P2CPPCR_FORMAT_Pos
#define DCMIPP_P2CPPCR_SWAPRB_SHIFT   DCMIPP_P2CPPCR_SWAPRB_Pos
#define DCMIPP_P2CPPCR_LINEMULT_SHIFT   DCMIPP_P2CPPCR_LINEMULT_Pos
#define DCMIPP_P2CPPCR_DBM_SHIFT   DCMIPP_P2CPPCR_DBM_Pos
#define DCMIPP_P2CPPCR_LMAWM_SHIFT   DCMIPP_P2CPPCR_LMAWM_Pos
#define DCMIPP_P2CPPCR_LMAWE_SHIFT   DCMIPP_P2CPPCR_LMAWE_Pos
#define DCMIPP_P2CPPM0AR1_M0A_SHIFT   DCMIPP_P2CPPM0AR1_M0A_Pos
#define DCMIPP_P2CPPM0AR2_M0A_SHIFT   DCMIPP_P2CPPM0AR2_M0A_Pos
#define DCMIPP_P2CPPM0PR_PITCH_SHIFT   DCMIPP_P2CPPM0PR_PITCH_Pos
#define DCMIPP_HWCFGR2_VPFT_SHIFT   DCMIPP_HWCFGR2_VPFT_Pos
#define DCMIPP_HWCFGR2_DBMFT_SHIFT   DCMIPP_HWCFGR2_DBMFT_Pos
#define DCMIPP_HWCFGR2_PROCCLK_SHIFT   DCMIPP_HWCFGR2_PROCCLK_Pos
#define DCMIPP_HWCFGR2_ADDMOD_SHIFT   DCMIPP_HWCFGR2_ADDMOD_Pos
#define DCMIPP_HWCFGR2_DEC1_SHIFT   DCMIPP_HWCFGR2_DEC1_Pos
#define DCMIPP_HWCFGR2_DEC2_SHIFT   DCMIPP_HWCFGR2_DEC2_Pos
#define DCMIPP_HWCFGR2_MCU_SHIFT   DCMIPP_HWCFGR2_MCU_Pos
#define DCMIPP_HWCFGR2_TPG_SHIFT   DCMIPP_HWCFGR2_TPG_Pos
#define DCMIPP_HWCFGR2_STV_SHIFT   DCMIPP_HWCFGR2_STV_Pos
#define DCMIPP_HWCFGR1_CSIFT_SHIFT   DCMIPP_HWCFGR1_CSIFT_Pos
#define DCMIPP_HWCFGR1_PIPENB_SHIFT   DCMIPP_HWCFGR1_PIPENB_Pos
#define DCMIPP_HWCFGR1_IPPLUGCFG_SHIFT   DCMIPP_HWCFGR1_IPPLUGCFG_Pos
#define DCMIPP_HWCFGR1_DSP1FT_SHIFT   DCMIPP_HWCFGR1_DSP1FT_Pos
#define DCMIPP_HWCFGR1_DSP2FT_SHIFT   DCMIPP_HWCFGR1_DSP2FT_Pos
#define DCMIPP_HWCFGR1_RB2RGB_SHIFT   DCMIPP_HWCFGR1_RB2RGB_Pos
#define DCMIPP_HWCFGR1_PLANARFT_SHIFT   DCMIPP_HWCFGR1_PLANARFT_Pos
#define DCMIPP_HWCFGR1_ROI1NB_SHIFT   DCMIPP_HWCFGR1_ROI1NB_Pos
#define DCMIPP_HWCFGR1_ROI2NB_SHIFT   DCMIPP_HWCFGR1_ROI2NB_Pos
#define DCMIPP_VERR_MINREV_SHIFT   DCMIPP_VERR_MINREV_Pos
#define DCMIPP_VERR_MAJREV_SHIFT   DCMIPP_VERR_MAJREV_Pos
#define DCMIPP_IPIDR_IDR_SHIFT   DCMIPP_IPIDR_IDR_Pos
#define DCMIPP_SIDR_SID_SHIFT   DCMIPP_SIDR_SID_Pos

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_DCMIPP_H */
