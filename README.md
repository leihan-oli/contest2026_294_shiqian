# contest2026_294_shiqian — 基于 openvela 的 STM32N647 眼动眨眼无接触控制器

> openvela 2026 大赛参赛项目（队伍 #294，dev-ai-contest-2026 分支）
>
> **参赛方向：新硬件平台适配 + 端侧 AI 应用** —— 从 BootROM / FSBL 到应用层，
> 把一块 STM32N647 完整跑起来，并在它的 NPU 上落地"人脸检测 + 眼动五分类"的
> 端侧交互 demo。

## 项目简介

面向无障碍交互、智能家居与工业免手操作场景，基于 **openvela 操作系统**（NuttX 系）和
**正点原子 ATK-DNN647（STM32N647X0H3Q，Cortex-M55 @800 MHz + 600 GOPS NPU）**构建一套
可视化控制界面：通过**边缘 AI（NPU）**在端侧识别人脸与眼部动作，把"看左 / 看右 / 闭眼"
转换为控制指令，实现**无需触摸、无需联网、保护隐私**的本地化人机交互控制器。

整条链路都跑在板子上：摄像头取帧 → NPU 人脸检测（YuNet 256×416）→ 按脸框裁剪眼部 →
NPU 眼动五分类（64×128）→ LVGL 界面动作 → 外设控制 + 蜂鸣器音调反馈。

## 选题方向

**新硬件平台适配（官方《支持的硬件平台》「待适配开发板」清单中的 STM32N647）
＋ 端侧 AI 应用。**

选它的理由：STM32N647 是 ST 面向边缘 AI 的旗舰 MCU（Cortex-M55 @800 MHz +
600 GOPS NPU），赛前 openvela 尚无任何 STM32N6 支持，属于「从 0 到 1」的首次适配；
它同时自带 MIPI CSI-2、LTDC、2D 加速等完整多媒体栈，足以在一块板上同时落地
**图形、AI、多媒体**三项 openvela 核心能力，并把 NPU 真正跑起来（而不是退回 CPU 推理）。

获奖后适配代码将按大赛要求 PR 至上游 `vendor_st` 的 `dev-ai-contest-2026` 分支。

## 当前状态（2026-09-20）

| 阶段 | 状态 | 说明 |
|------|------|------|
| Phase 0-3 | ✅ | 环境搭建、芯片层/板级层适配、NSH 启动、ostest |
| Phase 4 | ✅ | **自定义 FSBL（LRUN + RIF/RISAF）+ 固件落外部 NOR + Flash boot** |
| Phase 5 | ✅ | **LTDC 800×480 + LVGL 眼控界面**：3×2 网格主页 + 6 个二级页 |
| Phase 6 | ✅ | **DCMIPP + IMX335 实时预览**、拍照存 BMP（含倒计时与重拍） |
| Phase 7 | ✅ | **NPU/ATON 双模型**：人脸检测 + 眼动五分类（991 张自采数据集） |
| Phase 8 | ✅ | **眼控应用**：闭眼进入 / 左看返回 / 持续注视触发 + LED、信息、拍照、串口、关于 |
| 附加 | ✅ | HyperRAM(32 MB) 打通、GT911 触摸、蜂鸣器音调反馈、系统信息页 |

**眼动识别实测**：自采数据集 991 张（五类），float 991/991、int8 990/991；板端实时运行
（NPU 双模型，界面流畅）。

## 适配规模（一屏看完）

| 维度 | 数字 |
|------|------|
| **芯片层驱动** | **43 个源文件 / 36,732 行**（`arch/arm/stm32n6/`），其中 ATON、DCMIPP、IMX335、LTDC/LCD、GT911、CACHEAXI、GPDMA 扩展、XSPI 等为本项目开发或修复 |
| **板级支持** | 11 个源文件 / 2,632 行，**8 个板级配置**（产品 `eye`；NPU 分阶段 `npu` / `npu-cam` / `npu-o0`；端口自检 `nsh` / `nsh-test` / `nsh-qemu`；边缘计算历史配置 `edgesight`） |
| **应用层** | **12 个 openvela 应用 / 12,170 行**（`apps/examples/`）：眼控主应用（含自绘 4 档位图字库）+ 11 个移植与调试工具（取帧 / 存图 / NPU 探针 / FSBL 自检 / 显示 / 视频 / 成像调试） |
| **设备节点** | **17+ 个**：`/dev/aie0`(NPU)、`/dev/lcd0`、`/dev/fb0`、`/dev/input0`、`/dev/mmcsd0`、`/dev/userleds`、`/dev/buttons`、`/dev/timer0–5`、`/dev/oneshot`、`/dev/watchdog0/1`、`/dev/adc0/1`、`/dev/pwm0/1`、`/dev/temp0`、`/dev/cap0`、`/dev/i2c2`、`/dev/i2c4` |
| **核心树改动** | **13 个文件 / 4 个仓**：nuttx（Kconfig 挂接、fault 处理、cache、ATON 头）、apps（speexdsp、lvgldemo）、vendor/openvela（board hook 改 weak）、LVGL（MVE 编译门控）；补丁 + `core-tree-overlay/` 双通道交付，`scripts/verify-core-tree.sh` 离线自证三者一致 |
| **原厂例程对照** | 原厂 **66 个裸机例程工程**（`01_LED` … `99_Applications/995_AI_Hand_Landmarks`）；本项目把与产品相关的初始化序列逐一移植进 NuttX：`15_RGBLCD`（LTDC 时序/引脚）、`38_SD_Card` / `39_FatFs`（SDMMC1 + FAT32 读写，拍照落盘）、`995_AI_Hand_Landmarks`（NPU/ATON 移植路径）、`01_LED`（板级点亮）、`40_Chinese_Show` 与官方 FSBL（XSPI1/HyperRAM 配置金标准），并全部上板验证。按“所需外设是否已在端口内适配”逐项对照，**当前板级配置已支持跑通 41/66 个例程**（板上外设 + LTDC/DMA2D + NPU + FPU/DSP/RTOS 等能力；其中触摸与 NPU 相关例程为同能力覆盖并已实测：`11_TPAD`、`27_Touch`、`991–995_AI_*`），另有 **7 个**例程所需的 FDCAN、USB 主机/设备与以太网控制器驱动已随端口提供；受**固件窗口**限制（FSBL 单次载入 1 MiB + 内部 RAM 2 MiB），未把全部例程代码纳入本仓库 |
| **系统能力** | NSH shell、procfs / tmpfs、FAT32 + SD 卡、LVGL 9.1 图形栈、ATON NPU 运行时、V4L2 风格取帧、多路定时器 / oneshot / 看门狗 / ADC / PWM / RTC / HASH |

## 外设与驱动适配

★ = 本项目开发或修复的驱动。

### 产品路径（已上板验证）

| 外设 | 接口 | 驱动 | 验证方式 |
|------|------|------|----------|
| 外部 NOR 32 MB | XSPI2 @ `0x70000000` | ★ `stm32n6_xspi.c` | 启动介质；自定义 FSBL 从 NOR 引导 NuttX |
| **HyperRAM 32 MB** | XSPI1 @ `0x90000000` | ★ `stm32n6_xspi.c` | 读写自检通过；相机帧落 `0x90100000`，为内部 RAM 腾出 750 KB |
| 7" RGB 屏 800×480 | LTDC（20 路 RGB565，AF14）+ 背光 PA3 | ★ `stm32n6_ltdc.c`、★ `stm32n6_lcd.c` | `/dev/lcd0` + `/dev/fb0` 双通路，LVGL 界面正常显示 |
| 电容触摸 GT911 | 软件位时序（PD4/PD14）+ RST PD10 / INT PB3 | ★ `stm32n6_gt9xxx.c`、★ `stm32n6_gt9xxx_input.c` | `/dev/input0` 注册为 LVGL 触摸输入 |
| 摄像头 IMX335 | MIPI CSI-2 + I2C2 | ★ `stm32n6_imx335.c`、★ `stm32n6_dcmipp.c`、★ `stm32n6_video.c` | 实时预览、按帧拍照存 BMP |
| **NPU（Neural-ART 600 GOPS）** | ATON 运行时 | ★ `stm32n6_aton_aie.c`、★ `libs/ai_aton/` | `/dev/aie0`；双模型推理，结果实时上屏 |
| DMA 控制器 | GPDMA1 | ★ `stm32n6_dma.c`（新增 M2P/超时/回读） | 摄像头与传输通路 |
| 2D 图形加速 | DMA2D | `stm32n6_dma2d.c` | 图像搬运 / 格式转换 |
| TF 卡 | SDMMC1 | `stm32n6_sdmmc.c` | `/dev/mmcsd0`（FAT32），拍照写盘 |
| 串口控制台 | USART1（PE5/PE6，AF7） | `stm32n6_serial.c` | NSH 交互与日志输出 |
| 蜂鸣器 | PD3（经三极管驱动） + TIM3 中断 | ★ 板级 `stm32n6_buzzer.c` | 四音调反馈，可凭音高分辨动作 |
| 用户 LED | PG10 / PE10（active-low） | `stm32n6_userleds.c` | `/dev/userleds` |
| 按键 | PC6 / PD1 / PG11 | `stm32n6_userbuttons.c` | `/dev/buttons` |
| 定时器 / oneshot | TIM2 / TIM5 | `stm32n6_tim.c`、`stm32n6_oneshot.c` | `/dev/timer0`、`/dev/oneshot` |
| 看门狗 | IWDG / WWDG | `stm32n6_iwdg.c`、`stm32n6_wwdg.c` | `/dev/watchdog0/1` |
| 随机数 | RNG | `stm32n6_rng.c` | 端口自检 |
| I2C | I2C2（摄像头控制）/ I2C4（光感 AP3216C） | `stm32n6_i2c.c` | `/dev/i2c2`、`/dev/i2c4` |
| 缓存 / 安全域 | CACHEAXI、I-Cache / D-Cache、RIF/RISAF 授权 | ★ `stm32n6_cacheaxi.c`、`stm32n6_start.c`、`stm32n6_rcc.c` | 性能关键路径：D-Cache 生效后单帧 **2.7 s → 60 ms（45×）** |
| 异常与复位 | HardFault/MemFault/BusFault/UsageFault 处理、复位原因 | `arm_m/*`、★ `stm32n6_fsbl_regress.c` | 崩溃可定位到函数与行，不静默重启 |

### 随端口提供（代码与配置齐备，本产品路径未启用）

| 外设 | 驱动 | 设备节点 / 配置 |
|------|------|------------------|
| ADC1 / ADC2 | `stm32n6_adc.c` | `/dev/adc0`、`/dev/adc1`（`nsh-test` 配置） |
| PWM | `stm32n6_tim.c` | `/dev/pwm0`、`/dev/pwm1` |
| 温度传感器 | `stm32n6_dts.c` | `/dev/temp0` |
| RTC | `stm32n6_rtc.c` + lowerhalf | `/dev/rtc0` |
| HASH / 加解密 | `stm32n6_hash.c` | `nsh-test` 配置 |
| 低功耗定时器 | `stm32n6_lptim.c` | 可选 |
| CAN / CAN FD | `stm32n6_fdcan.c` | 可选 |
| 以太网 MAC | `stm32n6_ethernet.c` | `edgesight` 配置（含 lwIP、TCP/UDP） |
| USB OTG | `stm32n6_otg.c` | 可选 |
| SPI | `stm32n6_spi.c` | 可选 |
| 外部 NOR 文件系统 | MTD + `stm32n6_xspi.c` | 可挂载 |

## 端侧 AI 能力

| 环节 | 实现 | 结果 |
|------|------|------|
| 人脸检测 | YuNet 256×416，跑在 NPU（ATON），12 个输出张量三尺度（stride 8/16/32）解码 | 人脸框稳定框住双眼，输出眼心坐标与瞳距 |
| 眼动识别 | MobileNetV2 64×128 五分类（closed / open / left / right / other） | 自采数据集 991 张：float **991/991**、int8 **990/991** |
| 输入对齐 | 训练与推理走**同一条链路**：按人脸框裁剪（`crop_w = 2.0 × 瞳距`） | 这是 left/right 从"几乎零召回"到可用的关键（见 ADR-0002） |
| 推理调度 | 双模型都在 NPU，CPU 只做取帧、裁剪与界面 | 单帧开销 **2.7 s → 60 ms（45×）** |
| 模型交付 | 模型源码 + 权重镜像都在仓库内 | `libs/ai_aton/models/{eye,face}/`、`firmware/images/{eye,face}-data.hex` |

## 界面与交互

| 项 | 内容 |
|----|------|
| 图形栈 | LVGL 9.1（Helium 汇编加速）+ LTDC 800×480 RGB565；单 framebuffer 与摄像头互斥复用（省下 750 KB） |
| 界面结构 | 3×2 方形按钮主页 + **6 个二级页**：眼动识别展示 / 拍照 / 系统信息 / LED 控制 / 串口命令 / 关于 |
| 中文显示 | 自绘 4 档位图字库（48 / 32 / 24 px 界面字 + 64 px 倒计时数字），由 Noto TTF 生成，工具在 `tools/UI字体/` |
| 眼控动作 | 闭眼进入 / 左看返回 / 右看移动 / 持续注视 0.6–1.0 s 触发；带冷却时间与"静息判决"防误触 |
| 听觉反馈 | 蜂鸣器四音调：闭眼 784 Hz / 左 587 Hz / 右 988 Hz / 其它 660 Hz；二级页按住时连续发声 |
| 拍照 | 闭眼进入 → 3 s 大号数字倒计时 → 全帧 800×480 BMP 存入 SD 卡，产物不带任何标注 |
| 使用方式 | 人脸在画面内即可：系统自动定位并裁剪眼部，允许自然的头动与坐姿变化 |

## 目录结构

```text
contest2026_294_shiqian/
├── firmware/            # 固件与构建依赖（原创 + 自包含）
│   ├── fsbl/            #   自定义 FSBL 源码（LRUN + RIF/RISAF）
│   ├── stm32cube/ bsp/  #   ST HAL/CMSIS/ExtMem_Manager + 正点原子 BSP
│   ├── cubeide/ scripts/ sdtest/ build/
│   └── images/          #   ★ 可烧录镜像（见"烧录"）
├── arch/arm/stm32n6/    # 芯片层完整源码（含 ATON/DCMIPP/LTDC/XSPI 驱动）
│   ├── src/ include/    #   经 manifest 映射到 nuttx/arch/arm/{src,include}/stm32n6
│   └── nuttx-core-incremental.patch   # 核心树增量（Kconfig / 异常处理 / I-Cache / 向量表，9 个上游文件）
├── board/atk-dnn647/    # 板级层完整源码（经 manifest 映射到 vendor/openvela/boards/atk-dnn647）
├── app/                 # openvela 应用（经 manifest 映射到 apps/examples/）
│   ├── examples/eye_cam/    ★ 眼控主应用（UI + 摄像头 + NPU + 拍照 + 蜂鸣器）
│   ├── examples/            #   其余 11 个：eye_guide（眼位引导）、palm_cam / pose_cam（NPU 移植样例）、
│   │                        #   aie_probe（NPU 探针）、cam / cam_probe / cam_save（取帧·探针·存图）、
│   │                        #   imx335_tune（成像调试）、fbcolor / video（显示与视频）、fsbl_regress（FSBL 自检）
│   └── apps-core-incremental.patch    # apps 核心树增量（2 个上游文件）
├── libs/ai_aton/        # ATON NPU 运行时 + 全部模型（经 manifest 映射到 nuttx/libs/ai_aton）
├── models/              # 模型与数据
│   ├── dataset/         #   眼动数据集 991 张（五类，含 meta.txt）
│   ├── training/        #   训练/量化脚本 + checkpoint
│   ├── exported/        #   导出的 ONNX（float + QDQ int8）
│   └── cubeai/          #   Cube.AI(CPU) 路径未采用，目录内说明原因
├── docs/                # 项目文档（★ 先看 reproduce.md）
├── .claude/skills/      # ★ 自建 Skill（本项目的可复用流程）
├── hardware/            # 硬件资料（引脚分配表）
├── scripts/             # 构建 / 边界检查 / 镜像校验脚本
├── tests/               # 离线自检（产物 / 数据集 / 日志）
├── quickapp/            # 快应用目录（本作品是原生应用，未使用）
├── tools/               # 落板脚本、UI 字体工具、数据集审计、日志导出/渲染/校验
├── logs/                # AI Coding 会话日志（大赛评审用）
├── schema/              # 日志数据契约
├── contest2026_294_shiqian.xml   # ★ manifest：把上面各目录 <linkfile> 映射进 openvela 编译树
└── openvela.xml                  # openvela 全量工程清单（被上面的 manifest include）
```

## 从零复现

完整步骤见 **[docs/reproduce.md](docs/reproduce.md)**，概览：

```bash
# 1. 一键拉取「openvela 全量源码 + 本仓」（首次约 5 GB，需耐心）
repo init -u https://github.com/open-vela/contest2026_294_shiqian \
  -b dev-ai-contest-2026 -m contest2026_294_shiqian.xml
repo sync -c -j8

# 2. 本仓子目录由 manifest 的 <linkfile> 自动映射进编译树，无需手动拷贝：
#    board/atk-dnn647               -> vendor/openvela/boards/atk-dnn647
#    arch/arm/stm32n6/{src,include} -> nuttx/arch/arm/{src,include}/stm32n6
#    libs/ai_aton                   -> nuttx/libs/ai_aton
#    app/examples/*                 -> apps/examples/*

# 3. 核心树增量：4 个补丁 / 13 个文件（芯片层 / 板级 / 应用目录之外的上游改动）
#    nuttx(9) · apps(2) · vendor/openvela(1) · apps/graphics/lvgl/lvgl(1)
#    脚本会自动 git apply，失败则三方合并；仍失败时用 --overlay 直接覆盖
bash contest2026_294_shiqian/scripts/apply-core-tree.sh

# 4. 编译（★ 任何 defconfig 改动后必须先清构建目录）
rm -rf cmake_out/atk-dnn647_eye
./build.sh vendor/openvela/boards/atk-dnn647/configs/eye --cmake -j$(nproc)
```

产物：`cmake_out/atk-dnn647_eye/nuttx.bin`（本仓库 `firmware/images/nuttx.bin`，
948,064 B，md5 `a60d2701426123c94c480676ff56dbcf`）

## 烧录

用 CubeProgrammer + External Loader `MX25UM25645G_ATK-CNN647B`，写外部 NOR：

| 镜像 | 地址 | 文件 |
|---|---|---|
| 自定义 FSBL | `0x70000000` | `firmware/images/fsbl_rifsecfix.hex` |
| NuttX 固件 | `0x70100000` | `firmware/images/nuttx.bin` |
| 眼动模型权重 | `0x70400000` | `firmware/images/eye-data.hex` |
| 人脸模型权重 | `0x70700000` | `firmware/images/face-data.hex` |

> 串口调试：115200，**不要开启 DTR/RTS**（会导致复位/挂起）。

## 上板验收（nsh）

```text
nsh> eye_cam                 # 启动眼控界面：主页 3×2 网格
                             #   闭眼进页 / 左看返回 / 右看移动
                             #   二级页按住动作 0.6~1.0 s 触发
nsh> eye_cam tone 400 500    # 蜂鸣器低音（自检）
nsh> eye_cam tone 3000 500   # 蜂鸣器高音
nsh> eye_cam xspi dump       # HyperRAM / XSPI 状态
```

## 资源边界（改动代码前必读）

| 约束 | 上限 | 当前 | 说明 |
|---|---|---|---|
| `_ebss` | ≤ `0x34200000` | `0x341b45ac`（余 ~299 KB） | 之上是 NPU/AXISRAM 域，越界会**静默不启动** |
| 固件体积 | ≤ 1 MiB | 948,064 B（余 ~98 KB） | FSBL `EXTMEM_LRUN_SOURCE_SIZE` 限制 |
| LVGL 堆 | 192 KB（内部 RAM） | — | 放 HyperRAM 会崩（实测），保持 `LV_MEM_ADR=0x0` |

## 代码基线说明

**本仓库完全自包含，不依赖任何外部参考仓库。**

- 芯片层 / 板级层 / 应用 / ATON 运行时均为**完整源码**；核心树只改了少数文件，以补丁给出
- `firmware/` 为原创成果（自定义 FSBL + 脚本 + 镜像），构建所需 ST 资源（HAL/CMSIS/
  ExtMem_Manager/BSP/startup/ld）全部收入仓库
- 源码头文件保留原始 Apache-2.0 版权声明

## 已知限制

如实列出当前版本的限制，每条都给出了结论与后续路径（大赛要求：未完成的功能如实写明）：

| 限制 | 现状 | 说明与排查记录 |
|------|------|----------------|
| 喇叭播放（ES8388 + SAI1） | 未打通 | 时钟链（HSE→PLL2→IC7→SAI1）与 MCLK 实测正常，但 SAI FIFO 恒定不排空（DMA 与 CPU 直灌均超时）。声音反馈改用**蜂鸣器四音调**实现，见 `docs/audio-buzzer.md`、ADR-0005 |
| 界面显示与摄像头预览 | 分时互斥 | 只有一块 750 KB framebuffer（为内部 RAM 让路），预览与 LVGL 界面不并存 |
| 眼动识别泛化性 | 自采数据集 | 991 张五分类数据集上 float 991/991、int8 990/991；换人 / 换光照的系统性评测未覆盖 |
| 曝光与增益 | 参数化固定值 | 与裸机金标准一致的固定参数，未做自适应（AE / AGC） |
| USB 主机、以太网、CAN | 驱动随端口提供 | 本产品路径未接外设实测（见「外设与驱动适配 · 随端口提供」） |
| 原厂例程覆盖 | 部分纳入 | 所需外设未接入或未适配的例程（OLED / FMC 屏 / 外接传感器 / SD-NAND / JPEG / SPDIF 等）未纳入本仓库，见「适配规模」 |

## AI Coding 日志

`logs/leihan-oli/` 下为本项目的 AI Coding 会话记录（14 个会话 / 38,296 事件），
来源、转换方式与限制见 [logs/README-leihan-oli.md](logs/README-leihan-oli.md)；
校验与渲染工具见 [logs/README.md](logs/README.md)（`validate-log.py` → `ALL OK`）。

## AI Coding 使用说明

开发全程与 AI 协作（VS Code + AI Agent 驱动芯片层 bringup、驱动与应用开发），
对话日志已按大赛要求导出到 `logs/`。

| 环节 | AI 如何参与 | 实际效果 |
|------|-------------|----------|
| 需求拆解 / 方案 | 读官方文档与厂商资料，梳理自定义 FSBL、RIF/RISAF 授权、DCMIPP 寄存器等陌生领域 | 纠正 BootROM 向量表偏移（0x70 → 0x400）等关键误判，少走弯路 |
| 编码 | 按子系统生成驱动骨架（sensor / chardev / fb / input / timer…），批量过 `checkpatch.sh` | 支撑 43 个芯片层源文件 / 36,732 行的实现速度 |
| 调试 | 寄存器 dump 解读、**裸机金标准 vs NuttX 双端逐位 diff**、单变量最小实验设计 | 摄像头 MIPI / SDMMC / NPU 三个卡点的定位（见自建 Skill） |
| 文档 | 生成并随代码同步 requirements / design / tasks、ADR 与复现指南 | 6 篇 ADR + 13 篇 `docs/` |
| 方法论沉淀 | 把反复用到的流程固化成 Skill 与脚本 | 2 个 Skill（下节）+ 边界自检脚本 |

日志规模：**14 个会话 / 38,296 事件**（`logs/leihan-oli/`），官方 `validate-log.py` 校验 **ALL OK**。

## 自建 Skill

`.claude/skills/` 下是两个在开发过程中真正沉淀下来的流程（不是示例占位）：

| Skill | 解决什么 | 关键内容 |
|-------|----------|----------|
| [`aton-model-deploy`](.claude/skills/aton-model-deploy/SKILL.md) | **换模型 / 重训后重新落板** | 板端契约核对 → `.nsblob` 钩子 → 量化常量（含**派生**的 `Q_GAIN/Q_BIAS`）→ Intel HEX 权重镜像（逐字节自检）→ 边界自检 → 板端验证；附 8 条实测坑 |
| [`peripheral-bringup-debug`](.claude/skills/peripheral-bringup-debug/SKILL.md) | **外设 bringup 失败的系统化定位** | 同 FSBL 裸机金标准双世界对照 + 双端逐位 diff + 单变量最小实验 + 隐性使能位清单；附摄像头 / SDMMC 两个实战案例 |

装上任意支持 Skill 的 AI 工具后，直接说"帮我落板这个新模型"或"这个外设调不出来"
即会命中。

## 离线自检

不需要开发板，只要一个 `python3`：

```bash
bash tests/run_all.sh
```

| 检查 | 覆盖 |
|------|------|
| `check_artifacts.py` | 4 个镜像存在、**Intel HEX 行校验和**、装载地址与烧录表一致、`nuttx.bin ≤ 1 MiB` |
| `check_dataset.py` | 数据集 991 张 / 五类计数 / 命名前缀 / `meta.txt` |
| `check_logs.py` | AI 日志：优先官方 `tools/validate-log.py`（python ≥ 3.9），否则退回结构检查 |

## 运行证据

| 证据 | 位置 |
|------|------|
| 板端验收步骤与**预期串口特征行** | [docs/reproduce.md](docs/reproduce.md) 第 5 节 |
| 可离线复核的产物与数据一致性 | `tests/run_all.sh`（三条检查全绿） |
| AI Coding 全过程（含每次上板结果与排障） | `logs/leihan-oli/` |
| 真机照片 / 演示视频 | 随作品介绍文档一并提交（提交包内） |

性能与识别率数字的来源（不是估值）：识别率来自数据集上的 float/int8 复算
（`models/training/eval_model.py`），帧率与时延来自板端串口打印的时间戳。
