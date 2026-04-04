param(
    [string]$FlagName = ".ieda_wsl_stub_enabled"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$runtimeStub = Join-Path $repoRoot "scripts\wsl\run_with_stub.py"
$stubModule = Join-Path $repoRoot "scripts\wsl\librobot_stub_runtime.py"
if (-not (Test-Path $runtimeStub)) {
    throw "Missing runtime wrapper: $runtimeStub"
}
if (-not (Test-Path $stubModule)) {
    throw "Missing stub runtime module: $stubModule"
}

$flagPath = Join-Path $repoRoot $FlagName
Set-Content -Path $flagPath -Value "enabled $(Get-Date -Format s)" -NoNewline
Write-Output "Created stub mode flag at $flagPath"
