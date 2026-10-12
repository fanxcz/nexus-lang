@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0NEXUS-Windows.ps1" -Mode Install
set "NEXUS_RC=%ERRORLEVEL%"
echo.
pause
exit /b %NEXUS_RC%
