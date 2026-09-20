#!/usr/bin/env bash
# Build the eye configuration and check the two hard limits afterwards.
#
# Uses the surrounding openvela workspace, so run it from anywhere inside it.
# --clean also removes cmake_out/<target>, which is mandatory after any
# defconfig change (build.sh silently reuses the old .config otherwise).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG="vendor/openvela/boards/atk-dnn647/configs/eye"

# Walk up until the workspace root is found.
WS="$HERE"
while [ "$WS" != "/" ] && [ ! -d "$WS/nuttx" ]; do WS="$(dirname "$WS")"; done
if [ ! -d "$WS/nuttx" ]; then
  echo "ERROR: openvela workspace not found above $HERE" >&2
  exit 1
fi
cd "$WS"

if [ "${1:-}" = "--clean" ]; then
  echo "== removing cmake_out/atk-dnn647_eye (defconfig changes need this)"
  rm -rf cmake_out/atk-dnn647_eye
fi

echo "== building $CONFIG"
./build.sh "$CONFIG" --cmake -j"$(nproc)"

echo
bash "$HERE/check_boundaries.sh"
