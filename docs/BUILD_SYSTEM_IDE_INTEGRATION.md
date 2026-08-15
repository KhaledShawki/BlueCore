# Build System IDE Integration

BlueCore exposes selected Premake build-system operations as generated utility targets in IDE solutions.

## Utility Targets

The following targets are generated and grouped under a `Build System` folder:

- `BlueBuildSystemFiles` — Shows Lua build files, Python tooling, and build documentation.
- `BlueRegenerateSolution` — Regenerates the current project files when the build token is stale.
- `BlueValidateBuildGraph` — Runs build graph validation.
- `BlueListTests` — Prints registered test executables.

## Regeneration Policy

Regeneration is explicit. Normal C++ builds do not automatically regenerate project files.

**Windows**

```cmd
python scripts\blue.py regenerate vs2026 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

**Linux**

```bash
python scripts/blue.py regenerate ninja --toolchain=clang --blue-platforms=linux --blue-startup=BlueRunTests
```

**macOS**

```bash
python scripts/blue.py regenerate ninja --toolchain=clang --blue-platforms=macos --blue-startup=BlueRunTests
```

Generated IDE utility targets invoke the same semantic `blue regenerate` command instead of platform-specific wrapper scripts.

## Build Graph Token

Regeneration metadata is stored under:

```text
out/build/.blue/premake/
```

This token records build scripts, project declarations, third-party declarations, generation options, the Blue CLI implementation, and the current source/test file inventory.

Editing the contents of an existing `.cpp` file does not require regeneration. Adding, removing, or renaming files does trigger regeneration.