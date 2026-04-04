param(
    [string]$TargetHost = "192.168.3.33",
    [int]$TargetPort = 22,
    [string]$TargetUser = "root",
    [int]$RebootWaitSeconds = 25,
    [int]$SshWaitTimeoutSeconds = 180,
    [switch]$SkipReboot,
    [switch]$SkipBuildDeploy
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDeployBat = Join-Path $scriptDir "build_and_deploy_ddr_release.bat"
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptDir "..\.."))
$runRobotSh = Join-Path $scriptDir "run_robot_remote.sh"
$robotService = Join-Path $scriptDir "robot-runner.service"
$verifyScript = Join-Path $scriptDir "verify_robot_motion.py"
$sshExe = Join-Path $env:SystemRoot "System32\OpenSSH\ssh.exe"
$scpExe = Join-Path $env:SystemRoot "System32\OpenSSH\scp.exe"
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

function Wait-ForSsh {
    param(
        [string]$SshExe,
        [string[]]$SshOpts,
        [int]$Port,
        [string]$User,
        [string]$TargetHostName,
        [int]$TimeoutSeconds
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        try {
            & $SshExe @SshOpts -p $Port "$User@$TargetHostName" "true" 1>$null 2>$null
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
        [string]$TargetHostName,
        [string]$ProjectRoot,
        [string]$RunRobotSh,
        [string]$RobotService
    )

    $robotMain = Get-ChildItem -LiteralPath $ProjectRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -eq "main.py" -and $_.DirectoryName -match "motion_control_lib$" } |
        Select-Object -First 1 -ExpandProperty FullName
    $robotLib = Get-ChildItem -LiteralPath $ProjectRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -eq "librobot.so.1.0.0" -and $_.DirectoryName -match "\\build$" } |
        Select-Object -First 1 -ExpandProperty FullName

    if (-not $robotMain) { throw "Robot main.py not found under $ProjectRoot" }
    if (-not $robotLib) { throw "Robot librobot.so.1.0.0 not found under $ProjectRoot" }

    Write-Host "Deploying runtime files"
    Write-Host "  main.py  : $robotMain"
    Write-Host "  librobot : $robotLib"

    & $ScpExe @SshOpts -P $Port $robotMain "${User}@${TargetHostName}:/home/main.py"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    & $ScpExe @SshOpts -P $Port $robotLib "${User}@${TargetHostName}:/home/librobot.so.1.0.0"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    & $ScpExe @SshOpts -P $Port $RunRobotSh "${User}@${TargetHostName}:/home/run_robot_remote.sh"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    & $ScpExe @SshOpts -P $Port $RobotService "${User}@${TargetHostName}:/etc/systemd/system/robot-runner.service"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    $remoteCommand = "set -e; chmod 755 /home/run_robot_remote.sh /home/librobot.so.1.0.0; readelf -Ws /home/librobot.so.1.0.0 | grep Weld_Direct_SetTargetVel >/dev/null; rm -f /home/robot_runtime.log; systemctl daemon-reload; systemctl enable robot-runner.service >/dev/null 2>&1 || true; systemctl restart robot-runner.service"
    & $SshExe @SshOpts -p $Port "${User}@${TargetHostName}" $remoteCommand
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (-not (Test-Path $buildDeployBat)) { throw "Missing $buildDeployBat" }
if (-not (Test-Path $runRobotSh)) { throw "Missing $runRobotSh" }
if (-not (Test-Path $robotService)) { throw "Missing $robotService" }
if (-not (Test-Path $verifyScript)) { throw "Missing $verifyScript" }
if (-not (Test-Path $sshExe)) { throw "Missing $sshExe" }
if (-not (Test-Path $scpExe)) { throw "Missing $scpExe" }

Write-Host "Build + deploy + reboot + SSH verify"
Write-Host "Target: $TargetUser@$TargetHost`:$TargetPort"

if (-not $SkipBuildDeploy) {
    & $buildDeployBat
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Deploy-RobotRuntime -ScpExe $scpExe -SshExe $sshExe -SshOpts $sshOpts -Port $TargetPort -User $TargetUser -TargetHostName $TargetHost -ProjectRoot $projectRoot -RunRobotSh $runRobotSh -RobotService $robotService
}
else {
    Write-Host "Skipping build/deploy"
}

if (-not $SkipReboot) {
    Write-Host "Rebooting target..."
    & $sshExe @sshOpts -p $TargetPort "$TargetUser@$TargetHost" "sh -c 'sync; (sleep 1; reboot) >/dev/null 2>&1 &'"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "Waiting $RebootWaitSeconds second(s) before SSH probe..."
    Start-Sleep -Seconds $RebootWaitSeconds

    if (-not (Wait-ForSsh -SshExe $sshExe -SshOpts $sshOpts -Port $TargetPort -User $TargetUser -TargetHostName $TargetHost -TimeoutSeconds $SshWaitTimeoutSeconds)) {
        throw "Target SSH did not recover within $SshWaitTimeoutSeconds seconds"
    }
}
else {
    Write-Host "Skipping reboot and reusing current target runtime."
}

Write-Host "Uploading verify_robot_motion.py ..."
& $scpExe @sshOpts -P $TargetPort $verifyScript "${TargetUser}@${TargetHost}:/tmp/verify_robot_motion.py"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Running remote verification..."
& $sshExe @sshOpts -p $TargetPort "$TargetUser@$TargetHost" "python3 /tmp/verify_robot_motion.py"
exit $LASTEXITCODE
