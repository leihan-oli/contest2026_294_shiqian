/****************************************************************************
 * sdtest_main.c
 *
 * Standalone SD card test (mirror of the ALIENTEK 38_SD_Card example).
 * Runs AFTER the FSBL jumps (same copy/jump path as the NuttX image):
 *   - system clock switched back to HSI 64MHz (bare-metal pristine state)
 *   - USART1 @115200 for the console
 *   - ST HAL_SD_Init() on SDMMC1 (PC8-12/PH2, AF10)
 *   - prints SD OK / SD FAIL + error code, and card info when OK
 *
 * Used to isolate the problem: if this works, the FSBL environment is
 * fine and the bug is in the NuttX side; if it fails, the FSBL itself
 * leaves the SDMMC unusable.
 ****************************************************************************/

#include "stm32n6xx_hal.h"
#include <stdint.h>

/* newlib init-array hooks needed by the FSBL startup object */
void _init(void)
{
}

void _fini(void)
{
}

/* SysTick handler: HAL_Init() enables the 1 ms SysTick and HAL_Delay()
 * depends on HAL_IncTick().  The FSBL startup marks SysTick_Handler as
 * weak and aliases it to Default_Handler (infinite loop); without this
 * strong definition the first SysTick interrupt hangs the image about
 * 1 ms after HAL_Init() (observed: console stops mid-string). */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/* The FSBL startup calls __libc_init_array(); provide a no-op since we
 * link with -nostdlib (no libc crt). */
void __libc_init_array(void)
{
}

/* Tiny memset for the local BSS zeroing done in main(). */
void *memset(void *s, int c, unsigned int n)
{
  unsigned char *p = (unsigned char *)s;
  unsigned int i;
  for (i = 0U; i < n; i++)
    {
      p[i] = (unsigned char)c;
    }
  return s;
}

#define UART_NUM    USART1

static USART_TypeDef *g_u = USART1;

static void uart_putc(char c)
{
  while ((g_u->ISR & 0x80UL) == 0UL)  /* wait TXE */
    {
    }
  g_u->TDR = (uint8_t)c;
}

static void uart_str(const char *s)
{
  while (*s != '\0')
    {
      uart_putc(*s++);
    }
}

static void uart_hex(uint32_t v)
{
  static const char hexc[] = "0123456789ABCDEF";
  int i;

  uart_putc('0');
  uart_putc('x');
  for (i = 7; i >= 0; i--)
    {
      uart_putc(hexc[(v >> (i * 4)) & 0xF]);
    }
}

static void uart_init(void)
{
  GPIO_InitTypeDef gi = {0};

  __HAL_RCC_GPIOE_CLK_ENABLE();
  gi.Pin       = GPIO_PIN_5 | GPIO_PIN_6;
  gi.Mode      = GPIO_MODE_AF_PP;
  gi.Pull      = GPIO_NOPULL;
  gi.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  gi.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOE, &gi);

  /* USART1 clock enable (APB2ENSR @ RCC+0xA6C, bit4 = SET) */
  *(volatile uint32_t *)(0x56028000UL + 0xA6CUL) = 0x10UL;
  /* CCIPR13: USART1SEL = HSI (0x6) */
  *(volatile uint32_t *)(0x56028000UL + 0x174UL) = 0x6UL;

  g_u->BRR = 0x453UL;   /* OVER8 @ 64MHz -> 115200 */
  g_u->CR2 = 0UL;
  g_u->CR1 = 0x800DUL;  /* UE|RE|TE|OVER8 */
}

int main(void)
{
  RCC_ClkInitTypeDef clk = {0};
  RCC_PeriphCLKInitTypeDef pclk = {0};
  GPIO_InitTypeDef gi = {0};
  SD_HandleTypeDef hsd = {0};
  HAL_StatusTypeDef st;

  /* Undo FSBL SystemInit side effect: ASV/AVMEN only concern VDDA18ADC
   * monitoring; the bare-metal example never writes SVMCR3. */
  PWR->SVMCR3 &= ~(PWR_SVMCR3_ASV | PWR_SVMCR3_AVMEN);

  /* Mirror 38_SD_Card: enable I/D cache before HAL_Init. */
  SCB_EnableICache();
  SCB_EnableDCache();

  HAL_Init();
  SystemCoreClockUpdate();

  /* Switch the system clock to HSI (bare-metal 38_SD_Card state):
   * eliminates any FSBL PLL/config influence on the SD clock path. */
  clk.ClockType = RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK |
                  RCC_CLOCKTYPE_HCLK;
  clk.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  clk.AHBCLKDivider = RCC_HCLK_DIV1;
  (void)HAL_RCC_ClockConfig(&clk);
  SystemCoreClockUpdate();

  uart_init();
  uart_str("\r\n=== SDTEST (mirror of 38_SD_Card) ===\r\n");
  uart_str("[Q0]\r\n");   /* newest-build marker: must appear before MSP */

  /* --- SDMMC1 MSP (same as bare-metal stm32n6xx_hal_msp.c) ------------ */
  pclk.PeriphClockSelection = RCC_PERIPHCLK_SDMMC1;
  pclk.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_HCLK;
  (void)HAL_RCCEx_PeriphCLKConfig(&pclk);
  uart_str("[M1] periphclk done\r\n");

  __HAL_RCC_SDMMC1_CLK_ENABLE();
  uart_str("[M2] sdmmc clk en\r\n");
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  uart_str("[M3] gpio clk en\r\n");

  gi.Mode      = GPIO_MODE_AF_PP;
  gi.Pull      = GPIO_NOPULL;
  gi.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  gi.Alternate = GPIO_AF10_SDMMC1;
  gi.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOC, &gi);
  gi.Pin = GPIO_PIN_2;
  HAL_GPIO_Init(GPIOH, &gi);
  uart_str("[M4] gpio init done\r\n");

  /* --- Register-level probe (timeout protected, bypasses HAL waits) ------ */
  uart_str("\r\n  [Q1] PRE AHB5ENR=");
  uart_hex(*(volatile uint32_t *)(0x56028000UL + 0x260UL));
  uart_str(" CCIPR8=");
  uart_hex(*(volatile uint32_t *)(0x56028000UL + 0x160UL));
  uart_str(" CFGR1=");
  uart_hex(*(volatile uint32_t *)(0x56028000UL + 0x20UL));
  uart_str(" CFGR2=");
  uart_hex(*(volatile uint32_t *)(0x56028000UL + 0x24UL));
  uart_str("\r\n  SVMCR3=");
  uart_hex(PWR->SVMCR3);
  uart_str(" VTOR=");
  uart_hex(SCB->VTOR);
  uart_str(" AHB5RSTSR=");
  uart_hex(*(volatile uint32_t *)(0x56028000UL + 0x208UL));
  uart_str("\r\n  SDMMC1 STA=");
  uart_hex(SDMMC1->STA);
  uart_str(" POWER=");
  uart_hex(SDMMC1->POWER);
  uart_str(" CLKCR=");
  uart_hex(SDMMC1->CLKCR);
  uart_str(" CMD=");
  uart_hex(SDMMC1->CMD);
  uart_str("\r\n");

  /* NOTE: no manual SDMMC register poke here.  Any CMD0 issued before
   * HAL_SD_Init leaves the CPSM stuck in the Active state (CPSMACT=1) that
   * ICR cannot clear, which then makes HAL_SD_Init fail.  v6 mirrors the
   * bare-metal flow exactly: only HAL_SD_Init touches the SDMMC. */

  /* --- HAL_SD_Init (same as MX_SDMMC1_SD_Init) -------------------------- */
  hsd.Instance = SDMMC1;
  hsd.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
  hsd.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide             = SDMMC_BUS_WIDE_4B;    /* mirror MX_SDMMC1_SD_Init */
  hsd.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
  hsd.Init.ClockDiv            = 4;
  st = HAL_SD_Init(&hsd);

  if (st == HAL_OK)
    {
      uart_str("\r\nSD OK  ");
      uart_str("CardType=");
      uart_hex(hsd.SdCard.CardType);
      uart_str("  BlockNbr=");
      uart_hex(hsd.SdCard.BlockNbr);
      uart_str("  LogBlockNbr=");
      uart_hex(hsd.SdCard.LogBlockNbr);
      uart_str("\r\n");
    }
  else
    {
      uart_str("\r\nSD FAIL  ErrorCode=");
      uart_hex(hsd.ErrorCode);
      uart_str("  State=");
      uart_hex(hsd.State);
      uart_str("\r\n");
    }

  uart_str(">>> SDTEST DONE <<<\r\n");

  while (1)
    {
    }
}
