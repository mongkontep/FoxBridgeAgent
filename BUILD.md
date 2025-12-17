# FoxBridgeAgent Build Instructions

## Prerequisites

### Required Software
- **Visual Studio 2022** (or VS 2019 with C++20 support)
  - Install "Desktop development with C++" workload
- **CMake 3.20+** ([Download](https://cmake.org/download/))
- **Boost 1.75+** ([Download](https://www.boost.org/))
  - Recommended: Use vcpkg for easy installation

### Optional (for Cloudflare Tunnel)
- **cloudflared** ([Download](https://developers.cloudflare.com/cloudflare-one/connections/connect-apps/install-and-setup/installation/))

## Installing Dependencies with vcpkg

```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install boost:x64-windows
.\vcpkg install nlohmann-json:x64-windows
.\vcpkg install spdlog:x64-windows

# Integrate with Visual Studio
.\vcpkg integrate install
```

## Building the Project

### Option 1: Using CMake (Command Line)

```powershell
# Clone or navigate to project directory
cd FoxBridgeAgent

# Create build directory
mkdir build
cd build

# Configure with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]\scripts\buildsystems\vcpkg.cmake

# Build
cmake --build . --config Release

# Output will be in build/bin/Release/FoxBridgeAgent.exe
```

### Option 2: Using Visual Studio

1. Open Visual Studio 2022
2. File → Open → CMake...
3. Select `CMakeLists.txt`
4. Configure CMake settings if needed
5. Build → Build All (Ctrl+Shift+B)

## Running the Application

### Console Mode (for testing)

```powershell
FoxBridgeAgent.exe --console --config "path\to\config.json"
```

### Install as Windows Service

```powershell
# Install service (requires Administrator)
FoxBridgeAgent.exe --install

# Start service
FoxBridgeAgent.exe --start

# Stop service
FoxBridgeAgent.exe --stop

# Uninstall service
FoxBridgeAgent.exe --uninstall
```

## Configuration

Create `C:\ProgramData\FoxBridgeAgent\config.json` with your settings.
See `config/config.json.template` for an example.

## Troubleshooting

### ODBC Driver Not Found
- Install **Microsoft Visual FoxPro ODBC Driver**
- Download from Microsoft: [Visual FoxPro ODBC Driver](https://www.microsoft.com/en-us/download/details.aspx?id=14839)

### Boost Not Found
- Ensure Boost is installed via vcpkg
- Or set `BOOST_ROOT` environment variable to Boost installation path

### Permission Denied (Service Install)
- Run Command Prompt or PowerShell as Administrator

## Build Output

- **Executable**: `build/bin/Release/FoxBridgeAgent.exe`
- **Logs**: `C:\ProgramData\FoxBridgeAgent\logs\`
- **Config**: `C:\ProgramData\FoxBridgeAgent\config.json`
