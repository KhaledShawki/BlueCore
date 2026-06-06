[CmdletBinding()]
param(
    [ValidateSet("format", "check", "list")]
    [string]$Mode = "format",

    [string]$ClangFormat = $env:BLUE_CLANG_FORMAT
)

$ErrorActionPreference = "Stop"
$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..")

function Resolve-ClangFormat {
    param([string]$ExplicitPath)

    if ($ExplicitPath) {
        if (Test-Path $ExplicitPath) {
            return (Resolve-Path $ExplicitPath).Path
        }

        $explicitCommand = Get-Command $ExplicitPath -ErrorAction SilentlyContinue
        if ($explicitCommand) {
            return $explicitCommand.Source
        }

        throw "clang-format was not found from BLUE_CLANG_FORMAT value: $ExplicitPath"
    }

    $repoLocal = Join-Path $RootDir "tools/clang-format/windows/clang-format.exe"
    if (Test-Path $repoLocal) {
        return $repoLocal
    }

    $llvmInstall = "C:\Program Files\LLVM\bin\clang-format.exe"
    if (Test-Path $llvmInstall) {
        return $llvmInstall
    }

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $visualStudioRoot = & $vswhere -latest -property installationPath 2>$null
        if ($LASTEXITCODE -eq 0 -and $visualStudioRoot) {
            $vsLlvm = Join-Path $visualStudioRoot "VC\Tools\Llvm\x64\bin\clang-format.exe"
            if (Test-Path $vsLlvm) {
                return $vsLlvm
            }
        }
    }

    $fromPath = Get-Command clang-format -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    throw "clang-format was not found. Install LLVM, add clang-format to PATH, or set BLUE_CLANG_FORMAT."
}

function Assert-SingleRootClangFormat {
    $formatFiles = Get-ChildItem -Path $RootDir -Recurse -Force -File -Include ".clang-format", "_clang-format" |
        Where-Object {
            $relative = Convert-ToRelativePath $_.FullName
            $relative -notlike "out\*" -and
            $relative -notlike "third_party\*" -and
            $relative -notlike "external\*" -and
            $relative -notlike "tools\premake\*" -and
            $relative -notlike "tools\clang-format\*"
        }

    $rootFormat = Join-Path $RootDir ".clang-format"
    foreach ($file in $formatFiles) {
        if ((Resolve-Path $file.FullName).Path -ne (Resolve-Path $rootFormat).Path) {
            throw "Nested clang-format file is not allowed: $(Convert-ToRelativePath $file.FullName). Keep only the repository root .clang-format."
        }
    }
}

function Get-IgnorePatterns {
    $ignoreFile = Join-Path $RootDir ".clang-format-ignore"
    if (!(Test-Path $ignoreFile)) {
        return @()
    }

    return Get-Content $ignoreFile |
        ForEach-Object { $_.Trim() } |
        Where-Object { $_ -and !$_.StartsWith("#") } |
        ForEach-Object { $_.Replace("/", "\").Replace("**", "*") }
}

function Convert-ToRelativePath {
    param([string]$FullName)

    $rootText = $RootDir.Path.TrimEnd("\", "/")
    $relative = $FullName.Substring($rootText.Length).TrimStart("\", "/")
    return $relative.Replace("/", "\")
}

function Test-IgnoredPath {
    param(
        [string]$RelativePath,
        [string[]]$Patterns
    )

    foreach ($pattern in $Patterns) {
        if ($RelativePath -like $pattern) {
            return $true
        }
    }

    return $false
}

function Get-FormatFiles {
    $roots = @("modules", "apps", "tests", "tools")
    $extensions = @("*.h", "*.hpp", "*.hxx", "*.inl", "*.c", "*.cc", "*.cpp", "*.cxx")
    $ignorePatterns = Get-IgnorePatterns
    $result = New-Object System.Collections.Generic.List[string]

    foreach ($root in $roots) {
        $fullRoot = Join-Path $RootDir $root
        if (!(Test-Path $fullRoot)) {
            continue
        }

        Get-ChildItem -Path $fullRoot -Recurse -File -Include $extensions | ForEach-Object {
            $relative = Convert-ToRelativePath $_.FullName
            if (!(Test-IgnoredPath $relative $ignorePatterns)) {
                $result.Add($_.FullName)
            }
        }
    }

    return $result | Sort-Object
}

Assert-SingleRootClangFormat

$clangFormatPath = Resolve-ClangFormat $ClangFormat
$files = @(Get-FormatFiles)

if ($files.Count -eq 0) {
    Write-Host "[BlueFormat] No C/C++ files matched formatting scope."
    exit 0
}

Write-Host "[BlueFormat] clang-format: $clangFormatPath"
Write-Host "[BlueFormat] mode: $Mode"
Write-Host "[BlueFormat] files: $($files.Count)"

if ($Mode -eq "list") {
    foreach ($file in $files) {
        Write-Host (Convert-ToRelativePath $file)
    }
    exit 0
}

$failed = 0
foreach ($file in $files) {
    if ($Mode -eq "check") {
        & $clangFormatPath --style=file --dry-run --Werror $file
    }
    else {
        & $clangFormatPath --style=file -i $file
    }

    if ($LASTEXITCODE -ne 0) {
        Write-Error "clang-format failed for: $file"
        ++$failed
    }
}

if ($failed -ne 0) {
    Write-Error "[BlueFormat] Formatting failed for $failed file(s)."
    exit 1
}

if ($Mode -eq "check") {
    Write-Host "[BlueFormat] Formatting check passed."
}
else {
    Write-Host "[BlueFormat] Formatting completed."
}
