# 引脚分配表（ATK-DNN647 / STM32N647）

只列**本项目实际用到**的引脚。每行都给出配置它的文件，方便核对。
引脚事实来自 `ATK-DNN647 IO引脚分配表.xlsx`（原厂）+ 驱动源码。

## 启动与存储

| 功能 | 接口 | 引脚 | 地址映射 | 配置位置 |
|------|------|------|----------|----------|
| 外部 NOR（启动介质） | XSPI2 | PORTN PN0–PN11，AF9 | `0x70000000` | `stm32n6_gpio.h`（`GPIO_XSPI2_*`） |
| HyperRAM 32 MB | XSPI1 | PORTP PP0–PP7 + PORTO PO0/PO2/PO4/PO5，AF9 | `0x90000000` | `stm32n6_gpio.h`（`GPIO_XSPI1_*`） |

> HyperRAM 不在原厂分配表的"已用"列里，是本次开发打通后使用的（见
> `docs/adr/0004-*.md` 与 `tools/board_change_history/`）。

## 显示与触摸

| 功能 | 引脚 | 说明 |
|------|------|------|
| LCD RGB565（20 路） | AF14，分布在 PORTB/H/A/F/G 等 | 800×480，PCLK 33.33 MHz（IC16 = PLL1/36） | 
| LCD 背光 | **PA3** | active-high，`GPIO_LTDC_BL`，输出低有效 |
| LCD 触摸 INT | **PB3**（`T_PEN`） | 原厂：连接至 LCD 接口 TP_PEN 并引出 |
| LCD 触摸 CS | **PD10**（`T_CS`） | 原厂：连接至 LCD 接口 TP_CS |

⚠️ **一个只有对照引脚表才会发现的约束**：`PD4` / `PD14` 在原厂表里是
*双功能*焊盘 —— 既是 `I2C2_SDA` / `I2C2_SCL`（摄像头控制总线），又是 LCD 触摸的
`T_MOSI` / `T_SCK`。也就是说**触摸与摄像头控制共用同一对焊盘，不能同时访问**。
本作品最终以眼控（摄像头）为主，触摸只注册了驱动、未参与交互流程。
（对照：`stm32n6_imx335.c` 用 `stm32n6_i2cbus_initialize(2)`；
`stm32n6_gt9xxx.c` 用软件位时序驱同一对焊盘。）

## 摄像头与音频

| 功能 | 接口 / 引脚 | 说明 |
|------|-------------|------|
| IMX335（MIPI CSI-2） | CSI_CKP/CKN、CSI_D0P/N、CSI_D1P/N（1.8 V 差分） | 数据走 DCMIPP，不进 GPIO 复用 |
| IMX335 复位 / 掉电 | **PG4**（`CSI_CAM_RST`）/ **PG6**（`CSI_CAM_PWDN`） | 原厂表：连接至 MIPI 摄像头接口 |
| IMX335 控制 | **I2C2**（PD4/PD14 焊盘） | 7 位地址 `0x1A`（驱动注释写 `0x34` 为 8 位形式） |
| ES8388 音频 | **PB0**=FS_A / **PB2**=SD_A / **PC2**=SCK_A / **PE2**=MCLK_A（AF6） | SAI1 主模式 44.1 kHz/16 bit；⚠️ 该链路**未打通**，见 `docs/audio-buzzer.md` |
| 蜂鸣器 | **PD3** | 经 R66(1K) 驱动三极管 Q7；**无源蜂鸣器**，方波即音调 |

## 板载外设（原厂，沿用）

| 功能 | 引脚 | 说明 |
|------|------|------|
| LED0（红） | PG10 | active-low，`/dev/userleds` |
| LED1（绿） | PE10 | active-low，`/dev/userleds` |
| KEY0 / KEY1 / KEY2 | PC6 / PD1 / PG11 | `/dev/buttons` |
| 串口控制台 | PE5=TX / PE6=RX | USART1，AF7，115200 OVER8 |
| 光感 AP3216C | I2C4：PE13=SCL / PE14=SDA | `/dev/i2c4`，地址 `0x1e`（板级自检用） |

## 与 DVP 摄像头的关系

原厂表里另有 `DCMI_*` 一组引脚（PB7/PB8/PB9/PD0/PD5/PD7/PE0/PE5/PE6/PE8/PH9…），
那是 **DVP 并口摄像头**的走线。本作品用的是 **MIPI CSI（IMX335）**，
所以这些 DCMI 引脚本项目不用（注意 PE5/PE6 与串口复用，别误配）。
