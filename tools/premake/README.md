# Premake

Place Premake 5 binaries in the OS-specific folders:

```text
windows/premake5.exe
linux/premake5
macos/premake5
```

These binaries are not committed to the repository by default.

Use the wrapper scripts from the repository root for normal workflows:

```cmd
scripts\premake-windows.cmd validate
```

```bash
./scripts/premake-linux.sh validate
./scripts/premake-macos.sh validate
```
