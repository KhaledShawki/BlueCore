# Linkage Model

BlueCore uses two independent build axes:

- **Configurations**: `Debug` / `Release` / `Profile` / `Shipping`
- **Platforms**: `x64` / `x64_DLL`

Configurations control optimization level and debug information. Platforms control linkage type (static vs shared).

## Linkage Platforms

| Platform   | Description                  |
|------------|------------------------------|
| `x64`      | Static libraries             |
| `x64_DLL`  | Shared libraries / DLLs      |

The target operating system is selected separately using `--blue-platforms`.

## Generated Build Names

**Visual Studio**

```
Debug   | x64
Debug   | x64_DLL
Release | x64
Release | x64_DLL
Profile | x64
Profile | x64_DLL
Shipping| x64
Shipping| x64_DLL
```

**GNU Make / Ninja**

```
debug_x64
debug_x64_dll
release_x64
release_x64_dll
profile_x64
profile_x64_dll
shipping_x64
shipping_x64_dll
```

## Output Layout

Binaries are placed in a shared per-linkage directory so that executables can locate sibling shared libraries at runtime:

```text
out/bin/<system>/<platform>/<configuration>/
```

Object files remain isolated per project:

```text
out/obj/<system>/<platform>/<configuration>/<project>/
```

## Public Preprocessor Definitions

When building for the `x64_DLL` platform, Premake defines:

```cpp
BLUE_SHARED_LIBRARY=1
```

Additionally, each module receives a private build macro:

```cpp
BLUE_BUILD_BLUE_SYSTEM=1
BLUE_BUILD_BLUE_MEMORY=1
BLUE_BUILD_BLUE_CONTAINER=1
BLUE_BUILD_BLUE_JOB_SYSTEM=1
```

Public API headers use these definitions to apply the correct `dllexport` / `dllimport` attributes.

## Precompiled Headers

A project uses a precompiled header if one of the following pairs exists:

- `<project-root>/src/Pch.h` + `<project-root>/src/Pch.cpp`
- `<project-root>/Pch.h` + `<project-root>/Pch.cpp`

A project can disable precompiled headers with:

```lua
pch = false
```
