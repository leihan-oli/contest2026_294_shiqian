# core-tree-overlay — 目录外改动的「直接覆盖」备份

本目录存放本作品对 **openvela 公共树**的 13 处改动（内容 = 补丁应用后的结果），
分布在 4 个仓库里。它们是**构建必需**的（缺了会编译失败或功能不对），但不属于
本仓的 `board/` `arch/` `app/` `libs/` 映射目录，所以要另外交付。

| overlay 子目录 | 目标仓库（工作区路径） | 补丁文件 | 文件数 | 改了什么 |
|----------------|------------------------|----------|--------|----------|
| `nuttx/` | `nuttx/` | `arch/arm/stm32n6/nuttx-core-incremental.patch` | 9 | 根 `Kconfig` 引入 `LIB_AI_ATON`；`arch/arm/Kconfig` 增加 `ARCH_CHIP_STM32N6` 与芯片 Kconfig 挂接；`arm_m` 四个 fault handler 打印增强 + `arm_vectors.c` 启动诊断；`armv8-m/arm_cache.c` 修 cache 维护；新增 `include/nuttx/aie/stm32n6_aton_aie.h` |
| `apps/` | `apps/` | `app/apps-core-incremental.patch` | 2 | `audioutils/speexdsp/CMakeLists.txt`（音频尝试期的构建修正）、`examples/lvgldemo/lvgldemo.c`（LVGL 演示适配） |
| `vendor-openvela/` | `vendor/openvela/` | `board/vendor-openvela-core-incremental.patch` | 1 | `boards/common/src/qemu_initialize.c` 三个 board hook 改 **weak**（否则与 `board_bringup.c` 冲突，`multiple definition` 直接编译失败） |
| `apps-lvgl/` | `apps/graphics/lvgl/lvgl/` | `app/lvgl-core-incremental.patch` | 1 | `src/draw/lv_draw_buf_blur.c` 的 MVE 编译门控（openvela 选中了 `CONFIG_ARM_HAVE_MVE` 但没加 `-march=…+mve`，开 MVE 路径会编译失败） |

## 两种应用方式（结果完全一致）

在工作区根目录执行（即本仓的上一级）：

```bash
# 方式 A（默认）：打补丁，自动尝试 git apply → 失败则 git apply -3
bash contest2026_294_shiqian/scripts/apply-core-tree.sh

# 方式 B（兜底）：直接用 overlay 覆盖，不依赖上游文件内容
bash contest2026_294_shiqian/scripts/apply-core-tree.sh --overlay

# 只检查当前状态（是否已应用 / 是否匹配）
bash contest2026_294_shiqian/scripts/apply-core-tree.sh --check
```

> 何时用 B：`repo sync` 到的上游分支已经前移，`git apply` 报
> `patch does not apply` 且三方合并也失败时。overlay 是补丁应用后的**完整文件**，
> 覆盖即可得到同样的树。

## 与补丁的等价性

`scripts/verify-core-tree.sh` 会用「上游基线 + 补丁」重建这些文件，并与 overlay 逐
字节比对（本仓自检的一部分）。
