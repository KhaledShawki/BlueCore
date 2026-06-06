#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG="${1:-Debug_x64}"

"${ROOT_DIR}/scripts/premake-linux.sh" ninja --toolchain=clang --blue-platforms=linux
ninja -C "${ROOT_DIR}/out/build/ninja" BlueRunTests config="${CONFIG,,}"
