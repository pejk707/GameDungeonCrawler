@echo off
rem Builds the game with MSVC from Visual Studio 2022 ("Desktop development with C++" workload).
rem Usage: build.bat        - build Release into .\build
rem        build.bat test   - build and run unit tests
setlocal
set "ROOT=%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo vswhere.exe not found: install Visual Studio 2022 with the C++ workload.
    exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
    echo Visual Studio with the x64 C++ compiler was not found.
    exit /b 1
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 || exit /b 1
cmake -S "%ROOT%." -B "%ROOT%build" -G Ninja -DCMAKE_BUILD_TYPE=Release || exit /b 1
cmake --build "%ROOT%build" || exit /b 1
if /i "%~1"=="test" (
    "%ROOT%build\lamplighter_tests.exe" || exit /b 1
)
exit /b 0
