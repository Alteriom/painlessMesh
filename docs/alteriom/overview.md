# Alteriom Extensions Overview

The Alteriom extensions provide production-ready, type-safe packages for common IoT scenarios. These extensions demonstrate best practices for the painlessMesh plugin system while providing immediately useful functionality for sensor networks, device control, and system monitoring.

## What are Alteriom Extensions?

Alteriom extensions are pre-built painlessMesh packages that handle common IoT communication patterns:

- **Environmental Monitoring**: Temperature, humidity, pressure sensors
- **Device Control**: Commands for actuators, displays, relays  
- **Health Monitoring**: Device status, diagnostics, and telemetry
- **Type Safety**: Compile-time validation and automatic serialization
- **Production Ready**: Tested, documented, and optimized implementations

## Package Types

### SensorPackage (Type 200)
For broadcasting environmental sensor data across the mesh.

```cpp
alteriom::SensorPackage sensor;
sensor.temperature = 23.5;
sensor.humidity = 65.0;
sensor.pressure = 1013.25;
sensor.sensorId = 1001;
sensor.timestamp = mesh.getNodeTime();
sensor.batteryLevel = 85;

mesh.sendPackage(&sensor);
```

**Use Cases:**
- Weather stations
- Environmental monitoring
- HVAC system feedback
- Greenhouse automation
- Industrial sensor networks

### CommandPackage (Type 400)
For sending control commands to specific devices.

```cpp
alteriom::CommandPackage cmd;
cmd.dest = targetNodeId;
cmd.command = 1; // LED_CONTROL
cmd.targetDevice = 100; // LED strip ID
cmd.parameters = "{\"brightness\":75,\"color\":\"blue\"}";
cmd.commandId = generateCommandId();

mesh.sendPackage(&cmd);
```

**Use Cases:**
- Remote device control
- Actuator management
- Display updates
- System configuration
- Automation triggers

### StatusPackage (Type 202)
For broadcasting device health and operational status.

```cpp
alteriom::StatusPackage status;
status.deviceStatus = 1; // OPERATIONAL
status.uptime = millis() / 1000;
status.freeMemory = ESP.getFreeHeap();
status.wifiStrength = WiFi.RSSI();
status.firmwareVersion = "1.2.3";

mesh.sendPackage(&status);
```

**Use Cases:**
- System monitoring
- Predictive maintenance
- Network diagnostics
- Performance tracking
- Remote troubleshooting

## Key Features

### Type Safety
Compile-time validation prevents common messaging errors:

```cpp
// Compile error if field types don't match
sensor.temperature = "invalid"; // ❌ Compiler error
sensor.temperature = 25.0;      // ✅ Correct

// IDE autocomplete for all fields
sensor.| // IDE shows: temperature, humidity, pressure, etc.
```

### Automatic Serialization
No manual JSON handling required:

```cpp
// Automatic serialization to JSON
mesh.sendPackage(&sensor);

// Automatic deserialization from JSON
mesh.onPackage(200, [](protocol::Variant& variant) {
    alteriom::SensorPackage received = variant.to<alteriom::SensorPackage>();
    // All fields automatically populated
});
```

### Cross-Platform Compatibility
Works on ESP32, ESP8266, and desktop platforms:

```cpp
// TSTRING adapts to platform
#ifdef ESP32
    // Uses Arduino String class
#else
    // Uses std::string on desktop
#endif
```

### Memory Optimization
Efficient memory usage for resource-constrained devices:

```cpp
// Accurate buffer sizing
size_t bufferSize = sensor.jsonObjectSize();

// Minimal memory footprint
// No unnecessary string copies or allocations
```

## Architecture Integration

### Plugin System
Alteriom packages integrate seamlessly with painlessMesh's plugin architecture:

```
Application Layer (Your Code)
    ↓
Alteriom Packages (SensorPackage, CommandPackage, StatusPackage)  
    ↓
painlessMesh Plugin System (SinglePackage, BroadcastPackage)
    ↓
painlessMesh Core (Mesh, Protocol, Network)
```

### Message Flow
```
Sensor Reading → SensorPackage → JSON → Mesh Network → JSON → SensorPackage → Handler
```

### Type ID Allocation
Alteriom uses reserved type ID range 200-299:

```cpp
enum AlteriomTypes {
    ALTERIOM_SENSOR = 200,    // SensorPackage
    ALTERIOM_COMMAND = 400,   // CommandPackage  
    ALTERIOM_STATUS = 202,    // StatusPackage
    // 203-299 reserved for future Alteriom packages
};
```

## Getting Started

### 1. Include Alteriom Headers

```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom; // For convenience
```

### 2. Register Package Handlers

```cpp
void setup() {
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    
    // Register handlers for Alteriom packages
    mesh.onPackage(ALTERIOM_SENSOR, handleSensorData);
    mesh.onPackage(ALTERIOM_COMMAND, handleCommand);
    mesh.onPackage(ALTERIOM_STATUS, handleStatus);
}
```

### 3. Implement Handlers

```cpp
void handleSensorData(protocol::Variant& variant) {
    SensorPackage sensor = variant.to<SensorPackage>();
    
    Serial.printf("Sensor %u: T=%.1f°C, H=%.1f%%, P=%.1f hPa\n",
                 sensor.sensorId, sensor.temperature, 
                 sensor.humidity, sensor.pressure);
    
    // Process sensor data (store, analyze, forward, etc.)
    if (sensor.temperature > 30.0) {
        triggerCooling();
    }
}

void handleCommand(protocol::Variant& variant) {
    CommandPackage cmd = variant.to<CommandPackage>();
    
    if (cmd.dest != mesh.getNodeId()) {
        return; // Not for this node
    }
    
    Serial.printf("Command %u for device %u: %u\n",
                 cmd.commandId, cmd.targetDevice, cmd.command);
    
    // Execute command
    executeDeviceCommand(cmd);
    
    // Send acknowledgment
    sendCommandAcknowledgment(cmd);
}
```

### 4. Send Packages

```cpp
// Send sensor data every 30 seconds
Task taskSensorData(30000, TASK_FOREVER, [](){
    SensorPackage sensor;
    sensor.from = mesh.getNodeId();
    sensor.temperature = readTemperature();
    sensor.humidity = readHumidity();
    sensor.pressure = readPressure();
    sensor.sensorId = SENSOR_ID;
    sensor.timestamp = mesh.getNodeTime();
    sensor.batteryLevel = readBatteryLevel();
    
    mesh.sendPackage(&sensor);
});
```

## Advanced Usage

### Custom Command Types

Define application-specific command types:

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
        // ... other commands
    }
}
```

### Command Parameters

Use JSON parameters for complex commands:

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

### Status Monitoring

Implement comprehensive device monitoring:

```cpp
void sendStatusUpdate() {
    StatusPackage status;
    status.from = mesh.getNodeId();
    status.deviceStatus = getDeviceStatus();
    status.uptime = millis() / 1000;
    status.freeMemory = ESP.getFreeHeap();
    status.wifiStrength = WiFi.RSSI();
    status.firmwareVersion = FIRMWARE_VERSION;
    
    // Add custom diagnostics
    if (ESP.getFreeHeap() < 10000) {
        status.deviceStatus |= STATUS_LOW_MEMORY;
    }
    if (WiFi.RSSI() < -80) {
        status.deviceStatus |= STATUS_WEAK_SIGNAL;
    }
    
    mesh.sendPackage(&status);
}
```

### Error Handling

Implement robust error handling:

```cpp
void handleSensorData(protocol::Variant& variant) {
    try {
        SensorPackage sensor = variant.to<SensorPackage>();
        
        // Validate sensor data
        if (!isValidSensorReading(sensor)) {
            Serial.printf("Invalid sensor data from node %u\n", sensor.from);
            return;
        }
        
        // Check data age
        uint32_t age = mesh.getNodeTime() - sensor.timestamp;
        if (age > MAX_DATA_AGE) {
            Serial.printf("Stale sensor data (age: %u µs)\n", age);
            return;
        }
        
        processSensorData(sensor);
        
    } catch (const std::exception& e) {
        Serial.printf("Error processing sensor data: %s\n", e.what());
    }
}
```

## Integration Patterns

### Sensor Network Pattern

Central collector with multiple sensor nodes:

```cpp
class SensorCollector {
private:
    std::map<uint32_t, SensorData> sensorReadings;
    
public:
    void setup() {
        mesh.onPackage(ALTERIOM_SENSOR, [this](protocol::Variant& variant) {
            SensorPackage sensor = variant.to<SensorPackage>();
            storeSensorReading(sensor);
            return false;
        });
    }
    
    void storeSensorReading(const SensorPackage& sensor) {
        sensorReadings[sensor.from] = {
            sensor.temperature,
            sensor.humidity, 
            sensor.pressure,
            sensor.timestamp
        };
        
        // Trigger analysis
        analyzeEnvironmentalData();
    }
};
```

### Command and Control Pattern

Central controller managing multiple devices:

```cpp
class DeviceController {
private:
    std::map<uint32_t, DeviceInfo> devices;
    
public:
    void controlDevice(uint32_t nodeId, uint32_t deviceId, 
                      uint8_t command, const String& parameters) {
        CommandPackage cmd;
        cmd.dest = nodeId;
        cmd.command = command;
        cmd.targetDevice = deviceId;
        cmd.parameters = parameters;
        cmd.commandId = generateCommandId();
        
        mesh.sendPackage(&cmd);
        
        // Track pending command
        pendingCommands[cmd.commandId] = {nodeId, millis()};
    }
    
    void handleCommandAck(const CommandPackage& ack) {
        auto it = pendingCommands.find(ack.commandId);
        if (it != pendingCommands.end()) {
            Serial.printf("Command %u acknowledged by node %u\n", 
                         ack.commandId, ack.from);
            pendingCommands.erase(it);
        }
    }
};
```

### Health Monitoring Pattern

Network-wide device health monitoring:

```cpp
class HealthMonitor {
private:
    std::map<uint32_t, DeviceHealth> deviceHealth;
    
public:
    void setup() {
        mesh.onPackage(ALTERIOM_STATUS, [this](protocol::Variant& variant) {
            StatusPackage status = variant.to<StatusPackage>();
            updateDeviceHealth(status);
            return false;
        });
        
        // Check for unhealthy devices every minute
        userScheduler.addTask(Task(60000, TASK_FOREVER, [this]() {
            checkDeviceHealth();
        }));
    }
    
    void updateDeviceHealth(const StatusPackage& status) {
        deviceHealth[status.from] = {
            status.deviceStatus,
            status.uptime,
            status.freeMemory,
            status.wifiStrength,
            mesh.getNodeTime() // Last seen
        };
    }
    
    void checkDeviceHealth() {
        uint32_t now = mesh.getNodeTime();
        
        for (auto& [nodeId, health] : deviceHealth) {
            uint32_t timeSinceLastSeen = now - health.lastSeen;
            
            if (timeSinceLastSeen > DEVICE_TIMEOUT) {
                Serial.printf("Device %u appears offline\n", nodeId);
                triggerAlert(nodeId, "Device offline");
            }
            
            if (health.freeMemory < LOW_MEMORY_THRESHOLD) {
                Serial.printf("Device %u low memory: %u bytes\n", 
                             nodeId, health.freeMemory);
                triggerAlert(nodeId, "Low memory");
            }
        }
    }
};
```

## Best Practices

### Performance Optimization

1. **Batch sensor readings** when possible
2. **Use appropriate message frequency** (don't spam the network)
3. **Implement message filtering** to avoid processing irrelevant data
4. **Monitor memory usage** especially on ESP8266

### Reliability

1. **Validate all received data** before processing
2. **Implement timeouts** for commands and responses
3. **Handle network partitions** gracefully
4. **Add retry logic** for critical commands

### Security

1. **Validate message sources** in handlers
2. **Sanitize command parameters** before execution
3. **Implement rate limiting** for commands
4. **Consider encryption** for sensitive data

### Testing

1. **Test with realistic network loads**
2. **Simulate node failures** and recovery
3. **Validate under memory pressure**
4. **Test with maximum expected node count**

## Next Steps

- Learn about [Sensor Packages](sensor-packages.md) in detail
- Explore [Command System](command-system.md) implementation
- Study [Status Monitoring](status-monitoring.md) patterns
- See [Tutorial Examples](../tutorials/sensor-networks.md) for hands-on practice

The Alteriom extensions provide a solid foundation for building robust IoT applications with painlessMesh. They demonstrate production-ready patterns while remaining flexible enough to adapt to your specific needs.