param(
    [string]$PortName = "COM3",
    [int]$BaudRate = 115200,
    [int]$CaptureSeconds = 25,
    [int]$PostDeployDelaySeconds = 2,
    [int]$PostRebootDelaySeconds = 12
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDeployBat = Join-Path $scriptDir "build_and_deploy_ddr_release.bat"
$sshExe = Join-Path $env:SystemRoot "System32\OpenSSH\ssh.exe"
$logDir = Join-Path $scriptDir "serial_logs"
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logFile = Join-Path $logDir ("ddr_release_" + $timestamp + ".log")
$targetHost = "192.168.3.33"
$targetPort = 22
$targetUser = "root"
$sshOpts = @(
    "-o", "StrictHostKeyChecking=no",
    "-o", "UserKnownHostsFile=NUL",
    "-o", "KexAlgorithms=curve25519-sha256,diffie-hellman-group14-sha256,diffie-hellman-group14-sha1"
)

if (-not (Test-Path $buildDeployBat)) {
    throw "Missing build/deploy script: $buildDeployBat"
}

if (-not (Test-Path $sshExe)) {
    throw "Missing ssh.exe: $sshExe"
}

New-Item -ItemType Directory -Path $logDir -Force | Out-Null

Write-Host "Building and deploying ddr_release..."
& $buildDeployBat
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Waiting $PostDeployDelaySeconds second(s) before opening $PortName..."
Start-Sleep -Seconds $PostDeployDelaySeconds

Write-Host "Rebooting target $targetUser@$targetHost ..."
& $sshExe @sshOpts -p $targetPort "$targetUser@$targetHost" "sh -c 'sync; (sleep 1; reboot) >/dev/null 2>&1 &'"
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Waiting $PostRebootDelaySeconds second(s) for target reboot..."
Start-Sleep -Seconds $PostRebootDelaySeconds

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
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        while ($sw.Elapsed.TotalSeconds -lt $CaptureSeconds) {
            $chunk = $port.ReadExisting()
            if (-not [string]::IsNullOrEmpty($chunk)) {
                Write-Host -NoNewline $chunk
                $writer.Write($chunk)
                $writer.Flush()
            }
            Start-Sleep -Milliseconds 100
        }
    }
    finally {
        $writer.Dispose()
    }
}
finally {
    if ($port.IsOpen) {
        $port.Close()
    }
}

Write-Host ""
Write-Host "Done. Serial log saved to: $logFile"
