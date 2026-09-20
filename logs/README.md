# logs/ — AI Coding 日志目录

存放开发过程中与 AI 工具的对话日志，与作品代码一并提交。

## 目录结构

```text
logs/
└── leihan-oli/                  # GitHub 用户名，一人一目录
    ├── manifest.json            # 会话清单
    └── <date>/                  # 日期 YYYY-MM-DD
        └── <tool>__<sid>.jsonl  # 一个会话一个文件（工具名与 session id 用 __ 连接）
```

- `<tool>`：`claude-code` / `opencode` / `codex` / `kiro`
- 每个 `.jsonl` 每行一个事件，只提交 JSONL 本身

> 本仓已按上述规范填入了**真实日志**（不再是脚手架示例），
> 会话来源、筛选口径与限制见 [README-leihan-oli.md](README-leihan-oli.md)。

## 本组情况

日志由 **VS Code + GitHub Copilot Chat** 的会话转换而来（采集插件只 hook
`claude-code` / `opencode` / `codex`，不覆盖 Copilot），转换工具为仓内
`tools/export_copilot_logs.py`，转换细节与限制见 [README-leihan-oli.md](README-leihan-oli.md)。

| 项 | 值 |
|---|---|
| 会话数 | 14（2026-08-02 ~ 2026-09-18） |
| 事件数 | 38,296 |
| 校验 | `validate-log.py` → **ALL OK** |

## 工具（来自官方 `.claude` 工具仓）

```bash
# 合规校验（seq 单调性、跨字段一致性、manifest 对账）
python3 tools/validate-log.py logs/

# 终端预览（彩色）
python3 tools/render-log.py logs/leihan-oli/

# 单个会话 → HTML 评分报告
python3 tools/render-log.py logs/leihan-oli/<date>/claude-code__<sid>.jsonl \
        --format html --out report.html
```

> 两个工具需要 **Python ≥ 3.9**（本仓用 anaconda3 的 python3.10 验证通过）。

规范约定：会话日志写入 `logs/` 后由选手自行 `git push`。
导出与提交的完整步骤、字段定义见
[《AI Coding 日志归集与提交手册》](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/ai_coding_log_guide.md)。
