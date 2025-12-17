# FoxBridgeAgent - curl Examples

API endpoint examples using curl for testing and integration with **DBF file-based operations**.

## Prerequisites

```bash
# Set environment variables for easy testing
export API_KEY="quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks="

# For LOCAL access (on same server)
export BASE_URL="http://127.0.0.1:8787"

# For REMOTE access (via Cloudflare Tunnel)
export BASE_URL="https://foxbridge-agent.your-domain.com"

# Or for Windows (PowerShell)
$env:API_KEY = "quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks="
$env:BASE_URL = "http://127.0.0.1:8787"  # Local
$env:BASE_URL = "https://foxbridge-agent.your-domain.com"  # Remote
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ CLIENT (Your Application)                                    │
└────────────┬────────────────────────────────────────────────┘
             │
             ├─ LOCAL:  http://127.0.0.1:8787
             │           └─ Fast, no latency
             │           └─ Requires access to server
             │
             └─ REMOTE: https://foxbridge-agent.your-domain.com
                         └─ HTTPS encrypted
                         └─ Cloudflare DDoS protection
                         └─ Access from anywhere
                         │
                         ▼
             ┌─────────────────────────────┐
             │ Cloudflare Tunnel           │
             │ (cloudflared service)       │
             └──────────┬──────────────────┘
                        │
                        ▼
             ┌─────────────────────────────┐
             │ FoxBridgeAgent              │
             │ HTTP Server (127.0.0.1:8787)│
             │ ← Binds to localhost ONLY   │
             └──────────┬──────────────────┘
                        │
                        ▼
             ┌─────────────────────────────┐
             │ VFP ODBC Driver             │
             │ (Exclusive=No)              │
             └──────────┬──────────────────┘
                        │
                        ▼
             ┌─────────────────────────────┐
             │ ExpressD DBF Files          │
             │ (Multi-user safe)           │
             └─────────────────────────────┘
```

---

## 1. Health Check

No authentication required.

```bash
curl -X GET "${BASE_URL}/health"
```

**Expected Response:**
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

---

## 2. Export All Records as JSON

Get all records from a DBF file.

```bash
curl -X GET "${BASE_URL}/api/dbf/json/customers.dbf" \
  -H "X-API-Key: ${API_KEY}"
```

**Expected Response:**
```json
{
  "status": "success",
  "msg": "All records exported",
  "data": [
    {
      "custid": "C001",
      "name": "ABC Company",
      "address": "123 Main St",
      "phone": "555-1234"
    },
    {
      "custid": "C002",
      "name": "XYZ Corp",
      "address": "456 Oak Ave",
      "phone": "555-5678"
    }
  ],
  "index": "ok",
  "warnings": []
}
```

---

## 3. Export Filtered JSON by Document Number

Get specific record(s) filtered by document number.

```bash
curl -X GET "${BASE_URL}/api/dbf/json/invoice.dbf/HP0000001" \
  -H "X-API-Key: ${API_KEY}"
```

**Expected Response:**
```json
{
  "status": "success",
  "msg": "Record found",
  "data": [
    {
      "docnum": "HP0000001",
      "date": "2024-01-15",
      "custid": "C001",
      "amount": 15000.00,
      "status": "paid"
    }
  ],
  "index": "ok",
  "warnings": []
}
```

---

## 4. Export as CSV

Get all records from a DBF file in CSV format.

```bash
curl -X GET "${BASE_URL}/api/dbf/csv/customers.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  -o customers.csv
```

**CSV Output:**
```csv
"custid","name","address","phone"
"C001","ABC Company","123 Main St","555-1234"
"C002","XYZ Corp","456 Oak Ave","555-5678"
```

---

## 5. Export CSV Filtered by Document Number

```bash
curl -X GET "${BASE_URL}/api/dbf/csv/invoice.dbf/HP0000001" \
  -H "X-API-Key: ${API_KEY}" \
  -o invoice_HP0000001.csv
```

---

## 6. Search with Query Parameters

Search for records matching specific criteria.

```bash
curl -X GET "${BASE_URL}/api/dbf/search/customers.dbf?name=ABC&limit=10" \
  -H "X-API-Key: ${API_KEY}"
```

**Expected Response:**
```json
{
  "status": "success",
  "msg": "Search completed",
  "data": [
    {
      "custid": "C001",
      "name": "ABC Company",
      "address": "123 Main St",
      "phone": "555-1234"
    }
  ],
  "index": "ok",
  "warnings": []
}
```

---

## 7. View as HTML Table

Open in browser or view HTML output.

```bash
curl -X GET "${BASE_URL}/view/customers.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  -o customers.html

# Open in browser (macOS)
open customers.html

# Open in browser (Windows)
start customers.html
```

---

## 8. Find by Document Number (Across All Files)

Search for a document number across common DBF files.

```bash
curl -X GET "${BASE_URL}/docnum/HP0000001" \
  -H "X-API-Key: ${API_KEY}"
```

**Expected Response:**
```json
{
  "status": "success",
  "msg": "Document found in 2 table(s)",
  "data": [
    {
      "docnum": "HP0000001",
      "date": "2024-01-15",
      "amount": 15000.00,
      "_source_file": "invoice.dbf"
    },
    {
      "docnum": "HP0000001",
      "reference": "INV-001",
      "_source_file": "payment.dbf"
    }
  ],
  "index": "ok",
  "warnings": []
}
```

---

## 9. Direct Document Lookup (Shorthand)

Quick lookup using document number format (2 letters + 7 digits).

```bash
curl -X GET "${BASE_URL}/HP0000001" \
  -H "X-API-Key: ${API_KEY}"
```

Same response as `/docnum/HP0000001`.

---

## 10. Add Record

Insert a new record into a DBF file.

```bash
curl -X POST "${BASE_URL}/api/dbf/add/customers.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  -H "Content-Type: application/json" \
  -d '{
    "custid": "C003",
    "name": "New Customer Ltd",
    "address": "789 Park Lane",
    "phone": "555-9999",
    "email": "contact@newcust.com",
    "credit_limit": 50000.00,
    "status": "active"
  }'
```

**Expected Response:**
```json
{
  "status": "success",
  "msg": "Record added successfully",
  "data": {
    "custid": "C003",
    "name": "New Customer Ltd",
    "address": "789 Park Lane",
    "phone": "555-9999"
  },
  "index": "ok",
  "warnings": []
}
```

---

## 11. Update Records

Update records matching a WHERE clause.

```bash
curl -X POST "${BASE_URL}/api/dbf/update/customers.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  -H "Content-Type: application/json" \
  -d '{
    "where": {
      "custid": "C001"
    },
    "update": {
      "phone": "555-0000",
      "email": "newemail@abc.com",
      "credit_limit": 75000.00
    }
  }'
```

**Expected Response:**
```json
{
  "status": "success",
  "msg": "Record(s) updated successfully",
  "data": {
    "phone": "555-0000",
    "email": "newemail@abc.com",
    "credit_limit": 75000.00
  },
  "index": "ok",
  "warnings": []
}
```

---

## 12. Soft Delete (Mark as Deleted)

Mark records as deleted without physically removing them.

```bash
curl -X POST "${BASE_URL}/api/dbf/delete/customers.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  -H "Content-Type: application/json" \
  -d '{
    "where": {
      "custid": "C003"
    }
  }'
```

**Expected Response:**
```json
{
  "status": "success",
  "msg": "Record(s) marked as deleted",
  "data": null,
  "index": "ok",
  "warnings": []
}
```

---

## 13. Restore Deleted Records (Undelete)

Restore records that were previously soft-deleted.

```bash
curl -X POST "${BASE_URL}/api/dbf/undelete/customers.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  -H "Content-Type: application/json" \
  -d '{
    "where": {
      "custid": "C003"
    }
  }'
```

**Expected Response:**
```json
{
  "status": "success",
  "msg": "Record(s) restored",
  "data": null,
  "index": "ok",
  "warnings": []
}
```

---

## 14. Pack DBF File (Permanent Deletion)

**⚠️ CRITICAL:** This operation requires exclusive access. ExpressD must be closed.

```bash
curl -X POST "${BASE_URL}/api/dbf/pack/customers.dbf" \
  -H "X-API-Key: ${API_KEY}"
```

**Expected Response (Multi-User Mode):**
```json
{
  "status": "error",
  "msg": "PACK operation requires exclusive access. Please close ExpressD first.",
  "data": null,
  "index": "ok",
  "warnings": [
    "ExpressD must be closed for PACK operation",
    "Use maintenance window for PACK",
    "Consider using soft delete instead"
  ]
}
```

---

## Integration Examples

### Batch Export Multiple Files

```bash
#!/bin/bash
# Export multiple DBF files to JSON

files=("invoice.dbf" "customers.dbf" "products.dbf")

for file in "${files[@]}"; do
  echo "Exporting ${file}..."
  curl -X GET "${BASE_URL}/api/dbf/json/${file}" \
    -H "X-API-Key: ${API_KEY}" \
    -o "${file%.dbf}.json"
done
```

### Monitor Document Status

```bash
#!/bin/bash
# Check status of a document number

DOCNUM="HP0000001"

curl -X GET "${BASE_URL}/docnum/${DOCNUM}" \
  -H "X-API-Key: ${API_KEY}" \
  | jq '.data[] | {file: ._source_file, docnum, status}'
```

### Create Invoice with Auto Document Number

```bash
#!/bin/bash
# Add new invoice with auto-generated document number

curl -X POST "${BASE_URL}/api/dbf/add/invoice.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  -H "Content-Type: application/json" \
  -d '{
    "docnum": "HP0000123",
    "date": "2024-01-20",
    "custid": "C001",
    "amount": 25000.00,
    "tax": 1750.00,
    "total": 26750.00,
    "status": "pending"
  }'
```

### Daily CSV Backup

```bash
#!/bin/bash
# Backup all critical DBF files to CSV

DATE=$(date +%Y%m%d)
BACKUP_DIR="./backup/${DATE}"

mkdir -p "${BACKUP_DIR}"

files=("invoice.dbf" "receipt.dbf" "customers.dbf" "products.dbf")

for file in "${files[@]}"; do
  echo "Backing up ${file}..."
  curl -X GET "${BASE_URL}/api/dbf/csv/${file}" \
    -H "X-API-Key: ${API_KEY}" \
    -o "${BACKUP_DIR}/${file%.dbf}.csv"
done

echo "Backup completed: ${BACKUP_DIR}"
```

---

## Error Handling

---

## Error Examples

### 401 Unauthorized (Missing API Key)

```bash
curl -X GET "${BASE_URL}/api/v1/customers/query"
```

**Response:**
```json
{
  "status": "error",
  "msg": "Unauthorized: Invalid or missing X-API-Key",
  "data": null,
  "index": "ok",
  "warnings": []
}
```

### 401 Unauthorized (Invalid API Key)

```bash
curl -X GET "${BASE_URL}/api/v1/customers/query" \
  -H "X-API-Key: wrong-key"
```

**Response:**
```json
{
  "status": "error",
  "msg": "Unauthorized: Invalid or missing X-API-Key",
  "data": null,
  "index": "ok",
  "warnings": []
}
```

### 404 Not Found

```bash
curl -X GET "${BASE_URL}/api/v1/invalid/endpoint" \
  -H "X-API-Key: ${API_KEY}"
```

**Response:**
```json
{
  "status": "error",
  "msg": "Not Found",
  "data": null,
  "index": "ok",
  "warnings": []
}
```

---

## Batch Operations Script

### Bash Script

```bash
#!/bin/bash

API_KEY="your-api-key"
BASE_URL="http://127.0.0.1:8787"

# Add multiple customers
for i in {1..10}; do
  curl -X POST "${BASE_URL}/api/v1/customers/add" \
    -H "X-API-Key: ${API_KEY}" \
    -H "Content-Type: application/json" \
    -d "{
      \"cust_id\": \"C$(printf "%03d" $i)\",
      \"name\": \"Customer $i\",
      \"phone\": \"02-000-$(printf "%04d" $i)\",
      \"credit_limit\": $((50000 + i * 1000))
    }"
  
  echo ""
done
```

### PowerShell Script

```powershell
$apiKey = "your-api-key"
$baseUrl = "http://127.0.0.1:8787"

$headers = @{
    "X-API-Key" = $apiKey
    "Content-Type" = "application/json"
}

# Add multiple customers
1..10 | ForEach-Object {
    $body = @{
        cust_id = "C$($_.ToString('000'))"
        name = "Customer $_"
        phone = "02-000-$($_.ToString('0000'))"
        credit_limit = 50000 + ($_ * 1000)
    } | ConvertTo-Json

    $response = Invoke-RestMethod -Uri "$baseUrl/api/v1/customers/add" `
        -Method POST -Headers $headers -Body $body
    
    Write-Output $response
}
```

---

## Testing via Postman

### Import Collection

1. Create new Postman Collection: **FoxBridgeAgent API**
2. Add environment variable:
   - `base_url`: `http://127.0.0.1:8787`
   - `api_key`: `your-secret-key`

### Configure Authorization

In Collection settings:
- Type: `API Key`
- Key: `X-API-Key`
- Value: `{{api_key}}`
- Add to: `Header`

### Sample Requests

Create requests for each endpoint using `{{base_url}}` variable.

### 401 Unauthorized (Missing API Key)

```bash
curl -X GET "${BASE_URL}/api/dbf/json/customers.dbf"
```

**Response:**
```json
{
  "status": "error",
  "msg": "Unauthorized: Invalid or missing X-API-Key",
  "data": null,
  "index": "ok",
  "warnings": []
}
```

### 404 File Not Found

```bash
curl -X GET "${BASE_URL}/api/dbf/json/nonexistent.dbf" \
  -H "X-API-Key: ${API_KEY}"
```

**Response:**
```json
{
  "status": "error",
  "msg": "File not found: nonexistent.dbf",
  "data": null,
  "index": "ok",
  "warnings": []
}
```

### 400 Bad Request (Missing WHERE Clause)

```bash
curl -X POST "${BASE_URL}/api/dbf/update/customers.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  -H "Content-Type: application/json" \
  -d '{"update": {"phone": "555-0000"}}'
```

**Response:**
```json
{
  "status": "error",
  "msg": "Missing 'where' or 'update' in request body",
  "data": null,
  "index": "ok",
  "warnings": []
}
```

---

## Advanced: Using jq for Pretty Output

```bash
# Install jq (JSON processor)
# macOS: brew install jq
# Windows: choco install jq

# Pretty print
curl -s "${BASE_URL}/health" | jq .

# Extract specific fields
curl -s "${BASE_URL}/api/dbf/json/customers.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  | jq '.data[] | {name, phone}'

# Count records
curl -s "${BASE_URL}/api/dbf/json/customers.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  | jq '.data | length'
```

---

## Access Methods

### 1. Local Access (Same Server)

```bash
export BASE_URL="http://127.0.0.1:8787"
export API_KEY="quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks="

curl -X GET "${BASE_URL}/health"
```

**When to use:**
- Running on same Windows server as FoxBridgeAgent
- Maximum speed (no network latency)
- Testing and development

### 2. Remote Access via Cloudflare Tunnel

```bash
export BASE_URL="https://foxbridge-agent.your-domain.com"
export API_KEY="quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks="

# All commands work identically
curl -X GET "${BASE_URL}/health"

curl -X POST "${BASE_URL}/api/dbf/add/customers.dbf" \
  -H "X-API-Key: ${API_KEY}" \
  -H "Content-Type: application/json" \
  -d '{"custid": "C999", "name": "Remote Customer", "status": "active"}'
```

**When to use:**
- Accessing from external servers/applications
- Web applications (JavaScript/React/Vue)
- Mobile applications
- Integration with cloud services

**Benefits:**
- ✅ HTTPS encryption (API key protected in transit)
- ✅ No port forwarding or firewall changes needed
- ✅ Cloudflare DDoS protection (billions of attacks blocked daily)
- ✅ Zero Trust access policies (optional)
- ✅ Automatic failover and load balancing
- ✅ Works behind NAT/firewalls

**Why localhost + Cloudflare?**
- HTTP server binds to `127.0.0.1` ONLY = no direct internet access
- Cloudflare Tunnel is the ONLY way in from outside
- Even if attacker knows your IP, they can't reach port 8787
- Defense in depth: API key + localhost binding + Cloudflare protection

---

## Monitoring Health

```bash
#!/bin/bash
# Simple health monitor script

API_KEY="your-api-key"
BASE_URL="http://127.0.0.1:8787"

while true; do
  response=$(curl -s "${BASE_URL}/health")
  status=$(echo $response | jq -r '.status')
  connected=$(echo $response | jq -r '.data.database_connected')
  
  timestamp=$(date '+%Y-%m-%d %H:%M:%S')
  
  if [ "$status" = "success" ] && [ "$connected" = "true" ]; then
    echo "[$timestamp] ✓ FoxBridgeAgent healthy"
  else
    echo "[$timestamp] ✗ FoxBridgeAgent unhealthy"
  fi
  
  sleep 60
done
```

---

## Notes

### General
- **File Extensions**: Always include `.dbf` extension in filenames
- **Document Numbers**: Format is typically 2 letters + 7 digits (e.g., `HP0000001`)
- **CSV Export**: Includes header row with field names, UTF-8 encoded
- **HTML View**: Generates responsive table view for browser viewing
- **Multi-User**: All operations work while ExpressD is running (except PACK)
- **Index Updates**: Handled automatically by VFP ODBC driver

### Security
- **API Key**: `quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks=`
  - Generated with `openssl rand -base64 32`
  - **⚠️ CHANGE THIS** before production deployment!
  - Required in `X-API-Key` header for all endpoints except `/health`
  - Keep secret, never commit to public repositories
  
- **Localhost Binding**: Server binds to `127.0.0.1` ONLY
  - Cannot be accessed directly from network
  - Only accessible via localhost or Cloudflare Tunnel
  - Provides additional security layer

### Access URLs
- **Local**: `http://127.0.0.1:8787` (fast, requires server access)
- **Remote**: `https://foxbridge-agent.your-domain.com` (secure, from anywhere)

### CRUD Operations
- **Soft Delete**: Use `/api/dbf/delete` to mark as deleted, `/api/dbf/undelete` to restore
- **Hard Delete**: Use `/api/dbf/pack` (⚠️ requires ExpressD to be closed)


3. **Pipe to `jq` for readable JSON** - Much easier to debug
4. **Check HTTP status codes** - Use `curl -w "%{http_code}"`
5. **Enable verbose mode for debugging** - `curl -v`
6. **Save responses to file** - `curl ... -o response.json`
7. **Test locally first** - Before hitting production
8. **Monitor logs during testing** - `tail -f foxbridge.log`
