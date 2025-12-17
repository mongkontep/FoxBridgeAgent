# FoxBridgeAgent WiX Installer

## Prerequisites

- **WiX Toolset 3.11+** ([Download](https://wixtoolset.org/releases/))
- Built FoxBridgeAgent.exe (from Release build)

## Building the Installer

### Step 1: Compile WiX Source

```powershell
cd installer

# Compile .wxs to .wixobj
candle.exe FoxBridgeAgent.wxs -ext WixUtilExtension

# Link to create .msi
light.exe FoxBridgeAgent.wixobj -ext WixUtilExtension -out FoxBridgeAgent.msi
```

### Step 2: Customize GUIDs

Before building, replace placeholder GUIDs in `FoxBridgeAgent.wxs`:

```xml
<!-- Generate new GUIDs using PowerShell -->
[guid]::NewGuid()
```

Replace these in the .wxs file:
- `YOUR-GUID-HERE` (UpgradeCode)
- `YOUR-GUID-1` (Component 1)
- `YOUR-GUID-2` (Component 2)
- `YOUR-GUID-3` (Component 3)

### Step 3: Set Binary Path

Update the `BinPath` variable when compiling:

```powershell
candle.exe FoxBridgeAgent.wxs -dBinPath="..\build\bin\Release" -ext WixUtilExtension
```

## Installer Features

The installer will:

1. **Prompt for Configuration**:
   - Database Path (default: `D:\ExpressD\Data`)
   - API Key
   - HTTP Port (default: 8787)
   - Cloudflare Tunnel Token

2. **Install Files**:
   - `C:\Program Files\FoxBridgeAgent\bin\FoxBridgeAgent.exe`
   - `C:\ProgramData\FoxBridgeAgent\config.json`

3. **Create Directories**:
   - `C:\ProgramData\FoxBridgeAgent\logs\`

4. **Install Windows Service**:
   - Service Name: `FoxBridgeAgent`
   - Display Name: `FoxBridge Agent - ExpressD API Server`
   - Startup Type: Automatic
   - Account: LocalSystem

5. **Start Service**: Automatically starts after installation

## Testing the Installer

```powershell
# Install
msiexec /i FoxBridgeAgent.msi /l*v install.log

# Uninstall
msiexec /x FoxBridgeAgent.msi /l*v uninstall.log
```

## Alternative: NSIS Installer

If you prefer NSIS instead of WiX, see `FoxBridgeAgent.nsi` for an NSIS script.

### Building with NSIS

```powershell
makensis.exe FoxBridgeAgent.nsi
```

## Troubleshooting

### WiX Build Errors

- Ensure all GUIDs are unique
- Check that `BinPath` points to correct directory
- Verify all referenced files exist

### Service Installation Fails

- Check Windows Event Viewer for errors
- Ensure installer is run as Administrator
- Verify service account has necessary permissions
