# tools — 辅助工具

开发、标定与交付用的 **PC 侧**脚本（板端实现见 `app/`，不在本目录）。

| 目录 | 内容 | 说明 |
|------|------|------|
| `落板/` | `patch_nsblob.py` · `patch_qconsts.py` · `build_hex.py` | 新模型落板三件套：epoch program 放进 `.nsblob`、更新量化常量、生成可烧录 Intel HEX（含自检） |
| `dataset_audit/` | `audit.py` · `occlusion_*.py` · `yaw_check.py` | 数据集质检：几何/曝光/清晰度/旧模型判决审计、遮挡实验、头偏检验 |
| `board_change_history/` | `patch_capture.py` 等 4 个 | 开发过程中的板端改动脚本（历史留档；最终状态以 `arch/` `board/` `app/` 为准） |
| `UI字体/` | `gen_eye_font.py` · `preview_ui.py` · 4 档字体 `.c` · 预览图 | 由 Noto TTF 生成 LVGL 位图字体（48/32/24 界面字 + 64 倒计时数字），并在 PC 上预览界面 |
| （本目录） | `export_copilot_logs.py` · `render-log.py` · `validate-log.py` | AI Coding 日志的转换、渲染与合规校验，详见 `logs/README-leihan-oli.md` |

## 板端对应实现（这些能力在代码里的位置）

| 能力 | 实现位置 |
|------|----------|
| 图像采集 | `app/examples/eye_cam/`：`eye_cam cap` 采集模式，按脸框裁剪后存 BMP 到 SD 卡 |
| ROI 预处理 | 同上：`eye_cam_roi_to_input` + `eye_cam_build_taps`（多相重采样，与训练端 `PIL.BILINEAR` 对齐） |
| 串口上报 | 界面"串口命令"二级页打印动作事件；运行时日志走 `/dev/console` |
