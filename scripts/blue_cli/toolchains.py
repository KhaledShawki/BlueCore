from __future__ import annotations

import os
import stat
from pathlib import Path

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
