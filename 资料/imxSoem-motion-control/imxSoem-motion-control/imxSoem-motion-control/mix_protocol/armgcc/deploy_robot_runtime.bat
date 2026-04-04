@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%i in ("%SCRIPT_DIR%\..\..") do set "PROJECT_ROOT=%%~fi"
set "ROBOT_MAIN=%PROJECT_ROOT%\机器人\机器人\motion_control_lib\main.py"
set "ROBOT_LIB=%PROJECT_ROOT%\机器人\机器人\build\librobot.so.1.0.0"

if not defined TARGET_HOST set "TARGET_HOST=192.168.3.33"
if not defined TARGET_PORT set "TARGET_PORT=22"
if not defined TARGET_USER set "TARGET_USER=root"

set "SCP_EXE=%SystemRoot%\System32\OpenSSH\scp.exe"
set "SSH_EXE=%SystemRoot%\System32\OpenSSH\ssh.exe"
set "SSH_OPTS=-o StrictHostKeyChecking=no -o UserKnownHostsFile=NUL -o KexAlgorithms=curve25519-sha256,diffie-hellman-group14-sha256,diffie-hellman-group14-sha1"

if not exist "%SCP_EXE%" (
  echo scp.exe not found: "%SCP_EXE%"
  exit /b 1
)

if not exist "%SSH_EXE%" (
  echo ssh.exe not found: "%SSH_EXE%"
  exit /b 1
)

if not exist "%ROBOT_MAIN%" (
  echo Robot main.py not found under "%PROJECT_ROOT%"
  exit /b 1
)

if not exist "%ROBOT_LIB%" (
  echo Robot librobot.so.1.0.0 not found under "%PROJECT_ROOT%"
  exit /b 1
)

echo Deploying robot runtime files
echo   main.py  : "%ROBOT_MAIN%"
echo   librobot : "%ROBOT_LIB%"

"%SCP_EXE%" %SSH_OPTS% -P %TARGET_PORT% "%ROBOT_MAIN%" %TARGET_USER%@%TARGET_HOST%:/home/main.py
if errorlevel 1 exit /b %ERRORLEVEL%

"%SCP_EXE%" %SSH_OPTS% -P %TARGET_PORT% "%ROBOT_LIB%" %TARGET_USER%@%TARGET_HOST%:/home/librobot.so.1.0.0
if errorlevel 1 exit /b %ERRORLEVEL%

"%SCP_EXE%" %SSH_OPTS% -P %TARGET_PORT% "%SCRIPT_DIR%\run_robot_remote.sh" %TARGET_USER%@%TARGET_HOST%:/home/run_robot_remote.sh
if errorlevel 1 exit /b %ERRORLEVEL%

"%SCP_EXE%" %SSH_OPTS% -P %TARGET_PORT% "%SCRIPT_DIR%\robot-runner.service" %TARGET_USER%@%TARGET_HOST%:/etc/systemd/system/robot-runner.service
if errorlevel 1 exit /b %ERRORLEVEL%

"%SSH_EXE%" %SSH_OPTS% -p %TARGET_PORT% %TARGET_USER%@%TARGET_HOST% "set -e; chmod 755 /home/run_robot_remote.sh /home/librobot.so.1.0.0; readelf -Ws /home/librobot.so.1.0.0 | grep Weld_Direct_SetTargetVel >/dev/null; systemctl daemon-reload; systemctl enable robot-runner.service; systemctl restart robot-runner.service"
exit /b %ERRORLEVEL%
