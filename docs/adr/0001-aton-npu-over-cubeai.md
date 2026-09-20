# ADR-0001：推理走 ATON(NPU)，不采用 Cube.AI(CPU)

## 状态
已接受

## 背景
板子是 STM32N647：Cortex-M55 @800 MHz + Neural-ART NPU（600 GOPS）。
两条路都能跑眼动分类与人脸检测：

| 方案 | 优点 | 代价 |
|------|------|------|
| Cube.AI（CPU） | 工具链成熟、调试简单 | 实时预览 + 双模型推理时 CPU 不够；UI 帧率会被拖垮 |
| ATON（NPU） | 算力充足，CPU 留给 UI/摄像头 | 需要 ST Edge AI 工具链产出 `network.c`，且落板有额外步骤 |

## 决策
双模型（人脸 + 眼动）都跑在 **NPU/ATON** 上，CPU 只做取帧、裁剪和 UI。

## 后果
- ✅ 实时性达标：界面流畅、双模型推理不阻塞 UI。
- ✅ 模型产物自包含在仓库里（`libs/ai_aton/models/{eye,face}/`），可复现。
- ⚠️ 落板链路变长：ONNX → stedgeai → `network.c` + `network_atonbuf.xSPI2.raw`
  → 量化常量更新 → Intel HEX 权重镜像。这条链路沉淀成了自建 skill
  `.claude/skills/aton-model-deploy/`（本仓库），避免每次重踩。
- ⚠️ 需要改 `nuttx/libs/ai_aton` 与 `arch/.../stm32n6_aton_aie.c`，这些不在
  上游 trunk 里 —— 但都在本仓库中完整给出。
