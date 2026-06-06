# Linkage Model

Blue uses two build axes:

```text
Configurations: Debug / Release / Profile / Shipping
Platforms:      x64 / x64_DLL
```

Configurations describe optimization and debug policy. Platforms describe the linkage variant.

## Linkage platforms

```text
x64      Static libraries
x64_DLL  Shared libraries / DLLs
```

The target operating system is selected separately with `--blue-platforms`.

## Generated names

Visual Studio exposes:

```text
Debug   | x64
Debug   | x64_DLL
Release | x64
Release | x64_DLL
Profile | x64
Profile | x64_DLL
Shipping| x64
Shipping| x64_DLL
```

GNU Make and Ninja expose combined lowercase names such as:

```text
debug_x64
debug_x64_dll
release_x64
release_x64_dll
```

## Output layout

Binaries are emitted to a common per-linkage directory so executables can locate sibling shared libraries at runtime:

```text
out/bin/<system>/<platform>/<configuration>/
```

Object files remain project-isolated:

```text
out/obj/<system>/<platform>/<configuration>/<project>/
```

## Public preprocessor contract

Premake defines `BLUE_SHARED_LIBRARY=1` for the `x64_DLL` platform.

While compiling a module as a shared library, Premake also defines a private module build macro:

```text
BLUE_BUILD_BLUE_SYSTEM=1
BLUE_BUILD_BLUE_MEMORY=1
BLUE_BUILD_BLUE_CONTAINER=1
BLUE_BUILD_BLUE_JOB_SYSTEM=1
```

Public API headers translate these definitions into import/export attributes:

```cpp
#include <Blue/System/Api.h>
#include <Blue/Memory/Api.h>
#include <Blue/Container/Api.h>
#include <Blue/JobSystem/Api.h>
```

## Precompiled headers

A project uses a precompiled header when one of these pairs exists:

```text
<project-root>/src/Pch.h
<project-root>/src/Pch.cpp
```

or:

```text
<project-root>/Pch.h
<project-root>/Pch.cpp
```

A project can opt out with:

```lua
pch = false
```
