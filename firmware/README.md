# firmware — 固件工程（原创）

本目录收录本项目**原创实现**的固件部分：自定义 FSBL、构建/验证脚本、已烧录固件镜像。

## 目录

| 目录/文件 | 说明 |
|-----------|------|
| [fsbl/](fsbl/README.md) | 自定义 FSBL 源码（LRUN + RIF/RISAF 授权） |
| [scripts/](scripts/) | Linux 侧构建与 hex 验证脚本 |
| [images/](images/) | 已烧录固件（勿覆盖，可直接用于复现） |

## 已完成成果（Phase 4，2026-08-08）

- **自定义 FSBL**：摆脱 ST 预编译 FSBL 限制，支持 LRUN（Load & Run）+ RIF/RISAF 授权
- **Flash boot 彻底跑通**：外部 NOR 独立启动 openvela（NSH 可交互 + LED 可控）
- **固件落 Flash**：FSBL @ `0x70000000`、NuttX @ `0x70100000`

## 已烧录固件

| 固件 | 地址 | 文件 | 说明 |
|------|------|------|------|
| FSBL | `0x70000000` | `images/fsbl_rifsecfix.hex` | 自定义 FSBL（LRUN + RIF） |
| NuttX | `0x70100000` | `images/nuttx_errint_fix.bin` | openvela 固件（600MHz 时钟 + 串口增强） |

> 烧录工具：STM32CubeProgrammer + External Loader `MX25UM25645G_ATK-CNN647B`
> 串口调试：**PuTTY 115200**（勿用控制 DTR/RTS 的软件，会触发一键下载复位电路）
