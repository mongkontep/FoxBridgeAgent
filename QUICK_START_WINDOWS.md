# Quick Start Guide - Windows

## 🚀 วิธีรันโปรแกรมครั้งแรก

โปรแกรม FoxBridgeAgent ต้องการไฟล์ **config.json** ก่อนรัน

---

## ขั้นตอนที่ 1: สร้างไฟล์ Config

### วิธีที่ 1: ใช้ Command Prompt (แนะนำ)

1. **เปิด Command Prompt เป็น Administrator:**
   - กด `Win + X`
   - เลือก **Command Prompt (Admin)** หรือ **Windows Terminal (Admin)**

2. **สร้าง directory:**
   ```cmd
   mkdir C:\ProgramData\FoxBridgeAgent
   cd C:\ProgramData\FoxBridgeAgent
   ```

3. **สร้างไฟล์ config.json:**
   ```cmd
   notepad config.json
   ```

4. **Copy-paste config ตัวอย่างนี้:**
   ```json
   {
     "database": {
       "type": "vfp_odbc",
       "dsn": "VFP_ExpressD",
       "connection_string": "Driver={Microsoft Visual FoxPro Driver};SourceType=DBF;SourceDB=C:\\ExpressD\\Data;Exclusive=No;",
       "dbf_path": "C:\\ExpressD\\Data"
     },
     "server": {
       "host": "0.0.0.0",
       "port": 8080,
       "api_key": "quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks="
     },
     "cloudflare": {
       "enabled": false,
       "tunnel_name": "foxbridge",
       "config_path": "C:\\ProgramData\\FoxBridgeAgent\\cloudflare.yml"
     },
     "logging": {
       "level": "info",
       "path": "C:\\ProgramData\\FoxBridgeAgent\\logs"
     },
     "maintenance": {
       "auto_reindex": true,
       "check_interval_minutes": 60,
       "backup_before_pack": true
     }
   }
   ```

5. **Save และปิด Notepad** (`Ctrl+S` แล้ว `Alt+F4`)

### วิธีที่ 2: Copy จาก Template

```cmd
cd C:\ProgramData\FoxBridgeAgent
copy "C:\Path\To\FoxBridgeAgent\config\config.json.template" config.json
notepad config.json
```

แก้ไขค่าที่จำเป็น:
- `dbf_path`: เส้นทางไปยังโฟลเดอร์ที่มีไฟล์ .dbf ของคุณ
- `api_key`: สร้างใหม่หรือใช้ค่าเดิม
- `port`: port ที่ต้องการใช้

---

## ขั้นตอนที่ 2: รันโปรแกรมแบบ Console (สำหรับทดสอบ)

### รันเพื่อดู Error Message

```cmd
cd C:\Path\To\FoxBridgeAgent
FoxBridgeAgent.exe --console
```

**ถ้ามี error จะเห็นทันที:**
- ❌ Missing config.json
- ❌ Database connection failed
- ❌ Port already in use
- ✅ Success: Server running on http://0.0.0.0:8080

### ดู Help และ Options

```cmd
FoxBridgeAgent.exe --help
```

**Output:**
```
FoxBridgeAgent - HTTP API Server for Visual FoxPro / ExpressD

Usage:
  FoxBridgeAgent.exe [options]

Options:
  --console              Run as console application (default for testing)
  --service              Run as Windows Service (default for production)
  --install              Install as Windows Service
  --uninstall            Uninstall Windows Service
  --start                Start Windows Service
  --stop                 Stop Windows Service
  --config <path>        Specify config file path
  --help                 Show this help message

Example:
  FoxBridgeAgent.exe --console
  FoxBridgeAgent.exe --install
  FoxBridgeAgent.exe --service
```

---

## ขั้นตอนที่ 3: ติดตั้งเป็น Windows Service

### 1. Install Service

```cmd
FoxBridgeAgent.exe --install
```

**Output:**
```
Service installed successfully
```

### 2. Start Service

```cmd
FoxBridgeAgent.exe --start
```

หรือใช้ Windows Services Manager:
```cmd
services.msc
```
- หา **FoxBridge Agent - ExpressD API Server**
- Right-click → **Start**

### 3. ตรวจสอบ Status

```cmd
sc query FoxBridgeAgent
```

**Output ถ้ารันสำเร็จ:**
```
SERVICE_NAME: FoxBridgeAgent
        TYPE               : 10  WIN32_OWN_PROCESS
        STATE              : 4  RUNNING
        WIN32_EXIT_CODE    : 0  (0x0)
```

---

## 📝 Troubleshooting

### ปัญหา 1: โปรแกรมปิดทันที

**สาเหตุ:**
- ไม่มีไฟล์ config.json
- รันโดยไม่ใส่ `--console` จะพยายามรันเป็น Service แล้วปิดทันที

**แก้ไข:**
```cmd
# ตรวจสอบว่ามี config.json หรือไม่
dir C:\ProgramData\FoxBridgeAgent\config.json

# รันแบบ console เพื่อดู error
FoxBridgeAgent.exe --console
```

### ปัญหา 2: Configuration error

**Error:**
```
Configuration error: Failed to open config file
Please check: C:\ProgramData\FoxBridgeAgent\config.json
```

**แก้ไข:**
1. ตรวจสอบว่าไฟล์มีอยู่จริง
2. ตรวจสอบ JSON format ถูกต้อง (ใช้ https://jsonlint.com/)
3. ตรวจสอบ permissions ของโฟลเดอร์

### ปัญหา 3: Database connection failed

**Error:**
```
Failed to connect to database
ODBC Error: [Microsoft][ODBC Driver Manager] Data source name not found...
```

**แก้ไข:**
1. ติดตั้ง VFP ODBC Driver:
   ```cmd
   # Download: https://www.microsoft.com/en-us/download/details.aspx?id=14839
   ```

2. ตรวจสอบ DSN:
   ```cmd
   # เปิด ODBC Data Source Administrator
   odbcad32.exe
   ```

3. แก้ไข connection_string ใน config.json:
   ```json
   "connection_string": "Driver={Microsoft Visual FoxPro Driver};SourceType=DBF;SourceDB=C:\\YourPath\\Data;Exclusive=No;"
   ```

### ปัญหา 4: Port already in use

**Error:**
```
Failed to bind to port 8080: Address already in use
```

**แก้ไข:**
1. ตรวจสอบ port ที่ถูกใช้:
   ```cmd
   netstat -ano | findstr :8080
   ```

2. เปลี่ยน port ใน config.json:
   ```json
   "server": {
     "port": 8081
   }
   ```

### ปัญหา 5: Access Denied (Service)

**Error:**
```
Failed to install service
Access is denied
```

**แก้ไข:**
- เปิด Command Prompt เป็น **Administrator**
- กด `Win + X` → **Command Prompt (Admin)**

---

## 🧪 ทดสอบการทำงาน

### 1. Health Check

```cmd
curl http://localhost:8080/health
```

**Expected response:**
```json
{
  "status": "ok",
  "version": "1.0.0",
  "database": "connected"
}
```

### 2. API Test

```cmd
curl -H "X-API-Key: quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=" http://localhost:8080/api/json/ALL/ExpressD
```

### 3. ดู Logs

```cmd
type C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log
```

หรือใช้ Notepad:
```cmd
notepad C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log
```

---

## 🔧 จัดการ Service

### ดู Status
```cmd
sc query FoxBridgeAgent
```

### Start Service
```cmd
FoxBridgeAgent.exe --start
# หรือ
net start FoxBridgeAgent
```

### Stop Service
```cmd
FoxBridgeAgent.exe --stop
# หรือ
net stop FoxBridgeAgent
```

### Restart Service
```cmd
net stop FoxBridgeAgent && net start FoxBridgeAgent
```

### Uninstall Service
```cmd
FoxBridgeAgent.exe --uninstall
```

---

## 🎯 แนะนำ Workflow

### สำหรับ Development/Testing:
```cmd
# 1. สร้าง config
mkdir C:\ProgramData\FoxBridgeAgent
notepad C:\ProgramData\FoxBridgeAgent\config.json

# 2. รัน console mode
FoxBridgeAgent.exe --console

# 3. ทดสอบ API
curl http://localhost:8080/health

# 4. กด Ctrl+C เพื่อหยุด
```

### สำหรับ Production:
```cmd
# 1. ติดตั้ง Service
FoxBridgeAgent.exe --install

# 2. Start Service
FoxBridgeAgent.exe --start

# 3. ตรวจสอบ
sc query FoxBridgeAgent

# 4. ทดสอบ
curl http://localhost:8080/health
```

---

## 📚 เอกสารเพิ่มเติม

- [Configuration Guide](config/CONFIG.md)
- [API Documentation](docs/API.md)
- [Security Guide](docs/SECURITY.md)
- [Operations Guide](docs/OPERATIONS.md)

---

## 🆘 Support

หากยังมีปัญหา:

1. **ดู Logs:**
   ```cmd
   type C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log
   ```

2. **ตรวจสอบ Event Viewer:**
   ```cmd
   eventvwr.msc
   ```
   ไปที่: Windows Logs → Application

3. **รันแบบ Debug:**
   ```cmd
   FoxBridgeAgent.exe --console
   ```
   จะเห็น error messages โดยตรง

---

**สรุป: เปิด exe แล้วปิดทันทีเพราะไม่มี config.json!**

```cmd
# Fix ง่ายๆ:
mkdir C:\ProgramData\FoxBridgeAgent
notepad C:\ProgramData\FoxBridgeAgent\config.json
# (copy config จากด้านบน)
FoxBridgeAgent.exe --console
```

✅ แก้แล้วควรรันได้! 🎉
