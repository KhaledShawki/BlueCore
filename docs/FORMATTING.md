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

Formatter scripts locate `clang-format` in the following order:

1. `BLUE_CLANG_FORMAT` environment variable (if set)
2. Repository-local binary under `tools/clang-format/<os>/`
3. Standard LLVM installation locations
4. Visual Studio LLVM toolchain path (on Windows)
5. `clang-format` available in `PATH`

## Usage

**Windows**

```cmd
scripts\format-windows.cmd
scripts\format-check-windows.cmd
scripts\list-format-files-windows.cmd
```

**Linux**

```bash
./scripts/format-linux.sh
./scripts/format-check-linux.sh
./scripts/list-format-files-linux.sh
```

**macOS**

```bash
./scripts/format-macos.sh
./scripts/format-check-macos.sh
./scripts/list-format-files-macos.sh
```

**Premake Actions**

```cmd
scripts\premake-windows.cmd format
scripts\premake-windows.cmd check-format
scripts\premake-windows.cmd list-format-files
```

## Troubleshooting

To see which configuration is being used for a specific file:

```cmd
clang-format --style=file -dump-config modules\BlueSystem\src\Log\Logger.cpp
```

To check for accidental nested formatter files:

```cmd
dir /s /b .clang-format
dir /s /b _clang-format
```

Only the repository root `.clang-format` file should exist.