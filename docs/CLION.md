# CLion Integration

BlueCore supports CLion through generated compilation databases and local IDE integration files. The integration is driven by the Premake workspace and project graph. It does not rely on Bear, CMake, or CLion-specific metadata.

## Generating CLion Files

**Windows**

```cmd
python scripts\blue.py clion --toolchain=msvc --blue-platforms=windows
```

**Linux**

```bash
python scripts/blue.py clion --toolchain=clang --blue-platforms=linux
```

**macOS**

```bash
python scripts/blue.py clion --toolchain=clang --blue-platforms=macos
```

By default, the command generates the daily development view (`Debug` / `x64`). This includes the workspace build and the default executable run target.

Use the following flags when more comprehensive output is needed:

- `--clion-build-targets=all` — Expose every buildable Premake project
- `--clion-platform=all --clion-config=all` — Generate the full platform/configuration matrix

Generated compilation databases are written to:

```text
out/ide/clion/<target-os>/<platform>/<configuration>/compile_commands.json
```

A root `compile_commands.json` is also created so CLion can index the repository from the root directory.

## Build Integration

The `clion` action can also generate local CLion build files under `.idea/` (unless disabled with `--clion-idea=off`):

```text
.idea/customTargets.xml
.idea/tools/External Tools.xml
```

It creates one stable custom target named after the Premake workspace (for example `Blue`). This target contains build configurations derived from the workspace and project graph.

Use CLion’s normal build actions after opening the project:

- **Build → Build Project**
- **Build → Rebuild Project**
- **Build → Clean**

`Build Project` uses the first generated build configuration (typically the workspace build).

## Run Integration

Run configurations are generated only for Premake executable projects (`ConsoleApp` or `WindowedApp`).

Default run target selection follows this order:

1. Use the workspace `startproject` (if it is an executable)
2. If there is exactly one executable project in the workspace, use it
3. Otherwise, generate no default run configuration

To control run configuration generation:

```bash
python scripts/blue.py clion --clion-run-targets=all
python scripts/blue.py clion --clion-run-targets=BlueTests,BlueBenchmarks
python scripts/blue.py clion --clion-run-targets=none
```

## Build Target Selection

Build configurations are generated from buildable Premake projects (`StaticLib`, `SharedLib`, `ConsoleApp`, `WindowedApp`).

Supported modes:

```text
--clion-build-targets=default
--clion-build-targets=workspace
--clion-build-targets=all
--clion-build-targets=none
--clion-build-targets=ProjectA,ProjectB
```

The `default` mode generates the workspace build plus any configurations required by selected run targets. Use `all` when you want to build individual libraries and executables directly from CLion.

## Generating a Specific Database

```text
python scripts/blue.py clion     --blue-platforms=<platform>     --toolchain=<toolchain>     --clion-platform=x64_DLL     --clion-config=Release
```

Supported values:

```text
--clion-platform=default|all|x64|x64_DLL
--clion-config=default|all|Debug|Release|Profile|Shipping
```

## Internal Build Helpers

Generated CLion build targets invoke these internal repository helpers:

```text
scripts/clion-build-windows.cmd
scripts/clion-clean-windows.cmd
scripts/clion-build-unix.sh
scripts/clion-clean-unix.sh
```

They regenerate the selected native backend through `scripts/blue.py premake` before invoking the native build tool.

## Regeneration Policy

Regenerate CLion files after changing any of the following:

- Workspace name, platforms, configurations, or start project
- Project kind or target name
- Module dependencies
- Include directories or compiler definitions
- Source file lists or platform/configuration filters
- Premake project declarations
