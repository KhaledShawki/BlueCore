from __future__ import annotations

import argparse
import os
from dataclasses import dataclass, replace
from pathlib import Path
from typing import Sequence

from .context import BUILD_PLATFORMS, BuildContext, linkage_for_build_platform
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

from .sanitizers import (
    parse_sanitizer_set,
    sanitizer_output_root,
    sanitizer_value,
    plan_sanitizer_variants,
    validate_sanitizer_request,
)

from .toolchains import find_msbuild, prepare_unix_toolchain_environment

BUILD_BACKENDS = ("ninja", "gmake", "vs2026")
PREMAKE_VALUE_OPTIONS = {
    "--toolchain",
    "--blue-platforms",
    "--blue-build-platforms",
    "--memory-backend",
    "--sanitizer",
    "--blue-startup",
    "--msvc-toolset",
    "--msvc-tools-version",
}


@dataclass(frozen=True)
class BuildRequest:
    context: BuildContext
    backend: str
    target: str | None

    @property
    def host(self) -> str:
        return self.context.host

    @property
    def config(self) -> str:
        return self.context.configuration

    @property
    def platform(self) -> str:
        return self.context.build_platform

    @property
    def toolchain(self) -> str:
        return self.context.toolchain

    @property
    def memory_backend(self) -> str:
        return self.context.memory_backend

    @property
    def sanitizer(self) -> tuple[str, ...]:
        return self.context.sanitizer

    def with_sanitizer(self, sanitizers: Sequence[str]) -> "BuildRequest":
        return replace(self, context=self.context.with_sanitizer(sanitizers))


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
    parser.add_argument(
        "--sanitizer",
        default=(),
        type=parse_sanitizer_set,
        metavar="SANITIZERS",
        help="Sanitizers: asan, ubsan, tsan, or a comma-separated combination.",
    )
    return parser.parse_args(list(argv))


def resolve_build_request(host: str, args: argparse.Namespace) -> BuildRequest:
    backend = args.backend or default_build_backend(host)
    toolchain = args.toolchain or default_build_toolchain(host)
    context = BuildContext(
        host=host,
        configuration=args.config,
        linkage=linkage_for_build_platform(args.platform),
        toolchain=toolchain,
        memory_backend=args.memory_backend,
        sanitizer=args.sanitizer,
    )

    if host == "windows":
        if backend != "vs2026":
            raise BlueCliError("Windows builds currently require --backend=vs2026.")
        if toolchain != "msvc":
            raise BlueCliError("Windows builds currently require --toolchain=msvc.")
    else:
        if backend == "vs2026":
            raise BlueCliError(f"Backend 'vs2026' is not available on {host}; use --backend=ninja or --backend=gmake.")
        if backend == "ninja" and context.build_platform != "x64":
            raise BlueCliError("Ninja only supports the static x64 Blue build platform.")
        if host == "macos" and toolchain != "clang":
            raise BlueCliError("macOS builds currently require --toolchain=clang.")
        if host == "linux" and toolchain not in {"clang", "gcc"}:
            raise BlueCliError("Linux builds require --toolchain=clang or --toolchain=gcc.")

    return BuildRequest(
        context=context,
        backend=backend,
        target=args.target,
    )


def build_generation_args(request: BuildRequest) -> list[str]:
    context = request.context
    return [
        request.backend,
        f"--toolchain={context.toolchain}",
        f"--blue-platforms={HOST_TO_BLUE_PLATFORM[context.host]}",
        f"--blue-build-platforms={context.build_platform}",
        f"--memory-backend={context.memory_backend}",
        f"--sanitizer={sanitizer_value(context.sanitizer)}",
    ]


def find_visual_studio_solution(build_root: Path) -> Path:
    candidates = sorted([*build_root.glob("*.slnx"), *build_root.glob("*.sln")])
    if len(candidates) != 1:
        raise BlueCliError(f"Expected exactly one Visual Studio solution under {build_root}; found {len(candidates)}.")
    return candidates[0]


def run_ninja_build(root: Path, request: BuildRequest, *, clean: bool) -> int:
    context = request.context
    ninja = require_command("ninja", "Install Ninja and ensure it is available in PATH.")
    env = prepare_unix_toolchain_environment(root, context.host, context.toolchain)

    result = run_premake(root, context.host, build_generation_args(request), env=env)
    if result != 0:
        return result

    build_root = sanitizer_output_root(root, context.sanitizer) / "build" / "ninja"
    if not build_root.is_dir():
        raise BlueCliError(f"Expected Ninja build directory was not generated: {build_root}")

    target = f"{request.target}_{context.configuration}_{context.build_platform}" if request.target else None
    if clean:
        configuration_target = f"{context.configuration}_{context.build_platform}"
        command = [ninja, "-C", str(build_root), "-t", "clean", configuration_target]
    else:
        command = [ninja, "-C", str(build_root)]
        if target:
            command.append(target)

    return run_command(command, cwd=root, env=env)


def run_gmake_build(root: Path, request: BuildRequest, *, clean: bool) -> int:
    context = request.context
    make = require_gnu_make()

    env = prepare_unix_toolchain_environment(root, context.host, context.toolchain)
    result = run_premake(root, context.host, build_generation_args(request), env=env)
    if result != 0:
        return result

    build_root = sanitizer_output_root(root, context.sanitizer) / "build" / "gmake"
    if not build_root.is_dir():
        raise BlueCliError(f"Expected gmake build directory was not generated: {build_root}")

    config_key = f"{context.configuration.lower()}_{context.build_platform.lower()}"
    command = [make, "-C", str(build_root), f"config={config_key}"]
    if clean:
        command.append("clean")
    elif request.target:
        command.append(request.target)

    return run_command(command, cwd=root, env=env)


def run_windows_build(root: Path, request: BuildRequest, *, clean: bool) -> int:
    context = request.context
    msbuild = find_msbuild()

    result = run_premake(root, context.host, build_generation_args(request))
    if result != 0:
        return result

    build_root = sanitizer_output_root(root, context.sanitizer) / "build" / request.backend
    if not build_root.is_dir():
        raise BlueCliError(f"Expected Visual Studio build directory was not generated: {build_root}")

    solution = find_visual_studio_solution(build_root)
    target = "Clean" if clean else request.target
    command = [
        msbuild,
        str(solution),
        "/m",
        "/nr:false",
        f"/p:Configuration={context.configuration}",
        f"/p:Platform={context.build_platform}",
        "/v:minimal",
    ]
    if target:
        command.insert(4, f"/t:{target}")

    return run_command(command, cwd=root)


def run_build_variant(
    root: Path,
    request: BuildRequest,
    *,
    clean: bool,
) -> int:
    if request.context.host == "windows":
        return run_windows_build(root, request, clean=clean)

    if request.backend == "ninja":
        return run_ninja_build(root, request, clean=clean)

    return run_gmake_build(root, request, clean=clean)


def run_build(
    root: Path,
    host: str,
    args: argparse.Namespace,
    *,
    clean: bool = False,
) -> int:
    request = resolve_build_request(host, args)

    validate_sanitizer_request(
        request.context.host,
        request.context.toolchain,
        request.context.sanitizer,
    )

    variants = plan_sanitizer_variants(request.context.sanitizer)

    for variant in variants:
        variant_request = request.with_sanitizer(variant)

        if len(variants) > 1:
            print("[BlueBuild] Sanitizer variant    : " f"{sanitizer_value(variant)}")

        result = run_build_variant(
            root,
            variant_request,
            clean=clean,
        )

        if result != 0:
            return result

    return 0


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
