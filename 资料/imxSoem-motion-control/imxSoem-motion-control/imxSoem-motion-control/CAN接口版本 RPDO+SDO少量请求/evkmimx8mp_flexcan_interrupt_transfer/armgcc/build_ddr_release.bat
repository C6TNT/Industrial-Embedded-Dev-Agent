@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

for /f "delims=" %%i in ('wsl.exe wslpath "%SCRIPT_DIR%"') do set "SCRIPT_DIR_WSL=%%i"

if not defined SCRIPT_DIR_WSL (
  echo Failed to convert build script path for WSL.
  exit /b 1
)

wsl.exe bash "%SCRIPT_DIR_WSL%/build_ddr_release.sh"
exit /b %ERRORLEVEL%
