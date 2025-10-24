# Complete Documentation

Welcome to the complete documentation for Alteriom painlessMesh Library!

## 📖 Documentation Sections

### Getting Started
- [Installation Guide](Installation) - Set up the library in your development environment
- [Quick Start](https://github.com/Alteriom/painlessMesh/blob/main/docs/getting-started/quickstart.md) - Your first mesh network
- [First Mesh Tutorial](https://github.com/Alteriom/painlessMesh/blob/main/docs/getting-started/first-mesh.md) - Step-by-step guide

### API Documentation
- [API Reference](API-Reference) - Complete class and method documentation
- [Core API](https://github.com/Alteriom/painlessMesh/blob/main/docs/api/core-api.md) - painlessMesh core functionality
- [Plugin System](https://github.com/Alteriom/painlessMesh/blob/main/docs/architecture/plugin-system.md) - Extensible architecture

### Examples and Tutorials
- [Examples](Examples) - Working code examples
- [Basic Examples](https://github.com/Alteriom/painlessMesh/blob/main/docs/tutorials/basic-examples.md) - Fundamental usage patterns
- [Alteriom Extensions](https://github.com/Alteriom/painlessMesh/blob/main/docs/alteriom/overview.md) - SensorPackage, CommandPackage, StatusPackage

### Architecture
- [Mesh Architecture](https://github.com/Alteriom/painlessMesh/blob/main/docs/architecture/mesh-architecture.md) - How the mesh network works
- [Plugin System](https://github.com/Alteriom/painlessMesh/blob/main/docs/architecture/plugin-system.md) - Extensible package system

### Troubleshooting
- [FAQ](https://github.com/Alteriom/painlessMesh/blob/main/docs/troubleshooting/faq.md) - Frequently asked questions
- [Common Issues](https://github.com/Alteriom/painlessMesh/blob/main/docs/troubleshooting/common-issues.md) - Solutions to common problems

### Development
- [Contributing Guidelines](Contributing) - How to contribute to the project
- [Release Guide](https://github.com/Alteriom/painlessMesh/blob/main/RELEASE_GUIDE.md) - Release process documentation

## 🚀 Quick Links

### Installation
```bash
# Arduino Library Manager
Tools → Manage Libraries → Search "Alteriom PainlessMesh"

# PlatformIO
lib_deps = alteriom/painlessMesh@^1.6.1

# NPM
npm install @alteriom/painlessmesh
```

### Basic Usage
```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "YourMeshName"
#define MESH_PASSWORD   "YourPassword"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive([](uint32_t from, String& msg) {
    Serial.printf("Received: %s from %u\n", msg.c_str(), from);
  });
}

void loop() {
  mesh.update();
}
```

## 📦 Package Types

### SensorPackage (Type 200)
For environmental sensor data collection:
- Temperature, humidity, pressure measurements
- Battery level monitoring
- Timestamp synchronization

### CommandPackage (Type 400)
For device control and automation:
- Remote device commands
- Parameter passing via JSON
- Command acknowledgment tracking

### StatusPackage (Type 202)
For system health monitoring:
- Device operational status
- Memory usage tracking
- Network connectivity metrics

## 🌟 Key Features

- **Automatic Mesh Formation** - Nodes discover and connect automatically
- **JSON-Based Messaging** - Easy to use and extend
- **Time Synchronization** - Coordinated behaviors across nodes
- **Multi-Platform Support** - ESP32, ESP8266
- **Alteriom Extensions** - Enhanced packages for common use cases
- **Comprehensive Testing** - Full CI/CD pipeline with automated testing

## 📚 External Resources

- **GitHub Repository**: https://github.com/Alteriom/painlessMesh
- **NPM Package**: https://www.npmjs.com/package/@alteriom/painlessmesh
- **PlatformIO Registry**: https://registry.platformio.org/libraries/alteriom/painlessMesh
- **Issue Tracker**: https://github.com/Alteriom/painlessMesh/issues
- **Discussions**: https://github.com/Alteriom/painlessMesh/discussions

## 📝 License

This project is licensed under LGPL-3.0 - see the [LICENSE](https://github.com/Alteriom/painlessMesh/blob/main/LICENSE) file for details.

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](Contributing) for details on:
- Code style and standards
- Pull request process
- Issue reporting
- Feature requests

---

**Need Help?** Check the [FAQ](https://github.com/Alteriom/painlessMesh/blob/main/docs/troubleshooting/faq.md) or [open an issue](https://github.com/Alteriom/painlessMesh/issues).