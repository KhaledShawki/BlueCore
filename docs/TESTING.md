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

Use the cross-platform semantic test command:

```text
python scripts/blue.py test
python scripts/blue.py test --config=Release
python scripts/blue.py test --config=Debug --linkage=shared
```

`blue test` asks Premake to emit the authoritative registered-test manifest, builds the selected backend, and executes exactly those registered binaries through `BlueRunTests`.

## Test Metadata

The following commands expose the registered test model:

```text
python scripts/blue.py list-tests
python scripts/blue.py test-metadata
```

The `test-metadata` action generates the existing test metadata output for tooling. The semantic `test` command separately requests an internal manifest under `out/metadata/` for deterministic execution.

## Design Policy

- Every test builds as a separate executable to improve failure isolation.
- Test registration is managed through Premake declarations rather than scripts.
- `BlueRunTests` serves as the single test-runner implementation used by the CLI and generated IDE workflows.
- The test runner may use the C++ standard library, as it is not part of the core runtime.
- Runtime modules must not depend on the test runner implementation.