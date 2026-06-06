# Formatting

Blue uses a single repository-level `.clang-format` file for C and C++ formatting.

## Policy

- Keep one formatter configuration at the repository root.
- Do not add nested `.clang-format` or `_clang-format` files.
- Use `clang-format --style=file` so the repository configuration is discovered consistently.
- Do not format generated output, third-party payloads, or tool binaries.
- CI should check formatting; it should not rewrite source files.

## Style summary

- Tabs are used for indentation.
- Braces use Allman style.
- Short switch cases may remain on one line.
- Larger switch cases should use explicit braces in source code.

Example:

```cpp
switch ( level )
{
case LogLevel::Trace: return "Trace";
case LogLevel::Debug: return "Debug";
default: return "Unknown";
}
```

## Tool resolution

Formatter scripts resolve `clang-format` in this order:

1. `BLUE_CLANG_FORMAT`, if set.
2. Repo-local tool under `tools/clang-format/<os>/`.
3. Standard LLVM install locations.
4. Visual Studio LLVM toolchain path on Windows.
5. `clang-format` from `PATH`.

## Usage

Windows:

```cmd
scripts\format-windows.cmd
scripts\format-check-windows.cmd
scripts\list-format-files-windows.cmd
```

Linux:

```bash
./scripts/format-linux.sh
./scripts/format-check-linux.sh
./scripts/list-format-files-linux.sh
```

macOS:

```bash
./scripts/format-macos.sh
./scripts/format-check-macos.sh
./scripts/list-format-files-macos.sh
```

Premake actions:

```cmd
scripts\premake-windows.cmd format
scripts\premake-windows.cmd check-format
scripts\premake-windows.cmd list-format-files
```

## Troubleshooting

Check which formatter configuration is used for a file:

```cmd
clang-format --style=file -dump-config modules\BlueSystem\src\Log\Logger.cpp
```

Search for accidental nested formatter files:

```cmd
dir /s /b .clang-format
dir /s /b _clang-format
```

Only the repository root formatter file should exist.
