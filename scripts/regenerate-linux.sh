#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREMAKE="$ROOT_DIR/tools/premake/linux/premake5"
ACTION="${1:-ninja}"

if [[ $# -gt 0 ]]; then
    shift
fi

if [[ ! -x "$PREMAKE" ]]; then
    echo "[BlueBuild] Premake executable not found or not executable: $PREMAKE" >&2
    exit 1
fi

normalize_premake_arguments() {
    local -n output_ref=$1
    shift

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --toolchain|--blue-platforms|--memory-backend|--blue-startup|--msvc-toolset|--msvc-tools-version)
                if [[ $# -lt 2 ]]; then
                    echo "[BlueBuild] Missing value for option $1." >&2
                    exit 1
                fi
                output_ref+=("$1=$2")
                shift 2
                ;;
            --toolchain=*|--blue-platforms=*|--memory-backend=*|--blue-startup=*|--msvc-toolset=*|--msvc-tools-version=*)
                output_ref+=("$1")
                shift
                ;;
            *)
                output_ref+=("$1")
                shift
                ;;
        esac
    done
}

NORMALIZED_ARGS=()
normalize_premake_arguments NORMALIZED_ARGS "$@"

set +e
"$PREMAKE" --file="$ROOT_DIR/premake5.lua" --regen-action="$ACTION" "${NORMALIZED_ARGS[@]}" check-regeneration
CHECK_RESULT=$?
set -e

if [[ "$CHECK_RESULT" -eq 0 ]]; then
    echo "[BlueBuild] Regeneration skipped."
    exit 0
fi

if [[ "$CHECK_RESULT" -ne 2 ]]; then
    echo "[BlueBuild] Regeneration check failed with code $CHECK_RESULT." >&2
    exit "$CHECK_RESULT"
fi

"$PREMAKE" --file="$ROOT_DIR/premake5.lua" "${NORMALIZED_ARGS[@]}" "$ACTION"
"$PREMAKE" --file="$ROOT_DIR/premake5.lua" --regen-action="$ACTION" "${NORMALIZED_ARGS[@]}" update-build-token
