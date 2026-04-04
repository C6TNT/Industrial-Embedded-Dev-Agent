@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

powershell.exe -ExecutionPolicy Bypass -File "%SCRIPT_DIR%\build_deploy_verify_robot_no_serial.ps1"
exit /b %ERRORLEVEL%
