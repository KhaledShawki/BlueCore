from __future__ import annotations

import argparse
import os
from pathlib import Path
from typing import Sequence

from .core import (
    BlueCliError,
    CONFIGURATIONS,
    HOST_TO_BLUE_PLATFORM,
    MEMORY_BACKENDS,
    normalize_choice,
    TOOLCHAINS,
    require_command,
    require_gnu_make,
    run_command,
    run_premake,
)
from .toolchains import prepare_unix_toolchain_environment

BUILD_PLATFORMS = ("x64", "x64_DLL")
BUILD_BACKENDS = ("ninja", "gmake", "vs2026")
PREMAKE_VALUE_OPTIONS = {
    "--toolchain",
    "--blue-platforms",
    "--blue-build-platforms",
    "--memory-backend",
    "--blue-startup",
    "--msvc-toolset",
    "--msvc-tools-version",
}


def normalize_premake_options(arguments: Sequence[str]) -> list[str]:
    result: list[str] = []
    index = 0
    values = list(arguments)

    while index < len(values):
        argument = values[index]
        if argument in PREMAKE_VALUE_OPTIONS:
            if index + 1 >= len(values):
                raise BlueCliError(f"Missing value for option {argument}.")
            result.append(f"{argument}={values[index + 1]}")
            index += 2
            continue

        result.append(argument)
        index += 1

    return result


def default_build_backend(host: str) -> str:
    return "vs2026" if host == "windows" else "gmake"


def default_build_toolchain(host: str) -> str:
    if host == "windows":
        return "msvc"
    return os.environ.get("BLUE_TOOLCHAIN", "clang").lower()


def parse_build_args(argv: Sequence[str], *, command: str) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog=f"blue {command}",
        description=(
            "Generate the native build graph and build a BlueCore target."
            if command == "build"
            else "Generate the native build graph and clean a BlueCore configuration."
        ),
    )
    if command == "build":
        parser.add_argument(
            "target",
            nargs="?",
            default=None,
            help="Premake target name. Defaults to the workspace target.",
        )
    else:
        parser.set_defaults(target=None)
    parser.add_argument(
        "--config",
        default="Debug",
        type=lambda value: normalize_choice(value, CONFIGURATIONS, "--config"),
        metavar="CONFIG",
    )
    parser.add_argument(
        "--platform",
        default="x64",
        type=lambda value: normalize_choice(value, BUILD_PLATFORMS, "--platform"),
        metavar="PLATFORM",
    )
    parser.add_argument(
        "--backend",
        default=None,
        type=lambda value: normalize_choice(value, BUILD_BACKENDS, "--backend"),
        metavar="BACKEND",
        help=(
            "Native backend. Defaults to gmake on Unix and vs2026 on Windows. "
            "Ninja is supported for static x64 Unix builds."
        ),
    )
    parser.add_argument(
        "--toolchain",
        default=None,
        type=lambda value: normalize_choice(value, TOOLCHAINS, "--toolchain"),
        metavar="TOOLCHAIN",
    )
    parser.add_argument(
        "--memory-backend",
        default=os.environ.get("BLUE_MEMORY_BACKEND", "system"),
        type=lambda value: normalize_choice(value, MEMORY_BACKENDS, "--memory-backend"),
        metavar="BACKEND",
    )
    return parser.parse_args(list(argv))


def resolve_build_request(host: str, args: argparse.Namespace) -> argparse.Namespace:
    args.backend = args.backend or default_build_backend(host)
    args.toolchain = args.toolchain or default_build_toolchain(host)

    if host == "windows":
        if args.backend != "vs2026":
            raise BlueCliError("Windows builds currently require --backend=vs2026.")
        if args.toolchain != "msvc":
            raise BlueCliError("Windows builds currently require --toolchain=msvc.")
    else:
        if args.backend == "vs2026":
            raise BlueCliError(f"Backend 'vs2026' is not available on {host}; use --backend=ninja or --backend=gmake.")
        if args.backend == "ninja" and args.platform != "x64":
            raise BlueCliError("Ninja only supports the static x64 Blue build platform.")
        if host == "macos" and args.toolchain != "clang":
            raise BlueCliError("macOS builds currently require --toolchain=clang.")
        if host == "linux" and args.toolchain not in {"clang", "gcc"}:
            raise BlueCliError("Linux builds require --toolchain=clang or --toolchain=gcc.")

    return args


def build_generation_args(host: str, args: argparse.Namespace) -> list[str]:
    return [
        args.backend,
        f"--toolchain={args.toolchain}",
        f"--blue-platforms={HOST_TO_BLUE_PLATFORM[host]}",
        f"--blue-build-platforms={args.platform}",
        f"--memory-backend={args.memory_backend}",
    ]


def find_visual_studio_solution(build_root: Path) -> Path:
    candidates = sorted([*build_root.glob("*.slnx"), *build_root.glob("*.sln")])
    if len(candidates) != 1:
        raise BlueCliError(f"Expected exactly one Visual Studio solution under {build_root}; found {len(candidates)}.")
    return candidates[0]


def run_ninja_build(root: Path, host: str, args: argparse.Namespace, *, clean: bool) -> int:
    ninja = require_command("ninja", "Install Ninja and ensure it is available in PATH.")
    env = prepare_unix_toolchain_environment(root, host, args.toolchain)

    result = run_premake(root, host, build_generation_args(host, args), env=env)
    if result != 0:
        return result

    build_root = root / "out" / "build" / "ninja"
    if not build_root.is_dir():
        raise BlueCliError(f"Expected Ninja build directory was not generated: {build_root}")

    target = f"{args.target}_{args.config}_{args.platform}" if args.target else None
    if clean:
        configuration_target = f"{args.config}_{args.platform}"
        command = [ninja, "-C", str(build_root), "-t", "clean", configuration_target]
    else:
        command = [ninja, "-C", str(build_root)]
        if target:
            command.append(target)

    return run_command(command, cwd=root, env=env)


def run_gmake_build(root: Path, host: str, args: argparse.Namespace, *, clean: bool) -> int:
    make = require_gnu_make()

    env = prepare_unix_toolchain_environment(root, host, args.toolchain)
    result = run_premake(root, host, build_generation_args(host, args), env=env)
    if result != 0:
        return result

    build_root = root / "out" / "build" / "gmake"
    if not build_root.is_dir():
        raise BlueCliError(f"Expected gmake build directory was not generated: {build_root}")

    config_key = f"{args.config.lower()}_{args.platform.lower()}"
    command = [make, "-C", str(build_root), f"config={config_key}"]
    if clean:
        command.append("clean")
    elif args.target:
        command.append(args.target)

    return run_command(command, cwd=root, env=env)


def run_windows_build(root: Path, args: argparse.Namespace, *, clean: bool) -> int:
    msbuild = require_command(
        "msbuild",
        "Run from a Visual Studio Developer Command Prompt or ensure MSBuild is available in PATH.",
    )

    result = run_premake(root, "windows", build_generation_args("windows", args))
    if result != 0:
        return result

    build_root = root / "out" / "build" / args.backend
    if not build_root.is_dir():
        raise BlueCliError(f"Expected Visual Studio build directory was not generated: {build_root}")

    solution = find_visual_studio_solution(build_root)
    target = "Clean" if clean else args.target
    command = [
        msbuild,
        str(solution),
        "/m",
        "/nr:false",
        f"/p:Configuration={args.config}",
        f"/p:Platform={args.platform}",
        "/v:minimal",
    ]
    if target:
        command.insert(4, f"/t:{target}")

    return run_command(command, cwd=root)


def run_build(root: Path, host: str, args: argparse.Namespace, *, clean: bool = False) -> int:
    args = resolve_build_request(host, args)
    if host == "windows":
        return run_windows_build(root, args, clean=clean)
    if args.backend == "ninja":
        return run_ninja_build(root, host, args, clean=clean)
    return run_gmake_build(root, host, args, clean=clean)


def default_regeneration_action(host: str) -> str:
    if host == "windows":
        return "vs2026"
    return "ninja"


def parse_regenerate_args(argv: Sequence[str], *, host: str) -> tuple[str, list[str]]:
    arguments = list(argv)
    action = default_regeneration_action(host)
    if arguments and not arguments[0].startswith("-"):
        action = arguments.pop(0)
    return action, normalize_premake_options(arguments)


def run_regenerate(root: Path, host: str, action: str, premake_options: Sequence[str]) -> int:
    options = list(premake_options)
    check_args = [f"--regen-action={action}", *options, "check-regeneration"]
    print(f"[BlueBuild] Checking build graph token for {action}...")
    result = run_premake(root, host, check_args)

    if result == 0:
        print("[BlueBuild] Regeneration skipped.")
        return 0
    if result != 2:
        print(f"[BlueBuild] Regeneration check failed with code {result}.")
        return result

    print(f"[BlueBuild] Regenerating {action}...")
    result = run_premake(root, host, [*options, action])
    if result != 0:
        return result

    return run_premake(root, host, [f"--regen-action={action}", *options, "update-build-token"])
