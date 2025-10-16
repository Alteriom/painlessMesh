# painlessMesh Implementation Plan for MQTT Schema v0.5.0

**Date:** October 12, 2025  
**Target:** Support mesh_topology.schema.json and mesh_event.schema.json  
**Status:** 📋 IMPLEMENTATION PLAN

---

## Executive Summary

Assuming @alteriom/mqtt-schema v0.5.0 is released with the new mesh topology and event schemas, this document outlines **all required implementations** in the painlessMesh library to fully support these features.

### What's Already Done ✅

1. ✅ **Command Bridge** - MqttCommandBridge class (417 lines)
2. ✅ **Gateway Example** - mqttCommandBridge.ino with basic topology
3. ✅ **Node Handler** - mesh_command_node.ino with command responses
4. ✅ **Status Packages** - Enhanced StatusPackage with response tracking
5. ✅ **Basic Topology** - Simple node list publishing every 30s

### What Needs Implementation ⚠️

1. ⚠️ **Schema-Compliant Topology Reporter** - Full mesh structure with connections
2. ⚠️ **Connection Quality Tracking** - RSSI, latency, hop count per link
3. ⚠️ **Mesh Event Publisher** - Real-time node join/leave notifications
4. ⚠️ **Topology Command Handler** - `get_topology` command support
5. ⚠️ **Enhanced Gateway Bridge** - Schema v0.5.0 compliance
6. ⚠️ **Connection Metadata API** - Expose connection details from painlessMesh core

---

## Part 1: Core Library Enhancements

### 1.1 Add Connection Metadata API

**File:** `src/painlessmesh/mesh.hpp`

**Problem:** painlessMesh tracks connections internally but doesn't expose connection quality metrics (RSSI, latency, hop count).

**Solution:** Add public API to retrieve connection metadata.

#### New Public Methods

```cpp
/**
 * @brief Get connection quality for a specific link
 * @param fromNode Source node ID
 * @param toNode Destination node ID
 * @return Connection quality (0-100) or -1 if not connected
 */
int getConnectionQuality(uint32_t fromNode, uint32_t toNode);

/**
 * @brief Get RSSI for a direct connection
 * @param nodeId Connected node ID
 * @return RSSI in dBm (typically -30 to -90) or 0 if not available
 */
int getConnectionRSSI(uint32_t nodeId);

/**
 * @brief Get average latency to a node
 * @param nodeId Target node ID
 * @return Latency in milliseconds or -1 if not available
 */
int getConnectionLatency(uint32_t nodeId);

/**
 * @brief Get hop count to a node
 * @param nodeId Target node ID
 * @return Number of hops or -1 if unreachable
 */
int getHopCount(uint32_t nodeId);

/**
 * @brief Get all direct connections with metadata
 * @return List of connection objects with quality metrics
 */
std::vector<ConnectionInfo> getConnectionDetails();

/**
 * @brief Get routing table for mesh visualization
 * @return Map of destination -> next hop mappings
 */
std::map<uint32_t, uint32_t> getRoutingTable();
```

#### New Data Structure

```cpp
struct ConnectionInfo {
  uint32_t nodeId;           // Connected node ID
  uint32_t lastSeen;         // Timestamp of last message (ms)
  int rssi;                  // Signal strength (dBm)
  int avgLatency;            // Average round-trip time (ms)
  int hopCount;              // Hops from current node
  int quality;               // Connection quality (0-100)
  uint32_t messagesRx;       // Messages received
  uint32_t messagesTx;       // Messages sent
  uint32_t messagesDropped;  // Failed transmissions
};
```

#### Implementation Location

**File to modify:** `src/painlessmesh/mesh.hpp`

**Add after line 365 (after `subConnectionJson()`):**

```cpp
  /**
   * Get detailed connection information for all direct neighbors
   */
  std::vector<ConnectionInfo> getConnectionDetails() {
    std::vector<ConnectionInfo> connections;
    
    for (auto conn : this->connections) {
      if (conn->isConnected()) {
        ConnectionInfo info;
        info.nodeId = conn->nodeId;
        info.lastSeen = conn->timeLastReceived;
        info.rssi = conn->getRSSI();  // Need to add this to Connection class
        info.avgLatency = conn->getLatency();  // Need to add this
        info.hopCount = 1;  // Direct connection
        info.quality = conn->getQuality();  // Need to add this
        info.messagesRx = conn->messagesRx;  // Need to add counter
        info.messagesTx = conn->messagesTx;  // Need to add counter
        info.messagesDropped = conn->messagesDropped;  // Need to add counter
        
        connections.push_back(info);
      }
    }
    
    return connections;
  }
  
  /**
   * Get hop count to specific node using routing table
   */
  int getHopCount(uint32_t nodeId) {
    auto route = this->findRoute(nodeId);
    if (route.empty()) return -1;
    return route.size() - 1;  // Don't count source node
  }
  
  /**
   * Get routing table as map (destination -> next hop)
   */
  std::map<uint32_t, uint32_t> getRoutingTable() {
    std::map<uint32_t, uint32_t> table;
    auto nodeList = this->getNodeList(false);
    
    for (auto destNode : nodeList) {
      auto route = this->findRoute(destNode);
      if (!route.empty() && route.size() > 1) {
        table[destNode] = route[1];  // Next hop after source
      }
    }
    
    return table;
  }
```

**Estimated Effort:** 4-6 hours (requires adding metrics tracking to Connection class)

---

### 1.2 Enhance Connection Class with Metrics

**File:** `src/painlessmesh/tcp.hpp` (or wherever Connection is defined)

**Add to Connection class:**

```cpp
class Connection {
public:
  // Existing members...
  
  // New metrics tracking
  uint32_t messagesRx = 0;
  uint32_t messagesTx = 0;
  uint32_t messagesDropped = 0;
  uint32_t timeLastReceived = 0;
  
  // Latency tracking
  std::vector<uint32_t> latencySamples;
  const size_t MAX_LATENCY_SAMPLES = 10;
  
  /**
   * Record message received timestamp
   */
  void onMessageReceived() {
    messagesRx++;
    timeLastReceived = millis();
  }
  
  /**
   * Record message sent
   */
  void onMessageSent(bool success) {
    if (success) {
      messagesTx++;
    } else {
      messagesDropped++;
    }
  }
  
  /**
   * Record round-trip time sample
   */
  void recordLatency(uint32_t latencyMs) {
    latencySamples.push_back(latencyMs);
    if (latencySamples.size() > MAX_LATENCY_SAMPLES) {
      latencySamples.erase(latencySamples.begin());
    }
  }
  
  /**
   * Get average latency from recent samples
   */
  int getLatency() {
    if (latencySamples.empty()) return -1;
    
    uint32_t sum = 0;
    for (auto sample : latencySamples) {
      sum += sample;
    }
    return sum / latencySamples.size();
  }
  
  /**
   * Calculate connection quality (0-100)
   * Based on: latency, packet loss, RSSI
   */
  int getQuality() {
    // Simple quality calculation
    int quality = 100;
    
    // Penalize high latency (>100ms)
    int latency = getLatency();
    if (latency > 100) quality -= (latency - 100) / 5;
    
    // Penalize packet loss
    if (messagesTx > 0) {
      int lossRate = (messagesDropped * 100) / messagesTx;
      quality -= lossRate;
    }
    
    // Penalize weak RSSI (if available)
    int rssi = getRSSI();
    if (rssi < -80) quality -= (80 + rssi);
    
    return max(0, min(100, quality));
  }
  
  /**
   * Get WiFi RSSI if available
   * Note: Requires ESP8266/ESP32 specific code
   */
  int getRSSI() {
    #if defined(ESP32) || defined(ESP8266)
      // This needs platform-specific implementation
      // For now, return placeholder
      return -60;  // TODO: Get actual RSSI from WiFi
    #else
      return 0;  // Not available on non-WiFi platforms
    #endif
  }
};
```

**Estimated Effort:** 6-8 hours (requires testing on ESP32/ESP8266)

---

## Part 2: Topology Reporter Implementation

### 2.1 Create MeshTopologyReporter Class

**File:** `examples/bridge/mesh_topology_reporter.hpp` (NEW)

**Purpose:** Generate schema-compliant topology messages

```cpp
#ifndef MESH_TOPOLOGY_REPORTER_HPP
#define MESH_TOPOLOGY_REPORTER_HPP

#include "painlessMesh.h"
#include <ArduinoJson.h>

class MeshTopologyReporter {
private:
  painlessMesh& mesh;
  String meshId;
  String lastTopologyHash;  // Detect changes
  
public:
  MeshTopologyReporter(painlessMesh& mesh, const String& meshId) 
    : mesh(mesh), meshId(meshId) {}
  
  /**
   * Generate full topology message (schema v0.5.0 compliant)
   */
  String generateFullTopology() {
    DynamicJsonDocument doc(8192);  // Large buffer for full topology
    
    // Envelope fields (from @alteriom/mqtt-schema)
    doc["schema_version"] = 1;
    doc["device_id"] = String(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = "GW 2.3.4";  // Make configurable
    
    // Event discriminator
    doc["event"] = "mesh_topology";
    
    // Mesh identification
    doc["mesh_id"] = meshId;
    doc["gateway_node_id"] = String(mesh.getNodeId());
    
    // Nodes array
    JsonArray nodes = doc.createNestedArray("nodes");
    addNodeInfo(nodes, mesh.getNodeId(), "gateway");  // Self
    
    auto nodeList = mesh.getNodeList(false);
    for (auto nodeId : nodeList) {
      addNodeInfo(nodes, nodeId, "sensor");  // Assume sensor role
    }
    
    // Connections array
    JsonArray connections = doc.createNestedArray("connections");
    addConnectionInfo(connections);
    
    // Network metrics
    JsonObject metrics = doc.createNestedObject("metrics");
    metrics["total_nodes"] = nodeList.size() + 1;  // +1 for gateway
    metrics["online_nodes"] = nodeList.size() + 1;
    metrics["network_diameter"] = calculateNetworkDiameter();
    metrics["avg_connection_quality"] = calculateAvgQuality();
    metrics["messages_per_second"] = calculateThroughput();
    
    doc["update_type"] = "full";
    
    String output;
    serializeJson(doc, output);
    return output;
  }
  
  /**
   * Generate incremental topology update
   */
  String generateIncrementalUpdate(uint32_t nodeId, bool joined) {
    DynamicJsonDocument doc(2048);
    
    // Envelope fields
    doc["schema_version"] = 1;
    doc["device_id"] = String(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = "GW 2.3.4";
    
    doc["event"] = "mesh_topology";
    doc["mesh_id"] = meshId;
    doc["gateway_node_id"] = String(mesh.getNodeId());
    
    // Node that changed
    JsonArray nodes = doc.createNestedArray("nodes");
    if (joined) {
      addNodeInfo(nodes, nodeId, "sensor");
    } else {
      // For node leaving, mark as offline
      JsonObject node = nodes.createNestedObject();
      node["node_id"] = String(nodeId);
      node["status"] = "offline";
    }
    
    // Update connections
    JsonArray connections = doc.createNestedArray("connections");
    // Add connections related to this node
    
    doc["update_type"] = "incremental";
    
    String output;
    serializeJson(doc, output);
    return output;
  }
  
  /**
   * Check if topology has changed
   */
  bool hasTopologyChanged() {
    String currentHash = calculateTopologyHash();
    if (currentHash != lastTopologyHash) {
      lastTopologyHash = currentHash;
      return true;
    }
    return false;
  }
  
private:
  /**
   * Add node information to JSON array
   */
  void addNodeInfo(JsonArray& nodes, uint32_t nodeId, const char* role) {
    JsonObject node = nodes.createNestedObject();
    node["node_id"] = String(nodeId);
    node["role"] = role;
    node["status"] = "online";
    node["last_seen"] = getISO8601Timestamp();
    
    // If we have connection to this node, add metrics
    // This requires connection metadata API
    node["uptime_seconds"] = 0;  // TODO: Track per-node uptime
    node["free_memory_kb"] = 0;  // TODO: Get from node status messages
    node["connection_count"] = 0;  // TODO: Calculate from mesh structure
    
    // Gateway node has additional info
    if (nodeId == mesh.getNodeId()) {
      node["uptime_seconds"] = millis() / 1000;
      node["free_memory_kb"] = ESP.getFreeHeap() / 1024;
      node["connection_count"] = mesh.getNodeList().size();
    }
  }
  
  /**
   * Add connection information to JSON array
   */
  void addConnectionInfo(JsonArray& connections) {
    // Get direct connections with quality metrics
    // This requires new API: mesh.getConnectionDetails()
    
    auto nodeList = mesh.getNodeList(false);
    uint32_t gatewayId = mesh.getNodeId();
    
    // For now, create basic connection data
    // TODO: Use actual connection quality API
    for (auto nodeId : nodeList) {
      JsonObject conn = connections.createNestedObject();
      conn["from_node"] = String(gatewayId);
      conn["to_node"] = String(nodeId);
      conn["quality"] = 85;  // TODO: Get actual quality
      conn["latency_ms"] = 20;  // TODO: Get actual latency
      conn["rssi"] = -55;  // TODO: Get actual RSSI
      conn["hop_count"] = 1;  // TODO: Calculate hop count
    }
  }
  
  /**
   * Calculate network diameter (max hop count)
   */
  int calculateNetworkDiameter() {
    int maxHops = 0;
    auto nodeList = mesh.getNodeList(false);
    
    for (auto nodeId : nodeList) {
      // TODO: Use mesh.getHopCount(nodeId)
      // For now assume max 2 hops
      maxHops = max(maxHops, 2);
    }
    
    return maxHops;
  }
  
  /**
   * Calculate average connection quality
   */
  float calculateAvgQuality() {
    // TODO: Use connection quality API
    // For now return placeholder
    return 85.0;
  }
  
  /**
   * Calculate network throughput
   */
  float calculateThroughput() {
    // TODO: Track message rates
    // For now return placeholder
    return 12.4;
  }
  
  /**
   * Calculate topology hash for change detection
   */
  String calculateTopologyHash() {
    auto nodeList = mesh.getNodeList(true);
    String hash = String(nodeList.size());
    for (auto nodeId : nodeList) {
      hash += String(nodeId);
    }
    return hash;
  }
  
  /**
   * Get current timestamp in ISO 8601 format
   */
  String getISO8601Timestamp() {
    // TODO: Use proper NTP time if available
    // For now return epoch-based timestamp
    unsigned long ms = millis();
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "2025-10-12T%02lu:%02lu:%02luZ",
             (ms / 3600000) % 24,
             (ms / 60000) % 60,
             (ms / 1000) % 60);
    return String(timestamp);
  }
};

#endif  // MESH_TOPOLOGY_REPORTER_HPP
```

**Estimated Effort:** 8-10 hours (initial implementation + testing)

---

### 2.2 Create MeshEventPublisher Class

**File:** `examples/bridge/mesh_event_publisher.hpp` (NEW)

**Purpose:** Publish real-time mesh events

```cpp
#ifndef MESH_EVENT_PUBLISHER_HPP
#define MESH_EVENT_PUBLISHER_HPP

#include "painlessMesh.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

class MeshEventPublisher {
private:
  painlessMesh& mesh;
  PubSubClient& mqttClient;
  String meshId;
  String eventTopic;
  
public:
  MeshEventPublisher(painlessMesh& mesh, PubSubClient& mqttClient, 
                     const String& meshId, const String& topic = "mesh/events")
    : mesh(mesh), mqttClient(mqttClient), meshId(meshId), eventTopic(topic) {}
  
  /**
   * Publish node join event
   */
  void publishNodeJoin(uint32_t nodeId) {
    DynamicJsonDocument doc(1024);
    
    // Envelope
    doc["schema_version"] = 1;
    doc["device_id"] = String(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = "GW 2.3.4";
    
    // Event
    doc["event"] = "mesh_event";
    doc["event_type"] = "node_join";
    doc["mesh_id"] = meshId;
    
    JsonArray affected = doc.createNestedArray("affected_nodes");
    affected.add(String(nodeId));
    
    JsonObject details = doc.createNestedObject("details");
    details["total_nodes"] = mesh.getNodeList().size() + 1;
    details["timestamp"] = getISO8601Timestamp();
    
    publishEvent(doc);
  }
  
  /**
   * Publish node leave event
   */
  void publishNodeLeave(uint32_t nodeId) {
    DynamicJsonDocument doc(1024);
    
    // Envelope
    doc["schema_version"] = 1;
    doc["device_id"] = String(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = "GW 2.3.4";
    
    // Event
    doc["event"] = "mesh_event";
    doc["event_type"] = "node_leave";
    doc["mesh_id"] = meshId;
    
    JsonArray affected = doc.createNestedArray("affected_nodes");
    affected.add(String(nodeId));
    
    JsonObject details = doc.createNestedObject("details");
    details["reason"] = "connection_lost";
    details["last_seen"] = getISO8601Timestamp();
    details["total_nodes"] = mesh.getNodeList().size() + 1;
    
    publishEvent(doc);
  }
  
  /**
   * Publish connection lost event
   */
  void publishConnectionLost(uint32_t nodeId) {
    DynamicJsonDocument doc(1024);
    
    doc["schema_version"] = 1;
    doc["device_id"] = String(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = "GW 2.3.4";
    
    doc["event"] = "mesh_event";
    doc["event_type"] = "connection_lost";
    doc["mesh_id"] = meshId;
    
    JsonArray affected = doc.createNestedArray("affected_nodes");
    affected.add(String(nodeId));
    
    publishEvent(doc);
  }
  
  /**
   * Publish connection restored event
   */
  void publishConnectionRestored(uint32_t nodeId) {
    DynamicJsonDocument doc(1024);
    
    doc["schema_version"] = 1;
    doc["device_id"] = String(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = "GW 2.3.4";
    
    doc["event"] = "mesh_event";
    doc["event_type"] = "connection_restored";
    doc["mesh_id"] = meshId;
    
    JsonArray affected = doc.createNestedArray("affected_nodes");
    affected.add(String(nodeId));
    
    publishEvent(doc);
  }
  
private:
  void publishEvent(const DynamicJsonDocument& doc) {
    if (!mqttClient.connected()) return;
    
    String payload;
    serializeJson(doc, payload);
    
    mqttClient.publish(eventTopic.c_str(), payload.c_str());
    
    Serial.printf("[Event] Published: %s\n", 
                  doc["event_type"].as<const char*>());
  }
  
  String getISO8601Timestamp() {
    unsigned long ms = millis();
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "2025-10-12T%02lu:%02lu:%02luZ",
             (ms / 3600000) % 24,
             (ms / 60000) % 60,
             (ms / 1000) % 60);
    return String(timestamp);
  }
};

#endif  // MESH_EVENT_PUBLISHER_HPP
```

**Estimated Effort:** 4-6 hours

---

## Part 3: Enhanced Gateway Bridge

### 3.1 Update mqttCommandBridge.ino

**File:** `examples/mqttCommandBridge/mqttCommandBridge.ino`

**Changes Required:**

1. Include new reporters
2. Replace simple topology with schema-compliant topology
3. Add event publisher integration
4. Add topology command handler

```cpp
// Add includes
#include "examples/bridge/mesh_topology_reporter.hpp"
#include "examples/bridge/mesh_event_publisher.hpp"

// Add global instances
MeshTopologyReporter* topologyReporter;
MeshEventPublisher* eventPublisher;

// Update taskTopology
Task taskTopology(TASK_SECOND * 60, TASK_FOREVER, []() {
  if (mqttClient.connected()) {
    // Generate schema-compliant topology
    String topology = topologyReporter->generateFullTopology();
    mqttClient.publish("alteriom/mesh/MESH-001/topology", topology.c_str());
    
    Serial.println("[Topology] Published full mesh topology");
  }
});

// Add incremental topology task
Task taskTopologyIncremental(TASK_SECOND * 5, TASK_FOREVER, []() {
  if (mqttClient.connected() && topologyReporter->hasTopologyChanged()) {
    // Publish incremental update only if topology changed
    // Note: This requires tracking what changed
    Serial.println("[Topology] Topology changed, publishing update");
    
    // For now, publish full topology
    // TODO: Generate incremental updates
    String topology = topologyReporter->generateFullTopology();
    mqttClient.publish("alteriom/mesh/MESH-001/topology", topology.c_str());
  }
});

// Update setup()
void setup() {
  // ... existing setup code ...
  
  // Initialize topology reporter
  topologyReporter = new MeshTopologyReporter(mesh, "MESH-001");
  
  // Initialize event publisher
  eventPublisher = new MeshEventPublisher(mesh, mqttClient, "MESH-001");
  
  // Update mesh callbacks to use event publisher
  mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("New node connected: %u\n", nodeId);
    eventPublisher->publishNodeJoin(nodeId);
    
    // Trigger topology update
    String topology = topologyReporter->generateIncrementalUpdate(nodeId, true);
    mqttClient.publish("alteriom/mesh/MESH-001/topology", topology.c_str());
  });
  
  mesh.onDroppedConnection([](uint32_t nodeId) {
    Serial.printf("Node disconnected: %u\n", nodeId);
    eventPublisher->publishNodeLeave(nodeId);
    
    // Trigger topology update
    String topology = topologyReporter->generateIncrementalUpdate(nodeId, false);
    mqttClient.publish("alteriom/mesh/MESH-001/topology", topology.c_str());
  });
  
  // Add incremental topology task
  userScheduler.addTask(taskTopologyIncremental);
  taskTopologyIncremental.enable();
  
  // ... rest of setup ...
}
```

**Estimated Effort:** 3-4 hours

---

### 3.2 Add Topology Command Handler

**Update:** `examples/bridge/mqtt_command_bridge.hpp`

**Add command 300 (GET_TOPOLOGY):**

```cpp
case 300:  // GET_TOPOLOGY
  Serial.println("Command: GET_TOPOLOGY");
  if (cmd.targetDevice == mesh.getNodeId() || cmd.targetDevice == 0) {
    // Gateway should respond with full topology
    // This requires access to MeshTopologyReporter
    // For now, acknowledge and let scheduled task handle it
    executeCommand(cmd);
  } else {
    // Forward to specific node (may not make sense)
    forwardCommandToMesh(cmd);
  }
  break;
```

**Estimated Effort:** 2-3 hours

---

## Part 4: Testing & Validation

### 4.1 Create Test Sketch

**File:** `examples/mqttTopologyTest/mqttTopologyTest.ino` (NEW)

**Purpose:** Test topology reporting with 3-node mesh

```cpp
// Minimal test setup with 3 ESP32s
// - 1 Gateway (publishes topology)
// - 2 Sensor nodes
// Verify:
// - Full topology published every 60s
// - Incremental updates on node join/leave
// - Events published correctly
// - Schema validation passes
```

**Estimated Effort:** 4-6 hours (requires physical hardware)

---

### 4.2 Schema Validation Tests

**File:** `test/catch/catch_topology_schema.cpp` (NEW)

**Purpose:** Validate generated topology matches schema

```cpp
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
#include "../../examples/bridge/mesh_topology_reporter.hpp"

SCENARIO("Topology message validates against schema") {
  GIVEN("A MeshTopologyReporter") {
    // Test that generated JSON matches schema requirements
    REQUIRE(topology_has_envelope_fields());
    REQUIRE(topology_has_nodes_array());
    REQUIRE(topology_has_connections_array());
    REQUIRE(topology_has_metrics_object());
  }
}
```

**Estimated Effort:** 6-8 hours

---

## Part 5: Documentation Updates

### 5.1 Update MQTT_BRIDGE_COMMANDS.md

Add sections:

- Mesh Topology Reporting
- Mesh Events
- Topology command (300)

**Estimated Effort:** 2-3 hours

---

### 5.2 Create MESH_TOPOLOGY_GUIDE.md

**File:** `docs/MESH_TOPOLOGY_GUIDE.md` (NEW)

**Content:**

- How topology reporting works
- How to visualize with D3.js
- Troubleshooting topology issues
- Performance considerations

**Estimated Effort:** 4-6 hours

---

## Implementation Summary

### Phase 1: Core Library (HIGH PRIORITY)

| Task | File | Effort | Priority |
|------|------|--------|----------|
| Add ConnectionInfo struct | mesh.hpp | 2h | 🔴 HIGH |
| Add getConnectionDetails() | mesh.hpp | 4h | 🔴 HIGH |
| Add getHopCount() | mesh.hpp | 2h | 🔴 HIGH |
| Add getRoutingTable() | mesh.hpp | 2h | 🔴 HIGH |
| Add Connection metrics | tcp.hpp | 6h | 🔴 HIGH |
| **Subtotal** | | **16h** | |

### Phase 2: Topology Reporter (HIGH PRIORITY)

| Task | File | Effort | Priority |
|------|------|--------|----------|
| Create MeshTopologyReporter | NEW | 8h | 🔴 HIGH |
| Create MeshEventPublisher | NEW | 4h | 🔴 HIGH |
| Update mqttCommandBridge.ino | MODIFY | 4h | 🔴 HIGH |
| Add topology command handler | MODIFY | 2h | 🟡 MEDIUM |
| **Subtotal** | | **18h** | |

### Phase 3: Testing (MEDIUM PRIORITY)

| Task | File | Effort | Priority |
|------|------|--------|----------|
| Physical hardware testing | NEW | 6h | 🟡 MEDIUM |
| Schema validation tests | NEW | 8h | 🟡 MEDIUM |
| Integration testing | VARIOUS | 4h | 🟡 MEDIUM |
| **Subtotal** | | **18h** | |

### Phase 4: Documentation (LOW PRIORITY)

| Task | File | Effort | Priority |
|------|------|--------|----------|
| Update MQTT_BRIDGE_COMMANDS.md | MODIFY | 3h | 🟢 LOW |
| Create MESH_TOPOLOGY_GUIDE.md | NEW | 6h | 🟢 LOW |
| Update README examples | MODIFY | 2h | 🟢 LOW |
| **Subtotal** | | **11h** | |

---

## Total Implementation Effort

| Phase | Hours | Priority |
|-------|-------|----------|
| Phase 1: Core Library | 16h | 🔴 HIGH |
| Phase 2: Topology Reporter | 18h | 🔴 HIGH |
| Phase 3: Testing | 18h | 🟡 MEDIUM |
| Phase 4: Documentation | 11h | 🟢 LOW |
| **TOTAL** | **63h** | |

**Estimated Calendar Time:** 2-3 weeks (assuming 1 developer, part-time)

---

## Dependencies

### External Dependencies

1. ✅ **@alteriom/mqtt-schema v0.5.0** - Schema definitions
2. ✅ **ArduinoJson** - Already used
3. ✅ **PubSubClient** - Already used
4. ✅ **TaskScheduler** - Already used

### Internal Dependencies

1. ⚠️ **painlessMesh core API changes** - Breaking changes to Connection class
2. ⚠️ **RSSI tracking** - Platform-specific (ESP32/ESP8266)
3. ⚠️ **NTP time sync** - For accurate ISO 8601 timestamps

---

## Risk Assessment

### High Risk ⚠️

1. **Connection metrics API** - Requires changes to core library
   - Mitigation: Extensive testing, backward compatibility checks

2. **RSSI tracking** - Platform-specific implementation
   - Mitigation: Abstract platform differences, provide defaults

3. **Memory constraints** - Large JSON documents (8KB topology)
   - Mitigation: Use streaming JSON, reduce buffer sizes on ESP8266

### Medium Risk 🟡

1. **Performance impact** - Additional metric tracking overhead
   - Mitigation: Make metrics collection optional, optimize data structures

2. **Breaking changes** - Connection class modifications
   - Mitigation: Version bump to v1.7.0, update changelog

### Low Risk ✅

1. **Schema compliance** - Matching @alteriom/mqtt-schema exactly
   - Mitigation: Automated schema validation tests

---

## Backward Compatibility

### Breaking Changes ⚠️

1. **Connection class** - New public members may break existing code that copies/serializes Connection objects
   - **Solution:** Version bump to v1.7.0, deprecation warnings in v1.6.x

2. **Memory usage** - Increased RAM usage due to metric tracking
   - **Solution:** Make metrics optional via compile-time flag `PAINLESSMESH_ENABLE_METRICS`

### Non-Breaking Changes ✅

1. **New API methods** - additive, doesn't affect existing code
2. **MQTT bridge enhancements** - in examples, not core library
3. **Documentation** - purely additive

---

## Recommended Implementation Order

### Week 1: Foundation

1. ✅ Review and approve schema additions
2. Add ConnectionInfo struct to mesh.hpp
3. Add basic metric tracking to Connection class
4. Implement getConnectionDetails() API

### Week 2: Topology Reporting

1. Create MeshTopologyReporter class
2. Create MeshEventPublisher class
3. Update mqttCommandBridge.ino
4. Basic testing with 2-node mesh

### Week 3: Polish & Testing

1. Add topology command handler
2. Physical hardware testing (3-4 node mesh)
3. Schema validation tests
4. Documentation updates

---

## Success Criteria

✅ **Must Have (v1.7.0):**

- Schema-compliant topology messages
- Real-time mesh events (join/leave)
- Connection quality metrics (RSSI, latency, quality)
- Full and incremental topology updates
- Backward compatible with v1.6.x (no breaking changes to public API)

📋 **Should Have (v1.7.1):**

- Topology command handler (GET_TOPOLOGY)
- Hop count calculation
- Routing table export
- Performance optimization

🎯 **Nice to Have (v1.8.0):**

- Mesh diagnostics command
- Per-message route tracking
- Historical topology storage
- Advanced quality metrics (jitter, throughput)

---

## Next Steps

### Immediate Actions

1. 📝 **Review this plan** - Get team approval
2. 🔧 **Set up development branch** - `feature/mesh-topology-v0.5`
3. 📦 **Create GitHub issues** - Break down tasks
4. 🧪 **Set up test hardware** - 3x ESP32 boards
5. 📚 **Wait for schema release** - Monitor @alteriom/mqtt-schema repo

### Development Kickoff

Once @alteriom/mqtt-schema v0.5.0 is released:

1. Start with Phase 1 (Core Library APIs)
2. Weekly progress check-ins
3. Code reviews for each major component
4. Continuous integration testing

---

**Prepared By:** Alteriom Development Team  
**Date:** October 12, 2025  
**Status:** 📋 READY FOR IMPLEMENTATION  
**Estimated Completion:** 3 weeks from start
