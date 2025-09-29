# Core API Reference

This document provides a comprehensive reference for the painlessMesh core API. All functions and classes are part of the `painlessmesh` namespace unless otherwise specified.

## painlessMesh Class

The main class for mesh network functionality.

### Constructor

```cpp
painlessMesh mesh;
```

No parameters required. Creates a mesh instance ready for initialization.

### Core Methods

#### `init()`

Initialize the mesh network.

```cpp
void init(TSTRING meshPrefix, TSTRING meshPassword, uint16_t port);
void init(TSTRING meshPrefix, TSTRING meshPassword, Scheduler* userScheduler, uint16_t port);
```

**Parameters:**
- `meshPrefix` - Network name (SSID prefix)
- `meshPassword` - Network password  
- `userScheduler` - Optional external TaskScheduler instance
- `port` - TCP port for mesh communication (default: 5555)

**Example:**
```cpp
Scheduler userScheduler;
mesh.init("MyMesh", "password123", &userScheduler, 5555);
```

#### `update()`

Main mesh update loop. Call this in your `loop()` function.

```cpp
void update();
```

**Example:**
```cpp
void loop() {
    mesh.update(); // Always call this
}
```

### Message Sending

#### `sendBroadcast()`

Send a message to all nodes in the mesh.

```cpp
bool sendBroadcast(TSTRING msg, bool includeSelf = false);
```

**Parameters:**
- `msg` - Message to broadcast
- `includeSelf` - Whether to send to self (default: false)

**Returns:** `true` if message was queued successfully

**Example:**
```cpp
String msg = "Hello everyone!";
bool sent = mesh.sendBroadcast(msg);
if (!sent) {
    Serial.println("Failed to send broadcast");
}
```

#### `sendSingle()`

Send a message to a specific node.

```cpp
bool sendSingle(uint32_t destId, TSTRING msg);
```

**Parameters:**
- `destId` - Target node ID
- `msg` - Message to send

**Returns:** `true` if message was queued successfully

**Example:**
```cpp
uint32_t targetNode = 123456789;
String msg = "Hello specific node!";
bool sent = mesh.sendSingle(targetNode, msg);
```

### Plugin System

#### `sendPackage()`

Send a custom package through the plugin system.

```cpp
bool sendPackage(const protocol::PackageInterface* pkg);
```

**Parameters:**
- `pkg` - Pointer to package object

**Returns:** `true` if package was sent successfully

**Example:**
```cpp
SensorPackage sensor;
sensor.temperature = 25.0;
sensor.humidity = 60.0;
bool sent = mesh.sendPackage(&sensor);
```

#### `onPackage()`

Register a handler for specific package types.

```cpp
void onPackage(int type, std::function<bool(protocol::Variant&)> function);
```

**Parameters:**
- `type` - Package type ID to handle
- `function` - Handler function (return `true` to stop propagation)

**Example:**
```cpp
mesh.onPackage(SENSOR_DATA, [](protocol::Variant& variant) {
    SensorPackage sensor = variant.to<SensorPackage>();
    Serial.printf("Temp: %.1f°C\n", sensor.temperature);
    return false; // Don't stop propagation
});
```

### Node Information

#### `getNodeId()`

Get this node's unique identifier.

```cpp
uint32_t getNodeId();
```

**Returns:** 32-bit node ID

**Example:**
```cpp
uint32_t myId = mesh.getNodeId();
Serial.printf("My node ID: %u\n", myId);
```

#### `getNodeList()`

Get list of all connected nodes.

```cpp
std::list<uint32_t> getNodeList();
```

**Returns:** List of node IDs

**Example:**
```cpp
auto nodes = mesh.getNodeList();
Serial.printf("Connected to %d nodes\n", nodes.size());
for (auto nodeId : nodes) {
    Serial.printf("  Node: %u\n", nodeId);
}
```

#### `subConnectionJson()`

Get mesh topology as JSON string.

```cpp
TSTRING subConnectionJson(bool pretty = false);
```

**Parameters:**
- `pretty` - Whether to format JSON nicely (default: false)

**Returns:** JSON representation of mesh topology

**Example:**
```cpp
String topology = mesh.subConnectionJson(true);
Serial.println("Mesh topology:");
Serial.println(topology);
```

### Time Synchronization

#### `getNodeTime()`

Get synchronized mesh time.

```cpp
uint32_t getNodeTime();
```

**Returns:** Time in microseconds since mesh start

**Example:**
```cpp
uint32_t timestamp = mesh.getNodeTime();
uint32_t seconds = timestamp / 1000000;
Serial.printf("Mesh time: %u seconds\n", seconds);
```

### Task Management

#### `addTask()`

Add a task to the mesh scheduler.

```cpp
// Recurring task
std::shared_ptr<Task> addTask(Scheduler& scheduler, unsigned long interval,
                             long iterations, std::function<void()> callback);

// One-time task  
std::shared_ptr<Task> addTask(Scheduler& scheduler, std::function<void()> callback);
```

**Parameters:**
- `scheduler` - Scheduler instance
- `interval` - Task interval in milliseconds
- `iterations` - Number of iterations (`TASK_FOREVER` for infinite)
- `callback` - Function to execute

**Returns:** Shared pointer to task

**Example:**
```cpp
// Recurring task every 30 seconds
auto task = mesh.addTask(userScheduler, 30000, TASK_FOREVER, [](){
    Serial.println("Periodic task executed");
});

// One-time task
mesh.addTask(userScheduler, [](){
    Serial.println("One-time task executed");
});
```

### Callbacks

#### `onReceive()`

Set callback for receiving messages.

```cpp
void onReceive(std::function<void(uint32_t from, String& msg)> callback);
```

**Parameters:**
- `callback` - Function to call when message received

**Example:**
```cpp
mesh.onReceive([](uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
});
```

#### `onNewConnection()`

Set callback for new node connections.

```cpp
void onNewConnection(std::function<void(uint32_t nodeId)> callback);
```

**Example:**
```cpp
mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("New node connected: %u\n", nodeId);
});
```

#### `onDroppedConnection()`

Set callback for lost connections.

```cpp
void onDroppedConnection(std::function<void(uint32_t nodeId)> callback);
```

**Example:**
```cpp
mesh.onDroppedConnection([](uint32_t nodeId) {
    Serial.printf("Lost connection to: %u\n", nodeId);
});
```

#### `onChangedConnections()`

Set callback for topology changes.

```cpp
void onChangedConnections(std::function<void()> callback);
```

**Example:**
```cpp
mesh.onChangedConnections([]() {
    auto nodes = mesh.getNodeList();
    Serial.printf("Topology changed. Now connected to %d nodes\n", nodes.size());
});
```

#### `onNodeTimeAdjusted()`

Set callback for time synchronization events.

```cpp
void onNodeTimeAdjusted(std::function<void(int32_t offset)> callback);
```

**Parameters:**
- `offset` - Time adjustment in microseconds

**Example:**
```cpp
mesh.onNodeTimeAdjusted([](int32_t offset) {
    Serial.printf("Time adjusted by %d microseconds\n", offset);
});
```

### Network Testing

#### `startDelayMeas()`

Measure network delay to a specific node.

```cpp
bool startDelayMeas(uint32_t nodeId);
```

**Parameters:**
- `nodeId` - Target node for delay measurement

**Returns:** `true` if measurement started successfully

**Example:**
```cpp
uint32_t targetNode = 123456789;
if (mesh.startDelayMeas(targetNode)) {
    Serial.println("Delay measurement started");
}
```

#### `onNodeDelayReceived()`

Set callback for delay measurement results.

```cpp
void onNodeDelayReceived(std::function<void(uint32_t nodeId, int32_t delay)> callback);
```

**Parameters:**
- `nodeId` - Node that was measured
- `delay` - Round-trip delay in microseconds

**Example:**
```cpp
mesh.onNodeDelayReceived([](uint32_t nodeId, int32_t delay) {
    Serial.printf("Delay to node %u: %d µs\n", nodeId, delay);
});
```

### Debug and Diagnostics

#### `setDebugMsgTypes()`

Control debug message output.

```cpp
void setDebugMsgTypes(uint16_t types);
```

**Parameters:**
- `types` - Bitwise combination of debug types

**Debug Types:**
```cpp
#define ERROR         0x0001
#define STARTUP       0x0002  
#define CONNECTION    0x0004
#define SYNC          0x0008
#define COMMUNICATION 0x0010
#define GENERAL       0x0020
#define MSG_TYPES     0x0040
#define REMOTE        0x0080
```

**Example:**
```cpp
// Enable error, startup, and connection messages
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

// Enable all debug messages
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | SYNC | 
                     COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);
```

### Advanced Methods

#### `stop()`

Stop the mesh and clean up resources.

```cpp
void stop();
```

**Example:**
```cpp
void shutdown() {
    mesh.stop();
    Serial.println("Mesh stopped");
}
```

## Type Definitions

### TSTRING

Cross-platform string type that works on both ESP32/ESP8266 and desktop platforms.

```cpp
#if defined(ESP32) || defined(ESP8266)
    typedef String TSTRING;
#else
    typedef std::string TSTRING;
#endif
```

### Constants

```cpp
// Task scheduler constants
#define TASK_FOREVER -1
#define TASK_ONCE 1

// Common intervals
#define TASK_SECOND 1000
#define TASK_MINUTE (60 * TASK_SECOND)
#define TASK_HOUR   (60 * TASK_MINUTE)

// Default values
#define MESH_DEFAULT_PORT 5555
#define MAX_CONNECTIONS 4  // Platform dependent
```

## Usage Patterns

### Basic Setup Pattern

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX   "MyMeshNetwork"
#define MESH_PASSWORD "secretPassword"
#define MESH_PORT     5555

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
    Serial.begin(115200);
    
    // Set up callbacks
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    
    // Initialize mesh
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    
    // Set debug level
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
}

void loop() {
    mesh.update();
}
```

### Message Handling Pattern

```cpp
void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
    
    // Parse JSON if needed
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    
    if (doc["type"] == "sensor") {
        handleSensorData(doc);
    } else if (doc["type"] == "command") {
        handleCommand(doc);
    }
}
```

### Plugin Usage Pattern

```cpp
// Define custom package
class MyPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    float value = 0.0;
    
    MyPackage() : BroadcastPackage(100) {} // Unique type ID
    
    MyPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
        value = jsonObj["value"];
    }
    
    JsonObject addTo(JsonObject&& jsonObj) const override {
        jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
        jsonObj["value"] = value;
        return jsonObj;
    }
};

// Register handler
mesh.onPackage(100, [](protocol::Variant& variant) {
    MyPackage pkg = variant.to<MyPackage>();
    Serial.printf("Received value: %.2f\n", pkg.value);
    return false;
});

// Send package
MyPackage pkg;
pkg.value = 42.5;
mesh.sendPackage(&pkg);
```

## Error Handling

### Return Value Checking

```cpp
// Always check return values
if (!mesh.sendBroadcast(message)) {
    Serial.println("Failed to send broadcast - check connections");
}

if (!mesh.sendSingle(nodeId, message)) {
    Serial.println("Failed to send to specific node - node may be disconnected");
}
```

### Connection State Monitoring

```cpp
void monitorConnections() {
    auto nodes = mesh.getNodeList();
    if (nodes.empty()) {
        Serial.println("Warning: No mesh connections!");
    } else {
        Serial.printf("Connected to %d nodes\n", nodes.size());
    }
}
```

## Performance Considerations

### Memory Management

- Use `TSTRING::reserve()` for strings that will grow
- Implement `jsonObjectSize()` accurately in custom packages
- Monitor heap usage on ESP8266 (limited to ~80KB)

### Message Optimization

- Keep messages small to reduce network overhead
- Use binary data in JSON strings for large data
- Batch multiple values in single messages when possible

### Connection Limits

- ESP8266: Typically 2-4 connections maximum
- ESP32: Can handle 4-10 connections depending on memory
- Monitor connection count and implement backoff if needed

## Next Steps

- Explore [Plugin API](plugin-api.md) for advanced package handling
- Learn about [Configuration](configuration.md) options
- See [Callbacks](callbacks.md) for detailed event handling
- Check [Performance Optimization](../advanced/performance.md) guide