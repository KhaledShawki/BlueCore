#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REQUESTED_CONFIG="${1:-Debug_x64}"
BLUE_SYSTEM="linux"

require_command()
{
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "[BlueBuild] Required command not found: $1" >&2
		echo "[BlueBuild] Install Ninja and ensure it is available in PATH." >&2
		exit 1
	fi
}

normalize_config()
{
	case "$1" in
		Debug_x64|debug_x64) echo "Debug_x64" ;;
		Release_x64|release_x64) echo "Release_x64" ;;
		Profile_x64|profile_x64) echo "Profile_x64" ;;
		Shipping_x64|shipping_x64) echo "Shipping_x64" ;;
		*)
			echo "[BlueBuild] Unsupported Ninja test configuration: $1" >&2
			echo "[BlueBuild] Supported: Debug_x64, Release_x64, Profile_x64, Shipping_x64" >&2
			exit 1
			;;
	esac
}

require_command ninja

require_command clang++

CONFIG="$(normalize_config "$REQUESTED_CONFIG")"
BUILD_CONFIG="${CONFIG%%_*}"
BUILD_PLATFORM="${CONFIG#*_}"
TARGET="BlueRunTests_${CONFIG}"
BIN_DIR="${ROOT_DIR}/out/bin/${BLUE_SYSTEM}/${BUILD_PLATFORM}/${BUILD_CONFIG}"
RUNNER="${BIN_DIR}/BlueRunTests"

"${ROOT_DIR}/scripts/premake-linux.sh" ninja --toolchain=clang --blue-platforms=linux --blue-build-platforms=x64 --memory-backend=system --blue-startup=BlueRunTests
ninja -C "${ROOT_DIR}/out/build/ninja" "$TARGET"

if [[ ! -x "$RUNNER" ]]; then
	echo "[BlueBuild] Test runner was not built or is not executable: $RUNNER" >&2
	exit 1
fi

TESTS=()
while IFS= read -r test_binary
do
	name="$(basename "$test_binary")"
	if [[ "$name" != "BlueRunTests" ]]; then
		TESTS+=("$test_binary")
	fi
done < <(find "$BIN_DIR" -maxdepth 1 -type f -perm -111 -name '*Tests' | sort)

if [[ "${#TESTS[@]}" -eq 0 ]]; then
	echo "[BlueBuild] No test executables found in $BIN_DIR" >&2
	exit 1
fi

"$RUNNER" "${TESTS[@]}"
