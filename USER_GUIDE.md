# AlteriomPainlessMesh User Guide

> **Complete guide to building mesh networks with ESP8266/ESP32 devices**

This comprehensive guide covers everything you need to build production-ready mesh networks using the AlteriomPainlessMesh library.

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Core Concepts](#core-concepts)
4. [API Reference](#api-reference)
5. [Alteriom Extensions](#alteriom-extensions)
6. [Bridge to Internet](#bridge-to-internet)
7. [Advanced Features](#advanced-features)
8. [Troubleshooting](#troubleshooting)
9. [Examples](#examples)

---

## Introduction

### What is AlteriomPainlessMesh?

**AlteriomPainlessMesh** is a user-friendly library for creating self-organizing mesh networks with ESP8266 and ESP32 devices. This **Alteriom fork** extends the original painlessMesh library with specialized packages for IoT sensor networks, device control, and status monitoring.

### Key Features

**Core Features:**
- **Automatic Mesh Formation** - Nodes discover and connect automatically
- **Self-Healing Network** - Adapts when nodes join/leave
- **Time Synchronization** - Coordinated actions across all nodes  
- **Smart Routing** - Broadcast, point-to-point, and neighbor messaging
- **Plugin System** - Type-safe custom message packages
- **ESP32 & ESP8266 Support** - Full support for both platforms
- **Memory Efficient** - Optimized for resource-constrained devices

**Alteriom Extensions (v1.7.0+):**
- **Broadcast OTA Distribution** - 98% network traffic reduction for large meshes
- **MQTT Bridge** - Professional monitoring with Grafana/InfluxDB/Prometheus
- **Structured Packages** - SensorPackage, CommandPackage, StatusPackage
- **Bridge Failover** - Automatic high-availability for critical systems
- **Message Queue** - Zero data loss during Internet outages

### Why Use painlessMesh?

**True Ad-Hoc Networking**
- No planning, central controller, or router required
- Any system of 1 or more nodes will self-organize
- Maximum mesh size limited only by available memory

**JSON-Based Messaging**
- Human-readable and easy to understand
- Simple integration with web applications
- Straightforward debugging and development

**Production Ready**
- Tested in industrial deployments
- Enterprise-grade stability and performance
- Comprehensive error handling and recovery

---

## Getting Started

### What You'll Need

- **Hardware**: 2 or more ESP8266 or ESP32 development boards
- **Software**: Arduino IDE or PlatformIO
- **Accessories**: USB cables for programming

### Installation

#### Arduino Library Manager

**Manual Installation (Current Method):**

1. Go to [Releases](https://github.com/Alteriom/painlessMesh/releases/latest)
2. Download the latest release ZIP file
3. In Arduino IDE: **Sketch** → **Include Library** → **Add .ZIP Library...**
4. Select the downloaded ZIP file
5. Restart Arduino IDE

**Or via Git Clone:**
```bash
cd ~/Arduino/libraries/
git clone https://github.com/Alteriom/painlessMesh.git AlteriomPainlessMesh
# Restart Arduino IDE
```

#### PlatformIO

Add to your `platformio.ini`:
```ini
lib_deps = 
    https://github.com/Alteriom/painlessMesh.git
```

#### Dependencies

painlessMesh requires these libraries (auto-installed with PlatformIO):
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) v7.x
- [TaskScheduler](https://github.com/arkhipenko/TaskScheduler)
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) (ESP8266)
- [AsyncTCP](https://github.com/ESP32Async/AsyncTCP) (ESP32) - v3.3.0+ for ESP32-C6

### Your First Mesh Network

#### Step 1: Basic Example Code

Copy this code to your Arduino IDE:

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Task to send messages
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void sendMessage() {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast(msg);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  mesh.update();
}
```

#### Step 2: Upload and Test

1. Upload the code to your first ESP device
2. Open Serial Monitor (115200 baud)
3. Upload the same code to your second device
4. Watch them automatically discover each other!

#### Expected Output

```
New Connection, nodeId = 123456789
Received from 123456789: Hello from node 123456789
Changed connections
Adjusted time 1234567. Offset = 12
```

---

## Core Concepts

### Mesh Networking Basics

**What is a Mesh Network?**

A mesh network is a network topology where each node can relay data for other nodes. Unlike traditional star or tree topologies with a central hub, mesh networks are decentralized and self-organizing.

**How painlessMesh Works:**

1. **Node Discovery**: Nodes scan for other nodes broadcasting the same MESH_PREFIX
2. **Automatic Connection**: Nodes connect to the strongest available signals
3. **Route Building**: Each node maintains a routing table of the network topology
4. **Message Routing**: Messages are automatically routed through intermediate nodes
5. **Self-Healing**: When nodes disconnect, routes automatically reconfigure

### Node Identification

**Node ID:**
- Each ESP device has a unique 32-bit chip ID
- Retrieved automatically using `system_get_chip_id()`
- Used for targeted messaging: `mesh.sendSingle(nodeId, msg)`

**Getting Your Node ID:**
```cpp
uint32_t myNodeId = mesh.getNodeId();
Serial.printf("My Node ID: %u\n", myNodeId);
```

### Message Types

**Broadcast Messages:**
- Sent to all nodes in the mesh
- Propagate through the entire network
- Use for sensor data, status updates, alerts

```cpp
mesh.sendBroadcast("Hello everyone!");
```

**Single (Targeted) Messages:**
- Sent to a specific node by ID
- Automatically routed through the mesh
- Use for commands, replies, private data

```cpp
uint32_t targetNode = 123456789;
mesh.sendSingle(targetNode, "Hello specific node!");
```

### Time Synchronization

**Mesh Time:**
- All nodes maintain a synchronized timebase
- Uses SNTP-based protocol for synchronization
- Returns microseconds since mesh startup
- Rolls over every 71 minutes

```cpp
uint32_t meshTime = mesh.getNodeTime();
Serial.printf("Mesh time: %u µs\n", meshTime);
```

**Use Cases:**
- Coordinated LED displays
- Synchronized sensor readings
- Event scheduling across nodes
- Timestamp correlation

### Network Topology

**Getting Node List:**
```cpp
std::list<uint32_t> nodes = mesh.getNodeList();
Serial.printf("Connected nodes: %d\n", nodes.size());
for (auto nodeId : nodes) {
  Serial.printf("  - Node: %u\n", nodeId);
}
```

**Topology as JSON:**
```cpp
String topology = mesh.subConnectionJson();
Serial.println(topology);
```

**Checking Connections:**
```cpp
uint32_t nodeId = 123456789;
if (mesh.isConnected(nodeId)) {
  Serial.println("Node is connected");
}
```

**Note:** Connection status can change rapidly in dynamic mesh networks. This method reflects the current state at the moment of the call.

---

## API Reference

### painlessMesh Class

#### Initialization

**`init()`** - Initialize the mesh network

```cpp
void init(String ssid, String password, uint16_t port = 5555);
void init(String ssid, String password, Scheduler* scheduler, uint16_t port = 5555);
void init(String ssid, String password, Scheduler* scheduler, uint16_t port, 
          WiFiMode_t mode, uint8_t channel, phy_mode_t phymode, 
          uint8_t maxtpw, uint8_t hidden, uint8_t maxconn);
```

**Parameters:**
- `ssid` - Network name (same for all nodes)
- `password` - Network password
- `scheduler` - Optional TaskScheduler instance
- `port` - TCP port for mesh communication (default: 5555)
- `mode` - WiFi mode: WIFI_AP, WIFI_STA, or WIFI_AP_STA (default)
- `channel` - WiFi channel (1-13, or 0 for auto-detect)

**Example:**
```cpp
Scheduler userScheduler;
mesh.init("MyMesh", "password123", &userScheduler, 5555);
```

**`stop()`** - Stop the node

```cpp
void stop();
```

Disconnects from all nodes and stops sending/receiving messages.

**`update()`** - Main mesh update loop

```cpp
void update();
```

Call this in your `loop()` function. Handles all mesh maintenance tasks.

```cpp
void loop() {
  mesh.update(); // Required
}
```

#### Message Sending

**`sendBroadcast()`** - Send to all nodes

```cpp
bool sendBroadcast(String &msg, bool includeSelf = false);
```

**Parameters:**
- `msg` - Message to broadcast
- `includeSelf` - Send to yourself (default: false)

**Returns:** `true` if successful

**`sendSingle()`** - Send to specific node

```cpp
bool sendSingle(uint32_t dest, String &msg);
```

**Parameters:**
- `dest` - Destination node ID
- `msg` - Message to send

**Returns:** `true` if successful

#### Callbacks

**`onReceive()`** - Message received callback

```cpp
void onReceive(receivedCallback_t callback);
```

**Callback signature:**
```cpp
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("From %u: %s\n", from, msg.c_str());
}
```

**`onNewConnection()`** - New node connected

```cpp
void onNewConnection(newConnectionCallback_t callback);
```

**Callback signature:**
```cpp
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New node: %u\n", nodeId);
}
```

**`onChangedConnections()`** - Topology changed

```cpp
void onChangedConnections(changedConnectionsCallback_t callback);
```

**Callback signature:**
```cpp
void changedConnectionCallback() {
  Serial.println("Network topology changed");
}
```

**`onNodeTimeAdjusted()`** - Time synchronized

```cpp
void onNodeTimeAdjusted(nodeTimeAdjustedCallback_t callback);
```

**Callback signature:**
```cpp
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Time adjusted: %d µs\n", offset);
}
```

**`onNodeDelayReceived()`** - Delay measurement response

```cpp
void onNodeDelayReceived(nodeDelayCallback_t callback);
```

**Callback signature:**
```cpp
void onDelayReceived(uint32_t nodeId, int32_t delay) {
  Serial.printf("Delay to %u: %d µs\n", nodeId, delay);
}
```

#### Network Information

**`getNodeId()`** - Get this node's ID

```cpp
uint32_t getNodeId();
```

**`getNodeList()`** - Get list of all known nodes

```cpp
std::list<uint32_t> getNodeList();
```

**`getNodeTime()`** - Get mesh timebase (microseconds)

```cpp
uint32_t getNodeTime();
```

**`isConnected()`** - Check if node is connected

```cpp
bool isConnected(uint32_t nodeId);
```

**`subConnectionJson()`** - Get topology as JSON

```cpp
String subConnectionJson();
```

#### Network Measurement

**`startDelayMeas()`** - Measure network delay

```cpp
bool startDelayMeas(uint32_t nodeId);
```

Sends a packet to measure round-trip delay. Result delivered via `onNodeDelayReceived()` callback.

#### Debug Configuration

**`setDebugMsgTypes()`** - Configure debug output

```cpp
void setDebugMsgTypes(uint16_t types);
```

**Debug types:**
- `ERROR` - Error messages
- `STARTUP` - Startup information
- `CONNECTION` - Connection events
- `SYNC` - Time synchronization
- `COMMUNICATION` - Message routing
- `GENERAL` - General information
- `MSG_TYPES` - Message type details
- `REMOTE` - Remote node information

**Example:**
```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
```

---

## Alteriom Extensions

### Overview

Alteriom extensions provide production-ready, type-safe packages for common IoT scenarios:

- **Environmental Monitoring** - Temperature, humidity, pressure sensors
- **Device Control** - Commands for actuators, displays, relays
- **Health Monitoring** - Device status, diagnostics, telemetry
- **Type Safety** - Compile-time validation
- **Production Ready** - Tested and optimized implementations

### SensorPackage (Type 200)

For broadcasting environmental sensor data.

**Fields:**
- `temperature` (double) - Temperature in °C
- `humidity` (double) - Relative humidity (%)
- `pressure` (double) - Atmospheric pressure (hPa)
- `sensorId` (uint32_t) - Sensor identifier
- `timestamp` (uint32_t) - Measurement time (mesh time)
- `batteryLevel` (uint8_t) - Battery percentage (0-100)

**Example:**
```cpp
#include "examples/alteriom/alteriom_sensor_package.hpp"
using namespace alteriom;

SensorPackage sensor;
sensor.from = mesh.getNodeId();
sensor.temperature = 23.5;
sensor.humidity = 65.0;
sensor.pressure = 1013.25;
sensor.sensorId = 1001;
sensor.timestamp = mesh.getNodeTime();
sensor.batteryLevel = 85;

String json = sensor.toJsonString();
mesh.sendBroadcast(json);
```

**Receiving:**
```cpp
void receivedCallback(uint32_t from, String &msg) {
  JsonDocument doc;
  deserializeJson(doc, msg);
  
  if (doc["type"] == 200) {
    SensorPackage sensor(doc.as<JsonObject>());
    Serial.printf("Sensor %u: %.1f°C, %.1f%% RH, %.1f hPa\n",
                 sensor.sensorId, sensor.temperature, 
                 sensor.humidity, sensor.pressure);
  }
}
```

**Use Cases:**
- Weather stations
- Environmental monitoring
- HVAC feedback
- Greenhouse automation
- Industrial sensors

### CommandPackage (Type 400)

For sending control commands to specific devices.

**Fields:**
- `command` (uint8_t) - Command type
- `targetDevice` (uint32_t) - Device identifier
- `parameters` (String) - JSON parameters
- `commandId` (uint32_t) - Unique command ID

**Example:**
```cpp
CommandPackage cmd;
cmd.dest = targetNodeId;
cmd.from = mesh.getNodeId();
cmd.command = 1; // LED_CONTROL
cmd.targetDevice = 100;
cmd.parameters = "{\"brightness\":75,\"color\":\"blue\"}";
cmd.commandId = generateCommandId();

String json = cmd.toJsonString();
mesh.sendSingle(targetNodeId, json);
```

**Handling Commands:**
```cpp
void receivedCallback(uint32_t from, String &msg) {
  JsonDocument doc;
  deserializeJson(doc, msg);
  
  if (doc["type"] == 400) {
    CommandPackage cmd(doc.as<JsonObject>());
    
    if (cmd.dest != mesh.getNodeId()) return;
    
    // Parse parameters
    JsonDocument params;
    deserializeJson(params, cmd.parameters);
    
    int brightness = params["brightness"];
    String color = params["color"];
    
    // Execute command
    setLED(cmd.targetDevice, brightness, color);
  }
}
```

**Use Cases:**
- Remote device control
- Actuator management
- Display updates
- System configuration
- Automation triggers

### StatusPackage (Type 202)

For broadcasting device health and operational status.

**Fields:**
- `deviceStatus` (uint8_t) - Status flags
- `uptime` (uint32_t) - Uptime in seconds
- `freeMemory` (uint16_t) - Free heap memory (bytes)
- `wifiStrength` (uint8_t) - WiFi RSSI (dBm)
- `firmwareVersion` (String) - Firmware version

**Example:**
```cpp
StatusPackage status;
status.from = mesh.getNodeId();
status.deviceStatus = 1; // OPERATIONAL
status.uptime = millis() / 1000;
status.freeMemory = ESP.getFreeHeap();
status.wifiStrength = WiFi.RSSI();
status.firmwareVersion = "1.2.3";

String json = status.toJsonString();
mesh.sendBroadcast(json);
```

**Use Cases:**
- System monitoring
- Predictive maintenance
- Network diagnostics
- Performance tracking
- Remote troubleshooting

### Advanced Packages (v1.7.0+)

**MetricsPackage (Type 204)** - Comprehensive performance metrics
- CPU usage, memory health, network throughput
- Response time and latency tracking
- Connection quality monitoring

**HealthCheckPackage (Type 605)** - Proactive problem detection
- Overall health scoring (0-100)
- Problem flag indicators
- Memory leak detection
- Predictive maintenance

**Bridge Packages (v1.8.0+):**
- **BridgeStatusPackage (Type 610)** - Bridge health monitoring
- **BridgeElectionPackage (Type 611)** - Automatic failover election
- **BridgeTakeoverPackage (Type 612)** - Bridge role announcement
- **NTPTimeSyncPackage (Type 614)** - NTP time synchronization

See README.md for complete package documentation.

---

## Bridge to Internet

### Overview

Connect your mesh network to the Internet by creating a bridge node that connects to both the mesh and your WiFi router.

### Quick Start (Auto Channel Detection)

The recommended approach uses `initAsBridge()` which automatically detects your router's channel:

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "somethingSneaky"
#define ROUTER_SSID     "YourRouterSSID"
#define ROUTER_PASSWORD "YourRouterPassword"

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  
  // Automatically detects router channel and configures mesh
  mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                    ROUTER_SSID, ROUTER_PASSWORD,
                    &userScheduler, 5555);
  
  mesh.onReceive(&receivedCallback);
}

void loop() {
  mesh.update();
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Mesh message from %u: %s\n", from, msg.c_str());
  // Forward to Internet services (HTTP, MQTT, etc.)
}
```

### Bridge Failover (v1.8.0+)

Automatic high-availability for critical systems:

**Features:**
- RSSI-based election (best signal wins)
- 60-second failure detection via heartbeats
- Fast failover (60-70 seconds typical)
- Distributed consensus algorithm
- Split-brain prevention

**Example:**
```cpp
// Enable automatic bridge failover
mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
mesh.enableBridgeFailover(true);
mesh.onBridgeRoleChanged(&bridgeRoleCallback);

void bridgeRoleCallback(bool isBridge, const String& reason) {
  if (isBridge) {
    Serial.printf("🎯 Promoted to bridge: %s\n", reason.c_str());
    // Start MQTT connection, etc.
  } else {
    Serial.printf("Demoted from bridge: %s\n", reason.c_str());
    // Stop bridge services
  }
}
```

### Shared Gateway Mode (v1.9.0+)

All nodes as Internet gateways with automatic failover:

**Features:**
- Universal Internet access (all nodes connect to router)
- Instant failover through mesh relay
- No single point of failure
- Automatic gateway election
- Duplicate prevention

**Example:**
```cpp
// Initialize all nodes as shared gateways
mesh.initAsSharedGateway(
    MESH_PREFIX, MESH_PASSWORD,
    ROUTER_SSID, ROUTER_PASSWORD,
    &userScheduler, MESH_PORT
);

// Send data with automatic failover
mesh.sendToInternet(
    "https://api.example.com/data",
    sensorJson,
    [](bool success, uint16_t status, String error) {
        Serial.printf("Delivery: %s\n", success ? "OK" : error.c_str());
    }
);
```

For complete bridge documentation, see [BRIDGE_TO_INTERNET.md](BRIDGE_TO_INTERNET.md).

---

## Advanced Features

### Message Queue (v1.8.2+)

Zero data loss during Internet outages:

```cpp
// Enable message queue
mesh.enableMessageQueue(true);
mesh.setMaxQueueSize(100);

// Queue critical message
String criticalAlarm = "{\"sensor\":\"O2\",\"value\":2.5,\"alarm\":true}";
mesh.queueMessage(criticalAlarm, CRITICAL);

// Set callbacks
mesh.onQueueFull(&queueFullCallback);
mesh.onQueueFlushed(&queueFlushedCallback);
```

**Priority Levels:**
- `CRITICAL` - Never dropped
- `HIGH` - High priority
- `NORMAL` - Normal priority
- `LOW` - Dropped first when queue full

### Broadcast OTA (v1.7.0+)

Efficient firmware distribution for large meshes:

```cpp
// Enable broadcast OTA mode
mesh.initOTA("MyFirmware", "1.2.3", [](size_t progress, size_t total) {
  Serial.printf("OTA Progress: %d%%\n", (progress * 100) / total);
});

// 98% network traffic reduction for 50+ node meshes
// All nodes receive firmware simultaneously
// Scales to 100+ nodes efficiently
```

### MQTT Bridge

Professional monitoring integration:

```cpp
// Connect to MQTT broker
mesh.connectMQTT("mqtt://broker.example.com", 1883);

// Publish mesh data
mesh.onReceive([](uint32_t from, String &msg) {
  String topic = "mesh/" + String(from) + "/data";
  mqttClient.publish(topic.c_str(), msg.c_str());
});

// Subscribe to commands
mqttClient.subscribe("mesh/+/command");
mqttClient.onMessage([](String topic, String payload) {
  // Extract node ID from topic and forward command
  mesh.sendSingle(nodeId, payload);
});
```

### Multi-Bridge Coordination (v1.8.2+)

Enterprise load balancing:

```cpp
// Primary bridge (priority 10)
mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                  ROUTER_SSID, ROUTER_PASSWORD,
                  &userScheduler, MESH_PORT, 10);

// Configure load balancing
mesh.setBridgeSelectionStrategy(ROUND_ROBIN);

// Monitor coordination
mesh.onBridgeCoordination(&bridgeCoordinationCallback);
```

**Strategies:**
- `PRIORITY_BASED` - Use highest priority bridge
- `ROUND_ROBIN` - Distribute load evenly
- `BEST_SIGNAL` - Use best RSSI bridge

---

## Troubleshooting

### Common Issues

#### Nodes Not Connecting

**Symptoms:** Nodes don't discover each other

**Solutions:**
1. Verify MESH_PREFIX and MESH_PASSWORD are identical
2. Check devices are within WiFi range (~50-100m)
3. Ensure MESH_PORT is the same on all devices
4. Try different WiFi channels (avoid congested channels)
5. Check for physical obstacles blocking signal

#### Messages Not Received

**Symptoms:** Messages sent but not received

**Solutions:**
1. Check message size (keep under 1KB for reliability)
2. Verify callback is registered: `mesh.onReceive(&callback)`
3. Add debug output: `mesh.setDebugMsgTypes(ERROR | COMMUNICATION)`
4. Ensure `mesh.update()` is called in `loop()`
5. Check for network congestion (reduce message frequency)

#### High Memory Usage

**Symptoms:** Device crashes or becomes unstable

**Solutions:**
1. Monitor free heap: `ESP.getFreeHeap()`
2. Reduce message frequency
3. Limit mesh size (max ~50 nodes for ESP8266)
4. Use shorter messages
5. Clear unused String objects
6. Reduce TaskScheduler task count

#### Bridge Connection Issues

**Symptoms:** Bridge can't maintain router connection

**Solutions:**
1. Use `initAsBridge()` for automatic channel detection
2. Ensure router channel matches mesh channel
3. Check router signal strength (RSSI > -70 dBm)
4. Enable bridge debug: `mesh.setDebugMsgTypes(ERROR | CONNECTION)`
5. Try static IP configuration on router

### Debug Configuration

Enable verbose debugging:

```cpp
void setup() {
  Serial.begin(115200);
  
  // Enable all debug messages
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | 
                        SYNC | COMMUNICATION | GENERAL);
  
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}
```

### Performance Optimization

**Reduce Network Congestion:**
```cpp
// Use rate limiting
static unsigned long lastBroadcast = 0;
const unsigned long MIN_INTERVAL = 1000; // 1 second

if (millis() - lastBroadcast > MIN_INTERVAL) {
  mesh.sendBroadcast(msg);
  lastBroadcast = millis();
}
```

**Optimize Message Size:**
```cpp
// Bad: Large JSON with unnecessary data
String msg = "{\"temperature\":23.5,\"humidity\":65.0,\"description\":\"This is a very long description...\"}";

// Good: Compact JSON with essential data
String msg = "{\"t\":23.5,\"h\":65}";
```

### Best Practices

1. **Avoid `delay()`** - Use TaskScheduler instead
2. **Conservative messaging** - Don't spam the network
3. **Handle failures gracefully** - Messages can be dropped
4. **Monitor memory** - Especially on ESP8266
5. **Use retries for critical messages** - Implement acknowledgments
6. **Test at scale** - Test with expected number of nodes

For more troubleshooting help, see:
- [Common Issues](docs/troubleshooting/common-issues.md)
- [FAQ](docs/troubleshooting/faq.md)
- [Debugging Guide](docs/troubleshooting/debugging.md)

---

## Examples

### Basic Mesh Network

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMesh"
#define MESH_PASSWORD   "password"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, [](){
  String msg = "Hello from " + String(mesh.getNodeId());
  mesh.sendBroadcast(msg);
});

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("From %u: %s\n", from, msg.c_str());
}

void setup() {
  Serial.begin(115200);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  mesh.update();
}
```

### Sensor Network with Alteriom Packages

```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

#define MESH_PREFIX     "SensorMesh"
#define MESH_PASSWORD   "password"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// DHT22 sensor pins
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

Task taskSendSensor(30000, TASK_FOREVER, [](){
  SensorPackage sensor;
  sensor.from = mesh.getNodeId();
  sensor.temperature = dht.readTemperature();
  sensor.humidity = dht.readHumidity();
  sensor.sensorId = mesh.getNodeId();
  sensor.timestamp = mesh.getNodeTime();
  sensor.batteryLevel = readBattery();
  
  if (!isnan(sensor.temperature) && !isnan(sensor.humidity)) {
    mesh.sendBroadcast(sensor.toJsonString());
  }
});

void receivedCallback(uint32_t from, String &msg) {
  JsonDocument doc;
  deserializeJson(doc, msg);
  
  if (doc["type"] == 200) {
    SensorPackage sensor(doc.as<JsonObject>());
    Serial.printf("Sensor %u: %.1f°C, %.1f%% RH\n",
                 sensor.sensorId, sensor.temperature, sensor.humidity);
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  
  userScheduler.addTask(taskSendSensor);
  taskSendSensor.enable();
}

void loop() {
  mesh.update();
}
```

### Bridge with MQTT

```cpp
#include "painlessMesh.h"
#include <PubSubClient.h>

#define MESH_PREFIX     "MyMesh"
#define MESH_PASSWORD   "password"
#define ROUTER_SSID     "YourRouter"
#define ROUTER_PASSWORD "RouterPassword"

Scheduler userScheduler;
painlessMesh mesh;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

void receivedCallback(uint32_t from, String &msg) {
  // Forward mesh messages to MQTT
  String topic = "mesh/" + String(from) + "/data";
  mqttClient.publish(topic.c_str(), msg.c_str());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Forward MQTT messages to mesh
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  // Extract node ID from topic: mesh/<nodeId>/command
  String topicStr = String(topic);
  int startIdx = topicStr.indexOf("/") + 1;
  int endIdx = topicStr.lastIndexOf("/");
  String nodeIdStr = topicStr.substring(startIdx, endIdx);
  uint32_t nodeId = nodeIdStr.toInt();
  
  mesh.sendSingle(nodeId, msg);
}

void setup() {
  Serial.begin(115200);
  
  // Initialize bridge
  mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                    ROUTER_SSID, ROUTER_PASSWORD,
                    &userScheduler, 5555);
  mesh.onReceive(&receivedCallback);
  
  // Connect to MQTT
  mqttClient.setServer("mqtt.example.com", 1883);
  mqttClient.setCallback(mqttCallback);
  mqttClient.connect("painlessMeshBridge");
  mqttClient.subscribe("mesh/+/command");
}

void loop() {
  mesh.update();
  mqttClient.loop();
}
```

### More Examples

Complete examples are available in the repository (16 examples total):

**Getting Started:**
- [Start Here](examples/startHere/startHere.ino) - Best starting point for beginners
- [Basic Example](examples/basic/basic.ino) - Essential patterns and techniques

**IoT & Sensors:**
- [Alteriom Sensor Node](examples/alteriom/alteriom.ino) - IoT sensor packages
- [Named Mesh](examples/namedMesh/namedMesh.ino) - Using node names instead of IDs

**Internet Connectivity:**
- [Bridge Example](examples/bridge/bridge.ino) - Basic Internet bridge
- [Bridge Failover](examples/bridge_failover/) - High availability bridge
- [Shared Gateway](examples/sharedGateway/) - All nodes as gateways
- [Send to Internet](examples/sendToInternet/) - Direct Internet communication
- [MQTT Bridge](examples/mqttBridge/) - MQTT integration

**Advanced Features:**
- [Priority Queue](examples/priority/) - Message priority handling
- [OTA Sender](examples/otaSender/) - Firmware update distribution
- [OTA Receiver](examples/otaReceiver/) - Firmware update reception
- [Log Server](examples/logServer/) - Centralized logging
- [Log Client](examples/logClient/) - Log forwarding
- [Web Server](examples/webServer/) - Embedded web interface

---

## Additional Resources

### Documentation

- **[Main README](README.md)** - Quick reference and overview
- **[BRIDGE_TO_INTERNET.md](BRIDGE_TO_INTERNET.md)** - Complete bridge guide
- **[API Documentation](https://alteriom.github.io/painlessMesh/)** - Online docs
- **[CHANGELOG](CHANGELOG.md)** - Version history
- **[CONTRIBUTING](CONTRIBUTING.md)** - Contribution guidelines

### Community &amp; Support

- **[GitHub Issues](https://github.com/Alteriom/painlessMesh/issues)** - Bug reports and features
- **[Community Forum](https://groups.google.com/forum/#!forum/painlessmesh-user)** - Community support
- **[GitHub Discussions](https://github.com/Alteriom/painlessMesh/discussions)** - General discussion

### Development

- **[Release Guide](RELEASE_GUIDE.md)** - Release process
- **[Security Policy](SECURITY.md)** - Security information
- **[Code of Conduct](CODE_OF_CONDUCT.md)** - Community guidelines

---

## Version Information

This guide is for **AlteriomPainlessMesh v1.9.6** (December 2025)

For the latest updates and releases, visit:
- [GitHub Releases](https://github.com/Alteriom/painlessMesh/releases)
- [NPM Package](https://www.npmjs.com/package/@alteriom/painlessmesh)
- [PlatformIO Registry](https://registry.platformio.org/libraries/alteriom/AlteriomPainlessMesh)

---

*Made with ❤️ by the painlessMesh community*
