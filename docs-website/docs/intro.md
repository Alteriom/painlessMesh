---
sidebar_position: 1
---

# painlessMesh Documentation

Welcome to **painlessMesh** - the easiest way to create mesh networks with ESP32 and ESP8266 devices!

## What is painlessMesh?

painlessMesh is a library that handles all the complexities of mesh networking for you:

- 🔗 **Automatic routing** - Messages find the best path
- 🕒 **Time synchronization** - All nodes share the same time
- 📡 **Self-healing** - Network adapts when nodes join or leave  
- 💬 **JSON messaging** - Simple, structured communication
- 🛠️ **Easy setup** - Just a few lines of code to get started

## Key Features

### Zero Configuration Networking
No need to assign IPs or configure routes - the mesh handles everything automatically.

### JSON-Based Messaging  
Send structured data between nodes using familiar JSON syntax.

### Time Synchronization
All nodes maintain synchronized time for coordinated behaviors.

### Self-Healing Network
Automatically adapts when devices join, leave, or lose connection.

## Alteriom Extensions

This fork includes **Alteriom-specific packages** for common IoT scenarios:

- 🌡️ **SensorPackage** - Environmental data collection
- 🎮 **CommandPackage** - Device control and coordination
- 📊 **StatusPackage** - Health monitoring and diagnostics

## Quick Start

Get your first mesh network running in minutes:

```bash
# Install via PlatformIO
pio lib install "gmag11/painlessMesh"

# Or use Arduino Library Manager
# Search for "painlessMesh" and install
```

```cpp title="basic_mesh.ino"
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMesh"
#define MESH_PASSWORD   "password123"  
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  
  Serial.println("Mesh network started!");
}

void loop() {
  mesh.update();
}

void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("Received: %s from %u\n", msg.c_str(), from);
}
```

## What's Next?

import DocCardList from '@theme/DocCardList';

<DocCardList />
