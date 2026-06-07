@echo off
setlocal
set "ROOT_DIR=%~dp0.."

if "%~1"=="" goto :usage
set "BLUE_COMMAND=%~1"
shift /1

if /I "%BLUE_COMMAND%"=="add-file" goto :run
if /I "%BLUE_COMMAND%"=="remove-file" goto :run
if /I "%BLUE_COMMAND%"=="rename-file" goto :run
if /I "%BLUE_COMMAND%"=="add-project" goto :run

echo Unknown Blue command: %BLUE_COMMAND% 1>&2
exit /b 2

:run
call "%ROOT_DIR%\scripts\premake-windows.cmd" blue-%BLUE_COMMAND% %*
exit /b %ERRORLEVEL%

:usage
echo Usage:
echo   scripts\blue.cmd add-file --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp
echo   scripts\blue.cmd remove-file --blue-project=BlueSystem --blue-kind=source --blue-path=Log/FileLogger.cpp [--blue-delete-file]
echo   scripts\blue.cmd rename-file --blue-project=BlueSystem --blue-kind=source --blue-from=Old.cpp --blue-to=New.cpp
echo   scripts\blue.cmd add-project --blue-project=BlueGraphics [--blue-type=library] [--blue-linkage=auto]
exit /b 2
