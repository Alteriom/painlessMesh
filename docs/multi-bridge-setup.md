# Multi-Bridge Setup Guide

> **Complete guide for deploying multiple simultaneous bridge nodes in painlessMesh**

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Use Cases](#use-cases)
- [Requirements](#requirements)
- [Configuration](#configuration)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Testing](#testing)
- [Troubleshooting](#troubleshooting)
- [Advanced Topics](#advanced-topics)

## Overview

Multi-bridge coordination enables multiple bridge nodes to operate simultaneously in a painlessMesh network, providing:

- **High Availability**: Zero downtime during bridge failures
- **Load Balancing**: Distribute traffic across multiple Internet connections
- **Geographic Distribution**: Bridges in different physical locations
- **Redundancy**: Multiple paths to the Internet
- **Traffic Shaping**: Different message types to different bridges (planned)

### Key Features

| Feature | Status | Description |
|---------|--------|-------------|
| Multiple Simultaneous Bridges | ✅ Implemented | 2-5 bridges can operate concurrently |
| Bridge Priority System | ✅ Implemented | Priorities 1-10 for deterministic selection |
| Selection Strategies | ✅ Implemented | Priority-based, Round-robin, Best-signal |
| Automatic Coordination | ✅ Implemented | Bridges discover and coordinate automatically |
| Load Reporting | ✅ Implemented | Bridges report current load percentage |
| Failover Integration | ✅ Implemented | Works with bridge failover (Issue #64) |

## Architecture

### System Diagram

```
                    Internet
                       |
        +--------------+---------------+
        |                              |
    Router A                       Router B
        |                              |
   [Bridge 1]                     [Bridge 2]
   Priority: 10                   Priority: 5
   Role: Primary                  Role: Secondary
        |                              |
        +-------+----------+-----------+
                |          |
            [Node A]   [Node B]
            Regular    Regular
            Nodes      Nodes
```

### Component Roles

**Bridge Nodes:**
- Connect to both mesh (AP) and router (STA)
- Broadcast coordination messages (Type 613) every 30s
- Report priority, role, load, and peer bridges
- Act as Internet gateways for mesh

**Regular Nodes:**
- Connect to mesh only (STA mode)
- Receive bridge coordination messages
- Track available bridges and their priorities
- Select best bridge using configured strategy

### Coordination Protocol

**Message Type:** 613 (BRIDGE_COORDINATION)

**Message Structure:**
```json
{
  "type": 613,
  "from": 123456,
  "routing": 2,
  "priority": 10,
  "role": "primary",
  "load": 45,
  "timestamp": 1234567890,
  "peerBridges": [789012, 345678]
}
```

**Broadcast Interval:** 30 seconds  
**Timeout:** 60 seconds (2 missed heartbeats)

## Use Cases

### Use Case 1: High-Availability Production System

**Scenario:** Critical IoT system that cannot tolerate Internet outages

**Setup:**
- Primary Bridge (Priority 10): Main Internet connection
- Secondary Bridge (Priority 5): Backup Internet connection
- Strategy: PRIORITY_BASED (default)

**Behavior:**
- All traffic uses primary bridge normally
- If primary fails, automatic switch to secondary
- Zero downtime, no manual intervention

**Example:**
```cpp
// Primary Bridge
mesh.initAsBridge(MESH_SSID, MESH_PASSWORD,
                  "PrimaryRouter", "pass1",
                  &scheduler, 5555, 10);

// Secondary Bridge  
mesh.initAsBridge(MESH_SSID, MESH_PASSWORD,
                  "BackupRouter", "pass2",
                  &scheduler, 5555, 5);
```

### Use Case 2: Load Balancing

**Scenario:** High-traffic mesh network with multiple Internet connections

**Setup:**
- Bridge 1 (Priority 7): Internet connection A
- Bridge 2 (Priority 7): Internet connection B
- Strategy: ROUND_ROBIN

**Behavior:**
- Messages distributed evenly across both bridges
- 50/50 traffic split
- Maximizes available bandwidth

**Example:**
```cpp
// On regular nodes
mesh.setBridgeSelectionStrategy(painlessMesh::ROUND_ROBIN);

// Both bridges equal priority
mesh.initAsBridge(MESH_SSID, MESH_PASSWORD,
                  "RouterA", "pass1",
                  &scheduler, 5555, 7);
```

### Use Case 3: Geographic Distribution

**Scenario:** Large mesh network spanning multiple buildings

**Setup:**
- Building A: Bridge 1 (Priority 10) → Local Internet A
- Building B: Bridge 2 (Priority 10) → Local Internet B
- Strategy: BEST_SIGNAL

**Behavior:**
- Nodes use closest bridge (best signal strength)
- Automatic selection based on location
- Optimizes latency and reliability

**Example:**
```cpp
// On regular nodes
mesh.setBridgeSelectionStrategy(painlessMesh::BEST_SIGNAL);

// Both bridges same priority, selected by signal
mesh.initAsBridge(MESH_SSID, MESH_PASSWORD,
                  "BuildingA_Router", "pass1",
                  &scheduler, 5555, 10);
```

## Requirements

### Hardware

**Bridge Nodes:**
- ESP32 or ESP8266 with WiFi
- 2+ MB flash recommended
- Stable power supply (no battery)

**Regular Nodes:**
- ESP32 or ESP8266 with WiFi
- 1+ MB flash
- Battery power acceptable

**Network:**
- 1+ WiFi routers with Internet connection
- All devices must support 2.4 GHz WiFi
- Same WiFi channel recommended for all

### Software

**Library Version:**
- painlessMesh v1.8.1 or later
- ArduinoJson v6.x or v7.x
- TaskScheduler v3.x

**Platform:**
- Arduino IDE 1.8+ or PlatformIO
- ESP32 Arduino Core 2.x+ or ESP8266 Core 3.x+

**Dependencies:**
- Bridge Status Broadcast (Issue #63) ✅
- Bridge Failover (Issue #64) ✅

## Configuration

### Step 1: Enable Multi-Bridge Mode

Multi-bridge mode must be enabled on **all bridge nodes**:

```cpp
mesh.enableMultiBridge(true);
```

**Note:** Regular nodes automatically detect multi-bridge mode and don't need explicit configuration.

### Step 2: Set Bridge Priority

When initializing a bridge, specify priority (1-10):

```cpp
mesh.initAsBridge(MESH_SSID, MESH_PASSWORD,
                  ROUTER_SSID, ROUTER_PASSWORD,
                  &scheduler, MESH_PORT, 
                  priority);  // 1-10
```

**Priority Guidelines:**

| Priority | Role | When to Use |
|----------|------|-------------|
| 10 | Primary | Main bridge, best Internet connection |
| 8-9 | Primary-High | Co-primary for load sharing |
| 5-7 | Secondary | Backup bridge, hot standby |
| 2-4 | Tertiary | Last resort backup |
| 1 | Standby | Only if all others fail |

**Role Assignment:**
- Priority ≥ 8 → "primary"
- Priority ≥ 5 → "secondary"
- Priority < 5 → "standby"

### Step 3: Configure Selection Strategy

Choose how regular nodes select bridges:

```cpp
// Priority-Based (default) - Always use highest priority
mesh.setBridgeSelectionStrategy(painlessMesh::PRIORITY_BASED);

// Round-Robin - Distribute load evenly
mesh.setBridgeSelectionStrategy(painlessMesh::ROUND_ROBIN);

// Best Signal - Use bridge with best RSSI
mesh.setBridgeSelectionStrategy(painlessMesh::BEST_SIGNAL);
```

**Strategy Comparison:**

| Strategy | Pros | Cons | Best For |
|----------|------|------|----------|
| PRIORITY_BASED | Predictable, deterministic | All traffic on one bridge | Production systems |
| ROUND_ROBIN | Even load distribution | May not respect priority | High-traffic systems |
| BEST_SIGNAL | Optimizes latency | Requires WiFi scanning | Large/mobile networks |

### Step 4: Set Maximum Bridges (Optional)

Limit number of tracked bridges:

```cpp
mesh.setMaxBridges(3);  // Default: 2, Max: 5
```

**Recommendations:**
- Small networks (< 20 nodes): 2 bridges
- Medium networks (20-50 nodes): 3 bridges
- Large networks (50+ nodes): 4-5 bridges

## API Reference

### Configuration Methods

#### enableMultiBridge()

```cpp
void enableMultiBridge(bool enabled)
```

Enable or disable multi-bridge coordination mode.

**Parameters:**
- `enabled` - true to enable, false to disable

**Example:**
```cpp
mesh.enableMultiBridge(true);
```

**Notes:**
- Must be called on bridge nodes before `initAsBridge()`
- Regular nodes auto-detect multi-bridge mode
- Default: disabled (single-bridge mode)

#### setBridgeSelectionStrategy()

```cpp
void setBridgeSelectionStrategy(BridgeSelectionStrategy strategy)
```

Set bridge selection algorithm for regular nodes.

**Parameters:**
- `strategy` - One of:
  - `painlessMesh::PRIORITY_BASED` - Use highest priority (default)
  - `painlessMesh::ROUND_ROBIN` - Distribute evenly
  - `painlessMesh::BEST_SIGNAL` - Use best RSSI

**Example:**
```cpp
mesh.setBridgeSelectionStrategy(painlessMesh::ROUND_ROBIN);
```

#### setMaxBridges()

```cpp
void setMaxBridges(uint8_t maxBridges)
```

Set maximum number of concurrent bridges to track.

**Parameters:**
- `maxBridges` - Maximum bridges (1-5)

**Example:**
```cpp
mesh.setMaxBridges(3);
```

**Notes:**
- Values < 1 clamped to 1
- Values > 5 clamped to 5
- Default: 2

### Bridge Initialization

#### initAsBridge() with Priority

```cpp
void initAsBridge(TSTRING meshSSID, TSTRING meshPassword,
                  TSTRING routerSSID, TSTRING routerPassword,
                  Scheduler *baseScheduler, uint16_t port, 
                  uint8_t priority)
```

Initialize node as bridge with specified priority.

**Parameters:**
- `meshSSID` - Mesh network name
- `meshPassword` - Mesh password
- `routerSSID` - Router SSID to connect to
- `routerPassword` - Router password
- `baseScheduler` - Task scheduler instance
- `port` - TCP port (default: 5555)
- `priority` - Bridge priority (1-10)

**Example:**
```cpp
mesh.initAsBridge("MyMesh", "meshpass",
                  "MyRouter", "routerpass",
                  &scheduler, 5555, 10);
```

**Notes:**
- Priority 10 = highest (primary bridge)
- Priority 1 = lowest (standby bridge)
- Auto-detects router channel
- Sets node as root automatically

### Bridge Discovery Methods

#### getActiveBridges()

```cpp
std::vector<uint32_t> getActiveBridges()
```

Get list of all currently active bridge node IDs.

**Returns:**
- Vector of bridge node IDs with Internet connection

**Example:**
```cpp
auto bridges = mesh.getActiveBridges();
Serial.printf("Active bridges: %d\n", bridges.size());
for (auto bridgeId : bridges) {
  Serial.printf("  - Bridge: %u\n", bridgeId);
}
```

**Notes:**
- Only includes bridges with Internet connection
- Only includes healthy bridges (seen within 60s)
- Updates automatically from coordination messages

#### getRecommendedBridge()

```cpp
uint32_t getRecommendedBridge()
```

Get best bridge node ID based on current strategy.

**Returns:**
- Bridge node ID, or 0 if no bridge available

**Example:**
```cpp
uint32_t bridgeId = mesh.getRecommendedBridge();
if (bridgeId != 0) {
  mesh.sendSingle(bridgeId, "Hello!");
} else {
  Serial.println("No bridge available");
}
```

**Notes:**
- Respects current selection strategy
- Returns 0 if no bridges available
- Thread-safe, can call frequently

#### selectBridge()

```cpp
void selectBridge(uint32_t bridgeNodeId)
```

Manually select specific bridge for next transmission.

**Parameters:**
- `bridgeNodeId` - Node ID of bridge to use

**Example:**
```cpp
// Override strategy for critical message
mesh.selectBridge(primaryBridgeId);
mesh.sendSingle(primaryBridgeId, criticalData);
```

**Notes:**
- One-time override of selection strategy
- Resets after one message
- Use sparingly for critical messages only

#### isMultiBridgeEnabled()

```cpp
bool isMultiBridgeEnabled() const
```

Check if multi-bridge mode is enabled.

**Returns:**
- true if multi-bridge coordination active

**Example:**
```cpp
if (mesh.isMultiBridgeEnabled()) {
  Serial.println("Multi-bridge mode active");
}
```

## Examples

See complete examples in `examples/multi_bridge/`:

- **primary_bridge.ino** - Primary bridge configuration
- **secondary_bridge.ino** - Secondary bridge configuration  
- **regular_node.ino** - Regular node with bridge awareness

### Complete Primary Bridge Example

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "ProductionMesh"
#define MESH_PASSWORD   "meshpass"
#define MESH_PORT       5555
#define ROUTER_SSID     "PrimaryRouter"
#define ROUTER_PASSWORD "routerpass"

Scheduler userScheduler;
painlessMesh mesh;

// Monitor bridge status
Task taskBridgeMonitor(10000, TASK_FOREVER, [](){
  Serial.println("\n=== Primary Bridge Status ===");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Connected Nodes: %d\n", mesh.getNodeList().size());
  
  auto activeBridges = mesh.getActiveBridges();
  Serial.printf("Active Bridges: %d\n", activeBridges.size());
  for (auto bridgeId : activeBridges) {
    Serial.printf("  - Bridge: %u%s\n", bridgeId,
                  (bridgeId == mesh.getNodeId()) ? " (ME)" : "");
  }
  
  Serial.printf("Recommended Bridge: %u\n", mesh.getRecommendedBridge());
  Serial.println("============================\n");
});

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n=== PRIMARY BRIDGE INITIALIZATION ===\n");
  
  // Configure debugging
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Enable multi-bridge coordination
  mesh.enableMultiBridge(true);
  
  // Set selection strategy (PRIORITY_BASED is default)
  mesh.setBridgeSelectionStrategy(painlessMesh::PRIORITY_BASED);
  
  // Initialize as primary bridge (priority 10)
  mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                    ROUTER_SSID, ROUTER_PASSWORD,
                    &userScheduler, MESH_PORT, 10);
  
  // Setup callbacks
  mesh.onReceive([](uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
  });
  
  mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
  });
  
  // Add monitoring task
  userScheduler.addTask(taskBridgeMonitor);
  taskBridgeMonitor.enable();
  
  Serial.println("\n=== PRIMARY BRIDGE READY ===");
  Serial.println("Priority: 10 (Primary)");
  Serial.println("This bridge will handle all mesh traffic");
  Serial.println("Secondary bridge will activate if this fails\n");
}

void loop() {
  mesh.update();
}
```

### Complete Regular Node Example

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "ProductionMesh"
#define MESH_PASSWORD   "meshpass"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Send test messages periodically
Task taskSendMessage(5000, TASK_FOREVER, [](){
  uint32_t bridgeId = mesh.getRecommendedBridge();
  
  if (bridgeId != 0) {
    String msg = "Hello from node " + String(mesh.getNodeId());
    Serial.printf("Sending to bridge %u: %s\n", bridgeId, msg.c_str());
    mesh.sendSingle(bridgeId, msg);
  } else {
    Serial.println("No bridge available - message queued");
  }
});

// Monitor network status
Task taskNetworkStatus(15000, TASK_FOREVER, [](){
  Serial.println("\n=== Network Status ===");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Connected Nodes: %d\n", mesh.getNodeList().size());
  
  auto activeBridges = mesh.getActiveBridges();
  Serial.printf("Active Bridges: %d\n", activeBridges.size());
  for (auto bridgeId : activeBridges) {
    Serial.printf("  - Bridge: %u\n", bridgeId);
  }
  
  Serial.printf("Internet Available: %s\n",
                mesh.hasInternetConnection() ? "YES" : "NO");
  Serial.printf("Recommended Bridge: %u\n",
                mesh.getRecommendedBridge());
  Serial.println("=====================\n");
});

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n=== REGULAR NODE INITIALIZATION ===\n");
  
  // Configure debugging
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Initialize as regular node
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Setup callbacks
  mesh.onReceive([](uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
  });
  
  mesh.onBridgeStatusChanged([](uint32_t bridgeId, bool hasInternet) {
    Serial.printf("Bridge %u: Internet %s\n",
                  bridgeId, hasInternet ? "ONLINE" : "OFFLINE");
  });
  
  mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
  });
  
  // Add tasks
  userScheduler.addTask(taskSendMessage);
  userScheduler.addTask(taskNetworkStatus);
  taskSendMessage.enable();
  taskNetworkStatus.enable();
  
  Serial.println("\n=== REGULAR NODE READY ===");
  Serial.println("Automatically discovers bridges");
  Serial.println("Uses configured selection strategy\n");
}

void loop() {
  mesh.update();
}
```

## Testing

### Test Scenarios

#### Scenario 1: Dual Bridge Normal Operation

**Objective:** Verify two bridges coordinate and nodes prefer primary

**Steps:**
1. Flash primary bridge (priority 10)
2. Flash secondary bridge (priority 5)
3. Flash 2-3 regular nodes
4. Power on all devices
5. Monitor serial output

**Expected Results:**
- ✅ Both bridges connect to routers
- ✅ Both bridges see each other in coordination messages
- ✅ Regular nodes discover both bridges
- ✅ Regular nodes prefer primary bridge (priority 10)
- ✅ `getActiveBridges()` returns 2 bridge IDs
- ✅ `getRecommendedBridge()` returns primary bridge ID

#### Scenario 2: Primary Bridge Failure

**Objective:** Verify automatic failover to secondary bridge

**Steps:**
1. Setup dual bridges and regular nodes (as above)
2. Wait for stable operation (2+ minutes)
3. Disconnect primary bridge power
4. Monitor regular nodes for failover

**Expected Results:**
- ✅ Regular nodes detect primary bridge offline within 60s
- ✅ `getActiveBridges()` returns 1 bridge ID (secondary only)
- ✅ `getRecommendedBridge()` returns secondary bridge ID
- ✅ Messages continue flowing through secondary bridge
- ✅ No messages lost during transition

#### Scenario 3: Round-Robin Load Balancing

**Objective:** Verify even distribution across bridges

**Steps:**
1. Setup dual bridges with equal priority (both 7)
2. Flash regular node with ROUND_ROBIN strategy
3. Send 10 messages from regular node
4. Monitor which bridge receives each message

**Expected Results:**
- ✅ First message → Bridge 1
- ✅ Second message → Bridge 2
- ✅ Third message → Bridge 1
- ✅ Distribution is approximately 50/50
- ✅ Both bridges handle traffic

#### Scenario 4: Best Signal Selection

**Objective:** Verify selection based on WiFi signal strength

**Steps:**
1. Setup dual bridges at different physical locations
2. Flash regular node with BEST_SIGNAL strategy
3. Move regular node between locations
4. Monitor bridge selection

**Expected Results:**
- ✅ Node near Bridge 1 selects Bridge 1
- ✅ Node near Bridge 2 selects Bridge 2
- ✅ Selection changes as node moves
- ✅ Always uses bridge with best RSSI

### Unit Tests

Run comprehensive unit tests:

```bash
cd painlessMesh
cmake -G Ninja .
ninja
./bin/catch_plugin
```

**Expected Output:**
```
===============================================================================
All tests passed (67 assertions in 4 test cases)
```

**Test Coverage:**
- ✅ BridgeCoordinationPackage serialization
- ✅ JSON round-trip integrity
- ✅ Empty peer list handling
- ✅ Maximum value edge cases
- ✅ Field preservation

## Troubleshooting

### Issue: No Bridges Found

**Symptoms:**
```
Active Bridges: 0
Internet Available: NO
Recommended Bridge: 0
```

**Possible Causes:**
1. Bridges not powered on
2. Router credentials incorrect
3. Bridges failed to connect to router
4. Mesh SSID/password mismatch

**Solutions:**
1. Verify bridges are powered and running
2. Check router credentials in bridge code
3. Verify router SSID is correct
4. Check serial output of bridges for connection status
5. Ensure all devices use same MESH_PREFIX and MESH_PASSWORD

**Diagnostic Commands:**
```cpp
// On bridge node
Serial.printf("Router connected: %s\n",
              WiFi.status() == WL_CONNECTED ? "YES" : "NO");
Serial.printf("Router IP: %s\n",
              WiFi.localIP().toString().c_str());
Serial.printf("Router RSSI: %d dBm\n", WiFi.RSSI());
```

### Issue: Bridges Not Coordinating

**Symptoms:**
```
Active Bridges: 1
Expected: 2
Coordination messages not appearing in logs
```

**Possible Causes:**
1. `enableMultiBridge(true)` not called on bridges
2. Bridges not connected to mesh
3. Mesh network partitioned
4. Coordination broadcast failing

**Solutions:**
1. Verify `enableMultiBridge(true)` called before `initAsBridge()`
2. Check mesh connectivity between bridges
3. Enable CONNECTION debug messages
4. Verify bridges see each other in mesh node list

**Diagnostic Commands:**
```cpp
// On bridge node
Serial.printf("Multi-bridge enabled: %s\n",
              mesh.isMultiBridgeEnabled() ? "YES" : "NO");
              
auto nodes = mesh.getNodeList(false);
Serial.printf("Mesh nodes: %d\n", nodes.size());
for (auto node : nodes) {
  Serial.printf("  - Node: %u\n", node);
}
```

### Issue: Wrong Bridge Selected

**Symptoms:**
```
Expected: Primary bridge (priority 10)
Actual: Secondary bridge (priority 5)
```

**Possible Causes:**
1. Priority not set correctly
2. Primary bridge offline
3. Wrong selection strategy
4. Primary bridge no Internet connection

**Solutions:**
1. Verify priority values in `initAsBridge()` calls
2. Check primary bridge is online and healthy
3. Verify strategy is PRIORITY_BASED
4. Check primary bridge has Internet connection

**Diagnostic Commands:**
```cpp
// On regular node
auto bridges = mesh.getBridges();
for (auto& bridge : bridges) {
  Serial.printf("Bridge %u: RSSI=%d, Internet=%s, Healthy=%s\n",
                bridge.nodeId, bridge.routerRSSI,
                bridge.internetConnected ? "YES" : "NO",
                bridge.isHealthy() ? "YES" : "NO");
}
```

### Issue: Failover Not Working

**Symptoms:**
```
Primary bridge offline
Regular nodes not switching to secondary
Messages timing out
```

**Possible Causes:**
1. Secondary bridge not available
2. Bridge timeout not expired (60s)
3. Secondary bridge no Internet connection
4. Bridge status broadcasts disabled

**Solutions:**
1. Verify secondary bridge is online and connected
2. Wait full 60s timeout period
3. Check secondary bridge Internet connection
4. Verify `enableBridgeStatusBroadcast(true)` called

**Diagnostic Commands:**
```cpp
// Monitor bridge health
auto primary = mesh.getPrimaryBridge();
if (primary != nullptr) {
  Serial.printf("Primary: %u (seen %u ms ago)\n",
                primary->nodeId, millis() - primary->lastSeen);
} else {
  Serial.println("No primary bridge available");
}
```

### Issue: High Memory Usage

**Symptoms:**
```
Free heap: < 20KB
Crashes or resets
Memory allocation failures
```

**Possible Causes:**
1. Too many tracked bridges
2. Large peer bridge lists
3. Memory leak in application code

**Solutions:**
1. Reduce `setMaxBridges()` to 2-3
2. Limit number of concurrent bridges
3. Profile application memory usage
4. Check for memory leaks in custom code

**Diagnostic Commands:**
```cpp
Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
Serial.printf("Max bridges: %u\n", maxConcurrentBridges);
Serial.printf("Known bridges: %u\n", knownBridges.size());
```

## Advanced Topics

### Three or More Bridges

For very large or critical deployments:

```cpp
// Bridge 1: Primary (highest priority)
mesh.initAsBridge(ssid, pass, router1, pass1, &sched, port, 10);

// Bridge 2: Secondary (medium priority)
mesh.initAsBridge(ssid, pass, router2, pass2, &sched, port, 7);

// Bridge 3: Tertiary (backup)
mesh.initAsBridge(ssid, pass, router3, pass3, &sched, port, 3);

// Set max tracked bridges
mesh.setMaxBridges(3);
```

**Considerations:**
- Memory usage increases with more bridges
- Coordination overhead increases
- Recommended maximum: 5 bridges
- Most deployments work well with 2-3 bridges

### Geographic Distribution

For mesh networks spanning multiple buildings:

```cpp
// Building A Bridge (equal priority)
mesh.initAsBridge(ssid, pass, "BuildingA_Router", pass1, &sched, port, 10);

// Building B Bridge (equal priority)
mesh.initAsBridge(ssid, pass, "BuildingB_Router", pass2, &sched, port, 10);

// Regular nodes use BEST_SIGNAL strategy
mesh.setBridgeSelectionStrategy(painlessMesh::BEST_SIGNAL);
```

**Benefits:**
- Nodes automatically use closest bridge
- Optimizes latency and reliability
- Supports mobile nodes

### Traffic Shaping (Planned)

Future enhancement for routing different traffic types:

```cpp
// Coming in v1.8.2+
mesh.setBridgeSelectionStrategy(painlessMesh::TRAFFIC_TYPE);
mesh.routeTrafficType(ALARM_MESSAGE, primaryBridge);
mesh.routeTrafficType(SENSOR_DATA, secondaryBridge);
```

### Weighted Round-Robin (Planned)

Future enhancement for unequal load distribution:

```cpp
// Coming in v1.8.2+
mesh.setBridgeWeight(bridge1, 70);  // 70% of traffic
mesh.setBridgeWeight(bridge2, 30);  // 30% of traffic
```

### Performance Tuning

**Memory Optimization:**
```cpp
// Reduce max bridges if memory limited
mesh.setMaxBridges(2);

// Increase coordination interval to reduce overhead
// (requires code modification - not configurable via API)
```

**Network Optimization:**
```cpp
// Adjust bridge timeout for faster failover
mesh.setBridgeTimeout(30000);  // 30 seconds instead of 60

// Adjust bridge status broadcast interval
mesh.setBridgeStatusInterval(15000);  // 15 seconds instead of 30
```

## Related Documentation

- [Bridge Status Feature](../BRIDGE_STATUS_FEATURE.md) - Bridge status broadcasts
- [Bridge Failover](../docs/BRIDGE_FAILOVER.md) - Automatic failover system
- [Bridge Architecture](../BRIDGE_ARCHITECTURE_IMPLEMENTATION.md) - Overall bridge design
- [Examples](../examples/multi_bridge/README.md) - Complete working examples
- [API Reference](../src/arduino/wifi.hpp) - Full API documentation

## Support

**Resources:**
- GitHub Issues: https://github.com/Alteriom/painlessMesh/issues
- Discussions: https://github.com/Alteriom/painlessMesh/discussions
- Examples: `examples/multi_bridge/`
- Wiki: https://alteriom.github.io/painlessMesh/

**Reporting Issues:**
1. Check troubleshooting section
2. Enable debug messages: `mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);`
3. Capture serial output from all nodes
4. Report with code snippets and logs

## Changelog

### v1.8.1
- ✅ Initial multi-bridge coordination implementation
- ✅ BridgeCoordinationPackage (Type 613)
- ✅ Three bridge selection strategies
- ✅ Bridge priority system (1-10)
- ✅ Complete API and examples

---

**Document Version:** 1.0  
**Last Updated:** 2025-11-11  
**Status:** Complete and Production-Ready
