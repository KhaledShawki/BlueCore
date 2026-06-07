# Visual Studio 2026 Generation

Blue can generate a Visual Studio 2026 `.slnx` workflow when the installed Premake version supports the `vs2026` action.

## Generate

```cmd
scripts\generate-vs-windows.cmd
```

Equivalent explicit command:

```cmd
scripts\premake-windows.cmd vs2026 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

Open:

```text
out/build/vs2026/Blue.slnx
```

## Build axes

```text
Configuration: Debug / Release / Profile / Shipping
Platform:      x64 / x64_DLL
```

## Toolset selection

When `--msvc-toolset` is omitted, the generator selects an installed x64 MSVC platform toolset it can verify. Pin a toolset only when the target environment requires it and the toolset is installed:

```cmd
scripts\premake-windows.cmd vs2026 --toolchain=msvc --blue-platforms=windows --msvc-toolset=v143
```

Pin an exact tool version only when that exact version is installed:

```cmd
scripts\premake-windows.cmd vs2026 --toolchain=msvc --blue-platforms=windows --msvc-toolset=v145 --msvc-tools-version=14.50
```

## VS2022 compatibility

Visual Studio 2022 project generation remains available:

```cmd
scripts\premake-windows.cmd vs2022 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

## Blue project command layer

Generated Visual Studio .vcxproj files are not the source of truth for project membership. They are disposable output produced from the Blue build definitions.

Use the Blue project command layer for all automated project changes, including adding, removing, and renaming files or projects. The supported commands are documented in docs/BLUE_PROJECT_COMMANDS.md.

A Visual Studio extension may provide the user interface for these actions, but it must call scripts\blue.cmd to perform the actual changes and then regenerate the solution. The extension must not modify generated .vcxproj files directly.
