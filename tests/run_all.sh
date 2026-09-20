#!/usr/bin/env bash
# Offline checks for everything that can be verified without the board.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PY="${PYTHON:-$(command -v python3)}"
rc=0

for check in check_artifacts check_dataset check_logs; do
  echo "──────────────────────────────────────────────────────────────"
  if ! "$PY" "$HERE/$check.py"; then
    rc=1
  fi
  echo
done

# 核心树改动：补丁 ↔ core-tree-overlay ↔ 当前工作区 三者一致（需要 openvela 工作区）
echo "──────────────────────────────────────────────────────────────"
if bash "$HERE/../scripts/verify-core-tree.sh" > /tmp/core-tree-check.log 2>&1; then
  tail -3 /tmp/core-tree-check.log
else
  cat /tmp/core-tree-check.log
  echo "CORE TREE CHECK FAILED —— 补丁与 overlay 不一致"
  rc=1
fi
echo

echo "══════════════════════════════════════════════════════════════"
if [ "$rc" -eq 0 ]; then
  echo "ALL CHECKS PASSED"
else
  echo "SOME CHECKS FAILED"
fi
exit "$rc"
