# Architecture

Blue is split into small modules with strict dependency direction. Lower layers must not depend on higher layers.

```text
BlueSystem
    ↓
BlueMemory
    ↓
BlueContainer
    ↓
BlueJobSystem
```

## Dependency rules

- `BlueSystem` has no dependency on other Blue runtime modules.
- `BlueMemory` may depend on `BlueSystem` only.
- `BlueContainer` may depend on `BlueSystem` and `BlueMemory`.
- `BlueJobSystem` may depend on `BlueSystem`, `BlueMemory`, and `BlueContainer`.

These rules keep platform code, allocation code, containers, and job execution independent enough to test and replace in isolation.

## Repository ownership

```text
modules/    Reusable runtime libraries
apps/       Executables, benchmarks, demos, and tools that are shipped or run directly
tests/      Shared test runner and integration-level test support
tools/      Developer tooling used by the repository workflow
build/      Premake framework and dependency declarations
scripts/    Thin command wrappers for common workflows
docs/       Repository-level documentation
```

Module-specific documentation belongs under the owning module, for example `modules/BlueSystem/docs/`.

## Allocation ownership

Runtime allocations should go through `Blue::Allocator` or a higher-level BlueMemory API. Direct `malloc`, `free`, `new`, and `delete` are reserved for backend implementations, platform adapters, test harnesses, or controlled override files.

## Logging and allocation recursion

Logging belongs to `BlueSystem` because allocator and platform code need diagnostics. The logger must therefore be able to emit messages without heap allocation on its hot path.

## Public header policy

Public headers should avoid leaking platform headers such as `windows.h` and `pthread.h`. Native handles are wrapped in Blue types, and platform-specific implementation details stay in source files.
