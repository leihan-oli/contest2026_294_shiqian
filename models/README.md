# models — AI 模型

眼动识别与人脸检测相关的数据、训练与模型产物。

## 目录

| 目录 | 内容 | 状态 |
|------|------|------|
| `dataset/` | 自采数据集 991 张（open 193 / close 208 / left 196 / right 186 / other 208） | ✅ |
| `training/` | 训练脚本、量化脚本、`blink_best.pt`、一键训练脚本 | ✅ |
| `exported/` | 导出的 ONNX（float + QDQ int8） | ✅ |
| `cubeai/` | ST Cube.AI（CPU）产物 —— **本项目未采用该路径**，见目录内说明 | 不适用 |

## 一句话链路

```text
自采（板端按脸框裁剪）→ 训练（MobileNetV2, 64×128）→ ONNX → QDQ int8
→ stedgeai → ATON network.c → 烧录到 NOR 0x70400000
```

板端推理路径与人脸检测链路见 `docs/ai-pipeline.md`；
数据集为什么按"脸框裁剪"采集（这条决定了 left/right 能不能识别）见
`docs/adr/0001-*.md` 与 `tools/dataset_audit/`。
