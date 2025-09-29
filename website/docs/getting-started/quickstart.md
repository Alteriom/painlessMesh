---
sidebar_position: 2
---

# Quick Start

Get your painlessMesh network running in **under 5 minutes**!

## Prerequisites

- ESP32 or ESP8266 development board
- Arduino IDE or PlatformIO installed
- USB cable for programming

## Step 1: Install painlessMesh

### Arduino IDE
1. Open **Tools** → **Manage Libraries**
2. Search for "painlessMesh"
3. Click **Install**

### PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps = gmag11/painlessMesh@^1.6.0
```

## Step 2: Basic Mesh Example

Create a new sketch with this minimal mesh code:

```cpp title="quick_mesh.ino"
#include "painlessMesh.h"

#define MESH_PREFIX     "QuickMesh"
#define MESH_PASSWORD   "meshpass123"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Set callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  
  Serial.println("Mesh network initialized!");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
}

void loop() {
  mesh.update();
  
  // Send test message every 5 seconds
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 5000) {
    String msg = "Hello from node " + String(mesh.getNodeId());
    mesh.sendBroadcast(msg);
    lastSend = millis();
  }
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received: '%s' from node %u\n", msg.c_str(), from);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Network topology changed. Nodes: %d\n", 
                mesh.getNodeList().size());
}
```

## Step 3: Upload and Test

1. **Upload** the sketch to your first ESP32/ESP8266
2. **Open Serial Monitor** (115200 baud)
3. You should see: `Mesh network initialized!`

## Step 4: Add More Nodes

1. **Upload the same sketch** to additional devices
2. They will **automatically discover** each other
3. Watch the **Serial Monitor** for connection messages
4. See **broadcast messages** from all nodes

## What You'll See

```
Mesh network initialized!
Node ID: 123456789
New connection: 987654321
Network topology changed. Nodes: 2
Received: 'Hello from node 987654321' from node 987654321
```

## Next Steps

🎉 **Congratulations!** You have a working mesh network!

Now you can:
- [Learn the Core API](../api/core-api) - Discover all mesh features
- [Try Advanced Examples](../tutorials/basic-examples) - More complex scenarios
- [Build Sensor Networks](../alteriom/packages) - Use Alteriom packages

## Troubleshooting

**Not connecting?**
- Check that all devices use the same `MESH_PREFIX` and `MESH_PASSWORD`
- Ensure devices are within WiFi range (~30 meters)
- Try different `MESH_PORT` if there's interference

**Need help?** Check our [FAQ](../troubleshooting/faq) or [Common Issues](../troubleshooting/common-issues).