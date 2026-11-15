@echo off
rem ===========================================================================
rem  open_in_visual_studio.bat  --  make the Visual Studio solution and open it
rem
rem  Double-click this file. It turns CMakeLists.txt into a Visual Studio
rem  solution in build\vs\ (git ignores build\) and opens it in the newest
rem  Visual Studio on this computer (2026, 2022, ...).
rem
rem  Why not commit a .sln? A generated solution contains absolute paths of the
rem  computer that made it, so it would not open on a teammate's laptop.
rem  Re-run this file after pulling new .cpp files, or just build: CMake
rem  refreshes the solution itself when CMakeLists.txt or the file list changes.
rem
rem  Needs Visual Studio with the "Desktop development with C++" workload
rem  (it includes CMake). Owner: FF.
rem ===========================================================================
setlocal
cd /d "%~dp0"

rem 1. Find cmake: on PATH, or the copy that ships with Visual Studio.
set "CMAKE_EXE="
where cmake >nul 2>nul && set "CMAKE_EXE=cmake"
if defined CMAKE_EXE goto have_cmake

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto no_vs
set "VSDIR="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -prerelease -products * -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR goto no_vs
set "CMAKE_EXE=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%CMAKE_EXE%" goto no_cmake

:have_cmake
echo Using CMake: %CMAKE_EXE%

rem 2. Generate the solution (CMake picks the newest Visual Studio installed).
"%CMAKE_EXE%" -S . -B build\vs -A x64
if errorlevel 1 goto failed

rem 3. Open it (.slnx is the newer solution format used by Visual Studio 2026).
if exist "build\vs\SeamCarving.slnx" (
    start "" "build\vs\SeamCarving.slnx"
    goto done
)
if exist "build\vs\SeamCarving.sln" (
    start "" "build\vs\SeamCarving.sln"
    goto done
)
echo Could not find build\vs\SeamCarving.sln or .slnx - see the messages above.
goto failed

:no_vs
echo Visual Studio was not found. Install Visual Studio 2022 or 2026 with the
echo "Desktop development with C++" workload, then run this file again.
goto failed

:no_cmake
echo Visual Studio was found, but not its CMake component. In the Visual Studio
echo Installer, add "C++ CMake tools for Windows" (part of "Desktop development
echo with C++"), then run this file again.
goto failed

:failed
echo.
pause
exit /b 1

:done
echo Opened. In Visual Studio: Build ^> Build Solution (Ctrl+Shift+B), then F5 runs the demo.
exit /b 0
