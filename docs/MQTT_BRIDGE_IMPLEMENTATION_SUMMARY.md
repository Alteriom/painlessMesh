# MQTT Bridge Command System - Implementation Summary

## Overview

Successfully implemented a comprehensive MQTT-to-mesh bridge command system for painlessMesh Alteriom fork. This enables bidirectional communication between MQTT brokers and mesh networks, allowing web applications and external systems to control and monitor mesh nodes.

## Files Created

### Documentation

1. **`docs/MQTT_BRIDGE_COMMANDS.md`** (551 lines)
   - Complete command reference with 25+ command types
   - MQTT topic structure and naming conventions
   - Implementation guide with code examples
   - JavaScript and Python client examples
   - Troubleshooting guide and best practices

### Implementation

2. **`examples/bridge/mqtt_command_bridge.hpp`** (417 lines)
   - Bidirectional MQTT-mesh bridge class
   - Command routing and forwarding logic
   - Configuration management
   - Response handling and topic mapping
   - Support for all command types (device control, configuration, status)

3. **`examples/mqttCommandBridge/mqttCommandBridge.ino`** (181 lines)
   - Complete gateway bridge Arduino sketch
   - WiFi and MQTT connection management
   - Periodic status reporting
   - Topology updates
   - Event notifications (node connections/disconnections)

4. **`examples/alteriom/mesh_command_node.ino`** (217 lines)
   - Mesh node command handler example
   - Support for LED control, relay switching, PWM, sleep commands
   - Configuration management
   - Status reporting
   - Command acknowledgment system

### Core Package Enhancement

5. **`examples/alteriom/alteriom_sensor_package.hpp`** (Modified)
   - Added `responseToCommand` field to StatusPackage
   - Added `responseMessage` field to StatusPackage
   - Enhanced JSON serialization to include response fields
   - Maintains backward compatibility (fields only included when non-zero)

### Testing

6. **`test/catch/catch_mqtt_bridge.cpp`** (430 lines)
   - 8 comprehensive test scenarios
   - Command routing tests
   - Parameter parsing tests
   - Configuration management tests
   - Command ID tracking tests
   - Broadcast command tests
   - Error handling tests
   - 45+ individual test assertions

### Repository Updates

7. **`README.md`** (Modified)
   - Added "MQTT Bridge Commands" section
   - Linked to new documentation
   - Listed example sketches
   - Highlighted key capabilities

## Command Types Implemented

### Device Control (1-99)
- RESET (1) - Restart device
- SLEEP (2) - Enter deep sleep
- WAKE (3) - Wake from sleep
- LED_CONTROL (10) - Control onboard LED
- RELAY_SWITCH (11) - Control relay output
- PWM_SET (12) - Set PWM output
- SENSOR_ENABLE (20) - Enable/disable sensor
- SENSOR_CALIBRATE (21) - Calibrate sensor

### Configuration (100-199)
- GET_CONFIG (100) - Request current configuration
- SET_CONFIG (101) - Update configuration
- RESET_CONFIG (102) - Reset to factory defaults
- SAVE_CONFIG (103) - Persist config to flash
- SET_SAMPLE_RATE (110) - Change sensor sample rate
- SET_DEVICE_NAME (111) - Update device name

### Status (200-255)
- GET_STATUS (200) - Request basic status
- GET_METRICS (201) - Request performance metrics
- GET_DIAGNOSTICS (202) - Request detailed diagnostics
- START_MONITORING (210) - Begin continuous monitoring
- STOP_MONITORING (211) - Stop continuous monitoring

## MQTT Topics

### Command Topics (Published by Web App)
```
mesh/command/{nodeId}           # Send command to specific node
mesh/command/broadcast          # Broadcast command to all nodes
mesh/config/{nodeId}/get        # Request configuration
mesh/config/{nodeId}/set        # Update configuration
mesh/ota/{nodeId}/start         # Initiate OTA update
```

### Response Topics (Published by Gateway Bridge)
```
mesh/response/{nodeId}          # Command acknowledgments
mesh/status/{nodeId}            # Status updates (Type 202)
mesh/sensor/{nodeId}            # Sensor data (Type 200)
mesh/health/{nodeId}            # Enhanced status (Type 203)
mesh/config/{nodeId}            # Configuration data
mesh/error/{nodeId}             # Error reports
```

### System Topics
```
mesh/gateway/status             # Gateway bridge health
mesh/topology                   # Mesh network topology
mesh/nodes                      # List of connected nodes
mesh/events                     # Connection/disconnection events
```

## Architecture

```
┌─────────────────┐      MQTT      ┌──────────────┐      Mesh      ┌────────────┐
│ Web Application │ ◄──────────────► │ Gateway Node │ ◄──────────────► │ Mesh Nodes │
│  (Publisher)    │                  │   (Bridge)   │                  │ (Handlers) │
└─────────────────┘                  └──────────────┘                  └────────────┘
                                            │
                                            ▼
                                    ┌──────────────┐
                                    │ MQTT Broker  │
                                    │  (Mosquitto) │
                                    └──────────────┘
```

## Key Features

### 1. Command Routing
- **Unicast**: Commands targeted at specific nodes by nodeId
- **Broadcast**: Commands sent to all mesh nodes (targetDevice = 0)
- **Local Execution**: Gateway can execute commands on itself
- **Forwarding**: Automatic routing through mesh network

### 2. Response Tracking
- **Command IDs**: Unique identifiers for request/response matching
- **Acknowledgments**: Nodes send responses back through gateway
- **Status Codes**: 0=OK, 1=Warning, 2=Error
- **Message Context**: Detailed success/error messages

### 3. Configuration Management
- **Get Configuration**: Request current node settings
- **Set Configuration**: Update node settings remotely
- **Validation**: Parameter validation before applying
- **Persistence**: Support for saving to flash/EEPROM

### 4. Event Notifications
- **Node Connections**: Notifications when nodes join mesh
- **Node Disconnections**: Notifications when nodes leave mesh
- **Topology Updates**: Periodic mesh network structure reports
- **Gateway Health**: Regular heartbeat from gateway

## Usage Examples

### JavaScript (Node.js)
```javascript
const mqtt = require('mqtt');
const client = mqtt.connect('mqtt://192.168.1.100:1883');

// Send LED control command
const command = {
  type: 201,
  command: 10,
  targetDevice: 123456,
  commandId: Date.now(),
  parameters: JSON.stringify({ state: true, brightness: 75 })
};

client.publish('mesh/command/123456', JSON.stringify(command));

// Listen for response
client.subscribe('mesh/response/123456');
client.on('message', (topic, message) => {
  const response = JSON.parse(message.toString());
  console.log(`Command ${response.responseToCommand}: ${response.responseMessage}`);
});
```

### Python
```python
import paho.mqtt.client as mqtt
import json

def on_message(client, userdata, msg):
    response = json.loads(msg.payload.decode())
    if "responseToCommand" in response:
        print(f"Command {response['responseToCommand']}: {response['responseMessage']}")

client = mqtt.Client()
client.on_message = on_message
client.connect("192.168.1.100", 1883, 60)

# Send command
command = {
    "type": 201,
    "command": 10,
    "targetDevice": 123456,
    "commandId": 1001,
    "parameters": json.dumps({"state": True, "brightness": 75})
}

client.publish("mesh/command/123456", json.dumps(command))
client.subscribe("mesh/response/123456")
client.loop_forever()
```

## Testing

### Test Coverage
- ✅ Command package serialization/deserialization
- ✅ Parameter parsing for all command types
- ✅ Response field handling in StatusPackage
- ✅ Command ID tracking and matching
- ✅ Broadcast command routing
- ✅ Configuration management workflows
- ✅ Error handling and validation
- ✅ JSON format compliance

### Test Execution
Tests are configured in `CMakeLists.txt` and will be auto-discovered:
```bash
cmake -G Ninja .
ninja
./bin/catch_mqtt_bridge
```

## Integration with Existing Phase 2 Features

### OTA Updates via MQTT
The MQTT bridge can trigger OTA updates:
```json
{
  "topic": "mesh/ota/123456/start",
  "payload": {
    "role": "sensor",
    "hardware": "ESP32",
    "md5": "abc123...",
    "broadcasted": true
  }
}
```

### Status Reporting
Nodes automatically report status via MQTT:
- SensorPackage (200) → `mesh/sensor/{nodeId}`
- StatusPackage (202) → `mesh/status/{nodeId}`
- EnhancedStatusPackage (203) → `mesh/health/{nodeId}`

## Dependencies

### Arduino/PlatformIO
- painlessMesh (core library)
- PubSubClient (MQTT client)
- ArduinoJson (JSON serialization)
- TaskScheduler (background tasks)

### MQTT Broker
- Mosquitto (recommended)
- HiveMQ
- AWS IoT Core
- Azure IoT Hub

## Security Considerations

### Current Implementation
- No authentication on MQTT commands (prototype)
- No command signature verification
- No rate limiting

### Production Recommendations
1. **Enable MQTT Authentication**: Use username/password or certificates
2. **Implement ACLs**: Restrict topic access per client
3. **Add Rate Limiting**: Prevent command flooding
4. **Validate Commands**: Check command bounds and parameters
5. **Use TLS/SSL**: Encrypt MQTT traffic
6. **Command Signing**: Add HMAC signatures to verify command authenticity

## Future Enhancements

### Potential Features
1. **Command Queue**: Store commands for offline nodes
2. **Priority Commands**: Urgent commands bypass queue
3. **Batch Commands**: Send multiple commands in one message
4. **Command History**: Track all commands sent/received
5. **Scheduled Commands**: Execute commands at specific times
6. **Conditional Commands**: Execute based on node state
7. **Command Templates**: Predefined command sequences

### Performance Optimizations
1. **Message Compression**: Reduce payload sizes
2. **Persistent Sessions**: Maintain MQTT subscriptions
3. **QoS Levels**: Adjust quality of service per topic
4. **Topic Wildcards**: Reduce subscription count
5. **Retained Messages**: Store last known values

## Documentation Links

- 📖 [MQTT Bridge Commands Reference](../docs/MQTT_BRIDGE_COMMANDS.md)
- 🔧 [OTA Commands Reference](../docs/OTA_COMMANDS_REFERENCE.md)
- 🌉 [MQTT Command Bridge Example](../examples/mqttCommandBridge/mqttCommandBridge.ino)
- 📡 [Mesh Command Node Example](../examples/alteriom/mesh_command_node.ino)
- 📚 [API Documentation](https://alteriom.github.io/painlessMesh/)

## Author

Alteriom Development Team  
October 2024

## License

Same as painlessMesh: GNU GENERAL PUBLIC LICENSE V3
