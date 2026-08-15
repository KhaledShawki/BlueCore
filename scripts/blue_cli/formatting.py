from __future__ import annotations

import argparse
import fnmatch
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence

from .core import BlueCliError, run_command

CXX_EXTENSIONS = {".h", ".hpp", ".hxx", ".inl", ".c", ".cc", ".cpp", ".cxx"}
CXX_ROOTS = ("modules", "apps", "tests", "tools")
LUA_ROOTS = ("build", "modules", "apps", "tests", "tools")
PYTHON_ROOTS = ("scripts", "tools", "benchmarks")
NESTED_FORMAT_EXCLUDED_ROOTS = {
    ".git",
    ".idea",
    ".vs",
    "out",
    "generated",
    "external",
    "third_party",
    "thirdparty",
}


@dataclass(frozen=True)
class FormatFiles:
    cxx: tuple[Path, ...]
    lua: tuple[Path, ...]
    python: tuple[Path, ...]


@dataclass(frozen=True)
class FormatTools:
    clang_format: tuple[str, ...]
    stylua: tuple[str, ...]
    black: tuple[str, ...]


def parse_format_args(argv: Sequence[str], *, command: str) -> argparse.Namespace:
    parser = argparse.ArgumentParser(prog=f"blue {command}")
    parser.add_argument("--format-path", default=os.environ.get("BLUE_CLANG_FORMAT"))
    parser.add_argument("--lua-format-path", default=os.environ.get("BLUE_STYLUA"))
    parser.add_argument("--python-format-path", default=os.environ.get("BLUE_BLACK"))
    return parser.parse_args(list(argv))


def relative_posix(root: Path, path: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def load_clang_format_ignore(root: Path) -> tuple[str, ...]:
    ignore = root / ".clang-format-ignore"
    if not ignore.is_file():
        return ()

    patterns: list[str] = []
    for raw_line in ignore.read_text(encoding="utf-8").splitlines():
        line = raw_line.split("#", 1)[0].strip().replace("\\", "/")
        if line:
            patterns.append(line)
    return tuple(patterns)


def is_ignored(relative: str, patterns: Iterable[str]) -> bool:
    normalized = relative.replace("\\", "/")
    return any(fnmatch.fnmatchcase(normalized, pattern) for pattern in patterns)


def collect_under(root: Path, relative_roots: Iterable[str], extensions: set[str]) -> list[Path]:
    result: set[Path] = set()
    for relative_root in relative_roots:
        directory = root / relative_root
        if not directory.is_dir():
            continue
        for path in directory.rglob("*"):
            if path.is_file() and path.suffix.lower() in extensions:
                result.add(path.resolve())
    return sorted(result)


def collect_format_files(root: Path) -> FormatFiles:
    ignore_patterns = load_clang_format_ignore(root)
    cxx = [
        path
        for path in collect_under(root, CXX_ROOTS, CXX_EXTENSIONS)
        if not is_ignored(relative_posix(root, path), ignore_patterns)
    ]

    lua: set[Path] = set(collect_under(root, LUA_ROOTS, {".lua"}))
    for filename in ("build.lua", "premake5.lua"):
        path = root / filename
        if path.is_file():
            lua.add(path.resolve())

    python = collect_under(root, PYTHON_ROOTS, {".py"})
    return FormatFiles(tuple(sorted(cxx)), tuple(sorted(lua)), tuple(python))


def assert_single_root_clang_format(root: Path) -> None:
    root_format = (root / ".clang-format").resolve()
    nested: list[Path] = []

    for current_root, directory_names, filenames in os.walk(root):
        current = Path(current_root)
        relative_parts = current.relative_to(root).parts
        if not relative_parts:
            directory_names[:] = [name for name in directory_names if name not in NESTED_FORMAT_EXCLUDED_ROOTS]
        if len(relative_parts) >= 2 and relative_parts[:2] in {
            ("tools", "premake"),
            ("tools", "clang-format"),
            ("tools", "stylua"),
            ("tools", "black"),
        }:
            directory_names[:] = []

        for filename in filenames:
            if filename not in {".clang-format", "_clang-format"}:
                continue
            candidate = current / filename
            if candidate.resolve() != root_format:
                nested.append(candidate)

    if nested:
        relative = relative_posix(root, nested[0].resolve())
        raise BlueCliError(
            f"Nested clang-format file is not allowed: {relative}. " "Keep only the repository root .clang-format."
        )


def resolve_explicit_tool(value: str | None, display_name: str) -> str | None:
    if not value:
        return None

    candidate = Path(value).expanduser()
    if candidate.is_file():
        return str(candidate.resolve())

    resolved = shutil.which(value)
    if resolved:
        return resolved

    raise BlueCliError(f"{display_name} was not found from explicit value: {value}")


def first_existing_file(candidates: Iterable[Path]) -> str | None:
    for candidate in candidates:
        if candidate.is_file():
            return str(candidate.resolve())
    return None


def resolve_clang_format(root: Path, host: str, explicit: str | None) -> tuple[str, ...]:
    resolved = resolve_explicit_tool(explicit, "clang-format")
    if resolved:
        return (resolved,)

    platform_name = "macos" if host == "macos" else host
    executable = "clang-format.exe" if host == "windows" else "clang-format"
    candidates = [root / "tools" / "clang-format" / platform_name / executable]
    if host == "windows":
        candidates.append(Path(r"C:\Program Files\LLVM\bin\clang-format.exe"))
        program_files_x86 = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"))
        vswhere = program_files_x86 / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
        if vswhere.is_file():
            completed = subprocess.run(
                [str(vswhere), "-latest", "-property", "installationPath"],
                capture_output=True,
                text=True,
                check=False,
            )
            if completed.returncode == 0 and completed.stdout.strip():
                candidates.append(
                    Path(completed.stdout.strip()) / "VC" / "Tools" / "Llvm" / "x64" / "bin" / "clang-format.exe"
                )

    local = first_existing_file(candidates)
    if local:
        return (local,)

    system = shutil.which("clang-format")
    if system:
        return (system,)

    raise BlueCliError("clang-format was not found. Install it or set BLUE_CLANG_FORMAT/--format-path.")


def resolve_stylua(root: Path, host: str, explicit: str | None) -> tuple[str, ...]:
    resolved = resolve_explicit_tool(explicit, "stylua")
    if resolved:
        return (resolved,)

    platform_name = "macos" if host == "macos" else host
    local = first_existing_file(
        [root / "tools" / "stylua" / platform_name / ("stylua.exe" if host == "windows" else "stylua")]
    )
    if local:
        return (local,)

    system = shutil.which("stylua")
    if system:
        return (system,)

    raise BlueCliError("stylua was not found. Install it or set BLUE_STYLUA/--lua-format-path.")


def resolve_black(root: Path, host: str, explicit: str | None) -> tuple[str, ...]:
    resolved = resolve_explicit_tool(explicit, "black")
    if resolved:
        return (resolved,)

    platform_name = "macos" if host == "macos" else host
    local = first_existing_file(
        [root / "tools" / "black" / platform_name / ("black.exe" if host == "windows" else "black")]
    )
    if local:
        return (local,)

    system = shutil.which("black")
    if system:
        return (system,)

    if run_command([sys.executable, "-m", "black", "--version"], cwd=root, quiet=True) == 0:
        return (sys.executable, "-m", "black")

    raise BlueCliError("black was not found. Install it with: python -m pip install black")


def resolve_format_tools(root: Path, host: str, args: argparse.Namespace) -> FormatTools:
    return FormatTools(
        clang_format=resolve_clang_format(root, host, args.format_path),
        stylua=resolve_stylua(root, host, args.lua_format_path),
        black=resolve_black(root, host, args.python_format_path),
    )


def print_file_section(title: str, root: Path, files: Iterable[Path]) -> None:
    print(f"[{title}]")
    listed = list(files)
    if not listed:
        print("(none)")
        return
    for path in listed:
        print(relative_posix(root, path))


def run_formatter_for_files(
    root: Path,
    command: tuple[str, ...],
    files: Iterable[Path],
    arguments: Sequence[str],
) -> int:
    failures = 0
    for path in files:
        if run_command([*command, *arguments, str(path)], cwd=root) != 0:
            failures += 1
    return failures


def run_format(root: Path, host: str, mode: str, args: argparse.Namespace) -> int:
    assert_single_root_clang_format(root)
    files = collect_format_files(root)

    print(f"[BlueFormat] mode: {mode}")
    print(f"[BlueFormat] C/C++ files: {len(files.cxx)}")
    print(f"[BlueFormat] Lua files: {len(files.lua)}")
    print(f"[BlueFormat] Python files: {len(files.python)}")

    if mode == "list":
        print_file_section("C/C++", root, files.cxx)
        print_file_section("Lua", root, files.lua)
        print_file_section("Python", root, files.python)
        return 0

    tools = resolve_format_tools(root, host, args)
    print(f"[BlueFormat] clang-format: {' '.join(tools.clang_format)}")
    print(f"[BlueFormat] stylua: {' '.join(tools.stylua)}")
    print(f"[BlueFormat] black: {' '.join(tools.black)}")

    check = mode == "check"
    failures = 0
    failures += run_formatter_for_files(
        root,
        tools.clang_format,
        files.cxx,
        ["--style=file", "--dry-run", "--Werror"] if check else ["--style=file", "-i"],
    )
    failures += run_formatter_for_files(root, tools.stylua, files.lua, ["--check"] if check else [])
    failures += run_formatter_for_files(
        root,
        tools.black,
        files.python,
        ["--check", "--quiet"] if check else ["--quiet"],
    )

    if failures:
        print(f"[BlueFormat] Formatting failed for {failures} file(s).", file=sys.stderr)
        return 1

    print("[BlueFormat] Formatting check passed." if check else "[BlueFormat] Formatting completed.")
    return 0
