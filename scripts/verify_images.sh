#!/usr/bin/env bash
# Convenience wrapper: verify the flash images before plugging the board in.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$HERE/../tests/check_artifacts.py" "$@"
