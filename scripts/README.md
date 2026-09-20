# scripts — 构建 / 烧录辅助脚本

| 脚本 | 用途 |
|------|------|
| `build_eye.sh [--clean]` | 编译 eye 配置，编译后自动做边界自检。**改过 defconfig 必须加 `--clean`**（`build.sh` 会复用旧 `.config`） |
| `check_boundaries.sh [构建目录]` | 单独跑两项边界检查：`_ebss ≤ 0x34200000`、`nuttx.bin ≤ 1 MiB`。两者越界都表现为**静默不启动** |
| `verify_images.sh` | 烧录前校验 4 个镜像（存在性 / Intel HEX 行校验和 / 体积上限），等价于 `tests/check_artifacts.py` |

脚本会自动向上查找 openvela 工作区（找 `nuttx/`），所以在工作区任意位置都能跑。

```bash
bash scripts/build_eye.sh --clean     # 全量重建 + 边界检查
bash scripts/check_boundaries.sh      # 只检查现有构建
bash scripts/verify_images.sh         # 只校验待烧录镜像
```

烧录地址表见 [firmware/images/README.md](../firmware/images/README.md)；
从零复现见 [docs/reproduce.md](../docs/reproduce.md)。
