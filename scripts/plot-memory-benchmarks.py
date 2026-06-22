#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def print_missing_file_error(path: Path, description: str) -> None:
    print(f"error: {description} not found:", file=sys.stderr)
    print(f"  {path}", file=sys.stderr)


def main(argv: list[str]) -> int:
    root = repo_root()
    plotter_path = root / "scripts" / "plot-benchmarks.py"
    profile_path = root / "benchmarks" / "profiles" / "blue_memory_hot_path.json"

    if not plotter_path.is_file():
        print_missing_file_error(plotter_path, "Generic benchmark plotter")
        return 1

    if not profile_path.is_file():
        print_missing_file_error(profile_path, "BlueMemory benchmark profile")
        print(file=sys.stderr)
        return 1

    command = [
        sys.executable,
        str(plotter_path),
        "--profile",
        str(profile_path),
        *argv,
    ]

    completed = subprocess.run(command, cwd=root, check=False)
    return int(completed.returncode)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
