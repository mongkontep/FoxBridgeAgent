# Security Guide

## API Key Authentication

### Current API Key

```
quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=
```

⚠️ **CRITICAL**: This is a sample key. **Change it before production deployment!**

### Generating New API Key

```bash
# Generate 256-bit (32 bytes) random key
openssl rand -base64 32

# Example outputs:
# quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=
# J8fH2kL9mN0pQ1rS2tU3vW4xY5zA6bC7dE8fG9hI0j=
# K1lM2nO3pQ4rS5tU6vW7xY8zA9bC0dE1fG2hI3jK4l=
```

### Update API Key

1. **Generate new key**:
   ```bash
   openssl rand -base64 32
   ```

2. **Update config file** (`C:\ProgramData\FoxBridgeAgent\config.json`):
   ```json
   {
     "api_key": "YOUR-NEW-KEY-HERE"
   }
   ```

3. **Restart service**:
   ```powershell
   # PowerShell (as Administrator)
   Restart-Service FoxBridgeAgent
   ```

4. **Update all API clients** with new key

### Using API Key

All endpoints except `/health` require the API key in header:

```bash
curl -H "X-API-Key: quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=" \
  http://127.0.0.1:8787/api/dbf/json/customers.dbf
```

**Response without API key:**
```json
{
  "status": "error",
  "msg": "Unauthorized: Invalid or missing X-API-Key",
  "data": null,
  "index": "ok",
  "warnings": []
}
```

## Network Security Architecture

### Localhost Binding

```cpp
// HttpServer.cpp
server_.bind(tcp::endpoint(tcp::v4(), 8787));  // Binds to 127.0.0.1:8787
```

**What this means:**
- Server ONLY accepts connections from localhost (127.0.0.1)
- Cannot be accessed from network, even on same LAN
- External access ONLY via Cloudflare Tunnel

**Test it:**
```bash
# From same server - ✅ Works
curl http://127.0.0.1:8787/health

# From another computer - ❌ Fails (connection refused)
curl http://192.168.1.100:8787/health
```

### Defense Layers

```
┌─────────────────────────────────────────────────────────────┐
│ Layer 1: Cloudflare Global Network                          │
│ - DDoS protection (billions of attacks/day blocked)         │
│ - Web Application Firewall (WAF)                            │
│ - Bot detection and challenge                               │
│ - Rate limiting                                              │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ Layer 2: Cloudflare Tunnel (cloudflared)                    │
│ - Encrypted tunnel (no open ports)                          │
│ - Certificate-based authentication                          │
│ - Zero Trust access policies (optional)                     │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ Layer 3: Localhost Binding                                  │
│ - Server binds to 127.0.0.1 ONLY                            │
│ - No network access possible                                │
│ - Only cloudflared can connect                              │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ Layer 4: API Key Authentication                             │
│ - Required for all endpoints (except /health)               │
│ - 256-bit random key                                         │
│ - Validated on every request                                │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ Layer 5: SQL Injection Protection                           │
│ - Parameterized queries                                     │
│ - Input validation and sanitization                         │
│ - Filename whitelist (.dbf only)                            │
└─────────────────────────────────────────────────────────────┘
```

## Cloudflare Tunnel Setup

### Why Cloudflare Tunnel?

**Problem with traditional approach:**
```
Internet → Your IP:8787 → FoxBridgeAgent
```
❌ Exposes server IP address
❌ Requires port forwarding
❌ Firewall holes
❌ No DDoS protection
❌ No SSL/TLS encryption
❌ Direct attack surface

**Cloudflare Tunnel approach:**
```
Internet → Cloudflare Network → Encrypted Tunnel → localhost:8787
```
✅ Server IP hidden
✅ No open ports
✅ No firewall changes
✅ DDoS protection included
✅ Free SSL/TLS
✅ Cloudflare's security infrastructure

### Setup Steps

#### 1. Install cloudflared

**Windows:**
```powershell
# Download from Cloudflare
# https://github.com/cloudflare/cloudflared/releases

# Or use winget
winget install --id Cloudflare.cloudflared
```

#### 2. Authenticate

```powershell
cloudflared tunnel login
```
Opens browser → Login to Cloudflare → Authorize

#### 3. Create Tunnel

```powershell
# Create tunnel
cloudflared tunnel create foxbridge-agent

# Output:
# Tunnel credentials written to: C:\Users\User\.cloudflared\<UUID>.json
# Created tunnel foxbridge-agent with id <UUID>
```

#### 4. Configure DNS

```powershell
# Route domain to tunnel
cloudflared tunnel route dns foxbridge-agent foxbridge-agent.your-domain.com
```

#### 5. Create config file

`C:\Users\User\.cloudflared\config.yml`:
```yaml
tunnel: <YOUR-TUNNEL-ID>
credentials-file: C:\Users\User\.cloudflared\<UUID>.json

ingress:
  - hostname: foxbridge-agent.your-domain.com
    service: http://127.0.0.1:8787
  - service: http_status:404
```

#### 6. Start tunnel

```powershell
# Test run
cloudflared tunnel run foxbridge-agent

# Install as service
cloudflared service install

# Start service
sc start cloudflared
```

#### 7. Update FoxBridgeAgent config

`C:\ProgramData\FoxBridgeAgent\config.json`:
```json
{
  "cloudflare_token": "your-tunnel-token",
  "cloudflare_public_url": "https://foxbridge-agent.your-domain.com"
}
```

#### 8. Test

```bash
# From anywhere in the world
curl https://foxbridge-agent.your-domain.com/health
```

### Quick Tunnel (Development)

For testing without domain setup:

```powershell
# Start quick tunnel
cloudflared tunnel --url http://127.0.0.1:8787

# Output:
# Your quick Tunnel has been created!
# https://random-words-1234.trycloudflare.com
```

**Use the provided URL:**
```bash
export BASE_URL="https://random-words-1234.trycloudflare.com"
export API_KEY="quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks="

curl -H "X-API-Key: ${API_KEY}" \
  "${BASE_URL}/api/dbf/json/customers.dbf"
```

⚠️ **Note**: Quick tunnel URLs change on restart. For production, use named tunnels.

## Best Practices

### API Key Management

1. **Never hardcode keys** in source code
2. **Use environment variables** for scripts
3. **Rotate keys regularly** (every 90 days)
4. **Different keys** for dev/staging/production
5. **Revoke old keys** immediately after rotation
6. **Store securely** (Azure Key Vault, AWS Secrets Manager)

### Network Security

1. **Always use localhost binding** (`127.0.0.1`, never `0.0.0.0`)
2. **Enable Cloudflare Tunnel** for remote access
3. **Use HTTPS URLs** (Cloudflare provides free SSL)
4. **Enable Cloudflare WAF** rules
5. **Configure rate limiting** in Cloudflare dashboard
6. **Monitor access logs** regularly

### Access Control

1. **Principle of least privilege** - only grant necessary access
2. **IP whitelist** in Cloudflare (optional)
3. **Zero Trust policies** for sensitive operations
4. **Audit logs** for all API calls
5. **Alert on suspicious activity**

### Database Safety

1. **Never use `Exclusive=Yes`** - breaks ExpressD
2. **Use prepared statements** - prevents SQL injection
3. **Validate all inputs** - filenames, document numbers
4. **Limit query results** - prevent memory exhaustion
5. **Soft delete by default** - prevent accidental data loss

## Security Checklist

Before production deployment:

- [ ] Changed default API key
- [ ] Verified localhost binding (not `0.0.0.0`)
- [ ] Cloudflare Tunnel configured
- [ ] SSL/TLS enabled (via Cloudflare)
- [ ] Rate limiting configured
- [ ] WAF rules enabled
- [ ] Access logs monitored
- [ ] Backup strategy in place
- [ ] Incident response plan documented
- [ ] Security contacts defined

## Incident Response

### API Key Compromised

1. **Immediately generate new key**:
   ```bash
   openssl rand -base64 32
   ```

2. **Update config and restart**:
   ```powershell
   # Update C:\ProgramData\FoxBridgeAgent\config.json
   Restart-Service FoxBridgeAgent
   ```

3. **Update all clients** with new key

4. **Review access logs** for unauthorized usage

5. **Report to security team**

### Unauthorized Access Detected

1. **Enable Cloudflare challenge** (captcha)
2. **Review and update WAF rules**
3. **Add IP to blocklist** if identified
4. **Rotate API key** if compromised
5. **Audit recent API calls**
6. **Check database for unauthorized changes**

## Contact

For security issues, contact:
- Email: security@your-company.com
- Emergency: +66-xxx-xxx-xxxx
