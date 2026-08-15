# Formatting

BlueCore uses a single repository-level `.clang-format` file for all C and C++ code.

## Policy

- There is one formatter configuration at the repository root.
- Nested `.clang-format` or `_clang-format` files are not allowed.
- All formatting uses `clang-format --style=file` so the root configuration is discovered consistently.
- Generated output, third-party code, and tool binaries are not formatted.
- CI checks formatting but does not automatically rewrite source files.

## Style Guidelines

- Indentation uses tabs.
- Braces follow Allman style.
- Short switch cases may stay on one line.
- Larger switch cases should use explicit braces.

Example:

```cpp
switch ( level )
{
case LogLevel::Trace: return "Trace";
case LogLevel::Debug: return "Debug";
default: return "Unknown";
}
```

## Tool Resolution

Formatter implementations locate `clang-format` in the following order:

1. `BLUE_CLANG_FORMAT` environment variable (if set)
2. Repository-local binary under `tools/clang-format/<os>/`
3. Standard LLVM installation locations
4. Visual Studio LLVM toolchain path (on Windows)
5. `clang-format` available in `PATH`

## Usage

The public formatting interface is cross-platform:

```text
python scripts/blue.py format
python scripts/blue.py format-check
python scripts/blue.py list-format-files
```

Premake dispatches those actions to two internal implementations:

```text
scripts/format-unix.sh
scripts/format-windows.ps1
```

## Troubleshooting

To see which configuration is being used for a specific file:

```text
clang-format --style=file -dump-config modules/BlueSystem/src/Log/Logger.cpp
```

Only the repository root `.clang-format` file should exist.