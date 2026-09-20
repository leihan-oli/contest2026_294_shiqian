# app — openvela 应用

本目录经 manifest 的 `<linkfile>` 映射到 openvela 工作区 `apps/examples/`（无需手动拷贝）。
共 12 个应用：1 个产品主应用 + 11 个移植与调试工具。

| 应用 | 说明 | 状态 |
|------|------|------|
| [examples/eye_cam](examples/eye_cam/) | **眼控主应用**：LVGL 界面 + 摄像头预览 + NPU 双模型（人脸检测 / 眼动五分类）+ 拍照 + 蜂鸣器音调 | ✅ 已上板 |
| [examples/eye_guide](examples/eye_guide/) | 眼位引导工具（数据集采集前的构图辅助） | ✅ 已上板 |
| [examples/palm_cam](examples/palm_cam/) | NPU 移植样例：palm995 手掌检测 | ✅ 移植验证 |
| [examples/pose_cam](examples/pose_cam/) | NPU 移植样例：pose994 多人姿态 | ✅ 移植验证 |
| [examples/aie_probe](examples/aie_probe/) | NPU/ATON 探针：运行时版本、模型张量、推理时延 | ✅ 调试用 |
| [examples/cam](examples/cam/) | 摄像头综合命令：`start/stop/status/diag/xspi dump` | ✅ 调试用 |
| [examples/cam_probe](examples/cam_probe/) | 传感器 I2C 探测 / 寄存器读写回读 | ✅ 调试用 |
| [examples/cam_save](examples/cam_save/) | 单帧存图（BMP）到 SD 卡 | ✅ 调试用 |
| [examples/imx335_tune](examples/imx335_tune/) | IMX335 成像参数在线调试 | ✅ 调试用 |
| [examples/fbcolor](examples/fbcolor/) | framebuffer 颜色/几何自检 | ✅ 调试用 |
| [examples/video](examples/video/) | 视频播放路径试验（JPEG/帧序列） | 试验 |
| [examples/fsbl_regress](examples/fsbl_regress/) | FSBL → NuttX 跳转与 RIF/RISAF 闸门自检 | ✅ 调试用 |

核心树（`apps/` 里不属于上述目录）的少量改动见
[apps-core-incremental.patch](apps-core-incremental.patch)：

- `audioutils/speexdsp/CMakeLists.txt`
- `examples/lvgldemo/lvgldemo.c`

LVGL 库的一处改动（MVE 编译门控）单列在 [lvgl-core-incremental.patch](lvgl-core-incremental.patch)，
作用于 `apps/graphics/lvgl/lvgl/`。

> 注意：本 port 用 **CMake** 构建，新增源文件要改 `CMakeLists.txt`；
> 只改 `Make.defs` 不会生效。应用目录由 `nuttx_add_subdirectory()` 自动发现，
> 新增应用无需改动父层 `Kconfig` / `CMakeLists.txt`。
