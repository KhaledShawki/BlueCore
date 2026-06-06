param(
    [string]$Action = "vs2022",
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$PremakeArguments
)

$ErrorActionPreference = "Stop"
$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$Premake = Join-Path $RootDir "tools/premake/windows/premake5.exe"

function Convert-PremakeArguments {
    param([string[]]$Arguments)

    $valueOptions = @{
        "--toolchain" = $true
        "--blue-platforms" = $true
        "--memory-backend" = $true
        "--blue-startup" = $true
        "--msvc-toolset" = $true
        "--msvc-tools-version" = $true
    }

    $result = New-Object System.Collections.Generic.List[string]
    for ($index = 0; $index -lt $Arguments.Length; ++$index) {
        $argument = $Arguments[$index]

        if ($argument -match '^--([^=]+)=(.*)$') {
            $result.Add($argument)
            continue
        }

        if ($valueOptions.ContainsKey($argument)) {
            if (($index + 1) -ge $Arguments.Length) {
                throw "Missing value for option $argument."
            }

            $value = $Arguments[$index + 1]
            $result.Add("$argument=$value")
            ++$index
            continue
        }

        $result.Add($argument)
    }

    return [string[]]$result
}

if (!(Test-Path $Premake)) {
    Write-Error "Premake executable not found: $Premake"
}

$NormalizedPremakeArguments = Convert-PremakeArguments -Arguments $PremakeArguments

& $Premake --file="$RootDir/premake5.lua" --regen-action=$Action @NormalizedPremakeArguments check-regeneration
$check = $LASTEXITCODE

if ($check -eq 0) {
    Write-Host "[BlueBuild] Regeneration skipped."
    exit 0
}

if ($check -ne 2) {
    Write-Error "Regeneration check failed with code $check."
}

& $Premake --file="$RootDir/premake5.lua" @NormalizedPremakeArguments $Action
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $Premake --file="$RootDir/premake5.lua" --regen-action=$Action @NormalizedPremakeArguments update-build-token
exit $LASTEXITCODE
