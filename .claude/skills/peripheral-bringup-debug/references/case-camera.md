# 案例：STM32N647 DCMIPP + IMX335 摄像头黑屏（时钟源 SEL 铁证）

> 环境：NuttX(openvela) + 自定义 FSBL + ATK-DNN647（7寸 800x480）。
> 金标准：裸机 `352_IMX335_Appli_diag9f`（同一 FSBL）**能出图**（LTDC 实时显示）。
> 症状：NuttX 侧摄像头初始化「全部成功」，但画面黑屏；DCMIPP 报 `ERR1=0x2b`（RAW10 CRC 错误）。
>
> 📌 本案例体现**「同 FSBL 双世界对照法」**的「同时点逐位 diff」分支，
> 方法论文档见 `same-fsbl-golden-method.md`。

## 铁证：双端寄存器 dump 逐位 diff

| 项 | 裸机（出图 ✅） | NuttX（修复前 ❌） | 结论 |
|----|------------------|---------------------|------|
| `IC17CFGR` | `0x00030000`（SEL=0b00=**PLL1**） | `0x10030000`（SEL=0b01=**PLL2**） | ⚠️ 源错 |
| `IC18CFGR` | `0x003B0000`（SEL=0b00=**PLL1**） | `0x103B0000`（SEL=0b01=**PLL2**） | ⚠️ 源错 |
| `ERR1` | 0 | `0x2b`（RAW10 CRC 错误） | 数据错 |
| `IER1` | `0x1F1F`（D-PHY 零错误） | 0（D-PHY lane 错误全触发） | PHY 侧异常 |
| 显示 | ✅ | ❌ | — |

**ST 权威编码**：`LL_RCC_ICCLKSOURCE_PLL1 = 0U`（SEL=0b00）、`PLL2 = 0x10000000`（SEL=0b01）。
NuttX `hardware/stm32_rcc.h` 把 `IC17SEL_PLL1` **错定义成 `0x1<<28`（=PLL2）**。

## 修复

```c
/* hardware/stm32_rcc.h —— SEL 修正为 0b00 = PLL1 */
#define RCC_IC17CFGR_IC17SEL_PLL1   (0x0 << 28)   /* 原错误值: 0x1 << 28 (=PLL2) */
#define RCC_IC18CFGR_IC18SEL_PLL1   (0x0 << 28)
```

修复后：`IC17 = PLL1/4 = 300MHz`、`IC18 = PLL1/60 = 20MHz`（与裸机一致）→ **一次点亮**。

## 附带根因（同批修复，避免「修好一个还差一个」）

1. **capture-first 时序**：`start_capture`（VC0START+PIPEN+CPTREQ）必须在 sensor `start_stream` **之前**；
   命令序列 = `power_on → dcmipp_init → configure → start_capture → start_stream → wait_first_frame`。
2. **`wait_first_frame` 的等待对象**：等 **`frame_count` 增长**（ISR 在 `P1FRAMEF` 递增），
   ⚠️ **不要轮询 SOF0F**——CSI ISR 每次清 `FCR0=SR0` 会把 `SOF0F` 抢清，导致误报 `-110 (ETIMEDOUT)`。

## 关键教训（写入方法论）

1. **「配置值一致」≠「时钟源一致」**：分频比（/4、/60）对得上，但 **SEL 位（源选择）** 写错，
   实际频率完全不同。**对比时钟必须看完整寄存器（含 SEL 位）**。
2. **PHY 测试寄存器读回不可靠**（STM32N6 上）：裸机出图时读回 deskew/DLL 也是 `0x00`。
   读回 0x00 **不能**证明写入失败——不要用读回判断 PHY 状态，要用**行为**（出图/错误计数）。
3. **启动链差异**：裸机 app 不改系统时钟（FSBL 配置好即用）；**NuttX 会重配时钟树** →
   所有分频/源寄存器必须逐项与裸机 dump 对照。
4. **宏定义要查权威**：`stm32_rcc.h` 的手写宏很容易错位；对照 CMSIS `stm32n6xx.h` /
   HAL `LL_RCC_*` 枚举逐位验证。

## 诊断探针（可复用模式）

- `cam diag / hs / phy / rx`：分级打印 DCMIPP/CSI 寄存器与错误计数（`ERR1/IER1` 是数据正确性的第一指标）。
- 对比点选取：**「同一寄存器、同一打印时点」**（都在外设初始化完成后打印）才能直接 diff。
