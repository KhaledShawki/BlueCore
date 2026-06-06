@echo off
setlocal enabledelayedexpansion
set "ROOT_DIR=%~dp0.."
set "PREMAKE=%ROOT_DIR%\tools\premake\windows\premake5.exe"

if not exist "%PREMAKE%" (
    echo [BlueBuild] Premake executable not found: %PREMAKE%
    exit /b 1
)

set "GEN_ACTION=%~1"
if "%GEN_ACTION%"=="" set "GEN_ACTION=vs2022"
if not "%~1"=="" shift /1

set "GEN_ARGS="
:collect_args
if "%~1"=="" goto args_done
set "ARG=%~1"
call :append_normalized_arg "%ARG%" "%~2"
if "%ARG_CONSUMED_NEXT%"=="1" shift /1
shift /1
goto collect_args

:append_normalized_arg
set "ARG_CONSUMED_NEXT=0"
set "CURRENT_ARG=%~1"
set "NEXT_ARG=%~2"

if /i "%CURRENT_ARG:~0,12%"=="--toolchain=" goto append_equals
if /i "%CURRENT_ARG:~0,17%"=="--blue-platforms=" goto append_equals
if /i "%CURRENT_ARG:~0,17%"=="--memory-backend=" goto append_equals
if /i "%CURRENT_ARG:~0,15%"=="--blue-startup=" goto append_equals
if /i "%CURRENT_ARG:~0,14%"=="--msvc-toolset=" goto append_equals
if /i "%CURRENT_ARG:~0,21%"=="--msvc-tools-version=" goto append_equals

if /i "%CURRENT_ARG%"=="--toolchain" goto append_split_value
if /i "%CURRENT_ARG%"=="--blue-platforms" goto append_split_value
if /i "%CURRENT_ARG%"=="--memory-backend" goto append_split_value
if /i "%CURRENT_ARG%"=="--blue-startup" goto append_split_value
if /i "%CURRENT_ARG%"=="--msvc-toolset" goto append_split_value
if /i "%CURRENT_ARG%"=="--msvc-tools-version" goto append_split_value

goto append_plain

:append_equals
set "GEN_ARGS=!GEN_ARGS! %CURRENT_ARG%"
exit /b 0

:append_split_value
if "%NEXT_ARG%"=="" (
    echo [BlueBuild] Missing value for option %CURRENT_ARG%.
    exit /b 1
)
set "GEN_ARGS=!GEN_ARGS! %CURRENT_ARG%=%NEXT_ARG%"
set "ARG_CONSUMED_NEXT=1"
exit /b 0

:append_plain
set "GEN_ARGS=!GEN_ARGS! %CURRENT_ARG%"
exit /b 0

:args_done

echo [BlueBuild] Checking build graph token for %GEN_ACTION%...
"%PREMAKE%" --file="%ROOT_DIR%\premake5.lua" --regen-action=%GEN_ACTION% !GEN_ARGS! check-regeneration
set "CHECK_RESULT=%ERRORLEVEL%"

if "%CHECK_RESULT%"=="0" (
    echo [BlueBuild] Regeneration skipped.
    exit /b 0
)

if not "%CHECK_RESULT%"=="2" (
    echo [BlueBuild] Regeneration check failed with code %CHECK_RESULT%.
    exit /b %CHECK_RESULT%
)

echo [BlueBuild] Regenerating %GEN_ACTION%...
"%PREMAKE%" --file="%ROOT_DIR%\premake5.lua" !GEN_ARGS! %GEN_ACTION%
if errorlevel 1 exit /b %ERRORLEVEL%

"%PREMAKE%" --file="%ROOT_DIR%\premake5.lua" --regen-action=%GEN_ACTION% !GEN_ARGS! update-build-token
exit /b %ERRORLEVEL%
