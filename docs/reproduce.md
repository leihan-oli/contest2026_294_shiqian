# 从零复现（Reproduce）

本文是**唯一需要的复现入口**：从一台干净机器到板子上跑起眼控界面。

## 0. 需要什么

| 类别 | 内容 |
|---|---|
| 硬件 | 正点原子 ATK-DNN647（STM32N647X0H3Q）+ IMX335 摄像头模块 + 7" 800×480 RGB 屏 + microSD 卡（**FAT32**）+ USB-TTL 串口 |
| 软件 | Ubuntu 20.04+、`repo`、Python 3（脚本用）、STM32CubeProgrammer + External Loader `MX25UM25645G_ATK-CNN647B` |
| 源码 | 用组委会的 manifest 一键拉取：`repo init -u https://github.com/open-vela/contest2026_294_shiqian -b dev-ai-contest-2026 -m contest2026_294_shiqian.xml` + `repo sync -c -j8`（含 `nuttx/` `apps/` `vendor/` 与本仓） |

> 串口工具**不要开启 DTR/RTS**（会导致板子复位或挂起）。

## 1. 代码入位

```bash
# 1. 一键拉取「openvela 全量源码 + 本仓」
repo init -u https://github.com/open-vela/contest2026_294_shiqian \
  -b dev-ai-contest-2026 -m contest2026_294_shiqian.xml
repo sync -c -j8
cd ~/openvela
R=contest2026_294_shiqian

# 2. 本仓子目录由 manifest 的 <linkfile> 自动映射进编译树，**无需手动拷贝**：
#      board/atk-dnn647               -> vendor/openvela/boards/atk-dnn647   （板级层）
#      arch/arm/stm32n6/{src,include} -> nuttx/arch/arm/{src,include}/stm32n6 （芯片层）
#      libs/ai_aton                   -> nuttx/libs/ai_aton                （ATON 运行时 + 模型）
#      app/examples/*                 -> apps/examples/*                   （12 个应用）

# 3. 核心树增量：4 个补丁 / 13 个文件（nuttx 9 · apps 2 · vendor/openvela 1 · LVGL 1）
#    自动 git apply → 失败三方合并 → 仍失败可用 --overlay 直接覆盖（不依赖上游内容）
bash $R/scripts/apply-core-tree.sh
bash $R/scripts/apply-core-tree.sh --check   # 可选：确认 4 个仓都已应用
```

补丁涉及的文件（便于人工核对）：

- `nuttx`（9 个）：根 `Kconfig`、`arch/arm/Kconfig`（`ARCH_CHIP_STM32N6` 选项与
  `source "arch/arm/src/stm32n6/Kconfig"` 挂接）、`arch/arm/src/arm_m/arm_{bus,hard,mem,usage}fault.c`、
  `arch/arm/src/arm_m/arm_vectors.c`（启动阶段诊断）、`arch/arm/src/armv8-m/arm_cache.c`、
  `include/nuttx/aie/stm32n6_aton_aie.h`
- `apps`（2 个）：`audioutils/speexdsp/CMakeLists.txt`、`examples/lvgldemo/lvgldemo.c`
- `vendor/openvela`（1 个）：`boards/common/src/qemu_initialize.c` —— 三个 board hook 改 **weak**，
  否则与 `board_bringup.c` 冲突，链接期 `multiple definition` 必然失败
- `apps/graphics/lvgl/lvgl`（1 个）：`src/draw/lv_draw_buf_blur.c` —— MVE 编译门控
  （openvela 选了 `CONFIG_ARM_HAVE_MVE` 但没加 `-march=…+mve`）

> `core-tree-overlay/` 存放这 13 个文件的**补丁后内容**，是补丁的等价兜底：
> 若上游分支前移导致 `git apply` / 三方合并都失败，`bash $R/scripts/apply-core-tree.sh --overlay`
> 直接覆盖即可得到同样的树。离线自证：`bash $R/scripts/verify-core-tree.sh`（补丁 ↔ overlay ↔ 当前树 三者一致）。

> 上述改动都在 openvela 公共树里，属于「获奖后 PR 至上游」的部分；比赛期间以补丁形式
> 随仓提供，保证评委在专属仓内即可完整复现。

## 2. 编译

```bash
cd ~/openvela

# ⚠️ 只要动过 defconfig，就必须清构建目录：build.sh 复用旧 .config，
#    改动不会生效（本项目踩过两次，表现为"改了配置但行为没变"）
rm -rf cmake_out/atk-dnn647_eye

./build.sh vendor/openvela/boards/atk-dnn647/configs/eye --cmake -j$(nproc)
```

关键配置（`configs/eye/defconfig`）：V4L2 + DCMIPP 摄像头、LVGL（LCD PARTIAL 路径 +
Helium 汇编加速）、ATON NPU（`models/eye`）、UTF-8 中文字库、`/dev/userleds`、蜂鸣器。

## 3. 边界自检（**必做**）

```bash
export PATH=$PATH:$PWD/prebuilts/gcc/linux-x86_64/arm-none-eabi/bin

# ① RAM 上界：超过 0x34200000 会静默不启动（NPU 张量池从这里开始）
arm-none-eabi-nm cmake_out/atk-dnn647_eye/nuttx | grep ' A _ebss'

# ② 固件体积：FSBL 一次只拷 1 MiB
ls -l cmake_out/atk-dnn647_eye/nuttx.bin
```

参考值（本仓库产物）：`_ebss = 0x341b45ac`、`nuttx.bin = 948,064 B`。

## 4. 烧录

| 镜像 | 地址 | 必需 |
|---|---|---|
| `firmware/images/fsbl_rifsecfix.hex` | `0x70000000` | 首次/换 FSBL 时 |
| `firmware/images/nuttx.bin` | `0x70100000` | 每次改固件 |
| `firmware/images/eye-data.hex` | `0x70400000` | 换眼动模型时 |
| `firmware/images/face-data.hex` | `0x70700000` | 换人脸模型时 |

## 5. 上板验收

```text
nsh> eye_cam                 # 眼控界面：闭眼进页 / 左看返回 / 右看移动 / 按住触发
nsh> eye_cam tone 400 500    # 蜂鸣器：低音
nsh> eye_cam tone 3000 500   # 蜂鸣器：高音
nsh> eye_cam xspi dump       # HyperRAM/XSPI 状态（排查用）
nsh> ls /mnt/sdcard          # 拍照产物：photo_*.bmp
```

预期串口特征行：

```text
xspi1: HyperRAM armed (FMODE 1 -> 3, CCR=... WCCR=... DCR2=00000000 ...)
eye_ui: panel up, 3x2 cells, select 0
stm32n6_aton: eye/face model ready
eye_ui: CONFIRM 3 系统信息 (100%, held 1180ms) -> page 2, 5 cells
```

## 6.（可选）重训 → 重新生成模型

完整链路见 [ai-pipeline.md](ai-pipeline.md)。最短路径：

```bash
# 训练（Linux 或 Windows 均可；数据集在 models/dataset/）
python3 models/training/train_blink.py --flat models/dataset --task blink --size 64x128
python3 models/training/export_onnx.py          # → blink_v3.onnx
python3 models/training/quantize_onnx.py ...    # → blink_v3_qdq_int8.onnx (calib 345)
# stedgeai generate（需 ST Edge AI 工具链）→ network.c + network_atonbuf.xSPI2.raw
cp network.c network_ecblobs.h network_atonbuf.xSPI2.raw nuttx/libs/ai_aton/models/eye/
python3 tools/落板/patch_nsblob.py              # 把 epoch program 放进 .nsblob
python3 tools/落板/patch_qconsts.py             # 更新 6 个量化常量
python3 tools/落板/build_hex.py                 # 生成可烧录 Intel HEX（含自检）
```

## 7. 常见坑（都实际踩过）

| 现象 | 原因 |
|---|---|
| 改 defconfig 没效果 | `build.sh` 复用旧 `.config` → `rm -rf cmake_out/<board>` |
| 烧完不启动、无串口 | `_ebss > 0x34200000`（NPU 域），或固件 > 1 MiB |
| 屏幕黑屏 | RISAF 未授权 LTDC 读 AXISRAM + APB5/SRAM 时钟门控未开（三因，见 openvela-porting.md） |
| 中文显示成"斜 45° 分层" | LVGL 1bpp 是**连续位流**（行间不补齐），生成字库时必须按此编码 |
| 驱动加了却"没编进去" | 这个 port 用 **CMake**，只改 `Make.defs` 无效，要改 `CMakeLists.txt` |
| 日志顺序错乱 | `printf` 有缓冲、panic 直出；判断现场要看是否有 buffered 行 |
| 源码改了但行为不变 | VS Code 编辑器缓冲可能把改动覆盖回去；改完立刻 `md5sum` 复核 |
