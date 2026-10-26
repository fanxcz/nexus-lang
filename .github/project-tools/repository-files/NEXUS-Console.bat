@echo off
setlocal
set "NEXUS_ROOT=%~dp0"
set "PATH=%NEXUS_ROOT%;%NEXUS_ROOT%bin;%NEXUS_ROOT%runtime;%NEXUS_ROOT%toolchain;%PATH%"
cd /d "%NEXUS_ROOT%"
title NEXUS 2.1.0 Console
echo ============================================================
echo   NEXUS 2.1.0 - native programming language
echo ============================================================
echo.
echo Quick start:
echo   nexus --version
echo   nexus doctor
echo   nexus "%NEXUS_ROOT%MyPrograms\hello.nx"
echo.
call "%NEXUS_ROOT%nexus.cmd" --help
echo.
echo This terminal stays open. Type NEXUS commands or EXIT to close.
cmd /k
