#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Render contest session JSONL into human-readable formats.
#
# Usage:
#   render-log.py <jsonl-path>                    # Terminal output (default, color)
#   render-log.py <jsonl-path> --format html > out.html
#   render-log.py <jsonl-path> --format md
#   render-log.py <member-dir>                    # Render all sessions under a member dir
#
# Output formats: terminal (default) / html / md

from __future__ import annotations

import argparse
import html
import json
import sys
from datetime import datetime, timezone
from pathlib import Path


ROLE_BADGE = {
    "user": ("👤", "USER", "\033[1;36m"),
    "assistant": ("🤖", "ASSISTANT", "\033[1;32m"),
    "tool": ("🔧", "TOOL", "\033[1;33m"),
    "system": ("⚙️ ", "SYSTEM", "\033[1;90m"),
}
RESET = "\033[0m"
DIM = "\033[2m"
BOLD = "\033[1m"


def fmt_time(ts: str) -> str:
    try:
        dt = datetime.fromisoformat(ts.replace("Z", "+00:00"))
        local = dt.astimezone()
        return local.strftime("%H:%M:%S")
    except Exception:
        return ts


def truncate(s: str, n: int = 200) -> str:
    if len(s) <= n:
        return s
    return s[:n] + f" ... [+{len(s) - n} chars]"


def fmt_tool_io(value, max_str: int = 500, indent: int = 4) -> str:
    if value is None:
        return "(empty)"
    if isinstance(value, str):
        return truncate(value, max_str)
    try:
        formatted = json.dumps(value, ensure_ascii=False, indent=2)
        return "\n".join(" " * indent + line for line in formatted.split("\n"))
    except Exception:
        return repr(value)


def render_terminal(events: list[dict], use_color: bool = True) -> list[str]:
    lines = []
    if not events:
        lines.append("(empty)")
        return lines

    head = events[0]
    sep = "═" * 76
    lines.append(sep)
    lines.append(f"  Session: {head.get('session_id', '?')}")
    lines.append(f"  Tool:    {head.get('tool', '?')}")
    lines.append(f"  Team:    {head.get('team_id', '?')}")
    lines.append(f"  Member:  {head.get('github_login', '?')}")
    lines.append(f"  Events:  {len(events)}")
    lines.append(sep)
    lines.append("")

    total_tokens_in = 0
    total_tokens_out = 0

    for ev in events:
        role = ev.get("role", "?")
        emoji, label, color = ROLE_BADGE.get(role, ("·", role.upper(), ""))
        time = fmt_time(ev.get("ts", ""))
        seq = ev.get("seq", "?")

        if use_color:
            header = f"{color}{emoji} [{label}]{RESET} {DIM}seq={seq}  {time}{RESET}"
        else:
            header = f"{emoji} [{label}] seq={seq}  {time}"

        meta_bits = []
        if ev.get("model"):
            meta_bits.append(f"model={ev['model']}")
        if ev.get("tokens_in") is not None:
            total_tokens_in += ev["tokens_in"]
            meta_bits.append(f"in={ev['tokens_in']}")
        if ev.get("tokens_out") is not None:
            total_tokens_out += ev["tokens_out"]
            meta_bits.append(f"out={ev['tokens_out']}")
        if ev.get("redacted_count"):
            meta_bits.append(f"REDACTED×{ev['redacted_count']}")
        if meta_bits:
            if use_color:
                header += f"  {DIM}({', '.join(meta_bits)}){RESET}"
            else:
                header += f"  ({', '.join(meta_bits)})"

        lines.append(header)
        lines.append("─" * 76)

        if role == "tool":
            tool_name = ev.get("tool_name", "?")
            tool_call_id = ev.get("tool_call_id", "")
            is_result = tool_name == "<result>"
            err_marker = "  ❌ ERROR" if ev.get("is_error") else ""

            if is_result:
                lines.append(f"  ⤷  Tool result for: {tool_call_id[:20]}{err_marker}")
                lines.append(f"  Output:")
                lines.append(fmt_tool_io(ev.get("output"), max_str=800))
            else:
                lines.append(f"  → Tool call: {tool_name}  [{tool_call_id[:20]}]")
                lines.append(f"  Input:")
                lines.append(fmt_tool_io(ev.get("input"), max_str=400))
                if ev.get("output") is not None:
                    lines.append(f"  Output:")
                    lines.append(fmt_tool_io(ev.get("output"), max_str=400))
        else:
            if ev.get("thinking"):
                if use_color:
                    lines.append(f"  {DIM}💭 thinking:{RESET}")
                else:
                    lines.append(f"  💭 thinking:")
                for line in ev["thinking"].split("\n"):
                    if use_color:
                        lines.append(f"    {DIM}{line}{RESET}")
                    else:
                        lines.append(f"    {line}")
            if ev.get("text"):
                for line in ev["text"].split("\n"):
                    lines.append(f"  {line}")

        if ev.get("files_touched"):
            files_str = ", ".join(ev["files_touched"])
            if use_color:
                lines.append(f"  {DIM}📂 files: {files_str}{RESET}")
            else:
                lines.append(f"  📂 files: {files_str}")

        lines.append("")

    if total_tokens_in or total_tokens_out:
        lines.append(sep)
        lines.append(f"  Total tokens: in={total_tokens_in}  out={total_tokens_out}  total={total_tokens_in + total_tokens_out}")
        lines.append(sep)

    return lines


def render_markdown(events: list[dict]) -> list[str]:
    lines = []
    if not events:
        return ["_(empty session)_"]

    head = events[0]
    lines.append(f"# Session `{head.get('session_id', '?')}`")
    lines.append("")
    lines.append(f"- **Tool**: `{head.get('tool', '?')}`")
    lines.append(f"- **Team**: `{head.get('team_id', '?')}`")
    lines.append(f"- **Member**: `{head.get('github_login', '?')}`")
    lines.append(f"- **Events**: {len(events)}")
    lines.append("")
    lines.append("---")
    lines.append("")

    role_label = {"user": "👤 User", "assistant": "🤖 Assistant",
                  "tool": "🔧 Tool", "system": "⚙️ System"}

    for ev in events:
        role = ev.get("role", "?")
        label = role_label.get(role, role.upper())
        time = fmt_time(ev.get("ts", ""))
        seq = ev.get("seq", "?")

        meta = []
        if ev.get("model"):
            meta.append(f"model: `{ev['model']}`")
        if ev.get("tokens_in") is not None:
            meta.append(f"tokens: {ev.get('tokens_in')}/{ev.get('tokens_out', '?')}")
        if ev.get("redacted_count"):
            meta.append(f"⚠️ redacted×{ev['redacted_count']}")
        meta_str = " · ".join(meta)
        suffix = f"  _{meta_str}_" if meta_str else ""

        lines.append(f"## {label} · seq={seq} · {time}{suffix}")
        lines.append("")

        if role == "tool":
            name = ev.get("tool_name", "?")
            call_id = ev.get("tool_call_id", "")
            if name == "<result>":
                lines.append(f"**↩ Result for** `{call_id}`")
                if ev.get("is_error"):
                    lines.append("")
                    lines.append("> ❌ Tool returned an error.")
                lines.append("")
                lines.append("```")
                output = ev.get("output", "")
                lines.append(str(output) if not isinstance(output, str) else output)
                lines.append("```")
            else:
                lines.append(f"**🔧 Tool call**: `{name}`  ·  id: `{call_id}`")
                lines.append("")
                lines.append("**Input**:")
                lines.append("```json")
                lines.append(json.dumps(ev.get("input"), ensure_ascii=False, indent=2))
                lines.append("```")
                if ev.get("output"):
                    lines.append("**Output**:")
                    lines.append("```")
                    lines.append(str(ev["output"]))
                    lines.append("```")
        else:
            if ev.get("thinking"):
                lines.append("> 💭 **Thinking**:")
                for line in ev["thinking"].split("\n"):
                    lines.append(f"> {line}")
                lines.append("")
            if ev.get("text"):
                lines.append(ev["text"])

        if ev.get("files_touched"):
            lines.append("")
            lines.append(f"📂 _files touched_: {', '.join(f'`{f}`' for f in ev['files_touched'])}")

        lines.append("")
        lines.append("---")
        lines.append("")

    return lines


def render_html(events: list[dict]) -> str:
    if not events:
        return "<p>(empty)</p>"

    head = events[0]
    sid = html.escape(head.get("session_id", "?"))
    tool = html.escape(head.get("tool", "?"))
    team = html.escape(head.get("team_id", "?"))
    member = html.escape(head.get("github_login", "?"))

    parts = [f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<title>Session {sid}</title>
<style>
  body {{ font-family: -apple-system, "PingFang SC", "Microsoft YaHei", sans-serif;
         max-width: 980px; margin: 24px auto; padding: 0 24px; color: #24292e;
         line-height: 1.6; }}
  .meta {{ background: #f6f8fa; padding: 16px; border-radius: 6px;
          font-size: 14px; color: #586069; }}
  .meta b {{ color: #24292e; }}
  .ev {{ margin: 24px 0; padding: 16px; border-radius: 8px;
        border-left: 4px solid; }}
  .ev.user      {{ background: #e1f5fe; border-color: #0277bd; }}
  .ev.assistant {{ background: #f1f8e9; border-color: #558b2f; }}
  .ev.tool      {{ background: #fff3e0; border-color: #ef6c00; }}
  .ev.system    {{ background: #f5f5f5; border-color: #757575; }}
  .ev .head {{ font-weight: 600; margin-bottom: 8px; }}
  .ev .head .seq {{ color: #999; font-weight: normal; font-size: 13px; }}
  .ev .head .meta {{ color: #666; font-weight: normal; font-size: 12px;
                    background: none; padding: 0; display: inline; }}
  .think {{ background: rgba(0,0,0,0.05); padding: 8px 12px; border-radius: 4px;
           margin: 8px 0; font-size: 14px; color: #555;
           border-left: 3px solid #b39ddb; }}
  .think::before {{ content: "💭 thinking: "; color: #6a1b9a; font-weight: 600; }}
  pre {{ background: #fff; border: 1px solid #ddd; padding: 10px; border-radius: 4px;
        white-space: pre-wrap; word-break: break-all; max-height: 400px;
        overflow: auto; font-size: 13px; }}
  .text {{ white-space: pre-wrap; }}
  code {{ background: #f6f8fa; padding: 2px 6px; border-radius: 3px;
         font-family: ui-monospace, "SF Mono", Menlo, monospace; font-size: 90%; }}
  .files {{ font-size: 12px; color: #666; margin-top: 8px; }}
  .total {{ margin-top: 32px; padding: 12px; background: #fff8e1;
           border-radius: 6px; font-size: 14px; }}
</style></head><body>
<h1>Session <code>{sid}</code></h1>
<div class="meta">
  <b>Tool</b>: {tool} &nbsp;·&nbsp;
  <b>Team</b>: {team} &nbsp;·&nbsp;
  <b>Member</b>: {member} &nbsp;·&nbsp;
  <b>Events</b>: {len(events)}
</div>
"""]

    total_in = 0
    total_out = 0

    for ev in events:
        role = ev.get("role", "?")
        seq = ev.get("seq", "?")
        time = html.escape(fmt_time(ev.get("ts", "")))

        meta = []
        if ev.get("model"):
            meta.append(f"model={html.escape(ev['model'])}")
        if ev.get("tokens_in") is not None:
            total_in += ev["tokens_in"]
            meta.append(f"in={ev['tokens_in']}")
        if ev.get("tokens_out") is not None:
            total_out += ev["tokens_out"]
            meta.append(f"out={ev['tokens_out']}")
        if ev.get("redacted_count"):
            meta.append(f"⚠️ REDACTED×{ev['redacted_count']}")

        meta_str = ""
        if meta:
            meta_str = f"<span class='meta'> &nbsp; ({' · '.join(meta)})</span>"

        body_parts = []
        if role == "tool":
            name = html.escape(ev.get("tool_name", "?"))
            call_id = html.escape((ev.get("tool_call_id") or "")[:32])
            if name == "<result>":
                err = " <b style='color:#c62828'>❌ ERROR</b>" if ev.get("is_error") else ""
                body_parts.append(f"<div><b>↩ Result</b> for <code>{call_id}</code>{err}</div>")
                output = ev.get("output")
                output_str = output if isinstance(output, str) else json.dumps(output, ensure_ascii=False, indent=2)
                body_parts.append(f"<pre>{html.escape(str(output_str))}</pre>")
            else:
                body_parts.append(f"<div><b>🔧 Tool call</b>: <code>{name}</code> &nbsp; <code>{call_id}</code></div>")
                input_val = ev.get("input")
                input_str = input_val if isinstance(input_val, str) else json.dumps(input_val, ensure_ascii=False, indent=2)
                body_parts.append(f"<div><b>Input</b>:</div><pre>{html.escape(str(input_str))}</pre>")
                if ev.get("output"):
                    output = ev.get("output")
                    output_str = output if isinstance(output, str) else json.dumps(output, ensure_ascii=False, indent=2)
                    body_parts.append(f"<div><b>Output</b>:</div><pre>{html.escape(str(output_str))}</pre>")
        else:
            if ev.get("thinking"):
                body_parts.append(f"<div class='think'>{html.escape(ev['thinking'])}</div>")
            if ev.get("text"):
                body_parts.append(f"<div class='text'>{html.escape(ev['text'])}</div>")

        if ev.get("files_touched"):
            files_html = ", ".join(f"<code>{html.escape(f)}</code>" for f in ev["files_touched"])
            body_parts.append(f"<div class='files'>📂 files: {files_html}</div>")

        role_emoji = {"user": "👤 USER", "assistant": "🤖 ASSISTANT",
                      "tool": "🔧 TOOL", "system": "⚙️ SYSTEM"}.get(role, role.upper())

        parts.append(f"""<div class='ev {role}'>
<div class='head'>{role_emoji} <span class='seq'>seq={seq} · {time}</span>{meta_str}</div>
{''.join(body_parts)}
</div>""")

    if total_in or total_out:
        parts.append(f"""<div class='total'>
  <b>Total tokens</b>: in={total_in} · out={total_out} · total={total_in + total_out}
</div>""")

    parts.append("</body></html>")
    return "\n".join(parts)


def load_jsonl(path: Path) -> list[dict]:
    events = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            events.append(json.loads(line))
        except json.JSONDecodeError as e:
            print(f"WARN: bad JSON line in {path}: {e}", file=sys.stderr)
    return events


def find_jsonl_files(target: Path) -> list[Path]:
    if target.is_file() and target.suffix == ".jsonl":
        return [target]
    if target.is_dir():
        return sorted(target.rglob("*.jsonl"))
    return []


def main() -> int:
    parser = argparse.ArgumentParser(description="Render contest session JSONL into human-readable formats")
    parser.add_argument("path", help="Path to .jsonl file OR member dir")
    parser.add_argument("--format", choices=["terminal", "html", "md"], default="terminal")
    parser.add_argument("--no-color", action="store_true", help="disable terminal colors")
    parser.add_argument("--out", help="output file path (default: stdout)")
    args = parser.parse_args()

    target = Path(args.path).resolve()
    files = find_jsonl_files(target)
    if not files:
        print(f"ERROR: no .jsonl files found at {target}", file=sys.stderr)
        return 1

    output_chunks = []

    for jsonl_path in files:
        events = load_jsonl(jsonl_path)
        if args.format == "terminal":
            use_color = not args.no_color and (args.out is None) and sys.stdout.isatty()
            output_chunks.extend(render_terminal(events, use_color=use_color))
            output_chunks.append("")
        elif args.format == "md":
            output_chunks.extend(render_markdown(events))
            output_chunks.append("\n---\n")
        elif args.format == "html":
            output_chunks.append(render_html(events))

    text = "\n".join(output_chunks)

    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
        print(f"Wrote {Path(args.out).resolve()}", file=sys.stderr)
    else:
        print(text)

    return 0


if __name__ == "__main__":
    sys.exit(main())
