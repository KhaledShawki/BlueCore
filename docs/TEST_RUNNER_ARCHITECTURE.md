# Test Runner Architecture

Blue uses a target-based test model rather than one monolithic test binary.

## Model

- Tests live inside the owning module's `tests/` folder.
- Each test source builds as a separate executable.
- Premake owns test registration.
- IDEs and CI use the generated `BlueRunTests` target.
- Scripts do not maintain separate test lists.

## Rationale

Low-level systems tests can crash, deadlock, assert, or stress threading primitives. Isolating tests by executable keeps failures easier to diagnose.

`BlueRunTests` provides one entry point for local development and CI while preserving per-test process isolation.

## Runner implementation

The runner is a developer tool. It may use the C++ standard library because it is not part of the runtime modules.

The runner uses native process APIs instead of `std::system`:

- Windows: `CreateProcessA`
- Linux/macOS: `fork`, `execl`, `waitpid`
