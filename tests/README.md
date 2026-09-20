# tests — 离线自检

不需要开发板、不需要工具链，只要一个 `python3`。三条检查对应文档里的三类声明：

| 检查 | 验证什么 |
|------|----------|
| `check_artifacts.py` | 4 个烧录镜像：存在、**Intel HEX 行校验和正确**、装载地址与烧录表一致、`nuttx.bin ≤ 1 MiB` |
| `check_dataset.py` | 数据集与文档一致：991 张、五类计数（193/208/196/186/208）、文件名前缀、`meta.txt` 存在 |
| `check_logs.py` | AI Coding 日志：优先调用官方 `tools/validate-log.py`（需 python ≥ 3.9）；没有新解释器时退回结构检查（字段齐全 / `seq` 单调唯一 / 与 manifest 计数一致） |

```bash
bash tests/run_all.sh           # 一次跑完三条
python3 tests/check_artifacts.py
```

`check_artifacts.py` 之所以要验证**行校验和**而不只看文件大小：
镜像被截断或传输损坏时，板子不会报错，只会静默不启动。

> 板端功能（识别、界面、音调）无法离线验证，验收步骤见
> [docs/reproduce.md](../docs/reproduce.md) 第 5 节。
