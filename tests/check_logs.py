#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Validate the AI Coding logs, with or without the official tool.

The official validate-log.py needs python >= 3.9 (it uses str.removeprefix and
PEP 585 annotations).  This repo has to be checkable on machines that only have
3.8, so when no new enough interpreter is found we fall back to the structural
checks that matter most: schema fields, monotonic seq, and the manifest's
event_count.

Note: the schema's "tool" field is a closed enum (opencode/claude-code/codex/
kiro) which has no Copilot value, so the converted Copilot sessions carry
claude-code - see logs/README-leihan-oli.md.
"""
import glob
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
LOGS = os.path.join(REPO, "logs")
REQUIRED = ("schema_version", "session_id", "team_id", "github_login", "tool",
            "ts", "role", "seq")


def find_interpreter():
  """A python >= 3.9, preferring the one the caller pointed at.

  PYTHON is honoured first (set it when the default python3 is 3.8 but a newer
  one exists elsewhere, e.g. in a conda install), then the usual names on PATH.
  """
  candidates = []
  if os.environ.get("PYTHON"):
    candidates.append(os.environ["PYTHON"])
  for name in ("python3.13", "python3.12", "python3.11", "python3.10",
               "python3.9", "python3"):
    found = shutil.which(name)
    if found:
      candidates.append(found)
  for candidate in candidates:
    try:
      out = subprocess.run(
          [candidate, "-c", "import sys; print(sys.version_info >= (3, 9))"],
          capture_output=True, text=True, timeout=20)
    except Exception:
      continue
    if out.stdout.strip() == "True":
      return candidate
  return None


def structural_check():
  failures = []
  sessions = 0
  events = 0
  for member in sorted(os.listdir(LOGS)):
    member_dir = os.path.join(LOGS, member)
    manifest_path = os.path.join(member_dir, "manifest.json")
    if not os.path.isdir(member_dir) or not os.path.exists(manifest_path):
      continue
    manifest = json.load(open(manifest_path, encoding="utf-8"))
    declared = {}
    for session in manifest.get("sessions", []):
      declared[os.path.basename(session["file_path"])] = session
    for path in sorted(glob.glob(os.path.join(member_dir, "*", "*.jsonl"))):
      name = os.path.basename(path)
      session = declared.get(name)
      if session is None:
        failures.append("%s not declared in manifest" % name)
        continue
      seqs = []
      for number, line in enumerate(open(path, encoding="utf-8"), start=1):
        line = line.strip()
        if not line:
          continue
        try:
          event = json.loads(line)
        except Exception as exc:
          failures.append("%s:%d invalid JSON (%s)" % (name, number, exc))
          break
        missing = [k for k in REQUIRED if k not in event]
        if missing:
          failures.append("%s:%d missing %s" % (name, number, missing))
          break
        if event["session_id"] != session["session_id"]:
          failures.append("%s:%d session_id mismatch" % (name, number))
          break
        seqs.append(event["seq"])
      if len(seqs) != session.get("event_count"):
        failures.append("%s: %d events, manifest says %d" %
                        (name, len(seqs), session.get("event_count")))
      if seqs != sorted(set(seqs)):
        failures.append("%s: seq not strictly increasing/unique" % name)
      sessions += 1
      events += len(seqs)
  return sessions, events, failures


print("== AI coding logs ==")
interpreter = find_interpreter()
official = os.path.join(REPO, "tools/validate-log.py")

if interpreter and os.path.exists(official):
  print("  using official validator with %s" % interpreter)
  result = subprocess.run([interpreter, official, LOGS], capture_output=True,
                          text=True)
  tail = [l for l in (result.stdout + result.stderr).splitlines() if l.strip()]
  for line in tail[-6:]:
    print("  " + line)
  if result.returncode != 0:
    print("FAIL: official validator reported errors")
    sys.exit(1)
  print("PASS: official validate-log.py says ALL OK")
  sys.exit(0)

print("  no python >= 3.9 found (set PYTHON=... to point at one); "
        "running the structural checks instead")
sessions, events, failures = structural_check()
print("  %d sessions / %d events" % (sessions, events))
if failures:
  print("FAIL: %d problem(s)" % len(failures))
  for item in failures[:10]:
    print("  - %s" % item)
  sys.exit(1)
print("PASS: schema fields, seq monotonicity and manifest counts agree")
