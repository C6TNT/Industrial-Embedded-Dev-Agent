param(
    [string]$Source = "准备产物\\benchmark_v1.jsonl",
    [string]$DestinationDir = "data\\benchmark"
)

$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Force -Path $DestinationDir | Out-Null

$mapping = @{
    knowledge_qa = "knowledge_qa_v1.jsonl"
    log_attribution = "log_attribution_v1.jsonl"
    tool_safety = "tool_safety_v1.jsonl"
}

$lines = Get-Content -Path $Source -Encoding UTF8
$buckets = @{
    knowledge_qa = [System.Collections.Generic.List[string]]::new()
    log_attribution = [System.Collections.Generic.List[string]]::new()
    tool_safety = [System.Collections.Generic.List[string]]::new()
}

foreach ($line in $lines) {
    if ([string]::IsNullOrWhiteSpace($line)) {
        continue
    }
    $obj = $line | ConvertFrom-Json
    $buckets[$obj.item_type].Add($line)
}

foreach ($key in $mapping.Keys) {
    $target = Join-Path $DestinationDir $mapping[$key]
    Set-Content -Path $target -Value $buckets[$key] -Encoding UTF8
}

Write-Host "Benchmark split complete."
