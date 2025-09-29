# Core API

This section covers the main painlessMesh API functions and classes.

## painlessMesh Class

The main class that handles all mesh networking functionality.

### Constructor

```cpp
painlessMesh mesh;
```

### Initialization

```cpp
void init(String ssid, String password, uint16_t port = 5555)
void init(String ssid, String password, Scheduler* scheduler, uint16_t port = 5555)
```

**Parameters:**
- `ssid` - Network name for the mesh
- `password` - Network password  
- `scheduler` - Optional TaskScheduler instance
- `port` - TCP port for mesh communication (default: 5555)

**Example:**
```cpp
#include "painlessMesh.h"

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
    mesh.init("MyMesh", "password123", &userScheduler, 5555);
}
```

## Core Functions

### mesh.update()

Must be called in your main loop to handle mesh networking.

```cpp
void loop() {
    mesh.update();
}
```

### Sending Messages

#### Broadcast Messages

Send a message to all nodes in the mesh:

```cpp
bool sendBroadcast(String msg)
bool sendBroadcast(String msg, bool include_self)
```

**Parameters:**
- `msg` - Message to send (JSON string recommended)
- `include_self` - Whether to include this node in broadcast

**Example:**
```cpp
String msg = "{\"sensor\":\"temperature\",\"value\":23.5}";
mesh.sendBroadcast(msg);
```

#### Single Node Messages

Send a message to a specific node:

```cpp
bool sendSingle(uint32_t dest, String msg)
```

**Parameters:**
- `dest` - Destination node ID
- `msg` - Message to send

**Example:**
```cpp
uint32_t targetNode = 123456789;
String command = "{\"action\":\"toggle_led\"}";
mesh.sendSingle(targetNode, command);
```

### Node Information

#### Get Node ID

```cpp
uint32_t getNodeId()
```

Returns the unique ID of this node.

#### Get Connected Nodes

```cpp
std::list<uint32_t> getNodeList()
```

Returns a list of all connected node IDs.

**Example:**
```cpp
auto nodes = mesh.getNodeList();
Serial.printf("Connected nodes: %d\n", nodes.size());

for (auto node : nodes) {
    Serial.printf("Node: %u\n", node);
}
```

### Time Synchronization

#### Get Mesh Time

```cpp
uint32_t getNodeTime()
```

Returns synchronized time across all mesh nodes.

**Example:**
```cpp
uint32_t meshTime = mesh.getNodeTime();
Serial.printf("Mesh time: %u\n", meshTime);
```

## Callback Functions

### Message Reception

Set callback for incoming messages:

```cpp
void onReceive(receivedCallback_t callback)
```

**Callback signature:**
```cpp
void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received: %s from %u\n", msg.c_str(), from);
}
```

### Connection Events

#### New Connection

```cpp
void onNewConnection(newConnectionCallback_t callback)
```

**Callback signature:**
```cpp
void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
}
```

#### Changed Connections

```cpp
void onChangedConnections(changedConnectionCallback_t callback)
```

**Callback signature:**
```cpp
void changedConnectionCallback() {
    Serial.printf("Network topology changed\n");
    auto nodes = mesh.getNodeList();
    Serial.printf("Connected nodes: %d\n", nodes.size());
}
```

#### Dropped Connection

```cpp
void onDroppedConnection(droppedConnectionCallback_t callback)
```

**Callback signature:**
```cpp
void droppedConnectionCallback(uint32_t nodeId) {
    Serial.printf("Lost connection: %u\n", nodeId);
}
```

## Configuration

### Debug Messages

Control debug output:

```cpp
void setDebugMsgTypes(uint16_t types)
```

**Debug types:**
- `ERROR` - Error messages
- `STARTUP` - Startup information
- `CONNECTION` - Connection events
- `COMMUNICATION` - Message transmission
- `GENERAL` - General information
- `MSG_TYPES` - Message type information
- `REMOTE` - Remote debugging
- `MESH_STATUS` - Mesh status updates
- `SYNC` - Time synchronization

**Example:**
```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
```

### Network Settings

#### Set AP Credentials

```cpp
void setAP(String ssid, String password)
```

#### Set Station Credentials  

```cpp
void setSTANetworkInfo(String ssid, String password)
```

## Complete Example

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMesh"
#define MESH_PASSWORD   "password123"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
    Serial.begin(115200);
    
    // Configure debug output
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    
    // Initialize mesh
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    
    // Set callbacks
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onDroppedConnection(&droppedConnectionCallback);
    
    Serial.println("Mesh network initialized");
}

void loop() {
    mesh.update();
    
    // Send periodic status
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 10000) {
        String msg = "{\"type\":\"status\",\"uptime\":" + String(millis()) + "}";
        mesh.sendBroadcast(msg);
        lastSend = millis();
    }
}

void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received: %s from %u\n", msg.c_str(), from);
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
    Serial.printf("Total nodes: %d\n", mesh.getNodeList().size());
}

void changedConnectionCallback() {
    Serial.println("Network topology changed");
}

void droppedConnectionCallback(uint32_t nodeId) {
    Serial.printf("Lost connection: %u\n", nodeId);
}
```

## Next Steps

- [📞 Callbacks](callbacks.md) - Detailed callback documentation
- [⚙️ Configuration](configuration.md) - Advanced configuration options
- [🌟 Alteriom Packages](../alteriom/packages.md) - Extended functionality