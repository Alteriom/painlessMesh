# API Reference (Doxygen)

This section contains the complete API reference automatically generated from the painlessMesh source code using Doxygen.

## 📖 Browse Full API Documentation

[🔗 **Open Complete API Documentation**](../api-reference/index.html ':target=_blank')

## Quick Navigation

### 📚 Main Documentation Sections

| Section | Description |
|---------|-------------|
| [📋 **Classes**](../api-reference/annotated.html) | All painlessMesh classes and their methods |
| [⚙️ **Functions**](../api-reference/globals_func.html) | Global functions and utilities |
| [📁 **Files**](../api-reference/files.html) | Source file documentation |
| [💡 **Examples**](../api-reference/examples.html) | Code examples and usage patterns |

### 🎯 Core Classes

| Class | Purpose | Documentation |
|-------|---------|---------------|
| `painlessMesh` | Main mesh network class | [📖 Documentation](../api-reference/classpainlessMesh.html) |
| `Scheduler` | Task scheduling system | [📖 Documentation](../api-reference/classScheduler.html) |
| `Task` | Individual task management | [📖 Documentation](../api-reference/classTask.html) |

### 🔌 Plugin System

| Component | Purpose | Documentation |
|-----------|---------|---------------|
| `SinglePackage` | Point-to-point message packages | [📖 Documentation](../api-reference/classsinglemessage_1_1SinglePackage.html) |
| `BroadcastPackage` | Broadcast message packages | [📖 Documentation](../api-reference/classbroadcastmessage_1_1BroadcastPackage.html) |
| `Variant` | Type-safe message container | [📖 Documentation](../api-reference/classprotocol_1_1Variant.html) |

## 🚀 Getting Started with API

### Basic Mesh Operations

```cpp
#include "painlessMesh.h"

painlessMesh mesh;
Scheduler userScheduler;

void setup() {
    // Initialize mesh - see painlessMesh::init() documentation
    mesh.init("MyMesh", "password", &userScheduler, 5555);
    
    // Set callbacks - see painlessMesh::onReceive() documentation  
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
}

void loop() {
    // Update mesh - see painlessMesh::update() documentation
    mesh.update();
}
```

### Message Sending

```cpp
// Send to all nodes - see painlessMesh::sendBroadcast() documentation
bool success = mesh.sendBroadcast("Hello everyone!");

// Send to specific node - see painlessMesh::sendSingle() documentation  
uint32_t targetNode = 123456789;
bool sent = mesh.sendSingle(targetNode, "Private message");

// Check if message was queued successfully
if (!sent) {
    Serial.println("Message failed to send");
}
```

### Network Information

```cpp
// Get this node's ID - see painlessMesh::getNodeId() documentation
uint32_t myId = mesh.getNodeId();

// Get connected nodes - see painlessMesh::getNodeList() documentation
auto nodes = mesh.getNodeList();
Serial.printf("Connected to %d nodes\n", nodes.size());

// Get network topology JSON - see painlessMesh::subConnectionJson() documentation
String topology = mesh.subConnectionJson();
Serial.println("Network topology: " + topology);
```

## 🔍 Advanced Features

### Time Synchronization

```cpp
// Get synchronized mesh time - see painlessMesh::getNodeTime() documentation
uint32_t meshTime = mesh.getNodeTime();

// Check if time is synchronized - see painlessMesh::isTimeAdjusted() documentation
if (mesh.isTimeAdjusted()) {
    Serial.println("Time is synchronized with mesh");
}
```

### Configuration Options

```cpp
// Set debug message types - see painlessMesh::setDebugMsgTypes() documentation
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

// Configure as root node - see painlessMesh::setRoot() documentation
mesh.setRoot(true);

// Set node timeout - see painlessMesh::setNodeTimeout() documentation
mesh.setNodeTimeout(30 * 1000000); // 30 seconds in microseconds
```

### Package System

```cpp
// Register custom package handler - see painlessMesh::onPackage() documentation
mesh.onPackage(200, [](protocol::Variant& variant) {
    // Handle custom package type 200
    return true; // Package handled
});

// Send custom package - see painlessMesh::sendPackage() documentation
MyCustomPackage package;
package.data = "custom data";
mesh.sendPackage(&package);
```

## 📊 Performance Monitoring

### Memory Usage

```cpp
// Monitor memory in your application
void checkMemory() {
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 10000) {
        Serial.printf("Low memory warning: %u bytes\n", freeHeap);
    }
}
```

### Network Health

```cpp
// Monitor connection count
void monitorNetwork() {
    auto nodes = mesh.getNodeList();
    Serial.printf("Network health: %d connections\n", nodes.size());
    
    // Log individual node IDs
    for (auto nodeId : nodes) {
        Serial.printf("Connected to: %u\n", nodeId);
    }
}
```

## 🛠️ Troubleshooting with API

### Debug Information

```cpp
// Enable verbose debugging
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | SYNC | COMMUNICATION);

// Check mesh status
bool isConnected = mesh.getNodeList().size() > 0;
Serial.printf("Mesh status: %s\n", isConnected ? "Connected" : "Isolated");
```

### Error Handling

```cpp
// Validate message sending
bool sendMessage(String message) {
    bool success = mesh.sendBroadcast(message);
    if (!success) {
        Serial.println("Failed to send message - check network status");
        
        // Check network health
        if (mesh.getNodeList().size() == 0) {
            Serial.println("No connections - node is isolated");
        }
        
        // Check memory
        if (ESP.getFreeHeap() < 5000) {
            Serial.println("Low memory may be causing send failures");
        }
    }
    return success;
}
```

## 📝 Documentation Notes

### Automatic Generation

The API documentation is automatically generated from the source code using Doxygen. This ensures:

- **Always up-to-date** - Reflects the latest code
- **Complete coverage** - All public APIs documented
- **Detailed examples** - Usage patterns and code samples
- **Cross-references** - Easy navigation between related functions

### Documentation Standards

All painlessMesh APIs follow these documentation standards:

- **Brief description** - What the function/class does
- **Detailed description** - How it works and when to use it
- **Parameters** - All parameters with types and descriptions
- **Return values** - What the function returns
- **Examples** - Practical usage examples
- **See also** - Related functions and classes

### Contributing to Documentation

If you find missing or incorrect API documentation:

1. **Check the source code** - Documentation is in the `.h` files
2. **Submit an issue** - Report documentation problems
3. **Contribute improvements** - Submit pull requests with documentation fixes

## 🔗 Related Resources

- [Core API Guide](core-api.md) - High-level API overview with examples
- [Configuration Guide](configuration.md) - Detailed configuration options
- [Callbacks Guide](callbacks.md) - Event handling and callbacks
- [Troubleshooting](../troubleshooting/common-issues.md) - Common API usage issues

The Doxygen API reference provides the most comprehensive and detailed documentation for all painlessMesh functionality. Use it alongside the guides above for complete understanding of the library.