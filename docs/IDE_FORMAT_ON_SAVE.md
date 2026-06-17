# IDE Format-on-Save

IDE format-on-save is optional. The enforced workflow is the repository formatter check performed before committing.

## Visual Studio

- Configure Visual Studio to use `clang-format`.
- Ensure it discovers the repository root `.clang-format` file.
- Prefer running the generated `BlueFormat` and `BlueFormatCheck` targets before committing.

Do not rely on Visual Studio’s native C++ formatter for BlueCore source files.

## CLion

- Enable ClangFormat support in CLion.
- Ensure CLion discovers the root `.clang-format` file.
- Run `BlueFormatCheck` or the script-based check before committing.

## VS Code

The repository includes workspace settings for `clang-format` integration. Developers can still run the script-based format check to verify the final result.

## Enforcement Path

Formatting is enforced through the following mechanisms:

```text
scripts/format-check-*.cmd/.sh
BlueFormatCheck
CI format check
```
