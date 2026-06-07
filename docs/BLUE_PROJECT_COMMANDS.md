# Blue project command layer

Blue project files are source-controlled declarations. Visual Studio projects remain generated output under `out/build`.

The Blue command layer provides a stable command contract for IDE integrations, including a future Visual Studio extension. The extension should call these commands instead of editing `project.lua`, `build.lua`, or generated `.vcxproj` files directly.

## Command model

The command layer is implemented in Lua and runs through Premake actions. The wrapper scripts provide a stable entry point:

```bat
scripts\blue.cmd add-file --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp
scripts\blue.cmd remove-file --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp
scripts\blue.cmd rename-file --blue-project=BlueSystem --blue-kind=source --blue-from=Old.cpp --blue-to=New.cpp
scripts\blue.cmd add-project --blue-project=BlueGraphics --blue-type=library --blue-linkage=auto
```

On Unix-like hosts, use:

```sh
scripts/blue.sh add-file --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp
```

## Supported file kinds

| Kind | Manifest section | Physical path rule |
|---|---|---|
| `source` | `files.sources` | `src/<path>` |
| `public-header` | `files.public_headers` | `include/<ProjectName>/<path>` unless the path already starts with `include/` |
| `private-header` | `files.private_headers` | `src/<path>` |
| `windows-source` | `files.platform.windows.sources` | `src/Platform/Windows/<path>` |
| `linux-source` | `files.platform.linux.sources` | `src/Platform/Linux/<path>` |
| `macos-source` | `files.platform.macosx.sources` | `src/Platform/MacOS/<path>` |
| `posix-source` | `files.platform.linux.sources` | `src/Platform/POSIX/<path>` |

## Safety rules

- Commands edit only declared Blue manifests.
- Commands reject absolute paths, wildcard paths, and paths escaping the project root.
- `add-file` creates the file from a template unless `--blue-no-create` is passed.
- `remove-file` removes only the manifest entry by default. Use `--blue-delete-file` to delete the physical file.
- `rename-file` updates the manifest and renames the physical file unless `--blue-no-create` is passed.
- `add-project` updates `build.lua` and creates the project template unless `--blue-no-create` is passed.
- Existing files are never overwritten.
- Generated Visual Studio projects are never edited by the command layer.

## Visual Studio extension contract

A Visual Studio extension should act as a thin UI layer over these commands:

- Add Source File → `scripts\blue.cmd add-file ...`
- Add Public Header → `scripts\blue.cmd add-file --blue-kind=public-header ...`
- Rename File → `scripts\blue.cmd rename-file ...`
- Remove From Project → `scripts\blue.cmd remove-file ...`
- Add Project → `scripts\blue.cmd add-project ...`

After successful mutations, the extension should run normal regeneration or build the generated `BlueRegenerateSolution` / `BlueScaffoldSolution` utility project.
