$RootDir = Resolve-Path "$PSScriptRoot\.."
$Premake = Join-Path $RootDir "tools\premake\windows\premake5.exe"

if (!(Test-Path $Premake)) {
    Write-Error "Premake executable not found: $Premake"
    exit 1
}

& $Premake --file="$RootDir\premake5.lua" @args
exit $LASTEXITCODE
