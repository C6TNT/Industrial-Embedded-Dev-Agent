@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

set "BIN_FILE=%SCRIPT_DIR%\ddr_release\rpmsg_lite_str_echo_rtos.bin"
if not defined TARGET_HOST set "TARGET_HOST=192.168.3.33"
if not defined TARGET_PORT set "TARGET_PORT=22"
if not defined TARGET_USER set "TARGET_USER=root"
if not defined TARGET_PATH set "TARGET_PATH=/run/media/boot-mmcblk1p1/rpmsg_lite_str_echo_rtos.bin"
set "SCP_EXE=%SystemRoot%\System32\OpenSSH\scp.exe"
set "SSH_OPTS=-o StrictHostKeyChecking=no -o UserKnownHostsFile=NUL -o KexAlgorithms=curve25519-sha256,diffie-hellman-group14-sha256,diffie-hellman-group14-sha1"

if not exist "%BIN_FILE%" (
  echo Missing build artifact: "%BIN_FILE%"
  exit /b 1
)

if not exist "%SCP_EXE%" (
  echo scp.exe not found: "%SCP_EXE%"
  exit /b 1
)

echo Deploying "%BIN_FILE%"
echo Target: %TARGET_USER%@%TARGET_HOST%:%TARGET_PATH%
"%SCP_EXE%" %SSH_OPTS% -P %TARGET_PORT% "%BIN_FILE%" %TARGET_USER%@%TARGET_HOST%:%TARGET_PATH%
exit /b %ERRORLEVEL%
