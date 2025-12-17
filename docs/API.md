# FoxBridgeAgent API Documentation

## Base URL

**Local:** `http://127.0.0.1:8787`
**Cloudflare Tunnel:** `https://your-tunnel.trycloudflare.com` (or custom domain)

## Authentication

All API endpoints (except `/health`) require authentication via the `X-API-Key` header.

```bash
X-API-Key: your-secret-api-key-here
```

## Response Format

All responses follow this JSON structure:

```json
{
  "status": "success | error",
  "msg": "Human-readable message",
  "data": {}, 
  "index": "ok | pending | failed",
  "warnings": []
}
```

### Response Fields

- **status**: `success` or `error`
- **msg**: Descriptive message
- **data**: Response payload (object or array)
- **index**: Index health status after operation
- **warnings**: Array of warning messages

## Endpoints

### 1. Health Check

**GET** `/health`

Check if the service is running and database is connected.

**Authentication:** Not required

**Response:**
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

**Example:**
```bash
curl http://127.0.0.1:8787/health
```

---

### 2. Export as JSON (All Records)

**GET** `/api/dbf/json/:filename.dbf`

Export all records from DBF file as JSON.

**Parameters:**
- `:filename.dbf` - DBF filename (e.g., `customers.dbf`)

**Response:**
```json
{
  "status": "success",
  "msg": "Exported 150 records from customers.dbf",
  "data": [
    {
      "cust_id": "C001",
      "name": "บริษัท ABC จำกัด",
      "phone": "02-123-4567",
      "credit_limit": 50000.00
    },
    {
      "cust_id": "C002",
      "name": "ร้าน XYZ",
      "phone": "02-555-6666",
      "credit_limit": 75000.00
    }
  ],
  "index": "ok",
  "warnings": []
}
```

**Example:**
```bash
curl http://127.0.0.1:8787/api/dbf/json/customers.dbf \
  -H "X-API-Key: your-api-key"
```

---

### 3. Export as JSON (Filtered by Document Number)

**GET** `/api/dbf/json/:filename.dbf/:docnum`

Export records filtered by document number (DOCNUM field).

**Parameters:**
- `:filename.dbf` - DBF filename
- `:docnum` - Document number (e.g., `HP0000001`)

**Response:**
```json
{
  "status": "success",
  "msg": "Found 1 record matching HP0000001",
  "data": [{
    "docnum": "HP0000001",
    "docdate": "2025-12-17",
    "customer": "บริษัท ABC จำกัด",
    "amount": 15000.00,
    "status": "paid"
  }],
  "index": "ok",
  "warnings": []
}
```

**Example:**
```bash
curl http://127.0.0.1:8787/api/dbf/json/invoice.dbf/HP0000001 \
  -H "X-API-Key: your-api-key"
```

---

### 4. Export as CSV (All Records)

**GET** `/api/dbf/csv/:filename.dbf`

Export all records as CSV format.

**Parameters:**
- `:filename.dbf` - DBF filename

**Response Headers:**
```
Content-Type: text/csv
Content-Disposition: attachment; filename="customers.csv"
```

**Response Body:**
```csv
cust_id,name,phone,credit_limit
C001,บริษัท ABC จำกัด,02-123-4567,50000.00
C002,ร้าน XYZ,02-555-6666,75000.00
```

**Example:**
```bash
curl http://127.0.0.1:8787/api/dbf/csv/customers.dbf \
  -H "X-API-Key: your-api-key" \
  -o customers.csv
```

---

### 5. Export as CSV (Filtered by Document Number)

**GET** `/api/dbf/csv/:filename.dbf/:docnum`

Export filtered records as CSV.

**Parameters:**
- `:filename.dbf` - DBF filename
- `:docnum` - Document number filter

**Example:**
```bash
curl http://127.0.0.1:8787/api/dbf/csv/invoice.dbf/HP0000001 \
  -H "X-API-Key: your-api-key" \
  -o invoice_HP0000001.csv
```

---

### 6. Search with Filters

**GET** `/api/dbf/search/:filename.dbf?field=value&limit=100`

Search records with multiple field filters.

**Parameters:**
- `:filename.dbf` - DBF filename
- Query parameters: Any field name and value
- `limit` - Maximum records (default: 100)

**Response:**
```json
{
  "status": "success",
  "msg": "Found 5 matching records",
  "data": [
    {
      "cust_id": "C001",
      "name": "บริษัท ABC จำกัด",
      "status": "active"
    }
  ],
  "index": "ok",
  "warnings": []
}
```

**Example:**
```bash
# Search by status
curl "http://127.0.0.1:8787/api/dbf/search/customers.dbf?status=active&limit=50" \
  -H "X-API-Key: your-api-key"

# Search by date range
curl "http://127.0.0.1:8787/api/dbf/search/invoice.dbf?docdate>=2025-01-01&limit=100" \
  -H "X-API-Key: your-api-key"
```

---

### 7. View as HTML Table

**GET** `/view/:filename.dbf`

View DBF contents as HTML table in browser.

**Parameters:**
- `:filename.dbf` - DBF filename

**Response:**
```html
<!DOCTYPE html>
<html>
<head><title>customers.dbf</title></head>
<body>
  <h1>customers.dbf - 150 records</h1>
  <table border="1">
    <tr><th>cust_id</th><th>name</th><th>phone</th></tr>
    <tr><td>C001</td><td>บริษัท ABC จำกัด</td><td>02-123-4567</td></tr>
    <tr><td>C002</td><td>ร้าน XYZ</td><td>02-555-6666</td></tr>
  </table>
</body>
</html>
```

**Example:**
```bash
# Open in browser
open http://127.0.0.1:8787/view/customers.dbf
```

---

### 8. Find Document by Number

**GET** `/docnum/:docnum`

Search for document across all DBF files in database folder.

**Parameters:**
- `:docnum` - Document number (e.g., `HP0000001`)

**Response:**
```json
{
  "status": "success",
  "msg": "Document found in invoice.dbf",
  "data": {
    "file": "invoice.dbf",
    "record": {
      "docnum": "HP0000001",
      "docdate": "2025-12-17",
      "customer": "บริษัท ABC จำกัด",
      "amount": 15000.00
    }
  },
  "index": "ok",
  "warnings": []
}
```

**Example:**
```bash
curl http://127.0.0.1:8787/docnum/HP0000001 \
  -H "X-API-Key: your-api-key"
```

---

### 9. Direct Document Lookup

**GET** `/:docnum`

Shorthand for document lookup (searches common document tables).

**Parameters:**
- `:docnum` - Document number

**Example:**
```bash
curl http://127.0.0.1:8787/HP0000001 \
  -H "X-API-Key: your-api-key"
```

**Notes:**
- Searches: `invoice.dbf`, `receipt.dbf`, `order.dbf`, `quotation.dbf`
- Returns first match found

---

### 10. Add New Record

**POST** `/api/dbf/add/:filename.dbf`

Insert a new record into DBF file.

**Parameters:**
- `:filename.dbf` - DBF filename

**Request Body:**
```json
{
  "cust_id": "C999",
  "name": "ลูกค้าใหม่",
  "phone": "02-888-9999",
  "credit_limit": 100000.00,
  "status": "active"
}
```

**Response:**
```json
{
  "status": "success",
  "msg": "Record added to customers.dbf",
  "data": {
    "recno": 151,
    "cust_id": "C999",
    "name": "ลูกค้าใหม่"
  },
  "index": "ok",
  "warnings": []
}
```

**Example:**
```bash
curl -X POST http://127.0.0.1:8787/api/dbf/add/customers.dbf \
  -H "X-API-Key: your-api-key" \
  -H "Content-Type: application/json" \
  -d '{
    "cust_id": "C999",
    "name": "ลูกค้าใหม่",
    "phone": "02-888-9999"
  }'
```

---

### 11. Update Record

**POST** `/api/dbf/update/:filename.dbf`

Update existing record(s) by matching criteria.

**Parameters:**
- `:filename.dbf` - DBF filename

**Request Body:**
```json
{
  "where": {
    "cust_id": "C001"
  },
  "update": {
    "phone": "02-111-2222",
    "credit_limit": 150000.00
  }
}
```

**Response:**
```json
{
  "status": "success",
  "msg": "Updated 1 record in customers.dbf",
  "data": {
    "updated_count": 1
  },
  "index": "ok",
  "warnings": []
}
```

**Example:**
```bash
curl -X POST http://127.0.0.1:8787/api/dbf/update/customers.dbf \
  -H "X-API-Key: your-api-key" \
  -H "Content-Type: application/json" \
  -d '{
    "where": {"cust_id": "C001"},
    "update": {"phone": "02-111-2222"}
  }'
```

---

### 12. Mark Record as Deleted

**POST** `/api/dbf/delete/:filename.dbf`

Mark record(s) as deleted (soft delete).

**Parameters:**
- `:filename.dbf` - DBF filename

**Request Body:**
```json
{
  "where": {
    "cust_id": "C999"
  }
}
```

**Response:**
```json
{
  "status": "success",
  "msg": "Marked 1 record as deleted in customers.dbf",
  "data": {
    "deleted_count": 1
  },
  "index": "ok",
  "warnings": [
    "Records marked for deletion. Use PACK to permanently remove."
  ]
}
```

**Example:**
```bash
curl -X POST http://127.0.0.1:8787/api/dbf/delete/customers.dbf \
  -H "X-API-Key: your-api-key" \
  -H "Content-Type: application/json" \
  -d '{
    "where": {"cust_id": "C999"}
  }'
```

---

### 13. Restore Deleted Record

**POST** `/api/dbf/undelete/:filename.dbf`

Restore previously deleted record(s).

**Parameters:**
- `:filename.dbf` - DBF filename

**Request Body:**
```json
{
  "where": {
    "cust_id": "C999"
  }
}
```

**Response:**
```json
{
  "status": "success",
  "msg": "Restored 1 record in customers.dbf",
  "data": {
    "restored_count": 1
  },
  "index": "ok",
  "warnings": []
}
```

**Example:**
```bash
curl -X POST http://127.0.0.1:8787/api/dbf/undelete/customers.dbf \
  -H "X-API-Key: your-api-key" \
  -H "Content-Type: application/json" \
  -d '{
    "where": {"cust_id": "C999"}
  }'
```

---

### 14. Pack Database File

**POST** `/api/dbf/pack/:filename.dbf`

Permanently remove deleted records and reclaim disk space.

**Parameters:**
- `:filename.dbf` - DBF filename

**Response:**
```json
{
  "status": "success",
  "msg": "Packed customers.dbf - removed 5 deleted records",
  "data": {
    "removed_count": 5,
    "file_size_before": 1024000,
    "file_size_after": 950000
  },
  "index": "ok",
  "warnings": [
    "PACK requires exclusive access",
    "Ensure ExpressD is closed before packing"
  ]
}
```

**Example:**
```bash
curl -X POST http://127.0.0.1:8787/api/dbf/pack/customers.dbf \
  -H "X-API-Key: your-api-key"
```

**⚠️ CRITICAL WARNING:**
- PACK requires exclusive file access
- ExpressD **MUST** be closed
- Only run during maintenance window
- Creates backup before packing

---

## Error Responses

### 401 Unauthorized

Missing or invalid API key.

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

Endpoint does not exist.

```json
{
  "status": "error",
  "msg": "Not Found",
  "data": null,
  "index": "ok",
  "warnings": []
}
```

### 500 Internal Server Error

Server-side error (database, file lock, etc.).

```json
{
  "status": "error",
  "msg": "Internal Server Error: Failed to connect to database",
  "data": null,
  "index": "failed",
  "warnings": [
    "Check database path in config",
    "Verify VFP ODBC driver installed"
  ]
}
```

---

## Rate Limiting

Currently, no rate limiting is implemented. Consider adding:
- Application-level rate limiting
- Cloudflare Rate Limiting rules (if using Cloudflare Tunnel)

---

## Best Practices

### 1. Keep ExpressD Running
- FoxBridgeAgent designed for concurrent access
- VFP ODBC handles multi-user scenarios
- No need to close ExpressD

### 2. Use Auto Index Policy
```json
"index_policy": "auto"
```
- Safer for production
- VFP ODBC maintains indexes
- Manual reindex only during maintenance

### 3. Schedule Maintenance Window
- Set during off-hours
- Ensure ExpressD is closed
- Monitor index status

### 4. Monitor Logs
```
C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log
```

### 5. Secure API Key
- Use strong random keys (64+ chars)
- Rotate periodically
- Never expose in client-side code

### 6. Use HTTPS via Cloudflare Tunnel
- Enables secure remote access
- Built-in DDoS protection
- Zero Trust access controls

---

## Example Client (PowerShell)

```powershell
$apiKey = "your-api-key-here"
$baseUrl = "http://127.0.0.1:8787"

$headers = @{
    "X-API-Key" = $apiKey
    "Content-Type" = "application/json"
}

# Add record
$body = @{
    cust_id = "C999"
    name = "Test Customer"
    phone = "02-000-0000"
} | ConvertTo-Json

$response = Invoke-RestMethod -Uri "$baseUrl/api/v1/customers/add" `
    -Method POST -Headers $headers -Body $body

Write-Output $response
```

---

## Example Client (Node.js)

```javascript
const axios = require('axios');

const apiKey = 'your-api-key-here';
const baseUrl = 'http://127.0.0.1:8787';

async function addCustomer() {
  try {
    const response = await axios.post(
      `${baseUrl}/api/v1/customers/add`,
      {
        cust_id: 'C999',
        name: 'Test Customer',
        phone: '02-000-0000'
      },
      {
        headers: {
          'X-API-Key': apiKey,
          'Content-Type': 'application/json'
        }
      }
    );
    
    console.log(response.data);
  } catch (error) {
    console.error(error.response.data);
  }
}

addCustomer();
```
