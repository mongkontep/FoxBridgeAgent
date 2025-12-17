# Cloudflare Tunnel Setup Guide

คู่มือการตั้งค่า Cloudflare Tunnel สำหรับ FoxBridgeAgent แบบละเอียด (ภาษาไทย)

## ภาพรวม

```
┌─────────────────────────────────────────────────────────────┐
│ ลำดับการทำงาน                                               │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  1. ติดตั้ง FoxBridgeAgent                                  │
│     └─ เลือก database_path: D:\ExpressD\Data              │
│                                                              │
│  2. คิด domain/subdomain ที่ต้องการ                        │
│     └─ เช่น: hok.pkid.io, api.company.com                 │
│                                                              │
│  3. ติดตั้ง cloudflared                                     │
│     └─ ดาวน์โหลดและติดตั้งบน Windows                       │
│                                                              │
│  4. Login Cloudflare                                         │
│     └─ cloudflared tunnel login                             │
│                                                              │
│  5. สร้าง Tunnel                                            │
│     └─ cloudflared tunnel create foxbridge                  │
│     └─ ได้ Token มา                                        │
│                                                              │
│  6. เชื่อม Domain                                           │
│     └─ cloudflared tunnel route dns foxbridge hok.pkid.io  │
│                                                              │
│  7. ใส่ Token และ URL ใน FoxBridgeAgent config             │
│     └─ C:\ProgramData\FoxBridgeAgent\config.json           │
│                                                              │
│  8. เริ่มใช้งาน                                             │
│     └─ curl https://hok.pkid.io/health                     │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## ขั้นตอนที่ 1: ติดตั้ง FoxBridgeAgent

```powershell
# รัน Installer
.\FoxBridgeAgent-Setup.exe

# กรอกข้อมูล:
# - Database Path: D:\ExpressD\Data       ← เลือกตาม ExpressD ของคุณ
# - API Key: (ปล่อยว่างให้ auto-generate หรือใส่ของคุณ)
# - Port: 8787
```

**ผลลัพธ์:**
- ✅ FoxBridgeAgent ติดตั้งเป็น Windows Service
- ✅ Config file สร้างที่ `C:\ProgramData\FoxBridgeAgent\config.json`
- ✅ Service กำลังทำงานบน `http://127.0.0.1:8787`

**ทดสอบ:**
```powershell
curl http://127.0.0.1:8787/health
```

## ขั้นตอนที่ 2: เลือก Domain/Subdomain

คิดก่อนว่าต้องการใช้ URL อะไร:

### ตัวอย่าง Domain/Subdomain

```
# ถ้ามี domain pkid.io อยู่แล้ว
hok.pkid.io
api.pkid.io
foxbridge.pkid.io

# ถ้ามี domain บริษัท
api.yourcompany.com
foxbridge.yourcompany.com
expressd.yourcompany.com

# ถ้ามี subdomain เฉพาะแผนก
hok.accounting.company.com
api.it.company.com
```

**ในคู่มือนี้จะใช้ตัวอย่าง:** `hok.pkid.io`

⚠️ **หมายเหตุ:**
- Domain ต้องอยู่ใน Cloudflare (ย้าย nameserver มาที่ Cloudflare ก่อน)
- ถ้ายังไม่มี domain สามารถซื้อจาก Cloudflare Registrar หรือที่อื่นก็ได้

## ขั้นตอนที่ 3: ติดตั้ง cloudflared

### Download และติดตั้ง

```powershell
# วิธีที่ 1: ใช้ winget (แนะนำ)
winget install Cloudflare.cloudflared

# วิธีที่ 2: ดาวน์โหลดเอง
# ไปที่: https://github.com/cloudflare/cloudflared/releases
# ดาวน์โหลด: cloudflared-windows-amd64.exe
# เปลี่ยนชื่อเป็น: cloudflared.exe
# วางไว้ที่: C:\Windows\System32\

# ตรวจสอบ
cloudflared --version
```

**ผลลัพธ์:**
```
cloudflared version 2024.12.1 (built 2024-12-01...)
```

## ขั้นตอนที่ 4: Login Cloudflare

```powershell
cloudflared tunnel login
```

**สิ่งที่เกิดขึ้น:**
1. Browser เปิดขึ้นมา
2. Login Cloudflare account
3. เลือก domain ที่ต้องการใช้ (เช่น pkid.io)
4. คลิก "Authorize"

**ผลลัพธ์:**
```
You have successfully logged in.
Credentials saved to: C:\Users\YourUser\.cloudflared\cert.pem
```

## ขั้นตอนที่ 5: สร้าง Tunnel

```powershell
# สร้าง tunnel ชื่อ "foxbridge"
cloudflared tunnel create foxbridge
```

**ผลลัพธ์:**
```
Tunnel credentials written to: C:\Users\YourUser\.cloudflared\abc123...json
Created tunnel foxbridge with id abc123-def456-789...

{"AccountTag":"your-account-id","TunnelSecret":"your-tunnel-secret","TunnelID":"abc123-def456..."}
```

⚠️ **สำคัญ:**
- เก็บ Tunnel ID ไว้: `abc123-def456-789...`
- ไฟล์ credentials: `C:\Users\YourUser\.cloudflared\abc123...json`

## ขั้นตอนที่ 6: เชื่อม Domain

```powershell
# เชื่อม tunnel กับ domain hok.pkid.io
cloudflared tunnel route dns foxbridge hok.pkid.io
```

**ผลลัพธ์:**
```
Created DNS CNAME record for hok.pkid.io -> abc123....cfargotunnel.com
```

**ตรวจสอบใน Cloudflare Dashboard:**
1. เข้า https://dash.cloudflare.com
2. เลือก domain pkid.io
3. ไปที่ DNS Records
4. จะเห็น CNAME record:
   ```
   hok  CNAME  abc123-def456.cfargotunnel.com
   ```

## ขั้นตอนที่ 7: สร้าง Config File

```powershell
# สร้างโฟลเดอร์ config
New-Item -ItemType Directory -Force -Path C:\Users\$env:USERNAME\.cloudflared
```

สร้างไฟล์ `C:\Users\YourUser\.cloudflared\config.yml`:

```yaml
tunnel: abc123-def456-789...              # ← Tunnel ID จากขั้นตอนที่ 5
credentials-file: C:\Users\YourUser\.cloudflared\abc123....json

ingress:
  # Route hok.pkid.io -> localhost:8787
  - hostname: hok.pkid.io
    service: http://127.0.0.1:8787
  
  # Catch-all rule (required)
  - service: http_status:404
```

**ตัวอย่างเต็ม:**
```yaml
tunnel: abc123-def456-789ghi
credentials-file: C:\Users\Admin\.cloudflared\abc123-def456-789ghi.json

ingress:
  - hostname: hok.pkid.io
    service: http://127.0.0.1:8787
    originRequest:
      connectTimeout: 30s
      noTLSVerify: false
  
  - service: http_status:404

# Optional: Logging
# logfile: C:\ProgramData\cloudflared\logs\tunnel.log
# loglevel: info
```

## ขั้นตอนที่ 8: ทดสอบ Tunnel (ก่อนติดตั้งเป็น Service)

```powershell
# รัน tunnel แบบ interactive
cloudflared tunnel run foxbridge
```

**ผลลัพธ์:**
```
2024-12-17T10:30:00Z INF Starting tunnel tunnelID=abc123...
2024-12-17T10:30:01Z INF Connection registered connIndex=0 location=BKK
2024-12-17T10:30:02Z INF Connection registered connIndex=1 location=SIN
2024-12-17T10:30:03Z INF Connection registered connIndex=2 location=HKG
2024-12-17T10:30:04Z INF Connection registered connIndex=3 location=TPE
```

**ทดสอบจากเครื่องอื่น:**
```bash
curl https://hok.pkid.io/health
```

**ควรได้:**
```json
{
  "status": "success",
  "msg": "FoxBridgeAgent is running",
  "data": {
    "version": "1.0.0",
    "database_connected": true,
    "timestamp": 1702998765
  },
  "index": "ok",
  "warnings": []
}
```

✅ ถ้าได้แบบนี้แสดงว่าใช้งานได้แล้ว!

## ขั้นตอนที่ 9: ติดตั้ง Tunnel เป็น Windows Service

```powershell
# ติดตั้งเป็น service (รัน PowerShell as Administrator)
cloudflared service install
```

**ผลลัพธ์:**
```
Service cloudflared installed successfully.
```

**จัดการ Service:**
```powershell
# เริ่ม service
sc start cloudflared
# หรือ
Start-Service cloudflared

# ตรวจสอบสถานะ
sc query cloudflared
# หรือ
Get-Service cloudflared

# หยุด service
sc stop cloudflared

# Restart service
Restart-Service cloudflared

# ลบ service (ถ้าต้องการ)
cloudflared service uninstall
```

## ขั้นตอนที่ 10: อัพเดท FoxBridgeAgent Config

แก้ไข `C:\ProgramData\FoxBridgeAgent\config.json`:

```json
{
  "database_path": "D:\\ExpressD\\Data",
  "api_key": "quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=",
  "port": 8787,
  "host": "127.0.0.1",
  "cloudflare_token": "abc123-def456-789ghi",
  "cloudflare_public_url": "https://hok.pkid.io",
  "log_level": "info",
  "log_path": "C:\\ProgramData\\FoxBridgeAgent\\logs",
  "index_policy": "auto",
  "maintenance_window": "02:00-04:00",
  "max_retry_attempts": 3,
  "connection_timeout": 30
}
```

**Restart FoxBridgeAgent:**
```powershell
Restart-Service FoxBridgeAgent
```

## ขั้นตอนที่ 11: ทดสอบแบบเต็มรูปแบบ

### ทดสอบจาก Local (บน Server)

```bash
export API_KEY="quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks="

# Local access
curl http://127.0.0.1:8787/health

curl -H "X-API-Key: ${API_KEY}" \
  http://127.0.0.1:8787/api/dbf/json/customers.dbf
```

### ทดสอบจาก Remote (จากที่อื่น)

```bash
export API_KEY="quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks="

# Remote access via Cloudflare
curl https://hok.pkid.io/health

curl -H "X-API-Key: ${API_KEY}" \
  https://hok.pkid.io/api/dbf/json/customers.dbf
```

### ทดสอบจาก Web Browser

เปิด browser ไปที่:
```
https://hok.pkid.io/health
```

ควรเห็น JSON response:
```json
{
  "status": "success",
  "msg": "FoxBridgeAgent is running",
  ...
}
```

## การแก้ไขปัญหา (Troubleshooting)

### ปัญหา 1: "tunnel not found"

```powershell
# ดู tunnel ทั้งหมด
cloudflared tunnel list

# Output:
# ID                                   NAME       CREATED
# abc123-def456-789ghi                 foxbridge  2024-12-17T10:30:00Z
```

### ปัญหา 2: "connection refused"

```powershell
# ตรวจสอบ FoxBridgeAgent ทำงานไหม
Get-Service FoxBridgeAgent

# ตรวจสอบ port 8787
netstat -ano | findstr :8787

# Test localhost
curl http://127.0.0.1:8787/health
```

### ปัญหา 3: "certificate error"

```powershell
# Login ใหม่
cloudflared tunnel login

# ลบ cert เก่า
Remove-Item C:\Users\$env:USERNAME\.cloudflared\cert.pem
```

### ปัญหา 4: DNS ไม่ resolve

```powershell
# ตรวจสอบ DNS record
nslookup hok.pkid.io

# ตรวจสอบใน Cloudflare Dashboard
# https://dash.cloudflare.com -> pkid.io -> DNS
```

### ปัญหา 5: "502 Bad Gateway"

- FoxBridgeAgent ไม่ทำงาน → ตรวจสอบ service
- Port ผิด → ตรวจสอบ config.yml
- Localhost ผิด → ตรวจสอบ service: http://127.0.0.1:8787

### ปัญหา 6: "404 Not Found"

- ตรวจสอบ ingress ใน config.yml
- ตรวจสอบ hostname ตรงกันไหม

### ดู Logs

```powershell
# FoxBridgeAgent logs
Get-Content C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log -Tail 50

# Cloudflare tunnel logs (ถ้าตั้งค่าไว้)
Get-Content C:\ProgramData\cloudflared\logs\tunnel.log -Tail 50

# Windows Event Viewer
eventvwr.msc
# -> Applications and Services Logs -> FoxBridgeAgent
```

## Quick Tunnel (สำหรับทดสอบ)

ถ้าต้องการทดสอบเร็วๆ โดยไม่ต้องตั้ง domain:

```powershell
cloudflared tunnel --url http://127.0.0.1:8787
```

**ผลลัพธ์:**
```
Your quick Tunnel has been created! Visit it at:
https://funny-words-1234.trycloudflare.com
```

**ใช้ URL นี้ทดสอบได้เลย:**
```bash
curl https://funny-words-1234.trycloudflare.com/health
```

⚠️ **ข้อจำกัด:**
- URL จะเปลี่ยนทุกครั้งที่ restart
- ไม่เหมาะสำหรับ production
- ใช้สำหรับทดสอบเท่านั้น

## สรุป

### Configuration สุดท้าย

**FoxBridgeAgent Config** (`C:\ProgramData\FoxBridgeAgent\config.json`):
```json
{
  "database_path": "D:\\ExpressD\\Data",
  "api_key": "quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=",
  "port": 8787,
  "host": "127.0.0.1",
  "cloudflare_token": "abc123-def456-789ghi",
  "cloudflare_public_url": "https://hok.pkid.io",
  "log_level": "info"
}
```

**Cloudflare Tunnel Config** (`C:\Users\YourUser\.cloudflared\config.yml`):
```yaml
tunnel: abc123-def456-789ghi
credentials-file: C:\Users\YourUser\.cloudflared\abc123-def456-789ghi.json

ingress:
  - hostname: hok.pkid.io
    service: http://127.0.0.1:8787
  - service: http_status:404
```

### Access URLs

- **Local**: `http://127.0.0.1:8787`
- **Remote**: `https://hok.pkid.io`
- **API Key**: `quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=`

### คำสั่งที่ใช้บ่อย

```bash
# Local
curl -H "X-API-Key: quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=" \
  http://127.0.0.1:8787/api/dbf/json/customers.dbf

# Remote
curl -H "X-API-Key: quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=" \
  https://hok.pkid.io/api/dbf/json/customers.dbf
```

## เสร็จแล้ว! 🎉

ตอนนี้คุณมี:
- ✅ FoxBridgeAgent ทำงานบน localhost:8787
- ✅ Cloudflare Tunnel เชื่อมต่อ
- ✅ Domain hok.pkid.io ใช้งานได้
- ✅ HTTPS encryption ฟรีจาก Cloudflare
- ✅ DDoS protection
- ✅ เข้าถึงได้จากทุกที่ในโลก

**Next Steps:**
- เปลี่ยน API key ใหม่สำหรับ production
- ตั้ง rate limiting ใน Cloudflare
- Enable WAF rules
- Setup monitoring และ alerts
