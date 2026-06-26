param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Release",
    [string]$VcpkgRoot = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ($VcpkgRoot -eq "") {
    $VcpkgRoot = Join-Path $repoRoot ".deps\vcpkg"
}

$vcpkgToolchain = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
if (-not (Test-Path $vcpkgToolchain)) {
    & (Join-Path $PSScriptRoot "setup_windows.ps1") -VcpkgRoot $VcpkgRoot
}

$buildDir = Join-Path $repoRoot "build\windows"

cmake -S $repoRoot -B $buildDir `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="$vcpkgToolchain" `
    -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build $buildDir --config $Configuration

Write-Host ""
Write-Host "Derleme tamamlandi."
Write-Host "Calistirma ornegi:"
Write-Host ".\scripts\run_windows.ps1 -Source `"rtsp://user:password@192.168.1.50:554/stream1`""
