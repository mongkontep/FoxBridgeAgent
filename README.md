# FoxBridgeAgent

**HTTP API Server for Visual FoxPro / ExpressD Database**

Production-ready C++20 Windows Service that provides RESTful API access to ExpressD accounting system databases (Visual FoxPro DBF + CDX) while maintaining full multi-user compatibility.

## 🎯 Key Features

- ✅ **Multi-User Safe**: Runs concurrently with ExpressD without requiring exclusive access
- ✅ **Zero Downtime**: ExpressD never needs to close during API operations
- ✅ **Index Protection**: Automatic index maintenance via VFP ODBC - no CDX corruption
- ✅ **Windows Service**: Auto-start on boot/restart, graceful shutdown, automatic restart on failure
- ✅ **Cloudflare Tunnel**: Secure remote API access with built-in watchdog
- ✅ **Production Ready**: Comprehensive logging, error handling, and monitoring

> **🚀 Set-and-Forget:** Once installed, FoxBridgeAgent automatically starts when Windows boots and restarts itself if it crashes. No manual intervention needed!

## 🏗️ Architecture

```
External API ──> Cloudflare Tunnel ──> HTTP Server ──> VFP ODBC ──> DBF/CDX
                                                              │
                                                              └──> ExpressD
                                                                  (Running)
```

**Core Components:**
- HTTP Server (Boost.Beast) on `127.0.0.1:8787`
- Database Manager (VFP ODBC) with shared access
- Cloudflare Tunnel with auto-restart watchdog
- Index Maintenance Queue for scheduled operations
- Windows Service wrapper for 24/7 operation

## 📦 Quick Start

### Prerequisites

- Windows 10/11 or Windows Server 2019+
- Visual Studio 2022 (for building)
- [Microsoft Visual FoxPro ODBC Driver](https://www.microsoft.com/en-us/download/details.aspx?id=14839)
- [vcpkg](https://vcpkg.io/) (for dependencies)

### Installation (End Users)

1. **Download** `FoxBridgeAgent-Setup.exe` (or `.msi`)
2. **Run installer** as Administrator
3. **Enter configuration:**
   - Database Path: `D:\ExpressD\Data`
   - API Key: [generate secure key]
   - Port: `8787`
   - Cloudflare Token: [optional]
4. **Installer automatically:**
   - Installs Windows Service
   - Creates configuration file
   - Starts the service
5. **Test:** `curl http://127.0.0.1:8787/health`

### Building from Source

See [BUILD.md](BUILD.md) for detailed instructions.

```powershell
# Install dependencies via vcpkg
vcpkg install boost:x64-windows nlohmann-json:x64-windows spdlog:x64-windows

# Build with CMake
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg]\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release

# Output: build/bin/Release/FoxBridgeAgent.exe
```

## 🔧 Configuration

Edit `C:\ProgramData\FoxBridgeAgent\config.json`:

```json
{
  "database_path": "D:\\ExpressD\\Data",
  "api_key": "quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=",
  "host": "127.0.0.1",
  "port": 8787,
  "cloudflare_public_url": "https://foxbridge-agent.your-domain.com",
  "cloudflare_token": "optional-tunnel-token",
  "log_level": "info",
  "index_policy": "auto",
  "maintenance_window": "02:00-04:00"
}
```

See [config/CONFIG.md](config/CONFIG.md) for all options.

## 📚 API Documentation

### Authentication

All endpoints (except `/health`) require:
```
X-API-Key: quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=
```
⚠️ **Change this key before production!** Generate with: `openssl rand -base64 32`

### Access Methods

**Local Access** (same server):
```bash
http://127.0.0.1:8787
```

**Remote Access** (via Cloudflare Tunnel):
```bash
https://foxbridge-agent.your-domain.com
```

### Main Endpoints (DBF File-Based)

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/health` | Service health check (no auth) |
| GET | `/api/dbf/json/:filename.dbf` | Export all records as JSON |
| GET | `/api/dbf/json/:filename.dbf/:docnum` | Export filtered by document number |
| GET | `/api/dbf/csv/:filename.dbf` | Export as CSV |
| GET | `/api/dbf/search/:filename.dbf?field=value` | Search records |
| GET | `/view/:filename.dbf` | View as HTML table |
| GET | `/docnum/:docnum` | Find document across files |
| GET | `/:docnum` | Direct lookup (e.g., /HP0000001) |
| POST | `/api/dbf/add/:filename.dbf` | Add new record |
| POST | `/api/dbf/update/:filename.dbf` | Update records (with WHERE) |
| POST | `/api/dbf/delete/:filename.dbf` | Soft delete (mark) |
| POST | `/api/dbf/undelete/:filename.dbf` | Restore deleted |
| POST | `/api/dbf/pack/:filename.dbf` | Permanent deletion |

### Example Usage

```bash
# Set your API key
export API_KEY="quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks="

# Local access
export BASE_URL="http://127.0.0.1:8787"

# OR Remote access via Cloudflare Tunnel
export BASE_URL="https://foxbridge-agent.your-domain.com"

# Health check (no auth)
curl "${BASE_URL}/health"

# Export all customers as JSON
curl -H "X-API-Key: ${API_KEY}" \
  "${BASE_URL}/api/dbf/json/customers.dbf"

# Add new customer
curl -X POST -H "X-API-Key: ${API_KEY}" \
  -H "Content-Type: application/json" \
  -d '{"custid":"C001","name":"ABC Ltd","status":"active"}' \
  "${BASE_URL}/api/dbf/add/customers.dbf"

# Find document by number
curl -H "X-API-Key: ${API_KEY}" \
  "${BASE_URL}/docnum/HP0000001"

# Export to CSV
curl -H "X-API-Key: ${API_KEY}" \
  "${BASE_URL}/api/dbf/csv/invoice.dbf" -o invoice.csv
```

**Response Format:**
```json
{
  "status": "success",
  "msg": "Record added successfully",
  "data": {...},
  "index": "ok",
  "warnings": []
}
```

See [docs/API.md](docs/API.md) and [docs/CURL_EXAMPLES.md](docs/CURL_EXAMPLES.md) for complete documentation.

## 🛡️ ExpressD Safety

### Critical Design Principles

✅ **DO:**
- Run FoxBridgeAgent while ExpressD is open
- Use `index_policy: "auto"` (VFP ODBC maintains indexes)
- Let VFP handle file locking conflicts gracefully
- Schedule maintenance during off-hours

❌ **DON'T:**
- Force exclusive file access
- Manually reindex while ExpressD is running
- Bypass VFP ODBC for direct DBF writes
- Assume files are always available

### Multi-User Access Strategy

```
Connection String: Exclusive=No, SourceType=DBF
Index Updates: Automatic via VFP ODBC
Lock Conflicts: Retry → Warn → Queue (never force)
Maintenance: Scheduled window when ExpressD closed
```

See [docs/OPERATIONS.md](docs/OPERATIONS.md) for operational details.

## 🌐 Cloudflare Tunnel

Enable secure remote access without port forwarding:

1. **Install cloudflared:**
   ```powershell
   winget install Cloudflare.cloudflared
   ```

2. **Create tunnel:**
   ```bash
   cloudflared tunnel login
   cloudflared tunnel create foxbridge
   cloudflared tunnel token <tunnel-id>
   ```

3. **Add token to config:**
   ```json
   {
     "cloudflare_token": "eyJh..."
   }
   ```

4. **Restart service:**
   ```powershell
   Restart-Service FoxBridgeAgent
   ```

**Built-in Watchdog:**
- Monitors cloudflared process every 30 seconds
- Auto-restarts on failure
- Logs all restart events

## 📊 Monitoring

### Windows Service Management

```powershell
# Check status
Get-Service FoxBridgeAgent

# View logs (live tail)
Get-Content "C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log" -Tail 50 -Wait

# Restart service
Restart-Service FoxBridgeAgent
```

### Health Check Monitoring

```bash
# Scheduled health check (every 5 minutes)
*/5 * * * * curl -s http://127.0.0.1:8787/health || alert
```

### Log Files

Location: `C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log`

- Rotation: 10 MB per file, 5 files retained
- Levels: DEBUG, INFO, WARN, ERROR
- Format: `[timestamp] [level] message`

## 🚨 Troubleshooting

### Service Won't Start

1. Check config: `C:\ProgramData\FoxBridgeAgent\config.json`
2. Verify VFP ODBC installed
3. Test database path exists
4. Check Windows Event Viewer

### Database Connection Failed

```powershell
# Test ODBC connection
$conn = New-Object System.Data.Odbc.OdbcConnection
$conn.ConnectionString = "Driver={Microsoft Visual FoxPro Driver};SourceType=DBF;SourceDB=D:\ExpressD\Data;Exclusive=No"
$conn.Open()
$conn.Close()
```

### Index Status: Pending

- Check `index_policy` = `auto` in config
- Verify maintenance window schedule
- Ensure ExpressD closed during maintenance

See [docs/OPERATIONS.md](docs/OPERATIONS.md) for complete troubleshooting guide.

## 📁 Project Structure

```
FoxBridgeAgent/
├── src/                    # C++ source files
│   ├── main.cpp           # Entry point + service wrapper
│   ├── HttpServer.cpp     # Boost.Beast HTTP server
│   ├── DatabaseManager.cpp # VFP ODBC interface
│   ├── WindowsService.cpp  # Service control
│   ├── CloudflareTunnel.cpp # Tunnel manager + watchdog
│   └── IndexMaintenance.cpp # Queue + scheduled tasks
├── include/               # Header files
├── installer/             # WiX + NSIS installers
├── config/                # Configuration templates
├── docs/                  # Documentation
│   ├── API.md            # API reference
│   ├── OPERATIONS.md     # Operational guide
│   └── ...
├── CMakeLists.txt        # Build configuration
└── README.md             # This file
```

## 🔐 Security

- **API Key Authentication**: Required for all operations
- **Localhost Only**: HTTP server binds to `127.0.0.1`
- **HTTPS**: Enabled via Cloudflare Tunnel
- **Input Validation**: Table name sanitization
- **Logging**: All access attempts logged
- **No SQL Injection**: Parameterized queries via ODBC

**Recommendations:**
- Generate strong API keys (64+ chars)
- Rotate keys every 90 days
- Use Cloudflare Access for Zero Trust
- Monitor logs for suspicious activity

## 📄 License

[Specify your license here]

## 🤝 Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create feature branch
3. Submit pull request

## 📞 Support

- **Documentation**: 
  - [API Documentation](docs/API.md)
  - [cURL Examples](docs/CURL_EXAMPLES.md)
  - [Security Guide](docs/SECURITY.md)
  - [Cloudflare Tunnel Setup](docs/CLOUDFLARE_TUNNEL_SETUP.md)
  - [System Requirements](docs/REQUIREMENTS.md)
  - [**Auto-Start Configuration**](docs/AUTO_START.md) ⭐
- **Issues**: GitHub Issues
- **Email**: support@yourcompany.com

## 🎉 Acknowledgments

Built with:
- [Boost.Beast](https://www.boost.org/doc/libs/release/libs/beast/) - HTTP server
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing
- [spdlog](https://github.com/gabime/spdlog) - Logging
- [Visual FoxPro ODBC](https://www.microsoft.com/en-us/download/details.aspx?id=14839) - Database access

---

**FoxBridgeAgent** - Bridging ExpressD to the modern API world, safely and reliably. 🦊🌉
