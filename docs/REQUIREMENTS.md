# System Requirements

## ความต้องการของระบบสำหรับ FoxBridgeAgent

### Requirements ที่ **จำเป็น** (Critical)

#### 1. Windows Version ✅
**ต้องการ:**
- Windows 10 (Version 1809 or later)
- Windows 11
- Windows Server 2019 or later
- Windows Server 2022

**ตรวจสอบ:**
```powershell
# PowerShell
winver

# หรือ
[System.Environment]::OSVersion
```

**ติดตั้ง:** อัพเกรด Windows ถ้าเวอร์ชันต่ำกว่าที่กำหนด

---

#### 2. Administrator Privileges ✅
**ต้องการ:**
- ต้องรัน Installer ด้วย Administrator

**วิธีรัน:**
```
คลิกขวาที่ FoxBridgeAgent-Setup.exe
→ Run as Administrator
```

---

#### 3. Visual FoxPro ODBC Driver ✅
**ต้องการ:**
- Microsoft Visual FoxPro ODBC Driver (VFP 9.0 SP2)

**ดาวน์โหลด:**
- https://www.microsoft.com/en-us/download/details.aspx?id=14839
- ไฟล์: `VFPODBCSetup.msi`

**ติดตั้ง:**
```powershell
# ดาวน์โหลดแล้วรัน
.\VFPODBCSetup.msi
```

**ตรวจสอบ:**
```powershell
# เปิด ODBC Data Source Administrator
odbcad32.exe

# ไปที่ Drivers tab
# ต้องเห็น "Microsoft Visual FoxPro Driver"
```

---

#### 4. Visual C++ Redistributable ✅
**ต้องการ:**
- Microsoft Visual C++ 2015-2022 Redistributable (x64)

**ดาวน์โหลด:**
- https://aka.ms/vs/17/release/vc_redist.x64.exe

**ติดตั้ง:**
```powershell
# ดาวน์โหลดแล้วรัน
.\vc_redist.x64.exe
```

**ตรวจสอบ:**
```powershell
# Settings → Apps → Installed apps
# ค้นหา "Microsoft Visual C++ 2015-2022 Redistributable (x64)"
```

---

#### 5. Disk Space ✅
**ต้องการ:**
- อย่างน้อย 500 MB พื้นที่ว่างใน C:\

**ตรวจสอบ:**
```powershell
Get-PSDrive C | Select-Object Used,Free

# หรือ
(Get-PSDrive C).Free / 1MB
```

---

#### 6. Port 8787 Available ✅
**ต้องการ:**
- Port 8787 ต้องว่าง (ไม่มีโปรแกรมอื่นใช้อยู่)

**ตรวจสอบ:**
```powershell
# ตรวจสอบ port 8787
netstat -ano | findstr :8787

# ถ้าไม่มี output = port ว่าง (ดี)
# ถ้ามี output = มีโปรแกรมใช้อยู่ (แก้ไข)
```

**แก้ไข:**
```powershell
# ดู process ที่ใช้ port
Get-Process -Id <PID>

# หยุด service/process นั้น
Stop-Process -Id <PID>
```

---

### Requirements ที่ **แนะนำ** (Recommended)

#### 7. .NET Framework 4.7.2+ ⚠️
**ต้องการ:**
- .NET Framework 4.7.2 or later (สำหรับ installer components)

**ติดตั้ง:**
- มักจะมีใน Windows 10/11 อยู่แล้ว
- ดาวน์โหลด: https://dotnet.microsoft.com/download/dotnet-framework

**ตรวจสอบ:**
```powershell
Get-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full" -Name Release | 
  ForEach-Object { $_.Release }

# Release number >= 461808 = .NET 4.7.2+
```

---

#### 8. Windows Firewall ⚠️
**ต้องการ:**
- Windows Firewall enabled (แนะนำ)

**ตรวจสอบ:**
```powershell
Get-NetFirewallProfile | Select-Object Name,Enabled
```

**หมายเหตุ:**
- FoxBridgeAgent bind ที่ localhost (127.0.0.1) เท่านั้น
- ไม่ต้อง open port ใน firewall
- เข้าถึงผ่าน Cloudflare Tunnel

---

#### 9. ExpressD Database Path ⚠️
**ต้องการ:**
- Path ที่มี DBF files (เช่น `D:\ExpressD\Data`)

**ตรวจสอบ:**
```powershell
# ดูว่ามี DBF files หรือไม่
Get-ChildItem "D:\ExpressD\Data" -Filter *.dbf

# นับจำนวน DBF files
(Get-ChildItem "D:\ExpressD\Data" -Filter *.dbf).Count
```

---

### Requirements ที่ **เป็นทางเลือก** (Optional)

#### 10. cloudflared 🔧
**ต้องการ:**
- cloudflared สำหรับ Cloudflare Tunnel (ถ้าต้องการ remote access)

**ติดตั้ง:**
```powershell
# วิธีที่ 1: winget
winget install Cloudflare.cloudflared

# วิธีที่ 2: ดาวน์โหลดเอง
# https://github.com/cloudflare/cloudflared/releases
```

**ตรวจสอบ:**
```powershell
cloudflared --version
```

**หมายเหตุ:**
- ไม่จำเป็นถ้าใช้แค่ local access
- จำเป็นถ้าต้องการเข้าถึงจาก internet

---

## Quick Check Script

สคริปต์ตรวจสอบทุก requirements อัตโนมัติ:

```powershell
# SaveAs: check-requirements.ps1

Write-Host "=== FoxBridgeAgent System Requirements Check ===" -ForegroundColor Cyan
Write-Host ""

$passed = 0
$failed = 0
$warnings = 0

# 1. Windows Version
Write-Host "1. Checking Windows Version..." -NoNewline
$osVersion = [System.Environment]::OSVersion.Version
if ($osVersion.Major -ge 10) {
    Write-Host " ✓ Pass" -ForegroundColor Green
    Write-Host "   Version: $($osVersion)" -ForegroundColor Gray
    $passed++
} else {
    Write-Host " ✗ FAIL" -ForegroundColor Red
    Write-Host "   Need Windows 10+ or Server 2019+" -ForegroundColor Yellow
    $failed++
}

# 2. Administrator
Write-Host "2. Checking Administrator Privileges..." -NoNewline
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
$isAdmin = $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($isAdmin) {
    Write-Host " ✓ Pass" -ForegroundColor Green
    $passed++
} else {
    Write-Host " ✗ FAIL" -ForegroundColor Red
    Write-Host "   Run PowerShell as Administrator" -ForegroundColor Yellow
    $failed++
}

# 3. VFP ODBC Driver
Write-Host "3. Checking Visual FoxPro ODBC Driver..." -NoNewline
$vfpDriver = Test-Path "HKLM:\SOFTWARE\ODBC\ODBCINST.INI\Microsoft Visual FoxPro Driver"
if ($vfpDriver) {
    Write-Host " ✓ Pass" -ForegroundColor Green
    $passed++
} else {
    Write-Host " ✗ FAIL" -ForegroundColor Red
    Write-Host "   Download: https://www.microsoft.com/en-us/download/details.aspx?id=14839" -ForegroundColor Yellow
    $failed++
}

# 4. VC++ Redistributable
Write-Host "4. Checking Visual C++ Redistributable..." -NoNewline
$vcRedist = Get-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" -ErrorAction SilentlyContinue
if ($vcRedist -and $vcRedist.Installed -eq 1) {
    Write-Host " ✓ Pass" -ForegroundColor Green
    Write-Host "   Version: $($vcRedist.Version)" -ForegroundColor Gray
    $passed++
} else {
    Write-Host " ✗ FAIL" -ForegroundColor Red
    Write-Host "   Download: https://aka.ms/vs/17/release/vc_redist.x64.exe" -ForegroundColor Yellow
    $failed++
}

# 5. Disk Space
Write-Host "5. Checking Disk Space..." -NoNewline
$drive = Get-PSDrive C
$freeMB = [math]::Round($drive.Free / 1MB)
if ($freeMB -ge 500) {
    Write-Host " ✓ Pass" -ForegroundColor Green
    Write-Host "   Available: $freeMB MB" -ForegroundColor Gray
    $passed++
} else {
    Write-Host " ✗ FAIL" -ForegroundColor Red
    Write-Host "   Need 500 MB, Available: $freeMB MB" -ForegroundColor Yellow
    $failed++
}

# 6. Port 8787
Write-Host "6. Checking Port 8787..." -NoNewline
$portInUse = Get-NetTCPConnection -LocalPort 8787 -ErrorAction SilentlyContinue
if (-not $portInUse) {
    Write-Host " ✓ Pass" -ForegroundColor Green
    $passed++
} else {
    Write-Host " ✗ FAIL" -ForegroundColor Red
    Write-Host "   Port 8787 is in use" -ForegroundColor Yellow
    $failed++
}

# 7. .NET Framework (Optional)
Write-Host "7. Checking .NET Framework..." -NoNewline
$netRelease = (Get-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full" -ErrorAction SilentlyContinue).Release
if ($netRelease -ge 461808) {
    Write-Host " ✓ Pass" -ForegroundColor Green
    $passed++
} else {
    Write-Host " ⚠ Warning" -ForegroundColor Yellow
    Write-Host "   .NET Framework 4.7.2+ recommended" -ForegroundColor Yellow
    $warnings++
}

# 8. cloudflared (Optional)
Write-Host "8. Checking cloudflared..." -NoNewline
$cloudflared = Get-Command cloudflared -ErrorAction SilentlyContinue
if ($cloudflared) {
    Write-Host " ✓ Pass" -ForegroundColor Green
    $passed++
} else {
    Write-Host " ⚠ Warning" -ForegroundColor Yellow
    Write-Host "   Optional: winget install Cloudflare.cloudflared" -ForegroundColor Yellow
    $warnings++
}

Write-Host ""
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host "Passed:   $passed" -ForegroundColor Green
Write-Host "Failed:   $failed" -ForegroundColor Red
Write-Host "Warnings: $warnings" -ForegroundColor Yellow

if ($failed -eq 0) {
    Write-Host ""
    Write-Host "✓ Ready to install FoxBridgeAgent!" -ForegroundColor Green
    exit 0
} else {
    Write-Host ""
    Write-Host "✗ Please fix the issues above before installing." -ForegroundColor Red
    exit 1
}
```

**วิธีใช้:**
```powershell
# รัน PowerShell as Administrator
.\check-requirements.ps1
```

---

## Installation Order

ลำดับการติดตั้งที่แนะนำ:

```
1. ติดตั้ง Visual FoxPro ODBC Driver
   └─ VFPODBCSetup.msi

2. ติดตั้ง Visual C++ Redistributable
   └─ vc_redist.x64.exe

3. ติดตั้ง cloudflared (ถ้าต้องการ remote access)
   └─ winget install Cloudflare.cloudflared

4. รันคำสั่งตรวจสอบ requirements
   └─ .\check-requirements.ps1

5. ติดตั้ง FoxBridgeAgent
   └─ คลิกขวา FoxBridgeAgent-Setup.exe → Run as Administrator
```

---

## Troubleshooting

### ปัญหา: "Visual FoxPro ODBC Driver not found"

**วิธีแก้:**
1. ดาวน์โหลด VFP ODBC Driver
2. รัน VFPODBCSetup.msi
3. Restart เครื่อง
4. ทดสอบ: เปิด `odbcad32.exe` → Drivers tab

### ปัญหา: "Port 8787 in use"

**วิธีแก้:**
```powershell
# หา process ที่ใช้ port
netstat -ano | findstr :8787

# Output: TCP  0.0.0.0:8787  0.0.0.0:0  LISTENING  1234
# PID = 1234

# ดู process name
Get-Process -Id 1234

# หยุด process
Stop-Process -Id 1234
```

### ปัญหา: "Not running as Administrator"

**วิธีแก้:**
1. คลิกขวาที่ installer
2. เลือก "Run as Administrator"
3. คลิก Yes ใน UAC prompt

### ปัญหา: "VC++ Redistributable not found"

**วิธีแก้:**
```powershell
# ดาวน์โหลดและติดตั้ง
Invoke-WebRequest -Uri "https://aka.ms/vs/17/release/vc_redist.x64.exe" -OutFile "vc_redist.x64.exe"
.\vc_redist.x64.exe
```

---

## สรุป

### ✅ จำเป็นต้องมี (ติดตั้งก่อน):
1. Windows 10+ / Server 2019+
2. Administrator privileges
3. Visual FoxPro ODBC Driver
4. Visual C++ Redistributable
5. Disk space 500+ MB
6. Port 8787 available

### ⚠️ แนะนำให้มี:
7. .NET Framework 4.7.2+

### 🔧 เป็นทางเลือก:
8. cloudflared (สำหรับ remote access)

Installer จะตรวจสอบทุกอย่างอัตโนมัติก่อนติดตั้ง และแจ้งเตือนถ้าขาดอะไร
