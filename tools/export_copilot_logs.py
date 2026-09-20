#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Turn VS Code Copilot sessions into contest logs.

The contest collector only hooks claude-code / opencode / codex, so the Copilot
sessions this project was built with were never captured.  What VS Code does
keep is enough to rebuild them:

  transcripts/<sid>.jsonl          the conversation, in order, with each tool
                                   call's arguments and the model's reasoning
  chatSessions/<sid>.jsonl         a journal whose result records carry the
                                   tool outputs, the token counts and the model
  chat-session-resources/<sid>/...  spilled tool outputs for the largest results

Output is one JSONL per session under logs/<login>/<date>/, plus the manifest
the validator cross-checks against.

Two details are worth being explicit about, because they are visible to judges:

  * the source tool is GitHub Copilot, which the collector does not know about,
    so the events carry the closest supported label and say so in the manifest
  * tool outputs only exist for tools whose result VS Code serialised; when one
    is missing the event says so rather than pretending the call returned
    nothing
"""
import argparse
import glob
import hashlib
import json
import os
import re
import sys
from datetime import datetime, timezone

TEAM_ID = "contest2026_294_shiqian"
GITHUB_LOGIN = "leihan-oli"
TOOL = "claude-code"          # closest supported label; see README next to logs
REPO = "/home/leihann/openvela/contest2026_294_shiqian"
COLLECTOR = "/home/leihann/openvela/.claude/skills/contest-log-collector"

# Words only this project uses.  A session is ours when it has a few of them.
STRONG = ["atk-dnn647", "atk_dnn647", "stm32n647", "stm32n6", "eye_cam",
          "eye-cam", "contest2026", "fsbl", "risaf", "dcmipp", "imx335",
          "es8388", "aton", "xspi", "lvgl", "nuttx", "openvela", "eye_control"]

# Sessions that mention the stack but belong to something else.
EXCLUDE = {
    "306652e4-d588-4891-b508-8e58b05b2999",   # empty
    "181e52a1-0513-4a03-88d0-2a4d15d63338",   # empty
    "d6fd1ae0-286b-41b5-8bd4-81b46396be57",   # Natively / Whisper STT
}

REDACTIONS = [
    (re.compile(r"sk-[A-Za-z0-9]{16,}"), "<redacted:api-key>"),
    (re.compile(r"ghp_[A-Za-z0-9]{20,}"), "<redacted:github-token>"),
    (re.compile(r"(Bearer\s+)[A-Za-z0-9._\-]{16,}"), r"\1<redacted:token>"),
]

TOOL_NAME_MAP = [
    ("copilot_createFile", "create_file"),
    ("copilot_replaceString", "replace_string_in_file"),
    ("copilot_multiReplaceString", "multi_replace_string_in_file"),
    ("copilot_insertEdit", "insert_edit_into_file"),
    ("copilot_runInTerminal", "run_in_terminal"),
    ("copilot_readFile", "read_file"),
    ("copilot_findTextInFiles", "grep_search"),
    ("copilot_findFiles", "file_search"),
    ("copilot_listDirectory", "list_dir"),
    ("copilot_getErrors", "get_errors"),
    ("copilot_getChangedFiles", "get_changed_files"),
    ("copilot_searchCodebase", "semantic_search"),
    ("copilot_searchWorkspaceSymbols", "workspace_symbols"),
    ("copilot_fetchWebPage", "fetch_webpage"),
    ("copilot_applyPatch", "apply_patch"),
    ("copilot_runNotebookCell", "run_notebook_cell"),
    ("copilot_editNotebook", "edit_notebook_file"),
    ("copilot_getNotebookSummary", "copilot_getNotebookSummary"),
]


def short_tool(name):
  for prefix, friendly in TOOL_NAME_MAP:
    if name.startswith(prefix):
      return friendly
  return name


def iso(ts):
  """Normalise a transcript timestamp to ISO 8601 UTC."""
  if not ts:
    return None
  ts = ts.strip()
  if ts.endswith("Z"):
    return ts
  if re.search(r"[+-]\d{2}:\d{2}$", ts):
    return ts
  if re.match(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}", ts):
    return ts + "Z"
  return ts


def redact(text, stats):
  if not text:
    return text, 0
  count = 0
  for pattern, repl in REDACTIONS:
    text, n = pattern.subn(repl, text)
    count += n
  stats["redacted"] += count
  return text, count


# --------------------------------------------------------------------------
# the journal: tool outputs, tokens, model
# --------------------------------------------------------------------------

def load_journal(path, session_id):
  """Return {tool_call_id: output_text} plus session-level token/model facts."""
  facts = {"outputs": {}, "prompt_tokens": 0, "output_tokens": 0, "model": None,
           "agent": None, "summaries": []}
  if not path or not os.path.exists(path):
    return facts

  def walk(node):
    """Collect the text out of one toolCallResults payload."""
    if isinstance(node, dict):
      for field in ("value", "text"):
        if isinstance(node.get(field), str) and node[field].strip():
          return node[field]
      parts = [walk(v) for v in node.values()]
      return "\n".join(p for p in parts if p)
    if isinstance(node, list):
      parts = [walk(v) for v in node]
      return "\n".join(p for p in parts if p)
    if isinstance(node, str):
      return node
    return None

  with open(path, encoding="utf-8", errors="replace") as handle:
    for line in handle:
      line = line.strip()
      if not line:
        continue
      try:
        record = json.loads(line)
      except Exception:
        continue
      kind, key, value = record.get("kind"), record.get("k"), record.get("v")
      if kind == 1 and isinstance(value, dict):
        md = value.get("metadata") or {}
        results = md.get("toolCallResults") or {}
        for call_id, payload in results.items():
          text = walk(payload)
          if text:
            facts["outputs"].setdefault(call_id.split("__vscode-")[0], text)
        if md.get("promptTokens"):
          facts["prompt_tokens"] += md["promptTokens"]
        if md.get("outputTokens"):
          facts["output_tokens"] += md["outputTokens"]
        if md.get("resolvedModel") and not facts["model"]:
          facts["model"] = md["resolvedModel"]
        if md.get("agentId") and not facts["agent"]:
          facts["agent"] = md["agentId"]
        for summary in md.get("summaries") or []:
          if isinstance(summary, dict) and summary.get("text"):
            if summary["text"] not in facts["summaries"]:
              facts["summaries"].append(summary["text"])
  return facts


def spilled_outputs(ws, session_id):
  """Tool outputs VS Code spilled to disk for the largest results."""
  out = {}
  base = os.path.join(ws, "GitHub.copilot-chat", "chat-session-resources",
                      session_id)
  if not os.path.isdir(base):
    return out
  for path in glob.glob(os.path.join(base, "*", "content.txt")):
    name = os.path.basename(os.path.dirname(path))
    call_id = name.split("__vscode-")[0]
    try:
      out[call_id] = open(path, encoding="utf-8", errors="replace").read()
    except Exception:
      pass
  return out


# --------------------------------------------------------------------------
# the conversation
# --------------------------------------------------------------------------

def convert(transcript, journal_path, ws, session_id):
  stats = {"redacted": 0, "tools": 0, "outputs": 0, "missing_outputs": 0}
  journal_candidates = [journal_path]
  journal_candidates += glob.glob(
      "/home/leihann/SoftwarePackage/session_backup_2026-09-19_c9ba8b25/"
      "chatSessions_%s.jsonl" % session_id[:8])
  usable = [p for p in journal_candidates if p and os.path.exists(p)]
  if usable:
    # the live file can be a rebuilt shell; take the richest one
    journal_path = max(usable, key=os.path.getsize)
  facts = load_journal(journal_path, session_id)
  spilled = spilled_outputs(ws, session_id)

  # toolCallId -> (success, ts) from the transcript's completion records
  completions = {}
  records = []
  with open(transcript, encoding="utf-8", errors="replace") as handle:
    for line in handle:
      line = line.strip()
      if not line:
        continue
      try:
        records.append(json.loads(line))
      except Exception:
        continue

  for record in records:
    if record.get("type") == "tool.execution_complete":
      data = record.get("data") or {}
      completions[data.get("toolCallId")] = {
          "success": data.get("success"),
          "ts": iso(record.get("timestamp")),
      }

  events = []
  pending = {}                     # tool_call_id -> event index

  def emit(ts, role, **fields):
    event = {
        "schema_version": "1.0",
        "session_id": session_id,
        "team_id": TEAM_ID,
        "github_login": GITHUB_LOGIN,
        "tool": TOOL,
        "ts": iso(ts) or datetime.now(timezone.utc).strftime(
            "%Y-%m-%dT%H:%M:%S.000Z"),
        "role": role,
        "seq": len(events),
    }
    event.update({k: v for k, v in fields.items() if v not in (None, "",
                                                               [], {})})
    events.append(event)
    return len(events) - 1

  for record in records:
    kind = record.get("type")
    data = record.get("data") or {}
    ts = record.get("timestamp")

    if kind == "session.start":
      emit(ts, "system",
           text="session start",
           metadata={k: data.get(k) for k in
                     ("sessionId", "producer", "copilotVersion",
                      "vscodeVersion", "startTime") if data.get(k)})

    elif kind == "user.message":
      text, _ = redact(data.get("content") or "", stats)
      attachments = [a.get("filePath") or a.get("id") or "?"
                     for a in (data.get("attachments") or [])
                     if isinstance(a, dict)]
      emit(ts, "user", text=text,
           metadata={"attachments": attachments} if attachments else None)

    elif kind == "assistant.message":
      thinking, _ = redact(data.get("reasoningText") or "", stats)
      if thinking:
        emit(ts, "assistant", thinking=thinking)
      text, _ = redact(data.get("content") or "", stats)
      if text:
        emit(ts, "assistant", text=text)
      for request in (data.get("toolRequests") or []):
        args = request.get("arguments")
        if isinstance(args, str):
          try:
            args = json.loads(args)
          except Exception:
            args = {"raw": args}
        if isinstance(args, dict):
          try:
            args = json.loads(redact(json.dumps(args, ensure_ascii=False),
                                     stats)[0])
          except Exception:
            pass
        call_id = request.get("toolCallId") or ""
        files = []
        if isinstance(args, dict):
          for k in ("filePath", "path", "file", "notebookPath"):
            if isinstance(args.get(k), str):
              files.append(args[k])
          for k in ("filePaths", "files"):
            if isinstance(args.get(k), list):
              files.extend(str(x) for x in args[k])
        idx = emit(ts, "tool",
                   tool_name=short_tool(request.get("name") or "?"),
                   tool_call_id=call_id,
                   input=args,
                   files_touched=files)
        pending[call_id] = idx
        stats["tools"] += 1

  # Attach the outputs, now that every call is known.
  for call_id, idx in pending.items():
    event = events[idx]
    out = spilled.get(call_id) or facts["outputs"].get(call_id)
    if out:
      out, count = redact(out, stats)
      event["output"] = out
      stats["outputs"] += 1
    else:
      stats["missing_outputs"] += 1
    meta = {}
    done = completions.get(call_id)
    if done:
      if done.get("success") is not None:
        meta["success"] = done["success"]
    if not out:
      meta["output_unavailable"] = ("VS Code did not serialise this tool's "
                                   "result")
    if meta:
      event.setdefault("metadata", {}).update(meta)

  # A compaction is part of the session and explains any jump in the log.
  for i, summary in enumerate(facts["summaries"]):
    text, _ = redact(summary, stats)
    emit(records[-1].get("timestamp") if records else None, "system",
         text=text, metadata={"kind": "context-compaction", "index": i})

  return events, facts, stats


def discover():
  """Every session of this project, wherever VS Code keeps it."""
  found = []
  pattern = ("/home/leihann/.config/Code/User/workspaceStorage/*/"
             "GitHub.copilot-chat/transcripts/*.jsonl")
  for path in sorted(glob.glob(pattern)):
    session_id = os.path.basename(path)[:-len(".jsonl")]
    if session_id in EXCLUDE:
      continue
    text = open(path, encoding="utf-8", errors="replace").read()
    low = text.lower()
    hits = sum(low.count(w) for w in STRONG)
    if hits < 3:
      continue
    ws = path.split("/GitHub.copilot-chat/")[0]
    found.append({
        "session_id": session_id,
        "ws": ws,
        "transcript": path,
        "journal": os.path.join(ws, "chatSessions", session_id + ".jsonl"),
        "hits": hits,
    })
  return found


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("--dry-run", action="store_true")
  args = parser.parse_args()

  sessions = discover()
  print("%-38s %8s %7s %7s %9s %7s %9s %9s" %
        ("session", "events", "tools", "outputs", "bytes", "redact",
         "out_bytes", "biggest"))
  manifest_sessions = []
  total = 0
  for item in sessions:
    events, facts, stats = convert(item["transcript"], item["journal"],
                                   item["ws"], item["session_id"])
    body = "\n".join(json.dumps(e, ensure_ascii=False) for e in events) + "\n"
    total += len(body.encode("utf-8"))
    first = events[0]["ts"]
    last = events[-1]["ts"]
    date = first[:10]
    rel = "logs/%s/%s/%s__%s.jsonl" % (GITHUB_LOGIN, date, TOOL,
                                       item["session_id"])
    manifest_sessions.append({
        "session_id": item["session_id"],
        "tool": TOOL,
        "started_at": first,
        "last_event_at": last,
        "event_count": len(events),
        "file_path": rel,
        "collection_mode": "vscode_extension_partial",
        "data_completeness_warning": (
            "Converted from VS Code Copilot transcripts (the collector does "
            "not support Copilot). VS Code keeps full result metadata for "
            "only the most recent turns, so %d of %d tool calls carry an "
            "output; the rest are marked output_unavailable."
            % (stats["outputs"], stats["tools"])),
        "model": facts.get("model") or "github-copilot",
        "tokens_total": facts["prompt_tokens"] + facts["output_tokens"],
        "redacted_count_total": stats["redacted"],
        "health": "ok",
        "source": {
            "tool": "github-copilot",
            "agent": facts.get("agent"),
            "transcript": os.path.relpath(item["transcript"], "/"),
            "transcript_md5": hashlib.md5(
                open(item["transcript"], "rb").read()).hexdigest(),
            "journal_present": os.path.exists(item["journal"]),
        },
        "_body": None if args.dry_run else body,
    })
    out_bytes = sum(len(e.get("output", "") or "") for e in events)
    biggest = max([len(e.get("output", "") or "") for e in events] + [0])
    print("%-38s %8d %7d %7d %9d %7d %9d %9d" %
          (item["session_id"][:36], len(events), stats["tools"],
           stats["outputs"], len(body.encode("utf-8")), stats["redacted"],
           out_bytes, biggest))

  print("%-38s %8s %7s %7s %9d" % ("TOTAL", "", "", "", total))

  if args.dry_run:
    print("\n(dry run - nothing written)")
    return

  import shutil
  logs_dir = os.path.join(REPO, "logs", GITHUB_LOGIN)
  for item in manifest_sessions:
    path = os.path.join(REPO, item["file_path"])
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "w", encoding="utf-8").write(item["_body"])

  # the judge's tools in the repo come from the collector skill
  for name in ("render-log.py", "validate-log.py"):
    src = os.path.join(COLLECTOR, "tools", name)
    dst = os.path.join(REPO, "tools", name)
    if os.path.exists(src):
      shutil.copy2(src, dst)
  for name in ("event.schema.json", "manifest.schema.json"):
    src = os.path.join(COLLECTOR, "schema", name)
    dst = os.path.join(REPO, "schema", name)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if os.path.exists(src):
      shutil.copy2(src, dst)

  manifest = {
      "schema_version": "1.0",
      "team_id": TEAM_ID,
      "github_login": GITHUB_LOGIN,
      "generator": "copilot-transcript-converter@1.0",
      "updated_at": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
      "sessions": [{k: v for k, v in s.items() if not k.startswith("_")}
                   for s in manifest_sessions],
  }
  path = os.path.join(logs_dir, "manifest.json")
  open(path, "w", encoding="utf-8").write(
      json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
  print("\nmanifest: %s (%d sessions)" % (path, len(manifest_sessions)))


if __name__ == "__main__":
  main()
