from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
from pathlib import Path
from typing import Sequence

CONFIGURATIONS = ("Debug", "Release", "Profile", "Shipping")
LINKAGES = ("static", "shared")
MEMORY_BACKENDS = ("system", "mimalloc")
TOOLCHAINS = ("clang", "gcc", "msvc")

HOST_TO_PREMAKE_DIR = {
    "windows": "windows",
    "linux": "linux",
    "macos": "macos",
}

HOST_TO_BLUE_PLATFORM = {
    "windows": "windows",
    "linux": "linux",
    "macos": "macos",
}

HOST_TO_BIN_SYSTEM = {
    "windows": "windows",
    "linux": "linux",
    "macos": "macosx",
}


class BlueCliError(RuntimeError):
    pass


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def detect_host(system_name: str | None = None) -> str:
    system = (system_name or platform.system()).lower()

    if system == "darwin":
        return "macos"
    if system == "linux":
        return "linux"
    if system == "windows":
        return "windows"

    raise BlueCliError(f"Unsupported host platform: {system_name or platform.system()}")


def premake_executable(root: Path, host: str) -> Path:
    try:
        platform_dir = HOST_TO_PREMAKE_DIR[host]
    except KeyError as exc:
        raise BlueCliError(f"Unsupported Blue host: {host}") from exc

    executable = "premake5.exe" if host == "windows" else "premake5"
    return root / "tools" / "premake" / platform_dir / executable


def require_premake(root: Path, host: str) -> Path:
    premake = premake_executable(root, host)
    if not premake.is_file():
        raise BlueCliError(f"Premake executable not found: {premake}")

    if host != "windows" and not os.access(premake, os.X_OK):
        raise BlueCliError(f"Premake executable is not executable: {premake}")

    return premake


def resolve_command(command: str) -> str | None:
    candidate = Path(command).expanduser()
    if candidate.is_file():
        return str(candidate.resolve())
    return shutil.which(command)


def require_command(command: str, install_hint: str | None = None) -> str:
    resolved = resolve_command(command)
    if resolved:
        return resolved

    message = f"Required command not found: {command}"
    if install_hint:
        message += f". {install_hint}"
    raise BlueCliError(message)


def require_gnu_make() -> str:
    resolved = resolve_command("gmake") or resolve_command("make")
    if resolved:
        return resolved

    raise BlueCliError(
        "Required command not found: GNU Make. " "Install GNU Make and ensure gmake or make is available in PATH."
    )


def run_command(
    command: Sequence[str],
    *,
    cwd: Path,
    env: dict[str, str] | None = None,
    quiet: bool = False,
) -> int:
    resolved_command = [str(part) for part in command]
    if not quiet:
        print("+ " + " ".join(resolved_command))

    try:
        completed = subprocess.run(
            resolved_command,
            cwd=cwd,
            env=env,
            stdout=subprocess.DEVNULL if quiet else None,
            stderr=subprocess.DEVNULL if quiet else None,
            check=False,
        )
    except OSError as exc:
        raise BlueCliError(f"Failed to execute '{resolved_command[0]}': {exc}") from exc

    return completed.returncode


def run_premake(
    root: Path,
    host: str,
    premake_args: Sequence[str],
    *,
    env: dict[str, str] | None = None,
) -> int:
    premake = require_premake(root, host)
    command = [str(premake), f"--file={root / 'premake5.lua'}", *premake_args]
    return run_command(command, cwd=root, env=env)


def normalize_choice(value: str, choices: Sequence[str], option_name: str) -> str:
    normalized = {choice.lower(): choice for choice in choices}.get(value.lower())
    if normalized is None:
        raise argparse.ArgumentTypeError(f"{option_name} must be one of: {', '.join(choices)}")
    return normalized
