# Alteriom Packages

Alteriom provides production-ready, type-safe packages for painlessMesh that handle common IoT communication patterns. These packages eliminate the need for manual JSON handling and provide compile-time type safety.

## Available Packages

### SensorPackage (Type 200)

For broadcasting environmental sensor data across the mesh network.

```cpp
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

SensorPackage sensor;
sensor.temperature = 23.5;      // °C
sensor.humidity = 65.0;         // %
sensor.pressure = 1013.25;      // hPa
sensor.sensorId = 1001;         // Unique sensor identifier
sensor.timestamp = mesh.getNodeTime();
sensor.batteryLevel = 85;       // %

// Send to all nodes
mesh.sendPackage(&sensor);
```

**Fields:**
- `temperature` (double) - Temperature in Celsius
- `humidity` (double) - Relative humidity percentage
- `pressure` (double) - Atmospheric pressure in hPa
- `sensorId` (uint32_t) - Unique sensor identifier
- `timestamp` (uint32_t) - Measurement timestamp
- `batteryLevel` (uint8_t) - Battery level percentage

### CommandPackage (Type 400)

For sending control commands to specific devices in the mesh. (Type moved from 201 to 400 in v1.7.7+)

```cpp
CommandPackage cmd;
cmd.dest = targetNodeId;        // Target node ID
cmd.command = 1;                // Command type (LED_CONTROL)
cmd.targetDevice = 100;         // Device ID (LED strip)
cmd.parameters = "{\"brightness\":75,\"color\":\"blue\"}";
cmd.commandId = generateCommandId();

// Send to specific node
mesh.sendPackage(&cmd);
```

**Fields:**
- `command` (uint8_t) - Command type identifier
- `targetDevice` (uint32_t) - Target device ID
- `commandId` (uint32_t) - Unique command identifier for tracking
- `parameters` (TSTRING) - JSON parameters for the command

### StatusPackage (Type 202)

For broadcasting device health and operational status.

```cpp
StatusPackage status;
status.deviceStatus = 1;        // OPERATIONAL
status.uptime = millis() / 1000;
status.freeMemory = ESP.getFreeHeap();
status.wifiStrength = WiFi.RSSI();
status.firmwareVersion = "1.2.3";

// Broadcast to all nodes
mesh.sendPackage(&status);
```

**Fields:**
- `deviceStatus` (uint8_t) - Device status flags
- `uptime` (uint32_t) - Device uptime in seconds
- `freeMemory` (uint16_t) - Available memory in bytes
- `wifiStrength` (uint8_t) - WiFi signal strength
- `firmwareVersion` (TSTRING) - Firmware version string

## Package Handlers

Register handlers to process incoming packages:

```cpp
void setup() {
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    
    // Register package handlers
    mesh.onPackage(200, handleSensorData);
    mesh.onPackage(400, handleCommand);  // Updated to 400 in v1.7.7+
    mesh.onPackage(202, handleStatus);
}

void handleSensorData(protocol::Variant& variant) {
    SensorPackage sensor = variant.to<SensorPackage>();
    
    Serial.printf("Sensor %u: T=%.1f°C, H=%.1f%%, P=%.1f hPa\n",
                 sensor.sensorId, sensor.temperature, 
                 sensor.humidity, sensor.pressure);
    
    // Process sensor data
    if (sensor.temperature > 30.0) {
        triggerCooling();
    }
    
    if (sensor.batteryLevel < 20) {
        Serial.printf("Low battery warning: Node %u, Sensor %u\n", 
                     sensor.from, sensor.sensorId);
    }
}

void handleCommand(protocol::Variant& variant) {
    CommandPackage cmd = variant.to<CommandPackage>();
    
    // Check if command is for this node
    if (cmd.dest != mesh.getNodeId()) {
        return;
    }
    
    Serial.printf("Command %u for device %u: %u\n",
                 cmd.commandId, cmd.targetDevice, cmd.command);
    
    // Execute command
    executeDeviceCommand(cmd);
    
    // Send acknowledgment
    sendCommandAcknowledgment(cmd);
}

void handleStatus(protocol::Variant& variant) {
    StatusPackage status = variant.to<StatusPackage>();
    
    Serial.printf("Node %u status: %s, uptime %u seconds\n",
                 status.from, 
                 status.deviceStatus == 1 ? "OK" : "ERROR",
                 status.uptime);
    
    // Monitor network health
    if (status.freeMemory < 10000) {
        Serial.printf("Low memory warning: Node %u has %u bytes\n",
                     status.from, status.freeMemory);
    }
}
```

## Type Safety Benefits

### Compile-Time Validation

```cpp
// ✅ Correct - compiler validates types
sensor.temperature = 25.5;
sensor.sensorId = 1001;

// ❌ Compiler error - type mismatch
sensor.temperature = "invalid";
sensor.sensorId = "not a number";
```

### IDE Support

Modern IDEs provide autocompletion and type hints:

```cpp
sensor.| // IDE shows: temperature, humidity, pressure, sensorId, etc.
```

### Automatic Serialization

No manual JSON handling required:

```cpp
// Automatic conversion to/from JSON
mesh.sendPackage(&sensor);  // Automatically serializes

// In handler
SensorPackage received = variant.to<SensorPackage>();  // Automatically deserializes
```

## Advanced Usage Patterns

### Command Types

Define application-specific command constants:

```cpp
enum DeviceCommands {
    LED_CONTROL = 1,
    SERVO_POSITION = 2,
    RELAY_SWITCH = 3,
    DISPLAY_UPDATE = 4,
    SENSOR_CALIBRATION = 5
};

void executeDeviceCommand(const CommandPackage& cmd) {
    switch(cmd.command) {
        case LED_CONTROL:
            handleLEDCommand(cmd);
            break;
        case SERVO_POSITION:
            handleServoCommand(cmd);
            break;
        case RELAY_SWITCH:
            handleRelayCommand(cmd);
            break;
        // ... other commands
    }
}
```

### JSON Parameters

Use structured parameters for complex commands:

```cpp
void handleLEDCommand(const CommandPackage& cmd) {
    // Parse JSON parameters
    DynamicJsonDocument doc(256);
    deserializeJson(doc, cmd.parameters);
    
    int brightness = doc["brightness"];
    String color = doc["color"];
    int duration = doc["duration"];
    
    // Execute LED control
    setLEDColor(color);
    setLEDBrightness(brightness);
    
    if (duration > 0) {
        scheduleAutoOff(duration);
    }
}
```

### Status Flags

Use bit flags for detailed device status:

```cpp
enum StatusFlags {
    STATUS_OK = 0x01,
    STATUS_LOW_MEMORY = 0x02,
    STATUS_WEAK_SIGNAL = 0x04,
    STATUS_SENSOR_ERROR = 0x08,
    STATUS_BATTERY_LOW = 0x10
};

void updateDeviceStatus() {
    StatusPackage status;
    status.from = mesh.getNodeId();
    status.deviceStatus = STATUS_OK;
    
    if (ESP.getFreeHeap() < 10000) {
        status.deviceStatus |= STATUS_LOW_MEMORY;
    }
    
    if (WiFi.RSSI() < -80) {
        status.deviceStatus |= STATUS_WEAK_SIGNAL;
    }
    
    if (batteryVoltage < 3.2) {
        status.deviceStatus |= STATUS_BATTERY_LOW;
    }
    
    status.uptime = millis() / 1000;
    status.freeMemory = ESP.getFreeHeap();
    status.wifiStrength = WiFi.RSSI();
    status.firmwareVersion = FIRMWARE_VERSION;
    
    mesh.sendPackage(&status);
}
```

### Command Acknowledgments

Implement reliable command delivery:

```cpp
void sendCommandAcknowledgment(const CommandPackage& originalCmd) {
    CommandPackage ack;
    ack.dest = originalCmd.from;
    ack.command = 255;  // ACK command
    ack.targetDevice = originalCmd.targetDevice;
    ack.commandId = originalCmd.commandId;
    ack.parameters = "OK";
    
    mesh.sendPackage(&ack);
}

// In command sender
std::map<uint32_t, uint32_t> pendingCommands;  // commandId -> timestamp

void trackCommand(const CommandPackage& cmd) {
    pendingCommands[cmd.commandId] = millis();
}

void checkCommandTimeouts() {
    uint32_t now = millis();
    
    for (auto it = pendingCommands.begin(); it != pendingCommands.end();) {
        if (now - it->second > 5000) {  // 5 second timeout
            Serial.printf("Command %u timed out\n", it->first);
            it = pendingCommands.erase(it);
        } else {
            ++it;
        }
    }
}
```

## Memory Optimization

### Buffer Sizing

Packages automatically calculate required buffer sizes:

```cpp
// Get exact buffer size needed
size_t bufferSize = sensor.jsonObjectSize();
DynamicJsonDocument doc(bufferSize);

// Or use static sizing for known maximums
StaticJsonDocument<256> doc;  // Sufficient for most SensorPackages
```

### Memory Usage

Typical memory usage per package:

- **SensorPackage**: ~150-200 bytes
- **CommandPackage**: ~100-300 bytes (depending on parameters)
- **StatusPackage**: ~200-250 bytes

## Platform Compatibility

### TSTRING Adaptation

Packages use `TSTRING` for cross-platform string compatibility:

```cpp
#ifdef ARDUINO
typedef String TSTRING;        // Arduino String class
#else
typedef std::string TSTRING;   // Standard C++ string
#endif
```

### ArduinoJson Versions

Compatible with both ArduinoJson v6 and v7:

```cpp
#if ARDUINOJSON_VERSION_MAJOR >= 7
// v7 automatic sizing
#else
// v6 manual sizing with jsonObjectSize()
#endif
```

## Testing Packages

Unit tests ensure package reliability:

```cpp
#include "catch2/catch.hpp"
#include "examples/alteriom/alteriom_sensor_package.hpp"

SCENARIO("SensorPackage serialization works correctly") {
    GIVEN("A SensorPackage with test data") {
        auto pkg = SensorPackage();
        pkg.from = 12345;
        pkg.temperature = 25.5;
        pkg.humidity = 60.0;
        pkg.pressure = 1013.25;
        pkg.sensorId = 1001;
        pkg.timestamp = 1234567890;
        pkg.batteryLevel = 85;
        
        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<SensorPackage>();
            
            THEN("All fields should match") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.temperature == pkg.temperature);
                REQUIRE(pkg2.humidity == pkg.humidity);
                REQUIRE(pkg2.pressure == pkg.pressure);
                REQUIRE(pkg2.sensorId == pkg.sensorId);
                REQUIRE(pkg2.timestamp == pkg.timestamp);
                REQUIRE(pkg2.batteryLevel == pkg.batteryLevel);
            }
        }
    }
}
```

## Creating Custom Packages

Extend the Alteriom package system with your own types:

```cpp
class WeatherPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    double windSpeed = 0.0;
    double windDirection = 0.0;
    double rainfall = 0.0;
    uint32_t stationId = 0;
    TSTRING location = "";
    
    WeatherPackage() : BroadcastPackage(203) {}  // Use ID 203+
    
    WeatherPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
        windSpeed = jsonObj["windSpeed"];
        windDirection = jsonObj["windDirection"];
        rainfall = jsonObj["rainfall"];
        stationId = jsonObj["stationId"];
        location = jsonObj["location"].as<TSTRING>();
    }
    
    JsonObject addTo(JsonObject&& jsonObj) const {
        jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
        jsonObj["windSpeed"] = windSpeed;
        jsonObj["windDirection"] = windDirection;
        jsonObj["rainfall"] = rainfall;
        jsonObj["stationId"] = stationId;
        jsonObj["location"] = location;
        return jsonObj;
    }
    
#if ARDUINOJSON_VERSION_MAJOR < 7
    size_t jsonObjectSize() const {
        return JSON_OBJECT_SIZE(noJsonFields + 5) + location.length();
    }
#endif
};
```

## Best Practices

### 1. Use Appropriate Package Types

- **SensorPackage**: Environmental readings, measurements
- **CommandPackage**: Device control, configuration changes  
- **StatusPackage**: Health monitoring, diagnostics
- **Custom packages**: Domain-specific data structures

### 2. Design for Reliability

```cpp
// Include timestamps for data freshness validation
sensor.timestamp = mesh.getNodeTime();

// Use unique IDs for command tracking
cmd.commandId = random(1000000, 9999999);

// Include version info for compatibility
status.firmwareVersion = "2.1.0";
```

### 3. Handle Errors Gracefully

```cpp
void handleSensorData(protocol::Variant& variant) {
    try {
        SensorPackage sensor = variant.to<SensorPackage>();
        
        // Validate data ranges
        if (sensor.temperature < -50 || sensor.temperature > 100) {
            Serial.println("Invalid temperature reading");
            return;
        }
        
        processSensorData(sensor);
        
    } catch (const std::exception& e) {
        Serial.printf("Error processing sensor package: %s\n", e.what());
    }
}
```

### 4. Monitor Performance

```cpp
void handleSensorData(protocol::Variant& variant) {
    uint32_t startTime = micros();
    
    SensorPackage sensor = variant.to<SensorPackage>();
    processSensorData(sensor);
    
    uint32_t processingTime = micros() - startTime;
    if (processingTime > 10000) {  // > 10ms
        Serial.printf("Slow sensor processing: %u µs\n", processingTime);
    }
}
```

## Bridge Failover Packages (v1.8.0+)

### BridgeStatusPackage (Type 610)

For monitoring bridge health and Internet connectivity.

```cpp
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

BridgeStatusPackage bridgeStatus;
bridgeStatus.internetConnected = true;
bridgeStatus.routerRSSI = -45;         // dBm
bridgeStatus.routerChannel = 6;
bridgeStatus.uptime = millis();
bridgeStatus.gatewayIP = "192.168.1.1";
bridgeStatus.timestamp = mesh.getNodeTime();

// Broadcast to all nodes
mesh.sendPackage(&bridgeStatus);
```

**Fields:**
- `internetConnected` (bool) - Bridge Internet connectivity status
- `routerRSSI` (int8_t) - Router WiFi signal strength in dBm
- `routerChannel` (uint8_t) - Router WiFi channel
- `uptime` (uint32_t) - Bridge uptime in milliseconds
- `gatewayIP` (TSTRING) - Router gateway IP address
- `timestamp` (uint32_t) - Status check timestamp

**Use Cases:**
- Bridge health monitoring
- Internet connectivity tracking
- Automatic failover detection
- Network diagnostics

### BridgeElectionPackage (Type 611)

For coordinating automatic bridge failover elections.

```cpp
BridgeElectionPackage election;
election.routerRSSI = WiFi.RSSI();    // -127 to 0 dBm
election.uptime = millis();
election.freeMemory = ESP.getFreeHeap();
election.timestamp = mesh.getNodeTime();
election.routerSSID = "MyRouter";

// Broadcast election candidacy
mesh.sendPackage(&election);
```

**Fields:**
- `routerRSSI` (int8_t) - Router WiFi signal strength (-127 to 0 dBm)
- `uptime` (uint32_t) - Node uptime in milliseconds
- `freeMemory` (uint32_t) - Free memory in bytes
- `timestamp` (uint32_t) - Election timestamp
- `routerSSID` (TSTRING) - Router SSID for verification

**Use Cases:**
- Automatic bridge election
- Failover coordination
- RSSI-based selection
- Distributed consensus

### BridgeTakeoverPackage (Type 612)

For announcing bridge role transitions.

```cpp
BridgeTakeoverPackage takeover;
takeover.previousBridge = oldBridgeNodeId;  // 0 if none
takeover.reason = "Internet connectivity lost";
takeover.timestamp = mesh.getNodeTime();

// Announce new bridge role
mesh.sendPackage(&takeover);
```

**Fields:**
- `previousBridge` (uint32_t) - Previous bridge node ID (0 if none)
- `reason` (TSTRING) - Takeover reason description
- `timestamp` (uint32_t) - Takeover timestamp

**Use Cases:**
- Bridge transition announcements
- Failover notifications
- Network event logging
- Topology tracking

### NTPTimeSyncPackage (Type 614)

For distributing NTP time synchronization from bridge to mesh.

```cpp
NTPTimeSyncPackage ntpSync;
ntpSync.ntpTime = 1699564800;        // Unix timestamp
ntpSync.accuracy = 50;               // ±50ms uncertainty
ntpSync.source = "pool.ntp.org";
ntpSync.timestamp = mesh.getNodeTime();

// Broadcast NTP time to mesh
mesh.sendPackage(&ntpSync);
```

**Fields:**
- `ntpTime` (uint32_t) - Unix timestamp from NTP server
- `accuracy` (uint16_t) - Milliseconds uncertainty/precision
- `source` (TSTRING) - NTP server source
- `timestamp` (uint32_t) - Collection timestamp

**Use Cases:**
- Time synchronization from Internet
- RTC backup coordination
- Mesh-wide time distribution
- Offline timekeeping

## Complete Package Type Reference

| Type | Class | Version | Purpose |
|------|-------|---------|---------|
| 200 | `SensorPackage` | v1.0.0+ | Environmental sensor data |
| 202 | `StatusPackage` | v1.0.0+ | Device health monitoring |
| 204 | `MetricsPackage` | v1.7.7+ | Performance metrics |
| 400 | `CommandPackage` | v1.7.7+ | Device control (moved from 201) |
| 600 | `MeshNodeListPackage` | v1.7.7+ | Node inventory |
| 601 | `MeshTopologyPackage` | v1.7.7+ | Network topology |
| 602 | `MeshAlertPackage` | v1.7.7+ | Network alerts |
| 603 | `MeshBridgePackage` | v1.7.7+ | Protocol bridging |
| 604 | `EnhancedStatusPackage` | v1.7.7+ | Mesh-wide status |
| 605 | `HealthCheckPackage` | v1.7.7+ | Proactive health monitoring |
| 610 | `BridgeStatusPackage` | v1.8.0+ | Bridge health monitoring |
| 611 | `BridgeElectionPackage` | v1.8.0+ | Failover election |
| 612 | `BridgeTakeoverPackage` | v1.8.0+ | Bridge transition |
| 614 | `NTPTimeSyncPackage` | v1.8.0+ | NTP time distribution |

See also:
- [Sensor Networks Tutorial](sensor-networks.md) - Real-world applications
- [Examples](examples.md) - Complete working examples
- [Custom Packages Tutorial](../tutorials/custom-packages.md) - Creating your own packages
- [Bridge Failover Guide](../../BRIDGE_TO_INTERNET.md) - Bridge setup and failover