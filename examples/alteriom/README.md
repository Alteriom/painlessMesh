# Alteriom painlessMesh Extensions

This directory contains Alteriom-specific extensions and examples for the painlessMesh library.

## Package Types

### SensorPackage (Type 200)
Broadcast package for sharing environmental sensor data across the mesh.

**Fields:**
- `temperature` - Temperature in Celsius
- `humidity` - Relative humidity percentage  
- `pressure` - Atmospheric pressure in hPa
- `sensorId` - Unique sensor identifier
- `timestamp` - Unix timestamp of measurement
- `batteryLevel` - Battery level percentage (0-100)

### CommandPackage (Type 201)
Single-destination package for sending commands to specific nodes.

**Fields:**
- `command` - Command type identifier
- `targetDevice` - Target device ID
- `parameters` - Command parameters as JSON string
- `commandId` - Unique command identifier for tracking

### StatusPackage (Type 202)
Broadcast package for sharing device health and status information.

**Fields:**
- `deviceStatus` - Device status flags
- `uptime` - Device uptime in seconds
- `freeMemory` - Free memory in KB
- `wifiStrength` - WiFi signal strength (0-100)
- `firmwareVersion` - Current firmware version string

## Examples

### `alteriom_sensor_node.ino`
Complete Arduino sketch demonstrating:
- Periodic sensor data broadcasting
- Command handling
- Status reporting
- Message type discrimination
- Integration with painlessMesh

## Usage

```cpp
#include "alteriom_sensor_package.hpp"
using namespace alteriom;

// Create and send sensor data
SensorPackage sensor;
sensor.temperature = 25.0;
sensor.humidity = 60.0;
// ... set other fields
mesh.sendBroadcast(sensor.toJsonString());

// Handle incoming commands
void handleMessage(String& msg) {
    auto doc = parseJson(msg);
    if (doc["type"] == 201) {
        CommandPackage cmd(doc.as<JsonObject>());
        processCommand(cmd);
    }
}
```

## Testing

Run the Alteriom package tests:
```bash
./bin/catch_alteriom_packages
```

This validates:
- JSON serialization/deserialization
- Package type consistency
- Field preservation
- Edge case handling
- Integration with painlessMesh plugin system