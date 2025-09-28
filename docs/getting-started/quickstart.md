# Quick Start Guide

Get your first painlessMesh network running in just a few minutes! This guide will walk you through creating a simple mesh network with two ESP8266 or ESP32 devices.

## What You'll Need

- 2 or more ESP8266 or ESP32 development boards
- Arduino IDE or PlatformIO
- USB cables for programming

## Step 1: Install painlessMesh

### Arduino IDE
1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search for "painlessMesh"
4. Install the latest version by "Coopdis"

### PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps = 
    painlessMesh
```

## Step 2: Basic Mesh Example

Copy this code to your Arduino IDE or create a new PlatformIO project:

```cpp
#include "painlessMesh.h"

#define   MESH_PREFIX     "MyMeshNetwork"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage(); // Prototype so PlatformIO doesn't complain

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void sendMessage() {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast(msg);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);

  // Set debug messages before init()
  mesh.setDebugMsgTypes(ERROR | STARTUP);

  // Initialize mesh
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Add task to scheduler
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}
```

## Step 3: Upload and Test

1. **Upload the code** to your first ESP8266/ESP32 device
2. **Open Serial Monitor** (115200 baud) to see debug messages
3. **Upload the same code** to your second device
4. Watch them automatically discover each other and start exchanging messages!

## What You Should See

In the Serial Monitor, you'll see output like:
```
startHere: New Connection, nodeId = 123456789
startHere: Received from 123456789 msg=Hello from node 123456789
Changed connections
Adjusted time 1234567. Offset = 12
```

## Key Concepts

- **Mesh Network**: All nodes automatically discover and connect to each other
- **Broadcasting**: Messages sent to all nodes in the network
- **Node ID**: Each device gets a unique identifier
- **Time Sync**: All nodes automatically synchronize their clocks
- **Self-Healing**: If nodes disconnect, the mesh automatically reorganizes

## Next Steps

Now that you have a basic mesh working:

1. **Add more nodes** - Upload the same code to additional devices
2. **Try different message types** - See [Custom Packages Tutorial](../tutorials/custom-packages.md)
3. **Add sensors** - Check out the [Sensor Networks Tutorial](../tutorials/sensor-networks.md)
4. **Explore Alteriom features** - Learn about [Alteriom Extensions](../alteriom/overview.md)

## Troubleshooting

**Nodes not connecting?**
- Make sure MESH_PREFIX and MESH_PASSWORD are identical on all devices
- Check that devices are within WiFi range
- Verify MESH_PORT is the same on all devices

**Serial output not showing?**
- Check baud rate is set to 115200
- Ensure USB cable supports data transfer
- Try pressing the reset button after upload

For more help, see our [Troubleshooting Guide](../troubleshooting/common-issues.md).

## Configuration Options

You can customize your mesh network by changing these parameters:

```cpp
// Network credentials
#define MESH_PREFIX     "YourNetworkName"    // Network name (SSID)
#define MESH_PASSWORD   "YourPassword"       // Network password
#define MESH_PORT       5555                 // TCP port for mesh communication

// Debug levels - combine with | operator
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

// Available debug types:
// ERROR, STARTUP, CONNECTION, SYNC, COMMUNICATION, GENERAL, MSG_TYPES, REMOTE
```

Ready to dive deeper? Check out our [Installation Guide](installation.md) for more advanced setup options!