# arch/arm/stm32n6 — 芯片层源码（STM32N6）

openvela（NuttX）STM32N6 芯片层**完整源码**（当前固件实际编译使用的版本）。

> 来源：openvela 工作区 `nuttx/arch/arm/src/stm32n6/`（完整内嵌本仓库）。
> 芯片层源码源自 NuttX STM32N6 初始 port 基座（Apache-2.0，源码头文件保留版权声明），
> 已应用本项目全部增量修复（启动/串口/时钟/XSPI），与已烧录固件 `nuttx_errint_fix.bin` 对应。

## 源码树

| 路径 | 说明 |
|------|------|
| `src/` | 芯片层实现（80 个源文件，含 hardware/ 寄存器定义） |
| `include/` | 芯片层头文件（chip.h、irq.h） |

> openvela 核心树改动（`arch/arm/Kconfig` 新增 STM32N6 支持 + `arm_vectors.c` 诊断 LED）
> 以 `nuttx-core-incremental.patch` 提供（核心树文件过大，仅提交增量）。

## 本项目改动清单（逐文件）

### 启动（Flash boot 跑通关键）

| 文件 | 改动 | 说明 |
|------|------|------|
| `stm32n6_start.c` | `__start` 加 **CONTROL.SPSEL 检查** | FSBL 从 `arm_initialize_stack` 设 PSP 启动，若 SPSEL=1 则**保留中断栈 MSP**、不覆盖成 `g_idle_topstack`。否则 MSP==PSP 冲突 → 中断覆盖任务上下文 → IBUSERR。**Flash boot 跑通的关键修复**。 |

### 串口（增强）

| 文件 | 改动 |
|------|------|
| `stm32n6_serial.c` | ① 中断清 FE/NE/ORE 错误标志 ② FE/NE+RXNE 时丢弃无效字节 ③ `rxavailable` 防死循环（原逻辑 `head!=tail` 永不满足时卡死） |

### 时钟（ENSR 三件套修复）

STM32N6 RCC 是 **ENR(只读状态)/ENSR(write-1-set)/ENCR(clear)** 三件套。
原代码用 `putreg32(..., ENR)` 使能时钟 **无效**（ENR 只读）→ DEV boot 下外设时钟全没使能 → 串口无输出（Flash boot 靠 FSBL 预使能掩盖了此 bug）。

| 文件 | 改动 |
|------|------|
| `stm32n6_rcc.c` | GPIOE/USART1 时钟改写 `ENSR` |
| `stm32n6_gpio.c` | GPIO 端口时钟改写 `ENSR` |
| `stm32n6_dma.c` | GPDMA1 时钟改 `ENSR` |
| `stm32n6_adc.c` | ADC12 时钟改 `ENSR` |
| `stm32n6_tim.c` | TIM15 时钟改 `ENSR` |
| `stm32n6_lowputc.c` | 相关时钟改动 |

### 其他

| 文件 | 改动 |
|------|------|
| `stm32n6_xspi.c` | XSPI P1/P2 支持（NOR/HyperRAM），bus1 基址修正（`0x90000000`） |
| `stm32n6_gpio.h` | 补充 GPIO 宏（+83 行） |
| `hardware/stm32_memorymap.h` | RIFSC/RISAF 基址等补充 |
| `hardware/stm32_rcc.h` | ENSR 寄存器位定义补充 |
| `Kconfig` / `Make.defs` / `CMakeLists.txt` | 编译集成 |

### openvela 核心树

| 文件 | 改动 |
|------|------|
| `arch/arm/Kconfig` | 新增 `ARCH_CHIP_STM32N6`（Cortex-M55、MVE、ICACHE/DCACHE/ITCM/DTCM） |
| `arch/arm/src/arm_m/arm_vectors.c` | `start()` 增加诊断 LED 探针（**调试遗留**，secure 别名 `0x56021818/0x56021018`，最终版可清理） |

## 关键知识点（踩坑）

- **secure/NS 别名**：secure=`0x5xxxxxxx`、NS=`0x4xxxxxxx`。FSBL 不配 GPIO 引脚安全属性（引脚复位默认 SECURE）→ NuttX **必须用 secure 别名**访问外设，NS 别名写 secure 外设 → SecureFault。
- **ENSR 是 SET-only**：使能时钟用 `putreg32(位值, ...ENSR)`，禁止写 `ENR`。
- **时钟分支**：FSBL 配 600MHz CPU/400MHz AXI/200MHz HCLK，board.h 必须匹配（见 `board/atk-dnn647`），否则 SysTick 差 9 倍 → OS 崩溃。
