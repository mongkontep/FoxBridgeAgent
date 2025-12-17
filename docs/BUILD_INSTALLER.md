# วิธีการ Build ตัวติดตั้ง FoxBridgeAgent

## 🎯 Quick Start (วิธีที่เร็วที่สุด)

```powershell
# เปิด PowerShell as Administrator
cd FoxBridgeAgent
.\build-quick.ps1
```

**ผลลัพธ์:** ไฟล์ติดตั้งจะอยู่ที่ `output/`
- `FoxBridgeAgent-Setup.msi` (Windows Installer)
- `FoxBridgeAgent-Setup.exe` (NSIS Installer)
- `FoxBridgeAgent.exe` (Standalone executable)

---

## 📋 Prerequisites (สิ่งที่ต้องติดตั้งก่อน)

### 1. Visual Studio 2022

**ดาวน์โหลด:** https://visualstudio.microsoft.com/downloads/

**ติดตั้ง Workload:**
- ✅ Desktop development with C++
- ✅ C++ CMake tools for Windows
- ✅ MSVC v143 - VS 2022 C++ x64/x86 build tools
- ✅ Windows 10/11 SDK

**ตรวจสอบ:**
```powershell
# ค้นหา Visual Studio
"${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest
```

### 2. CMake

**ดาวน์โหลด:** https://cmake.org/download/

**Version:** 3.20 ขึ้นไป

**ติดตั้ง:** เลือก "Add CMake to system PATH"

**ตรวจสอบ:**
```powershell
cmake --version
# ผลลัพธ์: cmake version 3.28.x หรือสูงกว่า
```

### 3. vcpkg (Package Manager)

**ติดตั้ง:**
```powershell
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# Bootstrap
.\bootstrap-vcpkg.bat

# Integrate
.\vcpkg integrate install
```

**ตั้งค่า Environment Variable:**
```powershell
# เพิ่ม VCPKG_ROOT
[System.Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\vcpkg', 'User')

# หรือใช้ System Properties → Environment Variables
# Variable: VCPKG_ROOT
# Value: C:\vcpkg
```

**ติดตั้ง Dependencies:**
```powershell
cd C:\vcpkg
.\vcpkg install boost-beast:x64-windows
.\vcpkg install nlohmann-json:x64-windows
.\vcpkg install spdlog:x64-windows
```

**ตรวจสอบ:**
```powershell
$env:VCPKG_ROOT
# ผลลัพธ์: C:\vcpkg
```

### 4. WiX Toolset v3.11 (สำหรับ MSI Installer)

**ดาวน์โหลด:** https://wixtoolset.org/releases/

**เลือก:** WiX Toolset v3.11.x (ไม่ใช่ v4!)

**ติดตั้ง:** Double-click installer → Next → Next → Install

**Path ที่ติดตั้ง:** `C:\Program Files (x86)\WiX Toolset v3.11\bin`

**ตรวจสอบ:**
```powershell
& "${env:ProgramFiles(x86)}\WiX Toolset v3.11\bin\candle.exe" -?
```

### 5. NSIS (สำหรับ EXE Installer)

**ดาวน์โหลด:** https://nsis.sourceforge.io/Download

**เลือก:** NSIS 3.x (latest stable)

**ติดตั้ง:** Double-click installer → Next → Install

**Path ที่ติดตั้ง:** `C:\Program Files (x86)\NSIS`

**ตรวจสอบ:**
```powershell
& "${env:ProgramFiles(x86)}\NSIS\makensis.exe" /VERSION
```

### 6. Visual FoxPro ODBC Driver (สำหรับ Runtime)

**ดาวน์โหลด:** https://www.microsoft.com/en-us/download/details.aspx?id=14839

**ติดตั้ง:** VFPODBCSetup.msi

**ตรวจสอบ:**
```powershell
Get-OdbcDriver -Name "*Visual FoxPro*"
```

---

## 🔧 Build Steps (ขั้นตอนการ Build)

### วิธีที่ 1: Quick Build (แนะนำ)

```powershell
# 1. เปิด PowerShell as Administrator
# 2. ไปที่โปรเจค
cd C:\Path\To\FoxBridgeAgent

# 3. Run quick build
.\build-quick.ps1

# ผลลัพธ์อยู่ที่: output/
```

### วิธีที่ 2: Build ทีละขั้นตอน

```powershell
# 1. Build executable ก่อน
.\build.ps1

# 2. จากนั้นค่อย build installer
.\build.ps1 -SkipBuild -BuildInstaller

# หรือ build แค่ WiX MSI
.\build.ps1 -SkipBuild -WixOnly

# หรือ build แค่ NSIS EXE
.\build.ps1 -SkipBuild -NsisOnly
```

### วิธีที่ 3: Clean Build (ลบของเก่าก่อน)

```powershell
# Clean และ rebuild ทั้งหมด
.\build.ps1 -Clean -BuildInstaller
```

### วิธีที่ 4: Manual Build

```powershell
# 1. Create build directory
mkdir build
cd build

# 2. Configure CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" -G "Visual Studio 17 2022" -A x64

# 3. Build
cmake --build . --config Release

# 4. Check output
ls bin\Release\FoxBridgeAgent.exe

# 5. Build WiX installer
cd ..\installer
& "${env:ProgramFiles(x86)}\WiX Toolset v3.11\bin\candle.exe" FoxBridgeAgent.wxs -out ..\output\FoxBridgeAgent.wixobj -dBinPath="..\build\bin\Release" -arch x64 -ext WixUtilExtension
& "${env:ProgramFiles(x86)}\WiX Toolset v3.11\bin\light.exe" ..\output\FoxBridgeAgent.wixobj -out ..\output\FoxBridgeAgent-Setup.msi -ext WixUtilExtension -ext WixUIExtension

# 6. Build NSIS installer
& "${env:ProgramFiles(x86)}\NSIS\makensis.exe" FoxBridgeAgent.nsi
```

---

## 📁 Output Files

หลังจาก build เสร็จ ไฟล์จะอยู่ที่ `output/`:

```
output/
├── FoxBridgeAgent.exe          (~2-5 MB)   - Standalone executable
├── FoxBridgeAgent-Setup.msi    (~3-7 MB)   - Windows Installer
├── FoxBridgeAgent-Setup.exe    (~3-7 MB)   - NSIS Installer
└── FoxBridgeAgent.wixobj                   - Intermediate file
```

---

## 🎛️ Build Script Options

### build.ps1 Parameters

```powershell
# Build executable only
.\build.ps1

# Build executable + installers
.\build.ps1 -BuildInstaller

# Skip executable build, create installers only
.\build.ps1 -SkipBuild -BuildInstaller

# Build only WiX MSI
.\build.ps1 -WixOnly

# Build only NSIS EXE
.\build.ps1 -NsisOnly

# Clean build
.\build.ps1 -Clean

# Clean + rebuild with installers
.\build.ps1 -Clean -BuildInstaller

# Build Debug version
.\build.ps1 -Configuration Debug
```

---

## 🐛 Troubleshooting

### ❌ CMake not found

**ปัญหา:**
```
cmake : The term 'cmake' is not recognized...
```

**แก้ไข:**
1. ติดตั้ง CMake จาก https://cmake.org/
2. เลือก "Add CMake to system PATH" ตอนติดตั้ง
3. Restart PowerShell

### ❌ vcpkg not found

**ปัญหา:**
```
VCPKG_ROOT environment variable not set
```

**แก้ไข:**
```powershell
# ตั้งค่า environment variable
$env:VCPKG_ROOT = "C:\vcpkg"

# หรือตั้งค่าถาวร
[System.Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\vcpkg', 'User')

# Restart PowerShell
```

### ❌ Boost not found

**ปัญหา:**
```
Could NOT find Boost
```

**แก้ไข:**
```powershell
cd $env:VCPKG_ROOT
.\vcpkg install boost-beast:x64-windows boost-system:x64-windows
```

### ❌ Visual Studio not found

**ปัญหา:**
```
Visual Studio 2022 not found
```

**แก้ไข:**
1. ติดตั้ง Visual Studio 2022 Community (ฟรี)
2. เลือก "Desktop development with C++"
3. Restart

### ❌ WiX compilation failed

**ปัญหา:**
```
candle.exe not found
```

**แก้ไข:**
1. ติดตั้ง WiX Toolset v3.11 (ไม่ใช่ v4!)
2. Check path: `C:\Program Files (x86)\WiX Toolset v3.11\bin`

**ปัญหา:**
```
Error LGHT0001: Cannot find the file ...
```

**แก้ไข:**
```powershell
# Build executable ก่อน
.\build.ps1

# จากนั้นค่อย build installer
.\build.ps1 -SkipBuild -WixOnly
```

### ❌ NSIS compilation failed

**ปัญหา:**
```
makensis.exe not found
```

**แก้ไข:**
1. ติดตั้ง NSIS จาก https://nsis.sourceforge.io/
2. Check path: `C:\Program Files (x86)\NSIS`

**ปัญหา:**
```
Error: Can't open script file "LICENSE.txt"
```

**แก้ไข:**
```powershell
# สร้าง LICENSE.txt
@"
MIT License

Copyright (c) 2025 FoxBridgeAgent

Permission is hereby granted, free of charge...
"@ | Out-File -FilePath installer\LICENSE.txt -Encoding UTF8
```

### ❌ Executable crashes immediately

**ปัญหา:**
Application crashes หลัง build

**แก้ไข:**
1. ตรวจสอบ Visual C++ Redistributable:
   - ดาวน์โหลด: https://aka.ms/vs/17/release/vc_redist.x64.exe
2. ตรวจสอบ VFP ODBC Driver
3. ตรวจสอบ config.json

### ❌ PowerShell execution policy

**ปัญหา:**
```
cannot be loaded because running scripts is disabled
```

**แก้ไข:**
```powershell
# Run as Administrator
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser

# หรือรันแบบ bypass
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

---

## 🔐 Code Signing (Optional)

### Sign Executables and Installers

```powershell
# Sign executable
signtool sign /f YourCertificate.pfx /p YourPassword /t http://timestamp.digicert.com output\FoxBridgeAgent.exe

# Sign MSI installer
signtool sign /f YourCertificate.pfx /p YourPassword /t http://timestamp.digicert.com output\FoxBridgeAgent-Setup.msi

# Sign EXE installer
signtool sign /f YourCertificate.pfx /p YourPassword /t http://timestamp.digicert.com output\FoxBridgeAgent-Setup.exe
```

**ข้อดี:**
- ไม่มี SmartScreen warning
- เพิ่มความน่าเชื่อถือ
- จำเป็นสำหรับ production distribution

---

## 📦 Distribution

### Upload ไฟล์ Installer

**Recommended hosting:**
- GitHub Releases
- Azure Blob Storage
- AWS S3
- Google Drive
- Dropbox

### ตัวอย่าง GitHub Release

```bash
# Create release with installers
gh release create v1.0.0 \
  output/FoxBridgeAgent-Setup.msi \
  output/FoxBridgeAgent-Setup.exe \
  --title "FoxBridgeAgent v1.0.0" \
  --notes "Production release"
```

---

## 🎯 CI/CD (Automated Build)

### GitHub Actions Example

```yaml
name: Build Installer

on:
  push:
    tags:
      - 'v*'

jobs:
  build:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup CMake
      uses: lukka/get-cmake@latest
    
    - name: Setup vcpkg
      uses: lukka/run-vcpkg@v11
    
    - name: Build
      run: |
        .\build.ps1 -BuildInstaller
    
    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: installers
        path: output/*
```

---

## 📝 Checklist ก่อน Release

```
□ Build สำเร็จโดยไม่มี warnings
□ ทดสอบติดตั้งบนเครื่องสะอาด (Clean Windows)
□ ทดสอบ Service start อัตโนมัติ
□ ทดสอบ API endpoints
□ ทดสอบ Cloudflare Tunnel connection
□ ตรวจสอบ logs ไม่มี errors
□ ทดสอบ uninstall
□ Code signing (ถ้ามี certificate)
□ Update version numbers
□ Update README.md
□ Create release notes
```

---

## 🆘 Get Help

**ถ้าติดปัญหา:**
1. ตรวจสอบ error messages ใน PowerShell
2. ดู logs ใน `build/` directory
3. ตรวจสอบ prerequisites ทั้งหมดติดตั้งแล้ว
4. ลอง clean build: `.\build.ps1 -Clean -BuildInstaller`

**ข้อมูลที่ต้องเตรียมเมื่อขอความช่วยเหลือ:**
- Windows version
- Visual Studio version
- CMake version (`cmake --version`)
- vcpkg location (`$env:VCPKG_ROOT`)
- Complete error messages

---

## สรุป

**Build ตัวติดตั้งง่ายๆ:**
```powershell
# 1. ติดตั้ง prerequisites (ครั้งเดียว)
# 2. Clone project
# 3. Run:
.\build-quick.ps1

# 4. ผลลัพธ์อยู่ที่ output/
```

**ใช้เวลา:** ~5-10 นาที (ครั้งแรก), ~2-3 นาที (ครั้งต่อไป)

**Output:** MSI และ EXE installer พร้อมใช้งาน! 🎉
