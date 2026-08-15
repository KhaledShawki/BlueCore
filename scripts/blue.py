#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import stat
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

CONFIGURATIONS = ("Debug", "Release", "Profile", "Shipping")
LINKAGES = ("static", "shared")
MEMORY_BACKENDS = ("system", "mimalloc")
TEST_BACKENDS = ("ninja", "gmake", "vs2026")
TEST_BACKEND_ALIASES = {"gmake2": "gmake"}
TEST_MANIFEST_RELATIVE_PATH = Path("out/metadata/BlueTests.json")
TOOLCHAINS = ("clang", "gcc", "msvc")

PREMAKE_ACTIONS = {
    "add-file": "blue-add-file",
    "remove-file": "blue-remove-file",
    "rename-file": "blue-rename-file",
    "add-project": "blue-add-project",
    "validate": "validate",
    "format": "format",
    "format-check": "check-format",
    "list-format-files": "list-format-files",
    "clion": "clion",
    "graph": "graph",
    "metadata": "metadata",
    "list-tests": "list-tests",
    "test-metadata": "test-metadata",
    "list-benchmarks": "list-benchmarks",
    "benchmark-metadata": "benchmark-metadata",
    "build-graph-token": "build-graph-token",
    "check-regeneration": "check-regeneration",
    "update-build-token": "update-build-token",
}

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


@dataclass(frozen=True)
class TestRequest:
    host: str
    configuration: str
    backend: str
    toolchain: str
    linkage: str
    memory_backend: str
    build_platform: str


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


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


def require_command(command: str, install_hint: str | None = None) -> str:
    resolved = shutil.which(command)
    if resolved:
        return resolved

    message = f"Required command not found: {command}"
    if install_hint:
        message += f". {install_hint}"
    raise BlueCliError(message)


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


def normalize_test_backend(value: str) -> str:
    normalized = value.lower()
    normalized = TEST_BACKEND_ALIASES.get(normalized, normalized)
    if normalized in TEST_BACKENDS:
        return normalized

    accepted = ", ".join((*TEST_BACKENDS, *TEST_BACKEND_ALIASES))
    raise argparse.ArgumentTypeError(f"--backend must be one of: {accepted}")


def parse_legacy_test_config(value: str) -> tuple[str, str | None]:
    lowered = value.lower()

    for configuration in CONFIGURATIONS:
        prefix = configuration.lower()

        if lowered == prefix:
            return configuration, None
        if lowered == f"{prefix}_x64":
            return configuration, "static"
        if lowered == f"{prefix}_x64_dll":
            return configuration, "shared"

    raise argparse.ArgumentTypeError(
        "legacy configuration must be Debug, Release, Profile, or Shipping " "with an optional _x64 or _x64_DLL suffix"
    )


def parse_legacy_test_platform(value: str) -> str:
    lowered = value.lower()
    if lowered == "x64":
        return "static"
    if lowered == "x64_dll":
        return "shared"

    raise argparse.ArgumentTypeError("legacy platform must be x64 or x64_DLL")


def parse_test_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="blue test",
        description="Generate, build, and run all registered BlueCore tests for the current host.",
    )
    parser.add_argument(
        "legacy_config",
        nargs="?",
        help=argparse.SUPPRESS,
    )
    parser.add_argument(
        "legacy_platform",
        nargs="?",
        help=argparse.SUPPRESS,
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

    legacy_config = None
    legacy_linkage = None
    if args.legacy_config is not None:
        try:
            legacy_config, legacy_linkage = parse_legacy_test_config(args.legacy_config)
        except argparse.ArgumentTypeError as exc:
            parser.error(str(exc))

    legacy_platform_linkage = None
    if args.legacy_platform is not None:
        try:
            legacy_platform_linkage = parse_legacy_test_platform(args.legacy_platform)
        except argparse.ArgumentTypeError as exc:
            parser.error(str(exc))

    effective_legacy_linkage = legacy_platform_linkage or legacy_linkage

    if args.config is not None and legacy_config is not None and args.config != legacy_config:
        parser.error("--config conflicts with the legacy positional configuration")

    if args.linkage is not None and effective_legacy_linkage is not None and args.linkage != effective_legacy_linkage:
        parser.error("--linkage conflicts with the legacy positional platform")

    args.config = args.config or legacy_config or "Debug"
    args.linkage = args.linkage or effective_legacy_linkage or "static"
    del args.legacy_config
    del args.legacy_platform

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
            raise BlueCliError(f"Toolchain '{toolchain}' is not configured for macOS tests.")
        return make_macos_clang_environment(root)

    if host == "linux":
        if toolchain == "clang":
            require_command("clang++", "Install Clang and ensure clang++ is available in PATH.")
        elif toolchain == "gcc":
            require_command("g++", "Install GCC and ensure g++ is available in PATH.")
        else:
            raise BlueCliError(f"Toolchain '{toolchain}' is not configured for Linux tests.")
        return os.environ.copy()

    raise BlueCliError(f"Unix toolchain setup is not configured for host: {host}")


def run_ninja_tests(root: Path, request: TestRequest) -> int:
    require_command("ninja", "Install Ninja and ensure it is available in PATH.")
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
        ["ninja", "-C", str(root / "out" / "build" / "ninja"), target],
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
    require_command("make", "Install GNU Make and ensure make is available in PATH.")
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
            "make",
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
    require_command(
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
    runner_project = build_root / "BlueRunTests" / "BlueRunTests.vcxproj"
    bin_dir = test_binary_dir(root, request.host, request.build_platform, request.configuration)
    runner = bin_dir / "BlueRunTests.exe"

    if not build_root.is_dir():
        raise BlueCliError(f"Expected VS2026 build directory was not generated: {build_root}")

    if not runner_project.is_file():
        raise BlueCliError(f"Expected test runner project was not generated: {runner_project}")

    test_names = load_registered_test_names(root)
    for test_name in test_names:
        project_file = build_root / test_name / f"{test_name}.vcxproj"
        if not project_file.is_file():
            raise BlueCliError(f"Registered test project was not generated: {project_file}")

        print(f"[BlueBuild] Building {test_name}")
        result = run_command(
            [
                "msbuild",
                str(project_file),
                "/m",
                "/nr:false",
                "/t:Build",
                f"/p:Configuration={request.configuration}",
                f"/p:Platform={request.build_platform}",
                "/v:minimal",
            ],
            cwd=root,
        )
        if result != 0:
            return result

    print(f"[BlueBuild] Built {len(test_names)} registered test projects.")
    print("[BlueBuild] Building BlueRunTests")

    result = run_command(
        [
            "msbuild",
            str(runner_project),
            "/m",
            "/nr:false",
            "/t:Build",
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


def print_usage() -> None:
    commands = [
        ("test", "Generate, build, and run all registered tests."),
        ("validate", "Validate the Blue build graph."),
        ("format", "Format C/C++, Lua, and Python sources."),
        ("format-check", "Check source formatting."),
        ("list-format-files", "List files included in formatting."),
        ("add-file", "Add a file through the Blue manifest command layer."),
        ("remove-file", "Remove a file through the Blue manifest command layer."),
        ("rename-file", "Rename a file through the Blue manifest command layer."),
        ("add-project", "Add a project through the Blue manifest command layer."),
        ("clion", "Generate CLion integration files."),
        ("graph", "Generate the dependency graph."),
        ("metadata", "Generate project metadata."),
        ("list-tests", "List registered test executables."),
        ("test-metadata", "Generate registered test metadata."),
        ("list-benchmarks", "List registered benchmark executables."),
        ("benchmark-metadata", "Generate registered benchmark metadata."),
        ("premake", "Run a raw Premake action through the host bundled executable."),
    ]

    print("Usage: python scripts/blue.py <command> [arguments]")
    print()
    print("Commands:")
    for name, description in commands:
        print(f"  {name:<20} {description}")
    print()
    print("Run 'python scripts/blue.py test --help' for test options.")
    print("Premake-backed commands forward remaining arguments unchanged.")


def dispatch(argv: Sequence[str], *, root: Path, host: str) -> int:
    if not argv or argv[0] in {"-h", "--help", "help"}:
        print_usage()
        return 0

    command = argv[0]
    remaining = list(argv[1:])

    if command == "test":
        return run_tests(root, host, parse_test_args(remaining))

    if command == "premake":
        if not remaining:
            raise BlueCliError("The premake command requires an action or arguments.")
        return run_premake(root, host, remaining)

    premake_action = PREMAKE_ACTIONS.get(command)
    if premake_action is not None:
        return run_premake(root, host, [premake_action, *remaining])

    print(f"Unknown Blue command: {command}", file=sys.stderr)
    print("Run 'python scripts/blue.py --help' to list available commands.", file=sys.stderr)
    return 2


def main(argv: Sequence[str] | None = None) -> int:
    arguments = list(sys.argv[1:] if argv is None else argv)

    try:
        return dispatch(arguments, root=repo_root(), host=detect_host())
    except BlueCliError as exc:
        print(f"[BlueBuild] {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
