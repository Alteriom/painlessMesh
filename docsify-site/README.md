# painlessMesh Documentation

> A painless way to setup a mesh with ESP8266 and ESP32 devices

[![Version](https://img.shields.io/github/v/release/Alteriom/painlessMesh)](https://github.com/Alteriom/painlessMesh/releases)
[![License](https://img.shields.io/github/license/Alteriom/painlessMesh)](https://github.com/Alteriom/painlessMesh/blob/main/LICENSE)
[![Downloads](https://img.shields.io/github/downloads/Alteriom/painlessMesh/total)](https://github.com/Alteriom/painlessMesh/releases)

## What is painlessMesh?

painlessMesh is a library that handles all the complexities of mesh networking for you:

- 🔗 **Automatic routing** - Messages find the best path automatically
- 🕒 **Time synchronization** - All nodes share synchronized time
- 📡 **Self-healing** - Network adapts when nodes join or leave  
- 💬 **JSON messaging** - Simple, structured communication
- 🛠️ **Easy setup** - Just a few lines of code to get started

## Quick Start

Get your first mesh network running in minutes:

```cpp
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

## Alteriom Extensions

This fork includes **Alteriom-specific packages** for common IoT scenarios:

- 🌡️ **SensorPackage** - Environmental data collection and transmission
- 🎮 **CommandPackage** - Device control and coordination
- 📊 **StatusPackage** - Health monitoring and diagnostics

## Key Features

### Zero Configuration Networking
No need to assign IPs or configure routes - the mesh handles everything automatically.

### JSON-Based Messaging  
Send structured data between nodes using familiar JSON syntax.

### Time Synchronization
All nodes maintain synchronized time for coordinated behaviors like light shows.

### Self-Healing Network
Automatically adapts when devices join, leave, or lose connection.

## Supported Hardware

| Platform | Status | Notes |
|----------|--------|-------|
| ESP32 | ✅ Full Support | Recommended for new projects |
| ESP8266 | ✅ Full Support | Memory limitations on large meshes |
| ESP32-S2/S3 | ✅ Experimental | Limited testing |
| ESP32-C3 | ✅ Experimental | Limited testing |

## Installation

<!-- tabs:start -->

#### **Arduino IDE**

1. Open **Tools** → **Manage Libraries**
2. Search for "painlessMesh"
3. Install the latest version
4. Install dependencies: ArduinoJson, TaskScheduler

#### **PlatformIO**

Add to your `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    Alteriom/painlessMesh@^1.6.0
    bblanchon/ArduinoJson@^6.21.3
    arkhipenko/TaskScheduler@^3.7.0
```

<!-- tabs:end -->

## Documentation Navigation

- 📚 **[Getting Started](getting-started/)** - Installation and first steps
- 🔧 **[API Reference](api/)** - Complete API documentation  
- 🌟 **[Alteriom Extensions](alteriom/)** - IoT-specific packages
- 📖 **[Tutorials](tutorials/)** - Step-by-step guides
- 🏗️ **[Architecture](architecture/)** - How painlessMesh works
- 🚀 **[Advanced Topics](advanced/)** - Performance and optimization
- ❓ **[Troubleshooting](troubleshooting/)** - Common issues and solutions

## Community & Support

- 📁 **GitHub Repository**: [Alteriom/painlessMesh](https://github.com/Alteriom/painlessMesh)
- 🐛 **Issues**: [Report bugs and request features](https://github.com/Alteriom/painlessMesh/issues)
- 💬 **Discussions**: [Community discussions](https://github.com/Alteriom/painlessMesh/discussions)
- 📖 **Original Library**: [gmag11/painlessMesh](https://github.com/gmag11/painlessMesh)

## License

This project is licensed under the [MIT License](https://github.com/Alteriom/painlessMesh/blob/main/LICENSE).

---

*Ready to build your mesh network? Start with the [Installation Guide](getting-started/installation.md)!*