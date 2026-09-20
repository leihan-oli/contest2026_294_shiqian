# schema — AI Coding 日志数据契约

`logs/` 下每条会话日志都必须满足这里的 schema（组委会数据契约，随仓保存以便离线校验）：

| 文件 | 约束对象 | 说明 |
|------|----------|------|
| `event.schema.json` | 每个 `.jsonl` 的**每一行**（一个事件） | 必填 `schema_version` / `session_id` / `team_id` / `github_login` / `tool` / `ts` / `role` / `seq`；`seq` 会话内递增，用于检测断档与篡改 |
| `manifest.schema.json` | `logs/<login>/manifest.json` | 会话清单：会话 id、时间范围、事件数、文件路径、来源工具与 md5 |

校验：

```bash
python3 tools/validate-log.py logs/     # 需要 Python ≥ 3.9
```

本仓日志的采集方式、字段映射与已知限制见 [logs/README-leihan-oli.md](../logs/README-leihan-oli.md)。
