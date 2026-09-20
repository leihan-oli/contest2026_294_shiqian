# AI Coding 日志说明（leihan-oli）

## 一句话

本项目的 AI Coding 会话记录在 **VS Code + GitHub Copilot Chat** 中产生。大赛采集插件只 hook
`claude-code` / `opencode` / `codex` 三种工具，**不覆盖 Copilot**，因此这些会话不会自动落到
`logs/`。本目录下的日志由 VS Code 自身留存的数据**离线转换**而成，转换过程无任何人工改写。

## 转换来源（两者互补，都来自 VS Code 自己的记录）

| 来源 | 位置 | 提供什么 |
|---|---|---|
| transcript | `.../GitHub.copilot-chat/transcripts/<session>.jsonl` | 会话顺序、用户消息原文、助手回复、模型的推理过程、**每次工具调用的参数** |
| chatSessions journal | `.../chatSessions/<session>.jsonl` | **工具输出**、`promptTokens`/`outputTokens`、`resolvedModel` |
| chat-session-resources | `.../GitHub.copilot-chat/chat-session-resources/<session>/…/content.txt` | 超大工具输出被 VS Code 落盘的那部分 |

转换脚本：`tools/export_copilot_logs.py`（`--dry-run` 只统计不写文件）。它按关键词
（`atk-dnn647` / `risaf` / `dcmipp` / `openvela` …）自动挑出属于本项目的会话，共 **14 个**。

**收录范围**：只收录与**本项目开发**相关的会话。面试准备、简历排版、
其他板卡调研等与本作品无关的会话不在此列（它们只是提到 openvela 而已）。
另有 6 个会话的 VS Code transcript **不包含用户提问原文**（工具侧未记录），
因此这些会话以助手推理与工具调用为主 —— 这是来源的限制，不是筛选造成的。

## 三点需要如实说明的限制

1. **`tool` 字段填的是 `claude-code`**。数据契约 `event.schema.json` 对该字段是**封闭枚举**
   （`opencode`/`claude-code`/`codex`/`kiro`），没有 Copilot 取值；填别的名字会被
   `validate-log.py` 判为不合规。真实来源记录在
   `manifest.json → sessions[].source.tool = "github-copilot"`，每条记录也带
   `collection_mode: vscode_extension_partial` 与 `data_completeness_warning`。

2. **工具输出只覆盖"最近若干轮"**。VS Code 只保留最近约 13 轮的完整结果元数据（更早的被
   淘汰以控制存储），所以大部分工具调用**没有 output**。这些记录带
   `metadata.output_unavailable = true` 显式标注，**没有伪造任何输出**。

3. **工具输出等于 VS Code 序列化的原文**，包括它自己写入的
   `[value truncated for persistence]` 之类的截断标记（如果有）。

## 可复核性

- 每个会话的源 transcript 路径与 **md5** 记录在 `manifest.json → sessions[].source`
- `seq` 从 0 连续递增、每文件内唯一（防篡改校验项）
- 官方校验：`python3 tools/validate-log.py logs/` → `ALL OK`（14 会话 / 38,296 事件）
- 渲染预览：`python3 tools/render-log.py logs/leihan-oli/`

> `validate-log.py` / `render-log.py` 需要 Python ≥ 3.9（用了 `str.removeprefix` 与
> PEP 585 注解）。本机若只有 3.8，用 `python3.10` 以上的解释器执行。
