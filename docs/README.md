# 项目文档

| 文档 | 说明 |
|------|------|
| [reproduce.md](reproduce.md) | **从零复现（本仓库唯一入口：入位 → 编译 → 烧录 → 验收）** |
| [architecture.md](architecture.md) | 系统架构（三层 + 眼控数据流） |
| [openvela-porting.md](openvela-porting.md) | STM32N647 openvela 移植记录（启动链/已解决坑/调试） |
| [ROADMAP.md](ROADMAP.md) | 路线图与里程碑（Phase 0-11 状态） |
| [lvgl-ui.md](lvgl-ui.md) | 眼控界面设计（页面/动作映射/中文字库/渲染与内存） |
| [ai-pipeline.md](ai-pipeline.md) | 端侧 AI 链路（数据集 → 训练 → 量化 → NPU 落板） |
| [audio-buzzer.md](audio-buzzer.md) | 声音反馈（ES8388 排查结论 + 蜂鸣器音调实现） |
| [eye-control-state-machine.md](eye-control-state-machine.md) | 眼控状态机定义 |
| [ai-model.md](ai-model.md) | AI 模型规划（早期双轨方案） |
| [cam-liveview-guide.md](cam-liveview-guide.md) | 摄像头实时预览使用指南 |
| [demo-guide.md](demo-guide.md) | 演示指南 |
| [build-environment.md](build-environment.md) | 构建环境说明（自包含，从零复现 FSBL） |
| [model/](model/) | 模型训练与量化的回报文档（训练任务书、验收结论） |
| [adr/](adr/) | 架构决策记录（ADR） |

## 目录相关

- FSBL 改造说明：`../firmware/fsbl/README.md`
- 构建脚本说明：`../firmware/scripts/README.md`
- 芯片层源码：`../arch/arm/stm32n6/README.md`（含 ATON 驱动说明）
- 板级层源码：`../board/atk-dnn647/`
- 应用（眼控主应用）：`../app/examples/eye_cam/`
- AI 运行时与模型：`../libs/ai_aton/`
- 模型训练链与数据集：`../models/`
- AI Coding 日志：`../logs/README.md`
