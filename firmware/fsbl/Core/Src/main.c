/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "extmem_manager.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hyperram.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

XSPI_HandleTypeDef hxspi1;
XSPI_HandleTypeDef hxspi2;

/* USER CODE BEGIN PV */
static HyperRAM_ObjectTypeDef HyperRAMObject = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_XSPI1_Init(void);
static void MX_XSPI2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Progress LEDs: LED0 (PG10, active low) = FSBL running; LED1 (PE10, active
   low) = RIF configured, NuttX hand-off in progress.  The verbose bring-up
   blink markers (DBG_BLINK / DBG_BLINK_LED1 / DBG_LED1_SEG with multi-second
   busy-waits) were removed now that flash-boot is stable. */

/* Custom RIF setup: unlock the DMA-capable bus masters (DMA2D/GPDMA1/NPU/GPU2D)
 * so they can read/write the firmware SRAM after boot.  The stock (prebuilt)
 * FSBL does not program RIF at all, which leaves every non-CPU master blocked
 * from SRAM (verified by the RISAF probe on real HW, both DEV and flash boot).
 *
 * Register-level implementation (no HAL_RIF symbols): stm32n6xx_hal_rif.c is
 * not part of this CubeIDE source list, so the two thin HAL wrappers are
 * inlined here as direct register writes.
 */
static void RIF_Config(void)
{
  uint32_t i;

  /* Enable the RIFSC, RISAF and GPDMA1 clocks before touching RIF registers.
   * NOTE: RISAF has its OWN clock bit (RCC_AHB3ENR_RISAFEN, bit 14), separate
   * from RIFSC (bit 9).  Only enabling RIFSC leaves the RISAF blocks
   * clock-gated; writing RISAFx registers then raises a bus fault
   * (HardFault).  This was the missing piece that froze the boot at
   * marker 9 (45 blinks).
   *
   * RCC AHBxENSR are SET-only registers (write 1 = enable, write 0 = no
   * effect; clearing is done via the paired AHBxENCR).  So calling the three
   * __HAL_RCC_*_CLK_ENABLE() helpers in sequence is safe: each WRITE_REG
   * only sets its own bit and never clears the others. */
  __HAL_RCC_RIFSC_CLK_ENABLE();
  __HAL_RCC_RISAF_CLK_ENABLE();
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* 1) RIMC: give DMA2D (idx 8), GPU2D (idx 7) and NPU (idx 1) CID0 with
   *    secure+privileged attributes.  GPDMA1 has no RIMC entry; its CID is
   *    set per-channel via CCIDCFGR (step 2).
   */
  RIFSC->RIMC_ATTRx[RIF_MASTER_INDEX_DMA2D] =
    RIFSC_RIMC_ATTRx_MSEC | RIFSC_RIMC_ATTRx_MPRIV;  /* MCID = 0 -> CID0 */
#if defined(RIF_MASTER_INDEX_GPU2D)
  RIFSC->RIMC_ATTRx[RIF_MASTER_INDEX_GPU2D] =
    RIFSC_RIMC_ATTRx_MSEC | RIFSC_RIMC_ATTRx_MPRIV;
#endif
#if defined(RIF_MASTER_INDEX_NPU)
  RIFSC->RIMC_ATTRx[RIF_MASTER_INDEX_NPU] =
    RIFSC_RIMC_ATTRx_MSEC | RIFSC_RIMC_ATTRx_MPRIV;
#endif
#if 0 /* DISABLED 2026-09-07 EXP B: RIMC master attr for SDMMC1 (single-
       * variable test -- HAL_SD_Init still failed with it enabled; the
       * bare-metal example does not need it either since we poll the FIFO,
       * no IDMA to SRAM). */
#if defined(RIF_MASTER_INDEX_SDMMC1)
  /* SDMMC1 (TF card on the base board, PC8-12/PH2): CID0 secure+
   * privileged so the SDMMC master can reach the SRAM buffers.
   * Added 2026-09-06 for the dataset-capture task (TF card + FATFS).
   */
  RIFSC->RIMC_ATTRx[RIF_MASTER_INDEX_SDMMC1] =
    RIFSC_RIMC_ATTRx_MSEC | RIFSC_RIMC_ATTRx_MPRIV;
#endif
#endif

  /* 2) GPDMA1: static CID0 + isolation enable on all 8 channels. */
  for (i = 0U; i < 8U; i++)
    {
      ((DMA_Channel_TypeDef *)(GPDMA1_Channel0_BASE + (i * 0x80U)))->CCIDCFGR =
        DMA_CHANNEL_STATIC_CID_0 | DMA_CCIDCFGR_CFEN;
    }

  /* 3) RISAF: open the SRAM/AXISRAM address windows to every CID, so any
   *    master can read/write firmware memory.  Start/End are offsets from
   *    the base of each protected address space.  Region index 0 is the
   *    first base region (HAL names it RISAF_REGION_1).  Secure bit kept 0
   *    (non-secure) so both secure and non-secure masters are accepted.
   */
  #define RIF_OPEN_RISAF(base, endaddr)                                       \
    do {                                                                      \
      (base)->REG[0].STARTR  = 0x0U;                                          \
      (base)->REG[0].ENDR    = (endaddr);                                     \
      (base)->REG[0].CIDCFGR = RIF_CID_MASK |                                 \
                               (RIF_CID_MASK << RISAF_REGx_CIDCFGR_WRENC0_Pos); \
      /* SEC must be 1: the CPU here runs in SECURE state, and a secure        \
       * requestor is only allowed into SECURE (SEC=1) RISAF regions.  With    \
       * SEC=0 the FSBL itself (secure CPU) could not write the copied         \
       * NuttX image to 0x34000400 -> SRAM stayed 0 -> jump to 0 -> LOCKUP.    \
       * CFGR = BREN(0x1) | SEC(0x100) | 0xFF<<16 = 0xFF0101 (verified fix     \
       * from 2026-08-05).                                                     \
       */                                                                      \
      (base)->REG[0].CFGR    = RISAF_REGx_CFGR_BREN | RISAF_REGx_CFGR_SEC |    \
                               (RIF_CID_MASK << RISAF_REGx_CFGR_PRIVC0_Pos);  \
    } while (0)

  /* RISAF7 -> FLEXMEM/AXISRAM where the firmware runs (0x34000400). */
  RIF_OPEN_RISAF(RISAF7, RISAF7_LIMIT_ADDRESS_SPACE_SIZE);

  /* RISAF1 -> system space 0x00000000-0x3FFFFFFF (SRAM1-5 + AXISRAM aliases). */
  RIF_OPEN_RISAF(RISAF1, RISAF1_LIMIT_ADDRESS_SPACE_SIZE);

  /* RISAF2 (CPU AXI RAM0 = AXISRAM1, 1MB) and RISAF3 (CPU AXI RAM1 =
   * AXISRAM2, 1MB): the 800x480 RGB565 framebuffer (768000B) spans
   * FLEXMEM + AXISRAM1 + AXISRAM2.  The LTDC (CID0) reads AXISRAM1/2
   * through the CPU_NOC and is filtered by RISAF2/3; without these the
   * CID0 reads past RISAF7's 512KB window are denied by the default
   * region rule (secure+privileged+CID1 only) -> only the top ~48 lines
   * display.  Verified on real HW 2026-08-12. */
  RIF_OPEN_RISAF(RISAF2, RISAF2_LIMIT_ADDRESS_SPACE_SIZE);
  RIF_OPEN_RISAF(RISAF3, RISAF3_LIMIT_ADDRESS_SPACE_SIZE);

  /* NOTE (2026-08-12, FSBL regression): non-secure REG1 windows (SEC=0)
   * over AXISRAM1/2 let DMA2D/GPDMA1 (which present NS transactions) write
   * SRAM, but they also capture the CPU's own secure accesses to the heap
   * and stacks that live in AXISRAM1/2 -> boot HardFault in AppBringUp
   * with a blank LCD.  Keeping REG0 secure-only is required for a booting
   * system; DMA acceleration of SRAM needs a dedicated non-secure buffer
   * region (framebuffer moved out of the heap) as a separate task.
   */

#if 0 /* DISABLED: RISAF4/5/6 register writes HardFault on this board (verified
         on real HW: LED1 stops right after x7).  They cover the large AXISRAM
         windows; the current NuttX image runs in FLEXMEM @0x34000000 which is
         already fully covered by RISAF7 (0x0000-0x7FFFF = 512 KB).  Re-enable
         with a working configuration if a future image needs >512 KB of the
         large AXISRAM. */
#if defined(RISAF4)
  /* RISAF4/5/6 -> large AXISRAM windows (present when NPU is fitted). */
  RIF_OPEN_RISAF(RISAF4, 0x3FFFFFFFU);
  RIF_OPEN_RISAF(RISAF5, 0x3FFFFFFFU);
  RIF_OPEN_RISAF(RISAF6, 0x3FFFFFFFU);
#endif
#endif /* RISAF4/5/6 disabled */

  #undef RIF_OPEN_RISAF

#if 0 /* DISABLED 2026-09-07 EXP B: setting RISC SEC|PRIV for SDMMC1 BEFORE
       * the SD driver initializes LOCKS the SDMMC (HAL_SD_Init fails with
       * 'X' in the FSBL environment; the bare-metal 38_SD_Card example
       * initializes the SD FIRST, then runs SystemIsolation_Config()). */
  /* 3b) RISUP (RISC slave attributes) + GPIO pin security attributes for
   *     SDMMC1 (TF card on the base board, PC8-12/PH2).  Mirror of the
   *     ALIENTEK 38_SD_Card SystemIsolation_Config(): the SDMMC kernel
   *     clock path (RIMU) and the pin AF routing are gated by these
   *     attributes; without them the SDMMC command state machine never
   *     completes (CPSMACT stuck at 0x2000 -> GO_IDLE failed -110,
   *     seen on 2026-09-07 with every register/clock config correct).
   *     Must run AFTER the RIFSC clock is enabled (done above) and after
   *     RIF_Config() has started -- writing SECCFGR/PRIVCFGR before
   *     RIF_Config() SecureFaults the boot (see the LED note in main()).
   */
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_SDMMC1,
                                        RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOC,
    GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12,
    GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOH, GPIO_PIN_2,
    GPIO_PIN_SEC | GPIO_PIN_NPRIV);
#endif

  /* 4) Sleep-mode clock keep-alive (RM0486 14.10.105/93): the NuttX IDLE task
   *    runs WFI and the CPU enters Sleep.  RCC_APB5LPENR.LTDCLPEN and
   *    RCC_MEMLPENR (FLEXRAM/AXISRAM) reset to 0, so during Sleep the LTDC
   *    module clock AND the framebuffer SRAM clocks are gated -> the LCD
   *    goes black at the NSH prompt even though the LTDC is configured.
   *    Set the SET-alias registers so the clocks stay on during Sleep.
   *    Verified on real HW 2026-08-12 (fix in the NuttX ltdc driver too). */
  RCC->APB5LPENSR = RCC_APB5LPENR_LTDCLPEN;
  RCC->MEMLPENSR  = (RCC_MEMLPENR_FLEXRAMLPEN | RCC_MEMLPENR_AXISRAM1LPEN |
                     RCC_MEMLPENR_AXISRAM2LPEN);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* Progress LEDs for flash-boot bring-up (SWD debug is unavailable in flash
     boot mode on STM32N6, so LEDs are the only trace).  PG10 = LED0,
     PE10 = LED1, active low (verified on this board in Phase 1).
     NOTE: no HAL_GPIO_ConfigPinAttributes() here.  SECCFGR/PRIVCFGR are
     RIF-protected and writing them before RIF_Config() SecureFaults the
     boot (verified: versions with that call go fully dark).  The stock
     45-blink build had no such call and LED0/LED1 worked fine. */
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  GPIO_InitTypeDef led_gpio = {0};
  led_gpio.Pin   = GPIO_PIN_10;
  led_gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  led_gpio.Pull  = GPIO_NOPULL;
  led_gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &led_gpio);
  HAL_GPIO_Init(GPIOE, &led_gpio);
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET); /* LED0 ON: FSBL alive */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_SET);   /* LED1 OFF */

  /* USER CODE END 1 */

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_XSPI1_Init();
  MX_XSPI2_Init();
  MX_EXTMEM_MANAGER_Init();
  /* USER CODE BEGIN 2 */
  /* HyperRAM (XSPI1) init REMOVED: the LRUN boot flow (BOOT_Application)
     copies NuttX from the NOR (XSPI2) into internal SRAM and never touches
     the HyperRAM.  The HyperRAM_Init/EnableMemoryMappedMode calls left over
     from the XIP flow were hanging the boot in this area. */

  /* Program the RIF *before* launching the app: without this every bus
     master (DMA2D/GPDMA1/NPU/GPU2D) is blocked from writing SRAM. */
  RIF_Config();

  /* Progress: RIF configured, LRUN next.  LED1 ON / LED0 OFF. */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);

  /* Flush and disable the caches before the LRUN copy.  With a write-back
     D-Cache the byte-by-byte image copy in CopyApplication() would sit in
     cache and be lost when the boot code disables the caches right before
     jumping, leaving the SRAM image stale.  Cleaning first also pushes any
     pending RIFSC/RISAF register writes out to the peripherals. */
  SCB_CleanDCache();
  SCB_DisableDCache();
  SCB_DisableICache();

  /* USER CODE END 2 */

  /* ===== BOOT_Application (LRUN): map NOR, copy NuttX to SRAM, jump =====
     LED1 is turned ON just before the jump so a steady green LED marks a
     successful hand-over to NuttX; alternating LEDs = Error_Handler. */
  {
    extern BOOTStatus_TypeDef MapMemory(void);
    extern BOOTStatus_TypeDef CopyApplication(void);
    extern BOOTStatus_TypeDef JumpToApplication(void);

    if (BOOT_OK == MapMemory())
      {
        if (BOOT_OK == CopyApplication())
          {
            /* Verify the copied vector table at the jump target. */
            {
              uint32_t msp = *(volatile uint32_t *)0x34000400UL;
              uint32_t rst = *(volatile uint32_t *)0x34000404UL;
              if (!(msp >= 0x34000000UL && msp < 0x34200000UL &&
                    rst >= 0x34000000UL && rst < 0x34200000UL &&
                    (rst & 1UL)))
                {
                  /* Image looks wrong - refuse to jump. */
                  Error_Handler();
                }
            }

            /* Pre-configure USART1 kernel clock = HSI so the NuttX console
               baud rate is correct (RCC CCIPR13, USART1SEL = HSI = 6). */
            {
              volatile uint32_t *ccipr13 =
                (volatile uint32_t *)(0x56028000UL + 0x174UL);
              uint32_t v = *ccipr13;
              *ccipr13 = (v & ~0x7UL) | 0x6UL;
              __DSB();
            }

            /* Console probe: transmit three 'F's @115200 so the host sees
               the FSBL hand over to NuttX. */
            {
              USART_TypeDef *u = USART1;
              GPIO_InitTypeDef gi = {0};
              int k;

              gi.Pin       = GPIO_PIN_5 | GPIO_PIN_6;
              gi.Mode      = GPIO_MODE_AF_PP;
              gi.Pull      = GPIO_NOPULL;
              gi.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
              gi.Alternate = GPIO_AF7_USART1;
              HAL_GPIO_Init(GPIOE, &gi);

              /* USART1 clock enable (APB2ENSR @ RCC+0xA6C, bit4 = SET) */
              *(volatile uint32_t *)(0x56028000UL + 0xA6CUL) = 0x10UL;

              u->BRR = 0x453UL;              /* OVER8 @ 64MHz 115200 */
              u->CR2 = 0UL;
              u->CR1 = 0x800DUL;             /* UE|RE|TE|OVER8 */

              for (k = 0; k < 3; k++)
                {
                  while ((u->ISR & 0x80UL) == 0UL);   /* wait TXE */
                  u->TDR = 0x46UL;                    /* 'F' */
                }

              __DSB();
              HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_RESET); /* LED1 ON */
            }

            /* Jump-target self-check: print the copied vector table so a
               failed hand-over shows exactly what is at 0x34000400. */
            {
              USART_TypeDef *u = USART1;
              static const char hexc[] = "0123456789ABCDEF";
              uint32_t msp = *(volatile uint32_t *)0x34000400UL;
              uint32_t rst = *(volatile uint32_t *)0x34000404UL;
              int i;

              while ((u->ISR & 0x80UL) == 0UL);
              u->TDR = '[';               /* MSP=xxxxxxxx */
              for (i = 7; i >= 0; i--)
                {
                  while ((u->ISR & 0x80UL) == 0UL);
                  u->TDR = hexc[(msp >> (i * 4)) & 0xF];
                }
              while ((u->ISR & 0x80UL) == 0UL);
              u->TDR = ' ';               /* RST=xxxxxxxx */
              for (i = 7; i >= 0; i--)
                {
                  while ((u->ISR & 0x80UL) == 0UL);
                  u->TDR = hexc[(rst >> (i * 4)) & 0xF];
                }
              while ((u->ISR & 0x80UL) == 0UL);
              u->TDR = ']';
            }

            JumpToApplication();   /* does not return on success */
          }
      }
    Error_Handler();
  }
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
/* USER CODE BEGIN CLK 1 */
/* USER CODE END CLK 1 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the System Power Supply
  */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }

  /* Enable HSI */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Get current CPU/System buses clocks configuration and if necessary switch
 to intermediate HSI clock to ensure target clock can be set
  */
  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
  if ((RCC_ClkInitStruct.CPUCLKSource == RCC_CPUCLKSOURCE_IC1) ||
     (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_IC2_IC6_IC11))
  {
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK);
    RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
    {
      /* Initialization Error */
      Error_Handler();
    }
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL1.PLLM = 4;
  RCC_OscInitStruct.PLL1.PLLN = 75;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL1.PLLP1 = 1;
  RCC_OscInitStruct.PLL1.PLLP2 = 1;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2|RCC_CLOCKTYPE_PCLK5
                              |RCC_CLOCKTYPE_PCLK4;
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 2;
  RCC_ClkInitStruct.IC2Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC2Selection.ClockDivider = 3;
  RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC6Selection.ClockDivider = 4;
  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC11Selection.ClockDivider = 3;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief XSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_XSPI1_Init(void)
{

  /* USER CODE BEGIN XSPI1_Init 0 */

  /* USER CODE END XSPI1_Init 0 */

  XSPIM_CfgTypeDef sXspiManagerCfg = {0};
  XSPI_HyperbusCfgTypeDef sHyperBusCfg = {0};

  /* USER CODE BEGIN XSPI1_Init 1 */

  /* USER CODE END XSPI1_Init 1 */
  /* XSPI1 parameter configuration*/
  hxspi1.Instance = XSPI1;
  hxspi1.Init.FifoThresholdByte = 4;
  hxspi1.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
  hxspi1.Init.MemoryType = HAL_XSPI_MEMTYPE_HYPERBUS;
  hxspi1.Init.MemorySize = HAL_XSPI_SIZE_256MB;
  hxspi1.Init.ChipSelectHighTimeCycle = 2;
  hxspi1.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
  hxspi1.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
  hxspi1.Init.WrapSize = HAL_XSPI_WRAP_32_BYTES;
  hxspi1.Init.ClockPrescaler = 1 - 1;
  hxspi1.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
  hxspi1.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_DISABLE;
  hxspi1.Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_NONE;
  hxspi1.Init.MaxTran = 0;
  hxspi1.Init.Refresh = 0;
  hxspi1.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
  if (HAL_XSPI_Init(&hxspi1) != HAL_OK)
  {
    Error_Handler();
  }
  sXspiManagerCfg.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
  sXspiManagerCfg.IOPort = HAL_XSPIM_IOPORT_1;
  sXspiManagerCfg.Req2AckTime = 1;
  if (HAL_XSPIM_Config(&hxspi1, &sXspiManagerCfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  sHyperBusCfg.RWRecoveryTimeCycle = 7;
  sHyperBusCfg.AccessTimeCycle = 7;
  sHyperBusCfg.WriteZeroLatency = HAL_XSPI_LATENCY_ON_WRITE;
  sHyperBusCfg.LatencyMode = HAL_XSPI_FIXED_LATENCY;
  if (HAL_XSPI_HyperbusCfg(&hxspi1, &sHyperBusCfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN XSPI1_Init 2 */

  /* USER CODE END XSPI1_Init 2 */

}

/**
  * @brief XSPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_XSPI2_Init(void)
{

  /* USER CODE BEGIN XSPI2_Init 0 */

  /* USER CODE END XSPI2_Init 0 */

  XSPIM_CfgTypeDef sXspiManagerCfg = {0};

  /* USER CODE BEGIN XSPI2_Init 1 */

  /* USER CODE END XSPI2_Init 1 */
  /* XSPI2 parameter configuration*/
  hxspi2.Instance = XSPI2;
  hxspi2.Init.FifoThresholdByte = 4;
  hxspi2.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
  hxspi2.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
  hxspi2.Init.MemorySize = HAL_XSPI_SIZE_256MB;
  hxspi2.Init.ChipSelectHighTimeCycle = 1;
  hxspi2.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
  hxspi2.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
  hxspi2.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
  hxspi2.Init.ClockPrescaler = 1 - 1;
  hxspi2.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
  hxspi2.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_DISABLE;
  hxspi2.Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_NONE;
  hxspi2.Init.MaxTran = 0;
  hxspi2.Init.Refresh = 0;
  hxspi2.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
  if (HAL_XSPI_Init(&hxspi2) != HAL_OK)
  {
    Error_Handler();
  }
  sXspiManagerCfg.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
  sXspiManagerCfg.IOPort = HAL_XSPIM_IOPORT_2;
  sXspiManagerCfg.Req2AckTime = 1;
  if (HAL_XSPIM_Config(&hxspi2, &sXspiManagerCfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN XSPI2_Init 2 */

  /* USER CODE END XSPI2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable.
     NOTE: read-modify-write; the LL helpers overwrite the register, so the
     three separate calls would leave only GPIONEN set and clock-gate every
     other GPIO port (including GPIOG/GPIOE used for the LEDs). */
  RCC->AHB4ENSR |= (RCC_AHB4ENR_GPIOPEN | RCC_AHB4ENR_GPIOOEN |
                    RCC_AHB4ENR_GPIONEN);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  /* Both LEDs blink alternately, fast: BOOT_Application() failed
     (LRUN MapMemory/CopyApplication/JumpToApplication error). */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_RESET); /* LED1 ON */
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);   /* LED0 OFF */
  for (;;)
    {
      volatile uint32_t _d;
      HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET); /* LED0 ON */
      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_SET);   /* LED1 OFF */
      for (_d = 0; _d < 3000000U; _d++) { __NOP(); }
      HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);   /* LED0 OFF */
      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_RESET); /* LED1 ON */
      for (_d = 0; _d < 3000000U; _d++) { __NOP(); }
    }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
