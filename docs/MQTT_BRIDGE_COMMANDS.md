# MQTT Bridge Commands Reference

## Overview

This document provides a complete reference for MQTT-to-mesh bridge commands in painlessMesh Alteriom fork. The MQTT bridge enables bidirectional communication between MQTT brokers and mesh networks, allowing web applications to control and monitor mesh nodes.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Command Types](#command-types)
3. [MQTT Topic Structure](#mqtt-topic-structure)
4. [Command Definitions](#command-definitions)
5. [Implementation Guide](#implementation-guide)
6. [Complete Examples](#complete-examples)
7. [Troubleshooting](#troubleshooting)

---

## Architecture Overview

### Communication Flow

```
Web Application → MQTT Broker → Gateway Bridge → Mesh Network → Target Node
Target Node → Mesh Network → Gateway Bridge → MQTT Broker → Web Application
```

### Components

1. **MQTT Broker** - Central message router (Mosquitto, HiveMQ, AWS IoT)
2. **Gateway Bridge** - ESP32/ESP8266 with WiFi + mesh capability
3. **Mesh Nodes** - ESP devices in mesh network
4. **Web Application** - Control interface publishing MQTT commands

### Message Types

| Direction | Package Type | Purpose |
|-----------|--------------|---------|
| MQTT → Mesh | CommandPackage (201) | Control device, request data |
| Mesh → MQTT | StatusPackage (202) | Report device status |
| Mesh → MQTT | SensorPackage (200) | Sensor data reports |
| Mesh → MQTT | EnhancedStatusPackage (203) | Detailed health metrics |

---

## Command Types

### Device Control Commands (1-99)

| Command ID | Name | Description | Parameters |
|------------|------|-------------|------------|
| 1 | RESET | Restart device | None |
| 2 | SLEEP | Enter deep sleep | `duration_ms` (uint32_t) |
| 3 | WAKE | Wake from sleep | None |
| 10 | LED_CONTROL | Control onboard LED | `state` (bool), `brightness` (uint8_t) |
| 11 | RELAY_SWITCH | Control relay output | `channel` (uint8_t), `state` (bool) |
| 12 | PWM_SET | Set PWM output | `pin` (uint8_t), `duty` (uint16_t) |
| 20 | SENSOR_ENABLE | Enable/disable sensor | `sensor_id` (uint8_t), `enabled` (bool) |
| 21 | SENSOR_CALIBRATE | Calibrate sensor | `sensor_id` (uint8_t) |

### Configuration Commands (100-199)

| Command ID | Name | Description | Parameters |
|------------|------|-------------|------------|
| 100 | GET_CONFIG | Request current configuration | None |
| 101 | SET_CONFIG | Update configuration | JSON config object |
| 102 | RESET_CONFIG | Reset to factory defaults | None |
| 103 | SAVE_CONFIG | Persist config to flash | None |
| 110 | SET_SAMPLE_RATE | Change sensor sample rate | `rate_ms` (uint32_t) |
| 111 | SET_DEVICE_NAME | Update device name | `name` (string) |

### Status Commands (200-255)

| Command ID | Name | Description | Parameters |
|------------|------|-------------|------------|
| 200 | GET_STATUS | Request basic status | None |
| 201 | GET_METRICS | Request performance metrics | None |
| 202 | GET_DIAGNOSTICS | Request detailed diagnostics | None |
| 210 | START_MONITORING | Begin continuous monitoring | `interval_ms` (uint32_t) |
| 211 | STOP_MONITORING | Stop continuous monitoring | None |

### Topology Commands (300-399) ✨ NEW in v0.5.0

| Command ID | Name | Description | Parameters |
|------------|------|-------------|------------|
| 300 | GET_TOPOLOGY | Request mesh network topology | `format` (optional): "full" or "summary" |

**Response:** Full mesh topology published to `alteriom/mesh/{mesh_id}/topology/response` with `correlation_id` field matching the command ID.

**Example Response:**
```json
{
  "schema_version": 1,
  "event": "mesh_topology",
  "correlation_id": "12345",
  "mesh_id": "MESH-001",
  "nodes": [...],
  "connections": [...],
  "metrics": {...}
}
```

---

## MQTT Topic Structure

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
mesh/response/{nodeId}                    # Command acknowledgments
mesh/status/{nodeId}                      # Status updates (Type 202)
mesh/sensor/{nodeId}                      # Sensor data (Type 200)
mesh/health/{nodeId}                      # Enhanced status (Type 203)
mesh/config/{nodeId}                      # Configuration data
mesh/error/{nodeId}                       # Error reports
```

### System Topics (Legacy - v0.4.0)

```
mesh/gateway/status                       # Gateway bridge health
mesh/topology                             # Mesh network topology (old format)
mesh/nodes                                # List of connected nodes
```

### Topology & Event Topics ✨ NEW (v0.5.0 - @alteriom/mqtt-schema compliant)

```
alteriom/mesh/{mesh_id}/topology          # Full/incremental mesh topology
alteriom/mesh/{mesh_id}/topology/response # Topology responses to GET_TOPOLOGY command
alteriom/mesh/{mesh_id}/events            # Real-time mesh state change events
```

**Schema Compliance:** All topology and event messages conform to **@alteriom/mqtt-schema v0.5.0** specification.

**Key Features:**
- **Device ID Format:** `ALT-XXXXXXXXXXXX` (12 hex digits)
- **Envelope Fields:** `schema_version`, `device_id`, `device_type`, `timestamp`, `firmware_version`
- **Connection Metrics:** RSSI, latency, quality (0-100), packet counts
- **Network Metrics:** Total nodes, network diameter, average quality
- **Update Types:** Full (every 60s), Incremental (every 5s if changed)
- **Event Types:** node_join, node_leave, connection_lost, connection_restored, network_split, network_merged

---

## Command Definitions

### CommandPackage Structure (Type 201)

Defined in `examples/alteriom/alteriom_sensor_package.hpp`:

```cpp
class CommandPackage : public painlessmesh::plugin::SinglePackage {
public:
  uint8_t command = 0;          // Command ID (1-255)
  uint32_t targetDevice = 0;    // Destination node ID
  uint32_t commandId = 0;       // Unique command tracking ID
  TSTRING parameters = "";      // JSON-encoded parameters
  
  CommandPackage() : SinglePackage(201) {}
};
```

### Command Parameter Encoding

Parameters are JSON-encoded strings:

```json
{
  "state": "ON",
  "brightness": 75,
  "duration_ms": 5000
}
```

### Command Response Structure

Responses use StatusPackage (Type 202) with enhanced fields:

```cpp
class StatusPackage : public painlessmesh::plugin::BroadcastPackage {
public:
  uint8_t deviceStatus = 0;     // 0=OK, 1=Warning, 2=Error
  uint32_t uptime = 0;          // Seconds since boot
  uint16_t freeMemory = 0;      // Free heap in KB
  TSTRING firmwareVersion = ""; // Current firmware version
  
  // Response fields
  uint32_t responseToCommand = 0;  // Original commandId
  TSTRING responseMessage = "";    // Success/error message
  
  StatusPackage() : BroadcastPackage(202) {}
};
```

---

## Implementation Guide

### Gateway Bridge Implementation

The gateway bridge is implemented in `examples/bridge/mqtt_command_bridge.hpp` and provides:

1. **MQTT → Mesh Command Forwarding**
   - Subscribes to command topics
   - Parses JSON payloads
   - Routes commands to target nodes

2. **Mesh → MQTT Response Forwarding**
   - Receives mesh messages
   - Publishes to appropriate MQTT topics
   - Maintains message routing

3. **Configuration Management**
   - Handles config requests
   - Applies config updates
   - Sends acknowledgments

4. **Local Command Execution**
   - Executes commands targeted at gateway
   - Sends responses via MQTT

### Mesh Node Implementation

Mesh nodes implement command handlers in `examples/alteriom/mesh_command_node.ino`:

1. **Command Reception**
   - Listen for CommandPackage (Type 201)
   - Parse command and parameters
   - Execute appropriate action

2. **Response Generation**
   - Create StatusPackage response
   - Include commandId for tracking
   - Broadcast back to mesh

3. **Status Reporting**
   - Send periodic status updates
   - Report errors and warnings
   - Include relevant metrics

---

## Mesh Topology Reporting ✨ NEW in v0.5.0

### Overview

The mesh topology system provides real-time visibility into the structure and health of your mesh network. It publishes detailed information about nodes, connections, and network metrics in a standardized format that complies with **@alteriom/mqtt-schema v0.5.0**.

**Implementation:** `examples/bridge/mesh_topology_reporter.hpp`

### Topology Message Structure

#### Envelope Fields (Required by @alteriom/mqtt-schema)

```json
{
  "schema_version": 1,
  "device_id": "ALT-6825DD341CA4",
  "device_type": "gateway",
  "timestamp": "2025-01-12T15:00:00Z",
  "firmware_version": "GW 2.3.4"
}
```

#### Topology Payload

```json
{
  "event": "mesh_topology",
  "mesh_id": "MESH-001",
  "gateway_node_id": "ALT-6825DD341CA4",
  "nodes": [
    {
      "node_id": "ALT-6825DD341CA4",
      "role": "gateway",
      "status": "online",
      "last_seen": "2025-01-12T15:00:00Z",
      "firmware_version": "GW 2.3.4",
      "uptime_seconds": 86400,
      "free_memory_kb": 128,
      "connection_count": 3
    }
  ],
  "connections": [
    {
      "from_node": "ALT-6825DD341CA4",
      "to_node": "ALT-441D64F804A0",
      "quality": 95,
      "latency_ms": 12,
      "rssi": -42,
      "hop_count": 1
    }
  ],
  "metrics": {
    "total_nodes": 4,
    "online_nodes": 4,
    "network_diameter": 2,
    "avg_connection_quality": 85,
    "messages_per_second": 12.4
  },
  "update_type": "full"
}
```

### Publishing Schedule

| Update Type | Frequency | Trigger | Topic |
|-------------|-----------|---------|-------|
| Full Topology | Every 60 seconds | Timer | `alteriom/mesh/MESH-001/topology` |
| Incremental | Every 5 seconds | Change detection | `alteriom/mesh/MESH-001/topology` |
| On-Demand | Immediate | GET_TOPOLOGY command | `alteriom/mesh/MESH-001/topology/response` |

### Node Roles

| Role | Description | Typical Use |
|------|-------------|-------------|
| gateway | MQTT bridge with WiFi | Root node, MQTT publisher |
| sensor | Sensor measurement node | Data collection |
| repeater | Range extender | Network coverage |

### Connection Quality Metrics

**Quality Score (0-100):** Calculated from latency, packet loss, and RSSI

- **90-100:** Excellent - Low latency (<50ms), strong signal (>-50 dBm)
- **70-89:** Good - Moderate latency (<100ms), good signal (>-70 dBm)
- **50-69:** Fair - Higher latency (<200ms), weaker signal (>-80 dBm)
- **0-49:** Poor - High latency (>200ms), weak signal (<-80 dBm)

**RSSI (Received Signal Strength Indicator):**
- Values in dBm (negative numbers)
- Stronger signal = higher (less negative) value
- Example: -42 dBm is better than -75 dBm

**Latency:**
- Round-trip time in milliseconds
- Calculated from message exchange timing
- Lower is better

### GET_TOPOLOGY Command (300)

**Request:** Publish to `mesh/command/{gateway_id}` or `mesh/command/broadcast`

```json
{
  "type": 201,
  "command": 300,
  "targetDevice": 0,
  "commandId": 12345,
  "parameters": "{}"
}
```

**Response:** Published to `alteriom/mesh/MESH-001/topology/response`

```json
{
  "schema_version": 1,
  "device_id": "ALT-6825DD341CA4",
  "event": "mesh_topology",
  "correlation_id": "12345",
  "mesh_id": "MESH-001",
  ... full topology ...
}
```

**Key Feature:** The `correlation_id` field matches the command's `commandId` for request tracking.

---

## Mesh Events ✨ NEW in v0.5.0

### Overview

Real-time notifications of mesh network state changes. Published immediately when events occur.

**Implementation:** `examples/bridge/mesh_event_publisher.hpp`  
**Topic:** `alteriom/mesh/MESH-001/events`

### Event Types

| Event Type | Description | When Triggered |
|------------|-------------|----------------|
| node_join | New node connected | onNewConnection() callback |
| node_leave | Node disconnected | onDroppedConnection() callback |
| connection_lost | Direct connection failed | Connection timeout |
| connection_restored | Connection recovered | After connection_lost |
| network_split | Mesh partitioned | Network segmentation detected |
| network_merged | Partitions rejoined | Segments reconnected |

### Event Message Structure

#### Node Join Event

```json
{
  "schema_version": 1,
  "device_id": "ALT-6825DD341CA4",
  "device_type": "gateway",
  "timestamp": "2025-01-12T15:05:00Z",
  "firmware_version": "GW 2.3.4",
  "event": "mesh_event",
  "event_type": "node_join",
  "mesh_id": "MESH-001",
  "affected_nodes": ["ALT-441D64F804A0"],
  "details": {
    "total_nodes": 4,
    "timestamp": "2025-01-12T15:05:00Z"
  }
}
```

#### Node Leave Event

```json
{
  "schema_version": 1,
  "device_id": "ALT-6825DD341CA4",
  "device_type": "gateway",
  "timestamp": "2025-01-12T15:10:00Z",
  "firmware_version": "GW 2.3.4",
  "event": "mesh_event",
  "event_type": "node_leave",
  "mesh_id": "MESH-001",
  "affected_nodes": ["ALT-441D64F804A0"],
  "details": {
    "reason": "connection_lost",
    "last_seen": "2025-01-12T15:08:45Z",
    "total_nodes": 3
  }
}
```

#### Connection Lost Event

```json
{
  "event": "mesh_event",
  "event_type": "connection_lost",
  "affected_nodes": ["ALT-441D64F804A0"],
  "details": {
    "reason": "timeout",
    "timestamp": "2025-01-12T15:10:00Z"
  }
}
```

### Integration with Web Dashboards

**Subscribe to events:**

```javascript
const mqtt = require('mqtt');
const client = mqtt.connect('mqtt://broker.local:1883');

client.subscribe('alteriom/mesh/+/events');
client.subscribe('alteriom/mesh/+/topology');

client.on('message', (topic, message) => {
  const data = JSON.parse(message.toString());
  
  if (data.event === 'mesh_event') {
    switch (data.event_type) {
      case 'node_join':
        console.log(`✅ Node joined: ${data.affected_nodes[0]}`);
        break;
      case 'node_leave':
        console.log(`⚠️  Node left: ${data.affected_nodes[0]}`);
        break;
    }
  }
  
  if (data.event === 'mesh_topology') {
    console.log(`📊 Topology update: ${data.nodes.length} nodes`);
    // Render network graph with D3.js or similar
  }
});
```

**Python example:**

```python
import paho.mqtt.client as mqtt
import json

def on_message(client, userdata, msg):
    data = json.loads(msg.payload.decode())
    
    if data['event'] == 'mesh_event':
        event_type = data['event_type']
        nodes = data['affected_nodes']
        print(f"Event: {event_type} - Nodes: {nodes}")
    
    elif data['event'] == 'mesh_topology':
        total_nodes = data['metrics']['total_nodes']
        quality = data['metrics']['avg_connection_quality']
        print(f"Topology: {total_nodes} nodes, quality: {quality}%")

client = mqtt.Client()
client.on_message = on_message
client.connect("broker.local", 1883)
client.subscribe("alteriom/mesh/+/events")
client.subscribe("alteriom/mesh/+/topology")
client.loop_forever()
```

---

## Complete Examples

### Example 1: Send LED Control Command via MQTT

**Publish to:** `mesh/command/123456`

```json
{
  "type": 201,
  "command": 10,
  "targetDevice": 123456,
  "commandId": 1001,
  "parameters": "{\"state\":true,\"brightness\":75}"
}
```

**Expected Response on:** `mesh/response/123456`

```json
{
  "type": 202,
  "from": 123456,
  "deviceStatus": 0,
  "uptime": 3600,
  "freeMemory": 45,
  "firmwareVersion": "1.0.0",
  "responseToCommand": 1001,
  "responseMessage": "LED ON"
}
```

### Example 2: Broadcast Configuration Request

**Publish to:** `mesh/command/broadcast`

```json
{
  "type": 201,
  "command": 100,
  "targetDevice": 0,
  "commandId": 2001,
  "parameters": "{}"
}
```

All nodes respond with their configuration on respective `mesh/config/<nodeId>` topics.

### Example 3: Update Node Configuration

**Publish to:** `mesh/config/123456/set`

```json
{
  "config": {
    "deviceName": "Sensor-Living-Room",
    "sampleRate": 30000,
    "ledEnabled": false
  }
}
```

**Expected Response on:** `mesh/response/123456`

```json
{
  "status": "success",
  "device_id": "123456",
  "message": "Configuration updated"
}
```

### Example 4: Request Device Status

**Publish to:** `mesh/command/123456`

```json
{
  "type": 201,
  "command": 200,
  "targetDevice": 123456,
  "commandId": 3001,
  "parameters": "{}"
}
```

**Expected Response on:** `mesh/status/123456`

```json
{
  "type": 202,
  "from": 123456,
  "deviceStatus": 0,
  "uptime": 7200,
  "freeMemory": 42,
  "firmwareVersion": "1.0.0",
  "responseToCommand": 3001,
  "responseMessage": "OK"
}
```

---

## Using with Web Applications

### JavaScript/Node.js Example

```javascript
const mqtt = require('mqtt');
const client = mqtt.connect('mqtt://192.168.1.100:1883');

client.on('connect', () => {
  console.log('Connected to MQTT broker');
  
  // Subscribe to response topics
  client.subscribe('mesh/response/#');
  client.subscribe('mesh/status/#');
  
  // Send LED control command
  const command = {
    type: 201,
    command: 10,
    targetDevice: 123456,
    commandId: Date.now(),
    parameters: JSON.stringify({
      state: true,
      brightness: 75
    })
  };
  
  client.publish('mesh/command/123456', JSON.stringify(command));
});

client.on('message', (topic, message) => {
  console.log(`Received on ${topic}:`, message.toString());
  const response = JSON.parse(message.toString());
  
  if (response.responseToCommand) {
    console.log(`Command ${response.responseToCommand} result: ${response.responseMessage}`);
  }
});
```

### Python Example

```python
import paho.mqtt.client as mqtt
import json
import time

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    client.subscribe("mesh/response/#")
    client.subscribe("mesh/status/#")
    
    # Send LED control command
    command = {
        "type": 201,
        "command": 10,
        "targetDevice": 123456,
        "commandId": int(time.time() * 1000),
        "parameters": json.dumps({
            "state": True,
            "brightness": 75
        })
    }
    
    client.publish("mesh/command/123456", json.dumps(command))

def on_message(client, userdata, msg):
    print(f"Received on {msg.topic}: {msg.payload.decode()}")
    response = json.loads(msg.payload.decode())
    
    if "responseToCommand" in response:
        print(f"Command {response['responseToCommand']} result: {response['responseMessage']}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("192.168.1.100", 1883, 60)
client.loop_forever()
```

---

## Troubleshooting

### Commands Not Reaching Nodes

**Symptoms:**
- Commands published to MQTT but nodes don't respond
- No error messages in gateway logs

**Solutions:**
1. **Check MQTT Connection**: Verify bridge is connected to broker
   ```cpp
   if (mqttClient.connected()) {
     Serial.println("MQTT connected");
   }
   ```

2. **Verify Topic Format**: Ensure exact topic structure with nodeId
   ```
   Correct: mesh/command/123456
   Wrong:   mesh/commands/123456
   Wrong:   mesh/command/0x1E240
   ```

3. **Check JSON Format**: Validate command payload structure
   ```bash
   # Use mosquitto_pub to test
   mosquitto_pub -h 192.168.1.100 -t "mesh/command/123456" -m '{"type":201,"command":10,"targetDevice":123456,"commandId":1001,"parameters":"{}"}'
   ```

4. **Inspect Mesh Connectivity**: Use `mesh.getNodeList()` to verify nodes
   ```cpp
   auto nodes = mesh.getNodeList();
   Serial.printf("Connected nodes: %d\n", nodes.size());
   ```

5. **Enable Debug Logging**:
   ```cpp
   mesh.setDebugMsgTypes(ERROR | CONNECTION | COMMUNICATION);
   ```

### No Response from Nodes

**Symptoms:**
- Commands reach nodes but no response received
- Node serial shows command execution but no MQTT response

**Solutions:**
1. **Verify Command Handler**: Ensure nodes have command handler implemented
2. **Check Command ID**: Confirm command ID is supported by node
3. **Monitor Serial Output**: Check node serial for command reception
4. **Validate Parameters**: Ensure parameter JSON is valid
5. **Check Response Routing**: Verify responses are reaching gateway

### MQTT Broker Issues

**Symptoms:**
- Gateway can't connect to broker
- Messages not being delivered

**Solutions:**
1. **Connection Refused**: Check broker IP, port, credentials
   ```cpp
   mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
   if (!mqttClient.connect("painlessMesh-gateway")) {
     Serial.printf("MQTT error: %d\n", mqttClient.state());
   }
   ```

2. **Topic Not Found**: Verify subscription before publishing
   ```cpp
   // Subscribe first
   mqtt.subscribe("mesh/command/#");
   delay(100);
   // Then publish
   mqtt.publish("mesh/response/123", "test");
   ```

3. **QoS Issues**: Use QoS 1 for reliable delivery
   ```cpp
   mqtt.publish(topic, payload, true); // retained = true
   ```

4. **Retained Messages**: Clear retained messages if needed
   ```bash
   mosquitto_pub -h 192.168.1.100 -t "mesh/command/123456" -n -r
   ```

### Memory Issues

**Symptoms:**
- Gateway crashes or reboots unexpectedly
- Commands work initially but fail after time

**Solutions:**
1. **Monitor Heap**: Check free memory regularly
   ```cpp
   Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
   ```

2. **Optimize JSON Buffer Size**: Use appropriate buffer sizes
   ```cpp
   // Too large wastes memory
   DynamicJsonDocument doc(1024); // Adjust based on needs
   ```

3. **Clean Up Objects**: Delete unused objects
   ```cpp
   delete commandBridge; // If recreating
   ```

4. **Reduce Debug Output**: Disable verbose logging in production

### Network Stability

**Symptoms:**
- Intermittent command delivery
- Nodes dropping from mesh

**Solutions:**
1. **Check WiFi Signal**: Ensure strong WiFi for gateway
2. **Reduce Mesh Traffic**: Space out command sends
3. **Use Exponential Backoff**: Retry failed commands with delay
4. **Monitor Mesh Health**: Track connection changes

---

## Best Practices

### Command Design

1. **Use Unique Command IDs**: Generate unique IDs for tracking
   ```cpp
   cmd.commandId = millis() | (nodeId << 16);
   ```

2. **Keep Parameters Small**: Minimize JSON parameter size
   ```json
   Good:  {"s":1,"b":75}
   Avoid: {"state":"enabled","brightness_level":75,"extra_field":"unused"}
   ```

3. **Implement Timeouts**: Don't wait indefinitely for responses
   ```javascript
   const timeout = setTimeout(() => {
     console.log('Command timeout');
   }, 5000);
   ```

4. **Handle Failures Gracefully**: Retry important commands
   ```javascript
   let retries = 3;
   function sendCommand() {
     client.publish(topic, command);
     setTimeout(() => {
       if (!responseReceived && retries-- > 0) {
         sendCommand();
       }
     }, 2000);
   }
   ```

### Security Considerations

1. **Use Authentication**: Enable MQTT broker authentication
2. **Validate Commands**: Check command bounds and parameters
3. **Rate Limiting**: Limit command frequency per client
4. **Access Control**: Restrict sensitive commands
5. **Encryption**: Use TLS for production MQTT connections

### Performance Optimization

1. **Batch Commands**: Group related commands when possible
2. **Cache Configuration**: Avoid repeated config requests
3. **Use Broadcast Sparingly**: Unicast when targeting specific nodes
4. **Monitor Latency**: Track command response times

---

## Related Documentation

- [OTA Commands Reference](OTA_COMMANDS_REFERENCE.md) - Firmware update commands
- [API Reference](api/core-api.md) - Core painlessMesh API
- [Plugin System](architecture/plugin-system.md) - Custom package development
- [Alteriom Overview](alteriom/overview.md) - Alteriom package types

---

**Last Updated:** October 2025  
**painlessMesh Version:** 1.7.0+ Alteriom Fork  
**Author:** Alteriom Development Team
