# Premake for Windows

Place the Windows Premake 5 executable in this folder:

```text
premake5.exe
```

Use the repository Blue CLI for normal generation and validation:

```cmd
python scripts\blue.py validate
python scripts\blue.py premake vs2026 --toolchain=msvc --blue-platforms=windows
```
