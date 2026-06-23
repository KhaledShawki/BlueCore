#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-format}"
PLATFORM="${2:-linux}"

if [[ $# -ge 2 ]]; then
    shift 2
else
    shift $# || true
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

case "$MODE" in
    format|check|list) ;;
    *)
        echo "[BlueFormat] Unsupported mode: $MODE" >&2
        echo "[BlueFormat] Expected: format, check, or list" >&2
        exit 1
        ;;
esac

TEMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/blue-format.XXXXXX")"
trap 'rm -rf "$TEMP_DIR"' EXIT

CXX_FILES="$TEMP_DIR/cxx-files.txt"
LUA_FILES="$TEMP_DIR/lua-files.txt"
PYTHON_FILES="$TEMP_DIR/python-files.txt"

quote_tool_error() {
    local tool_name="$1"
    local override_name="$2"
    local install_hint="$3"

    echo "[BlueFormat] $tool_name was not found." >&2
    echo "[BlueFormat] Set $override_name or install it." >&2
    echo "[BlueFormat] $install_hint" >&2
}

resolve_explicit_tool() {
    local value="$1"
    local tool_name="$2"

    if [[ -z "$value" ]]; then
        return 1
    fi

    if [[ -x "$value" ]]; then
        printf '%s\n' "$value"
        return 0
    fi

    if command -v "$value" >/dev/null 2>&1; then
        command -v "$value"
        return 0
    fi

    echo "[BlueFormat] $tool_name was not found from explicit value: $value" >&2
    return 2
}

find_clang_format() {
    local explicit="${BLUE_CLANG_FORMAT:-}"
    local resolved

    if resolved="$(resolve_explicit_tool "$explicit" "clang-format")"; then
        printf '%s\n' "$resolved"
        return 0
    elif [[ $? -eq 2 ]]; then
        return 1
    fi

    if command -v clang-format >/dev/null 2>&1; then
        command -v clang-format
        return 0
    fi

    local repo_local="$ROOT_DIR/tools/clang-format/$PLATFORM/clang-format"
    if [[ -x "$repo_local" ]]; then
        printf '%s\n' "$repo_local"
        return 0
    fi

    quote_tool_error "clang-format" "BLUE_CLANG_FORMAT" "macOS: brew install llvm"
    return 1
}

find_stylua() {
    local explicit="${BLUE_STYLUA:-}"
    local resolved

    if resolved="$(resolve_explicit_tool "$explicit" "stylua")"; then
        printf '%s\n' "$resolved"
        return 0
    elif [[ $? -eq 2 ]]; then
        return 1
    fi

    local repo_local="$ROOT_DIR/tools/stylua/$PLATFORM/stylua"
    if [[ -x "$repo_local" ]]; then
        printf '%s\n' "$repo_local"
        return 0
    fi

    if command -v stylua >/dev/null 2>&1; then
        command -v stylua
        return 0
    fi

    quote_tool_error "stylua" "BLUE_STYLUA" "macOS: brew install stylua"
    return 1
}

find_black_command() {
    local explicit="${BLUE_BLACK:-}"

    if [[ -n "$explicit" ]]; then
        if [[ -x "$explicit" ]]; then
            BLACK_COMMAND=( "$explicit" )
            return 0
        fi

        if command -v "$explicit" >/dev/null 2>&1; then
            BLACK_COMMAND=( "$(command -v "$explicit")" )
            return 0
        fi

        echo "[BlueFormat] black was not found from BLUE_BLACK value: $explicit" >&2
        return 1
    fi

    local repo_local="$ROOT_DIR/tools/black/$PLATFORM/black"
    if [[ -x "$repo_local" ]]; then
        BLACK_COMMAND=( "$repo_local" )
        return 0
    fi

    if command -v black >/dev/null 2>&1; then
        BLACK_COMMAND=( "$(command -v black)" )
        return 0
    fi

    if command -v python3 >/dev/null 2>&1; then
        if python3 -m black --version >/dev/null 2>&1; then
            BLACK_COMMAND=( python3 -m black )
            return 0
        fi
    fi

    if command -v python >/dev/null 2>&1; then
        if python -m black --version >/dev/null 2>&1; then
            BLACK_COMMAND=( python -m black )
            return 0
        fi
    fi

    quote_tool_error "black" "BLUE_BLACK" "macOS: python3 -m pip install black"
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
            -not -path "$ROOT_DIR/generated/*" \
            -not -path "$ROOT_DIR/external/*" \
            -not -path "$ROOT_DIR/third_party/*" \
            -not -path "$ROOT_DIR/tools/premake/*" \
            -not -path "$ROOT_DIR/tools/clang-format/*" \
            -not -path "$ROOT_DIR/tools/stylua/*" \
            -not -path "$ROOT_DIR/tools/black/*"
    )
}

is_ignored_by_clang_format() {
    local relative="$1"
    local ignore_file="$ROOT_DIR/.clang-format-ignore"

    [[ -f "$ignore_file" ]] || return 1

    local pattern
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

collect_cxx_files() {
    local roots=( "modules" "apps" "tests" "tools" )
    local root
    local file
    local relative

    for root in "${roots[@]}"; do
        [[ -d "$ROOT_DIR/$root" ]] || continue

        while IFS= read -r file; do
            relative="${file#$ROOT_DIR/}"
            if ! is_ignored_by_clang_format "$relative"; then
                printf '%s\n' "$file"
            fi
        done < <(
            find "$ROOT_DIR/$root" -type f \( \
                -name '*.h' -o \
                -name '*.hpp' -o \
                -name '*.hxx' -o \
                -name '*.inl' -o \
                -name '*.c' -o \
                -name '*.cc' -o \
                -name '*.cpp' -o \
                -name '*.cxx' \
            \)
        )
    done | sort
}

collect_lua_files() {
    {
        [[ -f "$ROOT_DIR/build.lua" ]] && printf '%s\n' "$ROOT_DIR/build.lua"
        [[ -f "$ROOT_DIR/premake5.lua" ]] && printf '%s\n' "$ROOT_DIR/premake5.lua"

        [[ -d "$ROOT_DIR/build" ]] && find "$ROOT_DIR/build" -type f -name '*.lua'
        [[ -d "$ROOT_DIR/modules" ]] && find "$ROOT_DIR/modules" -type f -name '*.lua'
        [[ -d "$ROOT_DIR/apps" ]] && find "$ROOT_DIR/apps" -type f -name '*.lua'
        [[ -d "$ROOT_DIR/tests" ]] && find "$ROOT_DIR/tests" -type f -name '*.lua'
    } | sort
}

collect_python_files() {
    {
        [[ -d "$ROOT_DIR/scripts" ]] && find "$ROOT_DIR/scripts" -type f -name '*.py'
        [[ -d "$ROOT_DIR/tools" ]] && find "$ROOT_DIR/tools" -type f -name '*.py'
    } | sort
}

count_file_lines() {
    local file="$1"

    if [[ ! -s "$file" ]]; then
        printf '0\n'
        return 0
    fi

    wc -l < "$file" | tr -d '[:space:]'
}

print_relative_file_list() {
    local title="$1"
    local file_list="$2"
    local file

    echo "[$title]"

    if [[ ! -s "$file_list" ]]; then
        echo "(none)"
        return 0
    fi

    while IFS= read -r file; do
        printf '%s\n' "${file#$ROOT_DIR/}"
    done < "$file_list"
}

format_cxx_files() {
    local clang_format="$1"
    local failed=0
    local file

    while IFS= read -r file; do
        if [[ "$MODE" == "check" ]]; then
            "$clang_format" --style=file --dry-run --Werror "$file" || failed=$((failed + 1))
        else
            "$clang_format" --style=file -i "$file" || failed=$((failed + 1))
        fi
    done < "$CXX_FILES"

    return "$failed"
}

format_lua_files() {
    local stylua="$1"
    local failed=0
    local file

    while IFS= read -r file; do
        if [[ "$MODE" == "check" ]]; then
            "$stylua" --check "$file" || failed=$((failed + 1))
        else
            "$stylua" "$file" || failed=$((failed + 1))
        fi
    done < "$LUA_FILES"

    return "$failed"
}

format_python_files() {
    local failed=0
    local file

    while IFS= read -r file; do
        if [[ "$MODE" == "check" ]]; then
            "${BLACK_COMMAND[@]}" --check --quiet "$file" || failed=$((failed + 1))
        else
            "${BLACK_COMMAND[@]}" --quiet "$file" || failed=$((failed + 1))
        fi
    done < "$PYTHON_FILES"

    return "$failed"
}

assert_single_root_clang_format

collect_cxx_files > "$CXX_FILES"
collect_lua_files > "$LUA_FILES"
collect_python_files > "$PYTHON_FILES"

CXX_COUNT="$(count_file_lines "$CXX_FILES")"
LUA_COUNT="$(count_file_lines "$LUA_FILES")"
PYTHON_COUNT="$(count_file_lines "$PYTHON_FILES")"

echo "[BlueFormat] mode: $MODE"
echo "[BlueFormat] C/C++ files: $CXX_COUNT"
echo "[BlueFormat] Lua files: $LUA_COUNT"
echo "[BlueFormat] Python files: $PYTHON_COUNT"

if [[ "$MODE" == "list" ]]; then
    print_relative_file_list "C/C++" "$CXX_FILES"
    print_relative_file_list "Lua" "$LUA_FILES"
    print_relative_file_list "Python" "$PYTHON_FILES"
    exit 0
fi

FAILED=0

if [[ "$CXX_COUNT" -gt 0 ]]; then
    CLANG_FORMAT="$(find_clang_format)"
    echo "[BlueFormat] clang-format: $CLANG_FORMAT"

    format_cxx_files "$CLANG_FORMAT" || FAILED=$((FAILED + $?))
fi

if [[ "$LUA_COUNT" -gt 0 ]]; then
    STYLUA="$(find_stylua)"
    echo "[BlueFormat] stylua: $STYLUA"

    format_lua_files "$STYLUA" || FAILED=$((FAILED + $?))
fi

if [[ "$PYTHON_COUNT" -gt 0 ]]; then
    BLACK_COMMAND=()
    find_black_command
    echo "[BlueFormat] black: ${BLACK_COMMAND[*]}"

    format_python_files || FAILED=$((FAILED + $?))
fi

if [[ "$FAILED" -ne 0 ]]; then
    echo "[BlueFormat] Formatting failed for $FAILED file(s)." >&2
    exit 1
fi

if [[ "$MODE" == "check" ]]; then
    echo "[BlueFormat] Formatting check passed."
else
    echo "[BlueFormat] Formatting completed."
fi
