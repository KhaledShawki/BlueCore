@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
call "%SCRIPT_DIR%premake-windows.cmd" vs2022 --toolchain=msvc --blue-platforms=windows --msvc-toolset=v143 %*
