# การตั้งค่า Auto-Start สำหรับ FoxBridgeAgent

## สรุปการทำงาน

FoxBridgeAgent ถูกออกแบบให้ **รันอัตโนมัติเมื่อเปิดเครื่อง/รีสตาร์ท** และ **รีสตาร์ทอัตโนมัติเมื่อเกิดข้อผิดพลาด**

---

## 🚀 Auto-Start Configuration

### 1. Windows Service Auto-Start

เมื่อติดตั้ง FoxBridgeAgent เสร็จแล้ว โปรแกรมจะถูกติดตั้งเป็น **Windows Service** พร้อมการตั้งค่าดังนี้:

```
Service Name:    FoxBridgeAgent
Display Name:    FoxBridge Agent - ExpressD API Server
Startup Type:    Automatic (Delayed Start)
Account:         LocalSystem
Status:          Running
```

### คุณสมบัติการทำงาน:

✅ **เปิดเครื่อง → รันอัตโนมัติ**
- Service จะเริ่มทำงานทันทีหลังจาก Windows บูตเสร็จ
- ใช้ Delayed Auto-Start เพื่อไม่ให้รบกวน critical services

✅ **รีสตาร์ทเครื่อง → รันอัตโนมัติ**  
- Service จะเริ่มทำงานอีกครั้งหลังจากรีบูตระบบ
- ไม่ต้องเข้าไป start ด้วยตัวเอง

✅ **โปรแกรมแครช → รีสตาร์ทอัตโนมัติ**
- ครั้งที่ 1: รีสตาร์ทหลัง 60 วินาที
- ครั้งที่ 2: รีสตาร์ทหลัง 2 นาที
- ครั้งที่ 3+: รีสตาร์ทหลัง 5 นาที
- Reset failure counter หลังจาก 24 ชั่วโมง

---

## 📋 ตรวจสอบสถานะ Service

### ผ่าน Windows Services Manager

```powershell
# เปิด Services Manager
services.msc
```

**ค้นหา:** FoxBridge Agent - ExpressD API Server

**ตรวจสอบ:**
- Startup Type: **Automatic (Delayed Start)** ✅
- Status: **Running** ✅
- Recovery: **Restart the service** on failure ✅

### ผ่าน PowerShell

```powershell
# ตรวจสอบสถานะ service
Get-Service FoxBridgeAgent

# ดูข้อมูลแบบละเอียด
Get-Service FoxBridgeAgent | Format-List *

# ตรวจสอบ startup type
Get-WmiObject Win32_Service -Filter "Name='FoxBridgeAgent'" | Select-Object Name, StartMode, State

# ผลลัพธ์ที่คาดหวัง:
# Name            : FoxBridgeAgent
# StartMode       : Auto
# State           : Running
```

### ผ่าน Command Prompt (sc.exe)

```cmd
# ตรวจสอบ configuration
sc qc FoxBridgeAgent

# ผลลัพธ์:
# SERVICE_NAME: FoxBridgeAgent
#         TYPE               : 10  WIN32_OWN_PROCESS  
#         START_TYPE         : 2   AUTO_START
#         ERROR_CONTROL      : 1   NORMAL
#         BINARY_PATH_NAME   : "C:\Program Files\FoxBridgeAgent\bin\FoxBridgeAgent.exe" --service
#         ...

# ตรวจสอบ recovery settings
sc qfailure FoxBridgeAgent

# ผลลัพธ์:
# [SC] QueryServiceConfig2 SUCCESS
# SERVICE_NAME: FoxBridgeAgent
#         RESET_PERIOD (in seconds)    : 86400
#         REBOOT_MESSAGE               : 
#         COMMAND_LINE                 : 
#         FAILURE_ACTIONS              : RESTART -- Delay = 60000 milliseconds.
#                                        RESTART -- Delay = 120000 milliseconds.
#                                        RESTART -- Delay = 300000 milliseconds.
```

---

## 🔧 การจัดการ Service

### Start/Stop/Restart

**ผ่าน PowerShell (แนะนำ):**
```powershell
# Start
Start-Service FoxBridgeAgent

# Stop
Stop-Service FoxBridgeAgent

# Restart
Restart-Service FoxBridgeAgent

# ตรวจสอบสถานะ
Get-Service FoxBridgeAgent
```

**ผ่าน Command Prompt:**
```cmd
# Start
net start FoxBridgeAgent
# หรือ
sc start FoxBridgeAgent

# Stop
net stop FoxBridgeAgent
# หรือ
sc stop FoxBridgeAgent
```

**ผ่าน Executable:**
```cmd
# Start
"C:\Program Files\FoxBridgeAgent\bin\FoxBridgeAgent.exe" --start

# Stop
"C:\Program Files\FoxBridgeAgent\bin\FoxBridgeAgent.exe" --stop
```

### Enable/Disable Auto-Start

**ปิด Auto-Start (ไม่แนะนำ):**
```powershell
Set-Service FoxBridgeAgent -StartupType Manual
```

**เปิด Auto-Start อีกครั้ง:**
```powershell
Set-Service FoxBridgeAgent -StartupType Automatic
```

**ตั้งเป็น Delayed Start:**
```cmd
sc config FoxBridgeAgent start= delayed-auto
```

---

## 🛡️ Service Recovery Configuration

### Recovery Actions (ตั้งค่าอัตโนมัติตอนติดตั้ง)

| ครั้งที่ล้มเหลว | การกระทำ | หน่วง (Delay) |
|----------------|----------|---------------|
| ครั้งที่ 1 | Restart Service | 60 วินาที |
| ครั้งที่ 2 | Restart Service | 2 นาที |
| ครั้งที่ 3+ | Restart Service | 5 นาที |

**Reset Failure Counter:** 24 ชั่วโมง

### ปรับแต่ง Recovery (ถ้าต้องการ)

```cmd
# เปลี่ยนเวลา delay
sc failure FoxBridgeAgent reset= 86400 actions= restart/30000/restart/60000/restart/120000

# อธิบาย:
# reset= 86400        = Reset counter หลัง 24 ชั่วโมง (86400 วินาที)
# restart/30000       = Restart หลัง 30 วินาที (ครั้งที่ 1)
# restart/60000       = Restart หลัง 60 วินาที (ครั้งที่ 2)
# restart/120000      = Restart หลัง 2 นาที (ครั้งที่ 3+)
```

**หมายเหตุ:** ต้องรัน Command Prompt as Administrator

---

## 📊 Event Viewer Monitoring

### ดู Service Events

```powershell
# เปิด Event Viewer
eventvwr.msc
```

**Path:**
```
Event Viewer (Local)
├── Windows Logs
│   ├── Application          ← FoxBridgeAgent logs
│   └── System              ← Service control events
└── Custom Views
    └── Administrative Events
```

**ค้นหา Events:**
- **Source:** FoxBridgeAgent
- **Event ID:**
  - `1000` = Service Started
  - `1001` = Service Stopped
  - `1002` = Service Error

**ผ่าน PowerShell:**
```powershell
# ดู service events ล่าสุด
Get-EventLog -LogName System -Source "Service Control Manager" -Newest 20 | 
  Where-Object {$_.Message -like "*FoxBridgeAgent*"} |
  Format-Table TimeGenerated, EntryType, Message -AutoSize

# ดู application errors
Get-EventLog -LogName Application -Source "FoxBridgeAgent" -Newest 20 -EntryType Error
```

---

## 🔍 Troubleshooting

### Service ไม่ Start อัตโนมัติ

**1. ตรวจสอบ Startup Type:**
```powershell
Get-Service FoxBridgeAgent | Select-Object Name, StartType, Status

# ควรเป็น:
# Name            StartType  Status
# ----            ---------  ------
# FoxBridgeAgent  Automatic  Running
```

**แก้ไข:**
```powershell
Set-Service FoxBridgeAgent -StartupType Automatic
```

**2. ตรวจสอบ Dependencies:**
```cmd
sc qc FoxBridgeAgent

# ดู DEPENDENCIES
# ถ้ามี dependencies ต้องแน่ใจว่า services เหล่านั้น start ก่อน
```

**3. ตรวจสอบ Permissions:**
```powershell
# Service ควรรันด้วย LocalSystem account
Get-WmiObject Win32_Service -Filter "Name='FoxBridgeAgent'" | Select-Object Name, StartName

# ผลลัพธ์:
# Name            StartName
# ----            ---------
# FoxBridgeAgent  LocalSystem
```

**4. ตรวจสอบ Executable Path:**
```cmd
sc qc FoxBridgeAgent

# BINARY_PATH_NAME ควรถูกต้อง
# "C:\Program Files\FoxBridgeAgent\bin\FoxBridgeAgent.exe" --service
```

**5. ตรวจสอบ Logs:**
```
C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log
```

### Service Start แล้ว Stop ทันที

**สาเหตุที่พบบ่อย:**
1. **Config file ผิดพลาด** - ตรวจสอบ `config.json`
2. **Database path ไม่ถูกต้อง** - ตรวจสอบว่าเข้าถึง DBF folder ได้
3. **Port 8787 ถูกใช้งาน** - ปิดโปรแกรมที่ใช้ port นี้
4. **VFP ODBC Driver ไม่มี** - ติดตั้ง VFP ODBC Driver

**วิธีแก้:**
```powershell
# 1. ทดสอบรันแบบ console mode
cd "C:\Program Files\FoxBridgeAgent\bin"
.\FoxBridgeAgent.exe --console

# จะเห็น error messages ชัดเจน

# 2. ตรวจสอบ config
Get-Content "C:\ProgramData\FoxBridgeAgent\config.json"

# 3. ตรวจสอบ port
netstat -ano | findstr :8787

# 4. ตรวจสอบ ODBC
Get-OdbcDriver -Name "*Visual FoxPro*"
```

### Delayed Start ช้าเกินไป

**ลด Delay Time:**
```cmd
# เปลี่ยนเป็น Auto-Start ปกติ (ไม่ delay)
sc config FoxBridgeAgent start= auto

# Restart service
sc stop FoxBridgeAgent
sc start FoxBridgeAgent
```

---

## 📖 Manual Installation (ถ้าไม่ใช้ installer)

### Install Service ด้วยตัวเอง

```cmd
# 1. Copy FoxBridgeAgent.exe ไปที่
C:\Program Files\FoxBridgeAgent\bin\

# 2. Install service
"C:\Program Files\FoxBridgeAgent\bin\FoxBridgeAgent.exe" --install

# 3. Configure recovery
sc failure FoxBridgeAgent reset= 86400 actions= restart/60000/restart/120000/restart/300000

# 4. Configure delayed start
sc config FoxBridgeAgent start= delayed-auto

# 5. Set description
sc description FoxBridgeAgent "HTTP API Server for Visual FoxPro ExpressD database. Provides REST API access to DBF files with multi-user safety. Automatically starts on system boot."

# 6. Start service
"C:\Program Files\FoxBridgeAgent\bin\FoxBridgeAgent.exe" --start

# 7. Verify
sc query FoxBridgeAgent
```

### Uninstall Service

```cmd
# 1. Stop service
"C:\Program Files\FoxBridgeAgent\bin\FoxBridgeAgent.exe" --stop

# 2. Uninstall service
"C:\Program Files\FoxBridgeAgent\bin\FoxBridgeAgent.exe" --uninstall

# 3. Delete files
rmdir /s "C:\Program Files\FoxBridgeAgent"
```

---

## 🎯 Best Practices

### 1. ใช้ Delayed Auto-Start

**ข้อดี:**
- ไม่แย่งทรัพยากรกับ critical services ตอนบูต
- ระบบบูตเร็วขึ้น
- ลดโอกาส service dependencies issues

**การตั้งค่า:**
```cmd
sc config FoxBridgeAgent start= delayed-auto
```

### 2. Monitor Service Health

**สร้าง Scheduled Task เพื่อตรวจสอบ:**
```powershell
# สคริปต์ตรวจสอบทุก 5 นาที
$service = Get-Service FoxBridgeAgent -ErrorAction SilentlyContinue

if ($service.Status -ne 'Running') {
    # ส่งอีเมล หรือ log
    Write-EventLog -LogName Application -Source FoxBridgeAgent -EventId 9999 -EntryType Warning -Message "FoxBridgeAgent is not running!"
    
    # พยายาม start service
    Start-Service FoxBridgeAgent
}
```

### 3. Regular Log Rotation

**FoxBridgeAgent ใช้ rotating logs อัตโนมัติ:**
- ไฟล์ log: `foxbridge.log`
- ขนาดสูงสุด: 10 MB ต่อไฟล์
- เก็บไฟล์: 5 ไฟล์
- Path: `C:\ProgramData\FoxBridgeAgent\logs\`

### 4. Backup Configuration

**สำรองไฟล์ config:**
```powershell
# Backup
Copy-Item "C:\ProgramData\FoxBridgeAgent\config.json" "C:\Backups\foxbridge-config-$(Get-Date -Format 'yyyyMMdd').json"

# Restore
Copy-Item "C:\Backups\foxbridge-config-20250101.json" "C:\ProgramData\FoxBridgeAgent\config.json"
Restart-Service FoxBridgeAgent
```

### 5. Test After Windows Updates

หลังจาก Windows Update ให้ตรวจสอบ:
```powershell
# ตรวจสอบ service status
Get-Service FoxBridgeAgent

# ตรวจสอบ startup type
Get-WmiObject Win32_Service -Filter "Name='FoxBridgeAgent'" | Select-Object StartMode

# ทดสอบ API
Invoke-RestMethod -Uri "http://localhost:8787/health" -Headers @{"X-API-Key"="YOUR_API_KEY"}
```

---

## 🔐 Security Considerations

### LocalSystem Account

**FoxBridgeAgent รันด้วย LocalSystem:**

**ข้อดี:**
- มีสิทธิ์เข้าถึง DBF files
- ไม่ต้องตั้ง password
- รันอัตโนมัติได้โดยไม่ต้อง login

**ข้อควรระวัง:**
- LocalSystem มีสิทธิ์สูง
- ใช้ localhost binding (127.0.0.1) เท่านั้น
- ใช้ API Key authentication
- ใช้ Cloudflare Tunnel สำหรับ external access

### ปรับเปลี่ยน Service Account (ถ้าต้องการ)

```cmd
# เปลี่ยนเป็น specific user account
sc config FoxBridgeAgent obj= "DOMAIN\Username" password= "Password"

# หรือใช้ Network Service
sc config FoxBridgeAgent obj= "NT AUTHORITY\NetworkService"
```

**หมายเหตุ:** ต้องแน่ใจว่า account มีสิทธิ์:
- Read/Write DBF files
- Start as a service (Log on as a service right)

---

## 📞 Support

### ตรวจสอบปัญหา Auto-Start

**Checklist:**
```
□ Service Startup Type = Automatic
□ Service Status = Running
□ Recovery configured = Yes (3 restart actions)
□ Executable path ถูกต้อง
□ Config file ถูกต้อง
□ Port 8787 available
□ VFP ODBC Driver installed
□ Logs ไม่มี errors
□ Event Viewer ไม่มี critical errors
```

### ขอความช่วยเหลือ

**ข้อมูลที่ต้องเตรียม:**
1. Service status: `sc query FoxBridgeAgent`
2. Service config: `sc qc FoxBridgeAgent`
3. Recovery settings: `sc qfailure FoxBridgeAgent`
4. Recent logs: `C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log`
5. Event Viewer errors: System และ Application logs

---

## สรุป

✅ **FoxBridgeAgent รันอัตโนมัติเมื่อเปิดเครื่อง/รีสตาร์ท**
- Configured as Windows Service with AUTO_START
- Uses Delayed Start for better boot performance

✅ **รีสตาร์ทอัตโนมัติเมื่อเกิดข้อผิดพลาด**
- 3 recovery actions with increasing delays
- Automatic failure counter reset after 24 hours

✅ **ง่ายต่อการจัดการ**
- ใช้ PowerShell, services.msc, หรือ sc.exe
- Logs และ Event Viewer สำหรับ monitoring
- Configuration backup และ restore ง่าย

**ไม่ต้องกังวล!** หลังจากติดตั้งแล้ว FoxBridgeAgent จะดูแลตัวเองอัตโนมัติ 🎉
