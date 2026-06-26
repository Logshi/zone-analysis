param(
    [Parameter(Mandatory = $true)]
    [string]$Source,
    [string]$Model = "models\yolo26n.onnx",
    [double]$DwellSeconds = 10,
    [string]$Region = "180,200;500,200;560,560;120,560",
    [string]$AlertsDir = "alerts",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$modelPath = (Resolve-Path (Join-Path $repoRoot $Model)).Path
$alertsPath = Join-Path $repoRoot $AlertsDir

$exeCandidates = @(
    (Join-Path $repoRoot "build\windows\$Configuration\dwell_alert.exe"),
    (Join-Path $repoRoot "build\windows\dwell_alert.exe")
)

$exe = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $exe) {
    throw "dwell_alert.exe bulunamadi. Once .\scripts\build_windows.ps1 calistirin."
}

& $exe $Source $modelPath --dwell $DwellSeconds --region $Region --alerts $alertsPath
