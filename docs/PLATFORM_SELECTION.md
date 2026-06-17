# Platform Selection

BlueCore separates target operating system selection from static/shared linkage selection.

## Target Operating System

Use the `--blue-platforms` flag to select the target operating system:

```text
--blue-platforms=auto
--blue-platforms=all
--blue-platforms=windows
--blue-platforms=linux
--blue-platforms=macos
```

For normal development, select one native target OS per generation command.

## Linkage Platform

Linkage is controlled separately using the `x64` and `x64_DLL` platforms:

| Platform   | Description             |
|------------|-------------------------|
| `x64`      | Static libraries        |
| `x64_DLL`  | Shared libraries / DLLs |

Both platforms target the same CPU architecture. The only difference is whether static or shared libraries are produced.

## Examples

**Windows**

```cmd
scripts\premake-windows.cmd vs2026 \
    --toolchain=msvc \
    --blue-platforms=windows \
    --blue-startup=BlueRunTests
```

**Linux**

```bash
./scripts/premake-linux.sh gmake2 \
    --toolchain=gcc \
    --blue-platforms=linux \
    --blue-startup=BlueRunTests
```

**macOS**

```bash
./scripts/premake-macos.sh xcode4 \
    --toolchain=clang \
    --blue-platforms=macos \
    --blue-startup=BlueRunTests
```
