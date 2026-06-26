param(
    [switch]$InstallSystemTools,
    [string]$VcpkgRoot = ""
)

$ErrorActionPreference = "Stop"

function Test-Command {
    param([Parameter(Mandatory = $true)][string]$Name)
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Install-WithWinget {
    param(
        [Parameter(Mandatory = $true)][string]$Id,
        [string]$Override = ""
    )

    if (-not (Test-Command "winget")) {
        Write-Warning "winget bulunamadi. $Id otomatik kurulamadi."
        return
    }

    $args = @(
        "install", "--id", $Id, "--exact", "--source", "winget",
        "--accept-package-agreements", "--accept-source-agreements"
    )

    if ($Override -ne "") {
        $args += @("--override", $Override)
    }

    Write-Host "Kuruluyor: $Id"
    winget @args
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ($VcpkgRoot -eq "") {
    $VcpkgRoot = Join-Path $repoRoot ".deps\vcpkg"
}

if ($InstallSystemTools) {
    if (-not (Test-Command "git")) {
        Install-WithWinget -Id "Git.Git"
    }

    if (-not (Test-Command "cmake")) {
        Install-WithWinget -Id "Kitware.CMake"
    }

    $vsOverride = "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
    Install-WithWinget -Id "Microsoft.VisualStudio.2022.BuildTools" -Override $vsOverride
}

if (-not (Test-Command "git")) {
    throw "Git bulunamadi. Git kurup tekrar calistirin."
}

if (-not (Test-Command "cmake")) {
    throw "CMake bulunamadi. CMake kurup tekrar calistirin."
}

if (-not (Test-Path $VcpkgRoot)) {
    New-Item -ItemType Directory -Force -Path (Split-Path $VcpkgRoot) | Out-Null
    git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
}

$bootstrap = Join-Path $VcpkgRoot "bootstrap-vcpkg.bat"
$vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"

if (-not (Test-Path $vcpkgExe)) {
    & $bootstrap
}

Push-Location $repoRoot
try {
    & $vcpkgExe install --triplet x64-windows
}
finally {
    Pop-Location
}

Write-Host ""
Write-Host "Kurulum tamamlandi."
Write-Host "Derlemek icin: .\scripts\build_windows.ps1"
