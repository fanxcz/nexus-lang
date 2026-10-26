@echo off
setlocal
echo NEXUS Poker 2.1.0 - Texas Hold'em
set NEXUS=%LOCALAPPDATA%\Programs\NEXUS
if not exist "%NEXUS%\bin\nexus.exe" (
  echo NEXUS is not installed in %NEXUS%.
  echo Install it first or edit the NEXUS path in this script.
  exit /b 1
)
set PATH=%NEXUS%\runtime;%PATH%
pushd "%~dp0"
"%NEXUS%\bin\nexus.exe" build "poker_gui.nx" -o "poker_gui.exe" --backend llvm
if errorlevel 1 (
  popd
  exit /b 1
)
"poker_gui.exe"
popd
