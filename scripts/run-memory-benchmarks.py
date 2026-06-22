#!/usr/bin/env python3

from __future__ import annotations

import argparse
import platform
import subprocess
from pathlib import Path


BENCHMARK_NAME = "BlueMemoryHotPathBenchmarks"

CONFIG_TO_DIR = {
    "Debug_x64": "Debug",
    "Release_x64": "Release",
    "Profile_x64": "Profile",
    "Shipping_x64": "Shipping",
}

HOST_TO_PREMAKE_SCRIPT = {
    "macos": "premake-macos.sh",
    "linux": "premake-linux.sh",
    "windows": "premake-windows.cmd",
}

HOST_TO_PREMAKE_PLATFORM = {
    "macos": "macos",
    "linux": "linux",
    "windows": "windows",
}

HOST_TO_BIN_PLATFORM = {
    "macos": "macosx",
    "linux": "linux",
    "windows": "windows",
}


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def detect_host() -> str:
    system = platform.system().lower()

    if system == "darwin":
        return "macos"

    if system == "linux":
        return "linux"

    if system == "windows":
        return "windows"

    raise RuntimeError(f"Unsupported host platform: {platform.system()}")


def resolve_under_root(root: Path, value: str | None, default: Path) -> Path:
    if value is None:
        return default

    path_value = Path(value)
    if path_value.is_absolute():
        return path_value

    return root / path_value


def executable_suffix(host: str) -> str:
    return ".exe" if host == "windows" else ""


def command_for_host(command: list[str], host: str) -> list[str]:
    if host == "windows" and command and command[0].lower().endswith(".cmd"):
        return ["cmd.exe", "/c", *command]

    return command


def run_command(command: list[str], cwd: Path, host: str) -> None:
    resolved_command = command_for_host(command, host)
    print("+ " + " ".join(resolved_command))
    subprocess.run(resolved_command, cwd=cwd, check=True)


def display_path(path_value: Path, root: Path) -> Path:
    try:
        return path_value.relative_to(root)
    except ValueError:
        return path_value


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build and run BlueMemory Google Benchmark targets.")

    parser.add_argument(
        "--config",
        default="Release_x64",
        choices=sorted(CONFIG_TO_DIR.keys()),
        help="Blue build configuration.",
    )

    parser.add_argument(
        "--repetitions",
        default=5,
        type=int,
        help="Google Benchmark repetition count.",
    )

    parser.add_argument(
        "--memory-backend",
        default="system",
        choices=["system", "mimalloc"],
        help="BlueMemory backend.",
    )

    parser.add_argument(
        "--benchmark-filter",
        default=None,
        help="Optional Google Benchmark filter regex.",
    )

    parser.add_argument(
        "--output-dir",
        default=None,
        help="Optional output directory. Defaults to out/benchmarks/memory/<config>.",
    )

    parser.add_argument(
        "--no-build",
        action="store_true",
        help="Run an already-built benchmark executable without regenerating/building.",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.repetitions <= 0:
        raise ValueError("--repetitions must be greater than zero")

    root = repo_root()
    host = detect_host()
    config_dir = CONFIG_TO_DIR[args.config]

    output_dir = resolve_under_root(
        root,
        args.output_dir,
        root / "out" / "benchmarks" / "memory" / args.config,
    )
    output_dir.mkdir(parents=True, exist_ok=True)

    json_output = output_dir / "blue_memory_hot_path.json"

    benchmark_target = f"{BENCHMARK_NAME}_{args.config}"
    benchmark_binary = (
        root
        / "out"
        / "bin"
        / HOST_TO_BIN_PLATFORM[host]
        / "x64"
        / config_dir
        / f"{BENCHMARK_NAME}{executable_suffix(host)}"
    )

    if not args.no_build:
        premake = root / "scripts" / HOST_TO_PREMAKE_SCRIPT[host]
        if not premake.exists():
            raise FileNotFoundError(f"Premake script not found: {premake}")

        run_command(
            [
                str(premake),
                "ninja",
                f"--blue-platforms={HOST_TO_PREMAKE_PLATFORM[host]}",
                "--blue-build-platforms=x64",
                f"--memory-backend={args.memory_backend}",
                f"--blue-startup={BENCHMARK_NAME}",
            ],
            cwd=root,
            host=host,
        )

        run_command(
            [
                "ninja",
                "-C",
                str(root / "out" / "build" / "ninja"),
                benchmark_target,
            ],
            cwd=root,
            host=host,
        )

    if not benchmark_binary.exists():
        raise FileNotFoundError(f"Benchmark executable not found: {benchmark_binary}")

    command = [
        str(benchmark_binary),
        f"--benchmark_repetitions={args.repetitions}",
        "--benchmark_report_aggregates_only=true",
        f"--benchmark_out={json_output}",
        "--benchmark_out_format=json",
    ]

    if args.benchmark_filter:
        command.append(f"--benchmark_filter={args.benchmark_filter}")

    run_command(command, cwd=root, host=host)

    print()
    print(f"Benchmark JSON written to: {display_path(json_output, root)}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
