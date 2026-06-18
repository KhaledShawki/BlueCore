#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-format}"
PLATFORM="${2:-linux}"
shift 2 || true

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

find_clang_format() {
    # Highest priority: explicit override via environment variable
    if [[ -n "${BLUE_CLANG_FORMAT:-}" ]]; then
        if [[ -x "$BLUE_CLANG_FORMAT" ]]; then
            printf '%s\n' "$BLUE_CLANG_FORMAT"
            return 0
        fi

        if command -v "$BLUE_CLANG_FORMAT" >/dev/null 2>&1; then
            command -v "$BLUE_CLANG_FORMAT"
            return 0
        fi

        echo "[BlueFormat] clang-format was not found from BLUE_CLANG_FORMAT value: $BLUE_CLANG_FORMAT" >&2
        return 1
    fi

    # Prefer clang-format from PATH (system, Homebrew, or manually installed)
    if command -v clang-format >/dev/null 2>&1; then
        command -v clang-format
        return 0
    fi

    # Fallback: repo-local prebuilt binary (only if nothing in PATH)
    local repo_local="$ROOT_DIR/tools/clang-format/$PLATFORM/clang-format"
    if [[ -x "$repo_local" ]]; then
        printf '%s\n' "$repo_local"
        return 0
    fi

    echo "[BlueFormat] clang-format was not found. Please install it (e.g. brew install llvm or apt install clang-format) or set BLUE_CLANG_FORMAT." >&2
    return 1
}

assert_single_root_clang_format() {
    local root_format="$ROOT_DIR/.clang-format"
    local file

    while IFS= read -r file; do
        [[ "$file" == "$root_format" ]] && continue
        echo "[BlueFormat] Nested clang-format file is not allowed: ${file#$ROOT_DIR/}" >&2
        echo "[BlueFormat] Keep only the repository root .clang-format." >&2
        return 1
    done < <(
        find "$ROOT_DIR" -type f \( -name '.clang-format' -o -name '_clang-format' \) \
            -not -path "$ROOT_DIR/out/*" \
            -not -path "$ROOT_DIR/external/*" \
            -not -path "$ROOT_DIR/third_party/*" \
            -not -path "$ROOT_DIR/tools/premake/*" \
            -not -path "$ROOT_DIR/tools/clang-format/*"
    )
}

is_ignored() {
    local relative="$1"
    local ignore_file="$ROOT_DIR/.clang-format-ignore"

    [[ -f "$ignore_file" ]] || return 1

    while IFS= read -r pattern || [[ -n "$pattern" ]]; do
        pattern="${pattern%%#*}"
        pattern="${pattern//$'\r'/}"
        pattern="${pattern#${pattern%%[![:space:]]*}}"
        pattern="${pattern%${pattern##*[![:space:]]}}"

        [[ -z "$pattern" ]] && continue

        case "$relative" in
            $pattern) return 0 ;;
        esac
    done < "$ignore_file"

    return 1
}

collect_files() {
    local roots=("modules" "apps" "tests" "tools")
    local root

    for root in "${roots[@]}"; do
        [[ -d "$ROOT_DIR/$root" ]] || continue
        find "$ROOT_DIR/$root" -type f \( \
            -name '*.h' -o \
            -name '*.hpp' -o \
            -name '*.hxx' -o \
            -name '*.inl' -o \
            -name '*.c' -o \
            -name '*.cc' -o \
            -name '*.cpp' -o \
            -name '*.cxx' \
        \) | while IFS= read -r file; do
            local relative="${file#$ROOT_DIR/}"
            if ! is_ignored "$relative"; then
                printf '%s\n' "$file"
            fi
        done
    done | sort
}

assert_single_root_clang_format

CLANG_FORMAT="$(find_clang_format)"
mapfile -t FILES < <(collect_files)

echo "[BlueFormat] clang-format: $CLANG_FORMAT"
echo "[BlueFormat] mode: $MODE"
echo "[BlueFormat] files: ${#FILES[@]}"

if [[ "$MODE" == "list" ]]; then
    for file in "${FILES[@]}"; do
        printf '%s\n' "${file#$ROOT_DIR/}"
    done
    exit 0
fi

FAILED=0
for file in "${FILES[@]}"; do
    if [[ "$MODE" == "check" ]]; then
        "$CLANG_FORMAT" --style=file --dry-run --Werror "$file" || FAILED=$((FAILED + 1))
    else
        "$CLANG_FORMAT" --style=file -i "$file" || FAILED=$((FAILED + 1))
    fi
done

if [[ "$FAILED" -ne 0 ]]; then
    echo "[BlueFormat] Formatting failed for $FAILED file(s)." >&2
    exit 1
fi

if [[ "$MODE" == "check" ]]; then
    echo "[BlueFormat] Formatting check passed."
else
    echo "[BlueFormat] Formatting completed."
fi
