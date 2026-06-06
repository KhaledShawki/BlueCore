$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $ScriptDir "premake-windows.ps1") vs2022 --toolchain=msvc --blue-platforms=windows --msvc-toolset=v143 @args
exit $LASTEXITCODE
