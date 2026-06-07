# CLion

Blue supports CLion through generated compilation databases and generated local IDE integration files. The exporter is driven by the Premake workspace and project graph. It does not depend on Bear, CMake, or CLion-specific project metadata.

## Generate CLion files

Windows:

```cmd
scripts\premake-windows.cmd clion --toolchain=msvc --blue-platforms=windows
```

Linux:

```bash
./scripts/premake-linux.sh clion --toolchain=clang --blue-platforms=linux
```

macOS:

```bash
./scripts/premake-macos.sh clion --toolchain=clang --blue-platforms=macos
```

The default command writes the daily development view only: `Debug` / `x64`. It exposes the workspace build plus the default executable run target. Use `--clion-build-targets=all` to expose every buildable Premake project in CLion. Use `--clion-platform=all --clion-config=all` only when the full platform/configuration matrix is needed.

Generated databases are written under:

```text
out/ide/clion/<target-os>/<platform>/<configuration>/compile_commands.json
```

The root `compile_commands.json` is copied from the generated default database so CLion can index the repository root directly.

## Build integration

The `clion` action also generates local CLion build files under `.idea/` unless disabled:

```text
.idea/customTargets.xml
.idea/tools/External Tools.xml
```

The exporter creates one stable CLion custom target named after the Premake workspace, for example:

```text
Blue
```

That target contains build configurations generated from the workspace and project graph:

```text
Blue Workspace Debug x64
BlueTests Debug x64
BlueSystem Debug x64
BlueMemory Debug x64
```

Default generation includes the workspace configuration and any build configuration needed by the selected run targets. Generate build configurations for every buildable Premake project with:

```bash
./scripts/premake-linux.sh clion --clion-build-targets=all
```

Use CLion's normal build actions after opening the repository:

```text
Build -> Build Project
Build -> Rebuild Project
Build -> Clean
```

`Build Project` uses the first generated build configuration, which is the workspace build. Run/debug configurations reference the same stable custom target and the executable project's matching build configuration.

Disable `.idea` generation when only the compilation database is needed:

```bash
./scripts/premake-linux.sh clion --clion-idea=off
```

## Run integration

Run configurations are generated only for Premake executable projects:

```lua
kind "ConsoleApp"
kind "WindowedApp"
```

Default run target selection is deterministic:

1. Use the workspace `startproject` when it is an executable project.
2. If there is no executable `startproject` and the workspace contains exactly one executable project, use that project.
3. Otherwise generate no default run configuration until the target is selected explicitly.

Generate all runnable executables:

```bash
./scripts/premake-linux.sh clion --clion-run-targets=all
```

Generate specific executable projects:

```bash
./scripts/premake-linux.sh clion --clion-run-targets=BlueTests,BlueBenchmarks
```

Disable run configurations:

```bash
./scripts/premake-linux.sh clion --clion-run-targets=none
```

## Build target selection

Build configurations are generated from buildable Premake projects:

```lua
kind "StaticLib"
kind "SharedLib"
kind "ConsoleApp"
kind "WindowedApp"
```

Supported build target modes:

```text
--clion-build-targets=default
--clion-build-targets=workspace
--clion-build-targets=all
--clion-build-targets=none
--clion-build-targets=ProjectA,ProjectB
```

`default` generates the workspace build and the build configurations required by the selected run targets. `all` is useful when you want to build individual libraries and executables directly from CLion.

The exporter does not read or require `ide` or `clion` fields in project declarations. Use standard Premake data: workspace `startproject`, project `kind`, source files, dependencies, include directories, defines, filters, and debug arguments.

## Generate a specific database

Use `--clion-platform` and `--clion-config` when a build view other than the default `Debug` / `x64` view is needed:

```cmd
scripts\premake-windows.cmd clion --blue-platforms=windows --toolchain=msvc --clion-platform=x64_DLL --clion-config=Release
```

Supported values:

```text
--clion-platform=default|all|x64|x64_DLL
--clion-config=default|all|Debug|Release|Profile|Shipping
```

`default` means `x64` for the platform and `Debug` for the configuration.

## Build wrapper behavior

Generated CLion build targets call repository scripts instead of duplicating build logic in XML:

```text
scripts/clion-build-windows.cmd
scripts/clion-clean-windows.cmd
scripts/clion-build-unix.sh
scripts/clion-clean-unix.sh
```

The wrappers regenerate the native backend before building:

- Windows uses the configured Visual Studio action and MSBuild.
- Linux and macOS use the generated GNU Make backend.

The workspace build configuration calls the generated aggregate target, for example `BlueWorkspace`. Project build configurations call their normal Premake project targets.

## Regeneration policy

Regenerate CLion files after changing:

- workspace name, platforms, configurations, or start project
- project kind or target name
- module dependencies
- include directories
- compiler definitions
- source file lists
- platform/configuration filters
- Premake project declarations
