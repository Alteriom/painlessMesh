# Bridging painlessMesh to Internet via Router

You can bridge your mesh network to the Internet by creating a **gateway node** that connects to both the mesh network and your WiFi router simultaneously.

## Quick Start

The gateway node operates in **AP+STA mode** (Access Point + Station), allowing it to:
- Act as an Access Point for the mesh network
- Connect as a Station to your router for Internet access

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

// Your router credentials
#define STATION_SSID     "YourRouterSSID"
#define STATION_PASSWORD "YourRouterPassword"

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  
  // Initialize mesh with AP+STA mode
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6);
  
  // Connect to your router
  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname("MESH_BRIDGE");
  
  // Configure as root/bridge node
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  
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

## Important Requirements

1. **WiFi Channel Matching**: The mesh network and your router **must use the same WiFi channel**. In the example above, both use channel 6. You may need to configure your router to use a fixed channel.

2. **WIFI_AP_STA Mode**: This enables simultaneous AP (for mesh) and Station (for router) operation.

3. **Root Configuration**: 
   - Call `mesh.setRoot(true)` on the bridge node
   - Call `mesh.setContainsRoot(true)` on all mesh nodes for optimal routing

4. **ESP32-C6 Compatibility**: If using ESP32-C6 or experiencing crashes with `tcp_alloc` errors, ensure you have AsyncTCP v3.3.0+ installed. See the [ESP32-C6 Compatibility Guide](docs/troubleshooting/ESP32_C6_COMPATIBILITY.md) for details.

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

```
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

## Additional Resources

- [painlessMesh Wiki](https://github.com/Alteriom/painlessMesh/wiki)
- [Bridge Examples](https://github.com/Alteriom/painlessMesh/tree/main/examples/bridge)
- [MQTT Bridge Example](https://github.com/Alteriom/painlessMesh/tree/main/examples/mqttBridge)
- [Configuration API Reference](https://github.com/Alteriom/painlessMesh/wiki)

Feel free to ask if you need help with specific use cases like MQTT integration, web servers, or custom data forwarding!
