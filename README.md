# Blue

Blue is a modular C++ foundation framework for native applications, tools, and engine-style systems.

The repository is organized as a workspace. Core runtime code lives in `modules/`, runnable programs live in `apps/`, shared test infrastructure lives in `tests/`, and developer tooling lives in `tools/`.

## Modules

| Module | Responsibility |
|---|---|
| `BlueSystem` | Platform detection, compiler utilities, base types, assertions, time, processor queries, threading, synchronization, diagnostics, and logging. |
| `BlueMemory` | Allocation interfaces, memory backends, pool registry, allocator metrics, allocation tags, blocks, and tracking hooks. |
| `BlueContainer` | Allocator-aware containers and lightweight view types. |
| `BlueJobSystem` | Job-system foundation built on the system, memory, and container layers. |

## Repository layout

```text
Blue/
  modules/        Core reusable libraries
  apps/           Executable applications and benchmarks
  tests/          Shared test runner and integration-level test infrastructure
  tools/          Developer tools used by the build and test workflow
  build/          Premake build framework and dependency declarations
  scripts/        Entry-point scripts for generation, formatting, and testing
  docs/           Repository-level documentation
  third_party/    Optional external dependency payloads
  out/            Generated build output; not committed
```

## Build requirements

- C++20 compiler
- Premake 5
- Optional: `clang-format`
- Optional: Graphviz for build graph rendering

Place the Premake executable in the matching folder:

```text
tools/premake/windows/premake5.exe
tools/premake/linux/premake5
tools/premake/macos/premake5
```

## Quick start

Windows:

```cmd
scripts\premake-windows.cmd validate
scripts\premake-windows.cmd vs2022 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests
msbuild out\build\vs2022\Blue.sln /p:Configuration=Debug /p:Platform=Win64
scripts\run-tests-windows.cmd
```

Linux:

```bash
chmod +x tools/premake/linux/premake5 scripts/*.sh
./scripts/premake-linux.sh validate
./scripts/premake-linux.sh gmake2 --toolchain=gcc --blue-platforms=linux --blue-startup=BlueRunTests
make -C out/build/gmake2 config=debug_x64
./scripts/run-tests-linux.sh
```

macOS:

```bash
chmod +x tools/premake/macos/premake5 scripts/*.sh
./scripts/premake-macos.sh validate
./scripts/premake-macos.sh xcode4 --toolchain=clang --blue-platforms=macos --blue-startup=BlueRunTests
./scripts/run-tests-macos.sh
```

## Formatting

Format supported C/C++ files:

```cmd
scripts\premake-windows.cmd format
```

Check formatting without modifying files:

```cmd
scripts\premake-windows.cmd check-format
```

Linux/macOS use the corresponding `premake-linux.sh` or `premake-macos.sh` wrapper.

## Documentation

Start with these documents:

| Document | Purpose |
|---|---|
| `docs/ARCHITECTURE.md` | Module layering and dependency rules. |
| `docs/BUILDING.md` | Build setup and platform-specific commands. |
| `docs/FORMATTING.md` | Formatting policy and formatter scripts. |
| `docs/TESTING.md` | Test layout, registration, and execution. |
| `docs/LINKAGE.md` | Static/shared linkage model and output layout. |
| `docs/MEMORY_SYSTEM.md` | BlueMemory overview. |
| `docs/ALLOCATION_CONTRACT.md` | Allocation pool/proxy/invoker contract. |
| `modules/BlueSystem/README.md` | BlueSystem module overview. |

## Third-party dependencies

External payloads are not committed by default. Optional dependencies are placed under `third_party/` and wired through `build/third_party/` declarations.

The default memory backend is the system allocator. To use mimalloc, place the required headers and libraries under `third_party/mimalloc` and generate with:

```cmd
scripts\premake-windows.cmd vs2022 --memory-backend=mimalloc
```
