@echo off
setlocal
set "ROOT_DIR=%~dp0.."
set "PREMAKE=%ROOT_DIR%\tools\premake\windows\premake5.exe"
if not exist "%PREMAKE%" (
    echo Premake executable not found: %PREMAKE%
    exit /b 1
)
"%PREMAKE%" --file="%ROOT_DIR%\premake5.lua" %*
