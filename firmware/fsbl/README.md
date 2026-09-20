# 自定义 FSBL（LRUN + RIF/RISAF 授权）

> 本项目原创改造，基于正点原子 STM32N647 软件包 FSBL 工程
> （`SoftwarePackage/FSBL/MX25UM25645G_W958D8NBYA5I_Example/`）。

## 为什么需要自定义 FSBL

- ST **预编译 FSBL** 使用 **XIP 模式**（应用需链接到 `0x70100400` 直接执行），且 **`HAL_RIF_MODULE_ENABLED` 被注释**（不做 RIF/RISAF 配置）。
- openvela/NuttX 链接在 **AXISRAM `0x34000400`**（SRAM），无法 XIP 启动 → 必须走 **LRUN（Load & Run）**：把镜像从 NOR 拷贝到 SRAM 再跳转。
- DEV boot 下 NPU/DMA2D/GPDMA1 写 SRAM 被 RISAF **拦截**（Flash boot 同样）→ 必须在 FSBL 里做 **RIF/RISAF 授权**。

## 关键改造点

### 1. `stm32_extmem_conf.h` — XIP → LRUN

```c
#include "boot/stm32_boot_lrun.h"
#define EXTMEM_LRUN_DESTINATION        EXTMEMORY_1
#define EXTMEM_LRUN_SOURCE             EXTMEMORY_1
#define EXTMEM_LRUN_DESTINATION_INTERNAL
#define EXTMEM_LRUN_DESTINATION_ADDRESS  0x34000400U   /* NuttX 链接地址 */
#define EXTMEM_LRUN_SOURCE_ADDRESS       0x100000U     /* NOR 0x70100000 */
#define EXTMEM_LRUN_SOURCE_SIZE          0x100000U
```

### 2. `stm32n6xx_hal_conf.h` — 启用 RIF

```c
#define HAL_RIF_MODULE_ENABLED
```

### 3. `main.c` — RIF_Config()（寄存器级，不依赖 HAL_RIF）

在 `BOOT_Application()` 前调用，完成：

| 步骤 | 内容 |
|------|------|
| 时钟 | `RCC->AHB3ENSR \|= RIFSCEN\|RISAFEN`、`AHB1ENSR \|= GPDMA1EN`（⚠️ **ENSR 是 SET-only，必须 `\|=` 合并，禁止分开调用 `__HAL_RCC_*` 覆盖写！**） |
| RIMC | DMA2D(8)/GPU2D(7)/NPU(1) → CID0 + SEC\|PRIV |
| GPDMA1 | 8 通道 CCIDCFGR = SCID_0\|CFEN（GPDMA 不在 RIMC 表！） |
| RISAF7 | FLEXMEM/AXISRAM（固件 `0x34000400`），End=`0x7FFFF` |
| RISAF1 | 系统空间，End=`0x3FFFFFFF` |
| CFGR | **必须含 SEC 位**（`RISAF_REGx_CFGR_SEC` bit8，CFGR=`0xff0101`）——secure CPU 只能写 SECURE 区域，否则 CopyApplication 卡死 |

> 注意：IS_RISAF_LIMIT 检查 `addr < limit+1`，EndAddress=`0xFFFFFFFF` 会 32 位溢出 assert 失败 → 用 `0x3FFFFFFF`。

### 4. `main.c` — LRUN 启动流程

```text
LED 标记 → cache → HAL_Init → SystemClock(600MHz) → GPIO → XSPI1(HyperRAM 已移除) 
→ XSPI2(NOR) → EXTMEM/NORFlash init → RIF_Config → cache flush/disable → BOOT_Application(LRUN)
```

- **HyperRAM(XSPI1) 初始化已移除**：LRUN 流程只映射/拷贝 NOR(XSPI2)→SRAM，不碰 HyperRAM（XIP 遗留的卡死点）。
- 跳转前 **flush + disable cache**（write-back cache 数据回写，避免拷贝后跳转读到脏数据）。
- GPIO 引脚安全属性：FSBL 不调用 `HAL_GPIO_ConfigPinAttributes`（写 SECCFGR/PRIVCFGR 在 RIF 配置前会 SecureFault），引脚保持复位 SECURE 态 → **NuttX 侧必须用 secure 别名（`0x5602xxxx`）访问外设**（见 `arch/arm/stm32n6` 的说明）。

### 5. `postbuild.sh` — FSBL header 结构修复

STM32N6 BootROM 启动机制（真机验证）：

```text
header(0x000-0x240) + 0x1C0 全零填充(0x240-0x400) + 代码(0x400+，链接基址 0x34180400)
```

- BootROM 把整个 FSBL 加载到 `0x34180000`，**从 file 偏移 0x400 处的向量表读 MSP/PC 启动**（不是 header 0x70 入口！）
- **`0x1C0` 全零填充必须存在**（`0x240-0x400`），否则代码错位 → 全灭
- `0x64` = `byte_sum(image)`（逐字节求和 mod 2^32，非 CRC）
- `0x6C` = image size（= `0x1C0` + 实际 bin 长度，**必须按实际 bin 重算**，否则截断崩）
- `0x70` = entry（= bin[4] Reset_Handler）

## 调试方法（LED 标记定位）

main.c 内嵌 `DBG_BLINK`（LED0 计数）与 `DBG_BLINK_LED1`（LED1 段标记）宏：

- LED0 闪烁次数 = main() 流程进度（1=进入 main … 10=RIF 完成）
- LED1 x1..x8 = RIF_Config 内部子步骤（精确定位 fault 写入）
- 跳转后 LED1 段标记 = LRUN 拷贝/验证进度

> 这些标记在 bring-up 阶段使用，最终版可保留（不影响功能）。

## 构建

- **Linux 侧**：`scripts/build_fsbl.sh`（arm-none-eabi-gcc，`-mcmse` + `--build-id=none`，完整 objcopy 禁 `-j` 过滤）
- **Windows/CubeIDE**：工程 + `postbuild.sh`（本目录副本为修复版）

## 启动链（最终）

```text
BootROM → FSBL(LRUN+RIF) @ 0x34180000 → CopyApplication → 0x34000400
→ JumpToApplication → NuttX start() → arm_initialize_stack → __start
→ __start_c → ABCD → nx_start → NSH
```
