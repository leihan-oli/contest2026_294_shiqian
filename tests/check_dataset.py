#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Check the eye dataset matches what the documents claim.

The counts matter: the report quotes "991 images, 193/208/196/186/208", and the
class order (closed/open/left/right/other -> 0..4) is what the board assumes.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
DATA = os.path.join(REPO, "models/dataset")

EXPECTED = {"open": 193, "close": 208, "left": 196, "right": 186,
            "other": 208}
PREFIX = {"open": "open_", "close": "closed_", "left": "left_",
          "right": "right_", "other": "other_"}

failures = []
total = 0

print("== dataset ==")
for cls, want in sorted(EXPECTED.items()):
  folder = os.path.join(DATA, cls)
  if not os.path.isdir(folder):
    failures.append("%s/ missing" % cls)
    print("  ✗ %-6s missing" % cls)
    continue
  files = [f for f in os.listdir(folder) if f.lower().endswith(".bmp")]
  total += len(files)
  wrong = [f for f in files if not f.startswith(PREFIX[cls])]
  if len(files) != want:
    failures.append("%s has %d images, expected %d" % (cls, len(files), want))
  if wrong:
    failures.append("%s has %d files with unexpected prefix (e.g. %s)" %
                    (cls, len(wrong), wrong[0]))
  mark = "✓" if len(files) == want and not wrong else "✗"
  print("  %s %-6s %4d 张（期望 %4d）" % (mark, cls, len(files), want))

meta = os.path.join(DATA, "meta.txt")
if os.path.exists(meta):
  lines = [l for l in open(meta, encoding="utf-8", errors="replace")
           if l.strip()]
  print("  ✓ meta.txt %d 条（采集时的几何记录）" % len(lines))
else:
  failures.append("meta.txt missing")
  print("  ✗ meta.txt missing")

print()
print("  合计 %d 张" % total)
if failures:
  print("FAIL: %d problem(s)" % len(failures))
  for item in failures:
    print("  - %s" % item)
  sys.exit(1)
print("PASS: dataset matches the documented counts")
