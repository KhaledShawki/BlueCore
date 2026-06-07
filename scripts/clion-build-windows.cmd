@echo off
setlocal EnableExtensions

set "ROOT_DIR=%~dp0.."
set "TARGET=%~1"
set "CONFIGURATION=%~2"
set "PLATFORM=%~3"

if "%TARGET%"=="" (
    echo Usage: %~nx0 ^<target^> [configuration] [platform]
    exit /b 2
)

if "%CONFIGURATION%"=="" set "CONFIGURATION=Debug"
if "%PLATFORM%"=="" set "PLATFORM=x64"
if "%BLUE_CLION_VS_ACTION%"=="" set "BLUE_CLION_VS_ACTION=vs2026"
if "%BLUE_MEMORY_BACKEND%"=="" set "BLUE_MEMORY_BACKEND=system"

call "%ROOT_DIR%\scripts\premake-windows.cmd" %BLUE_CLION_VS_ACTION% --toolchain=msvc --blue-platforms=windows --memory-backend=%BLUE_MEMORY_BACKEND%
if errorlevel 1 exit /b %ERRORLEVEL%

msbuild "%ROOT_DIR%\out\build\%BLUE_CLION_VS_ACTION%\Blue.slnx" /m /t:%TARGET% /p:Configuration=%CONFIGURATION% /p:Platform=%PLATFORM%
exit /b %ERRORLEVEL%
