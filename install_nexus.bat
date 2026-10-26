@echo off
setlocal EnableExtensions DisableDelayedExpansion
title NEXUS Online Installer

rem This bootstrap downloads the official NEXUS Setup and verifies its checksum
rem before launching it. The installer is per-user and normally needs no admin rights.
set "NEXUS_BASE_URL=https://fanxcz.github.io/nexus-lang/assets/downloads"
set "NEXUS_SETUP_NAME=NEXUS-Setup-2.1.0.exe"
set "NEXUS_WORK_DIR=%TEMP%\NEXUS-Online-Install-%RANDOM%-%RANDOM%"
set "NEXUS_SETUP_PATH=%NEXUS_WORK_DIR%\%NEXUS_SETUP_NAME%"
set "NEXUS_CHECKSUM_PATH=%NEXUS_WORK_DIR%\%NEXUS_SETUP_NAME%.sha256.txt"
set "NEXUS_SETUP_URL=%NEXUS_BASE_URL%/%NEXUS_SETUP_NAME%"
set "NEXUS_CHECKSUM_URL=%NEXUS_SETUP_URL%.sha256.txt"

echo.
echo ============================================================
echo   NEXUS - Official Windows Installer
echo ============================================================
echo.
echo Downloading the official setup and its SHA-256 checksum...
echo Source: %NEXUS_BASE_URL%
echo.

mkdir "%NEXUS_WORK_DIR%" >nul 2>&1
if errorlevel 1 goto :failed

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; $ProgressPreference='SilentlyContinue'; try { [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -UseBasicParsing -Uri $env:NEXUS_SETUP_URL -OutFile $env:NEXUS_SETUP_PATH; Invoke-WebRequest -UseBasicParsing -Uri $env:NEXUS_CHECKSUM_URL -OutFile $env:NEXUS_CHECKSUM_PATH } catch { Write-Error $_; exit 1 }"
if errorlevel 1 goto :failed

echo.
echo Verifying installer integrity...
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; try { $line = (Get-Content -LiteralPath $env:NEXUS_CHECKSUM_PATH -Raw -Encoding ASCII).Trim(); if ($line -notmatch '^([0-9a-fA-F]{64})(?:\s+(.+))?$') { throw 'The official checksum file has an invalid format.' }; $expected = $Matches[1].ToLowerInvariant(); $actual = (Get-FileHash -LiteralPath $env:NEXUS_SETUP_PATH -Algorithm SHA256).Hash.ToLowerInvariant(); if ($actual -ne $expected) { throw ('SHA-256 mismatch. Expected ' + $expected + ', got ' + $actual + '. Installer will not be run.') }; Write-Host ('SHA-256 verified: ' + $actual) } catch { Write-Error $_; exit 1 }"
if errorlevel 1 goto :failed

echo.
echo Verification passed. Launching NEXUS Setup...
echo Follow the installer steps. Administrator rights are normally not required.
echo.
start "" /wait "%NEXUS_SETUP_PATH%" %*
set "NEXUS_INSTALL_EXIT=%ERRORLEVEL%"
if not "%NEXUS_INSTALL_EXIT%"=="0" goto :installer_failed

echo.
echo NEXUS Setup finished successfully.
echo Open a NEW terminal and verify:
echo   nexus --version
echo   nexus doctor
echo.
if exist "%NEXUS_WORK_DIR%" rmdir /s /q "%NEXUS_WORK_DIR%" >nul 2>&1
if /I "%NEXUS_INSTALLER_NO_PAUSE%"=="1" goto :success_no_pause
pause
:success_no_pause
exit /b 0

:installer_failed
echo.
echo [NEXUS ERROR] The installer returned exit code %NEXUS_INSTALL_EXIT%.
echo You can start setup manually from: "%NEXUS_SETUP_PATH%"
if /I "%NEXUS_INSTALLER_NO_PAUSE%"=="1" exit /b %NEXUS_INSTALL_EXIT%
pause
exit /b %NEXUS_INSTALL_EXIT%

:failed
echo.
echo [NEXUS ERROR] Download or checksum verification failed.
echo The setup executable was NOT started.
echo Check your internet connection and try again. Official download page:
echo   https://fanxcz.github.io/nexus-lang/downloads/
if exist "%NEXUS_WORK_DIR%" rmdir /s /q "%NEXUS_WORK_DIR%" >nul 2>&1
if /I "%NEXUS_INSTALLER_NO_PAUSE%"=="1" exit /b 1
pause
exit /b 1
