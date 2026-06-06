@echo off
setlocal
set "ROOT_DIR=%~dp0.."

call "%ROOT_DIR%\scripts\premake-windows.cmd" vs2026 --toolchain=msvc --blue-platforms=windows --blue-startup=BlueRunTests %*
exit /b %errorlevel%
