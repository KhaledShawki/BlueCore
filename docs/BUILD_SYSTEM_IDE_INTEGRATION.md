# Build-System IDE Integration

Blue exposes selected Premake build-system operations as generated utility targets.

## Utility targets

- `BlueBuildSystemFiles` shows Lua build files, scripts, and build documentation.
- `BlueRegenerateSolution` regenerates the current project files.
- `BlueValidateBuildGraph` runs build-graph validation.
- `BlueListTests` prints registered test executables.

These targets are grouped under `Build System` in generated IDE solutions.

## Regeneration policy

Regeneration is explicit. Normal C++ builds do not silently regenerate project files.

Windows:

```cmd
scripts\regenerate-windows.cmd vs2026 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
```

Linux:

```bash
./scripts/regenerate-linux.sh ninja --toolchain=clang --blue-platforms=linux --blue-startup=BlueRunTests
```

macOS:

```bash
./scripts/regenerate-macos.sh ninja --toolchain=clang --blue-platforms=macos --blue-startup=BlueRunTests
```

## Build graph token

Regeneration metadata is stored under:

```text
out/build/.blue/premake/
```

The token tracks build scripts, project declarations, third-party declarations, generation options, and source/test file inventory. Editing the contents of an existing `.cpp` file does not require project regeneration; adding, removing, or renaming files does.
