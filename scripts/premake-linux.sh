#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREMAKE="$ROOT_DIR/tools/premake/linux/premake5"
if [ ! -x "$PREMAKE" ]; then
    echo "Premake executable not found or not executable: $PREMAKE" >&2
    exit 1
fi
"$PREMAKE" --file="$ROOT_DIR/premake5.lua" "$@"
