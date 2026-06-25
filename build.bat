@echo off
setlocal

set BUILD_DIR=build\msvc
set GENERATOR=Visual Studio 17 2022
set CONFIG=Release

:args
if "%1"=="debug"   set CONFIG=Debug   & shift & goto args
if "%1"=="release" set CONFIG=Release & shift & goto args
if "%1"=="clean"   set DO_CLEAN=1     & shift & goto args
if "%1"=="test"    set DO_TEST=1      & shift & goto args
if "%1"==""        goto start
echo Unknown argument: %1
goto :eof

:start

if not defined CMAKE_ROOT (
    echo CMAKE_ROOT not specified. & exit /b 1
)

set CMAKE="%CMAKE_ROOT%\bin\cmake.exe"
set CTEST="%CMAKE_ROOT%\bin\ctest.exe"

if defined DO_CLEAN (
    echo Cleaning %BUILD_DIR%...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
)

if not exist %BUILD_DIR%\CMakeCache.txt (
    echo Configuring [%GENERATOR%] ...
    %CMAKE% -B %BUILD_DIR% ^
        -G "%GENERATOR%" ^
        -DPDFLIB_BUILD_TESTS=ON ^
        -DPDFLIB_BUILD_EXAMPLES=ON
    if errorlevel 1 ( echo Configure failed. & exit /b 1 )
)

echo Building [%CONFIG%] ...
%CMAKE% --build %BUILD_DIR% --config %CONFIG% --parallel 4
if errorlevel 1 ( echo Build failed. & exit /b 1 )
echo Build succeeded.

if defined DO_TEST (
    echo Running tests...
    pushd %BUILD_DIR%
    %CTEST% -C %CONFIG% --output-on-failure
    popd
)

endlocal
