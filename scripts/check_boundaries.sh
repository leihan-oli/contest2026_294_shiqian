#!/usr/bin/env bash
# Check the two limits that fail silently when exceeded:
#   _ebss <= 0x34200000   (above it lives the NPU/AXISRAM domain: boot just stops)
#   nuttx.bin <= 1 MiB    (the FSBL copies at most EXTMEM_LRUN_SOURCE_SIZE)
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS="$HERE"
while [ "$WS" != "/" ] && [ ! -d "$WS/nuttx" ]; do WS="$(dirname "$WS")"; done

OUT="${1:-$WS/cmake_out/atk-dnn647_eye}"
ELF="$OUT/nuttx"
BIN="$OUT/nuttx.bin"

if [ ! -f "$ELF" ] || [ ! -f "$BIN" ]; then
  echo "ERROR: no build in $OUT (build first: scripts/build_eye.sh)" >&2
  exit 1
fi

NM="$WS/prebuilts/gcc/linux-x86_64/arm-none-eabi/bin/arm-none-eabi-nm"
[ -x "$NM" ] || NM="$(command -v arm-none-eabi-nm || true)"
if [ -z "$NM" ]; then
  echo "ERROR: arm-none-eabi-nm not found (looked in prebuilts and PATH)" >&2
  exit 1
fi

EBSS_HEX="$($NM "$ELF" | awk '$2 == "A" && $3 == "_ebss" {print $1}')"
EBSS=$((16#$EBSS_HEX))
LIMIT=$((0x34200000))
SIZE="$(stat -c %s "$BIN")"
SIZE_LIMIT=1048576

printf '  _ebss      = 0x%08x  (limit 0x%08x, margin %d B)\n' \
       "$EBSS" "$LIMIT" "$((LIMIT - EBSS))"
printf '  nuttx.bin  = %d B        (limit %d B, margin %d B)\n' \
       "$SIZE" "$SIZE_LIMIT" "$((SIZE_LIMIT - SIZE))"

rc=0
[ "$EBSS" -le "$LIMIT" ] || { echo "  ✗ _ebss over the NPU domain" >&2; rc=1; }
[ "$SIZE" -le "$SIZE_LIMIT" ] || { echo "  ✗ firmware over 1 MiB" >&2; rc=1; }
[ "$rc" -eq 0 ] && echo "  ✓ both limits OK"
exit "$rc"
