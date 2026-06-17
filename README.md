# BlueCore

BlueCore is a modular C++ foundation framework focused on deterministic memory management, low-level primitives, and runtime observability.

The project follows a strict layered architecture where each layer depends only on the layers beneath it. This design results in clearer boundaries, better testability, and more maintainable code.

## Architecture

BlueCore is organized into four layers:

| Layer          | Module            | Responsibility |
|----------------|-------------------|----------------|
| **Foundation**     | `BlueSystem`      | Platform abstraction, threading, synchronization, logging, diagnostics, and time |
| **Memory**         | `BlueMemory`      | Pool-based allocators, small-block optimization, metrics, tracking, and OOM reporting |
| **Containers**     | `BlueContainer`   | Allocator-aware containers and lightweight view types |
| **Job System**     | `BlueJobSystem`   | Building blocks for concurrent task execution |

The dependency direction is strictly downward. Lower layers have no knowledge of higher layers.

## Design Characteristics

- Strong emphasis on predictable memory behavior and observability
- Threading and synchronization primitives with explicit memory ordering
- Support for 128-bit atomic operations
- Cross-platform implementation (Windows, Linux, macOS)
- Modern C++20 codebase

## Repository Structure

```
BlueCore/
├── modules/     Core libraries
├── apps/        Applications and benchmarks
├── tests/       Test infrastructure
├── tools/       Build tooling
├── build/       Premake configuration
├── scripts/     Automation scripts
├── docs/        Documentation
└── out/         Build output (ignored)
```

## Getting Started

See the documentation in the [`docs/`](docs/) directory, particularly:

- [Architecture](docs/ARCHITECTURE.md)
- [Building](docs/BUILDING.md)
- [Usage](docs/USAGE.md)
- [Memory System](docs/MEMORY_SYSTEM.md)

## Build Requirements

- C++20 compatible compiler
- [Premake 5](https://premake.github.io/)
