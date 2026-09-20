# board/atk-dnn647 — 板级层源码（ATK-DNN647）

正点原子 ATK-DNN647 板级层**完整源码**（当前固件实际编译使用的版本）。

> 来源：openvela 工作区 `vendor/openvela/boards/atk-dnn647/`（完整内嵌本仓库）。
> 板级层源码源自 NuttX STM32N6 初始 port 基座（Apache-2.0，源码头文件保留版权声明），
> 已应用本项目全部增量（600MHz 时钟、链接脚本、LED/按键驱动），
> 与已烧录固件 `nuttx_errint_fix.bin` 对应（NSH `/dev` 下可见 `buttons`/`userleds`/`ttyS0`）。

## 源码树

| 路径 | 说明 |
|------|------|
| `configs/nsh/defconfig` | 板级配置（nsh / nsh-qemu / nsh-test / edgesight） |
| `include/board.h` | 引脚/时钟定义（600MHz CPU/400MHz AXI/200MHz HCLK） |
| `include/board_pinmap.h` | 引脚复用表 |
| `scripts/flash.ld` | 链接脚本（DISCARD `.note.gnu.build-id`，Flash boot 关键） |
| `src/stm32n6_clockconfig.c` | 时钟配置 |
| `src/stm32n6_userleds.c` | **LED 驱动**（LED0=PG10 红、LED1=PE10 绿，active-low） |
| `src/stm32n6_userbuttons.c` | **按键驱动**（KEY0=PC6/KEY1=PD1/KEY2=PG11/WKUP=PC13，EXTI） |
| `src/stm32n6_reset.c` | 复位 |
| `src/board_bringup.c` | 板级设备注册 |
| `src/CMakeLists.txt` / `Makefile` | 编译集成 |

## 本项目关键改动

1. **600MHz 时钟**（`board.h`）：匹配 FSBL（PLL1 M4/N75 → 600MHz CPU/400MHz SYSCLK/200MHz HCLK），修复 SysTick 差 9 倍导致的 OS 崩溃
2. **链接脚本**（`flash.ld`）：`/DISCARD/ { *(.note.gnu.build-id) }` → 向量表回到 `0x34000400`（LRUN 固定拷贝地址），Flash boot 跑通关键
3. **LED 驱动** `stm32n6_userleds.c`：userleds upper-half
4. **按键驱动** `stm32n6_userbuttons.c`：input/buttons（EXTI 中断）

## 验证用法

```bash
# /dev/userleds 写入要求 ≥4 字节（userled_set_t=uint32_t）
echo -ne '\x01\x01\x01\x01' > /dev/userleds   # LED0 亮
echo -ne '\x02\x02\x02\x02' > /dev/userleds   # LED1 亮
echo -ne '\x04\x04\x04\x04' > /dev/userleds   # 全灭（首字节必须非零）
```
