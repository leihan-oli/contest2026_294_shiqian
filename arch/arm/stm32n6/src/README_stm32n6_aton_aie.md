# STM32N6 Neural-ART NPU AI Engine 驱动 (/dev/aie0)

> openvela(NuttX) + STM32N647 (ATK-DNN647) | Phase 7 轨道 B
> 驱动位置: `arch/arm/src/stm32n6/stm32n6_aton_aie.c`

## 简介

本驱动把 STM32N6 内嵌的 **Neural-ART NPU 加速器**(ST 自研, ~600 GOPS)
通过 **AI Engine (AIE) lower-half** 接口接入 NuttX,注册为 `/dev/aie0`。
应用通过标准 AIE ioctl (`AIE_CMD_LOAD/FEED_INPUT/GET_OUTPUT`) 进行推理,
最终由 ST **ATON 运行时**(`stai_*` API)驱动 NPU 硬件。

采用 arch 层实现(同 `stm32n6_video.c` 模式),因为需要访问芯片寄存器。

## 工作原理

```
应用 (ioctl /dev/aie0)
  → AIE upper-half (drivers/aie/ai_engine.c, 零改动)
  → [本驱动] stm32n6_aton_aie lower-half
      ├─ RCC: NPU 时钟使能 + 复位释放
      ├─ IRQ: NPU0-3 (NVIC 53-56) 注册
      └─ aie_register("/dev/aie0")
  → (ATON 运行时, 待模型生成后接入)
  → Neural-ART NPU @ 0x580E0000 + CACHEAXI_RAM @ 0x343C0000
```

## 支持功能

- ✅ AIE lower-half 注册 (`/dev/aie0`)
- ✅ NPU 时钟使能 (RCC AHB5ENR.NPUEN) + 复位释放 (AHB5RSTR.NPURST)
- ✅ NPU 中断注册 (STM32_IRQ_NPU0..3, 4 线)
- ✅ 会话管理 (AIE_CMD_LOAD 单会话门禁)
- ⏳ ATON 运行时调用 (TODO, 模型生成后接入)
- ⏳ 实际推理 (feed_input/get_output, TODO)
- ⏳ OSAL USER_IMPL (仿 ethosu_platform.c, 链接运行时后)
- ⏳ RISAF8 CACHEAXIRAM + RIMC NPU CID 授权 (FSBL 侧)

## 寄存器概览

| 资源 | 地址/值 | 说明 |
|------|---------|------|
| NPU 基址 | 0x580E0000 (secure) | `STM32_NPU_BASE` |
| CACHEAXI_RAM | 0x343C0000 (256KB) | `STM32_NPU_CACHEAXIRAM_BASE`, RISAF8 |
| NPU IRQ | NVIC 53/54/55/56 | `STM32_IRQ_NPU0..3` (NuttX 已定义) |
| RCC AHB5ENR.NPUEN | bit31 | NPU 时钟使能 |
| RCC AHB5RSTR.NPURST | bit31 | NPU 复位 |

> NuttX 运行在 **secure 域** (0x50000000 外设基址实测),故用 secure 地址。
> ATON 编译时需 `-DCPU_IN_SECURE_STATE` (ATON_BASE=NPU_BASE_S 正确)。

## 使用方法

### Kconfig 使能

```
CONFIG_AI_ENGINE=y              # AIE upper-half (drivers/aie)
CONFIG_STM32N6_ATON_AIE=y       # 本 lower-half
CONFIG_STM32N6_ATON_AIE_DEVPATH="/dev/aie0"
```

board_bringup.c 在 `CONFIG_STM32N6_ATON_AIE` 下自动调用
`stm32n6_aton_aie_initialize()` 完成注册。

### 应用调用 (ioctl)

```c
#include <nuttx/aie/ai_engine.h>
#include <nuttx/aie/stm32n6_aton_aie.h>

int fd = open("/dev/aie0", O_RDONLY);

/* 加载会话 */
ioctl(fd, AIE_CMD_LOAD, 0);             /* 0: 模型地址 (ATON 为编译期模型) */

/* 推理参数 (ATON 落地后) */
struct stm32n6_aton_invoke_params p = {
  .model = model_addr, .input = in_buf, .output = out_buf,
  .input_size = ..., .output_size = ...
};

ioctl(fd, AIE_CMD_FEED_INPUT, (unsigned long)&p);
ioctl(fd, AIE_CMD_GET_OUTPUT, (unsigned long)&p);

close(fd);
```

## 已知限制

1. **ATON 运行时未链接** — 当前回调为桩 (返回 OK), 需等模型训练阶段完成后:
   - 链接 `NetworkRuntime1000_CM55_GCC.a`
   - 编译定义 `-DCPU_IN_SECURE_STATE -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6(12)`
   - 提供 ST HAL 头 (`stm32n6xx.h`/`mcu_cache.h`/`npu_cache.h`), 实现 OSAL USER_IMPL
2. **RISAF/RIMC 授权** — NPU 访问内存需 FSBL 授权 CACHEAXI_RAM (RISAF8)
   与 NPU 主设备 CID (RIMC), 参考 `stm32n6_fsbl_regress.c` REGRESS_MASTER_NPU
3. **单会话** — 同时只允许一个 LOAD 会话; 跨 fd 二次 LOAD 静默返回
4. IRQ handler 当前仅日志 (ATON 落地后需转发 epoch 完成信号)

## 参考

- AIE 框架: `nuttx/drivers/aie/ai_engine.c` + `include/nuttx/aie/ai_engine.h`
- 骨架模板: `nuttx/drivers/aie/ethosu/ethosu_lowerhalf.c` (Ethos-U lower-half)
- 芯片定义: ST HAL `stm32n647xx.h` (secure 域地址/IRQ/RCC)
- ATON 本地: `SoftwarePackage/Middlewares/AI/`
