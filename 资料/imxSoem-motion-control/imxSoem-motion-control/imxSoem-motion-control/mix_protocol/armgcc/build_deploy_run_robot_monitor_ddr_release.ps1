param(
    [string]$PortName = "COM3",
    [int]$BaudRate = 115200,
    [int]$CaptureSeconds = 20,
    [int]$StartWindowSeconds = 50,
    [int]$PostDeployDelaySeconds = 2,
    [int]$PostRebootDelaySeconds = 8,
    [int]$PostStartDelaySeconds = 2,
    [int]$SshWaitTimeoutSeconds = 15,
    [int]$StartRetryCount = 10,
    [switch]$SkipBuildDeploy
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDeployBat = Join-Path $scriptDir "build_and_deploy_ddr_release.bat"
$runRobotSh = Join-Path $scriptDir "run_robot_remote.sh"
$robotService = Join-Path $scriptDir "robot-runner.service"
$sshExe = Join-Path $env:SystemRoot "System32\OpenSSH\ssh.exe"
$scpExe = Join-Path $env:SystemRoot "System32\OpenSSH\scp.exe"
$logDir = Join-Path $scriptDir "serial_logs"
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logFile = Join-Path $logDir ("robot_run_" + $timestamp + ".log")
$targetHost = if ($env:TARGET_HOST) { $env:TARGET_HOST } else { "192.168.3.33" }
$targetPort = if ($env:TARGET_PORT) { [int]$env:TARGET_PORT } else { 22 }
$targetUser = if ($env:TARGET_USER) { $env:TARGET_USER } else { "root" }
$sshOpts = @(
    "-o", "StrictHostKeyChecking=no",
    "-o", "UserKnownHostsFile=NUL",
    "-o", "BatchMode=yes",
    "-o", "ConnectTimeout=3",
    "-o", "ConnectionAttempts=1",
    "-o", "ServerAliveInterval=2",
    "-o", "ServerAliveCountMax=1",
    "-o", "KexAlgorithms=curve25519-sha256,diffie-hellman-group14-sha256,diffie-hellman-group14-sha1"
)

if (-not (Test-Path $buildDeployBat)) {
    throw "Missing build/deploy script: $buildDeployBat"
}

if (-not (Test-Path $sshExe)) {
    throw "Missing ssh.exe: $sshExe"
}

if (-not (Test-Path $scpExe)) {
    throw "Missing scp.exe: $scpExe"
}

if (-not (Test-Path $runRobotSh)) {
    throw "Missing run script: $runRobotSh"
}

if (-not (Test-Path $robotService)) {
    throw "Missing service file: $robotService"
}

New-Item -ItemType Directory -Path $logDir -Force | Out-Null

function Wait-ForSsh {
    param(
        [string]$SshExe,
        [string[]]$SshOpts,
        [int]$Port,
        [string]$User,
        [string]$TargetHost,
        [int]$TimeoutSeconds
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        try {
            & $SshExe @SshOpts -p $Port "$User@$TargetHost" "true" 1>$null 2>$null
            if ($LASTEXITCODE -eq 0) {
                return $true
            }
        }
        catch {
        }
        Start-Sleep -Seconds 2
    }
    return $false
}

function Deploy-RobotRuntime {
    param(
        [string]$ScpExe,
        [string]$SshExe,
        [string[]]$SshOpts,
        [int]$Port,
        [string]$User,
        [string]$TargetHost,
        [string]$ProjectRoot,
        [string]$RunRobotSh,
        [string]$RobotService
    )

    $robotMain = Join-Path $ProjectRoot "机器人\机器人\motion_control_lib\main.py"
    $robotLib = Join-Path $ProjectRoot "机器人\机器人\build\librobot.so.1.0.0"

    if (-not (Test-Path $robotMain)) {
        $robotMain = Get-ChildItem -LiteralPath $ProjectRoot -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -eq "main.py" -and $_.DirectoryName -match "motion_control_lib$" } |
            Select-Object -First 1 -ExpandProperty FullName
    }
    if (-not (Test-Path $robotLib)) {
        $robotLib = Get-ChildItem -LiteralPath $ProjectRoot -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -eq "librobot.so.1.0.0" -and $_.DirectoryName -match "\\build$" } |
            Select-Object -First 1 -ExpandProperty FullName
    }

    if (-not $robotMain -or -not (Test-Path $robotMain)) {
        throw "Robot main.py not found: $robotMain"
    }
    if (-not $robotLib -or -not (Test-Path $robotLib)) {
        throw "Robot librobot.so.1.0.0 not found: $robotLib"
    }

    Write-Host "Deploying robot runtime files"
    Write-Host "  main.py  : $robotMain"
    Write-Host "  librobot : $robotLib"

    & $ScpExe @SshOpts -P $Port $robotMain "${User}@${TargetHost}:/home/main.py"
    if ($LASTEXITCODE -ne 0) { throw "Failed to upload main.py" }
    & $ScpExe @SshOpts -P $Port $robotLib "${User}@${TargetHost}:/home/librobot.so.1.0.0"
    if ($LASTEXITCODE -ne 0) { throw "Failed to upload librobot.so.1.0.0" }
    & $ScpExe @SshOpts -P $Port $RunRobotSh "${User}@${TargetHost}:/home/run_robot_remote.sh"
    if ($LASTEXITCODE -ne 0) { throw "Failed to upload run_robot_remote.sh" }
    & $ScpExe @SshOpts -P $Port $RobotService "${User}@${TargetHost}:/etc/systemd/system/robot-runner.service"
    if ($LASTEXITCODE -ne 0) { throw "Failed to upload robot-runner.service" }

    $remoteCommand = "set -e; chmod 755 /home/run_robot_remote.sh /home/librobot.so.1.0.0; readelf -Ws /home/librobot.so.1.0.0 | grep Weld_Direct_SetTargetVel >/dev/null; rm -f /home/robot_runtime.log; systemctl daemon-reload; systemctl enable robot-runner.service >/dev/null 2>&1 || true; systemctl restart robot-runner.service"
    & $SshExe @SshOpts -p $Port "${User}@${TargetHost}" $remoteCommand
    if ($LASTEXITCODE -ne 0) {
        throw "Remote robot runtime verification failed"
    }
}

function Wait-ForPing {
    param(
        [string]$TargetHost,
        [int]$TimeoutSeconds
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if (Test-Connection -ComputerName $TargetHost -Count 1 -Quiet -ErrorAction SilentlyContinue) {
            return $true
        }
        Start-Sleep -Seconds 2
    }
    return $false
}

function Start-RemoteRobot {
    param(
        [string]$SshExe,
        [string[]]$SshOpts,
        [int]$Port,
        [string]$User,
        [string]$TargetHost,
        [int]$RetryCount
    )

    $remoteCommand = @"
if systemctl is-active --quiet robot-runner.service; then
    exit 0
fi
if pgrep -f '/home/run_robot_remote.sh' >/dev/null 2>&1; then
    exit 0
fi
if pgrep -f 'python3 -u /home/main.py' >/dev/null 2>&1; then
    exit 0
fi
if pgrep -f 'python3 /home/main.py' >/dev/null 2>&1; then
    exit 0
fi
nohup /bin/sh /home/run_robot_remote.sh >/dev/null 2>&1 &
"@
    & $SshExe @SshOpts -p $Port "$User@$TargetHost" $remoteCommand
    return ($LASTEXITCODE -eq 0)
}

function Test-RemoteRobotRuntime {
    param(
        [string]$SshExe,
        [string[]]$SshOpts,
        [int]$Port,
        [string]$User,
        [string]$TargetHost
    )

    try {
        $tail = & $SshExe @SshOpts -p $Port "$User@$TargetHost" "test -f /home/robot_runtime.log && tail -n 40 /home/robot_runtime.log" 2>$null
        if ($LASTEXITCODE -ne 0) {
            return $false
        }
        return (Test-SerialRobotActive -Text ($tail | Out-String))
    }
    catch {
        return $false
    }
}

function Test-SerialBootReady {
    param(
        [string]$Text
    )

    return (
        $Text.Contains("Nameservice sent, ready for incoming messages") -or
        $Text.Contains("CAN loop task entered") -or
        $Text.Contains("Runtime feedback:")
    )
}

function Test-SerialRobotActive {
    param(
        [string]$Text
    )

    return (
        $Text.Contains("[robot] robot launcher started") -or
        $Text.Contains("[robot-main] ") -or
        $Text.Contains("SMG running: main.py motion command received") -or
        ($Text -match "Runtime feedback:\s+status=0x[0-9A-Fa-f]+\s+actualPosition=") -or
        ($Text -match "ROT write f_target_vel=(?!0(?:\.0+)?)") -or
        ($Text -match "ROT service input: f_target_vel=(?!0(?:\.0+)?)")
    )
}

Write-Host "WSL build + SSH deploy + robot runtime + reboot + serial monitor"
Write-Host "Target: $targetUser@$targetHost`:$targetPort"
Write-Host "Serial: $PortName @ $BaudRate"

if (-not $SkipBuildDeploy) {
    $projectRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptDir "..\.."))
    Write-Host "Building firmware and deploying .bin ..."
    & $buildDeployBat
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    Write-Host "Deploying /home/main.py and /home/librobot.so.1.0.0 ..."
    Deploy-RobotRuntime -ScpExe $scpExe -SshExe $sshExe -SshOpts $sshOpts -Port $targetPort -User $targetUser -TargetHost $targetHost -ProjectRoot $projectRoot -RunRobotSh $runRobotSh -RobotService $robotService
}
else {
    Write-Host "Skipping build/deploy and reusing existing firmware/runtime files."
}

Write-Host "Waiting $PostDeployDelaySeconds second(s) before reboot..."
Start-Sleep -Seconds $PostDeployDelaySeconds

Write-Host "Rebooting target $targetUser@$targetHost ..."
& $sshExe @sshOpts -p $targetPort "$targetUser@$targetHost" "sh -c 'sync; (sleep 1; reboot) >/dev/null 2>&1 &'"
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Waiting $PostRebootDelaySeconds second(s) for target reboot..."
Start-Sleep -Seconds $PostRebootDelaySeconds

Write-Host "Skipping ping/SSH readiness gating and moving directly to serial capture plus start retries..."

$port = New-Object System.IO.Ports.SerialPort $PortName, $BaudRate, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
$port.ReadTimeout = 500
$port.WriteTimeout = 500
$port.NewLine = "`n"

try {
    $port.Open()
    $port.DiscardInBuffer()
    $port.DiscardOutBuffer()

    Write-Host "Capturing serial output from $PortName for $CaptureSeconds second(s)..."
    Write-Host "Serial log: $logFile"

    $writer = [System.IO.StreamWriter]::new($logFile, $false, [System.Text.Encoding]::UTF8)
    try {
        $robotStarted = $false
        $robotStartTimestamp = $null
        $lastStartAttempt = [datetime]::MinValue
        $bootReady = $false
        $serialBuffer = ""
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        while ($true) {
            if ((-not $robotStarted) -and ($sw.Elapsed.TotalSeconds -ge $StartWindowSeconds)) {
                break
            }

            if ($robotStarted -and (((Get-Date) - $robotStartTimestamp).TotalSeconds -ge $CaptureSeconds)) {
                break
            }

            if ((-not $robotStarted) -and $bootReady -and (((Get-Date) - $lastStartAttempt).TotalSeconds -ge 1)) {
                $lastStartAttempt = Get-Date
                Write-Host "Starting /home/main.py on target ..."
                $launchAccepted = Start-RemoteRobot -SshExe $sshExe -SshOpts $sshOpts -Port $targetPort -User $targetUser -TargetHost $targetHost -RetryCount $StartRetryCount
                if ($launchAccepted) {
                    Write-Host "Remote robot launch command accepted."
                    Start-Sleep -Seconds $PostStartDelaySeconds
                }
            }

            if (-not $robotStarted) {
                $robotStarted = Test-RemoteRobotRuntime -SshExe $sshExe -SshOpts $sshOpts -Port $targetPort -User $targetUser -TargetHost $targetHost
                if ($robotStarted) {
                    $robotStartTimestamp = Get-Date
                    Write-Host "Remote robot runtime confirmed via /home/robot_runtime.log."
                }
            }

            $chunk = $port.ReadExisting()
            if (-not [string]::IsNullOrEmpty($chunk)) {
                Write-Host -NoNewline $chunk
                $writer.Write($chunk)
                $writer.Flush()
                $serialBuffer += $chunk
                if ($serialBuffer.Length -gt 8192) {
                    $serialBuffer = $serialBuffer.Substring($serialBuffer.Length - 8192)
                }
                if ((-not $bootReady) -and (Test-SerialBootReady -Text $serialBuffer)) {
                    $bootReady = $true
                    Write-Host ""
                    Write-Host "Serial boot milestone detected. Switching to fast SSH start retries."
                }
                if ((-not $robotStarted) -and (Test-SerialRobotActive -Text $serialBuffer)) {
                    $robotStarted = $true
                    $robotStartTimestamp = Get-Date
                    Write-Host ""
                    Write-Host "Robot runtime already active on serial output."
                }
            }
            Start-Sleep -Milliseconds 100
        }
    }
    finally {
        $writer.Dispose()
    }

    if (-not $robotStarted) {
        $finalSerial = Get-Content $logFile -Raw
        if (Test-SerialRobotActive -Text $finalSerial) {
            $robotStarted = $true
        }
    }

    if (-not $robotStarted) {
        throw "Failed to start /home/run_robot_remote.sh within $StartWindowSeconds second(s)."
    }
}
finally {
    if ($port.IsOpen) {
        $port.Close()
    }
}

Write-Host ""
Write-Host "Done. Serial log saved to: $logFile"
