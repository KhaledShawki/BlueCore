from __future__ import annotations

import sys
from pathlib import Path
from typing import Sequence

from .build import parse_build_args, parse_regenerate_args, run_build, run_regenerate
from .core import BlueCliError, detect_host, repo_root, run_premake
from .formatting import parse_format_args, run_format
from .testing import parse_test_args, run_tests

PREMAKE_ACTIONS = {
    "add-file": "blue-add-file",
    "remove-file": "blue-remove-file",
    "rename-file": "blue-rename-file",
    "add-project": "blue-add-project",
    "validate": "validate",
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

FORMAT_COMMANDS = {
    "format": "format",
    "format-check": "check",
    "list-format-files": "list",
}


def print_usage() -> None:
    commands = [
        ("build", "Generate and build a native target."),
        ("clean", "Generate and clean a native configuration."),
        ("test", "Generate, build, and run all registered tests."),
        ("regenerate", "Regenerate project files only when the build token is stale."),
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
    print("Run 'python scripts/blue.py <command> --help' for semantic command options.")
    print("Premake-backed commands forward remaining arguments unchanged.")


def dispatch(argv: Sequence[str], *, root: Path, host: str) -> int:
    if not argv or argv[0] in {"-h", "--help", "help"}:
        print_usage()
        return 0

    command = argv[0]
    remaining = list(argv[1:])

    if command == "test":
        return run_tests(root, host, parse_test_args(remaining))

    if command in {"build", "clean"}:
        args = parse_build_args(remaining, command=command)
        return run_build(root, host, args, clean=command == "clean")

    if command == "regenerate":
        action, options = parse_regenerate_args(remaining, host=host)
        return run_regenerate(root, host, action, options)

    format_mode = FORMAT_COMMANDS.get(command)
    if format_mode is not None:
        return run_format(root, host, format_mode, parse_format_args(remaining, command=command))

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
