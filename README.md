# BlueCore

![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![License](https://img.shields.io/badge/license-Unlicensed-lightgrey.svg)

BlueCore is a modular C++ foundation framework focused on deterministic memory management, low-level primitives, and runtime observability.

## CI Status

| Platform | Debug | Release |
|----------|-------|---------|
| **Linux** | [![Linux Debug](https://img.shields.io/github/check-runs/KhaledShawki/BlueCore/main?nameFilter=Linux%20Debug&label=Linux%20Debug&logo=github)](https://github.com/KhaledShawki/BlueCore/actions/workflows/ci.yml) | [![Linux Release](https://img.shields.io/github/check-runs/KhaledShawki/BlueCore/main?nameFilter=Linux%20Release&label=Linux%20Release&logo=github)](https://github.com/KhaledShawki/BlueCore/actions/workflows/ci.yml) |
| **macOS** | [![macOS Debug](https://img.shields.io/github/check-runs/KhaledShawki/BlueCore/main?nameFilter=macOS%20Debug&label=macOS%20Debug&logo=github)](https://github.com/KhaledShawki/BlueCore/actions/workflows/ci.yml) | [![macOS Release](https://img.shields.io/github/check-runs/KhaledShawki/BlueCore/main?nameFilter=macOS%20Release&label=macOS%20Release&logo=github)](https://github.com/KhaledShawki/BlueCore/actions/workflows/ci.yml) |
| **Windows** | [![Windows Debug](https://img.shields.io/github/check-runs/KhaledShawki/BlueCore/main?nameFilter=Windows%20Debug&label=Windows%20Debug&logo=github)](https://github.com/KhaledShawki/BlueCore/actions/workflows/ci.yml) | [![Windows Release](https://img.shields.io/github/check-runs/KhaledShawki/BlueCore/main?nameFilter=Windows%20Release&label=Windows%20Release&logo=github)](https://github.com/KhaledShawki/BlueCore/actions/workflows/ci.yml) |

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
