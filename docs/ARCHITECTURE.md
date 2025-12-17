# FoxBridgeAgent - Architecture & Design Document

## 1. Executive Summary

**FoxBridgeAgent** is a production-grade Windows Service written in C++20 that provides HTTP API access to Visual FoxPro (VFP) databases used by the ExpressD accounting system. The design prioritizes **database safety**, **concurrent multi-user access**, and **zero-downtime operation** while ExpressD remains running.

### Key Design Principles

1. **Never Require Exclusive Access** - Operates in shared mode alongside ExpressD
2. **Automatic Index Maintenance** - Leverages VFP ODBC driver's built-in capabilities
3. **Graceful Degradation** - Handles file locks and conflicts without crashing
4. **Production Reliability** - Comprehensive logging, error handling, and monitoring

---

## 2. System Architecture

### 2.1 Component Diagram

```
┌────────────────────────────────────────────────────────────────┐
│                    External Clients                             │
│  (Web Apps, Mobile Apps, Integration Services)                 │
└─────────────────────────┬──────────────────────────────────────┘
                          │
                          ▼
┌────────────────────────────────────────────────────────────────┐
│                  Cloudflare Tunnel                              │
│  • Public HTTPS endpoint                                        │
│  • DDoS protection                                              │
│  • Zero Trust authentication                                    │
└─────────────────────────┬──────────────────────────────────────┘
                          │ (port forward)
                          ▼
┌────────────────────────────────────────────────────────────────┐
│              FoxBridgeAgent (Windows Service)                   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  CloudflareTunnel Manager                                 │ │
│  │  • Launch cloudflared process                             │ │
│  │  • Watchdog (30s check interval)                          │ │
│  │  • Auto-restart on failure                                │ │
│  └──────────────────────────────────────────────────────────┘ │
│                          │                                       │
│  ┌──────────────────────▼───────────────────────────────────┐ │
│  │  HTTP Server (Boost.Beast)                                │ │
│  │  • Binds to 127.0.0.1:8787                                │ │
│  │  • API Key authentication                                 │ │
│  │  • RESTful routing                                        │ │
│  │  • JSON request/response                                  │ │
│  └──────────────────────┬───────────────────────────────────┘ │
│                          │                                       │
│  ┌──────────────────────▼───────────────────────────────────┐ │
│  │  DatabaseManager (VFP ODBC)                               │ │
│  │  • Connection pooling                                     │ │
│  │  • Shared access (Exclusive=No)                           │ │
│  │  • Parameterized queries                                  │ │
│  │  • Index status tracking                                  │ │
│  └──────────────────────┬───────────────────────────────────┘ │
│                          │                                       │
│  ┌──────────────────────▼───────────────────────────────────┐ │
│  │  IndexMaintenance (Background Worker)                     │ │
│  │  • Maintenance queue                                      │ │
│  │  • Scheduled window (02:00-04:00)                         │ │
│  │  • Retry logic                                            │ │
│  └──────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  WindowsService Wrapper                                   │ │
│  │  • Service control (start/stop)                           │ │
│  │  • Auto-start on boot                                     │ │
│  │  • Graceful shutdown                                      │ │
│  └──────────────────────────────────────────────────────────┘ │
└─────────────────────────┬──────────────────────────────────────┘
                          │
                          ▼
┌────────────────────────────────────────────────────────────────┐
│            Visual FoxPro ODBC Driver                            │
│  • Multi-user file locking                                      │
│  • Automatic index updates                                      │
│  • Transaction support                                          │
└─────────────────────────┬──────────────────────────────────────┘
                          │
                          ▼
┌────────────────────────────────────────────────────────────────┐
│        ExpressD Database (DBF + CDX files)                      │
│                                                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │ .DBF     │  │ .CDX     │  │ .IDX     │  │ .FPT     │      │
│  │ (Data)   │  │ (Index)  │  │ (Legacy) │  │ (Memo)   │      │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘      │
└─────────────────────────┬──────────────────────────────────────┘
                          │
                          ▼
┌────────────────────────────────────────────────────────────────┐
│              ExpressD Application                               │
│  (Running concurrently, no conflicts)                           │
└────────────────────────────────────────────────────────────────┘
```

### 2.2 Data Flow

#### Read Operation (GET)
```
Client → Cloudflare → HTTP Server → DatabaseManager → VFP ODBC → DBF
                                                                    ↓
Client ← JSON Response ←──────────────────────────────────────────┘
```

#### Write Operation (POST/PUT/DELETE)
```
Client → Cloudflare → HTTP Server → DatabaseManager → VFP ODBC → DBF
                                                                    ↓
                                                              Update CDX
                                                                    ↓
Client ← JSON Response (index: ok) ←──────────────────────────────┘
```

#### Lock Conflict Handling
```
Client → Cloudflare → HTTP Server → DatabaseManager → VFP ODBC → DBF (LOCKED)
                                                                    ↓
                                                              Return Error
                                                                    ↓
                                            IndexMaintenance ← Queue Task
                                                                    ↓
Client ← JSON Response (index: pending) ←──────────────────────────┘
```

---

## 3. Component Details

### 3.1 HttpServer

**Technology:** Boost.Beast (Header-only HTTP library)

**Responsibilities:**
- Accept TCP connections on `127.0.0.1:8787`
- Parse HTTP requests
- Route to appropriate handlers
- Authenticate via `X-API-Key` header
- Serialize responses as JSON
- Handle errors gracefully

**Thread Model:**
- Single-threaded event loop (io_context)
- Asynchronous I/O for scalability
- Can be upgraded to multi-threaded pool if needed

**API Routes:**
```cpp
GET    /health                          → handleHealth()
POST   /api/v1/:table/add               → handleAdd()
PUT    /api/v1/:table/update/:id        → handleUpdate()
DELETE /api/v1/:table/delete/:id        → handleDelete()
GET    /api/v1/:table/get/:id           → handleGet()
GET    /api/v1/:table/query             → handleQuery()
POST   /api/v1/:table/maintenance/reindex → handleReindex()
GET    /api/v1/:table/maintenance/status  → handleIndexStatus()
```

### 3.2 DatabaseManager

**Technology:** Windows ODBC API + VFP ODBC Driver

**Connection String:**
```
Driver={Microsoft Visual FoxPro Driver};
SourceType=DBF;
SourceDB=D:\ExpressD\Data;
Exclusive=No;        // CRITICAL: Shared access
Collate=Machine;
```

**Responsibilities:**
- Establish ODBC connection to VFP database folder
- Execute SQL commands (INSERT, UPDATE, DELETE, SELECT)
- Sanitize table names to prevent SQL injection
- Monitor index health (.CDX file existence/integrity)
- Handle ODBC errors and return meaningful messages

**Safety Mechanisms:**
```cpp
// Table name sanitization
std::string sanitizeTableName(const std::string& table) {
    // Remove: / \ . ; - ' "
    // Allow: alphanumeric + underscore only
    // Prevents: ../../../etc/passwd attacks
}

// Lock conflict handling
try {
    executeSQL("INSERT INTO customers...");
} catch (ODBCLockError) {
    return {success: false, index: PENDING, warnings: ["File locked"]};
    // Never force exclusive access
}
```

### 3.3 CloudflareTunnel

**Technology:** cloudflared.exe process management

**Responsibilities:**
- Launch `cloudflared tunnel run --token <TOKEN>`
- Monitor process health every 30 seconds
- Auto-restart if process dies
- Log all lifecycle events

**Watchdog Logic:**
```cpp
while (watchdog_enabled) {
    Sleep(30 * 1000);
    
    DWORD exit_code;
    GetExitCodeProcess(process_handle, &exit_code);
    
    if (exit_code != STILL_ACTIVE) {
        log_warn("cloudflared died, restarting...");
        launchCloudflared();
    }
}
```

### 3.4 IndexMaintenance

**Technology:** Background worker thread + task queue

**Responsibilities:**
- Queue index maintenance tasks (reindex, verify)
- Process tasks only during maintenance window
- Retry failed tasks up to max_retry_attempts
- Log all maintenance activities

**Task Queue:**
```cpp
struct MaintenanceTask {
    std::string table;
    std::string operation;  // "reindex" | "verify"
    int retry_count;
    std::chrono::system_clock::time_point scheduled_time;
};

std::queue<MaintenanceTask> task_queue_;
std::mutex queue_mutex_;
std::condition_variable queue_cv_;
```

**Maintenance Window Logic:**
```cpp
bool isInMaintenanceWindow() {
    auto now = std::chrono::system_clock::now();
    int current_minutes = hour * 60 + minute;
    int start_minutes = 2 * 60 + 0;   // 02:00
    int end_minutes = 4 * 60 + 0;     // 04:00
    
    return current_minutes >= start_minutes && 
           current_minutes <= end_minutes;
}
```

### 3.5 WindowsService

**Technology:** Win32 Service Control Manager (SCM) API

**Responsibilities:**
- Register as Windows Service
- Handle service control commands (START, STOP)
- Report service status to SCM
- Enable auto-start on boot

**Service Lifecycle:**
```
Startup → SERVICE_START_PENDING → startCallback()
       → SERVICE_RUNNING
       → (running...)
Stop   → SERVICE_STOP_PENDING → stopCallback()
       → SERVICE_STOPPED
```

**Installation:**
```cpp
bool install(const std::string& service_name) {
    SC_HANDLE scm = OpenSCManager(...);
    SC_HANDLE service = CreateService(
        scm,
        "FoxBridgeAgent",
        "FoxBridge Agent - ExpressD API Server",
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,  // Auto-start on boot
        SERVICE_ERROR_NORMAL,
        "C:\\Program Files\\FoxBridgeAgent\\bin\\FoxBridgeAgent.exe --service",
        ...
    );
}
```

---

## 4. Database Strategy

### 4.1 Multi-User Access Model

**ExpressD Scenario:**
- ExpressD opens DBF files in shared mode
- Multiple users can read/write simultaneously
- VFP uses opportunistic locking (.LCK files)

**FoxBridgeAgent Strategy:**
- Connect via VFP ODBC in **non-exclusive mode**
- Let VFP driver handle file locking
- Retry on lock conflicts (don't force)

### 4.2 Index Maintenance Philosophy

#### Option 1: Auto (Recommended)

```json
"index_policy": "auto"
```

**How it works:**
- VFP ODBC automatically updates .CDX indexes on INSERT/UPDATE/DELETE
- Index integrity maintained by VFP engine
- No manual reindex needed
- Safe for concurrent ExpressD use

**Pros:**
- ✅ Zero maintenance
- ✅ Always consistent
- ✅ No downtime required

**Cons:**
- ❌ Slight performance overhead per write
- ❌ No control over timing

#### Option 2: Manual

```json
"index_policy": "manual_only"
```

**How it works:**
- Operations queue reindex tasks
- Tasks execute during maintenance window
- Requires ExpressD to be closed

**Pros:**
- ✅ Batch reindex during off-hours
- ✅ Can optimize indexes manually

**Cons:**
- ❌ Requires planned downtime
- ❌ Potential index lag

### 4.3 Lock Conflict Resolution

```
┌─────────────────┐
│  API Request    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Try Operation  │
└────────┬────────┘
         │
    ┌────▼────┐
    │ Success?│
    └────┬────┘
      YES│  NO
         │   │
         │   ▼
         │ ┌──────────────────┐
         │ │ Check Lock Reason│
         │ └────────┬─────────┘
         │          │
         │      ┌───▼───┐
         │      │Retry? │
         │      └───┬───┘
         │        YES│  NO
         │          │   │
         │   ┌──────▼──┐│
         │   │ Wait 1s ││
         │   └──────┬──┘│
         │          │   │
         │          ▼   ▼
         │   ┌─────────────┐
         │   │Queue Task   │
         │   │(Maintenance)│
         │   └──────┬──────┘
         │          │
         ▼          ▼
┌──────────────────────┐
│Return {index: ok}    │
│or {index: pending}   │
└──────────────────────┘
```

---

## 5. Security Architecture

### 5.1 Authentication

**API Key Based:**
- Every request requires `X-API-Key` header
- Constant-time comparison to prevent timing attacks
- 401 Unauthorized if missing/invalid

**Key Generation (PowerShell):**
```powershell
-join ((65..90) + (97..122) + (48..57) | Get-Random -Count 64 | % {[char]$_})
```

### 5.2 Network Security

**Localhost Binding:**
- HTTP server binds only to `127.0.0.1`
- Not accessible from network
- Requires Cloudflare Tunnel for remote access

**Cloudflare Tunnel Benefits:**
- TLS encryption (HTTPS)
- DDoS mitigation
- Zero Trust access policies
- No port forwarding needed

### 5.3 Input Validation

**Table Name Sanitization:**
```cpp
// Whitelist approach
if (!std::all_of(table.begin(), table.end(),
                 [](char c) { return std::isalnum(c) || c == '_'; })) {
    throw std::runtime_error("Invalid table name");
}
```

**SQL Injection Prevention:**
- Parameterized queries via ODBC
- No string concatenation for SQL
- Filter parameter sanitized by caller

### 5.4 Least Privilege

**Service Account:**
- LocalSystem account (for ODBC access)
- Read/Write to database folder only
- No network privileges

---

## 6. Error Handling & Reliability

### 6.1 Error Categories

| Category | Example | Handling |
|----------|---------|----------|
| **Client Errors** | Invalid JSON, Missing field | 400 Bad Request |
| **Auth Errors** | Missing API key | 401 Unauthorized |
| **Not Found** | Invalid endpoint | 404 Not Found |
| **Lock Errors** | File locked by ExpressD | Retry → Queue → 200 OK (pending) |
| **Database Errors** | Connection lost | 500 Internal Server Error |
| **System Errors** | Out of memory | Log + Crash → Service restart |

### 6.2 Graceful Degradation

**On File Lock:**
```cpp
QueryResult result;
result.success = false;
result.index_status = IndexStatus::PENDING;
result.warnings.push_back("Table locked, operation queued");
return result;  // Don't crash, don't force
```

**On Database Disconnect:**
```cpp
if (!db_manager_->isConnected()) {
    return {
        status: "error",
        msg: "Database unavailable, retrying...",
        index: "failed"
    };
}
```

### 6.3 Logging Strategy

**Log Levels:**
- **DEBUG**: SQL queries, detailed flow
- **INFO**: Normal operations, service lifecycle
- **WARN**: Retries, lock conflicts, degraded mode
- **ERROR**: Database errors, connection failures

**Log Rotation:**
- 10 MB per file
- 5 files retained
- Automatic rotation by spdlog

**Log Location:**
```
C:\ProgramData\FoxBridgeAgent\logs\foxbridge.log
C:\ProgramData\FoxBridgeAgent\logs\foxbridge.1.log
...
C:\ProgramData\FoxBridgeAgent\logs\foxbridge.5.log
```

---

## 7. Performance Considerations

### 7.1 Scalability

**Current Design:**
- Single-threaded HTTP server
- Sequential request processing
- Suitable for: 10-100 req/s

**Scaling Options:**
1. **Increase io_context threads:**
   ```cpp
   net::io_context ioc{4};  // 4 worker threads
   ```

2. **Add connection pooling:**
   ```cpp
   std::vector<std::unique_ptr<DatabaseManager>> pool_;
   ```

3. **Deploy multiple instances:**
   - Run on different ports (8787, 8788, ...)
   - Load balance via Cloudflare

### 7.2 Database Performance

**Optimization:**
- Use indexes for query filters
- Limit result sets (default: 100)
- Avoid `SELECT *` on large tables

**VFP ODBC Limitations:**
- No prepared statements
- No connection pooling
- Synchronous blocking I/O

### 7.3 Monitoring Metrics

**Key Metrics:**
- Requests per second
- Average response time
- Error rate
- Database connection status
- Cloudflared uptime

**Future Enhancement:**
- Prometheus exporter endpoint `/metrics`
- Grafana dashboard

---

## 8. Deployment Architecture

### 8.1 Single Server Deployment

```
┌────────────────────────────────────┐
│      Windows Server 2019+          │
│                                    │
│  ┌──────────────────────────────┐ │
│  │  ExpressD Application        │ │
│  └──────────────────────────────┘ │
│                                    │
│  ┌──────────────────────────────┐ │
│  │  FoxBridgeAgent Service      │ │
│  └──────────────────────────────┘ │
│                                    │
│  ┌──────────────────────────────┐ │
│  │  D:\ExpressD\Data\           │ │
│  │  (DBF + CDX files)           │ │
│  └──────────────────────────────┘ │
└────────────────────────────────────┘
```

### 8.2 High Availability Deployment

```
┌─────────────────┐    ┌─────────────────┐
│  Server 1       │    │  Server 2       │
│  (Primary)      │    │  (Standby)      │
│                 │    │                 │
│  FoxBridgeAgent │    │  FoxBridgeAgent │
│  (Active)       │    │  (Standby)      │
└────────┬────────┘    └────────┬────────┘
         │                      │
         └──────────┬───────────┘
                    │
         ┌──────────▼──────────┐
         │  Shared Storage     │
         │  (NAS/SAN)          │
         │  D:\ExpressD\Data\  │
         └─────────────────────┘
```

**Note:** VFP does not support true HA clustering. Use file replication instead.

---

## 9. Future Enhancements

### 9.1 Planned Features

- [ ] Connection pooling for VFP ODBC
- [ ] Webhook support for real-time notifications
- [ ] GraphQL endpoint
- [ ] Bulk operations API
- [ ] Prometheus metrics exporter
- [ ] Docker container support (Windows containers)
- [ ] PowerShell management module

### 9.2 Performance Improvements

- [ ] Multi-threaded HTTP server
- [ ] Response caching (Redis)
- [ ] Query result pagination
- [ ] Compression (gzip)

### 9.3 Security Enhancements

- [ ] OAuth2 / JWT authentication
- [ ] Role-based access control (RBAC)
- [ ] Rate limiting per API key
- [ ] Audit log trail
- [ ] Encrypted configuration file

---

## 10. Technical Constraints

### 10.1 VFP Limitations

- **No Transactions:** VFP has limited transaction support
- **File Locking:** Coarse-grained, entire table locks
- **No Unicode:** Limited to ANSI/Code Page encoding
- **32-bit Only:** VFP ODBC is 32-bit driver

### 10.2 Windows Dependencies

- Requires Windows Server 2016+ or Windows 10+
- .NET Framework not required (native C++)
- Visual C++ Redistributable required

### 10.3 Cloudflare Tunnel

- Requires cloudflared.exe installed
- Token expires periodically (need rotation)
- Latency: +50-100ms vs direct connection

---

## 11. Testing Strategy

### 11.1 Unit Tests

```cpp
TEST(DatabaseManager, SanitizeTableName) {
    EXPECT_EQ(sanitize("customers"), "customers");
    EXPECT_THROW(sanitize("../../../etc"), std::runtime_error);
}
```

### 11.2 Integration Tests

- Mock VFP ODBC driver
- Test lock conflict scenarios
- Verify index maintenance queue

### 11.3 Load Tests

- Apache Bench: 1000 requests, 10 concurrent
- Monitor memory leaks
- Verify graceful degradation

---

## 12. Conclusion

FoxBridgeAgent provides a robust, production-ready solution for API access to ExpressD databases while maintaining compatibility with the existing VFP multi-user environment. The architecture prioritizes safety, reliability, and ease of operation, ensuring that the ExpressD accounting system can integrate with modern web applications without compromising data integrity.

**Key Takeaways:**
- ✅ Concurrent access with ExpressD (no downtime)
- ✅ Automatic index maintenance (no manual intervention)
- ✅ Graceful error handling (never corrupts files)
- ✅ Production-ready logging and monitoring
- ✅ Secure remote access via Cloudflare Tunnel
