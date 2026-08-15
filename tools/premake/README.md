# Premake

Place Premake 5 binaries in the OS-specific folders:

```text
windows/premake5.exe
linux/premake5
macos/premake5
```

These binaries are not committed to the repository by default.

Normal developer workflows use the cross-platform Blue CLI, which resolves the correct bundled Premake binary for the current host:

```text
python scripts/blue.py validate
python scripts/blue.py premake <action> [options]
```

Direct invocation of the bundled binaries is reserved for low-level debugging of Premake itself.
