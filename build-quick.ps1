# Quick Build Script for FoxBridgeAgent
# Simple one-command build with sensible defaults

param(
    [switch]$Help
)

if ($Help) {
    Write-Host @"
FoxBridgeAgent Quick Build Script

Usage:
    .\build-quick.ps1           Build everything (exe + installers)
    .\build.ps1 -SkipBuild      Build installers only (skip exe build)
    .\build.ps1 -WixOnly        Build WiX MSI installer only
    .\build.ps1 -NsisOnly       Build NSIS EXE installer only
    .\build.ps1 -Clean          Clean and rebuild everything

Prerequisites:
    - Visual Studio 2022 with C++ tools
    - CMake 3.20+
    - vcpkg (set VCPKG_ROOT environment variable)
    - WiX Toolset v3.11 (for MSI installer)
    - NSIS 3.x (for EXE installer)

Examples:
    .\build-quick.ps1                    # Build everything
    .\build.ps1 -BuildInstaller          # Build exe + installers
    .\build.ps1 -Clean -BuildInstaller   # Clean rebuild with installers
"@
    exit 0
}

Write-Host "=========================================="  -ForegroundColor Cyan
Write-Host "FoxBridgeAgent Quick Build"  -ForegroundColor Cyan
Write-Host "=========================================="  -ForegroundColor Cyan
Write-Host ""

# Run full build with installer
& "$PSScriptRoot\build.ps1" -BuildInstaller -Configuration Release

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "=========================================="  -ForegroundColor Green
    Write-Host "✓ Build completed successfully!"  -ForegroundColor Green
    Write-Host "=========================================="  -ForegroundColor Green
    Write-Host ""
    Write-Host "Output files are in: $PSScriptRoot\output"  -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To install:"  -ForegroundColor Cyan
    Write-Host "  - Run FoxBridgeAgent-Setup.msi (Windows Installer)"  -ForegroundColor White
    Write-Host "  - Or run FoxBridgeAgent-Setup.exe (NSIS)"  -ForegroundColor White
    Write-Host ""
} else {
    Write-Host ""
    Write-Host "❌ Build failed. Check the errors above."  -ForegroundColor Red
    Write-Host ""
    exit 1
}
