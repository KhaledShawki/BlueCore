# IDE Format-on-Save

IDE format-on-save is optional. The enforced workflow is the repository formatter check.

## Visual Studio

- Configure Visual Studio to use clang-format.
- Let it discover the repository root `.clang-format` file.
- Prefer the generated `BlueFormat` and `BlueFormatCheck` targets before committing.

Do not rely on Visual Studio's native C++ formatter for Blue source files.

## CLion

- Enable ClangFormat support.
- Let CLion discover the root `.clang-format` file.
- Run `BlueFormatCheck` or the script-based check before committing.

## VS Code

The repository includes workspace settings for clang-format integration. Developers can still run the script-based check to verify the final result.

## Enforcement path

```text
scripts/format-check-*.cmd/.sh
BlueFormatCheck
CI format check
```
