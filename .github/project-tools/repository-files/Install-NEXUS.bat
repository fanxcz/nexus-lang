@echo off
setlocal
title NEXUS Installer

if not exist "%~dp0bin\nexus-core.exe" goto missing
if not exist "%~dp0toolchain\zig.exe" goto missing
if not exist "%~dp0Install-NEXUS-Path.ps1" goto missing

echo.
echo [NEXUS] Checking the bundled compiler and linker...
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0NEXUS-Windows.ps1" -Mode Check
if errorlevel 1 goto failed

echo.
echo [NEXUS] Adding this folder to your user PATH...
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-NEXUS-Path.ps1"
if errorlevel 1 goto failed

echo.
echo Installation completed.
echo Close this window, open a NEW terminal, then run:
echo   nexus --version
echo   nexus doctor
echo   nexus "C:\path\to\hello.nx"
echo.
echo No separate LLVM, Clang, Visual Studio Build Tools, CMake or SDK install is needed.
echo Keep this entire NEXUS folder in its current location.
echo.
if /I "%NEXUS_INSTALLER_NO_PAUSE%"=="1" goto noPauseSuccess
pause
:noPauseSuccess
exit /b 0

:missing
echo [NEXUS ERROR] This is not a complete NEXUS portable folder.
echo Extract the entire NEXUS_Windows_Portable.zip first.
goto failed

:failed
echo.
echo [NEXUS ERROR] Installation did not finish. Read the error above.
if /I "%NEXUS_INSTALLER_NO_PAUSE%"=="1" goto noPauseFailure
pause
:noPauseFailure
exit /b 1
