# Visual Studio 2026 Generation

BlueCore can generate Visual Studio 2026 `.slnx` workflow files when the installed Premake version supports the `vs2026` action.

## Generate

```cmd
scripts\generate-vs-windows.cmd
```

Equivalent explicit command:

```cmd
scripts\premake-windows.cmd vs2026 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

Open the solution at:

```text
out/build/vs2026/Blue.slnx
```

## Build Axes

- **Configuration**: `Debug` / `Release` / `Profile` / `Shipping`
- **Platform**: `x64` / `x64_DLL`

## Toolset Selection

When `--msvc-toolset` is omitted, the generator selects an installed x64 MSVC platform toolset that it can verify.

Pin a specific toolset only when required by the target environment:

```cmd
scripts\premake-windows.cmd vs2026 --toolchain=msvc --blue-platforms=windows --msvc-toolset=v143
```

Pin both the toolset and exact tool version only when that exact version is installed:

```cmd
scripts\premake-windows.cmd vs2026 \
    --toolchain=msvc \
    --blue-platforms=windows \
    --msvc-toolset=v145 \
    --msvc-tools-version=14.50
```

## VS2022 Compatibility

Visual Studio 2022 project generation remains available:

```cmd
scripts\premake-windows.cmd vs2022 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

## Project Command Layer

Generated `.vcxproj` files are disposable output produced from the Blue build definitions. They are not the source of truth for project membership.

Use the Blue project command layer for all automated changes to files and projects. The available commands are documented in `docs/BLUE_PROJECT_COMMANDS.md`.

After making changes through the command layer, regenerate the solution so that the generated project files are updated.