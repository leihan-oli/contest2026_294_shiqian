# 端侧 AI 链路（数据集 → 训练 → 量化 → NPU 落板）

板端同时跑**两个模型**，一帧内串行执行：

```text
IMX335 取帧(800×480) → 人脸检测 YuNet 256×416 → 取关键点算眼心/瞳距
                     → 按脸框裁剪眼部 → 眼动五分类 64×128 → 判决 → 界面动作
```

## 一、数据集（`models/dataset/`，991 张）

| 类别 | 数量 | 说明 |
|---|---|---|
| open | 193 | 睁眼正视 |
| closed | 208 | 闭眼 |
| left | 196 | 看左 |
| right | 186 | 看右 |
| other | 208 | **无人脸/纯场景**（固定框拍摄，让模型有明确的"其它"出口） |

采集要点（决定了模型可用性）：

1. **按脸框裁剪后再存**：实际推理输入就是"按脸框裁剪的眼部"，训练数据必须同分布。
   早期版本用固定 ROI 采、推理时用脸框裁 —— **这是 left/right 几乎不可用的根因**
2. **裁剪几何**：宽 `2.0 × 瞳距`、高 `0.75 × (145/300 × 宽)`（下界上推 1/4 去掉鼻孔），
   眼心落在框内 **y ≈ 0.413** 处
3. **每张记录 meta**（`meta.txt`）：类别、序号、裁剪框、眼心、瞳距、人脸分数 ——
   用于事后审计（几何离群 / 模糊 / 曝光异常）
4. other 类必须存在：否则"没人脸"时模型只能瞎猜

数据审计工具：`tools/UI字体/` 与本仓库 `models/dataset/meta.txt` 配合可复现审计结论
（几何一致性、遮挡实验、头偏检测）。

## 二、训练（`models/training/`）

| 项 | 值 |
|---|---|
| 网络 | MobileNetV2（ImageNet 预训练 + 5 类头） |
| 输入 | `[1,3,64,128]` float32，归一化到 `[-1,1]` |
| 类别顺序 | closed=0, open=1, left=2, right=3, other=4（**与板端一致**） |
| 数据划分 | `--val-split 0.2 --seed 42` |
| 产物 | `blink_best.pt` → `blink_v3.onnx`（opset 17） |

```bash
python3 models/training/train_blink.py --flat models/dataset --task blink --size 64x128
python3 models/training/export_onnx.py
```

## 三、量化（`quantize_onnx.py`）

| 配置 | acc | left 召回 |
|---|---|---|
| calib 200（旧） | 0.9675 | 70/80 = 0.875 |
| **calib 345 + QOperator（采用）** | **0.9884** | **15/16** |

- 校准集要覆盖**全部 5 类**（`--calib-count 345`），否则 left/right 掉点
- 采用 **QOperator** 格式（QDQ 亦可，但实测 QOperator 零掉点）
- 端点：`blink_v3_qdq_int8.onnx`；模型侧输入 scale/zp 与板端代码中的常量逐个比对过

## 四、NPU 落板（ATON / ST Edge AI）

1. `stedgeai generate` 产出 `network.c` + `network_ecblobs.h` +
   `network_atonbuf.xSPI2.raw`
2. 放入 `nuttx/libs/ai_aton/models/eye/`
3. **两处移植**（`tools/落板/patch_nsblob.py`）：
   - `ECBLOB_RUNTIME_SECTION` 宏：epoch program 放到**非安全段** `.nsblob`
     （NPU 是非安全主设备，放安全段会取不到）
   - `aton_model_ecblob` 钩子：文件末尾挂上 runtime section 指针
4. **量化常量同步**（`tools/落板/patch_qconsts.py`）：6 个常量
   （输入 scale/zp、输出 scale/zp、`Q_GAIN`/`Q_BIAS`）；
   后两个是派生量 `Q_GAIN = 1/(in_scale × 2^7)`、
   `Q_BIAS = -128 × Q_GAIN - zp_in × Q_GAIN`，脚本会反推校验
5. **权重镜像**（`tools/落板/build_hex.py`）：把 `.raw` 生成可烧录的 **Intel HEX**。
   ⚠️ 本工程的 hex 是 Intel HEX，**不是 SREC** —— 两者极容易混：Intel HEX 的 `LL` 只计**数据字节**、校验和为**二补数**
   （整条记录 `sum & 0xFF == 0`）；SREC 的 `LL` 含地址与类型字节、校验和为**一补数**。
   写解析器时按错的那种读，第一行就会被拒。
   生成器内置与上一版逐字节对拍的自检
6. 烧到 `0x70400000`（眼）/ `0x70700000`（人脸）

## 五、人脸检测（YuNet 256×416）

- 12 个输出张量（3 尺度 × cls/obj/kps），取**跨尺度分数最大**的 anchor：
  `score = sqrt(cls × obj)`（参考实现不做 NMS）
- 关键点顺序 `[右眼, 左眼, 鼻, 右嘴角, 左嘴角]` → 眼心 = 两点中点，瞳距 = 两点距离
- 板端实测检出率 **115/115**，眼心位置与裁剪框吻合（框能完整框住双眼）

## 六、精度与运行

| 指标 | 值 |
|---|---|
| float 精度 | 991/991 |
| int8 精度 | 990/991 |
| 板端 | 双模型串行，界面流畅（含 800×480 预览） |

## 七、踩过的坑（节省后来者时间）

- **训练/推理必须同链路**（本条价值最高）
- int8 与 float 的差异会集中在"难样本"（瞳距最小的一档），需单独看
- ATON 张量池：人脸模型的输出跨度**包含**眼动模型的输出偏移，顺序不能颠倒
- 权重表校验指纹：换模型后必须同步更新驱动里的期望值，否则启动即报错
- 探针读 NOR 前必须 `invalidate D-Cache`，否则读到的是陈旧内容（曾误判"权重损坏"）
