# Platform Selection

Blue separates target operating system selection from static/shared linkage selection.

## Target OS selection

```text
--blue-platforms=auto
--blue-platforms=all
--blue-platforms=windows
--blue-platforms=linux
--blue-platforms=macos
```

Use one native target OS per generation command for normal development.

## Linkage platform selection

```text
x64      Static libraries
x64_DLL  Shared libraries / DLLs
```

Both platforms target the same CPU architecture. The difference is linkage.

## Examples

Windows:

```cmd
scripts\premake-windows.cmd vs2026 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

Linux:

```bash
./scripts/premake-linux.sh gmake2 --toolchain=gcc --blue-platforms=linux --blue-startup=BlueRunTests
```

macOS:

```bash
./scripts/premake-macos.sh xcode4 --toolchain=clang --blue-platforms=macos --blue-startup=BlueRunTests
```
