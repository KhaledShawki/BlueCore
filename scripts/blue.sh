#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [ "$#" -lt 1 ]; then
    cat >&2 <<'USAGE'
Usage:
  scripts/blue.sh add-file     --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp
  scripts/blue.sh remove-file  --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp [--blue-delete-file]
  scripts/blue.sh rename-file  --blue-project=BlueSystem --blue-kind=source --blue-from=Old.cpp --blue-to=New.cpp
  scripts/blue.sh add-project  --blue-project=BlueGraphics [--blue-type=library] [--blue-linkage=auto]
USAGE
    exit 2
fi

COMMAND="$1"
shift

case "$COMMAND" in
    add-file|remove-file|rename-file|add-project)
        "$ROOT_DIR/scripts/premake-linux.sh" "blue-$COMMAND" "$@"
        ;;
    *)
        echo "Unknown Blue command: $COMMAND" >&2
        exit 2
        ;;
esac
