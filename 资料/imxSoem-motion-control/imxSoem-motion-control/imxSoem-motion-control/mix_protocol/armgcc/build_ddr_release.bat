@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

if not defined SDK_ZIP_WIN for /f "usebackq delims=" %%i in (`powershell.exe -NoProfile -Command "(Get-ChildItem -Path 'D:\' -Recurse -File -Filter 'SDK_2_14_0_EVK-MIMX8MP1.zip' -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName)"`) do set "SDK_ZIP_WIN=%%i"
if not defined TOOLCHAIN_ZIP_WIN for /f "usebackq delims=" %%i in (`powershell.exe -NoProfile -Command "(Get-ChildItem -Path 'D:\' -Recurse -File -Filter 'arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.zip' -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName)"`) do set "TOOLCHAIN_ZIP_WIN=%%i"

if not exist "%SDK_ZIP_WIN%" (
  echo SDK zip not found: "%SDK_ZIP_WIN%"
  exit /b 1
)

if not exist "%TOOLCHAIN_ZIP_WIN%" (
  echo Toolchain zip not found: "%TOOLCHAIN_ZIP_WIN%"
  exit /b 1
)

for /f "delims=" %%i in ('wsl.exe wslpath "%SCRIPT_DIR%"') do set "SCRIPT_DIR_WSL=%%i"
for /f "delims=" %%i in ('wsl.exe wslpath "%SDK_ZIP_WIN%"') do set "SDK_ZIP_WSL=%%i"
for /f "delims=" %%i in ('wsl.exe wslpath "%TOOLCHAIN_ZIP_WIN%"') do set "TOOLCHAIN_ZIP_WSL=%%i"

if not defined SCRIPT_DIR_WSL (
  echo Failed to convert build script path for WSL.
  exit /b 1
)

if not defined SDK_ZIP_WSL (
  echo Failed to convert SDK zip path for WSL: "%SDK_ZIP_WIN%"
  exit /b 1
)

if not defined TOOLCHAIN_ZIP_WSL (
  echo Failed to convert toolchain zip path for WSL: "%TOOLCHAIN_ZIP_WIN%"
  exit /b 1
)

echo Using SDK zip: "%SDK_ZIP_WIN%"
echo Using toolchain zip: "%TOOLCHAIN_ZIP_WIN%"
wsl.exe env SDK_ZIP_PATH="%SDK_ZIP_WSL%" TOOLCHAIN_ZIP_PATH="%TOOLCHAIN_ZIP_WSL%" bash "%SCRIPT_DIR_WSL%/build_ddr_release.sh"
exit /b %ERRORLEVEL%
