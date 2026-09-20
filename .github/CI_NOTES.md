# GitHub 配置

本目录目前**没有** CI 工作流，这是有意的：

- 构建需要完整 openvela 工作区 + 专属板级包 + ARM 工具链（数 GB），
  还要 ST Edge AI 产出的模型产物才能链出固件；在 GitHub 免费 runner 上
  重建这套环境的成本远高于它带来的保护。
- 可以离线跑、且足够有意义的检查已经放到了 `tests/`：
  ```bash
  bash tests/run_all.sh          # 产物 / 数据集 / 日志三项检查
  bash scripts/check_boundaries.sh   # 编译产物边界（需先编译）
  ```

如果后续要加 CI，建议的切入点是最小化的静态检查（`checkpatch.sh`、
`sha256sum -c CHECKSUMS.sha256`），而不是完整编译。
