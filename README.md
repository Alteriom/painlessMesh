# AlteriomPainlessMesh

> **📚 [Complete Documentation](https://alteriom.github.io/painlessMesh/)** | **📖 [API Reference](https://alteriom.github.io/painlessMesh/#/api/doxygen)** | **🎯 [Examples](https://alteriom.github.io/painlessMesh/#/tutorials/basic-examples)** | **📋 [Changelog](CHANGELOG.md)**

<div align="center">

**Version 2.0.0** — delivery confirmation, a unified send path, and gateway failover, routing and channel following validated on a six-family hardware rig

[![CI/CD Pipeline](https://github.com/Alteriom/painlessMesh/actions/workflows/ci.yml/badge.svg)](https://github.com/Alteriom/painlessMesh/actions/workflows/ci.yml)
[![Documentation](https://github.com/Alteriom/painlessMesh/actions/workflows/docs.yml/badge.svg)](https://github.com/Alteriom/painlessMesh/actions/workflows/docs.yml)
[![Release](https://github.com/Alteriom/painlessMesh/actions/workflows/release.yml/badge.svg)](https://github.com/Alteriom/painlessMesh/actions/workflows/release.yml)
[![GitHub release](https://img.shields.io/github/v/release/Alteriom/painlessMesh?label=version)](https://github.com/Alteriom/painlessMesh/releases)
[![NPM Version](https://img.shields.io/npm/v/@alteriom/painlessmesh?label=npm)](https://www.npmjs.com/package/@alteriom/painlessmesh)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/alteriom/library/AlteriomPainlessMesh.svg)](https://registry.platformio.org/libraries/alteriom/AlteriomPainlessMesh)

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

**Advanced Monitoring Packages:**

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

**Mesh Topology & Management:**

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

- **`EnhancedStatusPackage`** (Type 604) - Detailed mesh status (MESH_STATUS per mqtt-schema v0.7.2+)
  - Complete mesh statistics (node count, connections, messages)
  - Performance metrics (latency, packet loss, throughput)
  - Alert flags and error reporting
  - Firmware verification with MD5 hash

**Bridge Failover & High Availability:**

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

- **`BridgeCoordinationPackage`** (Type 613) - Multi-bridge coordination (BRIDGE_COORDINATION per mqtt-schema v0.7.3+)
  - Bridge priority levels (1-10)
  - Role assignment (primary/secondary/standby)
  - Peer bridge discovery
  - Load balancing metrics
  - Hot standby redundancy

- **`NTPTimeSyncPackage`** (Type 614) - NTP time synchronization (TIME_SYNC_NTP per mqtt-schema v0.7.3+)
  - Unix timestamp from NTP server
  - Accuracy/precision in milliseconds
  - NTP server source identification
  - Mesh-wide time distribution from bridge

All packages provide type-safe serialization, automatic JSON conversion, and mesh-wide broadcasting or targeted messaging. They align with mqtt-schema v0.7.3+ for enterprise IoT integration.

#### 🚀 Advanced Features

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

See [BRIDGE_TO_INTERNET.md](BRIDGE_TO_INTERNET.md) for complete documentation.

#### 🔄 Automatic Bridge Failover

**High Availability for Critical Systems**

- 🎯 **RSSI-Based Election** - Best router signal wins the bridge role; ties go to uptime, then free memory, then node ID
- 📣 **A bridge that stops cleanly says so** - `mesh.stop()` on a bridge announces it is leaving, and the candidates hold their election within seconds
- 🔍 **Heartbeat Detection** - A bridge that loses power is noticed when its last status ages out: the bridge timeout (60 s) plus up to one 30 s monitor tick
- 🏠 **The mesh follows the bridge** - Peers learn the elected bridge's channel from the takeover message and treat it as home; a node that loses its uplink there rescans that channel first
- 🌐 **Distributed Consensus** - No single coordinator; every node evaluates the same candidates by the same rules
- 🛡️ **Split-Brain Prevention** - One election at a time, a 60 s hold between role changes, and a minimum router RSSI for a lone candidate
- 🔬 **Validated on hardware** - The failover scenario runs on the Alteriom rig with a real router; see [CHANGELOG.md](CHANGELOG.md)

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

void bridgeRoleCallback(bool isBridge, const String& reason) {
  if (isBridge) {
    Serial.printf("🎯 Promoted to bridge: %s\n", reason.c_str());
  }
}
```

See [bridge_failover example](examples/bridge_failover/) for complete documentation.

#### 🌉 Multi-Bridge Coordination

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

// Enable bridge failover for high availability
mesh.enableBridgeFailover(true);

// Monitor bridge coordination (fires every ~30s per bridge)
mesh.onBridgeCoordination(
  [](const plugin::BridgeCoordinationPackage& pkg, uint32_t fromNode) {
    Serial.printf("Bridge %u: priority=%d, load=%d%%\n",
                  fromNode, pkg.priority, pkg.load);
  }
);

// Get notified when bridge state changes
mesh.onBridgeCoordinationChanged(
  [](const plugin::BridgeCoordinationPackage& pkg, uint32_t fromNode,
     TSTRING changeType) {
    Serial.printf("Bridge %s: %u (role=%s)\n",
                  changeType.c_str(), fromNode, pkg.role.c_str());
  }
);
```

See [BRIDGE_TO_INTERNET.md](BRIDGE_TO_INTERNET.md) for multi-bridge documentation.

#### 📬 Message Queue for Offline Mode

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
mesh.enableMessageQueue(true, 100);

// Queue a critical alarm for the Internet endpoint; it is sent when a
// gateway with Internet is available and never evicted by lower priorities
String criticalAlarm = "{\"sensor\":\"O2\",\"value\":2.5,\"alarm\":true}";
mesh.queueMessage(criticalAlarm, "https://api.example.com/alarm", PRIORITY_CRITICAL);
```

See [BRIDGE_TO_INTERNET.md](BRIDGE_TO_INTERNET.md) for message queue documentation.

#### 🌐 Shared Gateway Mode

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
mesh.onGatewayChanged([](uint32_t oldGateway, uint32_t newGateway) {
    Serial.printf("Primary gateway: %u -> %u\n", oldGateway, newGateway);
});
```

See [sharedGateway example](examples/sharedGateway/) for complete documentation.

#### MQTT Bridge Commands

The MQTT bridge enables bidirectional communication between MQTT brokers and mesh networks:

- **Device Control** - Send commands from web applications to mesh nodes via MQTT
- **Configuration Management** - Query and update node configurations remotely
- **Status Monitoring** - Receive real-time status updates and sensor data via MQTT
- **Event Notifications** - Track node connections, disconnections, and mesh topology changes

**Documentation:**

See [mqttBridge example](examples/mqttBridge/) for MQTT integration.

**Examples:**

- 🌉 [MQTT Bridge](examples/mqttBridge/mqttBridge.ino) - Gateway bridge with bidirectional MQTT-mesh routing
- 📡 [Alteriom Sensor Node](examples/alteriom/alteriom.ino) - Example node using SensorPackage, CommandPackage, and StatusPackage

### 🌐 Core Features

The library handles routing and network management automatically, so you can focus on your application. It uses JSON-based messaging and syncs time across all nodes, making it ideal for coordinated behaviour like synchronized light displays or sensor networks reporting to a central node. The original version was forked from [easymesh](https://github.com/Coopdis/easyMesh).

### True ad-hoc networking

painlessMesh is a true ad-hoc network, meaning that no-planning, central controller, or router is required. Any system of 1 or more nodes will self-organize into fully functional mesh. The maximum size of the mesh is limited (we think) by the amount of memory in the heap that can be allocated to the sub-connections buffer and so should be really quite high.

### ESP8266 capacity

The ESP8266 is specified for **small meshes**, or as a **leaf** in larger ones.
Measured as an interior node of a seven-node mesh it runs at 10–13 KB free —
a working set that tracks its live connections and the traffic through them,
not a leak — and at that level a single 8 KB package or one OTA part can fail
to allocate. That is the part's limit, not a library defect. Every ESP32
family holds within a few percent of its starting heap in the same mesh.

Configure an ESP8266 as a leaf with `init(..., maxconn = 0)` (or `1` to allow
one child). The library checks every thirty seconds on ESP8266 and logs an
`ERROR` when the node is below 12 KB free with more than one child attached;
`mesh.overCapacity()` reports the same condition to the sketch, and
`mesh.apChildren()` says how many are attached.

### Validated on hardware

Every 2.0 change to failover, routing, channel following and the station
scan was found in the serial logs of the Alteriom hardware-in-the-loop rig —
an ESP32, ESP32-C3, ESP32-C5, ESP32-C6, ESP32-S3 and ESP8266 in one mesh with
a real router — and confirmed there. The release gate is three consecutive
clean runs of the rig's whole suite (mesh formation, delivery and
acknowledgement, priorities, dedicated and shared gateways, Internet relay,
gateway failover, mesh OTA, sustained soak). The rig and its results live in
[alteriom-esp32-farm](https://github.com/Alteriom/alteriom-esp32-farm).

### JSON based

painlessMesh uses JSON objects for all its messaging. There are a couple of reasons for this. First, it makes the code and the messages human readable and painless to understand and second, it makes it painless to integrate painlessMesh with javascript front-ends, web applications, and other apps. Some performance is lost, but I haven’t been running into performance issues yet. Converting to binary messaging would be fairly straight forward if someone wants to contribute.

### Wifi &amp; Networking

painlessMesh is designed to be used with Arduino, but it does not use the Arduino WiFi libraries, as we were running into performance issues (primarily latency) with them. Rather the networking is all done using the native esp32 and esp8266 SDK libraries, which are available through the Arduino IDE. Hopefully though, which networking libraries are used won’t matter to most users much as you can just include painlessMesh.h, run the init() and then work the library through the API.

### painlessMesh is not IP networking

painlessMesh does not create a TCP/IP network of nodes. Rather each of the nodes is uniquely identified by its 32bit chipId which is retrieved from the esp8266/esp32 using the `system_get_chip_id()` call in the SDK. Every node will have a unique number. Messages can either be broadcast to all the nodes on the mesh, or sent specifically to an individual node which is identified by its `nodeId`.

### Limitations and caveats

- Try to avoid using `delay()` in your code. To maintain the mesh we need to perform some tasks in the background. Using `delay()` will stop these tasks from happening and can cause the mesh to lose stability/fall apart. Instead, we recommend using [TaskScheduler](http://playground.arduino.cc/Code/TaskScheduler) which is used in `painlessMesh` itself. Documentation can be found [here](https://github.com/arkhipenko/TaskScheduler/wiki/Full-Document). For other examples on how to use the scheduler see the example folder.
- `painlessMesh` subscribes to WiFi events. Please be aware that as a result `painlessMesh` can be incompatible with user programs/other libraries that try to bind to the same events.
- Try to be conservative in the number of messages (and especially broadcast messages) you sent per minute. This is to prevent the hardware from overloading. Both esp8266 and esp32 are limited in processing power/memory, making it easy to overload the mesh and destabilize it. And while `painlessMesh` tries to prevent this from happening, it is not always possible to do so.
- Messages can go missing or be dropped due to high traffic and you can not rely on all messages to be delivered. One suggestion to work around is to resend messages every so often. Even if some go missing, most should go through. Another option is to have your nodes send replies when they receive a message. The sending nodes can then resend the message if they haven’t gotten a reply in a certain amount of time.

## Installation

### Arduino Library Manager

The library is in the Arduino Library Manager as **Alteriom PainlessMesh**:

1. Open Arduino IDE
2. Go to **Tools** → **Manage Libraries...**
3. Search for **"Alteriom PainlessMesh"** and click **Install**; accept the dependencies it offers (ArduinoJson, TaskScheduler, PubSubClient)
4. Also install **AsyncTCP** (ESP32) or **ESPAsyncTCP** (ESP8266) from the same dialog; the Library Manager cannot pick one per platform, so they are not listed as dependencies

The index follows a GitHub release within a day. The header `AlteriomPainlessMesh.h` gives access to the core painlessMesh functionality and the Alteriom extensions; `painlessMesh.h` alone gives the core.

#### Manual Installation

**Option 1: Download ZIP from GitHub Release**

1. Go to [Releases](https://github.com/Alteriom/painlessMesh/releases/latest)
2. Download the latest release ZIP file
3. In Arduino IDE: **Sketch** → **Include Library** → **Add .ZIP Library...**
4. Select the downloaded ZIP file
5. Restart Arduino IDE

**Option 2: Git Clone**

```bash
cd ~/Arduino/libraries/
git clone https://github.com/Alteriom/painlessMesh.git AlteriomPainlessMesh
# Restart Arduino IDE
```

### PlatformIO

The library is published on the [PlatformIO registry](https://registry.platformio.org/libraries/alteriom/AlteriomPainlessMesh) as `alteriom/AlteriomPainlessMesh`:

```ini
lib_deps =
    alteriom/AlteriomPainlessMesh@^2.0.0
```

### Dependencies

painlessMesh makes use of the following libraries, which can be installed through the Arduino Library Manager

- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [TaskScheduler](https://github.com/arkhipenko/TaskScheduler)
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) (ESP8266)
- [AsyncTCP](https://github.com/ESP32Async/AsyncTCP) (ESP32) - v3.4.7 or later (v3.3.0 was the first to work on the ESP32-C6)

If platformio is used to install the library, then the dependencies will be installed automatically.

## Quick Start with Alteriom Packages

### Basic Sensor Node

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

Scheduler userScheduler;
painlessMesh mesh;

Task taskSendMessage(TASK_SECOND * 30, TASK_FOREVER, []() {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast(msg);
});

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
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
| 7 | `CONTROL` | Deprecated, unused |
| 8 | `BROADCAST` | Application data for every node |
| 9 | `SINGLE` | Application data for one node |
| 620 | `GATEWAY_DATA` | An Internet request routed to a gateway (`sendToInternet()`) |
| 621 | `GATEWAY_ACK` | The gateway's answer to it |
| 622 | `GATEWAY_HEARTBEAT` | Gateway health monitoring |
| 630 | `MESSAGE_ACK` | Delivery confirmation for a message sent with a callback (2.0) |

**Note**: These protocol types are managed automatically by painlessMesh and are not typically used in application code.

#### Application-Level Package Types

These are the message types used by applications built on painlessMesh:

| Type | Class | Purpose | Fields |
|------|-------|---------|--------|
| 200 | `SensorPackage` | Environmental data | `temperature`, `humidity`, `pressure`, `sensorId`, `timestamp`, `batteryLevel` |
| 202 | `StatusPackage` | Health monitoring | `deviceStatus`, `uptime`, `freeMemory`, `wifiStrength`, `firmwareVersion` |
| 204 | `MetricsPackage` | Sensor metrics (schema v0.7.2+) | `cpuUsage`, `freeHeap`, `bytesReceived`, `currentThroughput`, `connectionQuality`, `wifiRSSI` |
| 400 | `CommandPackage` | Device control | `command`, `targetDevice`, `parameters`, `commandId` |
| 600 | `MeshNodeListPackage` | Mesh node list (`MESH_NODE_LIST`) | `nodes[]` (nodeId, status, lastSeen, signalStrength), `nodeCount`, `meshId` |
| 601 | `MeshTopologyPackage` | Mesh topology (`MESH_TOPOLOGY`) | `connections[]` (fromNode, toNode, linkQuality, latencyMs), `rootNode` |
| 602 | `MeshAlertPackage` | Mesh alerts (`MESH_ALERT`) | `alerts[]` (alertType, severity, message, nodeId), `alertCount` |
| 603 | `MeshBridgePackage` | Mesh bridge (`MESH_BRIDGE`) | `meshProtocol`, `fromNodeId`, `toNodeId`, `meshType`, `rawPayload`, `rssi`, `hopCount` |
| 604 | `EnhancedStatusPackage` | Mesh status (MESH_STATUS per schema v0.7.2+) | `nodeCount`, `connectionCount`, `messagesReceived`, `messagesSent`, `avgLatency`, `packetLossRate` |
| 605 | `HealthCheckPackage` | Mesh metrics (`MESH_METRICS`, schema v0.7.2+) | `healthStatus`, `problemFlags`, `memoryHealth`, `networkHealth`, `performanceHealth`, `recommendations` |
| 610 | `BridgeStatusPackage` | Bridge health monitoring (`BRIDGE_STATUS`) | `internetConnected`, `routerRSSI`, `routerChannel`, `uptime`, `gatewayIP`, `timestamp` |
| 611 | `BridgeElectionPackage` | Bridge failover election (`BRIDGE_ELECTION`) | `routerRSSI`, `routerChannel`, `uptime`, `freeMemory`, `timestamp`, `routerSSID` |
| 612 | `BridgeTakeoverPackage` | Bridge role announcement (`BRIDGE_TAKEOVER`) | `previousBridge`, `reason`, `routerChannel`, `timestamp` |
| 613 | `BridgeCoordinationPackage` | Multi-bridge coordination (`BRIDGE_COORDINATION`) | `priority`, `role`, `peerBridges[]`, `load`, `timestamp` |
| 614 | `NTPTimeSyncPackage` | NTP time synchronization (`TIME_SYNC_NTP`) | `ntpTime`, `accuracy`, `source`, `timestamp` |

## Key Features

### Core Features

- **🔄 Automatic Mesh Formation** - Nodes discover and connect automatically
- **📡 Self-Healing Network** - Adapts when nodes join/leave
- **⏰ Time Synchronization** - Coordinated actions across all nodes  
- **🔀 Smart Routing** - Broadcast, point-to-point, and neighbor messaging
- **🔌 Plugin System** - Type-safe custom message packages
- **📱 ESP32 & ESP8266** - Full support for both platforms
- **🛡️ Memory Efficient** - Optimized for resource-constrained devices

### Advanced Features

- **📡 Broadcast OTA** - Efficient firmware distribution for large meshes (50-100+ nodes)
- **🌉 MQTT Bridge** - Professional monitoring with Grafana/InfluxDB/Prometheus
- **📊 Topology Visualization** - D3.js, Cytoscape.js, Node-RED examples
- **🎯 Production Ready** - Enterprise-grade stability and performance
- **🔄 Automatic Bridge Failover** - RSSI-based election for high availability
- **🌐 Multi-Bridge Coordination** - Load balancing and geographic redundancy
- **💾 Message Queueing** - Zero data loss during Internet outages

## Examples & Use Cases

- **IoT Sensor Networks** - Environmental monitoring, smart agriculture
- **Home Automation** - Distributed lighting, HVAC control
- **Industrial Monitoring** - Equipment status, predictive maintenance  
- **Event Coordination** - Synchronized displays, distributed processing
- **Bridge Networks** - Connect mesh to WiFi/Internet/MQTT - [📖 Bridge Guide](BRIDGE_TO_INTERNET.md)

## Latest Release: v2.0.0 (September 7, 2026)

**Delivery confirmation, a unified send path, and a mesh that holds together on real hardware**

- `sendSingle()` and `sendBroadcast()` accept a delivery callback — it fires with `delivered=true` and the round-trip latency on acknowledgment, or `delivered=false` on timeout — and a `SendOptions` struct that carries a priority and a callback in one call. Priority is kept across hops.
- Thirty-odd defects in gateway failover, channel following, routing and the station scan, every one found in the serial logs of a six-family hardware rig and fixed there: a bridge that stops says so, the mesh follows an elected bridge to its channel and treats it as home, a node is in one place in every neighbour's tree, a dead connection is not a route, a stale scan does not consume a live one.
- The ESP32-C5 and ESP32-C6 (Arduino core 3.x) no longer hang after a channel follow; the ESP8266 is specified for small meshes or as a leaf, and says so at runtime.
- Gateway HTTP work is bounded by `NODE_TIMEOUT`, so an Internet request can no longer partition the mesh around its own gateway.
- The release candidate passed the rig's whole suite three times in a row. Delivery confirmation and cross-hop priority need 2.0 on every node of the path.

**[📋 Full CHANGELOG](CHANGELOG.md)**

## Getting Help

- **[Troubleshooting](USER_GUIDE.md#troubleshooting)** - Common issues, debug configuration, best practices
- **[FAQ](docsify-site/troubleshooting/faq.md)** - Frequently asked questions
- **[GitHub Issues](https://github.com/Alteriom/painlessMesh/issues)** - Bug reports and feature requests
- **[API Documentation](https://alteriom.github.io/painlessMesh/#/api/doxygen)** - Generated API docs

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

- **ESP32**: Arduino core 2.0.x or 3.x; the ESP32-C5 and ESP32-C6 need 3.x
- **ESP8266**: Arduino core 3.x
- **Dependencies**: ArduinoJson 7.x, TaskScheduler 4.x  
- **Development**: CMake, Ninja, Boost (for desktop testing)

### Testing Bridge/Internet Functionality

painlessMesh includes a **Mock HTTP Server** for testing `sendToInternet()` functionality without requiring actual Internet connectivity. This enables:

- 🚀 **Fast testing cycles** - Instant responses instead of waiting for external APIs
- 🔧 **Offline development** - No Internet connection required
- ✅ **Reproducible scenarios** - Control all test conditions precisely
- 🤖 **CI/CD automation** - Automated testing in pipelines

```bash
# Start mock server
cd test/mock-http-server
python3 server.py

# Test various HTTP scenarios
curl http://localhost:8080/status/200     # Success
curl http://localhost:8080/status/404     # Not Found
curl http://localhost:8080/whatsapp?...   # WhatsApp API simulation
```

See [Mock HTTP Server Documentation](test/mock-http-server/README.md) for complete usage guide.

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
git commit -am "release: vX.Y.Z"
git push origin main  # Triggers automated release
```

See [RELEASE_GUIDE.md](RELEASE_GUIDE.md) for complete release documentation.

## Contributing

Pull requests go to `Feat/next-release`, the integration branch for the next version; `main` holds released code and a push to it publishes a release. For branches, tests and what a pull request needs to show, see [CONTRIBUTING.md](CONTRIBUTING.md).


## 📚 Documentation

### 📖 Essential Guides

| Document | Description |
|----------|-------------|
| **[📘 User Guide](USER_GUIDE.md)** | **Complete comprehensive guide** - Everything you need to know |
| **[🌉 Bridge Guide](BRIDGE_TO_INTERNET.md)** | Connect mesh to Internet, MQTT, and cloud services |
| **[📋 Documentation Hub](.github/DOCUMENTATION.md)** | Central navigation for all documentation resources |
| **[🌐 Online Docs](https://alteriom.github.io/painlessMesh/)** | Interactive documentation website with API reference |

### 🚀 Quick Links

**New to AlteriomPainlessMesh?**
- [Quick Start](USER_GUIDE.md#getting-started) - Get your first mesh running in 5 minutes
- [Installation](USER_GUIDE.md#installation) - Arduino IDE and PlatformIO setup
- [First Mesh](USER_GUIDE.md#your-first-mesh-network) - Build a multi-node network

**Reference Documentation:**
- [Core API](USER_GUIDE.md#api-reference) - painlessMesh class methods
- [Alteriom Extensions](examples/alteriom/README.md) - SensorPackage, CommandPackage, StatusPackage
- [Examples](examples/) - 18 example directories, 21 sketches, every one compiled for esp32 and esp8266 on each pull request; [.github/DOCUMENTATION.md](.github/DOCUMENTATION.md) lists what each shows

**Need Help?**
- [Troubleshooting](USER_GUIDE.md#troubleshooting) - Common issues and how to read the logs
- [FAQ](docsify-site/troubleshooting/faq.md) - Frequently asked questions
- [GitHub Issues](https://github.com/Alteriom/painlessMesh/issues) - Bug reports and support

## 🔧 Quick API Reference

**Core Methods:**
```cpp
#include "painlessMesh.h"

painlessMesh mesh;

// Initialize mesh
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

// Main loop - call this in loop()
mesh.update();

// Send messages
mesh.sendBroadcast("Hello everyone!");
mesh.sendSingle(nodeId, "Hello specific node");

// Get information
uint32_t myId = mesh.getNodeId();
std::list<uint32_t> nodes = mesh.getNodeList();

// Register callbacks
mesh.onReceive(&receivedCallback);
mesh.onNewConnection(&newConnectionCallback);
mesh.onChangedConnections(&changedConnectionCallback);

// Debug configuration
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
```

**For complete API documentation, see [USER_GUIDE.md](USER_GUIDE.md#api-reference) or [online docs](https://alteriom.github.io/painlessMesh/#/api/core-api).**

## Funding

If you like the library please consider supporting its development. Your contributions help me spend more time improving painlessMesh.

**[Donate via PayPal](https://www.paypal.com/paypalme/domlavoie)** • [dominic.lavoie@gmail.com](mailto:dominic.lavoie@gmail.com)

Most development of painlessMesh has been done as a hobby, but some specific features have been funded by the companies listed below:

![Sowillo](https://www.sowillo.com/wp-content/uploads/2019/04/Logo-Sowillo-1.png)

[Sowillo](http://sowillo.com/en/)
