# Changelog

## [1.0.0] - 2026-09-20

### 音频尝试与音调反馈 + 大赛日志归档
- **ES8388 + SAI1 播放链路（未打通，已归档结论）**：HSE/PLL2/IC7 时钟链、`MCKDIV`、
  `CCIPR7.SAI1SEL`、内核时钟源全部按 RM0486 与厂商 BSP 核对正确，MCLK 实测正常，
  但 **SAI FIFO 始终不排空**（`FLVL=5/8` 恒定），DMA 与 CPU 直灌均超时
- **本板 SAI 走通的关键事实**（留给后续）：ES8388 需要 MCLK 才应答 I2C；SAI 必须先配置
  `CR1.MCKEN` 再初始化 codec
- **蜂鸣器音调反馈**（PD3，无源蜂鸣器）：新增 `stm32n6_buzzer.c`
  - `tone(hz,ms)` 阻塞式 + `tone_async(hz)`/`tone_stop()` 基于 **TIM3 更新中断**翻转 PD3
  - 眼控策略：二级页面按住动作 → 对应音调**连续响**；菜单移动/进入 → 动作完成后
    **一声短音**；闭眼 784 Hz(G5) / 左 587 Hz(D5) / 右 988 Hz(B5) / 其它 660 Hz(E5)
- **AI Coding 日志归档**：采集插件不支持 Copilot，改用 VS Code 自有记录转换出 14 个会话
  （38,296 事件，含工具输出/tokens/模型名），官方 `validate-log.py` 校验 **ALL OK**

## [0.9.0] - 2026-09-19

### Phase 8：眼控应用完整化
- **LVGL 眼控界面**：3×2 方形按钮网格主页 + 6 个二级页（眼动展示 / 拍照 / 系统信息 /
  LED 控制 / 串口命令 / 关于），中文界面
  - 眼动判决状态机：**持续注视 0.6~1.0 s 触发**（防抖）、闭眼进入、左看返回、右看移动
  - 触发提示条 + 进度反馈；返回值即"正在做某事"的视觉确认
- **拍照功能**：闭眼进入 → 3 s 倒计时（大号数字叠在画面中央）→ 全帧 800×480 存 BMP 到
  `/mnt/sdcard`，产物**不含标注**（保存时刻早于 overlay），支持闭眼 2 s / 右看 2 s 重拍
- **系统信息页**：内存（`mallinfo`）、帧率、LVGL 堆占用、各版本与时间
- **LED 控制**：`/dev/userleds`（LED0=PG10、LED1=PE10，低有效）
- **串口命令页**：把常用 `eye_cam` 子命令做成可点选条目
- 字体：**4 档 TTF 生成的中文字库**（24/32/48/64 px），按实际文案裁剪，比全字库省 58 KB

## [0.8.0] - 2026-09-19

### Phase 7：NPU 双模型（人脸 + 眼动五分类）
- **ATON 运行时移植**（`nuttx/libs/ai_aton` + `stm32n6_aton_aie.c`）
  - 驱动侧：权重指纹校验、RISAF 区域授权（NPU 是非安全主设备）、SAU/DCache 一致性处理、
    epoch program 放非安全段（`.nsblob`）
  - 踩坑：CFGCON 打印地址写错（`0xe000e00c` 是 AIRCR）、探针读 NOR 前必须 invalidate D-Cache
- **人脸检测（YuNet 256×416）**：12 个输出张量解码（三尺度 cls/obj/kps），取分数最高 anchor，
  输出眼心坐标与瞳距，实测框选准确（`kps` 链路端到端验证通过）
- **眼动五分类（64×128，closed/open/left/right/other）**
  - **数据集 v3（991 张）**：按**脸框裁剪**采集，训练输入与推理输入同分布 —— 这是
    left/right 从"几乎不可用"（0/196、0/186）到全类可用的**根因修复**
  - 量化：`calib345` + QOperator，float 991/991、int8 990/991
  - 落板：`stedgeai` 产物 `network.c` 移植（`.nsblob` + 钩子）+ 量化常量替换
    （`Q_GAIN`/`Q_BIAS` 为派生量，公式反推校验）
  - 权重镜像重新生成：自写 Intel HEX 生成器（校验和为**二补数**，与 v21 逐字节对拍）

## [0.7.0] - 2026-08-22

### Phase 5/6：显示、摄像头与外设
- **LTDC 800×480 RGB565**（`stm32n6_ltdc.c`）：PLL1/IC16 = 33.33 MHz、RIF 授权、20 路 AF14
  GPIO、背光 PA3；`/dev/fb0` 注册
- **HyperRAM(32 MB) 打通**（`stm32n6_xspi.c`）
  - 根因：BootROM 只做了只读映射，**写通路**缺 `CCR/WCCR` 事务模板；且 `DCR2` 回绕突发
    会让 LVGL 访问超范围地址。补 `FMODE` 退出→事务寄存器写入→回 memory-mapped 全序列
  - 副产品：实验出可在板端任意读写寄存器的 `eye_cam xspi` 实验台
- **DCMIPP + IMX335 摄像头**（`stm32n6_dcmipp.c` / `stm32n6_imx335.c` / `stm32n6_video.c`）
  - V4L2 框架 + `/dev/video0`；IMX335 双 transfer I2C 序列
  - **冷/热启动分离**：`video_start()` 不再每帧重新上电，单帧开销 2.7 s → 60 ms（45×）
  - **D-Cache/I-Cache**：SDMMC 初始化后恢复 D-Cache、全树首次使能 I-Cache
- **GT911 触摸**（`stm32n6_gt9xxx.c` + input 驱动）
- **拍照能力**：复用采集管线，全帧 BMP 写 `/mnt/sdcard`

## [0.6.0] - 2026-08-08

### Phase 5：LTDC RGB-LCD 驱动实现（7" 800x480）
- **硬件分析**（15_RGBLCD 例程）：ATK-MD0700R-800480（ID 0x7084）、RGB565 16-bit 并行、PCLK 33.33MHz（PLL1/IC16=36）、背光 PA3
- **新建** `hardware/stm32_ltdc.h`：LTDC 寄存器偏移（STM32N6 布局 GCR@0x18、Layer@0x100）与位定义
- **实现** `stm32n6_ltdc.c` 硬件初始化：
  - `ltdc_clock_config`：APB5 LTDCEN + IC16（PLL1/36=33.33MHz）
  - `ltdc_rif_config`：LTDC1 RIMC master 授权（SEC|PRIV，CID0）
  - `ltdc_gpio_config`：20 路 RGB565 接口（AF14）+ 背光 PA3
  - `ltdc_hw_init`：时序/层/CFBAR 寄存器编程 + 使能
- **GPIO**：`stm32n6_gpio.h` 添加 21 个 LTDC 引脚宏（AF14）
- **RCC**：`stm32_rcc.h` 添加 APB5ENSR/AHB3ENSR/IC16CFGR 定义
- **板级**：`board_bringup.c` 注册 `/dev/fb0`（帧缓冲 800x480x2 放内部 SRAM）
- **配置**：defconfig 使能 `CONFIG_DRIVERS_VIDEO` + `CONFIG_VIDEO_FB`
- ✅ 编译通过（sram 1.14MB / 27%，含 768KB 帧缓冲），ltdc 符号与日志字符串已编入固件
- ⏳ 待真机验证：烧录后检查背光/黑屏/`/dev/fb0`

## [0.5.1] - 2026-08-08

### 清除历史参考仓库痕迹（全库无残留）
- board 目录统一更名 `atk-dnn647`（openvela 侧同步，构建命令更新）
- 清理全部源码注释/`include guard`/`Kconfig TAG`/`defconfig` 中的历史目录名引用
- 清理全部文档中的历史参考表述
- 删除 `nsh-test` 配置中无关的历史 demo 配置项
- 重新编译固件（无历史路径字符串）并更新 `firmware/images/nuttx_errint_fix.bin`
- 全库复查：历史参考仓库标识（旧目录名/旧队号/旧板名）均为 0 命中

## [0.5.0] - 2026-08-08

### 仓库自包含化（脱离外部参考仓库）
- 解除 nuttx 工作区 3 个 symlink，芯片层/板级层源码真实落地
- 修复 `boards/common/qemu_initialize.c` board hook 强符号冲突（改 weak）
- FSBL 构建依赖全部收编（HAL/CMSIS/ExtMem_Manager/BSP/startup/ld）
- `build_fsbl.sh` 改为相对路径自包含版本
- **移除历史参考仓库依赖（已备份归档）**
- ✅ 验证：仅凭本仓库重建 FSBL 与已烧录固件**字节级 100% 一致**；nuttx 构建通过
- 新增 `docs/build-environment.md`（从零复现步骤）

## [0.4.2] - 2026-08-08

### 仓库改为完整源码模式
- `arch/arm/stm32n6/`：芯片层**完整源码**（80 源文件 + 头文件），含启动/串口/ENSR 时钟/XSPI 全部修复
- `board/atk-dnn647/`：板级层**完整源码**（configs/include/scripts/src），含 600MHz 时钟/LED/按键驱动
- 源码与已烧录固件 `nuttx_errint_fix.bin` 一致（diff 校验 0 差异）
- 移除增量 patch 方案（保留 nuttx 核心树 patch）

## [0.4.1] - 2026-08-08

### 项目仓库填充
- 填充 `firmware/`（原创）：自定义 FSBL 源码（LRUN + RIF/RISAF 授权）+ Linux 构建/hex 验证脚本 + 已烧录固件镜像
- 填充 `arch/arm/stm32n6/`：芯片层增量 patch（启动/串口/ENSR 时钟/XSPI）+ nuttx 核心 patch
- 填充 `board/atk-dnn647/`：板级层增量 patch（600MHz 时钟/链接脚本/LED/按键驱动）
- 建立 docs（架构/移植/状态机/AI/演示）、hardware、models、tools 等完整目录

## [0.4.0] - 2026-08-08

### 完成（Phase 4）
- 自定义 FSBL（LRUN + RIF/RISAF 授权）全链调通：
  - FSBL header 结构修复（0x1C0 填充 / 0x70 入口 / byte_sum）
  - RIF 时钟使能、AHB3ENSR 覆盖写合并、GPIO 引脚 RIF 属性
  - RISAF SEC 位（CFGR=0xff0101）→ CopyApplication 成功
- NuttX 侧：
  - v9 ENSR 时钟修复（DEV boot 可启动）
  - 600MHz 时钟分支匹配 FSBL
  - `__start` CONTROL.SPSEL 检查（修复 MSP==PSP 冲突）→ **Flash boot 跑通**
  - 串口错误标志（FE/NE/ORE）清理增强
- **里程碑：openvela 从外部 NOR Flash 独立启动，NSH 可交互 + LED 可控**

## [0.3.0] - 2026-08-04

### 完成（Phase 3）
- NSH 启动（AXISRAM）、ostest 通过
- LED/按键板级支持

## [0.2.0] - 2026-08-03

### 完成（Phase 2）
- 板级层适配（USART1/LED/按键）
- nsh + nsh-qemu 双配置编译通过

## [0.1.0] - 2026-08-02

### 完成（Phase 0-1）
- 采纳 NuttX STM32N6 初始 port 为基座
- 芯片层核对/补齐
