#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REQUESTED_CONFIG="Debug_x64"
REQUESTED_MEMORY_BACKEND="system"
BLUE_SYSTEM="linux"

print_usage()
{
	echo "Usage:"
	echo "  ./scripts/run-tests-linux.sh [Debug_x64|Release_x64|Profile_x64|Shipping_x64] [--memory-backend=system|mimalloc]"
	echo ""
	echo "Examples:"
	echo "  ./scripts/run-tests-linux.sh"
	echo "  ./scripts/run-tests-linux.sh Debug_x64"
	echo "  ./scripts/run-tests-linux.sh Debug_x64 --memory-backend=mimalloc"
	echo "  ./scripts/run-tests-linux.sh --memory-backend=mimalloc"
}

require_command()
{
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "[BlueBuild] Required command not found: $1" >&2
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

normalize_memory_backend()
{
	case "$1" in
		system) echo "system" ;;
		mimalloc) echo "mimalloc" ;;
		*)
			echo "[BlueBuild] Unsupported memory backend: $1" >&2
			echo "[BlueBuild] Supported: system, mimalloc" >&2
			exit 1
			;;
	esac
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		-h|--help)
			print_usage
			exit 0
			;;
		--memory-backend=*)
			REQUESTED_MEMORY_BACKEND="${1#*=}"
			shift
			;;
		--memory-backend)
			shift
			if [[ $# -eq 0 ]]; then
				echo "[BlueBuild] Missing value for --memory-backend" >&2
				print_usage >&2
				exit 1
			fi
			REQUESTED_MEMORY_BACKEND="$1"
			shift
			;;
		--*)
			echo "[BlueBuild] Unknown option: $1" >&2
			print_usage >&2
			exit 1
			;;
		*)
			REQUESTED_CONFIG="$1"
			shift
			;;
	esac
done

require_command ninja
require_command clang++

CONFIG="$(normalize_config "$REQUESTED_CONFIG")"
MEMORY_BACKEND="$(normalize_memory_backend "$REQUESTED_MEMORY_BACKEND")"
BUILD_CONFIG="${CONFIG%%_*}"
BUILD_PLATFORM="${CONFIG#*_}"
TARGET="BlueRunTests_${CONFIG}"
BIN_DIR="${ROOT_DIR}/out/bin/${BLUE_SYSTEM}/${BUILD_PLATFORM}/${BUILD_CONFIG}"
RUNNER="${BIN_DIR}/BlueRunTests"

echo "[BlueBuild] Configuration      : $CONFIG"
echo "[BlueBuild] Memory backend     : $MEMORY_BACKEND"
echo "[BlueBuild] Generating Ninja build graph"

"${ROOT_DIR}/scripts/premake-linux.sh" ninja \
	--toolchain=clang \
	--blue-platforms=linux \
	--blue-build-platforms="$BUILD_PLATFORM" \
	--memory-backend="$MEMORY_BACKEND" \
	--blue-startup=BlueRunTests

echo "[BlueBuild] Building all test executables"
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

echo "[BlueBuild] Running ${#TESTS[@]} test executables"
"$RUNNER" --jobs=auto "${TESTS[@]}"
