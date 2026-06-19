#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REQUESTED_CONFIG="${1:-Debug_x64}"
BLUE_SYSTEM="macosx"

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

if ! xcrun --sdk macosx --find clang++ >/dev/null 2>&1; then
	echo "[BlueBuild] Apple clang++ was not found through xcrun." >&2
	echo "[BlueBuild] Install the Xcode Command Line Tools with: xcode-select --install" >&2
	exit 1
fi

if ! xcrun --sdk macosx --show-sdk-path >/dev/null 2>&1; then
	echo "[BlueBuild] macOS SDK was not found through xcrun." >&2
	echo "[BlueBuild] Install the Xcode Command Line Tools with: xcode-select --install" >&2
	exit 1
fi

# Premake's Ninja action emits `clang`/`clang++` command names. Put small
# xcrun-backed wrappers first in PATH so Ninja always uses Apple Clang and the
# active Xcode/Command Line Tools developer directory. The generated files and
# wrappers stay under out/ and are safe to delete.
CLANG_WRAPPER_DIR="${ROOT_DIR}/out/tools/macos-clang"
mkdir -p "$CLANG_WRAPPER_DIR"
cat > "${CLANG_WRAPPER_DIR}/clang" <<'EOF'
#!/usr/bin/env bash
exec xcrun --sdk macosx clang "$@"
EOF
cat > "${CLANG_WRAPPER_DIR}/clang++" <<'EOF'
#!/usr/bin/env bash
exec xcrun --sdk macosx clang++ "$@"
EOF
chmod +x "${CLANG_WRAPPER_DIR}/clang" "${CLANG_WRAPPER_DIR}/clang++"
export PATH="${CLANG_WRAPPER_DIR}:$PATH"

CONFIG="$(normalize_config "$REQUESTED_CONFIG")"
BUILD_CONFIG="${CONFIG%%_*}"
BUILD_PLATFORM="${CONFIG#*_}"
TARGET="BlueRunTests_${CONFIG}"
BIN_DIR="${ROOT_DIR}/out/bin/${BLUE_SYSTEM}/${BUILD_PLATFORM}/${BUILD_CONFIG}"
RUNNER="${BIN_DIR}/BlueRunTests"

echo "[BlueBuild] Generating Ninja build graph"
"${ROOT_DIR}/scripts/premake-macos.sh" ninja --toolchain=clang --blue-platforms=macos --blue-build-platforms=x64 --memory-backend=system --blue-startup=BlueRunTests

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
