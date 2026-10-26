@echo off
setlocal
set NEXUS=%LOCALAPPDATA%\Programs\NEXUS
set PATH=%NEXUS%\runtime;%PATH%
"%NEXUS%\bin\nexus.exe" build "%~dp0poker_gui.nx" -o "%~dp0poker_gui.exe" --backend llvm
if errorlevel 1 exit /b 1
"%~dp0poker_gui.exe"