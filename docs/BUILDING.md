# Building

## Requirements

- C++20 compiler
- Premake 5
- Optional: `clang-format`
- Optional: Graphviz for dependency graph rendering

Place Premake in the OS-specific tool folder:

```text
tools/premake/windows/premake5.exe
tools/premake/linux/premake5
tools/premake/macos/premake5
```

## Windows

Validate the build graph:

```cmd
scripts\premake-windows.cmd validate
```

Generate Visual Studio 2022 projects:

```cmd
scripts\premake-windows.cmd vs2022 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

Build:

```cmd
msbuild out\build\vs2022\Blue.sln /p:Configuration=Debug /p:Platform=Win64
```

Generate the Visual Studio 2026 `.slnx` workflow when supported by the installed Premake generator:

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

## Generated output

Generated files are written under `out/` and should not be committed.

```text
out/build/    Generated project files
out/bin/      Executables and libraries
out/obj/      Object files and intermediate output
```

## Memory backend

The default backend is the system allocator:

```text
--memory-backend=system
```

mimalloc can be selected when the dependency is available under `third_party/mimalloc`:

```text
--memory-backend=mimalloc
```
