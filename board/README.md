# board — 板级层（ATK-DNN647）

openvela（NuttX）板级层**完整源码**，对应 `vendor/openvela/boards/atk-dnn647/`）。

## 内容（已填充）

| 路径 | 说明 |
|------|------|
| [atk-dnn647/](atk-dnn647/README.md) | 板级层完整源码（configs/include/scripts/src）+ 改动说明 |

## 板卡关键资源

- LED0=PG10（红）、LED1=PE10（绿），active-low
- KEY0=PC6 / KEY1=PD1 / KEY2=PG11 / WKUP=PC13
- USART1=PE5(TX)/PE6(RX) AF7 115200 OVER8
- 时钟：600MHz CPU / 400MHz AXI / 200MHz HCLK（匹配 FSBL）

> 详细引脚见 `hardware/pinmap.md`。
