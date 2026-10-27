@echo off
setlocal
echo NEXUS Poker - Texas Hold'em
set NEXUS=%LOCALAPPDATA%\Programs\NEXUS
if not exist "%NEXUS%\nexus.cmd" (
  echo NEXUS is not installed in %NEXUS%.
  echo Install it first or edit the NEXUS path in this script.
  exit /b 1
)
set PATH=%NEXUS%\runtime;%PATH%
pushd "%~dp0"
"%NEXUS%\nexus.cmd" build "poker_gui.nx" -o "poker_gui.exe" --backend llvm
if errorlevel 1 (
  popd
  exit /b 1
)
"poker_gui.exe"
popd
