# Build FoxBridgeAgent บน Mac ด้วย GitHub Actions

เนื่องจาก FoxBridgeAgent เป็น Windows-specific application ไม่สามารถ build จาก Mac ได้โดยตรง แต่สามารถใช้ **GitHub Actions** ให้ GitHub build ให้อัตโนมัติบน Windows runner

---

## 🚀 Quick Start (จาก Mac)

### 1. Push โค้ดขึ้น GitHub

```bash
# อยู่ใน FoxBridgeAgent directory
cd /Users/inverz-mac/Desktop/Innovatative\ Projects/ExpressDxFoxpro/FoxBridgeAgent

# Init git (ถ้ายังไม่ได้ทำ)
git init
git add .
git commit -m "Initial commit"

# Create repo on GitHub (ใช้ gh cli หรือ web)
gh repo create FoxBridgeAgent --public --source=. --push

# หรือเชื่อมกับ existing repo
git remote add origin https://github.com/YOUR_USERNAME/FoxBridgeAgent.git
git branch -M main
git push -u origin main
```

### 2. GitHub Actions จะ Build อัตโนมัติ

- GitHub จะเห็น `.github/workflows/build.yml`
- Build บน Windows runner อัตโนมัติ
- ใช้เวลาประมาณ 10-15 นาที

### 3. Download Installers

**ผ่าน GitHub Web:**
1. ไปที่ `https://github.com/YOUR_USERNAME/FoxBridgeAgent/actions`
2. คลิกที่ workflow run ล่าสุด
3. Scroll ลงไปหา **Artifacts**
4. Download:
   - `FoxBridgeAgent-Executable`
   - `FoxBridgeAgent-MSI-Installer`
   - `FoxBridgeAgent-NSIS-Installer`

**ผ่าน GitHub CLI (แนะนำ):**
```bash
# ดู workflow runs
gh run list

# Download artifacts จาก run ล่าสุด
gh run download

# จะได้ไฟล์ใน:
# ./FoxBridgeAgent-MSI-Installer/FoxBridgeAgent-Setup.msi
# ./FoxBridgeAgent-NSIS-Installer/FoxBridgeAgent-Setup.exe
# ./FoxBridgeAgent-Executable/FoxBridgeAgent.exe
```

---

## 🏷️ Build Release Version

### สร้าง Git Tag

```bash
# Tag version
git tag v1.0.0
git push origin v1.0.0

# GitHub Actions จะ:
# 1. Build อัตโนมัติ
# 2. สร้าง GitHub Release
# 3. แนบ installers ใน Release
```

### Download Release

```bash
# ผ่าน gh cli
gh release download v1.0.0

# หรือไปที่
# https://github.com/YOUR_USERNAME/FoxBridgeAgent/releases/tag/v1.0.0
```

---

## 🔍 ตรวจสอบ Build Status

### ผ่าน GitHub Web

1. ไปที่ `https://github.com/YOUR_USERNAME/FoxBridgeAgent`
2. คลิกที่ **Actions** tab
3. เห็น workflow runs พร้อม status:
   - ✅ สีเขียว = Build สำเร็จ
   - ❌ สีแดง = Build ล้มเหลว
   - 🟡 สีเหลือง = กำลัง build

### ผ่าน Terminal (Mac)

```bash
# ดู workflow runs
gh run list

# ดูรายละเอียด run ล่าสุด
gh run view

# ดู logs
gh run view --log

# ดู status แบบ real-time
gh run watch
```

### เพิ่ม Badge ใน README

เพิ่มใน README.md:
```markdown
[![Build](https://github.com/YOUR_USERNAME/FoxBridgeAgent/actions/workflows/build.yml/badge.svg)](https://github.com/YOUR_USERNAME/FoxBridgeAgent/actions/workflows/build.yml)
```

---

## 🛠️ วิธีอื่นๆ (ถ้าไม่ใช้ GitHub Actions)

### Option 1: Parallels Desktop + Windows 11

```bash
# 1. ติดตั้ง Parallels Desktop (มีค่าใช้จ่าย ~$100/year)
# Download: https://www.parallels.com/

# 2. สร้าง Windows 11 VM (ฟรี)
# Download Windows 11 ARM: https://www.microsoft.com/software-download/windows11

# 3. ใน Windows VM:
# - ติดตั้ง Git
# - Clone project
# - ติดตั้ง prerequisites
# - Build ปกติ
```

**Performance:**
- M1/M2/M3 Mac: เร็วมาก (ARM architecture)
- Intel Mac: เร็วปกติ

### Option 2: UTM (Free)

```bash
# 1. ติดตั้ง UTM (ฟรี)
brew install --cask utm

# 2. ติดตั้ง Windows 11 ARM
# Follow: https://docs.getutm.app/guides/windows/

# 3. Build ใน Windows VM
```

**ข้อดี:**
- ฟรี
- Open source

**ข้อเสี:**
- ช้ากว่า Parallels เล็กน้อย

### Option 3: Remote Windows Server

```bash
# 1. เช่า Windows VPS (DigitalOcean, Vultr, AWS)
# ราคา: ~$10-20/month

# 2. SSH จาก Mac
ssh administrator@your-windows-server

# 3. Setup และ build
cd C:\Projects
git clone https://github.com/YOUR_USERNAME/FoxBridgeAgent.git
cd FoxBridgeAgent
.\build-quick.ps1

# 4. Download ผ่าน SCP
scp administrator@your-windows-server:C:\Projects\FoxBridgeAgent\output\* ./
```

### Option 4: Cross-Compilation (ไม่แนะนำ)

**ทำได้หรือไม่?** ทำได้แต่ยากมาก

**ปัญหา:**
- Windows Service APIs ไม่มีใน mingw-w64
- Visual Studio specific features
- WiX/NSIS ต้องใช้ Windows
- Testing ทำไม่ได้บน Mac

**สรุป:** ไม่คุ้มความพยายาม

---

## 📊 เปรียบเทียบวิธีต่างๆ

| วิธี | ค่าใช้จ่าย | ความยาก | เวลา Build | แนะนำ |
|-----|-----------|---------|-----------|-------|
| **GitHub Actions** | ฟรี | ง่าย | 10-15 นาที | ⭐⭐⭐⭐⭐ |
| Parallels Desktop | $100/ปี | ปานกลาง | 5-10 นาที | ⭐⭐⭐⭐ |
| UTM (Free VM) | ฟรี | ปานกลาง | 10-20 นาที | ⭐⭐⭐ |
| Remote Windows | $10-20/เดือน | ปานกลาง | 5-10 นาที | ⭐⭐⭐ |
| Cross-compile | ฟรี | **ยากมาก** | ไม่รู้ | ❌ |

---

## 🎯 คำแนะนำสำหรับคุณ

### ✅ ใช้ GitHub Actions (วิธีที่ดีที่สุด)

**ข้อดี:**
- ✅ ไม่ต้องมี Windows
- ✅ ฟรี (public repo: unlimited, private: 2000 minutes/month)
- ✅ Build อัตโนมัติทุกครั้งที่ push
- ✅ สร้าง Release อัตโนมัติ
- ✅ มี cache สำหรับ dependencies

**ข้อเสีย:**
- ❌ ช้ากว่า build ใน local (10-15 นาที)
- ❌ ต้อง push code ขึ้น GitHub

### วิธีใช้:

```bash
# 1. Push code (มีไฟล์ .github/workflows/build.yml แล้ว)
git add .
git commit -m "Build installer"
git push

# 2. รอ 10-15 นาที

# 3. Download
gh run download

# เสร็จแล้ว! 🎉
```

---

## 🐛 Troubleshooting

### GitHub Actions build ล้มเหลว

**ดู error logs:**
```bash
gh run view --log
```

**สาเหตุที่พบบ่อย:**
1. vcpkg dependencies ไม่พบ → เพิ่ม `vcpkg.json`
2. CMake configuration error → ตรวจสอบ `CMakeLists.txt`
3. WiX/NSIS ไม่พบ → workflow ติดตั้งให้อัตโนมัติแล้ว
4. Timeout → เพิ่ม `timeout-minutes` ใน workflow

### Download artifacts ไม่ได้

```bash
# ตรวจสอบว่า workflow run สำเร็จ
gh run list --workflow=build.yml

# Download specific run
gh run download RUN_ID

# หรือ download ผ่าน web
open https://github.com/YOUR_USERNAME/FoxBridgeAgent/actions
```

### VM ช้าเกินไป

**ปรับ VM settings:**
- เพิ่ม RAM (8GB+ แนะนำ)
- เพิ่ม CPU cores (4+ แนะนำ)
- ใช้ SSD storage

---

## 📚 เอกสารเพิ่มเติม

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [GitHub CLI Documentation](https://cli.github.com/manual/)
- [vcpkg on GitHub Actions](https://github.com/marketplace/actions/run-vcpkg)

---

## สรุป

**จาก Mac ไม่สามารถ build โดยตรงได้** แต่ใช้ **GitHub Actions เป็นวิธีที่ดีที่สุด:**

```bash
# Setup (ครั้งเดียว)
git init
gh repo create FoxBridgeAgent --public --source=. --push

# Build (ทุกครั้งที่ต้องการ)
git add .
git commit -m "Update"
git push

# Wait 10-15 minutes...

# Download
gh run download

# Done! 🎉
```

**ง่าย, ฟรี, อัตโนมัติ!** ✨
