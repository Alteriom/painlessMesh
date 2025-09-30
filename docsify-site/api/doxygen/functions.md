# ⚙️ Function Reference

> Complete function documentation automatically generated from painlessMesh source code using Doxygen.

## �️ Interactive Documentation Viewer

<div class="api-viewer" style="background: #f6f8fa; border: 1px solid #d0d7de; border-radius: 6px; padding: 16px; margin: 16px 0;">
  <div style="margin-bottom: 12px; display: flex; justify-content: space-between; align-items: center;">
    <strong>⚙️ API Reference - Functions</strong>
    <a href="../../api-reference/globals_func.html" target="_blank" style="color: #0969da; text-decoration: none; font-size: 14px;">
      🔗 Open in New Tab
    </a>
  </div>
  
  <iframe 
    src="../../api-reference/globals_func.html" 
    width="100%" 
    height="700px" 
    frameborder="0"
    style="border: 1px solid #d0d7de; border-radius: 6px; background: white;"
    loading="lazy"
    title="Doxygen Function Documentation">
    <p style="padding: 20px; text-align: center; color: #656d76;">
      📄 Your browser doesn't support iframes. 
      <a href="../../api-reference/globals_func.html" target="_blank">Open function documentation in new tab</a>
    </p>
  </iframe>
</div>

## 🔗 Quick Navigation

| Function Category | Description | Direct Link |
|------------------|-------------|-------------|
| **All Functions** | Complete function listing | [📖 Browse All](../../api-reference/globals_func.html) |
| **Global Variables** | Module-level variables | [🌐 View Globals](../../api-reference/globals_vars.html) |
| **Defines & Macros** | Preprocessor definitions | [🔧 Browse Defines](../../api-reference/globals_defs.html) |
| **Enumerations** | Enum values and types | [📋 View Enums](../../api-reference/globals_enum.html) |

## 🎯 Key Functions by Category

### 🌐 Mesh Operations
- **Initialization**: `mesh.init()`, `mesh.stop()`
- **Messaging**: `mesh.sendBroadcast()`, `mesh.sendSingle()`
- **Network Info**: `mesh.getNodeList()`, `mesh.getNodeId()`

### ⏰ Time & Scheduling  
- **Time Sync**: `mesh.getNodeTime()`, `mesh.isTimeAdjusted()`
- **Task Management**: `scheduler.addTask()`, `task.enable()`

### 🔧 Configuration
- **Debug Settings**: `mesh.setDebugMsgTypes()`
- **Network Config**: `mesh.setRoot()`, `mesh.setNodeTimeout()`

### 📦 Package Handling
- **Package Registration**: `mesh.onPackage()`
- **Message Processing**: `variant.to<T>()`, `package.addTo()`

## 💡 Function Usage Guide

### 🔍 Understanding Function Documentation

Each function entry includes:
- **Function signature** with parameter types
- **Brief description** of what it does
- **Detailed description** of behavior and usage
- **Parameter documentation** with types and descriptions
- **Return value details** including error conditions
- **Usage examples** showing practical implementation

### 📖 Common Patterns

**Mesh Initialization**
```cpp
// Basic mesh setup
mesh.init("NetworkName", "password", &scheduler, 5555);
mesh.onReceive(&messageCallback);
mesh.onNewConnection(&connectionCallback);
```

**Message Handling**
```cpp
// Send to all nodes
bool success = mesh.sendBroadcast("Hello everyone!");

// Send to specific node
uint32_t targetId = 123456789;
bool sent = mesh.sendSingle(targetId, "Private message");
```

**Task Scheduling**
```cpp
// Create and schedule task
Task myTask(1000, TASK_FOREVER, &taskCallback);
scheduler.addTask(myTask);
myTask.enable();
```

### ⚡ Performance Tips

- **Check return values** - Many functions return success/failure status
- **Use callbacks efficiently** - Avoid blocking operations in mesh callbacks
- **Monitor memory** - Large message queues can cause memory issues
- **Handle errors gracefully** - Network operations can fail

## 🔧 Advanced Function Reference

### 🎚️ Configuration Functions

| Function | Purpose | Parameters |
|----------|---------|------------|
| `setDebugMsgTypes()` | Control debug output | Message type flags |
| `setRoot()` | Configure as root node | Boolean root status |
| `setNodeTimeout()` | Set connection timeout | Timeout in microseconds |

### 📊 Information Functions

| Function | Purpose | Return Type |
|----------|---------|-------------|
| `getNodeId()` | Get this node's ID | `uint32_t` |
| `getNodeList()` | Get connected nodes | `std::list<uint32_t>` |
| `getNodeTime()` | Get synchronized time | `uint32_t` |

### 🔄 Callback Functions

| Callback | Triggered When | Signature |
|----------|----------------|-----------|
| `onReceive` | Message received | `void(uint32_t from, String& msg)` |
| `onNewConnection` | Node connects | `void(uint32_t nodeId)` |
| `onChangedConnections` | Topology changes | `void()` |

## 🚀 Next Steps

After exploring the function documentation:

1. **Try examples** - See functions in action in [Basic Examples](../../tutorials/basic-examples.md)
2. **Read class docs** - Understand context in [Class Reference](classes.md)
3. **Check file structure** - Browse [File Reference](files.md)
4. **Review guides** - Learn patterns in [Core API Guide](../core-api.md)

?> **💡 Tip**: Use the search functionality in the embedded documentation to quickly find specific functions. If the iframe doesn't load, use the direct links provided above.