# Build System IDE Integration

BlueCore exposes selected Premake build system operations as generated utility targets in IDE solutions.

## Utility Targets

The following targets are generated and grouped under a `Build System` folder:

- `BlueBuildSystemFiles` — Shows Lua build files, scripts, and build documentation.
- `BlueRegenerateSolution` — Regenerates the current project files.
- `BlueValidateBuildGraph` — Runs build graph validation.
- `BlueListTests` — Prints registered test executables.

## Regeneration Policy

Regeneration is explicit. Normal C++ builds do not automatically regenerate project files.

**Windows**

```cmd
scripts\regenerate-windows.cmd vs2026 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

**Linux**

```bash
./scripts/regenerate-linux.sh ninja --toolchain=clang --blue-platforms=linux --blue-startup=BlueRunTests
```

**macOS**

```bash
./scripts/regenerate-macos.sh ninja --toolchain=clang --blue-platforms=macos --blue-startup=BlueRunTests
```

## Build Graph Token

Regeneration metadata is stored under:

```text
out/build/.blue/premake/
```

This token records build scripts, project declarations, third-party declarations, generation options, and the current source/test file inventory.

Editing the contents of an existing `.cpp` file does not require regeneration. Adding, removing, or renaming files does trigger regeneration.