# Logo และ Icons สำหรับ FoxBridgeAgent

## โครงสร้างโฟลเดอร์

```
FoxBridgeAgent/
└── assets/
    ├── icons/              ← Application Icons
    │   ├── foxbridge.ico   (โปรแกรม .exe)
    │   ├── foxbridge-16.png
    │   ├── foxbridge-32.png
    │   ├── foxbridge-48.png
    │   ├── foxbridge-64.png
    │   ├── foxbridge-128.png
    │   └── foxbridge-256.png
    │
    └── installer/          ← Installer Images
        ├── installer.ico   (ไฟล์ installer)
        ├── banner.bmp      (493 x 58 pixels)
        ├── dialog.bmp      (493 x 312 pixels)
        └── license.rtf     (ใบอนุญาต)
```

---

## 1. Application Icon (.ico)

### ไฟล์: `assets/icons/foxbridge.ico`

**ใช้สำหรับ:**
- FoxBridgeAgent.exe (executable icon)
- Task Manager
- Services (Windows Service icon)
- Desktop shortcut

**ขนาดที่ต้องการใน .ico file:**
```
16x16   - Small icons, Task Manager
32x32   - Default size
48x48   - Windows Explorer
64x64   - High DPI
128x128 - Large icons
256x256 - Extra large icons
```

**วิธีสร้าง .ico จาก PNG:**

### Windows:
```powershell
# ใช้ online converter
# https://www.icoconverter.com/
# https://convertio.co/png-ico/

# หรือใช้ ImageMagick
magick convert foxbridge.png -define icon:auto-resize=256,128,64,48,32,16 foxbridge.ico
```

### macOS:
```bash
# ใช้ ImageMagick
brew install imagemagick
magick convert foxbridge.png -define icon:auto-resize=256,128,64,48,32,16 foxbridge.ico
```

---

## 2. Installer Icon (.ico)

### ไฟล์: `assets/installer/installer.ico`

**ใช้สำหรับ:**
- FoxBridgeAgent-Setup.exe
- FoxBridgeAgent-Setup.msi
- เห็นใน Downloads folder, Desktop

**ขนาด:** เหมือน Application Icon (16x16 ถึง 256x256)

---

## 3. Installer Banner Image

### ไฟล์: `assets/installer/banner.bmp`

**ขนาด:** 493 x 58 pixels

**ใช้สำหรับ:**
- แถบด้านบนของ installer dialog

**ตัวอย่าง:**
```
┌─────────────────────────────────────────────────────┐
│ [Logo] FoxBridgeAgent Installation           [X]   │ ← Banner อยู่ตรงนี้
├─────────────────────────────────────────────────────┤
│ Welcome to the installation wizard...               │
│                                                      │
│ ...                                                  │
└─────────────────────────────────────────────────────┘
```

**คำแนะนำ:**
- พื้นหลังสีขาวหรือไล่ระดับสี (gradient)
- Logo ด้านซ้าย
- ชื่อโปรแกรม "FoxBridgeAgent" ตรงกลาง
- ความละเอียด 96 DPI
- Format: BMP (24-bit color)

---

## 4. Installer Dialog Image

### ไฟล์: `assets/installer/dialog.bmp`

**ขนาด:** 493 x 312 pixels

**ใช้สำหรับ:**
- หน้า Welcome/Finish ของ installer

**ตัวอย่าง:**
```
┌─────────────────────────────────────────────────────┐
│ Welcome to FoxBridgeAgent Setup              [X]   │
├────┬────────────────────────────────────────────────┤
│    │ Welcome to the FoxBridgeAgent Setup Wizard   │
│    │                                               │
│ L  │ This will install FoxBridgeAgent on your     │
│ o  │ computer.                                     │
│ g  │                                               │
│ o  │ Click Next to continue.                      │
│    │                                               │
│ B  │                                               │
│ M  │                                               │
│ P  │                        [< Back] [Next >]     │
└────┴────────────────────────────────────────────────┘
     ↑
     Dialog Image อยู่ตรงนี้
```

**คำแนะนำ:**
- รูปภาพแนวตั้งด้านซ้าย
- สีสันสอดคล้องกับ brand
- อาจใส่ logo, ภาพ server/database
- Format: BMP (24-bit color)

---

## 5. Add/Remove Programs Icon

**ใช้สำหรับ:**
- Settings → Apps → Installed apps
- Control Panel → Programs and Features

**ขนาด:** 32x32 หรือ 48x48 pixels

**ตั้งค่าใน WiX:**
```xml
<Property Id="ARPPRODUCTICON" Value="foxbridge.ico" />
```

---

## วิธีใช้งาน Logo ใน Project

### 1. ใส่ใน CMakeLists.txt (Application Icon)

```cmake
# ไฟล์: CMakeLists.txt

# Windows resource file
if(WIN32)
    set(APP_ICON "${CMAKE_SOURCE_DIR}/assets/icons/foxbridge.ico")
    configure_file(
        "${CMAKE_SOURCE_DIR}/src/resources.rc.in"
        "${CMAKE_BINARY_DIR}/resources.rc"
    )
    list(APPEND SOURCES "${CMAKE_BINARY_DIR}/resources.rc")
endif()

add_executable(FoxBridgeAgent ${SOURCES})
```

**สร้างไฟล์ `src/resources.rc.in`:**
```rc
// FoxBridgeAgent Resources
#include <windows.h>

// Application Icon
IDI_ICON1 ICON "@APP_ICON@"

// Version Info
VS_VERSION_INFO VERSIONINFO
FILEVERSION     1,0,0,0
PRODUCTVERSION  1,0,0,0
FILEFLAGSMASK   VS_FFI_FILEFLAGSMASK
FILEFLAGS       0x0L
FILEOS          VOS_NT_WINDOWS32
FILETYPE        VFT_APP
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName",      "YourCompany"
            VALUE "FileDescription",  "FoxBridgeAgent - HTTP API Server for ExpressD"
            VALUE "FileVersion",      "1.0.0.0"
            VALUE "InternalName",     "FoxBridgeAgent"
            VALUE "LegalCopyright",   "Copyright (C) 2025"
            VALUE "OriginalFilename", "FoxBridgeAgent.exe"
            VALUE "ProductName",      "FoxBridgeAgent"
            VALUE "ProductVersion",   "1.0.0.0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END
```

### 2. ใส่ใน WiX Installer

**ไฟล์: `installer/FoxBridgeAgent.wxs`**

```xml
<Product Id="*" 
         Name="FoxBridgeAgent" 
         Language="1033" 
         Version="1.0.0.0" 
         Manufacturer="YourCompany" 
         UpgradeCode="YOUR-GUID-HERE">
  
  <!-- Installer Icon -->
  <Icon Id="foxbridge.ico" SourceFile="assets\installer\installer.ico" />
  <Property Id="ARPPRODUCTICON" Value="foxbridge.ico" />
  
  <!-- Banner and Dialog -->
  <WixVariable Id="WixUIBannerBmp" Value="assets\installer\banner.bmp" />
  <WixVariable Id="WixUIDialogBmp" Value="assets\installer\dialog.bmp" />
  <WixVariable Id="WixUILicenseRtf" Value="assets\installer\license.rtf" />
  
  <!-- ... -->
</Product>
```

### 3. ใส่ใน NSIS Installer

**ไฟล์: `installer/FoxBridgeAgent.nsi`**

```nsis
; Installer Icon
!define MUI_ICON "assets\installer\installer.ico"
!define MUI_UNICON "assets\installer\installer.ico"

; Header Image
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "assets\installer\banner.bmp"
!define MUI_HEADERIMAGE_RIGHT

; Welcome/Finish Page Image
!define MUI_WELCOMEFINISHPAGE_BITMAP "assets\installer\dialog.bmp"

; Installer Settings
Icon "assets\installer\installer.ico"
UninstallIcon "assets\installer\installer.ico"

; Application Icon (for executable)
Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  File "build\bin\Release\FoxBridgeAgent.exe"
  CreateShortCut "$DESKTOP\FoxBridgeAgent.lnk" "$INSTDIR\FoxBridgeAgent.exe" "" "$INSTDIR\FoxBridgeAgent.exe" 0
SectionEnd
```

---

## ขนาดรูปภาพที่แนะนำ

### Application Icon (.ico)
```
foxbridge.ico
├─ 16x16   pixels (16-bit and 32-bit)
├─ 32x32   pixels (16-bit and 32-bit)
├─ 48x48   pixels (16-bit and 32-bit)
├─ 64x64   pixels (32-bit)
├─ 128x128 pixels (32-bit)
└─ 256x256 pixels (32-bit)
```

### Installer Banner
```
banner.bmp: 493 x 58 pixels, 96 DPI, 24-bit BMP
```

### Installer Dialog
```
dialog.bmp: 493 x 312 pixels, 96 DPI, 24-bit BMP
```

### PNG Sources (สำหรับสร้าง .ico)
```
foxbridge-16.png   (16x16)
foxbridge-32.png   (32x32)
foxbridge-48.png   (48x48)
foxbridge-64.png   (64x64)
foxbridge-128.png  (128x128)
foxbridge-256.png  (256x256)
foxbridge-512.png  (512x512) ← สำหรับ web/documentation
foxbridge-1024.png (1024x1024) ← Master file
```

---

## ตัวอย่าง Logo Design

### แนวคิด Logo สำหรับ FoxBridgeAgent

```
┌────────────────────────────────────────┐
│                                        │
│          ╔═══╗                         │
│          ║ F ║   FoxBridge             │
│          ║ B ║   Agent                 │
│          ╚═══╝                         │
│         /  │  \                        │
│       DB   │  API                      │
│      (VFP) │ (HTTP)                    │
│            ↓                           │
│        ExpressD                        │
│                                        │
└────────────────────────────────────────┘

หรือ

┌────────────────────────────────────────┐
│                                        │
│        🦊 ← →  📊                      │
│       Fox    Bridge                    │
│                                        │
│     [Database] ⟷ [API Server]        │
│                                        │
└────────────────────────────────────────┘

หรือแบบ Modern

┌────────────────────────────────────────┐
│                                        │
│         ┌─────┐                        │
│         │  F  │                        │
│     ┌───┤  B  ├───┐                   │
│     │   │  A  │   │                   │
│     │   └─────┘   │                   │
│    DBF          HTTP                  │
│                                        │
└────────────────────────────────────────┘
```

**สีแนะนำ:**
- Primary: `#2C3E50` (Dark Blue-Gray) - เสถียรภาพ, เชื่อถือได้
- Secondary: `#3498DB` (Blue) - เทคโนโลยี, API
- Accent: `#E67E22` (Orange) - ความเร็ว, Bridge
- Success: `#27AE60` (Green) - ทำงานได้ดี

---

## Tools สำหรับสร้าง Logo

### Design Tools
- **Adobe Illustrator** - Professional (มีค่าใช้จ่าย)
- **Figma** - Free, online, ใช้งานง่าย
- **Inkscape** - Free, open source
- **Canva** - Free, มี template

### Icon Conversion
- **Online:** https://www.icoconverter.com/
- **Online:** https://convertio.co/png-ico/
- **ImageMagick:** สำหรับ batch conversion
- **GIMP:** Free, รองรับ .ico

### BMP Creation (Installer Images)
- **Photoshop** - Export as 24-bit BMP
- **GIMP** - Free, Export as BMP
- **Paint.NET** - Free, Windows only

---

## Checklist การเตรียม Assets

```
□ Logo Design เสร็จแล้ว
□ สร้าง PNG หลายขนาด (16, 32, 48, 64, 128, 256)
□ Convert PNG → .ico (foxbridge.ico)
□ สร้าง installer.ico
□ สร้าง banner.bmp (493x58)
□ สร้าง dialog.bmp (493x312)
□ สร้าง license.rtf (ถ้าต้องการ)
□ เพิ่ม resources.rc ใน CMake
□ อัพเดท WiX installer references
□ อัพเดท NSIS installer references
□ ทดสอบ build
□ ตรวจสอบ icon ใน executable
□ ตรวจสอบ icon ใน installer
```

---

## ตัวอย่างการใช้งาน

### 1. วาง Logo ของคุณ

```bash
# Copy logo files
cp your-logo.png assets/icons/foxbridge-256.png
cp installer-icon.ico assets/installer/installer.ico
cp banner-image.bmp assets/installer/banner.bmp
cp dialog-image.bmp assets/installer/dialog.bmp
```

### 2. สร้าง .ico file

```bash
# ใช้ ImageMagick
cd assets/icons
magick convert foxbridge-256.png -define icon:auto-resize=256,128,64,48,32,16 foxbridge.ico
```

### 3. Build Project

```bash
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=...
cmake --build . --config Release
```

### 4. ตรวจสอบผลลัพธ์

```bash
# Check executable icon
ls -lh build/bin/Release/FoxBridgeAgent.exe

# Build installer
cd installer
candle FoxBridgeAgent.wxs
light FoxBridgeAgent.wixobj -out FoxBridgeAgent.msi

# Check installer
ls -lh FoxBridgeAgent.msi
```

---

## FAQ

**Q: ต้องเตรียม logo กี่ไฟล์?**
A: อย่างน้อย 3 ไฟล์:
- `foxbridge.ico` (application)
- `installer.ico` (installer)
- `banner.bmp` + `dialog.bmp` (installer UI)

**Q: สามารถใช้ PNG แทน .ico ได้ไหม?**
A: ไม่ได้ - Windows ต้องการ .ico สำหรับ executable icons

**Q: ขนาดไฟล์ .ico ควรเป็นเท่าไหร่?**
A: ประมาณ 50-200 KB (ขึ้นกับความซับซ้อน)

**Q: ถ้าไม่มี logo จะใช้อะไร?**
A: ใช้ default Windows icon ได้ หรือสร้าง simple icon ด้วย text "FB"

**Q: ต้องมี license.rtf ไหม?**
A: ไม่จำเป็น แต่แนะนำให้มี (สร้างจาก MIT License หรือ proprietary license)

---

## สรุป

**วาง Logo ตามนี้:**

```
FoxBridgeAgent/
└── assets/
    ├── icons/
    │   ├── foxbridge.ico         ← Application Icon (วางที่นี่!)
    │   └── foxbridge-*.png       ← PNG sources
    │
    └── installer/
        ├── installer.ico         ← Installer Icon (วางที่นี่!)
        ├── banner.bmp            ← Top banner (วางที่นี่!)
        ├── dialog.bmp            ← Side image (วางที่นี่!)
        └── license.rtf           ← License text
```

**ขนาดที่ต้องการ:**
- `.ico` files: 16x16 ถึง 256x256 (multiple sizes in one file)
- `banner.bmp`: 493 x 58 pixels
- `dialog.bmp`: 493 x 312 pixels

**จากนั้น:**
1. อัพเดท CMakeLists.txt เพื่อใช้ resources.rc
2. อัพเดท installer scripts (WiX/NSIS)
3. Build และตรวจสอบผลลัพธ์
