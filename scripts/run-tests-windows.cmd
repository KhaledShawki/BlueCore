@echo off
setlocal

set ROOT_DIR=%~dp0..
set CONFIG=%~1
if "%CONFIG%"=="" set CONFIG=Debug
set PLATFORM=%~2
if "%PLATFORM%"=="" set PLATFORM=x64

call "%ROOT_DIR%\scripts\premake-windows.cmd" vs2026 --toolchain=msvc --blue-platforms=windows
if errorlevel 1 exit /b %errorlevel%

msbuild "%ROOT_DIR%\out\build\vs2026\Blue.slnx" /m /t:BlueRunTests /p:Configuration=%CONFIG% /p:Platform=%PLATFORM%
exit /b %errorlevel%
