@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

call "%SCRIPT_DIR%\build_ddr_release.bat"
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%\deploy_ddr_release.bat"
exit /b %ERRORLEVEL%
