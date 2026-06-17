# Blue Project Command Layer

Blue project files are source-controlled declarations. Generated Visual Studio projects under `out/build/` are considered disposable output.

The Blue command layer provides a stable set of commands for modifying project structure. These commands should be used instead of manually editing `project.lua`, `build.lua`, or generated project files.

## Command Model

The command layer is implemented in Lua and exposed through Premake actions. Wrapper scripts provide a stable entry point:

**Windows**

```bat
scripts\blue.cmd add-file --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp
scripts\blue.cmd remove-file --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp
scripts\blue.cmd rename-file --blue-project=BlueSystem --blue-kind=source --blue-from=Old.cpp --blue-to=New.cpp
scripts\blue.cmd add-project --blue-project=BlueGraphics --blue-type=library --blue-linkage=auto
```

**Linux / macOS**

```sh
scripts/blue.sh add-file --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp
```

## Supported File Kinds

| Kind              | Manifest Section              | Physical Path Rule |
|-------------------|-------------------------------|--------------------|
| `source`          | `files.sources`               | `src/<path>` |
| `public-header`   | `files.public_headers`        | `include/<ProjectName>/<path>` (unless path already starts with `include/`) |
| `private-header`  | `files.private_headers`       | `src/<path>` |
| `windows-source`  | `files.platform.windows.sources` | `src/Platform/Windows/<path>` |
| `linux-source`    | `files.platform.linux.sources`   | `src/Platform/Linux/<path>` |
| `macos-source`    | `files.platform.macosx.sources`  | `src/Platform/MacOS/<path>` |
| `posix-source`    | `files.platform.linux.sources`   | `src/Platform/POSIX/<path>` |

## Safety Rules

- Commands only modify declared Blue manifests.
- Absolute paths, wildcards, and paths escaping the project root are rejected.
- `add-file` creates the file from a template unless `--blue-no-create` is passed.
- `remove-file` removes only the manifest entry by default. Use `--blue-delete-file` to also delete the physical file.
- `rename-file` updates the manifest and renames the physical file unless `--blue-no-create` is passed.
- `add-project` updates `build.lua` and creates a project template unless `--blue-no-create` is passed.
- Existing files are never overwritten.
- Generated Visual Studio projects are never modified by the command layer.

## Usage from IDEs

IDEs and editor extensions can use these commands to perform project mutations safely:

- Add Source File → `scripts\blue.cmd add-file ...`
- Add Public Header → `scripts\blue.cmd add-file --blue-kind=public-header ...`
- Rename File → `scripts\blue.cmd rename-file ...`
- Remove From Project → `scripts\blue.cmd remove-file ...`
- Add Project → `scripts\blue.cmd add-project ...`

After making changes, run a normal regeneration step so that generated project files are updated.