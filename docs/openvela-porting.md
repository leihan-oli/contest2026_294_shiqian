# openvela 移植记录（STM32N647）

> 完整调试流水见外部工作区 `SoftwarePackage/SESSION_HANDOFF_2026-08-08.md` 与 `plan*.md`。
> 本文件收录可复用的移植知识，随项目更新。

## 1. 芯片/板卡事实

- **芯片**：STM32N647X0H3Q（Cortex-M55，ARMv8.1-M），secure/NS 内存别名
  - secure = `0x5xxxxxxx`，NS = `0x4xxxxxxx`
- **板卡**：正点原子 ATK-DNN647
  - LED0=PG10（红）、LED1=PE10（绿），**active-low**
  - 按键 KEY0=PC6 / KEY1=PD1 / KEY2=PG11 / WKUP=PC13
  - USART1=PE5(TX)/PE6(RX) AF7 115200 OVER8，base=`0x52001000`（secure）
- **时钟**：HSI 64MHz → PLL1 M4/N75 → 600MHz CPU / 400MHz AXI / 200MHz HCLK
- **内存**：AXISRAM @ `0x34000400`（4MB-1KB）
- **外存**：
  - NOR MX25UM25645G（XSPI2 @ `0x70000000`，32MB）— **启动介质**
  - HyperRAM W958D8NBYA5I（XSPI1 @ `0x90000000`）

## 2. 启动链

```text
BootROM → FSBL(LRUN+RIF) @ 0x34180000
  → CopyApplication → 0x34000400
  → JumpToApplication
  → NuttX start() → arm_initialize_stack → __start
  → __start_c → ABCD → nx_start → NSH
```

## 3. 已解决的关键问题（10 条坑）

| # | 问题 | 根因 | 解决 |
|---|------|------|------|
| 1 | Flash boot HardFault | MSP==PSP 冲突，中断覆盖任务上下文 → IBUSERR | `__start` 加 CONTROL.SPSEL 检查 |
| 2 | USB 插拔"卡死" | 串口软件(XCOM) DTR/RTS 触发一键下载复位电路 | 改用 PuTTY |
| 3 | RIF_Config 卡死 | RISAF 时钟未使能 + AHB3ENSR 覆盖写 | 合并一次写 + 加 RISAF 时钟位 |
| 4 | CopyApplication 卡死 | RISAF 区域 SEC=0 | CFGR 加 SEC 位（=0xff0101） |
| 5 | NuttX 跳转后执行垃圾 | `.note.gnu.build-id` 推偏移 0x400 | flash.ld DISCARD |
| 6 | DEV boot 无输出 | ENR 只读 | 全部改 ENSR（置位） |
| 7 | 串口死循环 | rxavailable 逻辑 | 修复 stm32n6_serial.c |
| 8 | 串口丢字节/卡死 | FE/NE/ORE 未清理 | ISR 清理无效字节 |
| 9 | 中断误触发 | — | — |
| 10 | 时钟分支不匹配 | FSBL 600MHz vs NuttX 不同 | 统一 600MHz 分支 |

## 4. 调试方法

- **GDB/OpenOCD**：`stm32n6x.cfg`（强制 Secure AP 访问）+ `run.gdb`
- **判断系统是否死**：卡死后 attach，PC=up_idle 表示系统没死
- **串口**：PuTTY 115200（勿用控制 DTR/RTS 的 XCOM）
- **DSCSR** `0xE000EE08` CDS=bit16，复位默认 `0x30000`

## 5. 已烧录固件（勿覆盖）

| 固件 | 地址 | 文件 |
|------|------|------|
| FSBL | @ `0x70000000` | `fsbl_rifsecfix.hex` |
| NuttX | @ `0x70100000` | `nuttx_errint_fix.bin` |

## 6. 待办验证

- [ ] RIF 授权真机验证（DMA2D/GPDMA1 写 SRAM，NPU 前置）
- [ ] XSPI P1/P2（NuttX 侧 NOR 读写）
- [ ] LTDC framebuffer + LVGL
