# libs — 系统库

| 目录 | 内容 | 映射到 openvela 工程 |
|------|------|----------------------|
| `ai_aton/` | **ATON NPU 运行时**（本项目在 NuttX 上的落地） | `nuttx/libs/ai_aton`（manifest `<linkfile>`） |

## libs/ai_aton 结构

| 子目录 / 文件 | 说明 |
|---------------|------|
| `ll_aton_osal_nuttx.c`、`ll_aton_osal_user_impl.h` | 我们用 NuttX 原语（`nxsem` / `nxmutex`）实现的 OSAL：WFE 信号量、ATON/NPU/cache 锁、临界区、DSB —— 官方只给了 FreeRTOS/ThreadX 参考实现 |
| `nuttx/` | 设备胶水：`aton_model.c`（模型装载与推理入口）、`npu_cache.c` / `mcu_cache.c`（CACHEAXI 与 CPU cache 维护） |
| `compat/` | ST 头文件在 NuttX 下的最小兼容垫片 |
| `models/` | 模型源码与权重：`eye/`（眼动五分类，**产品在用**）、`face/`（YuNet 人脸检测，**产品在用**）、`palm995/` `pose994/`（移植期样例）、`*_bak/`（历史版本留档） |
| `vendor/` | ST 官方 ATON 运行时素材：`Inc/`、`ll_aton/`、`Devices/`、`lib/NetworkRuntime1000_CM55_GCC.a`、`LICENSE.txt`（Apache-2.0） |

`CMakeLists.txt` / `Kconfig` 提供 `CONFIG_LIB_AI_ATON` 开关，编译为 `nuttx/libs/ai_aton`
内的静态库并链入内核镜像。

> 换模型 / 重训后重新落板的完整流程见 `.claude/skills/aton-model-deploy/SKILL.md`：
> 契约核对 → `.nsblob` 钩子 → 量化常量（含派生 `Q_GAIN` / `Q_BIAS`）→ Intel HEX 权重镜像
> → 边界自检 → 板端验证。
