@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0NEXUS-Windows.ps1" -Mode Build -SourcePath "%~1"
set "NEXUS_RC=%ERRORLEVEL%"
echo.
if not "%NEXUS_RC%"=="0" echo NEXUS exited with code %NEXUS_RC%.
pause
exit /b %NEXUS_RC%
