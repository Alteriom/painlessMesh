# API Reference

This page contains the complete API reference for the Alteriom painlessMesh Library.

## Core Classes

### painlessMesh

The main class for creating and managing mesh networks.

#### Initialization Methods

```cpp
void init(String ssid, String password, Scheduler *baseScheduler, uint16_t port = 5555);
void setDebugMsgTypes(uint16_t types);
```

#### Network Management

```cpp
void update();                                    // Call in loop()
bool sendBroadcast(String &msg);                // Send to all nodes
bool sendSingle(uint32_t &destId, String &msg); // Send to specific node
```

#### Event Callbacks

```cpp
void onReceive(std::function<void(uint32_t from, String &msg)> cb);
void onNewConnection(std::function<void(uint32_t nodeId)> cb);
void onChangedConnections(std::function<void()> cb);
void onNodeTimeAdjusted(std::function<void(int32_t offset)> cb);
```

#### Node Information

```cpp
uint32_t getNodeId();                    // Get this node's ID
std::list<uint32_t> getNodeList();      // Get all connected nodes
uint32_t getNodeTime();                  // Get synchronized time
size_t connectionCount();                // Number of direct connections
```

## Alteriom Extensions

### SensorPackage (Type 200)

Environmental data collection package for sensor networks.

```cpp
class SensorPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    uint32_t sensorId = 0;        // Unique sensor identifier
    uint32_t timestamp = 0;       // Unix timestamp
    double temperature = 0.0;     // Temperature in Celsius
    double humidity = 0.0;        // Relative humidity (0-100%)
    double pressure = 0.0;        // Atmospheric pressure in hPa
    uint8_t batteryLevel = 0;     // Battery level (0-100%)
    
    // Constructors
    SensorPackage();
    SensorPackage(JsonObject jsonObj);
    
    // Serialization
    JsonObject addTo(JsonObject&& jsonObj) const;
    size_t jsonObjectSize() const;
};
```

**Usage Example:**
```cpp
SensorPackage sensor;
sensor.sensorId = mesh.getNodeId();
sensor.timestamp = mesh.getNodeTime();
sensor.temperature = 23.5;
sensor.humidity = 65.0;
sensor.pressure = 1013.25;
sensor.batteryLevel = 85;

// Send as broadcast
auto variant = painlessmesh::protocol::Variant(&sensor);
String message;
variant.printTo(message);
mesh.sendBroadcast(message);
```

### CommandPackage (Type 201)

Device control and automation commands for remote node management.

```cpp
class CommandPackage : public painlessmesh::plugin::SinglePackage {
public:
    uint8_t command = 0;          // Command type identifier
    uint32_t targetDevice = 0;    // Target node ID
    uint32_t commandId = 0;       // Unique command identifier
    TSTRING parameters = "";      // JSON parameters string
    
    // Constructors
    CommandPackage();
    CommandPackage(JsonObject jsonObj);
    
    // Serialization
    JsonObject addTo(JsonObject&& jsonObj) const;
    size_t jsonObjectSize() const;
};
```

**Usage Example:**
```cpp
CommandPackage cmd;
cmd.dest = targetNodeId;
cmd.command = 1;              // LED control
cmd.targetDevice = targetNodeId;
cmd.commandId = millis();
cmd.parameters = "{\"state\":\"ON\",\"brightness\":75}";

// Send to specific node
auto variant = painlessmesh::protocol::Variant(&cmd);
String message;
variant.printTo(message);
mesh.sendSingle(targetNodeId, message);
```

### StatusPackage (Type 202)

Health monitoring and system status reporting for network diagnostics.

```cpp
class StatusPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    uint8_t deviceStatus = 0;         // Device status flags
    uint32_t uptime = 0;              // Uptime in seconds
    uint16_t freeMemory = 0;          // Free memory in KB
    uint8_t wifiStrength = 0;         // WiFi signal strength (0-100)
    TSTRING firmwareVersion = "";     // Firmware version string
    
    // Constructors
    StatusPackage();
    StatusPackage(JsonObject jsonObj);
    
    // Serialization
    JsonObject addTo(JsonObject&& jsonObj) const;
    size_t jsonObjectSize() const;
};
```

**Usage Example:**
```cpp
StatusPackage status;
status.deviceStatus = 0x01;   // Operational
status.uptime = millis() / 1000;
status.freeMemory = ESP.getFreeHeap() / 1024;
status.wifiStrength = 75;
status.firmwareVersion = "1.6.1-alteriom";

// Broadcast status
auto variant = painlessmesh::protocol::Variant(&status);
String message;
variant.printTo(message);
mesh.sendBroadcast(message);
```

## Message Handling

### Receiving Messages

```cpp
void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    JsonObject obj = doc.as<JsonObject>();
    uint8_t msgType = obj["type"];
    
    switch(msgType) {
        case 200: // SensorPackage
            handleSensorData(alteriom::SensorPackage(obj), from);
            break;
        case 201: // CommandPackage  
            handleCommand(alteriom::CommandPackage(obj), from);
            break;
        case 202: // StatusPackage
            handleStatus(alteriom::StatusPackage(obj), from);
            break;
        default:
            Serial.printf("Unknown message type: %d\n", msgType);
    }
}
```

### Debug Message Types

```cpp
// Debug levels (can be combined with |)
#define ERROR           0x00000001
#define STARTUP         0x00000002  
#define CONNECTION      0x00000004
#define SYNC            0x00000008
#define COMMUNICATION   0x00000010
#define GENERAL         0x00000020
#define MSG_TYPES       0x00000040
#define REMOTE          0x00000080

// Usage
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
```

## Platform Constants

### Memory Constraints
- **ESP32**: ~320KB RAM available
- **ESP8266**: ~80KB RAM available
- **MAX_CONN**: Typically 4-10 connections depending on platform

### Network Limits
- **Message Size**: Recommended max 4KB
- **String Length**: Recommended max 256 characters for compatibility
- **Network Delay**: ~50-200ms typical mesh routing delay

## Error Handling

### Common Return Values
- **`true`**: Operation successful
- **`false`**: Operation failed (check network/memory)

### Best Practices
```cpp
// Always check return values
if (!mesh.sendBroadcast(message)) {
    Serial.println("Failed to send message");
    // Implement retry logic or error handling
}

// Monitor connection changes
void changedConnectionCallback() {
    auto nodeList = mesh.getNodeList();
    Serial.printf("Network size: %d nodes\n", nodeList.size() + 1);
}
```

## See Also

- [Installation Guide](Installation)
- [Examples](Examples)
- [GitHub Repository](https://github.com/Alteriom/painlessMesh)
- [Release Notes](https://github.com/Alteriom/painlessMesh/releases)