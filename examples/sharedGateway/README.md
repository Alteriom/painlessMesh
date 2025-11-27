# Shared Gateway Example

This example demonstrates the **Shared Gateway Mode** feature of painlessMesh. Unlike Bridge Mode where only one node connects to the router, Shared Gateway Mode allows **ALL nodes** to connect to the same WiFi router while maintaining mesh connectivity.

## What is Shared Gateway Mode?

In a traditional painlessMesh network:
- Regular nodes only communicate over the mesh
- Bridge nodes connect to a router AND the mesh
- Only the bridge can access the Internet

With Shared Gateway Mode:
- **All nodes** connect to the same WiFi router
- **All nodes** maintain mesh connectivity
- **Any node** can access the Internet directly
- Provides redundant Internet paths

## Architecture

```
                    Internet
                       │
                    Router
         ┌─────────────┼─────────────┐
         │             │             │
     Node A        Node B        Node C
     (AP+STA)      (AP+STA)      (AP+STA)
         │             │             │
         └─────────────┼─────────────┘
                 Mesh Network
```

## Hardware Requirements

- **ESP32** or **ESP8266** microcontroller
- WiFi router within range of all nodes
- USB cable for programming and serial monitoring

## Configuration

Edit the following defines in `sharedGateway.ino`:

```cpp
// Mesh network settings (must match across all nodes)
#define MESH_PREFIX     "SharedGatewayMesh"  // Your mesh network name
#define MESH_PASSWORD   "meshPassword123"    // Your mesh network password
#define MESH_PORT       5555                 // TCP port (default: 5555)

// Router settings (your WiFi router)
#define ROUTER_SSID     "YourRouterSSID"     // Your WiFi router name
#define ROUTER_PASSWORD "YourRouterPassword" // Your WiFi router password
```

## Building and Uploading

### Using PlatformIO (Recommended)

```bash
# For ESP32
pio run -e esp32 -t upload

# For ESP8266
pio run -e esp8266 -t upload
```

### Using Arduino IDE

1. Open `sharedGateway.ino` in Arduino IDE
2. Install the required libraries:
   - ArduinoJson
   - TaskScheduler
   - painlessMesh
   - AsyncTCP (ESP32) or ESPAsyncTCP (ESP8266)
3. Select your board (ESP32 or ESP8266)
4. Click Upload

## Expected Serial Output

After uploading, open Serial Monitor at 115200 baud. You should see:

```
================================================
   painlessMesh - Shared Gateway Example
================================================

Initializing Shared Gateway Mode...

  Mesh SSID: SharedGatewayMesh
  Mesh Port: 5555
  Router SSID: YourRouterSSID

=== Shared Gateway Mode Initialization ===
Step 1: Scanning for router YourRouterSSID to detect channel...
✓ Router connected on channel 6
✓ Router IP: 192.168.1.105
Step 2: Initializing mesh on channel 6...
Step 3: Establishing router connection in shared gateway mode...
=== Shared Gateway Mode Active ===
  Mesh Prefix: SharedGatewayMesh
  Mesh Channel: 6 (synced with router)
  Router: YourRouterSSID
  Port: 5555
  Mode: AP+STA (all nodes can connect to router)

✓ Shared Gateway Mode initialized successfully!

================================================
Node ID: 2748965421
Shared Gateway Mode: Active
Router IP: 192.168.1.105
Router Channel: 6
================================================

Status broadcasts will be sent every 10 seconds.
```

### Status Broadcast Output

Every 10 seconds, you'll see:

```
========== STATUS BROADCAST ==========
Node ID: 2748965421
Router: Connected
  IP: 192.168.1.105
  RSSI: -45 dBm
  Channel: 6
Mesh Nodes: 2
Shared Gateway Mode: Yes
Free Heap: 45320 bytes
Uptime: 120 seconds
=======================================
```

## Use Cases

### Industrial Sensor Networks
All sensor nodes can independently send data to cloud services, providing redundancy if any individual node loses connection.

### Fish Farm Monitoring
Critical O2 alarms can reach the cloud through any available path, ensuring no alerts are missed.

### Smart Building Systems
HVAC and security data maintains connectivity through redundant paths for maximum reliability.

### Remote Monitoring
Environmental monitoring stations with shared WiFi infrastructure can relay through nearby nodes if needed.

## Troubleshooting

### Router Connection Fails

**Symptoms:** `✗ Failed to initialize Shared Gateway Mode!`

**Solutions:**
1. Verify router SSID and password are correct
2. Ensure the router is powered on and within range
3. Check that the router is operating on 2.4GHz (5GHz not supported)
4. Try moving the node closer to the router

### Mesh Not Forming

**Symptoms:** `Mesh Nodes: 0` even with multiple nodes

**Solutions:**
1. Ensure all nodes use the same `MESH_PREFIX` and `MESH_PASSWORD`
2. Verify all nodes are using the same router (same channel)
3. Check that nodes are within WiFi range of each other
4. Restart all nodes simultaneously

### Frequent Router Disconnections

**Symptoms:** Router status alternates between Connected/Disconnected

**Solutions:**
1. Check router signal strength (RSSI should be > -70 dBm)
2. Move nodes closer to the router
3. Reduce number of devices on the router
4. Check for WiFi interference

### Low Memory Warnings

**Symptoms:** `Free Heap` decreasing or node crashes

**Solutions:**
1. Reduce JSON document sizes in your code
2. Limit the number of mesh nodes
3. Use ESP32 instead of ESP8266 for larger networks

## API Reference

### Key Method

```cpp
bool initAsSharedGateway(
  String meshPrefix,      // Mesh network name
  String meshPassword,    // Mesh network password
  String routerSSID,      // Router SSID to connect to
  String routerPassword,  // Router password
  Scheduler* scheduler,   // Task scheduler
  uint16_t port = 5555    // TCP port (optional)
);
```

**Returns:** `true` if initialization succeeded, `false` if router connection failed.

### Status Methods

```cpp
// Check if shared gateway mode is active
bool isSharedGatewayMode();

// Get router connection status
WiFi.status() == WL_CONNECTED

// Get router IP address
WiFi.localIP()

// Get router signal strength
WiFi.RSSI()

// Get current WiFi channel
WiFi.channel()
```

## Related Documentation

- [Shared Gateway Design Document](../../docs/design/SHARED_GATEWAY_DESIGN.md)
- [Bridge Mode Example](../bridge/README.md)
- [Bridge Failover Example](../bridge_failover/README.md)
- [painlessMesh Documentation](https://gitlab.com/painlessMesh/painlessMesh/-/wikis/home)

## License

This example is part of painlessMesh and is released under the same license.
