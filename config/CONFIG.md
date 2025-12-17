# FoxBridgeAgent Configuration Guide

## Configuration File Location

The configuration file is located at:
```
C:\ProgramData\FoxBridgeAgent\config.json
```

## Configuration Options

### Required Settings

#### `database_path` (string, required)
Path to the ExpressD/Visual FoxPro database folder containing DBF files.

**Example:**
```json
"database_path": "D:\\ExpressD\\Data"
```

**Notes:**
- Must be an existing directory
- Use double backslashes (`\\`) in Windows paths
- ExpressD can remain running while FoxBridgeAgent operates

#### `api_key` (string, required)
Secret key for authenticating API requests via `X-API-Key` header.

**Example:**
```json
"api_key": "sk_live_abc123xyz789defghi456"
```

**Security:**
- Minimum 32 characters recommended
- Use cryptographically secure random generation
- Rotate periodically
- Never commit to version control

**Generate a secure key (PowerShell):**
```powershell
-join ((65..90) + (97..122) + (48..57) | Get-Random -Count 64 | % {[char]$_})
```

#### `port` (integer, required)
HTTP server listening port on localhost.

**Example:**
```json
"port": 8787
```

**Notes:**
- Must be between 1024-65535
- Ensure port is not used by other applications
- Firewall configuration not needed (localhost only)

### Optional Settings

#### `cloudflare_token` (string, optional)
Cloudflare Tunnel token for exposing API to the internet.

**Example:**
```json
"cloudflare_token": "eyJhIjoiNzg5..."
```

**How to get:**
1. Log in to [Cloudflare Zero Trust](https://one.dash.cloudflare.com/)
2. Navigate to Networks → Tunnels
3. Create a tunnel
4. Copy the token

**Leave empty** if not using Cloudflare Tunnel.

#### `log_level` (string, optional)
Logging verbosity level.

**Options:** `debug`, `info`, `warn`, `error`

**Default:** `info`

**Example:**
```json
"log_level": "debug"
```

#### `log_path` (string, optional)
Directory for storing log files.

**Default:** `C:\ProgramData\FoxBridgeAgent\logs`

**Example:**
```json
"log_path": "E:\\Logs\\FoxBridge"
```

#### `index_policy` (string, optional)
Index maintenance strategy.

**Options:**
- `auto` - Automatically handle index updates via VFP ODBC
- `manual_only` - Require manual reindex via API

**Default:** `auto`

**Example:**
```json
"index_policy": "auto"
```

**Recommendation:** Use `auto` for production (safer with ExpressD running).

#### `maintenance_window` (string, optional)
Time window for scheduled maintenance tasks (24-hour format).

**Format:** `HH:MM-HH:MM`

**Default:** `02:00-04:00` (2 AM - 4 AM)

**Example:**
```json
"maintenance_window": "01:30-03:30"
```

**Notes:**
- Reindex operations only occur during this window
- Ensure ExpressD is idle during maintenance
- Use overnight hours

#### `max_retry_attempts` (integer, optional)
Maximum retry attempts for failed operations.

**Default:** `3`

**Example:**
```json
"max_retry_attempts": 5
```

#### `connection_timeout` (integer, optional)
Database connection timeout in seconds.

**Default:** `30`

**Example:**
```json
"connection_timeout": 60
```

## Complete Example

```json
{
  "database_path": "D:\\ExpressD\\Data",
  "api_key": "sk_prod_a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6",
  "port": 8787,
  "cloudflare_token": "eyJhIjoiNzg5YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXoiLCJ0IjoiMTIzNDU2Nzg5MCIsInMiOiJhYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3h5eiJ9",
  "log_level": "info",
  "log_path": "C:\\ProgramData\\FoxBridgeAgent\\logs",
  "index_policy": "auto",
  "maintenance_window": "02:00-04:00",
  "max_retry_attempts": 3,
  "connection_timeout": 30
}
```

## Applying Configuration Changes

After modifying the configuration file:

```powershell
# Restart the service
Restart-Service FoxBridgeAgent

# Or via command line
FoxBridgeAgent.exe --stop
FoxBridgeAgent.exe --start
```

## Validation

To validate configuration without starting the service:

```powershell
# Test configuration
FoxBridgeAgent.exe --console --config "C:\ProgramData\FoxBridgeAgent\config.json"

# If valid, you'll see:
# [info] Configuration loaded successfully
# [info] FoxBridgeAgent Starting...
```

Press Ctrl+C to stop test mode.

## Troubleshooting

### "Configuration error: database_path is required"
- Ensure `database_path` is set in config.json
- Check that the path exists

### "Configuration error: api_key is required"
- Add `api_key` to config.json
- Use a strong random key

### "Configuration error: port must be between 1024 and 65535"
- Check that `port` value is valid
- Change to unused port if conflict exists

### "Failed to open config file"
- Verify config file exists at `C:\ProgramData\FoxBridgeAgent\config.json`
- Check file permissions
- Ensure JSON syntax is valid
