# Testing

BlueCore uses a target-based test model. Each test is built as a separate executable rather than being bundled into a single test binary.

## Test Layout

Tests for a module are placed inside that module’s `tests/` directory:

```text
modules/BlueSystem/
├── include/
├── src/
└── tests/
    ├── BlueSystemAtomicTests.cpp
    └── BlueSystemThreadingTests.cpp
```

The build system enforces that test sources must reside within the owning module’s `tests/` folder.

## Test Registration

Tests are registered in the module’s `project.lua` file:

```lua
bb.module_tests {
    module = "BlueSystem",
    root = "modules/BlueSystem",
    deps = {
        "BlueSystem",
    },
    tests = {
        "BlueSystemAtomicTests",
        "BlueSystemThreadingTests",
    },
}
```

Each registered test is built as an independent executable. This isolation makes it easier to diagnose crashes, deadlocks, or threading issues.

## Running Tests

The aggregate test runner `BlueRunTests` builds and executes all registered test executables.

**Windows**

```cmd
scripts\run-tests-windows.cmd
```

**Linux**

```bash
./scripts/run-tests-linux.sh
```

**macOS**

```bash
./scripts/run-tests-macos.sh
```

## Premake Actions

The following Premake actions are available for working with tests:

```cmd
scripts\premake-windows.cmd list-tests
scripts\premake-windows.cmd test-metadata
scripts\premake-windows.cmd run-tests
```

The `test-metadata` action generates a JSON file containing test information:

```text
generated/tests/BlueTests.json
```

## Design Policy

- Every test builds as a separate executable to improve failure isolation.
- Test registration is managed through Premake declarations rather than scripts.
- `BlueRunTests` serves as the single entry point for running all tests in IDEs and CI.
- The test runner may use the C++ standard library, as it is not part of the core runtime.
- Runtime modules must not depend on the test runner implementation.