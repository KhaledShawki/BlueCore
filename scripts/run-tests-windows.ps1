param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"
$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..")

& (Join-Path $RootDir "scripts/premake-windows.ps1") vs2026 --toolchain=msvc --blue-platforms=windows
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& msbuild (Join-Path $RootDir "out/build/vs2026/Blue.slnx") /m /t:BlueRunTests /p:Configuration=$Configuration /p:Platform=$Platform
exit $LASTEXITCODE
