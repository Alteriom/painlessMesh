# 📁 File Reference

> Complete file documentation automatically generated from painlessMesh source code using Doxygen.

## 🖥️ Interactive Documentation Viewer

<div class="api-viewer" style="background: #f6f8fa; border: 1px solid #d0d7de; border-radius: 6px; padding: 16px; margin: 16px 0;">
  <div style="margin-bottom: 12px; display: flex; justify-content: space-between; align-items: center;">
    <strong>📁 API Reference - Files</strong>
    <a href="../../api-reference/files.html" target="_blank" style="color: #0969da; text-decoration: none; font-size: 14px;">
      🔗 Open in New Tab
    </a>
  </div>
  
  <iframe 
    src="../../api-reference/files.html" 
    width="100%" 
    height="700px" 
    frameborder="0"
    style="border: 1px solid #d0d7de; border-radius: 6px; background: white;"
    loading="lazy"
    title="Doxygen File Documentation">
    <p style="padding: 20px; text-align: center; color: #656d76;">
      📄 Your browser doesn't support iframes. 
      <a href="../../api-reference/files.html" target="_blank">Open file documentation in new tab</a>
    </p>
  </iframe>
</div>

## 🔗 Quick Navigation

| File Category | Description | Direct Link |
|---------------|-------------|-------------|
| **All Files** | Complete file listing | [📖 Browse All](../../api-reference/files.html) |
| **Header Files** | Public API headers | [📋 View Headers](../../api-reference/files.html) |
| **Source Files** | Implementation files | [🔧 Browse Source](../../api-reference/files.html) |
| **File Index** | Alphabetical file listing | [📁 File Index](../../api-reference/files.html) |

## 🎯 Key Files by Category

### 🌐 Core Library Files

**Main Headers**
- `painlessMesh.h` - Primary library interface
- `AlteriomPainlessMesh.h` - Alteriom extensions wrapper
- `painlessMeshSTA.h` - Station mode functionality

**Core Implementation**
- `mesh.hpp` - Core mesh networking implementation  
- `protocol.hpp` - Mesh communication protocols
- `router.hpp` - Message routing and forwarding

### 📦 Plugin System Files

**Package Framework**
- `plugin.hpp` - Plugin system architecture
- `SinglePackage` classes - Point-to-point messaging
- `BroadcastPackage` classes - Network-wide broadcasts

### 🔧 Utility Files

**Supporting Infrastructure**
- `logger.hpp` - Debug and logging utilities
- `buffer.hpp` - Memory management
- `configuration.hpp` - Settings and options

### 🎯 Alteriom Extensions

**IoT Packages**
- `alteriom_sensor_package.hpp` - Sensor data packages
- Example files - Practical usage demonstrations

## 💡 File Documentation Guide

### 🔍 Understanding File Documentation

Each file entry includes:
- **File overview** with purpose and role
- **Include dependencies** and requirements
- **Public API** exposed by the file
- **Class and function listings** defined in the file
- **Usage examples** for key functionality
- **Source code links** for implementation details

### 📖 File Organization

**Header Files (.h/.hpp)**
- **Public interfaces** - Classes and functions for library users
- **API declarations** - Function signatures and class definitions  
- **Documentation** - Detailed usage information and examples

**Source Files (.cpp)**
- **Implementation details** - How the library actually works
- **Private functions** - Internal helper functions
- **Platform-specific code** - ESP32/ESP8266 optimizations

### 🔧 Include Hierarchy

```cpp
// Main include for users
#include "AlteriomPainlessMesh.h"

// Core mesh functionality  
#include "painlessMesh.h"

// Alteriom extensions
#include "examples/alteriom/alteriom_sensor_package.hpp"

// Internal headers (advanced users)
#include "painlessmesh/mesh.hpp"
#include "painlessmesh/protocol.hpp"
```

## 🚀 File Usage Patterns

### 📋 Basic Integration

**Simple Mesh Project**
```cpp
#include "AlteriomPainlessMesh.h"

painlessMesh mesh;
Scheduler userScheduler;

void setup() {
    mesh.init("MyMesh", "password", &userScheduler, 5555);
}
```

**Custom Extensions**
```cpp
#include "painlessmesh/plugin.hpp"

class CustomPackage : public painlessmesh::plugin::SinglePackage {
    // Custom message implementation
};
```

### 🔍 Advanced Usage

**Direct Core Access**
```cpp
#include "painlessmesh/mesh.hpp"
#include "painlessmesh/protocol.hpp"

// Access lower-level mesh functionality
```

**Platform Optimization**
```cpp
#ifdef ESP32
    #include "arduino/wifi.hpp"
#endif
```

## 📊 File Metrics

### 📈 Library Structure

| Category | File Count | Purpose |
|----------|------------|---------|
| **Public Headers** | ~5 files | User-facing API |
| **Core Implementation** | ~15 files | Mesh networking |
| **Plugin System** | ~8 files | Extensible messaging |
| **Utilities** | ~10 files | Support functions |
| **Examples** | ~12 files | Usage demonstrations |

### 🔧 Include Guidelines

**For Basic Usage**
- Include `AlteriomPainlessMesh.h` only
- Use high-level API functions
- Follow example patterns

**For Advanced Development**
- Include specific core headers as needed
- Understand file dependencies
- Review implementation details

## 🚀 Next Steps

After exploring the file documentation:

1. **Study examples** - See file usage in [Basic Examples](../../tutorials/basic-examples.md)
2. **Review classes** - Understand context in [Class Reference](classes.md)
3. **Check functions** - Browse [Function Reference](functions.md)
4. **Read guides** - Learn patterns in [Core API Guide](../core-api.md)

?> **💡 Tip**: Use the file documentation to understand the library structure and find the right headers for your project. Each file page shows exactly what functionality it provides.