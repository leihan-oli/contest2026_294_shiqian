# 案例：STM32N647 SDMMC1 + TF 卡 bringup（8 项根因链）

> 环境：NuttX(openvela) + 自定义 FSBL（RIF 授权 + LRUN 拷贝到 SRAM）+ ATK-DNN647。
> 金标准：裸机 `baremetal_tfcard` 在**同一 FSBL** 下实测能识别卡（`LogBlockNbr=30535680`）。
> 驱动：`nuttx/arch/arm/src/stm32n6/stm32n6_sdmmc.c`（raw SD + NuttX block_operations 桥接）。
> 全程现象：命令状态机冻结/超时，寄存器 dump 与裸机「看起来逐项一致」。
>
> 📌 本案例是**「同 FSBL 双世界对照法」**的完整实例（`[P]/[QX]/[E3]/[E3D]/Q1` 探针矩阵 + 反向实验），
> 方法论文档见 `same-fsbl-golden-method.md`。

## 症状演进时间线（每一步都是新根因浮出的标志）

| 阶段 | 症状 | 当轮根因 |
|------|------|---------|
| 1 | `CMD0` 冻结，`STA=0x2000`（CPSMACT 常驻） | VDDIO4 供电域未使能（终极根因） |
| 2 | CMD0 通后 `card not ready`（ACMD41 重试耗尽） | CMD55 当无响应命令发送（响应语义） |
| 3 | `CMD9 CSD failed: -110`（CTIMEOUT） | CMD9 在 Transfer 态（CMD7 之后）发送 |
| 4 | `card initialized` 后 mount `-15 (ENOTBLK)` | 板级 `nx_mount(NULL,...)` source 参数错 |
| 5 | mount `-5 (EIO)` | `DTIMER=0`（数据超时立即触发） |
| 6 | `CMDSENT timeout (STA=0x81000)` | `DCTRL` 先置 `DTEN=1` → 命令被 DPSM 阻塞 |
| 7 | 数据阶段不启动（`STA=0x45000`） | `DCTRL=0`：无方向/块大小信息 |
| 8 | `DCOUNT` 冻结在 `0x1F0`（16B/32B） | FIFO 读取与 HWFC 流控冲突（未排空） |
| 9 | DCOUNT 达标但 mount `-19 (ENODEV)` | **块驱动 read/write 返回字节数而非扇区数** |
| 10 | 全部通后 mount 仍 `-19` | 卡未格式化（扇区 0 全 0）；`mkfatfs -F 32` 后全通 |

## 8 项根因链（按发现顺序）

### 1. VDDIO4 供电域未使能（终极根因）

- **现象**：`CMD0` 永久冻结（`CPSMACT=1`），一切寄存器配置与裸机 diff 相同。
- **证据链**（三元组自洽）：
  - 裸机 `[QX]`（HAL_Init 前）失败 + 裸机 `[P]`（HAL_Init 后）成功 + NuttX 全失败；
  - 差集 = `HAL_MspInit` 的 `HAL_PWREx_EnableVddIO4()`；
  - AN5967 引脚域表：`VDDIO4 = PC[1], PC[12:6], PH[9,2]` —— **SDMMC1 的 PC8-12/PH2 全在域内**。
- **修复**：`stm32n6_start.c` 中 `PWR_SVMCR1.VDDIO4VRSEL=0`（3.3V）+ `VDDIO4SV=1`（**在 SVMCR1，不是 SVMCR3**）。
- **教训**：寄存器全对但引脚不动 → 优先查**供电域 SV 位**。

### 2. GPIO 端口时钟门控（探针/驱动写的键值被静默丢弃）

- **现象**：`[E3]` 探针写 `GPIOC MODER/AFR` 后，`CMD0` 仍冻结。
- **证据**：写后读回 `MODER=0`（复位值）→ 写入被时钟门控丢弃。
- **根因**：本板 `stm32_boardinitialize()` 为空，无任何代码统一开 GPIO 时钟。
- **修复**：写 GPIO 寄存器前 `AHB4ENSR` 置 `GPIOCEN|GPIOHEN`（对齐裸机 `__HAL_RCC_GPIOC/GPIOH_CLK_ENABLE`）。
- **教训**：**所有 GPIO 裸操作前先确认端口时钟**；读回值可立刻证伪。

### 3. 响应语义：CMD55/CMD2/CMD7/CMD16 当无响应命令发送

- **现象**：CMD0 通后 `card not ready`（ACMD41 永不响应）。
- **根因**：`send_cmd(..., NULL)` 只等 `CMDSENT` 不等 R1 → 命令状态机重叠。
- **修复**：全部改等 R1（CMD2 用长响应 136-bit）。
- **教训**：**响应有无不是可选项**——照抄 HAL 每条命令的 `Response=` 类型。

### 4. CMD9 位置：必须在 CMD7 之前（Standby 态）

- **现象**：`CMD9 CSD failed: -110`（CTIMEOUT）。
- **根因**：CMD9 放在 CMD7 选卡之后（Transfer 态），很多卡在 Transfer 态不响应 CMD9。
- **修复**：顺序改为 `CMD2 → CMD3 → CMD9 → CMD7 → CMD16`（对齐 HAL `SD_InitCard`）。
- **教训**：命令的**状态机时机**与命令本身同等重要。

### 5. ACMD6 缺失（控制器切了 4-bit，卡还是 1-bit）

- **现象**：数据读无 start bit；`DCOUNT` 冻结；数据线 IDR 全程高（无活动）。
- **根因**：只改了控制器 `CLKCR.WIDBUS=4bit`，**没有发 ACMD6（SET_BUS_WIDTH, arg=2）** 给卡。
- **修复**：切控制器总线位前先 `CMD55(RCA<<16) → CMD6(arg=2)`。
- **教训**：**「谁的总线谁来切」**——控制器与卡的配置是两件事。

### 6. HWFC_EN（RM0486 数据收发步骤第 2 步）

- **现象**：数据完全不来（`i=0`）。
- **根因**：`CLKCR` 缺 `HWFC_EN`（bit17）；RM0486 数据收发流程明文要求；裸机值 `0x24004` 含此位。
- **修复**：`CLKCR |= HWFC_EN`（与裸机逐位一致）。
- **教训**：**即使命令全通、初始化全过**，数据阶段还有独立要求（本项 + 下项）。

### 7. FIFO 读取策略与 HWFC 流控冲突（冻结在 16/32 字节）

- **现象**：`DCOUNT` 停在 0x1F0/0x1E0（16/32B）；`DATline_changes≈0`（**时钟被冻**）。
- **根因**：HWFC 传输中「FIFO 不满足半空条件」会**停止 SDMMC_CK**；固定读 4/8 字不排空 → 永不满足 → 卡被冻死。
- **修复**：`RXFIFOHF` 触发后**循环读到 `RXFIFOE`（彻底排空）**。
- **教训**：软件 FIFO 策略必须与硬件流控的恢复条件对齐；`DCOUNT`/`IDR` 是判断「谁冻了」的关键仪表。

### 8. 块驱动 read/write 返回「扇区数」而非字节数

- **现象**：数据 512B 全读对（`first bytes=EB 58 90 4E` 正确 MBR），mount 仍 `-19 (ENODEV)`。
- **根因**：NuttX `block_operations.read` 约定返回**扇区数**；返回 512 使 `fat_hwread` 的
  `if (nsectorsread == nsectors)`（512 == 1）恒假 → 恒 `-ENODEV`。
- **修复**：`return (ssize_t)nsectors;`（read/write 各一处）。
- **教训**：**RTOS 框架的返回语义要用框架源码校验**（`fat_hwread`/`fat_hwwrite` 全文），不要凭习惯。

## 决定性实验记录（方法论沉淀）

| 实验 | 目的 | 方法 | 结论 |
|------|------|------|------|
| `[E3]` GPIO-aware 探针 | 区分「启动环境 vs 后续环境」 | 在 `__start_c` 的 `'B'` 之后跑裸机 CMD0 序列 | 排除启动环境；暴露 GPIO 时钟问题 |
| `[E3B]/[E3C]` | 测 SPSEL/无关时钟位 | ⚠️ **无效**：跑在被冻结的外设上 | **教训：实验前必须复位外设** |
| `[E3D]` | 干净状态测 SPSEL | `RSTSR/RSTCR` 复位 + **naked 汇编**清 SPSEL 跑 CMD0 | 干净排除 SPSEL（C 内切栈会崩） |
| `[QX]`（裸机侧） | 反向二分：裸机成功依赖什么 | 在 `HAL_Init` **之前**跑同样序列 | 失败 → 差分锁定到 `HAL_MspInit`（VDDIO4） |
| `Q1` | 驱动内环境对齐实验 | 对齐 CCR 后跑裸机序列 | 排除 CCR；证明驱动路径可通 |
| IDR 监测 | 判断卡是否在数据线发数据 | 循环采样 `GPIOC IDR`（只统计 D0-3 位）| 区分「卡没发」vs「控制器没采到」 |
| R1 解读 | 判断命令是否被卡接受 | `R1=0x900` = READY_FOR_DATA + **current_state=4(Transfer)** | 命令链路 OK，问题在数据阶段 |

## 交付陷阱（非驱动问题，但阻断验收）

1. **新卡没有文件系统**：扇区 0 全 0x00 → mount 必失败（错码如 -ENODEV）；
2. **`mkfatfs` 默认只试 FAT12/16**：15GB 卡全部放不下 → `-ENFILE(23)`；
   显式 `mkfatfs -F 32 /dev/mmcsd0`；
3. 验证读回：`ls` → `echo hello > f` → `cat f`（证明读+写双向打通）。

## 最终验证（全链路）

```
sdmmc1: card initialized, RCA=00010000, 30535680 blocks (CSD v1)
sdmmc1: card switched to 4-bit (ACMD6 ok)
mount 成功（无错误）
nsh> echo hello > /mnt/sdcard/test.txt && cat /mnt/sdcard/test.txt  → hello
```
