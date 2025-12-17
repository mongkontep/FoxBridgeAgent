# FoxBridgeAgent Build Script
# Builds the executable and creates installer packages
# Requires: Visual Studio 2022, CMake, vcpkg, WiX Toolset v3

param(
    [switch]$SkipBuild,
    [switch]$BuildInstaller,
    [switch]$WixOnly,
    [switch]$NsisOnly,
    [switch]$Clean,
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

# Colors for output
function Write-Success { Write-Host $args -ForegroundColor Green }
function Write-Info { Write-Host $args -ForegroundColor Cyan }
function Write-Warning { Write-Host $args -ForegroundColor Yellow }
function Write-Error { Write-Host $args -ForegroundColor Red }

# Project paths
$ProjectRoot = $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build"
$InstallerDir = Join-Path $ProjectRoot "installer"
$OutputDir = Join-Path $ProjectRoot "output"
$ConfigDir = Join-Path $ProjectRoot "config"

Write-Info "=========================================="
Write-Info "FoxBridgeAgent Build Script"
Write-Info "=========================================="
Write-Info "Configuration: $Configuration"
Write-Info "Project Root: $ProjectRoot"
Write-Info ""

# Check prerequisites
Write-Info "Checking prerequisites..."

# Check CMake
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "❌ CMake not found. Install from https://cmake.org/"
    exit 1
}
Write-Success "✓ CMake found: $(cmake --version | Select-Object -First 1)"

# Check vcpkg
if (-not $env:VCPKG_ROOT) {
    Write-Warning "⚠ VCPKG_ROOT environment variable not set"
    Write-Info "Please set VCPKG_ROOT to your vcpkg installation directory"
    Write-Info "Example: `$env:VCPKG_ROOT = 'C:\vcpkg'"
    exit 1
}
Write-Success "✓ vcpkg found: $env:VCPKG_ROOT"

# Check Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Error "❌ Visual Studio 2022 not found"
    exit 1
}
$vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    Write-Error "❌ Visual Studio C++ tools not found"
    exit 1
}
Write-Success "✓ Visual Studio found: $vsPath"

# Check WiX Toolset (if building installer)
if ($BuildInstaller -or $WixOnly) {
    $wixPath = "${env:ProgramFiles(x86)}\WiX Toolset v3.11\bin"
    if (-not (Test-Path "$wixPath\candle.exe")) {
        Write-Warning "⚠ WiX Toolset v3 not found at $wixPath"
        Write-Info "Install from: https://wixtoolset.org/releases/"
        if ($WixOnly) { exit 1 }
    } else {
        Write-Success "✓ WiX Toolset found: $wixPath"
    }
}

# Check NSIS (if building installer)
if ($BuildInstaller -or $NsisOnly) {
    $nsisPath = "${env:ProgramFiles(x86)}\NSIS"
    if (-not (Test-Path "$nsisPath\makensis.exe")) {
        Write-Warning "⚠ NSIS not found at $nsisPath"
        Write-Info "Install from: https://nsis.sourceforge.io/"
        if ($NsisOnly) { exit 1 }
    } else {
        Write-Success "✓ NSIS found: $nsisPath"
    }
}

Write-Info ""

# Clean build directory
if ($Clean) {
    Write-Info "Cleaning build directory..."
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Success "✓ Build directory cleaned"
    }
    if (Test-Path $OutputDir) {
        Remove-Item -Recurse -Force $OutputDir
        Write-Success "✓ Output directory cleaned"
    }
}

# Create output directory
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

# Build executable
if (-not $SkipBuild -and -not $WixOnly -and -not $NsisOnly) {
    Write-Info "=========================================="
    Write-Info "Step 1: Building FoxBridgeAgent"
    Write-Info "=========================================="
    
    # Create build directory
    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
    }
    
    # Configure CMake
    Write-Info "Configuring CMake..."
    $vcpkgToolchain = Join-Path $env:VCPKG_ROOT "scripts\buildsystems\vcpkg.cmake"
    
    Push-Location $BuildDir
    try {
        & cmake .. `
            -DCMAKE_TOOLCHAIN_FILE="$vcpkgToolchain" `
            -DCMAKE_BUILD_TYPE=$Configuration `
            -G "Visual Studio 17 2022" `
            -A x64
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "❌ CMake configuration failed"
            exit 1
        }
        Write-Success "✓ CMake configuration successful"
        
        # Build
        Write-Info "Building..."
        & cmake --build . --config $Configuration --parallel
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "❌ Build failed"
            exit 1
        }
        Write-Success "✓ Build successful"
        
        # Check output
        $exePath = Join-Path $BuildDir "bin\$Configuration\FoxBridgeAgent.exe"
        if (-not (Test-Path $exePath)) {
            Write-Error "❌ FoxBridgeAgent.exe not found at $exePath"
            exit 1
        }
        
        $exeSize = (Get-Item $exePath).Length / 1MB
        Write-Success "✓ FoxBridgeAgent.exe built successfully ($([math]::Round($exeSize, 2)) MB)"
        
        # Copy to output
        Copy-Item $exePath -Destination $OutputDir
        Write-Success "✓ Copied to output directory"
        
    } finally {
        Pop-Location
    }
    
    Write-Info ""
}

# Build WiX installer
if (($BuildInstaller -or $WixOnly) -and (Test-Path "${env:ProgramFiles(x86)}\WiX Toolset v3.11\bin\candle.exe")) {
    Write-Info "=========================================="
    Write-Info "Step 2: Building WiX Installer"
    Write-Info "=========================================="
    
    $wixPath = "${env:ProgramFiles(x86)}\WiX Toolset v3.11\bin"
    $candleExe = Join-Path $wixPath "candle.exe"
    $lightExe = Join-Path $wixPath "light.exe"
    $wxsFile = Join-Path $InstallerDir "FoxBridgeAgent.wxs"
    $wixobjFile = Join-Path $OutputDir "FoxBridgeAgent.wixobj"
    $msiFile = Join-Path $OutputDir "FoxBridgeAgent-Setup.msi"
    
    # Set WiX variables
    $binPath = Join-Path $BuildDir "bin\$Configuration"
    
    Push-Location $InstallerDir
    try {
        # Compile WXS
        Write-Info "Compiling WXS..."
        & $candleExe "FoxBridgeAgent.wxs" `
            -out $wixobjFile `
            -dBinPath="$binPath" `
            -arch x64 `
            -ext WixUtilExtension
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "❌ WiX compilation failed"
            exit 1
        }
        Write-Success "✓ WiX compilation successful"
        
        # Link MSI
        Write-Info "Linking MSI..."
        & $lightExe $wixobjFile `
            -out $msiFile `
            -ext WixUtilExtension `
            -ext WixUIExtension `
            -cultures:en-us
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "❌ WiX linking failed"
            exit 1
        }
        Write-Success "✓ MSI created successfully"
        
        # Show MSI info
        $msiSize = (Get-Item $msiFile).Length / 1MB
        Write-Success "✓ FoxBridgeAgent-Setup.msi ($([math]::Round($msiSize, 2)) MB)"
        Write-Info "Location: $msiFile"
        
    } finally {
        Pop-Location
    }
    
    Write-Info ""
}

# Build NSIS installer
if (($BuildInstaller -or $NsisOnly) -and (Test-Path "${env:ProgramFiles(x86)}\NSIS\makensis.exe")) {
    Write-Info "=========================================="
    Write-Info "Step 3: Building NSIS Installer"
    Write-Info "=========================================="
    
    $nsisPath = "${env:ProgramFiles(x86)}\NSIS"
    $makensisExe = Join-Path $nsisPath "makensis.exe"
    $nsiFile = Join-Path $InstallerDir "FoxBridgeAgent.nsi"
    $exeFile = Join-Path $OutputDir "FoxBridgeAgent-Setup.exe"
    
    Push-Location $InstallerDir
    try {
        Write-Info "Compiling NSIS..."
        
        # Check if LICENSE.txt exists
        $licenseFile = Join-Path $InstallerDir "LICENSE.txt"
        if (-not (Test-Path $licenseFile)) {
            Write-Warning "⚠ LICENSE.txt not found, creating dummy file..."
            "MIT License`n`nCopyright (c) 2025 FoxBridgeAgent" | Out-File -FilePath $licenseFile -Encoding UTF8
        }
        
        & $makensisExe `
            /DPRODUCT_VERSION="1.0.0.0" `
            /DOUTPUT_DIR="$OutputDir" `
            "FoxBridgeAgent.nsi"
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "❌ NSIS compilation failed"
            exit 1
        }
        Write-Success "✓ NSIS installer created successfully"
        
        # Show installer info
        if (Test-Path $exeFile) {
            $exeSize = (Get-Item $exeFile).Length / 1MB
            Write-Success "✓ FoxBridgeAgent-Setup.exe ($([math]::Round($exeSize, 2)) MB)"
            Write-Info "Location: $exeFile"
        }
        
    } finally {
        Pop-Location
    }
    
    Write-Info ""
}

# Summary
Write-Info "=========================================="
Write-Info "Build Summary"
Write-Info "=========================================="

$outputs = Get-ChildItem -Path $OutputDir -File | Select-Object Name, @{Name="Size (MB)";Expression={[math]::Round($_.Length / 1MB, 2)}}
$outputs | Format-Table -AutoSize

Write-Success "✓ Build completed successfully!"
Write-Info "Output directory: $OutputDir"
Write-Info ""

# Instructions
Write-Info "Next steps:"
Write-Info "1. Test the installer: Run the .msi or .exe file"
Write-Info "2. Verify installation: Check C:\Program Files\FoxBridgeAgent\"
Write-Info "3. Check service: Run 'services.msc' and look for FoxBridgeAgent"
Write-Info "4. Test API: curl http://localhost:8787/health -H 'X-API-Key: YOUR_KEY'"
Write-Info ""
