# arch — 芯片层（STM32N6）

openvela（NuttX）STM32N6 芯片层**完整源码**，对应 `nuttx/arch/arm/src/stm32n6/`。

## 内容（已填充）

| 路径 | 说明 |
|------|------|
| [arm/stm32n6/](arm/stm32n6/README.md) | 芯片层完整源码（80 源文件 + 头文件）+ 改动说明 |
| `arm/stm32n6/nuttx-core-incremental.patch` | openvela 核心树增量（arch/arm/Kconfig + arm_vectors.c） |

## 说明

- 源码来自当前固件实际编译使用的版本（`nuttx/arch/arm/src/stm32n6/`，Apache-2.0）
- 已包含本项目全部增量修复：`__start` SPSEL 检查（Flash boot 关键）、串口增强、ENSR 时钟修复、XSPI
- 参考 `docs/openvela-porting.md` 的已解决问题清单
