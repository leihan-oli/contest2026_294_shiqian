---
name: aton-model-deploy
description: >-
  把 ST Edge AI（stedgeai）产出的模型落到 openvela(NuttX) + ATON(NPU) 板端的完整流程：
  板端契约核对 → network.c 两处钩子（.nsblob 段 / ecblobs 注册）→ 量化常量
  （input/output scale+zp 与**派生**的 Q_GAIN/Q_BIAS）→ Intel HEX 权重镜像（逐字节自检）
  → 边界自检（`_ebss ≤ 0x34200000`、固件 ≤ 1 MiB）→ 板端跑通第一个 epoch program。
  适用于：换模型、重训后重新落板、把 ATON 移植到新板子。
  Use when: 模型落板, 换模型, stedgeai generate, ATON, NPU 模型部署, neural-art,
  network.c, network_ecblobs.h, network_atonbuf.xSPI2.raw, nsblob, epoch program,
  量化常量, quant scale, zero_point, Q_GAIN, Q_BIAS, 权重镜像, Intel HEX, 0x70400000,
  model deployment, quantized model bring-up, ATON model, NPU bring-up.
---

# ATON 模型落板

把一个 stedgeai 生成的模型从"PC 上的三个文件"变成"板子上跑得起来的推理"。
这条链路有 6 个可以静默出错的地方（下面的"坑"表里都是实测踩过的），
所以每一步都配了**验证动作**，不要跳。

## 前提

| 项 | 要求 |
|---|---|
| 板端 | openvela + `nuttx/libs/ai_aton/`（ATON 运行时 + 至少一个已跑通的模型） |
| PC | ST Edge AI（stedgeai）产出 `network.c` / `network_ecblobs.h` / `network_atonbuf.xSPI2.raw` |
| 已知 | 训练侧量化参数（input/output 的 scale、zero_point）—— 来自量化脚本的输出 |
| 心态 | 读源码只信**终端里的磁盘内容**；编辑器/工具缓存曾多次给出旧版本 |

## 步骤

### 1. 先核对板端契约，再动文件

确认这些数字与即将换上的模型**一致**（不一致会得到"模型换了但结果不变"或直接拒跑）：

- 应用里对张量尺寸的守卫：`input_size` / `output_size`（例如眼控应用会拒绝
  尺寸不符的模型，直接打印 `refusing to run: tensor sizes are ...`）
- 输入预处理：归一化方式、通道顺序、是否 `-128` 偏移
- 输出解码：类别数、`(q - zp) × scale`

> **验证**：把新旧两组数字写在一张纸上，逐项对。这一步省不得 —— 本项目
> "换了模型但输出一模一样"就是张量尺寸守卫先报的警。

### 2. 三个文件进模型目录

```bash
cp network.c network_ecblobs.h network_atonbuf.xSPI2.raw \
   nuttx/libs/ai_aton/models/<model>/
```

先备份旧的（`cp -r models/<model> /tmp/<model>.bak`），出问题时能秒回退。

### 3. 给 `network.c` 打两处钩子

`tools/落板/patch_nsblob.py` 做的两件事，都很关键：

1. **`ECBLOB_RUNTIME_SECTION` 宏** —— 把 epoch program 放进 `.nsblob`
   （非安全 blob 段）。NPU 在这颗芯片上是**非安全主设备**，放错段会导致
   推理直接不工作。
2. **`aton_model_ecblobs` 注册钩子** —— 追加在文件末尾，让运行时能找到 blob 表。

> **验证**：`grep -c ECBLOB_RUNTIME_SECTION network.c` 应为 1，
> 且文档里那句"两处改动，一字不多"能对上（`diff` 行数 = 常量行 + 2）。

### 4. 更新量化常量（6 个，其中 2 个是**派生**的）

应用侧常量（以眼控应用为例）：

| 常量 | 来源 |
|---|---|
| `EYE_CAM_IN_SCALE` / `EYE_CAM_IN_ZP` | 量化脚本输出的 input scale/zp |
| `EYE_CAM_OUT_SCALE` / `EYE_CAM_OUT_ZP` | 量化脚本输出的 output scale/zp |
| `EYE_CAM_Q_GAIN` / `EYE_CAM_Q_BIAS` | **由 IN_SCALE/IN_ZP 推导，不要手抄** |

推导公式（本项目输入预处理为 `v/255 → (x-0.5)/0.5` 的 affine 折叠）：

```text
gain = 1 / (127.5 × in_scale)
bias = in_zp - 1 / in_scale
```

脚本 `tools/落板/patch_qconsts.py` 会把新值写进去，并且**反推旧值做自检**：
用公式重算旧模型的常量，必须与仓库里的旧值逐位吻合 —— 公式错了这一步就会拦下。

> **验证**：脚本末尾的 `must_have` / `must_not` 断言（新值必须在、旧值必须消失）。
> 注意常量以 `%f`（6 位小数）写进代码，用字节搜索校验时要按 6 位小数的 float32 去找。

### 5. 生成权重镜像（Intel HEX）+ 逐字节自检

```bash
python3 tools/落板/build_hex.py
```

它输出 `*.hex` 与配套 `*.raw`。**本项目用的是 Intel HEX，不是 SREC**：
长度字段只计数据字节，校验和为二补数（整条记录 `sum & 0xFF == 0`）。

> **验证**：脚本用同一份 `.raw` 重建**上一版**镜像，必须与历史镜像逐字节一致；
> 不一致就先查生成器，别急着烧。
> 另可用离线测试复核装载地址：`python3 tests/check_artifacts.py`。

### 6. 编译 + 两道边界自检

```bash
rm -rf cmake_out/atk-dnn647_eye          # 动过 defconfig 就必须清
./build.sh vendor/openvela/boards/atk-dnn647/configs/eye --cmake -j$(nproc)
bash scripts/check_boundaries.sh
```

| 约束 | 上限 | 越界后果 |
|---|---|---|
| `_ebss` | `0x34200000` | 上面是 NPU/AXISRAM 域，**静默不启动**（没有 panic、没有打印） |
| 固件体积 | 1 MiB | 超过 FSBL 的 `EXTMEM_LRUN_SOURCE_SIZE`，同样静默 |

### 7. 烧录

| 镜像 | 地址 |
|---|---|
| `nuttx.bin` | `0x70100000` |
| 眼动模型权重 | `0x70400000` |
| 人脸模型权重 | `0x70700000` |

**固件和权重是两份镜像**：换模型时两份都要刷；只刷固件会得到"新代码 + 旧权重"。

### 8. 板端验证（顺序不要换）

1. 权重指纹/PROD_ID 读回 —— 先证明"读到的权重就是刚烧的那份"
2. 一次推理，确认输出是**新模型的量化参数**下的结果
3. 再谈实时性与 UI 表现

> **坑**：固件里可能有一张**权重指纹期望表**（本项目在 `stm32n6_aton_aie.c`，用于
> 启动自检）。换权重后这张表必须同步更新，否则会打印
> "权重不匹配" —— 那是**校验表过期**，不是权重坏了。先用 `od`/`xxd` 从 `.raw`
> 里读出真实指纹再改表，别反过来假设固件对。

## 坑（全部实测）

| 现象 | 根因 | 做法 |
|---|---|---|
| 换了模型，输出和旧模型一模一样 | 权重没进去（只刷了固件）或张量守卫拦下 | 先看板端有没有 `refusing to run: tensor sizes are …`；两份镜像都要刷 |
| 启动自检报"权重指纹不匹配" | 固件里的期望表过期 | 从 `.raw` 读出真实前 6 个 word 再更新表 |
| 推理结果整体偏移/饱和 | `Q_GAIN`/`Q_BIAS` 与 `IN_SCALE` 不同源 | 用公式重算，别手抄；用旧值反推自检 |
| 搜不到刚写进代码的常量 | `%f` 只保留 6 位小数 | 按 6 位小数格式化后再搜字节 |
| 抓到的寄存器/代码是旧版 | 编辑器或工具缓存 | 一律用终端读盘（`grep`/`sed`/`md5sum`）确认 |
| hex 被解析器第一行就拒 | 按 SREC 读 Intel HEX（或反之） | 长度字段/校验和语义不同，见第 5 步 |
| 改完 defconfig 行为没变 | `build.sh` 复用了旧 `.config` | `rm -rf cmake_out/<target>` 后全量重建 |
| 越界后板子毫无反应 | 边界是"静默失败" | 每次编译后跑 `scripts/check_boundaries.sh` |

## 验收清单

- [ ] 应用侧张量尺寸守卫与实际模型一致
- [ ] `network.c` 两处钩子都在，且 diff 只有预期行数
- [ ] 6 个量化常量更新，派生值由公式重算并自检
- [ ] Intel HEX 镜像生成器自检通过（与上一版逐字节对拍）
- [ ] `_ebss ≤ 0x34200000` 且固件 ≤ 1 MiB
- [ ] 固件 + 权重**两份**都烧了
- [ ] 板端读到的是新权重（指纹/尺度打印）
- [ ] 一次推理的输出与 PC 端同输入下的结果对齐

## 本仓库落点

| 用途 | 文件 |
|---|---|
| 落板三件套 | `tools/落板/{patch_nsblob,patch_qconsts,build_hex}.py` |
| 完整链路说明 | `docs/ai-pipeline.md` |
| 决策背景（为什么用 NPU） | `docs/adr/0001-aton-npu-over-cubeai.md` |
| 模型产物 | `libs/ai_aton/models/{eye,face}/` |
| 可烧录镜像 | `firmware/images/`（地址表见其 README） |
| 离线复核 | `tests/check_artifacts.py` |
