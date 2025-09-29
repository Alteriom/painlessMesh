---
sidebar_position: 1
---

# Alteriom Sensor Package

This page documents the **Alteriom-specific extensions** to painlessMesh, including the SensorPackage, CommandPackage, and StatusPackage classes designed for IoT sensor networks.

## Overview

The Alteriom package system extends painlessMesh with standardized message types for common IoT use cases:

- 🌡️ **SensorPackage** - Environmental data collection
- 🎮 **CommandPackage** - Device control and coordination  
- 📊 **StatusPackage** - Health monitoring and diagnostics

## SensorPackage (Type 200)

### Purpose
Collects and transmits environmental sensor data across the mesh network.

### Properties

| Field | Type | Description | Range/Units |
|-------|------|-------------|-------------|
| `temperature` | `double` | Temperature reading | °C |
| `humidity` | `double` | Humidity percentage | 0-100% |
| `pressure` | `double` | Atmospheric pressure | hPa/mBar |
| `sensorId` | `uint32_t` | Unique sensor identifier | - |
| `timestamp` | `uint32_t` | Unix timestamp | seconds |
| `batteryLevel` | `uint8_t` | Battery charge level | 0-100% |

### Usage Example

```cpp title="sensor_reading.cpp"
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

// Create sensor package
SensorPackage sensor;
sensor.sensorId = 1001;
sensor.temperature = 23.5;
sensor.humidity = 65.2;
sensor.pressure = 1013.25;
sensor.batteryLevel = 87;
sensor.timestamp = mesh.getNodeTime();

// Send as broadcast to all nodes
mesh.sendBroadcast(sensor.toString());
```

### JSON Format

```json title="sensor_data.json"
{
  "type": 200,
  "sensorId": 1001,
  "temperature": 23.5,
  "humidity": 65.2, 
  "pressure": 1013.25,
  "batteryLevel": 87,
  "timestamp": 1643723400
}
```

## CommandPackage (Type 201)

### Purpose
Sends control commands between nodes for device coordination and automation.

### Properties

| Field | Type | Description | Values |
|-------|------|-------------|---------|
| `command` | `uint8_t` | Command type | See command types below |
| `targetDevice` | `uint32_t` | Target device ID | Node ID |
| `commandId` | `uint32_t` | Unique command ID | For tracking |
| `parameters` | `TSTRING` | JSON parameters | Command-specific |

### Command Types

```cpp title="command_types.hpp"
// Standard command types
#define CMD_RESET          1
#define CMD_SLEEP          2  
#define CMD_WAKE           3
#define CMD_SET_CONFIG     4
#define CMD_GET_STATUS     5
#define CMD_CALIBRATE      6
#define CMD_UPDATE_RATE    7
```

### Usage Example

```cpp title="send_command.cpp"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

// Send calibration command to specific node
CommandPackage cmd;
cmd.command = CMD_CALIBRATE;
cmd.targetDevice = 2001;  // Target node ID
cmd.commandId = 12345;
cmd.parameters = "{\"sensor\":\"temperature\",\"offset\":0.5}";

// Send to specific node
mesh.sendSingle(cmd.targetDevice, cmd.toString());
```

### JSON Format

```json title="command_data.json"
{
  "type": 201,
  "command": 6,
  "targetDevice": 2001,
  "commandId": 12345,
  "parameters": "{\"sensor\":\"temperature\",\"offset\":0.5}"
}
```

## StatusPackage (Type 202)

### Purpose
Reports device health, performance metrics, and operational status.

### Properties

| Field | Type | Description | Units |
|-------|------|-------------|-------|
| `deviceStatus` | `uint8_t` | Operational status | See status codes |
| `uptime` | `uint32_t` | Device uptime | seconds |
| `freeMemory` | `uint16_t` | Available RAM | bytes |
| `wifiStrength` | `uint8_t` | WiFi signal strength | dBm |
| `firmwareVersion` | `TSTRING` | Software version | semver |

### Status Codes

```cpp title="status_codes.hpp"
#define STATUS_OK              0
#define STATUS_WARNING         1
#define STATUS_ERROR           2  
#define STATUS_CRITICAL        3
#define STATUS_MAINTENANCE     4
#define STATUS_CALIBRATING     5
```

### Usage Example

```cpp title="status_report.cpp"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

// Create status report
StatusPackage status;
status.deviceStatus = STATUS_OK;
status.uptime = millis() / 1000;
status.freeMemory = ESP.getFreeHeap();
status.wifiStrength = WiFi.RSSI();
status.firmwareVersion = "1.2.3";

// Broadcast status to mesh
mesh.sendBroadcast(status.toString());
```

## Message Handling

### Receiving Messages

```cpp title="message_handler.cpp"
void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    JsonObject obj = doc.as<JsonObject>();
    
    uint8_t msgType = obj["type"];
    
    switch(msgType) {
        case 200: // SensorPackage
            handleSensorData(SensorPackage(obj), from);
            break;
            
        case 201: // CommandPackage  
            handleCommand(CommandPackage(obj), from);
            break;
            
        case 202: // StatusPackage
            handleStatus(StatusPackage(obj), from);
            break;
            
        default:
            Serial.printf("Unknown message type: %d\n", msgType);
    }
}

void handleSensorData(const SensorPackage& sensor, uint32_t from) {
    Serial.printf("Sensor %u: %.1f°C, %.1f%% humidity\n", 
                  sensor.sensorId, sensor.temperature, sensor.humidity);
    
    // Store in database, trigger alerts, etc.
    if (sensor.temperature > 30.0) {
        Serial.println("Temperature alert!");
    }
}

void handleCommand(const CommandPackage& cmd, uint32_t from) {
    if (cmd.targetDevice != mesh.getNodeId()) {
        return; // Not for us
    }
    
    switch(cmd.command) {
        case CMD_RESET:
            Serial.println("Reset command received");
            ESP.restart();
            break;
            
        case CMD_CALIBRATE:
            Serial.println("Calibration command received");
            // Parse parameters and calibrate
            break;
    }
}
```

## Best Practices

### Memory Management

```cpp title="memory_efficient.cpp"
// Use object pooling for frequent messages
class SensorPackagePool {
private:
    static const size_t POOL_SIZE = 5;
    SensorPackage pool[POOL_SIZE];
    bool used[POOL_SIZE] = {false};
    
public:
    SensorPackage* acquire() {
        for (size_t i = 0; i < POOL_SIZE; i++) {
            if (!used[i]) {
                used[i] = true;
                return &pool[i];
            }
        }
        return nullptr; // Pool exhausted
    }
    
    void release(SensorPackage* pkg) {
        for (size_t i = 0; i < POOL_SIZE; i++) {
            if (&pool[i] == pkg) {
                used[i] = false;
                break;
            }
        }
    }
};
```

### Rate Limiting

```cpp title="rate_limiting.cpp"
class RateLimiter {
private:
    unsigned long lastSend = 0;
    const unsigned long MIN_INTERVAL = 5000; // 5 seconds
    
public:
    bool canSend() {
        unsigned long now = millis();
        if (now - lastSend >= MIN_INTERVAL) {
            lastSend = now;
            return true;
        }
        return false;
    }
};

RateLimiter sensorLimiter;

void loop() {
    if (sensorLimiter.canSend()) {
        // Read and send sensor data
        sendSensorReading();
    }
    mesh.update();
}
```

### Error Handling

```cpp title="error_handling.cpp"
bool sendWithRetry(const String& message, uint32_t target = 0) {
    const int MAX_RETRIES = 3;
    
    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        bool success = (target == 0) ? 
            mesh.sendBroadcast(message) : 
            mesh.sendSingle(target, message);
            
        if (success) {
            return true;
        }
        
        // Exponential backoff
        delay(100 * (1 << attempt));
    }
    
    Serial.println("Failed to send message after retries");
    return false;
}
```

## Testing

### Unit Tests

The Alteriom packages include comprehensive unit tests using Catch2:

```bash
# Run Alteriom package tests
cd test
cmake -G Ninja .
ninja
./bin/catch_alteriom_packages
```

### Test Coverage

- ✅ **Serialization** - JSON encoding/decoding
- ✅ **Validation** - Input validation and bounds checking
- ✅ **Memory safety** - No buffer overflows
- ✅ **Performance** - Optimized for ESP32/ESP8266

## Integration Examples

- 📊 [Environmental Monitoring](./sensor-networks) - Multi-sensor networks
- 🏠 [Home Automation](./home-automation) - Device control systems
- 🚨 [Alert Systems](./alert-systems) - Critical event handling
- 📈 [Data Logging](./data-logging) - Centralized data collection

## API Reference

For complete API documentation, see the [generated API docs](../api/core-api.md).