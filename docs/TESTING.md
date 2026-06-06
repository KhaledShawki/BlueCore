# Testing

Blue tests are registered through the Premake framework. Module tests live inside the owning module's `tests/` folder.

## Layout

```text
modules/BlueSystem/
  include/
  src/
  tests/
    BlueSystemAtomicTests.cpp
    BlueSystemThreadingTests.cpp
```

The framework rejects test sources outside the owning project's `tests/` folder.

## Registration

Module tests are registered in `project.lua`:

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

Each listed test builds as a separate executable.

## Aggregate runner

`BlueRunTests` builds and runs all registered test executables.

Windows:

```cmd
scripts\run-tests-windows.cmd
```

Linux:

```bash
./scripts/run-tests-linux.sh
```

macOS:

```bash
./scripts/run-tests-macos.sh
```

## Premake actions

```cmd
scripts\premake-windows.cmd list-tests
scripts\premake-windows.cmd test-metadata
scripts\premake-windows.cmd run-tests
```

`test-metadata` writes:

```text
generated/tests/BlueTests.json
```

## Policy

- Each test source should build as an isolated executable.
- Test registration belongs in Premake project declarations, not duplicated scripts.
- `BlueRunTests` is the aggregate target for IDEs and CI.
- Test runner tooling may use the C++ standard library.
- Runtime modules must not depend on the test runner implementation.
