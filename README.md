# AlteriomPainlessMesh

> **📚 [Complete Documentation](https://alteriom.github.io/painlessMesh/)** | **📖 [API Reference](https://alteriom.github.io/painlessMesh/#/api/doxygen)** | **🎯 [Examples](https://alteriom.github.io/painlessMesh/#/tutorials/basic-examples)** | **📋 [Changelog](CHANGELOG.md)**

<div align="center">

**Version 1.9.0** - Major update with improved bridge detection, consolidated examples, and cleaned documentation

[![CI/CD Pipeline](https://github.com/Alteriom/painlessMesh/actions/workflows/ci.yml/badge.svg)](https://github.com/Alteriom/painlessMesh/actions/workflows/ci.yml)
[![Documentation](https://github.com/Alteriom/painlessMesh/actions/workflows/docs.yml/badge.svg)](https://github.com/Alteriom/painlessMesh/actions/workflows/docs.yml)
[![Release](https://github.com/Alteriom/painlessMesh/actions/workflows/release.yml/badge.svg)](https://github.com/Alteriom/painlessMesh/actions/workflows/release.yml)
[![GitHub release](https://img.shields.io/github/v/release/Alteriom/painlessMesh?label=version)](https://github.com/Alteriom/painlessMesh/releases)
[![NPM Version](https://img.shields.io/npm/v/@alteriom/painlessmesh?label=npm)](https://www.npmjs.com/package/@alteriom/painlessmesh)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/alteriom/library/painlessMesh.svg)](https://registry.platformio.org/libraries/alteriom/painlessMesh)
[![Arduino Library Manager](https://img.shields.io/badge/Arduino-Library%20Manager-blue.svg)](https://www.arduino.cc/reference/en/libraries/alteriompainlessmesh/)

</div>

## 🌐 Intro to AlteriomPainlessMesh

**AlteriomPainlessMesh** is a user-friendly library for creating mesh networks with ESP8266 and ESP32 devices. This **Alteriom fork** extends the original painlessMesh library with specialized packages for IoT sensor networks, device control, and status monitoring.

### 🎯 Alteriom Extensions

This fork includes specialized packages for structured IoT communication:

#### Custom Package Types

**Core IoT Packages:**

- **`SensorPackage`** (Type 200) - Environmental data collection
  - Temperature, humidity, pressure monitoring
  - Battery level tracking
  - Sensor ID and timestamp fields
  - Ideal for environmental monitoring and smart agriculture

- **`StatusPackage`** (Type 202) - Basic health monitoring
  - Device status flags and uptime
  - Free memory and WiFi strength
  - Firmware version tracking
  - Command response capability for MQTT bridge

- **`CommandPackage`** (Type 400) - Device control and automation (COMMAND per mqtt-schema v0.7.2+)
  - Targeted command execution
  - JSON parameter support
  - Command tracking with unique IDs
  - Perfect for remote device control

**Advanced Monitoring Packages (Phase 2):**

- **`MetricsPackage`** (Type 204) - Comprehensive performance metrics (SENSOR_METRICS per mqtt-schema v0.7.2+)
  - CPU usage and processing metrics
  - Memory health (heap, fragmentation, max allocation)
  - Network throughput and packet statistics
  - Response time and latency tracking
  - Connection quality and WiFi RSSI
  - Dashboard-ready data collection

- **`HealthCheckPackage`** (Type 605) - Proactive problem detection (MESH_METRICS per mqtt-schema v0.7.2+)
  - Overall health scoring (0-100 for memory, network, performance)
  - Problem flag indicators (16-bit flags for specific issues)
  - Memory leak detection with trend analysis
  - Predictive maintenance (estimated time to failure)
  - Crash tracking and reboot reason codes
  - Actionable recommendations

**Mesh Topology & Management (Phase 2):**

- **`EnhancedStatusPackage`** (Type 604) - Detailed mesh status (MESH_STATUS per mqtt-schema v0.7.2+)
  - Complete mesh statistics (node count, connections, messages)
  - Performance metrics (latency, packet loss, throughput)
  - Alert flags and error reporting
  - Firmware verification with MD5 hash

- **`MeshNodeListPackage`** (Type 600) - Node discovery and inventory (MESH_NODE_LIST per mqtt-schema v0.7.2+)
  - List of all mesh nodes with status (offline/online/unreachable)
  - Signal strength (RSSI) for each node
  - Last seen timestamps
  - Supports up to 50 nodes per message

- **`MeshTopologyPackage`** (Type 601) - Network topology visualization (MESH_TOPOLOGY per mqtt-schema v0.7.2+)
  - Connection graph with link quality
  - Latency measurements per connection
  - Hop count tracking
  - Root/gateway node identification
  - Supports up to 100 connections per message

- **`MeshAlertPackage`** (Type 602) - Network event notifications (MESH_ALERT per mqtt-schema v0.7.2+)
  - Configurable alert types (low memory, node offline, connection lost)
  - Severity levels (info, warning, critical)
  - Metric-based threshold triggering
  - Human-readable alert messages
  - Supports up to 20 alerts per message

- **`MeshBridgePackage`** (Type 603) - Protocol bridging (MESH_BRIDGE per mqtt-schema v0.7.2+)
  - Encapsulates native mesh protocol messages
  - Multi-protocol support (painlessMesh, ESP-NOW, BLE-Mesh)
  - Raw payload with signal strength
  - Gateway node identification
  - Enables heterogeneous mesh networks

**Bridge Failover & High Availability (v1.8.0):**

- **`BridgeStatusPackage`** (Type 610) - Bridge health monitoring (BRIDGE_STATUS per mqtt-schema v0.7.3+)
  - Internet connectivity status
  - Router signal strength (RSSI)
  - Gateway IP and router channel
  - Heartbeat for failure detection

- **`BridgeElectionPackage`** (Type 611) - Automatic failover election (BRIDGE_ELECTION per mqtt-schema v0.7.3+)
  - Router RSSI measurement
  - Node uptime and free memory
  - Distributed consensus protocol
  - RSSI-based winner selection

- **`BridgeTakeoverPackage`** (Type 612) - Bridge role announcement (BRIDGE_TAKEOVER per mqtt-schema v0.7.3+)
  - New bridge identification
  - Previous bridge tracking
  - Takeover reason and timestamp
  - Seamless failover notification

- **`NTPTimeSyncPackage`** (Type 614) - NTP time synchronization (TIME_SYNC_NTP per mqtt-schema v0.7.3+)
  - Unix timestamp from NTP server
  - Accuracy/precision in milliseconds
  - NTP server source identification
  - Mesh-wide time distribution from bridge

All packages provide type-safe serialization, automatic JSON conversion, and mesh-wide broadcasting or targeted messaging. They align with mqtt-schema v0.7.3+ for enterprise IoT integration.

#### 🚀 Phase 2 Features (v1.7.0+)

**Broadcast OTA Distribution**

- 📡 **98% Network Traffic Reduction** for 50+ node meshes
- ⚡ **Parallel Firmware Updates** - All nodes receive simultaneously
- 🔄 **Backward Compatible** - Single parameter enables broadcast mode
- 📊 **Scales to 100+ Nodes** efficiently

**MQTT Status Bridge**

- 🌉 **Professional Monitoring** - Grafana, InfluxDB, Prometheus integration
- 📈 **Real-Time Topology** - Complete mesh visualization over MQTT
- 🎯 **Production Ready** - Enterprise IoT and commercial deployments
- ⚙️ **Fully Configurable** - Adjustable intervals and feature toggles

See [Phase 2 Guide](docs/PHASE2_GUIDE.md) for complete documentation.

#### 🔄 Automatic Bridge Failover (v1.8.0)

**High Availability for Critical Systems**

- 🎯 **RSSI-Based Election** - Best signal strength wins bridge role
- 🔍 **Automatic Detection** - 60-second failure detection via heartbeats
- ⚡ **Fast Failover** - 60-70 second typical recovery time
- 🌐 **Distributed Consensus** - No single coordinator, deterministic winner selection
- 🛡️ **Split-Brain Prevention** - State machine prevents concurrent elections
- 📊 **Tiebreaker Rules** - RSSI → Uptime → Memory → Node ID

**Use Cases:**
- Fish farm alarm systems requiring 24/7 Internet connectivity
- Industrial IoT networks with critical sensor monitoring
- Smart building systems needing continuous cloud connectivity

**Example:**
```cpp
// Enable automatic bridge failover
mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
mesh.enableBridgeFailover(true);
mesh.onBridgeRoleChanged(&bridgeRoleCallback);

void bridgeRoleCallback(bool isBridge, String reason) {
  if (isBridge) {
    Serial.printf("🎯 Promoted to bridge: %s\n", reason.c_str());
  }
}
```

See [Bridge Failover Guide](docs/BRIDGE_FAILOVER.md) and [bridge_failover example](examples/bridge_failover/) for complete documentation.

#### 🌉 Multi-Bridge Coordination (v1.8.2)

**Enterprise Load Balancing and Geographic Redundancy**

- 🏢 **Multiple Simultaneous Bridges** - Run 2+ bridges for load distribution
- ⚖️ **Smart Load Balancing** - Three strategies: Priority-Based, Round-Robin, Best Signal
- 🎯 **Priority System** - 10-level priority (10=primary, 1=standby)
- 🔄 **Hot Standby** - Zero-downtime redundancy without failover delays
- 🌍 **Geographic Distribution** - Bridges in different locations for large areas
- 📊 **Traffic Shaping** - Route different data types through different bridges
- 🤝 **Automatic Coordination** - Bridges discover and coordinate automatically

**Use Cases:**
- Large warehouses/factories with multiple Internet connections
- Geographic distribution across multiple buildings
- Traffic shaping (sensors → Bridge A, commands → Bridge B)
- Load balancing for high-traffic deployments

**Example:**
```cpp
// Primary bridge (priority 10)
mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                  ROUTER_SSID, ROUTER_PASSWORD,
                  &userScheduler, MESH_PORT, 10);

// Configure load balancing strategy
mesh.setBridgeSelectionStrategy(ROUND_ROBIN);

// Monitor bridge coordination
mesh.onBridgeCoordination(&bridgeCoordinationCallback);
```

See [Multi-Bridge Implementation](docs/implementation/MULTI_BRIDGE_IMPLEMENTATION.md), [Issue #65 Verification](docs/internal/ISSUE_65_VERIFICATION.md), and [examples/multi_bridge/](examples/multi_bridge/) for complete documentation.

#### 📬 Message Queue for Offline Mode (v1.8.2)

**Zero Data Loss During Internet Outages**

- 🛡️ **Priority-Based Queuing** - CRITICAL, HIGH, NORMAL, LOW priorities
- 💾 **Smart Eviction** - CRITICAL messages never dropped, oldest LOW messages dropped first
- 📡 **Automatic Online/Offline Detection** - Integrates with bridge status monitoring
- 🔄 **Auto-Flush When Online** - Queued messages sent automatically when Internet restored
- ⚙️ **Configurable** - Queue size, priorities, callbacks
- 📊 **Queue Statistics** - Monitor queue usage, drops, flushes
- 🎯 **Production Ready** - Battle-tested for critical sensor data

**Use Cases:**
- Fish farms with critical O2 alarms (original Issue #66 use case)
- Industrial sensors that cannot lose data during outages
- Medical monitoring systems requiring guaranteed delivery
- Any system where data loss is unacceptable

**Example:**
```cpp
// Enable message queue with max 100 messages
mesh.enableMessageQueue(true);
mesh.setMaxQueueSize(100);

// Queue critical alarm message
String criticalAlarm = "{\"sensor\":\"O2\",\"value\":2.5,\"alarm\":true}";
mesh.queueMessage(criticalAlarm, CRITICAL);

// Set callbacks
mesh.onQueueFull(&queueFullCallback);
mesh.onQueueFlushed(&queueFlushedCallback);
```

See [Message Queue Implementation](docs/implementation/MESSAGE_QUEUE_IMPLEMENTATION.md), [Issue #66 Closure](docs/internal/ISSUE_66_CLOSURE.md), and [examples/queued_alarms/](examples/queued_alarms/) for complete documentation.

#### 🌐 Shared Gateway Mode (v1.9.0+)

**All Nodes as Internet Gateways with Automatic Failover**

- 🌍 **Universal Internet Access** - All nodes connect to the same router and can send to Internet
- 🔄 **Instant Failover** - Near-instant relay through mesh when local Internet fails
- ⚡ **No Single Point of Failure** - Any node can serve as gateway, eliminating bridge dependency
- 🎯 **Automatic Gateway Election** - RSSI-based election determines primary gateway
- 🛡️ **Duplicate Prevention** - Built-in deduplication prevents multiple deliveries
- 📊 **Delivery Confirmation** - Acknowledgment system confirms Internet delivery

**Use Cases:**
- Fish farm monitoring where any node can send critical O₂ alarms
- Industrial sensor networks with redundant Internet paths
- Smart building systems with resilient cloud connectivity
- Remote monitoring stations with shared WiFi infrastructure

**Example:**
```cpp
// Initialize all nodes as shared gateways
bool success = mesh.initAsSharedGateway(
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

// Monitor gateway changes
mesh.onGatewayChanged([](uint32_t newGateway) {
    Serial.printf("Primary gateway: %u\n", newGateway);
});
```

See [Shared Gateway API Reference](docs/api/shared-gateway.md), [Shared Gateway Design](docs/design/SHARED_GATEWAY_DESIGN.md), and [examples/sharedGateway/](examples/sharedGateway/) for complete documentation.

#### MQTT Bridge Commands

The MQTT bridge enables bidirectional communication between MQTT brokers and mesh networks:

- **Device Control** - Send commands from web applications to mesh nodes via MQTT
- **Configuration Management** - Query and update node configurations remotely
- **Status Monitoring** - Receive real-time status updates and sensor data via MQTT
- **Event Notifications** - Track node connections, disconnections, and mesh topology changes

**Documentation:**

- 📖 [MQTT Bridge Commands Reference](docs/MQTT_BRIDGE_COMMANDS.md) - Complete command API documentation
- 🔧 [OTA Commands Reference](docs/OTA_COMMANDS_REFERENCE.md) - Over-the-air firmware updates

**Examples:**

- 🌉 [MQTT Command Bridge](examples/mqttCommandBridge/mqttCommandBridge.ino) - Gateway bridge with bidirectional MQTT-mesh routing
- 📡 [Alteriom Sensor Node](examples/alteriom/alteriom.ino) - Example node using SensorPackage, CommandPackage, and StatusPackage

### 🌐 Core Features

The library handles routing and network management automatically, so you can focus on your application. It uses JSON-based messaging and syncs time across all nodes, making it ideal for coordinated behaviour like synchronized light displays or sensor networks reporting to a central node. The original version was forked from [easymesh](https://github.com/Coopdis/easyMesh).

### True ad-hoc networking

painlessMesh is a true ad-hoc network, meaning that no-planning, central controller, or router is required. Any system of 1 or more nodes will self-organize into fully functional mesh. The maximum size of the mesh is limited (we think) by the amount of memory in the heap that can be allocated to the sub-connections buffer and so should be really quite high.

### JSON based

painlessMesh uses JSON objects for all its messaging. There are a couple of reasons for this. First, it makes the code and the messages human readable and painless to understand and second, it makes it painless to integrate painlessMesh with javascript front-ends, web applications, and other apps. Some performance is lost, but I haven’t been running into performance issues yet. Converting to binary messaging would be fairly straight forward if someone wants to contribute.

### Wifi &amp; Networking

painlessMesh is designed to be used with Arduino, but it does not use the Arduino WiFi libraries, as we were running into performance issues (primarily latency) with them. Rather the networking is all done using the native esp32 and esp8266 SDK libraries, which are available through the Arduino IDE. Hopefully though, which networking libraries are used won’t matter to most users much as you can just include painlessMesh.h, run the init() and then work the library through the API.

### painlessMesh is not IP networking

painlessMesh does not create a TCP/IP network of nodes. Rather each of the nodes is uniquely identified by its 32bit chipId which is retrieved from the esp8266/esp32 using the `system_get_chip_id()` call in the SDK. Every node will have a unique number. Messages can either be broadcast to all the nodes on the mesh, or sent specifically to an individual node which is identified by its `nodeId.

### Limitations and caveats

- Try to avoid using `delay()` in your code. To maintain the mesh we need to perform some tasks in the background. Using `delay()` will stop these tasks from happening and can cause the mesh to lose stability/fall apart. Instead, we recommend using [TaskScheduler](http://playground.arduino.cc/Code/TaskScheduler) which is used in `painlessMesh` itself. Documentation can be found [here](https://github.com/arkhipenko/TaskScheduler/wiki/Full-Document). For other examples on how to use the scheduler see the example folder.
- `painlessMesh` subscribes to WiFi events. Please be aware that as a result `painlessMesh` can be incompatible with user programs/other libraries that try to bind to the same events.
- Try to be conservative in the number of messages (and especially broadcast messages) you sent per minute. This is to prevent the hardware from overloading. Both esp8266 and esp32 are limited in processing power/memory, making it easy to overload the mesh and destabilize it. And while `painlessMesh` tries to prevent this from happening, it is not always possible to do so.
- Messages can go missing or be dropped due to high traffic and you can not rely on all messages to be delivered. One suggestion to work around is to resend messages every so often. Even if some go missing, most should go through. Another option is to have your nodes send replies when they receive a message. The sending nodes can the resend the message if they haven’t gotten a reply in a certain amount of time.

## Installation

### Arduino Library Manager

> **✅ FIXED**: Library name restored to match Arduino Library Manager registry.  
> **Status**: New releases (v1.8.3+) will now be indexed automatically within 24-48 hours.  
> **Issue**: Versions v1.7.0-v1.8.1 had incorrect library name and were not indexed.  
> See [Arduino Library Manager Documentation](docs/ARDUINO_LIBRARY_MANAGER_SUBMISSION.md) for details.

**Once registered**, installation will be available via Arduino IDE:

1. Open Arduino IDE
2. Go to **Tools** → **Manage Libraries...**
3. Search for **"AlteriomPainlessMesh"**
4. Click **Install**

The library includes the header file `AlteriomPainlessMesh.h` which provides access to both the core painlessMesh functionality and Alteriom-specific extensions.

#### Manual Installation (Current Method)

**Option 1: Download ZIP from GitHub Release**

1. Go to [Releases](https://github.com/Alteriom/painlessMesh/releases/latest)
2. Download `painlessMesh-v1.8.3.zip` (or latest version)
3. In Arduino IDE: **Sketch** → **Include Library** → **Add .ZIP Library...**
4. Select the downloaded ZIP file
5. Restart Arduino IDE

**Option 2: Create ZIP from Repository**

If you need to create a ZIP file from source (e.g., for testing unreleased versions):

```bash
# Clone and create ZIP
git clone https://github.com/Alteriom/painlessMesh.git
cd painlessMesh
./scripts/create-arduino-zip.sh

# Output: dist/painlessMesh-vX.X.X.zip
# Import this ZIP in Arduino IDE
```

**Option 3: Git Clone**

```bash
cd ~/Arduino/libraries/
git clone https://github.com/Alteriom/painlessMesh.git AlteriomPainlessMesh
# Restart Arduino IDE
```

📖 **Detailed Guide**: See [Manual Arduino IDE Installation](docs/getting-started/arduino-manual-install.md) for complete instructions, troubleshooting, and ZIP creation details.

### PlatformIO

`painlessMesh` is included in both the Arduino Library Manager and the platformio library registry and can easily be installed via either of those methods.

### Dependencies

painlessMesh makes use of the following libraries, which can be installed through the Arduino Library Manager

- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [TaskScheduler](https://github.com/arkhipenko/TaskScheduler)
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) (ESP8266)
- [AsyncTCP](https://github.com/ESP32Async/AsyncTCP) (ESP32) - v3.3.0+ required for ESP32-C6

If platformio is used to install the library, then the dependencies will be installed automatically.

> **⚠️ ESP32-C6 Users:** If you experience crashes with `tcp_alloc` errors, you need AsyncTCP v3.3.0+ with LWIP locking support. Use the [ESP32Async/AsyncTCP](https://github.com/ESP32Async/AsyncTCP) library or see the [ESP32-C6 Compatibility Guide](docs/troubleshooting/ESP32_C6_COMPATIBILITY.md) for details.

## Quick Start with Alteriom Packages

### Basic Sensor Node

```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_password"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
    Serial.begin(115200);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
}

void loop() {
    mesh.update();
    
    // Create and send sensor data
    SensorPackage sensor;
    sensor.temperature = 25.5;
    sensor.humidity = 60.0;
    sensor.sensorId = mesh.getNodeId();
    sensor.timestamp = mesh.getNodeTime();
    
    mesh.sendBroadcast(sensor.toJsonString());
    delay(30000); // Send every 30 seconds
}

void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    
    if (doc["type"] == 200) { // SensorPackage
        SensorPackage sensor(doc.as<JsonObject>());
        Serial.printf("Sensor %u: %.1f°C, %.1f%% RH\n", 
                     sensor.sensorId, sensor.temperature, sensor.humidity);
    }
}
```

### Bridge to Internet (Auto Channel Detection)

The new bridge-centric architecture makes it easy to connect your mesh to the Internet via a router. The bridge node automatically detects the router's channel and configures the mesh accordingly.

#### Bridge Node

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
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Single call does everything:
  // 1. Connects to router and detects its channel
  // 2. Initializes mesh on detected channel
  // 3. Sets node as root/bridge
  mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                    ROUTER_SSID, ROUTER_PASSWORD,
                    &userScheduler, 5555);
  
  mesh.onReceive(&receivedCallback);
}

void loop() { mesh.update(); }

void receivedCallback(uint32_t from, String& msg) {
  // Forward mesh data to Internet services (MQTT, HTTP, etc.)
}
```

#### Regular Nodes (Auto Channel Detection)

```cpp
void setup() {
  Serial.begin(115200);
  
  // channel=0 means auto-detect the mesh channel
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, 5555, 
            WIFI_AP_STA, 0);
  
  mesh.onReceive(&receivedCallback);
}
```

See [BRIDGE_TO_INTERNET.md](BRIDGE_TO_INTERNET.md) for complete documentation.

### Message Types

painlessMesh uses two categories of message types:

#### Protocol-Level Types (Internal Mesh Operations)

These types are used internally by painlessMesh for mesh management and are handled automatically:

| Type | Name | Purpose |
|------|------|---------|
| 3 | `TIME_DELAY` | Network latency measurement between nodes |
| 4 | `TIME_SYNC` | Time synchronization protocol messages |
| 5 | `NODE_SYNC_REQUEST` | Node discovery and topology requests |
| 6 | `NODE_SYNC_REPLY` | Node discovery and topology responses |
| 7 | `CONTROL` | Deprecated control messages |
| 8 | `BROADCAST` | Internal broadcast routing indicator |
| 9 | `SINGLE` | Internal single-node routing indicator |

**Note**: These protocol types are managed automatically by painlessMesh and are not typically used in application code.

#### Application-Level Package Types

These are the message types used by applications built on painlessMesh:

| Type | Class | Purpose | Fields |
|------|-------|---------|--------|
| 200 | `SensorPackage` | Environmental data | `temperature`, `humidity`, `pressure`, `sensorId`, `timestamp`, `batteryLevel` |
| 202 | `StatusPackage` | Health monitoring | `deviceStatus`, `uptime`, `freeMemory`, `wifiStrength`, `firmwareVersion` |
| 204 | `MetricsPackage` | Sensor metrics (v1.7.7+, aligns with schema v0.7.2+) | `cpuUsage`, `freeHeap`, `bytesReceived`, `currentThroughput`, `connectionQuality`, `wifiRSSI` |
| 400 | `CommandPackage` | Device control (v1.7.7+, moved from 201) | `command`, `targetDevice`, `parameters`, `commandId` |
| 600 | `MeshNodeListPackage` | Mesh node list (v1.7.7+, MESH_NODE_LIST) | `nodes[]` (nodeId, status, lastSeen, signalStrength), `nodeCount`, `meshId` |
| 601 | `MeshTopologyPackage` | Mesh topology (v1.7.7+, MESH_TOPOLOGY) | `connections[]` (fromNode, toNode, linkQuality, latencyMs), `rootNode` |
| 602 | `MeshAlertPackage` | Mesh alerts (v1.7.7+, MESH_ALERT) | `alerts[]` (alertType, severity, message, nodeId), `alertCount` |
| 603 | `MeshBridgePackage` | Mesh bridge (v1.7.7+, MESH_BRIDGE) | `meshProtocol`, `fromNodeId`, `toNodeId`, `meshType`, `rawPayload`, `rssi`, `hopCount` |
| 604 | `EnhancedStatusPackage` | Mesh status (MESH_STATUS per schema v0.7.2+) | `nodeCount`, `connectionCount`, `messagesReceived`, `messagesSent`, `avgLatency`, `packetLossRate` |
| 605 | `HealthCheckPackage` | Mesh metrics (v1.7.7+, MESH_METRICS per schema v0.7.2+) | `healthStatus`, `problemFlags`, `memoryHealth`, `networkHealth`, `performanceHealth`, `recommendations` |
| 610 | `BridgeStatusPackage` | Bridge health monitoring (v1.8.0+, BRIDGE_STATUS per schema v0.7.3+) | `internetConnected`, `routerRSSI`, `routerChannel`, `uptime`, `gatewayIP`, `timestamp` |
| 611 | `BridgeElectionPackage` | Bridge failover election (v1.8.0+, BRIDGE_ELECTION per schema v0.7.3+) | `routerRSSI`, `uptime`, `freeMemory`, `timestamp`, `routerSSID` |
| 612 | `BridgeTakeoverPackage` | Bridge role announcement (v1.8.0+, BRIDGE_TAKEOVER per schema v0.7.3+) | `previousBridge`, `reason`, `timestamp` |
| 613 | `BridgeCoordinationPackage` | Multi-bridge coordination (v1.8.2+, BRIDGE_COORDINATION) | `priority`, `role`, `peerBridges[]`, `load`, `timestamp` |
| 614 | `NTPTimeSyncPackage` | NTP time synchronization (v1.8.0+, TIME_SYNC_NTP per schema v0.7.3+) | `ntpTime`, `accuracy`, `source`, `timestamp` |

## Key Features

### Core Features

- **🔄 Automatic Mesh Formation** - Nodes discover and connect automatically
- **📡 Self-Healing Network** - Adapts when nodes join/leave
- **⏰ Time Synchronization** - Coordinated actions across all nodes  
- **🔀 Smart Routing** - Broadcast, point-to-point, and neighbor messaging
- **🔌 Plugin System** - Type-safe custom message packages
- **📱 ESP32 & ESP8266** - Full support for both platforms
- **🛡️ Memory Efficient** - Optimized for resource-constrained devices

### Advanced Features (v1.7.0+)

- **📡 Broadcast OTA** - Efficient firmware distribution for large meshes (50-100+ nodes)
- **🌉 MQTT Bridge** - Professional monitoring with Grafana/InfluxDB/Prometheus
- **📊 Topology Visualization** - D3.js, Cytoscape.js, Node-RED examples
- **🎯 Production Ready** - Enterprise-grade stability and performance

## Examples & Use Cases

- **IoT Sensor Networks** - Environmental monitoring, smart agriculture
- **Home Automation** - Distributed lighting, HVAC control
- **Industrial Monitoring** - Equipment status, predictive maintenance  
- **Event Coordination** - Synchronized displays, distributed processing
- **Bridge Networks** - Connect mesh to WiFi/Internet/MQTT - [📖 Bridge Guide](BRIDGE_TO_INTERNET.md)

## Latest Release: v1.9.0 (November 30, 2025)

**Major Update: Improved Bridge Detection, Consolidated Examples & Documentation**

- 🔍 **Mesh Connectivity Detection** - New `hasActiveMeshConnections()` and `getLastKnownBridge()` APIs
- 🌉 **Improved Bridge Detection** - `getPrimaryBridge()` returns last known bridge when disconnected
- ⚡ **Election Guard** - Skip election trigger when node is disconnected from mesh
- 📦 **Consolidated Examples** - Reduced from 32 to 14 essential examples
- 📚 **Cleaned Documentation** - Removed obsolete docs, kept only essentials
- ⚙️ **Configurable Election Timing** - Prevent split-brain with `setElectionStartupDelay()` and `setElectionRandomDelay()`

**[📋 Full CHANGELOG](CHANGELOG.md)**

## Getting Help

- **[FAQ](docs/troubleshooting/faq.md)** - Common questions and solutions
- **[Common Issues](docs/troubleshooting/common-issues.md)** - Troubleshooting guide
- **[GitHub Issues](https://github.com/Alteriom/painlessMesh/issues)** - Bug reports and feature requests  
- **[Community Forum](https://groups.google.com/forum/#!forum/painlessmesh-user)** - Community support
- **[API Documentation](http://painlessmesh.gitlab.io/painlessMesh/index.html)** - Generated API docs

## Development

### Building from Source

```bash
git clone https://github.com/Alteriom/painlessMesh.git
cd painlessMesh
git submodule update --init
cmake -G Ninja .
ninja
run-parts --regex catch_ bin/  # Run tests
```

### Requirements

- **ESP32/ESP8266**: Arduino Core 2.0.0+
- **Dependencies**: ArduinoJson 6.x, TaskScheduler 3.x  
- **Development**: CMake, Ninja, Boost (for desktop testing)

### CI/CD Pipeline

painlessMesh features a state-of-the-art automated CI/CD pipeline:

**🔄 Continuous Integration:**

- Automated builds on gcc/clang with strict warnings
- Cross-platform testing (Arduino CLI, PlatformIO)
- Code quality and formatting validation
- Comprehensive test suite execution

**🚀 Automated Releases:**

- Semantic versioning with automated tagging
- GitHub Releases with changelog generation
- Library package distribution
- Documentation deployment to GitHub Pages
- Arduino Library Manager & PlatformIO Registry integration

**📋 Release Management:**

```bash
# Bump version and prepare release
./scripts/bump-version.sh patch  # or minor, major
./scripts/validate-release.sh    # Validate release readiness

# Edit CHANGELOG.md, then commit with release prefix
git commit -am "release: v1.5.7"
git push origin main  # Triggers automated release
```

See [RELEASE_GUIDE.md](RELEASE_GUIDE.md) for complete release documentation.

## Contributing

We try to follow the [git flow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow) development model. Which means that we have a `develop` branch and `master` branch. All development is done under feature branches, which are (when finished) merged into the development branch. When a new version is released we merge the `develop` branch into the `master` branch. For more details see the [CONTRIBUTING](https://gitlab.com/painlessMesh/painlessMesh/blob/master/CONTRIBUTING.md) file.

## Funding

If you like the library please consider supporting its development. Your contributions help me spend more time improving painlessMesh.

[![PayPal Donation](paypal/qrcode.png)](https://www.paypal.com/paypalme/domlavoie)

**[Donate via PayPal](https://www.paypal.com/paypalme/domlavoie)** • [dominic.lavoie@gmail.com](mailto:dominic.lavoie@gmail.com)

## 📚 Complete Documentation

> **� [Visit the Full Documentation Website](https://alteriom.github.io/painlessMesh/)**

### 🚀 Getting Started

**New to AlteriomPainlessMesh?** Start with these essential guides:

| Guide | Description | Link |
|-------|-------------|------|
| **🎯 Quick Start** | Get your first mesh running in 5 minutes | [📖 Start Here](https://alteriom.github.io/painlessMesh/#/getting-started/quickstart) |
| **💾 Installation** | Arduino IDE, PlatformIO, and more | [📖 Install Guide](https://alteriom.github.io/painlessMesh/#/getting-started/installation) |
| **🌐 First Mesh** | Build a real multi-node network | [📖 Build Now](https://alteriom.github.io/painlessMesh/#/getting-started/first-mesh) |

### 📖 API Documentation

**Complete reference for all classes, functions, and features:**

| Section | Description | Link |
|---------|-------------|------|
| **🔧 Core API** | painlessMesh class reference and methods | [📖 Core API](https://alteriom.github.io/painlessMesh/#/api/core-api) |
| **📦 Doxygen API** | Auto-generated complete API documentation | [📖 Browse API](https://alteriom.github.io/painlessMesh/#/api/doxygen) |
| **⚙️ Configuration** | All mesh configuration options | [📖 Configure](https://alteriom.github.io/painlessMesh/#/api/configuration) |
| **🔄 Callbacks** | Event handling and callback patterns | [📖 Events](https://alteriom.github.io/painlessMesh/#/api/callbacks) |

### 🎯 Alteriom Extensions

**IoT-ready packages for production applications:**

| Package | Purpose | Documentation |
|---------|---------|---------------|
| **📊 SensorPackage** | Environmental data collection | [📖 Sensor Docs](https://alteriom.github.io/painlessMesh/#/alteriom/overview) |
| **⚡ CommandPackage** | Device control and automation | [📖 Command Docs](https://alteriom.github.io/painlessMesh/#/alteriom/overview) |
| **📈 StatusPackage** | Health monitoring and diagnostics | [📖 Status Docs](https://alteriom.github.io/painlessMesh/#/alteriom/overview) |

### 🏗️ Advanced Topics

**Deep dive into architecture and advanced usage:**

| Topic | Description | Link |
|-------|-------------|------|
| **🌳 Architecture** | How painlessMesh works internally | [📖 Architecture](https://alteriom.github.io/painlessMesh/#/architecture/mesh-architecture) |
| **🌉 Bridge to Internet** | Connect mesh to WiFi/Internet/MQTT | [📖 Bridge Guide](BRIDGE_TO_INTERNET.md) |
| **🔌 Plugin System** | Create custom message packages | [📖 Plugins](https://alteriom.github.io/painlessMesh/#/architecture/plugin-system) |
| **🎓 Tutorials** | Step-by-step examples and patterns | [📖 Tutorials](https://alteriom.github.io/painlessMesh/#/tutorials/basic-examples) |
| **🛠️ Troubleshooting** | Common issues and solutions | [📖 Help](https://alteriom.github.io/painlessMesh/#/troubleshooting/common-issues) |

### 📝 Quick Reference

**Bookmark these for daily development:**

- **[📋 Class Index](https://alteriom.github.io/painlessMesh/#/api/doxygen/classes)** - All classes with methods
- **[⚙️ Function Index](https://alteriom.github.io/painlessMesh/#/api/doxygen/functions)** - All functions and globals  
- **[📁 File Structure](https://alteriom.github.io/painlessMesh/#/api/doxygen/files)** - Source code organization
- **[❓ FAQ](https://alteriom.github.io/painlessMesh/#/troubleshooting/faq)** - Frequently asked questions

**📖 Tutorials & Examples:**

- **[Basic Examples](docs/tutorials/basic-examples.md)** - Essential patterns and techniques
- **[Custom Packages](docs/tutorials/custom-packages.md)** - Type-safe message handling
- **[Sensor Networks](docs/tutorials/sensor-networks.md)** - IoT sensor network patterns

**🚀 Alteriom Extensions:**

- **[Alteriom Overview](docs/alteriom/overview.md)** - Production-ready IoT packages
- **[Sensor Packages](docs/alteriom/sensor-packages.md)** - Environmental monitoring
- **[Command System](docs/alteriom/command-system.md)** - Device control and automation

**🔧 Troubleshooting:**

- **[Common Issues](docs/troubleshooting/common-issues.md)** - Solutions to frequent problems
- **[FAQ](docs/troubleshooting/faq.md)** - Frequently asked questions
- **[Debugging Guide](docs/troubleshooting/debugging.md)** - Tools and techniques

**📋 Complete Documentation Index:** [docs/README.md](docs/README.md)

## painlessMesh API Summary

Here's a quick API overview. **For complete documentation, see [Core API Reference](docs/api/core-api.md)**

```cpp
#include "painlessMesh.h"

painlessMesh mesh;
```

### Member Functions

#### void painlessMesh::init(String ssid, String password, uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA, _auth_mode authmode = AUTH_WPA2_PSK, uint8_t channel = 1, phy_mode_t phymode = PHY_MODE_11G, uint8_t maxtpw = 82, uint8_t hidden = 0, uint8_t maxconn = 4)

Add this to your setup() function.
Initialize the mesh network. This routine does the following things.

- Starts a wifi network
- Begins searching for other wifi networks that are part of the mesh
- Logs on to the best mesh network node it finds… if it doesn’t find anything, it starts a new search in 5 seconds.

`ssid` = the name of your mesh.  All nodes share same AP ssid. They are distinguished by BSSID.
`password` = wifi password to your mesh.
`port` = the TCP port that you want the mesh server to run on. Defaults to 5555 if not specified.
[`connectMode`](https://gitlab.com/painlessMesh/painlessMesh/wikis/connect-mode:-WIFI_AP,-WIFI_STA,-WIFI_AP_STA-mode) = switch between WIFI_AP, WIFI_STA and WIFI_AP_STA (default) mode

#### void painlessMesh::stop()

Stop the node. This will cause the node to disconnect from all other nodes and stop/sending messages.

#### void painlessMesh::update( void )

Add this to your loop() function
This routine runs various maintenance tasks... Not super interesting, but things don't work without it.

#### void painlessMesh::onReceive( &amp;receivedCallback )

Set a callback routine for any messages that are addressed to this node. Callback routine has the following structure.

`void receivedCallback( uint32_t from, String &amp;msg )`

Every time this node receives a message, this callback routine will the called. “from” is the id of the original sender of the message, and “msg” is a string that contains the message. The message can be anything. A JSON, some other text string, or binary data.

#### void painlessMesh::onNewConnection( &amp;newConnectionCallback )

This fires every time the local node makes a new connection. The callback has the following structure.

`void newConnectionCallback( uint32_t nodeId )`

`nodeId` is new connected node ID in the mesh.

#### void painlessMesh::onChangedConnections( &amp;changedConnectionsCallback )

This fires every time there is a change in mesh topology. Callback has the following structure.

`void onChangedConnections()`

There are no parameters passed. This is a signal only.

#### bool painlessMesh::isConnected( nodeId )

Returns if a given node is currently connected to the mesh.

`nodeId` is node ID that the request refers to.

#### void painlessMesh::onNodeTimeAdjusted( &amp;nodeTimeAdjustedCallback )

This fires every time local time is adjusted to synchronize it with mesh time. Callback has the following structure.

`void onNodeTimeAdjusted(int32_t offset)`

`offset` is the adjustment delta that has been calculated and applied to local clock.

#### void onNodeDelayReceived(nodeDelayCallback_t onDelayReceived)

This fires when a time delay measurement response is received, after a request was sent. Callback has the following structure.

`void onNodeDelayReceived(uint32_t nodeId, int32_t delay)`

`nodeId` The node that originated response.

`delay` One way network trip delay in microseconds.

#### bool painlessMesh::sendBroadcast( String &amp;msg, bool includeSelf = false)

Sends msg to every node on the entire mesh network. By default the current node is excluded from receiving the message (`includeSelf = false`). `includeSelf = true` overrides this behavior, causing the `receivedCallback` to be called when sending a broadcast message.

returns true if everything works, false if not. Prints an error message to Serial.print, if there is a failure.

#### bool painlessMesh::sendSingle(uint32_t dest, String &amp;msg)

Sends msg to the node with Id == dest.

returns true if everything works, false if not.  Prints an error message to Serial.print, if there is a failure.

#### String painlessMesh::subConnectionJson()

Returns mesh topology in JSON format.

#### std::list<uint32_t> painlessMesh::getNodeList()

Get a list of all known nodes. This includes nodes that are both directly and indirectly connected to the current node.

#### uint32_t painlessMesh::getNodeId( void )

Return the chipId of the node that we are running on.

#### uint32_t painlessMesh::getNodeTime( void )

Returns the mesh timebase microsecond counter. Rolls over 71 minutes from startup of the first node.

Nodes try to keep a common time base synchronizing to each other using [an SNTP based protocol](https://gitlab.com/painlessMesh/painlessMesh/wikis/mesh-protocol#time-sync)

#### bool painlessMesh::startDelayMeas(uint32_t nodeId)

Sends a node a packet to measure network trip delay to that node. Returns true if nodeId is connected to the mesh, false otherwise. After calling this function, user program have to wait to the response in the form of a callback specified by `void painlessMesh::onNodeDelayReceived(nodeDelayCallback_t onDelayReceived)`.

nodeDelayCallback_t is a function in the form of `void (uint32_t nodeId, int32_t delay)`.

#### void painlessMesh::stationManual( String ssid, String password, uint16_t port, uint8_t *remote_ip )

Connects the node to an AP outside the mesh. When specifying a `remote_ip` and `port`, the node opens a TCP connection after establishing the WiFi connection.

Note: The mesh must be on the same WiFi channel as the AP.

#### void painlessMesh::setDebugMsgTypes( uint16_t types )

Change the internal log level. List of types defined in Logger.hpp:
ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE

# Funding

Most development of painlessMesh has been done as a hobby, but some specific features have been funded by the companies listed below:

![Sowillo](https://www.sowillo.com/wp-content/uploads/2019/04/Logo-Sowillo-1.png)

[Sowillo](http://sowillo.com/en/)
