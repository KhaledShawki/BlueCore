[CmdletBinding()]
param(
    [ValidateSet("format", "check", "list")]
    [string]$Mode = "format",

    [string]$ClangFormat = $env:BLUE_CLANG_FORMAT,
    [string]$StyLua = $env:BLUE_STYLUA,
    [string]$Black = $env:BLUE_BLACK
)

$ErrorActionPreference = "Stop"
$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..")

function Convert-ToRelativePath {
    param([string]$FullName)

    $rootText = $RootDir.Path.TrimEnd("\", "/")
    $relative = $FullName.Substring($rootText.Length).TrimStart("\", "/")
    return $relative.Replace("/", "\")
}

function Resolve-CommandPath {
    param(
        [string]$ExplicitPath,
        [string]$DisplayName,
        [string[]]$CandidatePaths,
        [string[]]$CandidateCommands
    )

    if ($ExplicitPath) {
        if (Test-Path $ExplicitPath) {
            return (Resolve-Path $ExplicitPath).Path
        }

        $explicitCommand = Get-Command $ExplicitPath -ErrorAction SilentlyContinue
        if ($explicitCommand) {
            return $explicitCommand.Source
        }

        throw "$DisplayName was not found from explicit value: $ExplicitPath"
    }

    foreach ($candidatePath in $CandidatePaths) {
        if ($candidatePath -and (Test-Path $candidatePath)) {
            return (Resolve-Path $candidatePath).Path
        }
    }

    foreach ($candidateCommand in $CandidateCommands) {
        $command = Get-Command $candidateCommand -ErrorAction SilentlyContinue
        if ($command) {
            return $command.Source
        }
    }

    throw "$DisplayName was not found. Install it or set the matching BLUE_* environment variable."
}

function Resolve-ClangFormat {
    $repoLocal = Join-Path $RootDir "tools\clang-format\windows\clang-format.exe"
    $llvmInstall = "C:\Program Files\LLVM\bin\clang-format.exe"
    $candidatePaths = New-Object System.Collections.Generic.List[string]
    $candidatePaths.Add($repoLocal)
    $candidatePaths.Add($llvmInstall)

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $visualStudioRoot = & $vswhere -latest -property installationPath 2>$null
        if ($LASTEXITCODE -eq 0 -and $visualStudioRoot) {
            $candidatePaths.Add((Join-Path $visualStudioRoot "VC\Tools\Llvm\x64\bin\clang-format.exe"))
        }
    }

    return Resolve-CommandPath `
        -ExplicitPath $ClangFormat `
        -DisplayName "clang-format" `
        -CandidatePaths $candidatePaths.ToArray() `
        -CandidateCommands @("clang-format")
}

function Resolve-StyLua {
    return Resolve-CommandPath `
        -ExplicitPath $StyLua `
        -DisplayName "stylua" `
        -CandidatePaths @((Join-Path $RootDir "tools\stylua\windows\stylua.exe")) `
        -CandidateCommands @("stylua")
}

function Resolve-BlackCommand {
    if ($Black) {
        if (Test-Path $Black) {
            return @{ FilePath = (Resolve-Path $Black).Path; Arguments = @() }
        }

        $explicitCommand = Get-Command $Black -ErrorAction SilentlyContinue
        if ($explicitCommand) {
            return @{ FilePath = $explicitCommand.Source; Arguments = @() }
        }

        throw "black was not found from BLUE_BLACK value: $Black"
    }

    $repoLocal = Join-Path $RootDir "tools\black\windows\black.exe"
    if (Test-Path $repoLocal) {
        return @{ FilePath = (Resolve-Path $repoLocal).Path; Arguments = @() }
    }

    $blackCommand = Get-Command black -ErrorAction SilentlyContinue
    if ($blackCommand) {
        return @{ FilePath = $blackCommand.Source; Arguments = @() }
    }

    foreach ($pythonName in @("python3", "python")) {
        $pythonCommand = Get-Command $pythonName -ErrorAction SilentlyContinue
        if (!$pythonCommand) {
            continue
        }

        & $pythonCommand.Source -m black --version *> $null
        if ($LASTEXITCODE -eq 0) {
            return @{ FilePath = $pythonCommand.Source; Arguments = @("-m", "black") }
        }
    }

    throw "black was not found. Install it with: python -m pip install black"
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

function Add-ExistingFile {
    param(
        [System.Collections.Generic.List[string]]$Result,
        [string]$RelativePath
    )

    $fullPath = Join-Path $RootDir $RelativePath
    if (Test-Path $fullPath) {
        $Result.Add((Resolve-Path $fullPath).Path)
    }
}

function Add-FilesByExtension {
    param(
        [System.Collections.Generic.List[string]]$Result,
        [string]$RelativeRoot,
        [string[]]$Extensions,
        [string[]]$IgnorePatterns = @()
    )

    $fullRoot = Join-Path $RootDir $RelativeRoot
    if (!(Test-Path $fullRoot)) {
        return
    }

    Get-ChildItem -Path $fullRoot -Recurse -File -Include $Extensions | ForEach-Object {
        $relative = Convert-ToRelativePath $_.FullName
        if (!(Test-IgnoredPath $relative $IgnorePatterns)) {
            $Result.Add($_.FullName)
        }
    }
}

function Get-CppFormatFiles {
    $roots = @("modules", "apps", "tests", "tools")
    $extensions = @("*.h", "*.hpp", "*.hxx", "*.inl", "*.c", "*.cc", "*.cpp", "*.cxx")
    $ignorePatterns = Get-IgnorePatterns
    $result = New-Object System.Collections.Generic.List[string]

    foreach ($root in $roots) {
        Add-FilesByExtension $result $root $extensions $ignorePatterns
    }

    return $result | Sort-Object -Unique
}

function Get-LuaFormatFiles {
    $roots = @("build", "modules", "apps", "tests", "tools")
    $result = New-Object System.Collections.Generic.List[string]

    Add-ExistingFile $result "build.lua"
    Add-ExistingFile $result "premake5.lua"

    foreach ($root in $roots) {
        Add-FilesByExtension $result $root @("*.lua")
    }

    return $result | Sort-Object -Unique
}

function Get-PythonFormatFiles {
    $roots = @("scripts", "tools", "benchmarks")
    $result = New-Object System.Collections.Generic.List[string]

    foreach ($root in $roots) {
        Add-FilesByExtension $result $root @("*.py")
    }

    return $result | Sort-Object -Unique
}

function Write-FileSection {
    param(
        [string]$Name,
        [string[]]$Files
    )

    Write-Host "[$Name]"
    foreach ($file in $Files) {
        Write-Host (Convert-ToRelativePath $file)
    }
}

function Invoke-ClangFormat {
    param(
        [string]$ToolPath,
        [string[]]$Files
    )

    if ($Files.Count -eq 0) {
        return 0
    }

    Write-Host "[BlueFormat] clang-format: $ToolPath"
    Write-Host "[BlueFormat] C/C++ files: $($Files.Count)"

    $failed = 0
    foreach ($file in $Files) {
        if ($Mode -eq "check") {
            & $ToolPath --style=file --dry-run --Werror $file
        }
        else {
            & $ToolPath --style=file -i $file
        }

        if ($LASTEXITCODE -ne 0) {
            Write-Error "clang-format failed for: $file"
            ++$failed
        }
    }

    return $failed
}

function Invoke-StyLua {
    param(
        [string]$ToolPath,
        [string[]]$Files
    )

    if ($Files.Count -eq 0) {
        return 0
    }

    Write-Host "[BlueFormat] stylua: $ToolPath"
    Write-Host "[BlueFormat] Lua files: $($Files.Count)"

    $failed = 0
    foreach ($file in $Files) {
        if ($Mode -eq "check") {
            & $ToolPath --check $file
        }
        else {
            & $ToolPath $file
        }

        if ($LASTEXITCODE -ne 0) {
            Write-Error "stylua failed for: $file"
            ++$failed
        }
    }

    return $failed
}

function Invoke-Black {
    param(
        [hashtable]$Command,
        [string[]]$Files
    )

    if ($Files.Count -eq 0) {
        return 0
    }

    $display = @($Command.FilePath) + @($Command.Arguments)
    Write-Host "[BlueFormat] black: $($display -join ' ')"
    Write-Host "[BlueFormat] Python files: $($Files.Count)"

    $failed = 0
    foreach ($file in $Files) {
        $arguments = @($Command.Arguments)
        if ($Mode -eq "check") {
            $arguments += "--check"
        }
        $arguments += $file

        & $Command.FilePath @arguments
        if ($LASTEXITCODE -ne 0) {
            Write-Error "black failed for: $file"
            ++$failed
        }
    }

    return $failed
}

Assert-SingleRootClangFormat

$cppFiles = @(Get-CppFormatFiles)
$luaFiles = @(Get-LuaFormatFiles)
$pythonFiles = @(Get-PythonFormatFiles)

Write-Host "[BlueFormat] mode: $Mode"

if ($Mode -eq "list") {
    Write-FileSection "C/C++" $cppFiles
    Write-FileSection "Lua" $luaFiles
    Write-FileSection "Python" $pythonFiles
    exit 0
}

$clangFormatPath = Resolve-ClangFormat
$styLuaPath = Resolve-StyLua
$blackCommand = Resolve-BlackCommand

$failed = 0
$failed += Invoke-ClangFormat $clangFormatPath $cppFiles
$failed += Invoke-StyLua $styLuaPath $luaFiles
$failed += Invoke-Black $blackCommand $pythonFiles

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
