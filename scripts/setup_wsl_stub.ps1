param(
    [string]$FlagName = ".ieda_wsl_stub_enabled",
    [string]$Scenario = "nominal"
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

$profilePath = Join-Path $repoRoot ".ieda_stub_profile.json"
$profile = @{
    scenario = $Scenario
} | ConvertTo-Json -Depth 2
Set-Content -Path $profilePath -Value $profile
Write-Output "Wrote stub profile at $profilePath with scenario=$Scenario"
