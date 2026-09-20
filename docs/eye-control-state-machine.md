# 眼控状态机

> 动作事件由视觉/AI 层产生（看左/看右/看上/看下/眨眼/闭眼），状态机将其映射为 UI 与控制指令。

## 1. 动作事件定义

| 事件 | 触发条件 | 映射动作 |
|------|---------|---------|
| LOOK_LEFT | 视线左移 | 光标左移 / 上一页 |
| LOOK_RIGHT | 视线右移 | 光标右移 / 下一页 |
| LOOK_UP | 视线上移 | 光标上移 |
| LOOK_DOWN | 视线下移 | 光标下移 |
| BLINK | 快速眨眼（<300ms） | 确认/点击 |
| CLOSE_HOLD | 闭眼保持（>800ms） | 长按/返回 |
| NO_FACE | 无人脸 | 待机/锁屏 |

## 2. 状态定义

```text
IDLE（待机）
 ├─ 检测到人脸 → TRACKING（跟踪）
 ├─ 无人脸持续 Ns → SLEEP（休眠，降低功耗）
TRACKING（跟踪，常态）
 ├─ 视线进入方向区间 → DIRECTION（方向指示）
 ├─ 眨眼 → CONFIRM（确认，闪烁反馈）
 └─ 闭眼保持 → HOLD（长按）
CONFIRM（瞬态，反馈后回 TRACKING）
HOLD（长按，反馈后回 TRACKING）
SLEEP（休眠）
 └─ 唤醒事件 → IDLE
```

## 3. 状态迁移表

| 当前状态 | 事件 | 下一状态 | 动作 |
|---------|------|---------|------|
| IDLE | FACE_DETECTED | TRACKING | 点亮 UI |
| IDLE | NO_FACE 持续 Ns | SLEEP | 关屏/低功耗 |
| TRACKING | 方向事件 | DIRECTION | 移动光标 |
| TRACKING | BLINK | CONFIRM | 触发点击 |
| TRACKING | CLOSE_HOLD | HOLD | 长按动作 |
| TRACKING | NO_FACE 短暂 | TRACKING | 保持 |
| TRACKING | NO_FACE 持续 | SLEEP | 休眠 |
| DIRECTION | 无方向 | TRACKING | 停止移动 |
| CONFIRM | 反馈完成 | TRACKING | — |
| HOLD | 睁眼 | TRACKING | — |
| SLEEP | 唤醒（人脸/按键） | IDLE | — |

## 4. 防误触策略

- 方向动作需要 **连续 N 帧一致** 才生效（防抖动）
- 眨眼需满足时长窗口（100ms < t < 300ms），过快/过慢忽略
- 提供灵敏度参数（可调）
