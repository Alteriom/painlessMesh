# 📖 API Reference (Doxygen)

> **Comprehensive API documentation automatically generated from the AlteriomPainlessMesh source code using Doxygen.**

<div class="doxygen-intro" style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 24px; border-radius: 8px; margin: 20px 0;">
  <h2 style="color: white; margin-top: 0;">🚀 Complete API Documentation</h2>
  <p style="margin-bottom: 0; opacity: 0.9;">Explore every class, function, and file in the AlteriomPainlessMesh library. Documentation is automatically generated from source code comments to ensure accuracy and completeness.</p>
</div>

## 🎯 Quick Access

<div class="quick-access-grid" style="display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 16px; margin: 24px 0;">
  
  <div style="background: #f8f9fa; border: 1px solid #e9ecef; border-radius: 8px; padding: 20px;">
    <h3 style="margin-top: 0; color: #495057;">📚 Classes</h3>
    <p style="color: #6c757d; margin-bottom: 16px;">Browse all classes, their methods, and inheritance relationships</p>
    <a href="doxygen/classes.md" style="background: #007bff; color: white; padding: 8px 16px; border-radius: 4px; text-decoration: none; display: inline-block;">Browse Classes</a>
  </div>
  
  <div style="background: #f8f9fa; border: 1px solid #e9ecef; border-radius: 8px; padding: 20px;">
    <h3 style="margin-top: 0; color: #495057;">⚙️ Functions</h3>
    <p style="color: #6c757d; margin-bottom: 16px;">Explore all functions, globals, and their detailed documentation</p>
    <a href="doxygen/functions.md" style="background: #28a745; color: white; padding: 8px 16px; border-radius: 4px; text-decoration: none; display: inline-block;">Browse Functions</a>
  </div>
  
  <div style="background: #f8f9fa; border: 1px solid #e9ecef; border-radius: 8px; padding: 20px;">
    <h3 style="margin-top: 0; color: #495057;">📁 Files</h3>
    <p style="color: #6c757d; margin-bottom: 16px;">Navigate the complete source code structure and file organization</p>
    <a href="doxygen/files.md" style="background: #6f42c1; color: white; padding: 8px 16px; border-radius: 4px; text-decoration: none; display: inline-block;">Browse Files</a>
  </div>
  
</div>

## 🔗 Direct API Access

For power users who prefer direct access to the Doxygen documentation:

| Section | Description | Link |
|---------|-------------|------|
| 🏠 **Main Index** | Complete API documentation homepage | [📖 Open Documentation](../api-reference/index.html) |
| 📋 **Class List** | All classes with brief descriptions | [🔍 Browse Classes](../api-reference/annotated.html) |
| ⚙️ **Function Index** | Global functions and utilities | [🔧 Browse Functions](../api-reference/globals_func.html) |
| 📁 **File Structure** | Source files and their contents | [📂 Browse Files](../api-reference/files.html) |
| 🌳 **Class Hierarchy** | Inheritance and relationships | [🌲 View Hierarchy](../api-reference/hierarchy.html) |

## 🎯 Core AlteriomPainlessMesh Classes

Quick links to the most important classes in the library:

### 🌐 Networking Core

| Class | Purpose | Documentation |
|-------|---------|---------------|
| `painlessMesh` | Main mesh network management | [📖 Documentation](../api-reference/classpainlessMesh.html) |
| `Scheduler` | Task scheduling and timing | [📖 Documentation](../api-reference/classScheduler.html) |
| `Task` | Individual task management | [📖 Documentation](../api-reference/classTask.html) |

### 📦 Alteriom Package System

| Package Type | Purpose | Documentation |
|--------------|---------|---------------|
| `SensorPackage` | Environmental data collection | [📖 Documentation](../api-reference/classalteriom_1_1SensorPackage.html) |
| `CommandPackage` | Device control commands | [📖 Documentation](../api-reference/classalteriom_1_1CommandPackage.html) |
| `StatusPackage` | System status monitoring | [📖 Documentation](../api-reference/classalteriom_1_1StatusPackage.html) |

### 🔌 Plugin Framework

| Component | Purpose | Documentation |
|-----------|---------|---------------|
| `SinglePackage` | Point-to-point messaging | [📖 Documentation](../api-reference/classsinglemessage_1_1SinglePackage.html) |
| `BroadcastPackage` | Network-wide broadcasts | [📖 Documentation](../api-reference/classbroadcastmessage_1_1BroadcastPackage.html) |
| `Variant` | Type-safe message container | [📖 Documentation](../api-reference/classprotocol_1_1Variant.html) |

## 🚀 API Usage Examples

### Basic Mesh Setup

```cpp
#include "AlteriomPainlessMesh.h"

painlessMesh mesh;
Scheduler userScheduler;

void setup() {
    // Initialize mesh - see painlessMesh class documentation
    mesh.init("MyMesh", "password", &userScheduler, 5555);
    
    // Set up callbacks - see callback documentation
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
}

void loop() {
    mesh.update(); // Keep the mesh running
}
```

### Alteriom Package Usage

```cpp
#include "AlteriomPainlessMesh.h"

void sendSensorData() {
    // Create sensor package - see SensorPackage documentation
    alteriom::SensorPackage sensor;
    sensor.temperature = 25.5;
    sensor.humidity = 60.0;
    sensor.sensorId = mesh.getNodeId();
    sensor.timestamp = mesh.getNodeTime();
    
    // Send to all nodes - see mesh.sendPackage() documentation
    mesh.sendPackage(&sensor);
}

void sendCommand(uint32_t targetNode) {
    // Create command package - see CommandPackage documentation  
    alteriom::CommandPackage cmd;
    cmd.command = 1; // Turn on LED
    cmd.targetDevice = targetNode;
    cmd.commandId = random(1000000);
    
    // Send to specific node - see mesh.sendSingle() documentation
    mesh.sendSingle(targetNode, cmd.toString());
}
```

## 📚 Documentation Features

### What's Included

- **🔍 Complete API Coverage** - Every public class, method, and function
- **📝 Detailed Descriptions** - Purpose, parameters, return values, and examples
- **🌳 Inheritance Diagrams** - Visual class relationships and hierarchies
- **🔗 Cross-References** - Easy navigation between related components
- **💡 Usage Examples** - Practical code snippets for common tasks
- **⚠️ Notes & Warnings** - Important usage information and gotchas

### Documentation Standards

All API documentation follows consistent standards:

- **Brief Description** - One-line summary of purpose
- **Detailed Description** - How it works and when to use it
- **Parameters** - All parameters with types and descriptions
- **Return Values** - What the function returns and possible errors
- **Examples** - Practical usage patterns
- **See Also** - Related functions and classes

## 🛠️ For Developers

### Contributing to Documentation

The API documentation is automatically generated from source code comments. To improve it:

1. **Edit source files** - Documentation is in `.h` and `.hpp` files
2. **Use Doxygen syntax** - Follow the existing comment patterns
3. **Test locally** - Run `doxygen doxygen/Doxyfile` to preview changes
4. **Submit PR** - Documentation updates deploy automatically

### Generating Documentation Locally

```bash
# Install Doxygen
sudo apt-get install doxygen graphviz  # Linux
brew install doxygen graphviz          # macOS

# Generate documentation
doxygen doxygen/Doxyfile

# Open documentation
open docs/api-reference/html/index.html
```

## 🔗 Related Resources

- [📖 Core API Guide](core-api.md) - High-level API overview
- [🎯 Configuration Guide](configuration.md) - Detailed setup options
- [🔄 Callbacks Guide](callbacks.md) - Event handling patterns
- [🛠️ Troubleshooting](../troubleshooting/common-issues.md) - Common API issues

---

?> **💡 Pro Tip**: The Doxygen documentation is searchable! Use Ctrl+F in the embedded viewers or the search functionality in the full documentation to quickly find what you need.