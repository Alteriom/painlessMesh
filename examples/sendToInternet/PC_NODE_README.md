# PC Mesh Node - Testing sendToInternet() from Regular Node Through Bridge

## Overview

This example provides a **PC-based mesh node** that can join a painlessMesh network with ESP32/ESP8266 devices and test the `sendToInternet()` functionality as a **regular node** (not a bridge).

### The Problem This Solves

Previous examples (`mock_server_test.ino`) tested the bridge making HTTP requests directly, but did **NOT** test the complete flow:

```
Regular Node → Mesh Network → Bridge → Internet → Bridge → Mesh Network → Regular Node
```

The **PC Mesh Node** solution allows you to:
- ✅ Test from a **regular mesh node** (no direct WiFi/Internet)
- ✅ Verify routing **through the bridge** works correctly
- ✅ Debug issues on a PC with full development tools
- ✅ Run on **Windows, Linux, or macOS**
- ✅ No need for multiple ESP32/ESP8266 devices for testing

## Architecture

```
┌──────────────────┐          ┌──────────────────┐          ┌──────────────────┐
│   PC Mesh Node   │          │  ESP32 Bridge    │          │  Mock HTTP       │
│  (Regular Node)  │◄────────►│  (Has WiFi)      │◄────────►│  Server          │
│  No WiFi         │  Mesh    │  Router Access   │  HTTP    │  localhost:8080  │
└──────────────────┘          └──────────────────┘          └──────────────────┘
       │                              │                              │
       │ sendToInternet()             │                              │
       │─────────────────────────────►│                              │
       │                              │ HTTP GET/POST                │
       │                              │─────────────────────────────►│
       │                              │                              │
       │                              │◄─────────────────────────────│
       │                              │ HTTP 200 OK                  │
       │◄─────────────────────────────│                              │
       │ Callback with result         │                              │
```

## Prerequisites

### Software Requirements

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libboost-dev libboost-system-dev
```

#### macOS
```bash
brew install cmake boost
```

#### Windows
1. Install [Visual Studio](https://visualstudio.microsoft.com/) with C++ support
2. Install [CMake](https://cmake.org/download/)
3. Install [Boost](https://www.boost.org/) or use vcpkg:
   ```powershell
   vcpkg install boost-asio boost-system
   ```

### Hardware Requirements

- **Bridge Node**: One ESP32 or ESP8266 with:
  - Router credentials configured
  - Running `sendToInternet.ino` with `IS_BRIDGE_NODE = true`
  - Connected to the same network as your PC

### Mock HTTP Server

For testing, start the mock HTTP server:

```bash
cd ../../test/mock-http-server
python3 server.py
```

The server will run on `http://localhost:8080` by default.

## Building

### Quick Build (Using make)

```bash
cd examples/sendToInternet

# Initialize dependencies (if not already done)
cd ../../test
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arkhipenko/TaskScheduler
cd ../examples/sendToInternet

# Build
cmake .
make

# Run
./pc_mesh_node <bridge_ip> <mesh_port>
```

### Using CMake

```bash
mkdir build
cd build
cmake ..
make
./pc_mesh_node 192.168.1.100 5555
```

### Manual Build (without CMake)

```bash
g++ -std=c++14 -o pc_mesh_node pc_mesh_node.cpp \
    -I../../src -I../../test/include -I../../test/ArduinoJson/src \
    -I../../test/TaskScheduler/src -I../../test/boost \
    ../../test/catch/fake_serial.cpp ../../src/scheduler.cpp \
    -lboost_system -pthread
```

## Usage

### Basic Usage

```bash
./pc_mesh_node <bridge_ip> <mesh_port>
```

**Example:**
```bash
./pc_mesh_node 192.168.1.100 5555
```

### Advanced Usage

Specify custom mock server location:

```bash
./pc_mesh_node <bridge_ip> <mesh_port> <mock_server_ip> <mock_server_port>
```

**Example:**
```bash
./pc_mesh_node 192.168.1.100 5555 192.168.1.50 8080
```

## Complete Testing Workflow

### Step 1: Start Mock HTTP Server

```bash
cd test/mock-http-server
python3 server.py

# Server output:
# Mock HTTP Server starting on 0.0.0.0:8080
# Server ready to accept connections
```

### Step 2: Configure and Upload Bridge Node

On your ESP32/ESP8266, upload `sendToInternet.ino` with:

```cpp
#define IS_BRIDGE_NODE  true          // This is the bridge
#define MESH_PREFIX     "TestMesh"
#define MESH_PASSWORD   "testpass123"
#define MESH_PORT       5555

#define ROUTER_SSID     "YourWiFiSSID"     // Your router
#define ROUTER_PASSWORD "YourWiFiPassword"
```

Wait for the bridge to connect and note its IP address from serial output.

### Step 3: Find Your PC's IP Address

**Linux/macOS:**
```bash
ifconfig | grep inet
# Or
ip addr show
```

**Windows:**
```powershell
ipconfig
```

### Step 4: Run PC Mesh Node

```bash
# If bridge IP is 192.168.1.100 and you're running mock server on same PC
./pc_mesh_node 192.168.1.100 5555

# If mock server is on a different machine (e.g., 192.168.1.50)
./pc_mesh_node 192.168.1.100 5555 192.168.1.50 8080
```

### Expected Output

```
============================================================
painlessMesh - PC Mesh Node Example
Testing sendToInternet() Through Bridge
============================================================

Configuration:
  Bridge:      192.168.1.100:5555
  Mock Server: http://127.0.0.1:8080

✓ PC Mesh Node initialized with ID: 1234567
✓ sendToInternet() API enabled
Connecting to bridge at 192.168.1.100:5555...
✓ Connected to bridge!

Waiting for mesh to establish...
🔄 Starting update loop (running for 10 seconds)...
✓ Update loop completed after 10 seconds

============================================================
Starting Automated Test Suite
============================================================

Waiting 5 seconds for mesh to stabilize...

[Test 1/5] HTTP 200 Success

📡 Testing sendToInternet()...
   URL: http://127.0.0.1:8080/status/200
   ✓ Bridge with Internet found
   ✓ Request queued with message ID: 2766733313
   Waiting for response from bridge...

📥 Response received:
   Success: ✓ YES
   HTTP Status: 200
   🎉 TEST PASSED: Request successful!

[Test 2/5] HTTP 404 Not Found
...
```

## What Gets Tested

The automated test suite validates:

### Test 1: HTTP 200 Success
- ✅ Request successfully routed through bridge
- ✅ HTTP 200 status code received
- ✅ Callback invoked with success=true

### Test 2: HTTP 404 Not Found
- ✅ Error handling works correctly
- ✅ HTTP 404 status code received
- ✅ Callback invoked with success=false

### Test 3: HTTP 500 Server Error
- ✅ Server error handling
- ✅ HTTP 500 status code received
- ✅ Proper error reporting

### Test 4: WhatsApp API Simulation
- ✅ Complex URL with query parameters
- ✅ URL encoding handled correctly
- ✅ Simulates real-world API call

### Test 5: JSON Payload Echo
- ✅ POST request with JSON payload
- ✅ Data correctly transmitted through bridge
- ✅ Response received and processed

## Troubleshooting

### "Failed to connect to bridge"

**Possible causes:**
1. Bridge IP address is incorrect
2. Bridge is not running or not on the network
3. Firewall blocking connections
4. Wrong mesh port

**Solutions:**
```bash
# Verify bridge is reachable
ping 192.168.1.100

# Check firewall (Linux)
sudo iptables -L

# Check if port is open (Linux/macOS)
nc -zv 192.168.1.100 5555

# Windows
Test-NetConnection -ComputerName 192.168.1.100 -Port 5555
```

### "No Internet connection available through bridge"

**Possible causes:**
1. Bridge lost WiFi connection
2. Bridge hasn't initialized yet
3. sendToInternet API not enabled on bridge

**Solutions:**
- Check bridge serial output
- Verify bridge shows "Is Bridge: YES"
- Wait 30-60 seconds after bridge startup
- Ensure `mesh.enableSendToInternet()` is called on bridge

### "Connection timeout after 30 seconds"

**Possible causes:**
1. Network connectivity issues
2. Mesh network not forming
3. Wrong mesh credentials

**Solutions:**
- Verify MESH_PREFIX and MESH_PASSWORD match
- Check both devices are on same network
- Try restarting both bridge and PC node

### Build Errors

#### "Boost not found"

```bash
# Ubuntu/Debian
sudo apt-get install libboost-dev libboost-system-dev

# macOS
brew install boost

# Windows
# Use vcpkg or download from boost.org
```

#### "ArduinoJson not found"

```bash
cd ../../test
git clone https://github.com/bblanchon/ArduinoJson.git
```

#### "TaskScheduler not found"

```bash
cd ../../test
git clone https://github.com/arkhipenko/TaskScheduler
```

## Testing on Different Platforms

### Linux

Should work out of the box with the dependencies installed:

```bash
cmake . && make
./pc_mesh_node 192.168.1.100 5555
```

### macOS

Same as Linux. Use Homebrew for dependencies:

```bash
brew install cmake boost
cmake . && make
./pc_mesh_node 192.168.1.100 5555
```

### Windows

#### Using Visual Studio
1. Open `CMakeLists.txt` in Visual Studio
2. Build → Build All
3. Run from PowerShell:
   ```powershell
   .\pc_mesh_node.exe 192.168.1.100 5555
   ```

#### Using MinGW
```bash
cmake -G "MinGW Makefiles" .
mingw32-make
pc_mesh_node.exe 192.168.1.100 5555
```

#### Using WSL (Windows Subsystem for Linux)
```bash
# In WSL terminal
sudo apt-get install cmake g++ libboost-dev libboost-system-dev
cmake . && make
./pc_mesh_node 192.168.1.100 5555
```

## Advanced Configuration

### Custom Mock Server Endpoints

Edit `pc_mesh_node.cpp` to test custom endpoints:

```cpp
// Test a custom endpoint
node.testSendToInternet("http://192.168.1.50:8080/custom-endpoint", 
                       "{\"custom\":\"payload\"}");
```

### Logging Levels

Adjust logging in `pc_mesh_node.cpp`:

```cpp
// More verbose logging
Log.setLogLevel(painlessmesh::logger::ERROR | 
               painlessmesh::logger::STARTUP | 
               painlessmesh::logger::CONNECTION |
               painlessmesh::logger::COMMUNICATION |
               painlessmesh::logger::GENERAL);

// Minimal logging (errors only)
Log.setLogLevel(painlessmesh::logger::ERROR);
```

### Custom Node ID

By default, the node ID is random. To use a fixed ID:

```cpp
uint32_t nodeId = 9999999;  // Fixed node ID
PCMeshNode node(&scheduler, nodeId, io_service);
```

## Integration with CI/CD

This PC mesh node can be integrated into automated testing:

```bash
#!/bin/bash
# Start mock server
python3 test/mock-http-server/server.py &
SERVER_PID=$!

# Build PC node
cd examples/sendToInternet
cmake . && make

# Run test (assuming bridge is running at known IP)
./pc_mesh_node 192.168.1.100 5555 127.0.0.1 8080

# Cleanup
kill $SERVER_PID
```

## Comparison with ESP Node Testing

| Feature | ESP32 Node | PC Mesh Node |
|---------|-----------|--------------|
| **Build Time** | 1-2 minutes | 5-10 seconds |
| **Upload Time** | 30-60 seconds | N/A (instant) |
| **Debugging** | Serial only | GDB, IDEs, etc. |
| **Logging** | Serial monitor | stdout, files |
| **Iteration Speed** | Slow | Fast |
| **Cost** | ~$5-10 per device | Free |
| **Setup** | Physical wiring | Just software |
| **Portability** | Fixed location | Run anywhere |

## Related Documentation

- [sendToInternet Example](sendToInternet.ino) - ESP32/ESP8266 example
- [Mock HTTP Server](../../test/mock-http-server/README.md) - Testing endpoint
- [Bridge Documentation](../../BRIDGE_TO_INTERNET.md) - Bridge setup guide
- [Testing Guide](../../test/mock-http-server/TESTING_GUIDE.md) - Complete testing workflow

## Credits

- **Issue Reporter**: @woodlist (Issue #337)
- **Implementation**: GitHub Copilot with painlessMesh team
- **Date**: December 26, 2024

## License

Part of the painlessMesh project. See main repository LICENSE.
