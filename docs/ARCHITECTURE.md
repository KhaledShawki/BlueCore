# Architecture

BlueCore follows a strict layered architecture. Each layer depends only on the layers below it. This is a fundamental design decision that shapes the entire project.

```text
BlueSystem
    ↓
BlueMemory
    ↓
BlueContainer
    ↓
BlueJobSystem
```

## Layering Principles

The strict layering approach provides several important benefits:

- **Clear boundaries** — Code in a given layer can only use types and functions from lower layers.
- **Improved testability** — Each layer can be tested independently.
- **Easier maintenance** — Changes in higher layers cannot accidentally break lower layers.
- **Better reasoning** — When working in a layer, the available dependencies are limited and predictable.

## Dependency Rules

| Module            | Allowed Dependencies                     | Forbidden Dependencies          |
|-------------------|------------------------------------------|---------------------------------|
| `BlueSystem`      | None (within Blue runtime modules)       | `BlueMemory`, `BlueContainer`, `BlueJobSystem` |
| `BlueMemory`      | `BlueSystem`                             | `BlueContainer`, `BlueJobSystem` |
| `BlueContainer`   | `BlueSystem`, `BlueMemory`               | `BlueJobSystem`                 |
| `BlueJobSystem`   | `BlueSystem`, `BlueMemory`, `BlueContainer` | —                            |

These rules are treated as part of the architecture contract.

## Repository Layout

| Directory   | Purpose |
|-------------|---------|
| `modules/`  | Core runtime libraries |
| `apps/`     | Applications and benchmarks |
| `tests/`    | Shared test infrastructure |
| `tools/`    | Build and development tools |
| `build/`    | Premake build system |
| `scripts/`  | Automation and helper scripts |
| `docs/`     | Documentation |

Documentation specific to a module is placed inside that module’s directory.

## Memory Allocation Rules

All allocations performed by BlueCore code should go through the `BlueMemory` layer. Direct use of `malloc`, `free`, `new`, and `delete` is restricted to:

- Memory backend implementations
- Platform adapter code
- Test infrastructure
- Controlled override mechanisms

This restriction exists to keep memory tracking, metrics, and diagnostics centralized.

## Logging

The logging system resides in `BlueSystem`. This placement is intentional, as both the memory allocator and platform layers require diagnostic capabilities. The logger is designed to operate without heap allocations on its hot path.

## Public API Constraints

Public headers must not include platform-specific headers such as `windows.h` or `pthread.h`. Operating system handles are wrapped in Blue types, and platform-specific implementation details remain in source files. This keeps the public interface portable and clean.
