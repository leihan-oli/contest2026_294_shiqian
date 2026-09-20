# models/exported — 导出模型

训练侧导出的中间模型，是"训练 → 板端"链路的交接点。

| 文件 | 说明 | 大小 |
|------|------|------|
| `blink_v3.onnx` | float32 ONNX（opset 17），输入 `[1,3,64,128]`，输出 `[1,5]` | 见 `ls` |
| `blink_v3_qdq_int8.onnx` | QDQ 量化版（238 个 Q/DQ 节点），calib 345 张 | 见 `ls` |

## 它在整条链路里的位置

```text
models/dataset/  ──train_blink.py──▶  models/training/blink_best.pt
                                              │ export_onnx.py
                                              ▼
                                       models/exported/blink_v3.onnx
                                              │ quantize_onnx.py（QDQ，calib 345）
                                              ▼
                                       models/exported/blink_v3_qdq_int8.onnx
                                              │ stedgeai generate（ST 工具链）
                                              ▼
                       libs/ai_aton/models/eye/{network.c, network_ecblobs.h,
                                               network_atonbuf.xSPI2.raw}
                                              │ tools/落板/*.py
                                              ▼
                             firmware/images/eye-data.hex（可烧录 Intel HEX）
```

> 板端**实际使用**的是 `libs/ai_aton/models/eye/` 下的三个文件，不是本目录的 ONNX。
> 量化常量（input/output scale、Q_GAIN/Q_BIAS）随之更新，见 `docs/ai-pipeline.md`。
