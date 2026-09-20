# 架构决策记录（ADR）

每个 ADR 记录一个"当时有别的选择、最后为什么这么选"的决定。
这些决定大多来自排障过程，写下来是为了让读者不必重走一遍。

| ADR | 标题 | 状态 |
|-----|------|------|
| [0001](0001-aton-npu-over-cubeai.md) | 推理走 ATON(NPU)，不采用 Cube.AI(CPU) | 已接受 |
| [0002](0002-face-crop-as-training-input.md) | **训练输入改用"按脸框裁剪的眼图"** | 已接受（修正了原方案） |
| [0003](0003-single-framebuffer-handoff.md) | LVGL 与摄像头共用一个 framebuffer，靠"模式互斥"而非双缓冲 | 已接受 |
| [0004](0004-hyperram-for-nuttx.md) | 把 HyperRAM（0x90000000）打通并用于大缓冲 | 已接受 |
| [0005](0005-buzzer-tone-instead-of-audio.md) | 声音反馈方案：蜂鸣器四音调 | 已接受 |

## 模板

```markdown
# ADR-000：标题

## 状态
提议 / 已接受 / 已废弃

## 背景
为什么需要这个决策

## 决策
我们选择……

## 后果
正面 / 负面影响
```
