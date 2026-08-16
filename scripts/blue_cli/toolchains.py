from __future__ import annotations

import os
import shutil
import subprocess
import stat
from pathlib import Path
from typing import Sequence

from .core import BlueCliError, require_command, run_command


def make_macos_clang_environment(root: Path) -> dict[str, str]:
    require_command("xcrun", "Install the Xcode Command Line Tools with: xcode-select --install")

    if run_command(["xcrun", "--sdk", "macosx", "--find", "clang++"], cwd=root, quiet=True) != 0:
        raise BlueCliError(
            "Apple clang++ was not found through xcrun. "
            "Install the Xcode Command Line Tools with: xcode-select --install"
        )

    if run_command(["xcrun", "--sdk", "macosx", "--show-sdk-path"], cwd=root, quiet=True) != 0:
        raise BlueCliError(
            "The macOS SDK was not found through xcrun. "
            "Install the Xcode Command Line Tools with: xcode-select --install"
        )

    wrapper_dir = root / "out" / "tools" / "macos-clang"
    wrapper_dir.mkdir(parents=True, exist_ok=True)

    wrappers = {
        "clang": '#!/usr/bin/env bash\nexec xcrun --sdk macosx clang "$@"\n',
        "clang++": '#!/usr/bin/env bash\nexec xcrun --sdk macosx clang++ "$@"\n',
    }

    for name, content in wrappers.items():
        wrapper = wrapper_dir / name
        wrapper.write_text(content, encoding="utf-8")
        wrapper.chmod(wrapper.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    env = os.environ.copy()
    current_path = env.get("PATH", "")
    env["PATH"] = str(wrapper_dir) if not current_path else str(wrapper_dir) + os.pathsep + current_path
    return env


def prepare_unix_toolchain_environment(root: Path, host: str, toolchain: str) -> dict[str, str]:
    if host == "macos":
        if toolchain != "clang":
            raise BlueCliError(f"Toolchain '{toolchain}' is not configured for macOS builds.")
        return make_macos_clang_environment(root)

    if host == "linux":
        if toolchain == "clang":
            require_command("clang++", "Install Clang and ensure clang++ is available in PATH.")
        elif toolchain == "gcc":
            require_command("g++", "Install GCC and ensure g++ is available in PATH.")
        else:
            raise BlueCliError(f"Toolchain '{toolchain}' is not configured for Linux builds.")
        return os.environ.copy()

    raise BlueCliError(f"Unix toolchain setup is not configured for host: {host}")


def find_vswhere() -> Path:
    program_files_x86 = os.environ.get("ProgramFiles(x86)")
    if not program_files_x86:
        raise BlueCliError("ProgramFiles(x86) is unavailable; cannot locate Visual Studio.")

    vswhere = Path(program_files_x86) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.is_file():
        raise BlueCliError("vswhere.exe was not found; install Visual Studio or Visual Studio Build Tools.")

    return vswhere


def find_visual_studio_installation() -> Path:
    vswhere = find_vswhere()
    result = subprocess.run(
        [
            str(vswhere),
            "-latest",
            "-products",
            "*",
            "-property",
            "installationPath",
        ],
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode != 0:
        raise BlueCliError(f"vswhere failed while locating Visual Studio with code {result.returncode}.")

    candidates = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if not candidates:
        raise BlueCliError("No Visual Studio installation was found.")

    return Path(candidates[0])


def find_msbuild() -> str:
    msbuild = shutil.which("msbuild")
    if msbuild:
        return msbuild

    vswhere = find_vswhere()
    result = subprocess.run(
        [
            str(vswhere),
            "-latest",
            "-products",
            "*",
            "-requires",
            "Microsoft.Component.MSBuild",
            "-find",
            r"MSBuild\**\Bin\MSBuild.exe",
        ],
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode != 0:
        raise BlueCliError(f"vswhere failed while locating MSBuild with code {result.returncode}.")

    candidates = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if not candidates:
        raise BlueCliError("MSBuild was not found in the installed Visual Studio instance.")

    return candidates[0]


def find_msvc_asan_runtime() -> Path:
    installation = find_visual_studio_installation()
    toolchain_root = installation / "VC" / "Tools" / "MSVC"
    candidates = list(toolchain_root.glob("*/bin/Hostx64/x64/clang_rt.asan_dynamic-x86_64.dll"))

    if not candidates:
        raise BlueCliError(
            "The MSVC AddressSanitizer runtime clang_rt.asan_dynamic-x86_64.dll was not found. "
            "Install the Visual Studio C++ AddressSanitizer components."
        )

    return max(candidates, key=lambda path: path.stat().st_mtime_ns)


def prepare_windows_sanitizer_environment(sanitizers: Sequence[str]) -> dict[str, str]:
    env = os.environ.copy()
    if "asan" not in sanitizers:
        return env

    runtime = find_msvc_asan_runtime()
    current_path = env.get("PATH", "")
    runtime_dir = str(runtime.parent)
    env["PATH"] = runtime_dir if not current_path else runtime_dir + os.pathsep + current_path
    return env
