# FoxBridgeAgent - ExpressD Operational Notes

## ⚠️ CRITICAL: ExpressD Safety Rules

### The Golden Rules

1. **NEVER close ExpressD to use FoxBridgeAgent**
   - Designed for concurrent multi-user access
   - VFP ODBC Driver handles shared file access
   - Both can run simultaneously

2. **NEVER force exclusive file access**
   - Connection string: `Exclusive=No`
   - Let VFP ODBC manage locks
   - Graceful degradation on lock failures

3. **NEVER reindex while ExpressD is running**
   - Reindex requires exclusive access
   - Schedule during maintenance window
   - Close ExpressD before maintenance

4. **NEVER bypass VFP ODBC for writes**
   - Always use ODBC/ADO
   - Automatic index maintenance
   - Proper transaction handling

---

## Architecture Overview

```
┌─────────────────┐         ┌──────────────────┐
│  External API   │         │   Cloudflare     │
│   Consumers     │◄────────┤     Tunnel       │
└─────────────────┘         └──────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────┐
│         FoxBridgeAgent (Windows Service)     │
│  ┌─────────────┐  ┌──────────────────────┐ │
│  │ HTTP Server │  │  Cloudflare Watchdog │ │
│  │ (Boost)     │  │  (Auto-restart)      │ │
│  └─────────────┘  └──────────────────────┘ │
│         │                                    │
│  ┌─────────────────────────────────────┐   │
│  │   Database Manager (VFP ODBC)       │   │
│  │   - Shared Access (Exclusive=No)    │   │
│  │   - Auto Index Maintenance          │   │
│  └─────────────────────────────────────┘   │
│         │                                    │
│  ┌─────────────────────────────────────┐   │
│  │   Index Maintenance Queue           │   │
│  │   - Runs during maintenance window  │   │
│  │   - Retry failed operations         │   │
│  └─────────────────────────────────────┘   │
└─────────────────────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────┐
│      ExpressD Database (DBF + CDX)          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │  .DBF    │  │  .CDX    │  │  .IDX    │ │
│  │ (Data)   │  │ (Index)  │  │ (Legacy) │ │
│  └──────────┘  └──────────┘  └──────────┘ │
└─────────────────────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────┐
│           ExpressD Application              │
│         (Running Concurrently)              │
└─────────────────────────────────────────────┘
```

---

## File Access Strategy

### Shared Access Mode (Default)

```
Connection: Exclusive=No, SourceType=DBF
Locking: Opportunistic + Record-level
Conflict: Retry → Warning → Queue
```

### Lock Conflict Handling

```cpp
try {
    executeSQL("INSERT INTO ...");
    return {success: true, index: "ok"};
} catch (FileLockError) {
    // DO NOT force
    // DO NOT crash
    return {success: false, index: "pending", warnings: ["File locked, queued"]};
}
```

### Index Update Policy

| Policy | Behavior | Use Case |
|--------|----------|----------|
| `auto` | VFP ODBC auto-updates indexes | **Production (Recommended)** |
| `manual_only` | Queue reindex for maintenance | Legacy systems with custom indexes |

---

## Index Maintenance

### Auto Mode (Recommended)

```json
"index_policy": "auto"
```

**Behavior:**
- VFP ODBC automatically updates .CDX on INSERT/UPDATE/DELETE
- No manual reindex needed
- Index status always `ok`
- Safe for ExpressD concurrent use

### Manual Mode (Advanced)

```json
"index_policy": "manual_only"
```

**Behavior:**
- Operations queue reindex tasks
- Reindex runs during maintenance window
- Requires ExpressD to be closed

**Maintenance Window:**
```json
"maintenance_window": "02:00-04:00"
```

**Process:**
1. Stop ExpressD at 2:00 AM
2. FoxBridgeAgent processes queue
3. Reindex tables
4. Start ExpressD at 4:00 AM

---

## Database Connection String

### Production (Multi-User)

```cpp
Driver={Microsoft Visual FoxPro Driver};
SourceType=DBF;
SourceDB=D:\ExpressD\Data;
Exclusive=No;       // CRITICAL
Collate=Machine;
```

### Maintenance (Exclusive - Only during window)

```cpp
Driver={Microsoft Visual FoxPro Driver};
SourceType=DBF;
SourceDB=D:\ExpressD\Data;
Exclusive=Yes;      // Only when ExpressD closed
Collate=Machine;
```

---

## Cloudflare Tunnel Integration

### Setup

1. **Install cloudflared:**
```powershell
winget install --id Cloudflare.cloudflared
```

2. **Create Tunnel:**
```bash
cloudflared tunnel login
cloudflared tunnel create foxbridge-agent
cloudflared tunnel token <tunnel-id>
```

3. **Configure FoxBridgeAgent:**
```json
{
  "cloudflare_token": "eyJh...",
  "port": 8787
}
```

### Watchdog

FoxBridgeAgent monitors `cloudflared` process:

```
Check Interval: 30 seconds
On Failure: Auto-restart
Max Restarts: Unlimited
```

### Public URL

```
https://foxbridge-agent-xyz.trycloudflare.com
```

Forward to: `http://127.0.0.1:8787`

---

## Windows Service Management

### Service Properties

- **Name:** `FoxBridgeAgent`
- **Display Name:** `FoxBridge Agent - ExpressD API Server`
- **Startup Type:** Automatic
- **Recovery:** Restart on failure

### Commands

```powershell
# Start service
Start-Service FoxBridgeAgent

# Stop service
Stop-Service FoxBridgeAgent

# Restart service
Restart-Service FoxBridgeAgent

# Check status
Get-Service FoxBridgeAgent

# View logs
Get-Content "C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log" -Tail 50 -Wait
```

### Graceful Shutdown

When stopping service:

1. Stop accepting new HTTP requests
2. Wait for pending requests (max 10s)
3. Stop Cloudflare Tunnel
4. Stop index maintenance worker
5. Disconnect database (ODBC)
6. Flush logs

**Result:** No file corruption, no pending transactions

---

## Monitoring & Logging

### Log Files

```
C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log
```

**Rotation:**
- Max size: 10 MB per file
- Keep: 5 files
- Format: `[timestamp] [level] message`

### Log Levels

```json
"log_level": "info"  // debug | info | warn | error
```

### Key Events

| Event | Level | Action Required |
|-------|-------|-----------------|
| Service started | INFO | None |
| Database connected | INFO | None |
| File lock detected | WARN | Check ExpressD activity |
| Index pending | WARN | Verify maintenance window |
| Connection failed | ERROR | Check ODBC driver |
| CDX corruption | ERROR | Run REINDEX during maintenance |

### Health Monitoring

```bash
# Check health every 5 minutes
while true; do
  curl -s http://127.0.0.1:8787/health | jq .
  sleep 300
done
```

**Expected:**
```json
{
  "status": "success",
  "data": {
    "database_connected": true
  }
}
```

---

## Common Scenarios

### Scenario 1: Normal Operation

```
ExpressD: Running (8:00 AM - 6:00 PM)
FoxBridgeAgent: Running (24/7)
API Calls: Success, index=ok
```

**No issues:** Both systems work concurrently.

### Scenario 2: File Lock Conflict

```
ExpressD: Heavy batch processing
API Call: INSERT customer
Result: success=false, warnings=["File locked"]
```

**Action:**
- FoxBridgeAgent queues operation
- Retries automatically
- No crash, no data loss

### Scenario 3: Maintenance Window

```
Time: 2:30 AM
ExpressD: Stopped
FoxBridgeAgent: Running maintenance queue
Tasks: REINDEX customers, REINDEX invoices
```

**Action:**
- Exclusive access granted
- Indexes rebuilt
- Verify integrity

### Scenario 4: Cloudflared Crash

```
Cloudflared Process: Died (exit code 1)
Watchdog: Detected failure
Action: Restarted cloudflared (PID 1234)
```

**Downtime:** < 30 seconds

---

## Troubleshooting

### Problem: "Database connection failed"

**Symptoms:**
```json
{"status": "error", "msg": "Failed to connect to database"}
```

**Solutions:**
1. Check database_path in config.json
2. Verify VFP ODBC Driver installed
3. Test ODBC connection:
```powershell
$conn = New-Object System.Data.Odbc.OdbcConnection
$conn.ConnectionString = "Driver={Microsoft Visual FoxPro Driver};SourceType=DBF;SourceDB=D:\ExpressD\Data;Exclusive=No"
$conn.Open()
$conn.Close()
```

### Problem: "Index status: pending"

**Symptoms:**
```json
{"index": "pending", "warnings": ["Reindex queued"]}
```

**Solutions:**
1. Check `index_policy` (should be `auto`)
2. Verify maintenance window
3. Ensure ExpressD closed during window

### Problem: "Cloudflare Tunnel not starting"

**Symptoms:**
- Service starts but tunnel unreachable
- Logs show: "cloudflared.exe not found"

**Solutions:**
1. Install cloudflared:
```powershell
winget install Cloudflare.cloudflared
```
2. Add to PATH
3. Test manually:
```powershell
cloudflared tunnel --url http://127.0.0.1:8787 run --token <token>
```

### Problem: "High CPU usage"

**Symptoms:**
- FoxBridgeAgent.exe using >50% CPU

**Solutions:**
1. Check for infinite retry loops in logs
2. Reduce `max_retry_attempts`
3. Increase maintenance window
4. Monitor DBF file locks

---

## Performance Tuning

### Database

```json
{
  "connection_timeout": 60,      // Increase for slow networks
  "max_retry_attempts": 5        // Increase for busy systems
}
```

### HTTP Server

```cpp
// In HttpServer.cpp
net::io_context ioc{4};  // Increase thread count for high load
```

### Cloudflare Tunnel

```bash
# Optimize tunnel for latency
cloudflared tunnel --metrics 127.0.0.1:9090 \
  --protocol http2 \
  --url http://127.0.0.1:8787 \
  run --token <token>
```

---

## Security Checklist

- [ ] Strong API key (64+ chars)
- [ ] HTTPS via Cloudflare Tunnel
- [ ] Firewall: Block port 8787 externally
- [ ] Cloudflare Access policies configured
- [ ] API key rotated every 90 days
- [ ] Logs reviewed weekly
- [ ] Database backups daily
- [ ] Service account has minimal permissions

---

## Production Deployment Checklist

### Pre-Deployment

- [ ] Visual FoxPro ODBC Driver installed
- [ ] Database path accessible
- [ ] Strong API key generated
- [ ] Cloudflare Tunnel created (if remote access needed)
- [ ] Maintenance window scheduled
- [ ] Firewall configured

### Deployment

- [ ] Run installer as Administrator
- [ ] Enter configuration during setup
- [ ] Verify service installed: `Get-Service FoxBridgeAgent`
- [ ] Test health endpoint: `curl http://127.0.0.1:8787/health`
- [ ] Verify database connection in logs
- [ ] Test API operations (add/update/query)

### Post-Deployment

- [ ] Monitor logs for 24 hours
- [ ] Test concurrent ExpressD + API access
- [ ] Verify Cloudflare Tunnel connectivity
- [ ] Schedule daily health checks
- [ ] Document API endpoints for team
- [ ] Train users on API usage

---

## Backup & Recovery

### Database Backups

**FoxBridgeAgent does not modify backup strategy.**

Continue existing ExpressD backup procedures:
- Full backup daily
- Incremental backups hourly
- Offsite replication

### Configuration Backup

```powershell
# Backup config
Copy-Item "C:\ProgramData\FoxBridgeAgent\config.json" `
  -Destination "\\backup\foxbridge\config_$(Get-Date -Format 'yyyyMMdd').json"
```

### Disaster Recovery

1. Reinstall FoxBridgeAgent from installer
2. Restore config.json
3. Verify database path
4. Start service

**RTO:** < 15 minutes
**RPO:** Last config backup

---

## Support & Contact

For issues specific to FoxBridgeAgent:

1. Check logs: `C:\ProgramData\FoxBridgeAgent\logs\`
2. Review this document
3. Test in console mode: `FoxBridgeAgent.exe --console`
4. Contact: support@yourcompany.com
