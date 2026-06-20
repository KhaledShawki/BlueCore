@echo off
setlocal EnableExtensions EnableDelayedExpansion

pushd "%~dp0.." >nul
set "ROOT_DIR=%CD%"
popd >nul

set "REQUESTED_CONFIG=Debug_x64"
set "REQUESTED_PLATFORM="
set "REQUESTED_MEMORY_BACKEND=system"
set "REQUESTED_CONFIG_SET="

:ParseArgs
if "%~1"=="" goto EndParseArgs

set "ARG=%~1"

if /I "%ARG%"=="-h" goto Usage
if /I "%ARG%"=="--help" goto Usage

if /I "%ARG%"=="--memory-backend" (
    shift
    if "%~1"=="" (
        echo [BlueBuild] Missing value for --memory-backend
        goto UsageError
    )
    set "REQUESTED_MEMORY_BACKEND=%~1"
    shift
    goto ParseArgs
)

if /I "%ARG:~0,17%"=="--memory-backend=" (
    set "REQUESTED_MEMORY_BACKEND=%ARG:~17%"
    shift
    goto ParseArgs
)

if "%ARG:~0,2%"=="--" (
    echo [BlueBuild] Unknown option: %ARG%
    goto UsageError
)

if "%REQUESTED_CONFIG_SET%"=="" (
    set "REQUESTED_CONFIG=%ARG%"
    set "REQUESTED_CONFIG_SET=1"
) else if "%REQUESTED_PLATFORM%"=="" (
    set "REQUESTED_PLATFORM=%ARG%"
) else (
    echo [BlueBuild] Unexpected argument: %ARG%
    goto UsageError
)

shift
goto ParseArgs

:EndParseArgs

set "MEMORY_BACKEND="
if /I "%REQUESTED_MEMORY_BACKEND%"=="system" set "MEMORY_BACKEND=system"
if /I "%REQUESTED_MEMORY_BACKEND%"=="mimalloc" set "MEMORY_BACKEND=mimalloc"

if "%MEMORY_BACKEND%"=="" (
    echo [BlueBuild] Unsupported memory backend: %REQUESTED_MEMORY_BACKEND%
    echo [BlueBuild] Supported: system, mimalloc
    exit /b 1
)

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

if not "%REQUESTED_PLATFORM%"=="" set "BUILD_PLATFORM=%REQUESTED_PLATFORM%"

if "%BUILD_CONFIG%"=="" (
    echo [BlueBuild] Unsupported Windows test configuration: %REQUESTED_CONFIG%
    echo [BlueBuild] Supported: Debug_x64, Release_x64, Profile_x64, Shipping_x64
    echo [BlueBuild] Supported DLL platforms: Debug_x64_DLL, Release_x64_DLL, Profile_x64_DLL, Shipping_x64_DLL
    exit /b 1
)

if /I not "%BUILD_PLATFORM%"=="x64" if /I not "%BUILD_PLATFORM%"=="x64_DLL" (
    echo [BlueBuild] Unsupported Windows build platform: %BUILD_PLATFORM%
    echo [BlueBuild] Supported: x64, x64_DLL
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

echo [BlueBuild] Configuration      : %BUILD_CONFIG%_%BUILD_PLATFORM%
echo [BlueBuild] Memory backend     : %MEMORY_BACKEND%
echo [BlueBuild] Generating VS2026 solution

call "%ROOT_DIR%\scripts\premake-windows.cmd" vs2026 --toolchain=msvc --blue-platforms=windows --blue-build-platforms=%BUILD_PLATFORM% --memory-backend=%MEMORY_BACKEND% --blue-startup=BlueRunTests
if errorlevel 1 exit /b %errorlevel%

set "BUILD_ROOT=%ROOT_DIR%\out\build\vs2026"
set "RUNNER_PROJECT=%BUILD_ROOT%\BlueRunTests\BlueRunTests.vcxproj"
set "BIN_DIR=%ROOT_DIR%\out\bin\windows\%BUILD_PLATFORM%\%BUILD_CONFIG%"
set "RUNNER_EXE=%BIN_DIR%\BlueRunTests.exe"

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
echo [BlueBuild] Building BlueRunTests

msbuild "%RUNNER_PROJECT%" /m /nr:false /t:Build /p:Configuration=%BUILD_CONFIG% /p:Platform=%BUILD_PLATFORM% /v:minimal
if errorlevel 1 exit /b %errorlevel%

if not exist "%RUNNER_EXE%" (
    echo [BlueBuild] Test runner executable was not built: %RUNNER_EXE%
    exit /b 1
)

set "TEST_EXECUTABLES="
set "TEST_EXECUTABLE_COUNT=0"

for %%F in ("%BIN_DIR%\*Tests.exe") do (
    if exist "%%~fF" (
        if /I not "%%~nxF"=="BlueRunTests.exe" (
            set "TEST_EXECUTABLES=!TEST_EXECUTABLES! "%%~fF""
            set /A TEST_EXECUTABLE_COUNT+=1
        )
    )
)

if "%TEST_EXECUTABLE_COUNT%"=="0" (
    echo [BlueBuild] No test executables found in %BIN_DIR%
    exit /b 1
)

echo [BlueBuild] Running %TEST_EXECUTABLE_COUNT% test executables
"%RUNNER_EXE%" --jobs=auto !TEST_EXECUTABLES!
exit /b %errorlevel%

:Usage
echo Usage:
echo   scripts\run-tests-windows.cmd [Debug_x64^|Release_x64^|Profile_x64^|Shipping_x64] [x64^|x64_DLL] [--memory-backend=system^|mimalloc]
echo.
echo Examples:
echo   scripts\run-tests-windows.cmd
echo   scripts\run-tests-windows.cmd Debug_x64
echo   scripts\run-tests-windows.cmd Debug_x64 --memory-backend=mimalloc
echo   scripts\run-tests-windows.cmd Debug_x64 x64_DLL --memory-backend=mimalloc
exit /b 0

:UsageError
echo.
goto Usage
