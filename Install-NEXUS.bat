@echo off
setlocal
title NEXUS 2.1.1 Installer

where powershell.exe >nul 2>&1
if errorlevel 1 (
  echo [NEXUS] Windows PowerShell was not found.
  echo Enable Windows PowerShell and try again.
  pause
  exit /b 1
)

if not exist "%~dp0Install-NEXUS.ps1" (
  echo [NEXUS] Install-NEXUS.ps1 was not found beside this BAT file.
  echo Keep both root-level installer scripts in the same folder.
  pause
  exit /b 1
)

echo.
echo NEXUS Setup 2.1.1
echo This script downloads the official installer, verifies SHA-256,
echo then opens the normal per-user setup wizard.
echo No administrator rights are required for the default install.
echo.

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-NEXUS.ps1"
set "NEXUS_INSTALL_RC=%ERRORLEVEL%"
if not "%NEXUS_INSTALL_RC%"=="0" (
  echo.
  echo [NEXUS] Installation did not complete. Read the error above.
  pause
)
exit /b %NEXUS_INSTALL_RC%
