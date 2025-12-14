# Bridging painlessMesh to Internet via Router

You can bridge your mesh network to the Internet by creating a **gateway node** that connects to both the mesh network and your WiFi router simultaneously.

## Quick Start (Recommended: Auto Channel Detection)

The **bridge-centric approach** automatically detects your router's channel and configures the mesh accordingly. No manual channel configuration required!

### Resilient Initialization (v1.9.7+)

**Power-up order no longer matters!** The bridge will initialize successfully even if:
- Router is not yet powered on
- Internet connection is unavailable
- Router is temporarily offline

The bridge will:
- Establish the mesh network immediately
- Accept connections from mesh nodes right away
- Retry router connection automatically in the background
- Update status when router/Internet becomes available

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

// Your router credentials
#define ROUTER_SSID     "YourRouterSSID"
#define ROUTER_PASSWORD "YourRouterPassword"

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Single call does everything:
  // 1. Attempts to connect to router and detect its channel
  // 2. Initializes mesh on detected channel (or default if router unavailable)
  // 3. Sets node as root/bridge
  // 4. Maintains/retries router connection automatically
  mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                    ROUTER_SSID, ROUTER_PASSWORD,
                    &userScheduler, MESH_PORT);
  
  mesh.onReceive(&receivedCallback);
}

void loop() {
  mesh.update();
}

void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("Received from mesh node %u: %s\n", from, msg.c_str());
  // Forward to Internet services (HTTP, MQTT, etc.)
}
```

**Expected Output (Router Available):**
```
=== Bridge Mode Initialization ===
Step 1: Attempting to connect to router YourRouterSSID...
✓ Router connected on channel 6
✓ Router IP: 192.168.1.100
Step 2: Initializing mesh on channel 6...
STARTUP: init(): Mesh channel set to 6
Step 3: Establishing bridge connection...
=== Bridge Mode Active ===
  Mesh SSID: MyMeshNetwork
  Mesh Channel: 6 (matches router)
  Router: YourRouterSSID (connected)
  Port: 5555
```

**Expected Output (Router Unavailable but Visible):**
```
=== Bridge Mode Initialization ===
Step 1: Attempting to connect to router YourRouterSSID...
⚠ Router connection unavailable during initialization
⚠ Scanning for router 'YourRouterSSID' to detect channel...
✓ Router found on channel 6 (not connected, will retry)
⚠ Proceeding with bridge setup on channel 6
⚠ Bridge will retry router connection in background
Step 2: Initializing mesh on channel 6...
STARTUP: init(): Mesh channel set to 6
Step 3: Establishing bridge connection...
=== Bridge Mode Active ===
  Mesh SSID: MyMeshNetwork
  Mesh Channel: 6 (default, router pending)
  Router: YourRouterSSID (will retry)
  Port: 5555

INFO: Bridge initialized without router connection
INFO: Mesh network is active and accepting node connections
INFO: Router connection will be established automatically when available
```

**Note:** If the router cannot connect but is visible in a WiFi scan, the bridge detects its channel and uses it for the mesh. This minimizes channel switching when the router becomes connectable. If the router is completely invisible (powered off), channel 1 is used as default.

### Regular Nodes with Auto-Detection

Regular mesh nodes can also auto-detect the mesh channel:

```cpp
void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // channel=0 means auto-detect
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, 
            WIFI_AP_STA, 0);
  
  mesh.onReceive(&receivedCallback);
}
```

**Expected Output:**
```
STARTUP: Auto-detecting mesh channel...
CONNECTION: Scanning all channels for mesh 'MyMeshNetwork'...
CONNECTION: Found mesh on channel 6 (RSSI: -45)
STARTUP: Mesh channel auto-detected: 6
```

### Automatic Channel Re-synchronization

Nodes automatically follow the mesh if the bridge changes channels:

- When nodes can't find the mesh on their current channel for ~30 seconds, they trigger a full channel scan
- If the mesh is found on a different channel, nodes automatically switch to that channel
- This ensures the mesh stays connected even if the bridge switches channels (e.g., during bridge election)

For detailed information about channel synchronization, see [Channel Synchronization Documentation](docs/CHANNEL_SYNCHRONIZATION.md).

## Manual Configuration (Legacy Approach)

If you prefer the traditional approach or need more control, you can still manually configure the channel:

```cpp
void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Initialize mesh with AP+STA mode on specific channel
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6);
  
  // Connect to your router
  mesh.stationManual(ROUTER_SSID, ROUTER_PASSWORD);
  mesh.setHostname("MESH_BRIDGE");
  
  // Configure as root/bridge node
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  
  mesh.onReceive(&receivedCallback);
}
```

## Important Requirements

### WiFi Channel Behavior

#### With initAsBridge() (Recommended)

The new `initAsBridge()` method automatically handles all channel detection and configuration:

1. Connects to your router first in STA mode
2. Detects the router's actual channel
3. Initializes the mesh AP on the detected channel
4. Maintains both connections on the same channel

**No manual channel configuration needed!** Just provide your router and mesh credentials.

#### With Manual Configuration

When using the legacy `stationManual()` approach, the library will automatically handle channel switching. The ESP32/ESP8266 will:

1. Initially operate the mesh AP on your specified channel (e.g., channel 6)
2. Automatically switch to the router's channel when connecting via `stationManual()`
3. The mesh AP channel will adjust to match the router's channel

**Note:** While ESP32/ESP8266 hardware can only operate on one channel at a time in AP+STA mode, the WiFi stack automatically coordinates this. When connected to a router on a different channel, the mesh AP will operate on that channel instead.

**Best Practices:**

- Use `initAsBridge()` for new projects - it handles everything automatically
- Use channels 1, 6, or 11 (non-overlapping 2.4GHz channels) if not using a router
- Regular nodes should use `channel=0` to auto-detect the mesh
- For optimal performance, you may choose to configure your router to use your preferred mesh channel

### Other Requirements

1. **WIFI_AP_STA Mode**: This enables simultaneous AP (for mesh) and Station (for router) operation.

2. **Root Configuration**:
   - Call `mesh.setRoot(true)` on the bridge node
   - Call `mesh.setContainsRoot(true)` on all mesh nodes for optimal routing

3. **ESP32-C6 Compatibility**: If using ESP32-C6 or experiencing crashes with `tcp_alloc` errors, ensure you have AsyncTCP v3.3.0+ installed. See the [ESP32-C6 Compatibility Guide](docs/troubleshooting/ESP32_C6_COMPATIBILITY.md) for details.

## Complete Examples

We provide several working bridge examples in the repository:

- **Basic Bridge**: `examples/bridge/bridge.ino`
- **MQTT Bridge**: `examples/mqttBridge/mqttBridge.ino` - Bridges mesh to MQTT broker
- **Web Server Bridge**: `examples/webServer/webServer.ino` - Provides web interface
- **Enhanced MQTT Bridge**: `examples/bridge/enhanced_mqtt_bridge_example.ino` - Advanced MQTT integration with metrics and health monitoring

## Forwarding Data to Internet

Once the bridge is established, you can forward mesh data to Internet services:

```cpp
void receivedCallback(uint32_t from, String& msg) {
  // Check if connected to router
  if (WiFi.status() == WL_CONNECTED) {
    // Forward to MQTT broker
    mqttClient.publish("mesh/data", msg.c_str());
    
    // Or send via HTTP
    HTTPClient http;
    http.begin("http://myserver.com/api/data");
    http.POST(msg);
    http.end();
  }
}
```

## Architecture Diagram

```text
Internet
   |
Router (WiFi)
   |
Bridge Node (AP+STA mode)
   |
Mesh Network
 / | \
Node1 Node2 Node3...
```

## Frequently Asked Questions

### Why does `mesh.init()` require a separate `mesh.stationManual()` call?

Great question! The library now offers **three ways** to connect a bridge:

1. **Original**: `init()` + `stationManual()` (most flexible)
2. **Convenience**: Pass credentials directly to `init()` (new feature)
3. **Modern**: Use `initAsBridge()` with auto-detection (recommended)

See [Station Credentials Design Rationale](docs/design/STATION_CREDENTIALS_DESIGN.md) for detailed explanations and comparisons.

## Additional Resources

- [painlessMesh Wiki](https://github.com/Alteriom/painlessMesh/wiki)
- [Bridge Examples](https://github.com/Alteriom/painlessMesh/tree/main/examples/bridge)
- [MQTT Bridge Example](https://github.com/Alteriom/painlessMesh/tree/main/examples/mqttBridge)
- [Configuration API Reference](https://github.com/Alteriom/painlessMesh/wiki)
- [Station Credentials Design](docs/design/STATION_CREDENTIALS_DESIGN.md) - Why three approaches exist

Feel free to ask if you need help with specific use cases like MQTT integration, web servers, or custom data forwarding!
