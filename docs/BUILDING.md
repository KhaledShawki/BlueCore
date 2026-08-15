# Building

This document describes how to build BlueCore.

## Requirements

- Python 3.11 or newer
- A C++20 compatible compiler
- The bundled Premake executable for the current host under `tools/premake/<os>/`
- Backend tools required by the selected generator, such as Ninja, GNU Make, or MSBuild
- Optional: `clang-format` for code formatting
- Optional: Graphviz for visualizing build dependencies

The developer-facing build entry point is:

```text
python scripts/blue.py <command> [arguments]
```

## Windows

Validate the build graph:

```cmd
python scripts\blue.py validate
```

Generate Visual Studio 2026 projects:

```cmd
python scripts\blue.py premake vs2026 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

Build:

```cmd
msbuild out\build\vs2026\Blue.slnx /m /p:Configuration=Debug /p:Platform=x64
```

## Linux

Run tests with the default Ninja + Clang configuration:

```bash
python scripts/blue.py test --config=Debug
```

Generate GNU Make files explicitly:

```bash
python scripts/blue.py premake gmake --toolchain=gcc --blue-platforms=linux --blue-startup=BlueRunTests
make -C out/build/gmake config=debug_x64
```

## macOS

Run tests with the default Ninja + Apple Clang configuration:

```bash
python scripts/blue.py test --config=Debug
```

Generate a Ninja build explicitly:

```bash
python scripts/blue.py premake ninja --toolchain=clang --blue-platforms=macos --blue-build-platforms=x64 --blue-startup=BlueRunTests
```

## CLion

Generate CLion integration files:

```text
python scripts/blue.py clion --toolchain=<toolchain> --blue-platforms=<platform>
```

CLion output is written to `out/ide/clion/`. See `docs/CLION.md` for more details.

## Generated Output

All generated files are written under the `out/` directory and should not be committed to version control:

```text
out/build/    Generated project files
out/bin/      Executables and libraries
out/obj/      Object files
out/ide/      IDE helper files (e.g. CLion compile_commands.json)
```

## Memory Backend Selection

The default memory backend is the system allocator:

```text
--memory-backend=system
```

To use mimalloc instead, place the dependency under `third_party/mimalloc` and pass:

```text
--memory-backend=mimalloc
```
