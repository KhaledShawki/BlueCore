#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="${1:-}"
CONFIGURATION="${2:-Debug}"
PLATFORM="${3:-x64}"
TOOLCHAIN="${BLUE_CLION_TOOLCHAIN:-clang}"
MEMORY_BACKEND="${BLUE_MEMORY_BACKEND:-system}"

if [[ -z "${TARGET}" ]]; then
    echo "Usage: $0 <target> [configuration] [platform]" >&2
    exit 2
fi

case "$(uname -s)" in
    Darwin*)
        BLUE_PLATFORM="macos"
        PREMAKE_SCRIPT="premake-macos.sh"
        ;;
    Linux*)
        BLUE_PLATFORM="linux"
        PREMAKE_SCRIPT="premake-linux.sh"
        ;;
    *)
        echo "Unsupported CLion Unix clean host: $(uname -s)" >&2
        exit 1
        ;;
esac

if command -v gmake >/dev/null 2>&1; then
    MAKE_COMMAND="gmake"
else
    MAKE_COMMAND="make"
fi

CONFIG_KEY="$(printf '%s_%s' "${CONFIGURATION}" "${PLATFORM}" | tr '[:upper:]' '[:lower:]')"

"${ROOT_DIR}/scripts/${PREMAKE_SCRIPT}" gmake --toolchain="${TOOLCHAIN}" --blue-platforms="${BLUE_PLATFORM}" --memory-backend="${MEMORY_BACKEND}"
"${MAKE_COMMAND}" -C "${ROOT_DIR}/out/build/gmake" clean config="${CONFIG_KEY}"
