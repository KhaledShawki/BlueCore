@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0format-windows.ps1" -Mode check %*
exit /b %ERRORLEVEL%
