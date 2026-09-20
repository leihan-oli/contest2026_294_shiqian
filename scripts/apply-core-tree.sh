#!/bin/bash
# ============================================================================
# apply-core-tree.sh — 把本仓的「核心树改动」应用到 openvela 工作区
#
# 芯片层 / 板级 / 应用目录之外的改动共 13 个文件，分布在 4 个仓库里，本脚本
# 一条命令全部处理：
#
#   nuttx            → arch/arm/stm32n6/nuttx-core-incremental.patch   （9 个文件）
#   apps             → app/apps-core-incremental.patch                 （2 个文件）
#   vendor/openvela  → board/vendor-openvela-core-incremental.patch    （1 个文件）
#   apps/graphics/lvgl/lvgl → app/lvgl-core-incremental.patch          （1 个文件）
#
# 用法（在 openvela 工作区根目录执行，也就是本仓的上一级）：
#     bash contest2026_294_shiqian/scripts/apply-core-tree.sh            # 打补丁
#     bash contest2026_294_shiqian/scripts/apply-core-tree.sh --overlay  # 直接覆盖
#     bash contest2026_294_shiqian/scripts/apply-core-tree.sh --check    # 只检查
#
# 两种方式结果完全一致：core-tree-overlay/ 里的文件就是补丁应用后的内容。
# 若上游分支已前移、补丁上下文对不上（git apply 报错），用 --overlay 即可，
# 它不依赖上游文件内容。
# ============================================================================
set -u

MODE="patch"
case "${1:-}" in
  --overlay) MODE="overlay" ;;
  --check)   MODE="check" ;;
  "")        ;;
  *) echo "用法: $0 [--overlay|--check] [工作区根目录]"; exit 2 ;;
esac

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"     # contest2026_294_shiqian
WS="${2:-$(cd "$REPO_DIR/.." && pwd)}"                          # openvela 工作区根

# 仓库 → 补丁 → overlay 子目录
REPOS=(  "nuttx"                  "apps"                  "vendor/openvela"           "apps/graphics/lvgl/lvgl" )
PATCHES=("arch/arm/stm32n6/nuttx-core-incremental.patch" \
         "app/apps-core-incremental.patch" \
         "board/vendor-openvela-core-incremental.patch" \
         "app/lvgl-core-incremental.patch" )
OVERLAYS=("nuttx" "apps" "vendor-openvela" "apps-lvgl" )

rc=0
for i in "${!REPOS[@]}"; do
  repo="$WS/${REPOS[$i]}"
  patch="$REPO_DIR/${PATCHES[$i]}"
  overlay="$REPO_DIR/core-tree-overlay/${OVERLAYS[$i]}"
  name="${REPOS[$i]}"

  if [ ! -d "$repo/.git" ]; then
    echo "[skip] $name 不存在（$repo）"; continue
  fi
  if [ ! -f "$patch" ]; then
    echo "[FAIL] 缺少补丁 $patch"; rc=1; continue
  fi

  n=$(grep -c '^diff --git' "$patch")
  case "$MODE" in
    check)
      if git -C "$repo" apply --check "$patch" 2>/dev/null; then
        echo "[ok]   $name：补丁可应用（$n 个文件）"
      elif git -C "$repo" apply --check --reverse "$patch" 2>/dev/null; then
        echo "[done] $name：改动已在（无需重复应用）"
      else
        echo "[warn] $name：补丁与当前树不匹配 → 需要 --overlay"; rc=1
      fi
      ;;
    overlay)
      if [ -d "$overlay" ]; then
        cp -r "$overlay/." "$repo/"
        echo "[ok]   $name：已用 overlay 覆盖 $n 个文件"
      else
        echo "[FAIL] 缺少 overlay：$overlay"; rc=1
      fi
      ;;
    patch)
      if git -C "$repo" apply --check --reverse "$patch" 2>/dev/null; then
        echo "[done] $name：改动已在（跳过）"
      elif git -C "$repo" apply "$patch" 2>/dev/null; then
        echo "[ok]   $name：补丁已应用（$n 个文件）"
      elif git -C "$repo" apply -3 "$patch" 2>/dev/null; then
        echo "[ok]   $name：三方合并成功（$n 个文件）"
      else
        echo "[FAIL] $name：补丁应用失败 → 请用：bash $0 --overlay"; rc=1
      fi
      ;;
  esac
done

echo
if [ $rc -eq 0 ]; then
  echo "核心树改动处理完成。下一步："
  echo "  rm -rf cmake_out/atk-dnn647_eye"
  echo "  ./build.sh vendor/openvela/boards/atk-dnn647/configs/eye --cmake -j\$(nproc)"
else
  echo "有仓库未处理成功，请看上面 [FAIL]/[warn] 行。"
fi
exit $rc
