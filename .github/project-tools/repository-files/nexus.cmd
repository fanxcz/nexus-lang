@echo off
setlocal
rem Expose bundled backend, toolchain and runtime DLLs for every NEXUS subcommand.
rem Install-NEXUS-Path.ps1 adds this folder (not its bin subfolder) to the user PATH.
set "PATH=%~dp0bin;%~dp0runtime;%~dp0toolchain;%PATH%"
if "%~1"=="" goto help
if /I "%~x1"==".nx" goto runfile
if /I "%~1"=="run" goto runcommand
if /I "%~1"=="build" goto buildcommand
"%~dp0bin\nexus-core.exe" %*
exit /b %ERRORLEVEL%

:runfile
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0NEXUS-Windows.ps1" -Mode Run -PauseOnExit -SourcePath "%~1"
exit /b %ERRORLEVEL%

:runcommand
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0NEXUS-Windows.ps1" -Mode Run -PauseOnExit -SourcePath "%~2"
exit /b %ERRORLEVEL%

:buildcommand
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0NEXUS-Windows.ps1" -Mode Build -SourcePath "%~2"
exit /b %ERRORLEVEL%

:help
echo Usage:
echo   nexus ^<file.nx^>       Compile and run a source file
echo   nexus run ^<file.nx^>   Compile and run a source file
echo   nexus build ^<file.nx^> Compile to an .exe beside the source
echo   nexus doctor            Check the bundled native toolchain
echo   nexus --version         Show compiler version
"%~dp0bin\nexus-core.exe" --help
exit /b %ERRORLEVEL%
