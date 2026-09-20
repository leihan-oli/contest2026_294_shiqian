# firmware/images — 可烧录镜像

四个镜像，四个地址。**烧错地址不会报错，只会静默不启动**，所以地址列在这里。

| 镜像 | 烧录地址 | 说明 | 字节 | md5（前 12 位） |
|------|----------|------|------|------------------|
| `fsbl_rifsecfix.hex` | `0x70000000` | 自定义 FSBL（LRUN + RIF/RISAF），由本仓库源码构建 | 85300 | `032d1ffd3a5c` |
| `nuttx.bin` | `0x70100000` | NuttX 应用固件（眼控 UI + 双模型） | 948064 | `a60d27014261` |
| `eye-data.hex` | `0x70400000` | 眼动五分类模型权重（Intel HEX） | 7975062 | `f8048978ef84` |
| `face-data.hex` | `0x70700000` | 人脸检测模型权重（Intel HEX） | 300114 | `7b9a44f59176` |

## FSBL 镜像可复现

`fsbl_rifsecfix.hex` 由本仓库 `firmware/fsbl/` 源码构建，一条命令即可复现：

```bash
bash firmware/scripts/build_fsbl.sh     # 输出 firmware/build/fsbl.hex（含 Intel HEX 生成与自检）
```

自检输出应包含 `byte_sum 校验: OK`、`0x240-0x400 全零: True`、`MSP=0x34200000`、
`Reset=0x34181E61`；重建结果与仓内镜像 **字节级一致**（md5 `032d1ffd3a5c`，2026-09-20 复核）。
本镜像包含 LTDC 帧缓冲所需的 **RISAF2/3 窗口**（真机验证 2026-08-12）。

> `history/fsbl_2026-08-08_rifsecfix.hex` 是 8/8 旧版，**不含** RISAF2/3 窗口
> （烧了屏只会显示顶部约 48 行），仅作追溯留档，**不要烧**。

## 什么时候烧哪个

| 场景 | 需要烧的 |
|------|----------|
| 首次上电 / 更换 FSBL | `fsbl_rifsecfix.hex` + `nuttx.bin` |
| 只改了固件（本次迭代绝大多数情况） | `nuttx.bin` |
| 换了眼动模型 | `nuttx.bin` + `eye-data.hex` |
| 换了人脸模型 | `nuttx.bin` + `face-data.hex` |

权重镜像在 `0x70200000` 之后是**连续排布**的（眼动 `0x70400000`、人脸 `0x70700000`），
两次烧录互不覆盖，可以分开更新。

## 原始（非 hex）权重镜像

给只认 bin 的工具（或不走 CubeProgrammer 的场合）：

| 文件 | 字节 | md5（前 12 位） |
|------|------|------------------|
| `eye_model_data.xSPI2.bin` | 2899745 | `fbc1f4c01614` |
| `face_model_data.xSPI2.bin` | 109105 | `548dec03f71d` |

`*.hex` 是同内容的 **Intel HEX** 文本形式（含地址记录，可直接用 CubeProgrammer 烧）。
生成脚本见 `tools/落板/build_hex.py`，它带自检：用同一脚本重建上一版镜像，
必须与历史镜像逐字节一致才算通过。

## 工具与校验

```bash
# 烧录前先核对是否是预期的镜像
md5sum firmware/images/nuttx.bin        # 应等于上表
# 烧录后确认板子起来了：串口应出现
#   xspi1: HyperRAM armed (...)
#   eye_ui: panel up, 3x2 cells, select 0
```

工具：STM32CubeProgrammer + External Loader `MX25UM25645G_ATK-CNN647B`
（NOR 在 XSPI2 上，映射 `0x70000000`）。

## history/

`history/` 存放被取代的历史镜像（如最早的 `nuttx_errint_fix.bin`），保留是为了
对照排障，不是提交产物的一部分。
