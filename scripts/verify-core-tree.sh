#!/bin/bash
# ============================================================================
# verify-core-tree.sh — 离线验证「补丁」与「overlay」等价、且与当前树一致
#
# 对 4 个仓库各做三步：
#   1. 把仓内（已应用补丁的）文件复制到临时目录 → git apply --reverse 补丁
#      ⇒ 应当得到上游基线内容（证明补丁确实描述了这些改动）
#   2. 再 git apply 补丁 ⇒ 得到「补丁应用后」的内容
#   3. 与 core-tree-overlay/ 里的对应文件逐字节比对（md5）
#
# 全部通过输出 ALL CORE TREE CHECKS PASSED；任何一步失败则给出具体文件。
# 用法：bash scripts/verify-core-tree.sh [工作区根目录]
# ============================================================================
set -u

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WS="${1:-$(cd "$REPO_DIR/.." && pwd)}"

REPOS=(  "nuttx"                  "apps"                  "vendor/openvela"           "apps/graphics/lvgl/lvgl" )
PATCHES=("arch/arm/stm32n6/nuttx-core-incremental.patch" \
         "app/apps-core-incremental.patch" \
         "board/vendor-openvela-core-incremental.patch" \
         "app/lvgl-core-incremental.patch" )
OVERLAYS=("nuttx" "apps" "vendor-openvela" "apps-lvgl" )

fail=0
total=0
for i in "${!REPOS[@]}"; do
  repo="$WS/${REPOS[$i]}"
  patch="$REPO_DIR/${PATCHES[$i]}"
  overlay="$REPO_DIR/core-tree-overlay/${OVERLAYS[$i]}"
  [ -d "$repo/.git" ] || { echo "[skip] ${REPOS[$i]} 不存在"; continue; }

  files=$(grep '^diff --git' "$patch" | sed 's|.*a/||; s| b/.*||')
  nfiles=$(echo "$files" | wc -l)
  tmp=$(mktemp -d)
  ok=1

  # 1) 仓内文件 → 临时目录
  while read -r f; do
    [ -n "$f" ] || continue
    mkdir -p "$tmp/$(dirname "$f")"
    cp "$repo/$f" "$tmp/$f"
  done <<< "$files"

  # 2) 反向应用 ⇒ 基线；正向应用 ⇒ 补丁后
  ( cd "$tmp" && git init -q && git add -A >/dev/null 2>&1 && git -c user.email=a@b -c user.name=a commit -qm base >/dev/null 2>&1 \
      && git apply --reverse "$patch" >/dev/null 2>&1 && git apply "$patch" >/dev/null 2>&1 ) || ok=0

  # 3) 与 overlay 比对
  while read -r f; do
    [ -n "$f" ] || continue
    total=$((total+1))
    m1=$(md5sum "$tmp/$f" 2>/dev/null | cut -d' ' -f1)
    m2=$(md5sum "$overlay/$f" 2>/dev/null | cut -d' ' -f1)
    m3=$(md5sum "$repo/$f" 2>/dev/null | cut -d' ' -f1)
    if [ "$m1" = "$m2" ] && [ "$m2" = "$m3" ]; then
      :
    else
      echo "  x  $f： patch-temp=${m1:0:8} overlay=${m2:0:8} repo=${m3:0:8}"
      fail=$((fail+1)); ok=0
    fi
  done <<< "$files"

  if [ $ok -eq 1 ]; then
    echo "  ok ${REPOS[$i]}：补丁 ↔ overlay ↔ 当前树 三者一致（$nfiles 个文件）"
  else
    echo "  FAIL ${REPOS[$i]}"
  fi
  rm -rf "$tmp"
done

echo
if [ $fail -eq 0 ]; then
  echo "ALL CORE TREE CHECKS PASSED（$total 个文件）"
  exit 0
else
  echo "CORE TREE CHECKS FAILED：$fail 个文件不一致"
  exit 1
fi
