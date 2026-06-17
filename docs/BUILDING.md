# Building

This document describes how to build BlueCore.

## Requirements

- A C++20 compatible compiler
- [Premake 5](https://premake.github.io/)
- Optional: `clang-format` for code formatting
- Optional: Graphviz for visualizing build dependencies

Place the Premake executable in the platform-specific directory:

```text
tools/premake/windows/premake5.exe
tools/premake/linux/premake5
tools/premake/macos/premake5
```

## Windows

### Validate the build graph

```cmd
scripts\premake-windows.cmd validate
```

### Generate Visual Studio projects

```cmd
scripts\premake-windows.cmd vs2022 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

### Build

```cmd
msbuild out\build\vs2022\Blue.sln /p:Configuration=Debug /p:Platform=Win64
```

A helper script is also available to generate Visual Studio 2026 projects when supported:

```cmd
scripts\generate-vs-windows.cmd
```

## Linux

```bash
chmod +x tools/premake/linux/premake5 scripts/*.sh

./scripts/premake-linux.sh validate
./scripts/premake-linux.sh gmake2 --toolchain=gcc --blue-platforms=linux --blue-startup=BlueRunTests

make -C out/build/gmake2 config=debug_x64
```

## macOS

```bash
chmod +x tools/premake/macos/premake5 scripts/*.sh

./scripts/premake-macos.sh validate
./scripts/premake-macos.sh xcode4 --toolchain=clang --blue-platforms=macos --blue-startup=BlueRunTests
```

## CLion

Generate compilation databases for CLion:

```cmd
scripts\premake-windows.cmd clion --toolchain=msvc --blue-platforms=windows
```

On Linux and macOS, use the corresponding wrapper scripts.

CLion output is written to `out/ide/clion/`. See `docs/CLION.md` for more details.

## Generated Output

All generated files are written under the `out/` directory and should not be committed to version control:

```
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

To use mimalloc instead, place the dependency under `third_party/mimalloc` and generate with:

```text
--memory-backend=mimalloc
```
