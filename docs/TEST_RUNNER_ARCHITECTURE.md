# Test Runner Architecture

BlueCore uses a target-based test model instead of a single monolithic test binary.

## Model

- Tests are located inside the owning module’s `tests/` folder.
- Each test source is built as a separate executable.
- Test registration is managed by Premake.
- IDEs and CI use the generated `BlueRunTests` aggregate target.
- Scripts do not maintain separate test lists.

## Rationale

Low-level systems tests can crash, deadlock, trigger assertions, or stress threading primitives. Running each test in its own process makes failures significantly easier to diagnose and contain.

`BlueRunTests` provides a single, convenient entry point for both local development and CI while preserving per-test process isolation.

## Runner Implementation

The test runner is a developer tool and is therefore allowed to use the C++ standard library (unlike the core runtime modules).

Instead of using `std::system`, the runner uses native process APIs for better control and error reporting:

- **Windows**: `CreateProcessA`
- **Linux / macOS**: `fork`, `execl`, `waitpid`
