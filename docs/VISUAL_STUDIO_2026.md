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
