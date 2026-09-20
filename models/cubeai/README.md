# models/cubeai — 本目录为何是空的

模板为"CPU 侧 ST Cube.AI 转换产物"预留了本目录，**本项目没有走这条路**，
最终全部推理都在 **NPU（ATON）** 上完成：

| 方案 | 状态 | 说明 |
|------|------|------|
| Cube.AI（CPU，Cortex-M55） | ❌ 未采用 | 需要在 PC 上装 X-CUBE-AI，且 CPU 推理时延与占空比都吃不住实时预览 |
| **ATON（NPU，Neural-ART 600 GOPS）** | ✅ **采用** | 见 `libs/ai_aton/models/{eye,face}/`，板端 `nuttx/libs/ai_aton/` |

原始方案里"CPU 保底"的判断也留在这里，避免读者以为漏交了：真到 NPU 跑不起来时，
退路是把同一个 ONNX（`models/exported/blink_v3.onnx`）用 Cube.AI 转成 CPU 代码 ——
模型本身只有 MobileNetV2 量级，CPU 上单帧勉强可用，但**帧率会显著下降**，
所以最终没有选它。

两个模型的板端产物位置：

| 模型 | 板端源码 | 权重镜像 |
|------|----------|----------|
| 眼动五分类 | `libs/ai_aton/models/eye/` | `firmware/images/eye-data.hex` |
| 人脸检测（YuNet） | `libs/ai_aton/models/face/` | `firmware/images/face-data.hex` |
