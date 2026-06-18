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

set "BUILD_ROOT=%ROOT_DIR%\out\build\vs2026"
set "RUNNER_PROJECT=%BUILD_ROOT%\BlueRunTests\BlueRunTests.vcxproj"

if not exist "%BUILD_ROOT%" (
    echo [BlueBuild] Expected VS2026 build directory was not generated: %BUILD_ROOT%
    exit /b 1
)

if not exist "%RUNNER_PROJECT%" (
    echo [BlueBuild] Expected test runner project was not generated: %RUNNER_PROJECT%
    dir "%BUILD_ROOT%"
    exit /b 1
)

set "BUILT_TEST_COUNT=0"

for /D %%D in ("%BUILD_ROOT%\*Tests") do (
    if /I not "%%~nxD"=="BlueRunTests" (
        set "TEST_PROJECT=%%~fD\%%~nxD.vcxproj"

        if exist "!TEST_PROJECT!" (
            echo [BlueBuild] Building %%~nxD
            msbuild "!TEST_PROJECT!" /m /nr:false /t:Build /p:Configuration=%BUILD_CONFIG% /p:Platform=%BUILD_PLATFORM% /v:minimal
            if errorlevel 1 exit /b !errorlevel!

            set /A BUILT_TEST_COUNT+=1
        )
    )
)

if "%BUILT_TEST_COUNT%"=="0" (
    echo [BlueBuild] No test projects were found under %BUILD_ROOT%
    dir "%BUILD_ROOT%"
    exit /b 1
)

echo [BlueBuild] Built %BUILT_TEST_COUNT% test projects.
echo [BlueBuild] Building and running BlueRunTests

msbuild "%RUNNER_PROJECT%" /m /nr:false /t:Build /p:Configuration=%BUILD_CONFIG% /p:Platform=%BUILD_PLATFORM% /v:minimal
exit /b %errorlevel%
