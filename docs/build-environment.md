# 构建环境说明（自包含）

> 本仓库为**完全自包含**：不依赖任何外部参考仓库，仅凭本仓库 + openvela 官方工作区即可构建出已烧录的 FSBL 与 nuttx/openvela 固件。
> 已通过验证：FSBL 重建结果与仓内镜像 `firmware/images/fsbl_rifsecfix.hex`
> **字节级一致**（md5 `032d1ffd3a5c`，85,300 B，2026-09-20 复核）。

## 0. 前置条件

- openvela 官方工作区（含 `nuttx/`、`apps/`、`vendor/openvela/boards/`、`prebuilts/`）
- ARM 工具链：`prebuilts/gcc/linux-x86_64/arm-none-eabi/`（或 PATH 中的 arm-none-eabi-gcc）
- Python + kconfiglib（openvela `prebuilts/tools/python/`）

## 1. openvela 工作区必需的环境修改

### 1.1 芯片层/板级层源码

将本仓库源码放入 openvela 工作区对应位置（**已解除 symlink，不再指向任何外部仓库**）：

| 本仓库 | openvela 工作区目标 |
|--------|---------------------|
| `arch/arm/stm32n6/src/` | `nuttx/arch/arm/src/stm32n6/` |
| `arch/arm/stm32n6/include/` | `nuttx/arch/arm/include/stm32n6/` |
| `board/atk-dnn647/` | `vendor/openvela/boards/atk-dnn647/` |

> ⚠️ 原 openvela 工作区中这些位置是 **symlink** 指向外部参考仓库，必须替换为真实目录。

### 1.2 openvela 核心树修改

- `nuttx/arch/arm/Kconfig`：新增 `ARCH_CHIP_STM32N6`（见 `arch/arm/stm32n6/nuttx-core-incremental.patch`）
- `nuttx/arch/arm/src/arm_m/arm_vectors.c`：诊断 LED 探针（可选，见同一 patch）

### 1.3 ⚠️ qemu_initialize.c weak 修复（必需）

`vendor/openvela/boards/common/src/qemu_initialize.c` 中的三个 board hook 是**强符号**，
与我们的 `board_bringup.c` 强定义冲突导致链接失败（`multiple definition`）。
需改为 weak（与同目录 `qemu_weakfunc.c` 模式一致，QEMU 虚拟板不受影响）：

```c
// 修改前 → 修改后
void board_early_initialize(void)
    → void __attribute__((weak)) board_early_initialize(void)
void board_late_initialize(void)
    → void __attribute__((weak)) board_late_initialize(void)
int board_app_initialize(uintptr_t arg)
    → int __attribute__((weak)) board_app_initialize(uintptr_t arg)
```

## 2. 构建 nuttx/openvela

```bash
cd ~/openvela
export PATH="$PWD/prebuilts/tools/python/bin:$PATH"
export PYTHONPATH="$PWD/prebuilts/tools/python/dist-packages"

# 全新构建（源码路径变化后需删除旧缓存）
rm -rf cmake_out/atk-dnn647_nsh
./build.sh vendor/openvela/boards/atk-dnn647/configs/nsh/ --cmake -j$(nproc)

# 产物
#   cmake_out/atk-dnn647_nsh/nuttx.bin  → 烧录 @ 0x70100000
# 成功标志: #### build completed successfully ####
```

## 3. 构建 FSBL（完全自包含）

```bash
cd ~/contest2026_294_shiqian
firmware/scripts/build_fsbl.sh

# 产物
#   firmware/build/fsbl.elf / fsbl.bin
# 生成可烧录 hex（自动加 header + 0x1C0 填充 + 修正字段）：
firmware/scripts/gen_fsbl_hex.py firmware/build/fsbl.bin firmware/build/fsbl.hex
#  → 烧录 @ 0x70000000
```

> FSBL 全部依赖均在本仓库 `firmware/` 内：
> `fsbl/`（Core 源码）、`stm32cube/`（HAL + CMSIS + ExtMem_Manager）、
> `bsp/`（HyperRAM + NORFlash）、`cubeide/`（startup.s + 链接脚本）。

## 4. 烧录与验证

| 固件 | 地址 | 文件 |
|------|------|------|
| FSBL | `0x70000000` | `firmware/build/fsbl.hex`（或 `firmware/images/fsbl_rifsecfix.hex`） |
| NuttX | `0x70100000` | `cmake_out/.../nuttx.bin`（或 `firmware/images/nuttx_errint_fix.bin`） |

- 烧录：STM32CubeProgrammer + External Loader `MX25UM25645G_ATK-CNN647B`
- 串口：PuTTY 115200（勿用控制 DTR/RTS 的软件）
