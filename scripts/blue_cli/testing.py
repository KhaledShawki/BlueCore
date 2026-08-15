from __future__ import annotations

import argparse
import json
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

from .build import find_visual_studio_solution
from .core import (
    BlueCliError,
    CONFIGURATIONS,
    HOST_TO_BIN_SYSTEM,
    HOST_TO_BLUE_PLATFORM,
    LINKAGES,
    normalize_choice,
    MEMORY_BACKENDS,
    TOOLCHAINS,
    require_command,
    require_gnu_make,
    run_command,
    run_premake,
)
from .toolchains import prepare_unix_toolchain_environment

TEST_BACKENDS = ("ninja", "gmake", "vs2026")
TEST_BACKEND_ALIASES = {"gmake2": "gmake"}
TEST_MANIFEST_RELATIVE_PATH = Path("out/metadata/BlueTests.json")


@dataclass(frozen=True)
class TestRequest:
    host: str
    configuration: str
    backend: str
    toolchain: str
    linkage: str
    memory_backend: str
    build_platform: str


def normalize_test_backend(value: str) -> str:
    normalized = value.lower()
    normalized = TEST_BACKEND_ALIASES.get(normalized, normalized)
    if normalized in TEST_BACKENDS:
        return normalized

    accepted = ", ".join((*TEST_BACKENDS, *TEST_BACKEND_ALIASES))
    raise argparse.ArgumentTypeError(f"--backend must be one of: {accepted}")


def parse_test_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="blue test",
        description="Generate, build, and run all registered BlueCore tests for the current host.",
    )
    parser.add_argument(
        "--config",
        default=None,
        type=lambda value: normalize_choice(value, CONFIGURATIONS, "--config"),
        metavar="CONFIG",
        help="Build configuration: Debug, Release, Profile, or Shipping (default: Debug).",
    )
    parser.add_argument(
        "--linkage",
        default=None,
        type=lambda value: normalize_choice(value, LINKAGES, "--linkage"),
        metavar="LINKAGE",
        help="Library linkage: static or shared (default: static).",
    )
    parser.add_argument(
        "--backend",
        default=None,
        type=normalize_test_backend,
        metavar="BACKEND",
        help=(
            "Build backend: ninja, gmake, or vs2026. "
            "The legacy gmake2 name is accepted as an alias for gmake. "
            "Defaults to Ninja for static Unix builds, gmake for shared Unix builds, and vs2026 on Windows."
        ),
    )
    parser.add_argument(
        "--toolchain",
        default=None,
        type=lambda value: normalize_choice(value, TOOLCHAINS, "--toolchain"),
        metavar="TOOLCHAIN",
        help=(
            "Compiler toolchain: clang, gcc, or msvc. "
            "Defaults to clang for macOS/Ninja, gcc for Linux/gmake, and msvc for Windows."
        ),
    )
    parser.add_argument(
        "--memory-backend",
        default="system",
        type=lambda value: normalize_choice(value, MEMORY_BACKENDS, "--memory-backend"),
        metavar="BACKEND",
        help="BlueMemory backend: system or mimalloc (default: system).",
    )

    args = parser.parse_args(list(argv))
    args.config = args.config or "Debug"
    args.linkage = args.linkage or "static"
    return args


def resolve_test_backend(host: str, linkage: str, requested_backend: str | None) -> str:
    if host == "windows":
        backend = requested_backend or "vs2026"
        if backend != "vs2026":
            raise BlueCliError(f"Backend '{backend}' is not configured for Windows tests; use --backend=vs2026.")
        return backend

    if host not in {"linux", "macos"}:
        raise BlueCliError(f"Test execution is not configured for host: {host}")

    backend = requested_backend or ("gmake" if linkage == "shared" else "ninja")
    if backend == "vs2026":
        raise BlueCliError(f"Backend 'vs2026' is not available on {host}; use --backend=ninja or --backend=gmake.")

    if backend == "ninja" and linkage == "shared":
        raise BlueCliError(
            "Ninja does not support BlueCore shared/x64_DLL builds. "
            "Use --backend=gmake or omit --backend so Blue selects gmake automatically."
        )

    return backend


def resolve_test_toolchain(host: str, backend: str, requested_toolchain: str | None) -> str:
    if host == "windows":
        toolchain = requested_toolchain or "msvc"
        if toolchain != "msvc":
            raise BlueCliError(
                f"Toolchain '{toolchain}' is not configured for Windows {backend} tests; use --toolchain=msvc."
            )
        return toolchain

    if host == "macos":
        toolchain = requested_toolchain or "clang"
        if toolchain != "clang":
            raise BlueCliError(f"Toolchain '{toolchain}' is not available for macOS tests; use --toolchain=clang.")
        return toolchain

    if host == "linux":
        default_toolchain = "gcc" if backend == "gmake" else "clang"
        toolchain = requested_toolchain or default_toolchain
        if toolchain not in {"clang", "gcc"}:
            raise BlueCliError(
                f"Toolchain '{toolchain}' is not available for Linux tests; use --toolchain=clang or --toolchain=gcc."
            )
        return toolchain

    raise BlueCliError(f"Test toolchain resolution is not configured for host: {host}")


def build_platform_for(backend: str, linkage: str) -> str:
    if backend == "ninja":
        if linkage != "static":
            raise BlueCliError("Ninja only supports the static x64 Blue build platform.")
        return "x64"

    return "x64_DLL" if linkage == "shared" else "x64"


def resolve_test_request(host: str, args: argparse.Namespace) -> TestRequest:
    backend = resolve_test_backend(host, args.linkage, args.backend)
    toolchain = resolve_test_toolchain(host, backend, args.toolchain)
    build_platform = build_platform_for(backend, args.linkage)
    return TestRequest(
        host=host,
        configuration=args.config,
        backend=backend,
        toolchain=toolchain,
        linkage=args.linkage,
        memory_backend=args.memory_backend,
        build_platform=build_platform,
    )


def test_binary_dir(root: Path, host: str, build_platform: str, configuration: str) -> Path:
    try:
        system_name = HOST_TO_BIN_SYSTEM[host]
    except KeyError as exc:
        raise BlueCliError(f"Unsupported Blue host: {host}") from exc

    return root / "out" / "bin" / system_name / build_platform / configuration


def is_runnable_file(path: Path, host: str) -> bool:
    if not path.is_file():
        return False
    if host == "windows":
        return True
    return os.access(path, os.X_OK)


def test_manifest_path(root: Path) -> Path:
    return root / TEST_MANIFEST_RELATIVE_PATH


def load_registered_test_names(root: Path) -> list[str]:
    manifest = test_manifest_path(root)
    if not manifest.is_file():
        raise BlueCliError(f"Registered test manifest was not generated: {manifest}")

    try:
        payload = json.loads(manifest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise BlueCliError(f"Could not read registered test manifest: {manifest}: {exc}") from exc

    entries = payload.get("tests")
    if not isinstance(entries, list):
        raise BlueCliError(f"Registered test manifest has no tests array: {manifest}")

    names: list[str] = []
    seen: set[str] = set()
    for entry in entries:
        name = entry.get("name") if isinstance(entry, dict) else None
        if not isinstance(name, str) or not name:
            raise BlueCliError(f"Registered test manifest contains an invalid test entry: {manifest}")
        if name in seen:
            raise BlueCliError(f"Registered test manifest contains duplicate test name: {name}")
        seen.add(name)
        names.append(name)

    if not names:
        raise BlueCliError(f"No registered tests were found in {manifest}")

    return names


def registered_test_executables(root: Path, request: TestRequest, names: Sequence[str]) -> list[Path]:
    bin_dir = test_binary_dir(root, request.host, request.build_platform, request.configuration)
    extension = ".exe" if request.host == "windows" else ""
    executables = [bin_dir / f"{name}{extension}" for name in names]

    missing = [path for path in executables if not is_runnable_file(path, request.host)]
    if missing:
        formatted = ", ".join(str(path) for path in missing)
        raise BlueCliError(f"Registered test executable was not built or is not runnable: {formatted}")

    return executables


def premake_test_generation_args(request: TestRequest) -> list[str]:
    return [
        request.backend,
        f"--toolchain={request.toolchain}",
        f"--blue-platforms={HOST_TO_BLUE_PLATFORM[request.host]}",
        f"--blue-build-platforms={request.build_platform}",
        f"--memory-backend={request.memory_backend}",
        "--blue-startup=BlueRunTests",
        f"--blue-test-manifest={TEST_MANIFEST_RELATIVE_PATH.as_posix()}",
        "--blue-test-postbuild=off",
    ]


def run_ninja_tests(root: Path, request: TestRequest) -> int:
    ninja = require_command("ninja", "Install Ninja and ensure it is available in PATH.")
    env = prepare_unix_toolchain_environment(root, request.host, request.toolchain)

    target = f"BlueRunTests_{request.configuration}_{request.build_platform}"
    bin_dir = test_binary_dir(root, request.host, request.build_platform, request.configuration)
    runner = bin_dir / "BlueRunTests"

    print(f"[BlueBuild] Configuration      : {request.configuration}_{request.build_platform}")
    print(f"[BlueBuild] Build backend      : {request.backend}")
    print(f"[BlueBuild] Toolchain          : {request.toolchain}")
    print(f"[BlueBuild] Memory backend     : {request.memory_backend}")
    print("[BlueBuild] Generating Ninja build graph")

    result = run_premake(
        root,
        request.host,
        premake_test_generation_args(request),
        env=env,
    )
    if result != 0:
        return result

    print("[BlueBuild] Building all test executables")
    result = run_command(
        [ninja, "-C", str(root / "out" / "build" / "ninja"), target],
        cwd=root,
        env=env,
    )
    if result != 0:
        return result

    if not is_runnable_file(runner, request.host):
        raise BlueCliError(f"Test runner was not built or is not executable: {runner}")

    test_names = load_registered_test_names(root)
    tests = registered_test_executables(root, request, test_names)

    print(f"[BlueBuild] Running {len(tests)} registered test executables")
    return run_command([str(runner), "--jobs=auto", *(str(test) for test in tests)], cwd=root, env=env)


def run_gmake_tests(root: Path, request: TestRequest) -> int:
    make = require_gnu_make()
    env = prepare_unix_toolchain_environment(root, request.host, request.toolchain)

    make_config = f"{request.configuration.lower()}_{request.build_platform.lower()}"
    build_root = root / "out" / "build" / "gmake"
    bin_dir = test_binary_dir(root, request.host, request.build_platform, request.configuration)
    runner = bin_dir / "BlueRunTests"

    print(f"[BlueBuild] Configuration      : {request.configuration}_{request.build_platform}")
    print(f"[BlueBuild] Build backend      : {request.backend}")
    print(f"[BlueBuild] Toolchain          : {request.toolchain}")
    print(f"[BlueBuild] Memory backend     : {request.memory_backend}")
    print("[BlueBuild] Generating GNU Make build graph")

    result = run_premake(
        root,
        request.host,
        premake_test_generation_args(request),
        env=env,
    )
    if result != 0:
        return result

    if not build_root.is_dir():
        raise BlueCliError(f"Expected gmake build directory was not generated: {build_root}")

    print("[BlueBuild] Building all test executables")
    jobs = max(1, os.cpu_count() or 1)
    result = run_command(
        [
            make,
            "-C",
            str(build_root),
            f"-j{jobs}",
            f"config={make_config}",
            "BlueRunTests",
        ],
        cwd=root,
        env=env,
    )
    if result != 0:
        return result

    if not is_runnable_file(runner, request.host):
        raise BlueCliError(f"Test runner was not built or is not executable: {runner}")

    test_names = load_registered_test_names(root)
    tests = registered_test_executables(root, request, test_names)

    print(f"[BlueBuild] Running {len(tests)} registered test executables")
    return run_command([str(runner), "--jobs=auto", *(str(test) for test in tests)], cwd=root, env=env)


def run_windows_tests(root: Path, request: TestRequest) -> int:
    msbuild = require_command(
        "msbuild",
        "Run from a Visual Studio Developer Command Prompt or ensure MSBuild is available in PATH.",
    )

    print(f"[BlueBuild] Configuration      : {request.configuration}_{request.build_platform}")
    print(f"[BlueBuild] Build backend      : {request.backend}")
    print(f"[BlueBuild] Toolchain          : {request.toolchain}")
    print(f"[BlueBuild] Memory backend     : {request.memory_backend}")
    print("[BlueBuild] Generating VS2026 solution")

    result = run_premake(
        root,
        "windows",
        premake_test_generation_args(request),
    )
    if result != 0:
        return result

    build_root = root / "out" / "build" / request.backend
    bin_dir = test_binary_dir(root, request.host, request.build_platform, request.configuration)
    runner = bin_dir / "BlueRunTests.exe"

    if not build_root.is_dir():
        raise BlueCliError(f"Expected VS2026 build directory was not generated: {build_root}")

    solution = find_visual_studio_solution(build_root)
    test_names = load_registered_test_names(root)

    print("[BlueBuild] Building all test executables through BlueRunTests")
    result = run_command(
        [
            msbuild,
            str(solution),
            "/m",
            "/nr:false",
            r"/t:Tests\Runner\BlueRunTests:Build",
            f"/p:Configuration={request.configuration}",
            f"/p:Platform={request.build_platform}",
            "/v:minimal",
        ],
        cwd=root,
    )
    if result != 0:
        return result

    if not runner.is_file():
        raise BlueCliError(f"Test runner executable was not built: {runner}")

    tests = registered_test_executables(root, request, test_names)

    print(f"[BlueBuild] Running {len(tests)} registered test executables")
    return run_command([str(runner), "--jobs=auto", *(str(test) for test in tests)], cwd=root)


def run_tests(root: Path, host: str, args: argparse.Namespace) -> int:
    request = resolve_test_request(host, args)

    if request.backend == "vs2026":
        return run_windows_tests(root, request)
    if request.backend == "gmake":
        return run_gmake_tests(root, request)

    return run_ninja_tests(root, request)
