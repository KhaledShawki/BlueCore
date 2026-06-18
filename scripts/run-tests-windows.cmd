@echo off
setlocal EnableExtensions EnableDelayedExpansion

pushd "%~dp0.." >nul
set "ROOT_DIR=%CD%"
popd >nul

set "REQUESTED_CONFIG=%~1"

if "%REQUESTED_CONFIG%"=="" set "REQUESTED_CONFIG=Debug_x64"

set "BUILD_CONFIG="
set "BUILD_PLATFORM="

for %%C in (Debug Release Profile Shipping) do (
    if /I "%REQUESTED_CONFIG%"=="%%C" (
        set "BUILD_CONFIG=%%C"
        set "BUILD_PLATFORM=x64"
    )

    if /I "%REQUESTED_CONFIG%"=="%%C_x64" (
        set "BUILD_CONFIG=%%C"
        set "BUILD_PLATFORM=x64"
    )

    if /I "%REQUESTED_CONFIG%"=="%%C_x64_DLL" (
        set "BUILD_CONFIG=%%C"
        set "BUILD_PLATFORM=x64_DLL"
    )
)

if not "%~2"=="" set "BUILD_PLATFORM=%~2"

if "%BUILD_CONFIG%"=="" (
    echo [BlueBuild] Unsupported Windows test configuration: %REQUESTED_CONFIG%
    echo [BlueBuild] Supported: Debug_x64, Release_x64, Profile_x64, Shipping_x64
    echo [BlueBuild] Supported DLL platforms: Debug_x64_DLL, Release_x64_DLL, Profile_x64_DLL, Shipping_x64_DLL
    exit /b 1
)

if not exist "%ROOT_DIR%\scripts\premake-windows.cmd" (
    echo [BlueBuild] Required script not found: %ROOT_DIR%\scripts\premake-windows.cmd
    exit /b 1
)

where msbuild >nul 2>nul
if errorlevel 1 (
    echo [BlueBuild] Required command not found: MSBuild.
    echo [BlueBuild] Use microsoft/setup-msbuild@v3 in GitHub Actions or run from a Visual Studio Developer Command Prompt.
    exit /b 1
)

call "%ROOT_DIR%\scripts\premake-windows.cmd" vs2026 --toolchain=msvc --blue-platforms=windows --blue-build-platforms=%BUILD_PLATFORM% --memory-backend=system --blue-startup=BlueRunTests
if errorlevel 1 exit /b %errorlevel%

set "SOLUTION=%ROOT_DIR%\out\build\vs2026\Blue.slnx"

if not exist "%SOLUTION%" (
    set "SOLUTION=%ROOT_DIR%\out\build\vs2026\Blue.sln"
)

if not exist "%SOLUTION%" (
    echo [BlueBuild] Expected VS2026 solution was not generated.
    echo [BlueBuild] Checked:
    echo [BlueBuild]   %ROOT_DIR%\out\build\vs2026\Blue.slnx
    echo [BlueBuild]   %ROOT_DIR%\out\build\vs2026\Blue.sln

    if exist "%ROOT_DIR%\out\build\vs2026" (
        dir "%ROOT_DIR%\out\build\vs2026"
    )

    exit /b 1
)

msbuild "%SOLUTION%" /m /nr:false /t:BlueRunTests /p:Configuration=%BUILD_CONFIG% /p:Platform=%BUILD_PLATFORM% /v:minimal
if errorlevel 1 exit /b %errorlevel%

set "BIN_DIR=%ROOT_DIR%\out\bin\windows\%BUILD_PLATFORM%\%BUILD_CONFIG%"
set "RUNNER=%BIN_DIR%\BlueRunTests.exe"

if not exist "%RUNNER%" (
    echo [BlueBuild] Test runner was not built: %RUNNER%

    if exist "%BIN_DIR%" (
        dir "%BIN_DIR%"
    )

    exit /b 1
)

set "TEST_ARGS="

for %%F in ("%BIN_DIR%\*Tests.exe") do (
    if exist "%%~fF" (
        if /I not "%%~nxF"=="BlueRunTests.exe" (
            set "TEST_ARGS=!TEST_ARGS! "%%~fF""
        )
    )
)

if "%TEST_ARGS%"=="" (
    echo [BlueBuild] No test executables found in %BIN_DIR%

    if exist "%BIN_DIR%" (
        dir "%BIN_DIR%"
    )

    exit /b 1
)

"%RUNNER%" %TEST_ARGS%
exit /b %errorlevel%
