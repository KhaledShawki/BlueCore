# Blue CLI

`scripts/blue.py` is the single cross-platform developer entry point for BlueCore build tooling.

The executable is intentionally small. Implementation lives in `scripts/blue_cli/`, while Premake and the Blue Lua framework remain authoritative for the project graph, dependencies, build options, test registration, generation policy, and structural project mutations.

## Requirements

- Python 3.11 or newer
- The bundled Premake executable for the current host under `tools/premake/<os>/`
- Native backend/compiler tools required by the selected operation
- Formatting tools when running formatting commands: `clang-format`, StyLua, and Black

Use the same entry point on every supported host:

```text
python scripts/blue.py <command> [arguments]
```

## Semantic commands

```text
python scripts/blue.py build [target]
python scripts/blue.py clean
python scripts/blue.py test
python scripts/blue.py regenerate [premake-action]

python scripts/blue.py format
python scripts/blue.py format-check
python scripts/blue.py list-format-files

python scripts/blue.py validate
python scripts/blue.py clion
```

Other graph/metadata actions remain available through the same CLI:

```text
python scripts/blue.py graph
python scripts/blue.py metadata
python scripts/blue.py list-tests
python scripts/blue.py test-metadata
python scripts/blue.py list-benchmarks
python scripts/blue.py benchmark-metadata
```

Project mutation commands forward their existing `--blue-*` arguments to Premake:

```text
python scripts/blue.py add-file --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp
python scripts/blue.py remove-file --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp
python scripts/blue.py rename-file --blue-project=BlueSystem --blue-kind=source --blue-from=Old.cpp --blue-to=New.cpp
python scripts/blue.py add-project --blue-project=BlueGraphics --blue-type=library --blue-linkage=auto
```

## Build and clean

`build` and `clean` provide the native-build operations used by editor integrations without platform wrapper scripts. `build` may select a target; `clean` is intentionally configuration-scoped on every backend so its semantics are identical across Ninja, GNU Make, and Visual Studio.

```text
python scripts/blue.py build BlueTests --config=Debug --platform=x64
python scripts/blue.py clean --config=Debug --platform=x64
```

Supported build options:

```text
--config=Debug|Release|Profile|Shipping
--platform=x64|x64_DLL
--backend=ninja|gmake|vs2026
--toolchain=clang|gcc|msvc
--memory-backend=system|mimalloc
```

Unix build/clean defaults to GNU Make so workspace and CLion utility targets remain available. Ninja is supported for real static `x64` targets. Windows uses Visual Studio 2026/MSBuild. `clean` always cleans the selected configuration rather than pretending that every native backend supports target-scoped clean semantics. Premake still generates and owns the native graph; the CLI only selects and invokes it.

For generic Unix `build`/`clean` commands, `BLUE_TOOLCHAIN` may be used to override the default Clang toolchain when `--toolchain` is omitted.

## Regeneration

Regeneration uses the existing Blue build-graph token rather than regenerating unconditionally:

```text
python scripts/blue.py regenerate ninja --toolchain=clang --blue-platforms=linux
python scripts/blue.py regenerate vs2026 --toolchain=msvc --blue-platforms=windows
```

The command runs `check-regeneration`; return code `0` skips generation, return code `2` regenerates and updates the token, and any other return code is treated as a failure.

## Running tests

```text
python scripts/blue.py test
python scripts/blue.py test --config=Release
python scripts/blue.py test --config=Debug --memory-backend=mimalloc
```

Supported options:

```text
--config=Debug|Release|Profile|Shipping
--backend=ninja|gmake|vs2026
--toolchain=clang|gcc|msvc
--linkage=static|shared
--memory-backend=system|mimalloc
```

Backend, toolchain, and linkage remain separate build axes. Windows defaults to Visual Studio 2026 + MSVC; macOS defaults to Ninja + Clang for static tests and gmake + Clang for shared tests; Linux defaults to Ninja + Clang for static tests and gmake for shared tests. `gmake2` remains accepted as a compatibility alias for `gmake`.

Premake writes the authoritative registered-test manifest under `out/`. `blue test` builds and executes exactly those registered binaries once through `BlueRunTests`.

## Formatting

Formatting orchestration is implemented directly in `scripts/blue_cli/formatting.py`; there are no Bash, CMD, or PowerShell formatter wrappers.

```text
python scripts/blue.py format
python scripts/blue.py format-check
python scripts/blue.py list-format-files
```

Optional formatter overrides:

```text
--format-path=<clang-format>
--lua-format-path=<stylua>
--python-format-path=<black>
```

The corresponding `BLUE_CLANG_FORMAT`, `BLUE_STYLUA`, and `BLUE_BLACK` environment variables are also supported.

## Raw Premake escape hatch

For build-system operations that do not yet have a semantic command:

```text
python scripts/blue.py premake <premake-action> [premake-options]
```

The escape hatch invokes the bundled host Premake binary with `premake5.lua`. It is not a second build graph.

## Structure and ownership

```text
scripts/blue.py
      |
      v
scripts/blue_cli/
  core.py        host/tool/process primitives
  build.py       build, clean, regeneration orchestration
  testing.py     test build and execution orchestration
  toolchains.py  host compiler/toolchain environment setup
  formatting.py  formatter discovery and execution
  cli.py         command routing and user-facing CLI
      |
      v
Premake + Blue Lua framework
      |
      +--> Ninja
      +--> GNU Make
      +--> MSBuild
```

Python owns CLI UX, host/tool discovery, and process orchestration. Premake/Lua owns project and build semantics. Native tools perform compilation, linking, formatting, and execution.
