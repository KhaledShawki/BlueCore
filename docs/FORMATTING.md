# Formatting

BlueCore uses a single repository-level `.clang-format` file for all C and C++ code.

## Policy

- There is one formatter configuration at the repository root.
- Nested `.clang-format` or `_clang-format` files are not allowed.
- C/C++ formatting uses `clang-format --style=file`.
- Generated output, third-party code, and tool binaries are excluded.
- CI checks formatting but does not rewrite source files.
- Formatting orchestration is cross-platform Python; no platform formatter wrappers are required.

## Style Guidelines

- Indentation uses tabs.
- Braces follow Allman style.
- Short switch cases may stay on one line.
- Larger switch cases should use explicit braces.

## Tool resolution

The Blue CLI supports explicit tool overrides through CLI options or environment variables:

```text
--format-path / BLUE_CLANG_FORMAT
--lua-format-path / BLUE_STYLUA
--python-format-path / BLUE_BLACK
```

Without an override, the CLI checks repository-local formatter binaries where available and then standard host installations/PATH. Black may also be executed as `python -m black`.

## Usage

```text
python scripts/blue.py format
python scripts/blue.py format-check
python scripts/blue.py list-format-files
```

Premake `format`, `check-format`, and `list-format-files` actions invoke the same Python CLI, so CI, IDE utility projects, and terminal usage share one implementation.

To inspect the resolved clang-format configuration for a specific file:

```text
clang-format --style=file -dump-config modules/BlueSystem/src/Log/Logger.cpp
```