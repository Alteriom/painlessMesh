# Shared Gateway Mode API Reference

> **Version:** v1.9.0+  
> **Status:** Implemented  
> **Author:** AlteriomPainlessMesh Team

This document provides comprehensive API reference for the Shared Gateway Mode feature in painlessMesh.

---

## Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [API Reference](#api-reference)
4. [Configuration](#configuration)
5. [Package Types](#package-types)
6. [Advanced Usage](#advanced-usage)
7. [Troubleshooting](#troubleshooting)
8. [Performance](#performance)
9. [Migration](#migration)

---

## Overview

### What is Shared Gateway Mode?

Shared Gateway Mode is a painlessMesh feature that enables **all nodes** in a mesh network to connect to the same WiFi router while maintaining mesh connectivity. Unlike Bridge Mode where only one dedicated node connects to the router, Shared Gateway Mode allows every node to potentially send data directly to the Internet.

### Architecture Diagram

```
                      Internet
                         │
                      Router (shared SSID)
           ┌─────────────┼─────────────┐
           │             │             │
        Node A        Node B        Node C
        (STA+AP)      (STA+AP)      (STA+AP)
        [Primary]     [Backup]      [Backup]
           │             │             │
           └─────────────┼─────────────┘
                    Mesh Network

Legend:
- STA: Station connection to router (Internet access)
- AP: Access point for mesh network
- [Primary]: Elected primary gateway for relayed messages
- [Backup]: Backup gateway, can relay if primary fails
```

### Key Benefits

| Benefit | Description |
|---------|-------------|
| **Redundant Internet Access** | All nodes can send data to the Internet, eliminating single points of failure |
| **Automatic Failover** | If a node's Internet connection fails, data routes through the mesh to find a working gateway |
| **Simplified Deployment** | No dedicated bridge node required; any node can serve as gateway |
| **Load Distribution** | Internet traffic can be distributed across multiple nodes |
| **Resilient Architecture** | Mesh continues to function even if individual router connections fail |

### Use Cases

- **Fish Farm Monitoring**: Critical O₂ alarms can reach the cloud through any available path
- **Industrial Sensor Networks**: Sensor data reliably uploaded even during partial network outages
- **Smart Building Systems**: HVAC and security data maintains connectivity through redundant paths
- **Remote Monitoring**: Environmental stations with spotty Internet can relay through nearby nodes

### Comparison: Bridge Mode vs Shared Gateway Mode

| Aspect | Bridge Mode | Shared Gateway Mode |
|--------|-------------|---------------------|
| Internet Access | Bridge node only | All nodes |
| Router Connection | Bridge node only | All nodes |
| WiFi Mode | AP+STA (bridge only) | AP+STA (all nodes) |
| Failover Time | 60-70 seconds (election) | Near-instant relay |
| Primary Gateway | Fixed (bridge) | Elected dynamically |
| Duplicate Prevention | Not required | Built-in |
| Single Point of Failure | Yes (bridge node) | No |

---

## Quick Start

### Basic Setup Example

```cpp
#include "painlessMesh.h"

// Mesh network configuration
#define MESH_PREFIX     "SharedGatewayMesh"
#define MESH_PASSWORD   "meshPassword123"
#define MESH_PORT       5555

// Router configuration (all nodes connect to same router)
#define ROUTER_SSID     "YourRouterSSID"
#define ROUTER_PASSWORD "YourRouterPassword"

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
    Serial.begin(115200);
    
    // Set debug level
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    
    // Initialize as Shared Gateway
    bool success = mesh.initAsSharedGateway(
        MESH_PREFIX,      // Mesh network name
        MESH_PASSWORD,    // Mesh network password
        ROUTER_SSID,      // Router SSID
        ROUTER_PASSWORD,  // Router password
        &userScheduler,   // Task scheduler
        MESH_PORT         // Mesh TCP port
    );
    
    if (success) {
        Serial.println("Shared Gateway Mode initialized!");
        Serial.printf("Node ID: %u\n", mesh.getNodeId());
        Serial.printf("Router IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("Failed to initialize - falling back to mesh only");
        mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    }
    
    // Register callbacks
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
}

void loop() {
    mesh.update();
}

void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
}
```

---

## API Reference

### Initialization Methods

#### `initAsSharedGateway()`

Initialize the mesh in Shared Gateway Mode, connecting to both the mesh network and an external WiFi router.

```cpp
bool initAsSharedGateway(
    TSTRING meshPrefix, 
    TSTRING meshPassword,
    TSTRING routerSSID,
    TSTRING routerPassword,
    Scheduler* userScheduler,
    uint16_t port = 5555,
    painlessmesh::gateway::SharedGatewayConfig config = SharedGatewayConfig()
);
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `meshPrefix` | TSTRING | Mesh network name (SSID prefix) |
| `meshPassword` | TSTRING | Mesh network password |
| `routerSSID` | TSTRING | External WiFi router SSID |
| `routerPassword` | TSTRING | External WiFi router password |
| `userScheduler` | Scheduler* | TaskScheduler instance for mesh operations |
| `port` | uint16_t | TCP port for mesh communication (default: 5555) |
| `config` | SharedGatewayConfig | Optional advanced configuration |

**Returns:** `true` if initialization succeeded, `false` otherwise

**Behavior:**

1. Connects to the router to detect its WiFi channel
2. Initializes the mesh network on the same channel
3. Re-establishes the router connection using `stationManual()`
4. Sets up automatic router reconnection monitoring

**Example:**

```cpp
// Basic initialization
bool success = mesh.initAsSharedGateway(
    "MyMesh", "meshPass",
    "HomeRouter", "routerPass",
    &userScheduler
);

// With custom configuration
painlessmesh::gateway::SharedGatewayConfig config;
config.internetCheckInterval = 15000;  // Check every 15 seconds
config.gatewayHeartbeatInterval = 10000;

bool success = mesh.initAsSharedGateway(
    "MyMesh", "meshPass",
    "HomeRouter", "routerPass",
    &userScheduler, 5555, config
);
```

---

### Internet Connectivity Methods

#### `sendToInternet()`

Send data to an Internet destination. Automatically handles failover through the mesh if local Internet is unavailable.

```cpp
uint32_t sendToInternet(
    TSTRING destination,
    TSTRING payload,
    std::function<void(bool success, uint16_t httpStatus, TSTRING error)> callback,
    uint8_t priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_NORMAL)
);
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `destination` | TSTRING | URL or endpoint (e.g., "https://api.example.com/data") |
| `payload` | TSTRING | Data to send (typically JSON) |
| `callback` | function | Callback for delivery result |
| `priority` | uint8_t | Message priority (0=CRITICAL to 3=LOW) |

**Returns:** Unique message ID for tracking

**Priority Levels:**

| Value | Enum | Description |
|-------|------|-------------|
| 0 | `PRIORITY_CRITICAL` | Immediate processing, alarms |
| 1 | `PRIORITY_HIGH` | High priority data |
| 2 | `PRIORITY_NORMAL` | Standard data (default) |
| 3 | `PRIORITY_LOW` | Background/bulk data |

**Example:**

```cpp
// Send sensor data to cloud
String sensorData = "{\"temperature\": 25.5, \"humidity\": 60}";

uint32_t msgId = mesh.sendToInternet(
    "https://api.example.com/sensors",
    sensorData,
    [](bool success, uint16_t httpStatus, String error) {
        if (success) {
            Serial.printf("Delivered! HTTP %d\n", httpStatus);
        } else {
            Serial.printf("Failed: %s\n", error.c_str());
        }
    },
    static_cast<uint8_t>(painlessmesh::gateway::GatewayPriority::PRIORITY_HIGH)
);

Serial.printf("Message queued with ID: %u\n", msgId);
```

---

#### `hasLocalInternet()`

Check if this node has direct Internet connectivity.

```cpp
bool hasLocalInternet();
```

**Returns:** `true` if this node can reach the Internet directly

**Example:**

```cpp
if (mesh.hasLocalInternet()) {
    Serial.println("Local Internet available");
    // Send directly
} else {
    Serial.println("No local Internet - will route through mesh");
}
```

---

#### `getInternetStatus()`

Get detailed information about Internet connectivity status.

```cpp
painlessmesh::gateway::InternetStatus getInternetStatus();
```

**Returns:** `InternetStatus` structure with detailed connectivity information

**InternetStatus Structure:**

| Field | Type | Description |
|-------|------|-------------|
| `available` | bool | Whether Internet is currently available |
| `lastCheckTime` | uint32_t | Timestamp of last check (millis) |
| `lastSuccessTime` | uint32_t | Timestamp of last successful check |
| `checkCount` | uint32_t | Total number of checks performed |
| `successCount` | uint32_t | Number of successful checks |
| `failureCount` | uint32_t | Number of failed checks |
| `lastLatencyMs` | uint32_t | Latency of last successful check |
| `lastError` | TSTRING | Error message from last failed check |
| `checkHost` | TSTRING | Host used for connectivity check |
| `checkPort` | uint16_t | Port used for connectivity check |

**InternetStatus Methods:**

```cpp
// Get uptime percentage (0-100)
uint8_t getUptimePercent() const;

// Get time since last successful check
uint32_t getTimeSinceLastSuccess() const;

// Check if status is stale (no recent check)
bool isStale(uint32_t maxAgeMs = 60000) const;
```

**Example:**

```cpp
auto status = mesh.getInternetStatus();

Serial.printf("Internet: %s\n", status.available ? "Available" : "Unavailable");
Serial.printf("Uptime: %d%%\n", status.getUptimePercent());
Serial.printf("Last latency: %u ms\n", status.lastLatencyMs);
Serial.printf("Checks: %u success / %u total\n", 
              status.successCount, status.checkCount);

if (!status.lastError.empty()) {
    Serial.printf("Last error: %s\n", status.lastError.c_str());
}
```

---

### Gateway Status Methods

#### `isPrimaryGateway()`

Check if this node is the elected primary gateway.

```cpp
bool isPrimaryGateway();
```

**Returns:** `true` if this node is the primary gateway

**Example:**

```cpp
if (mesh.isPrimaryGateway()) {
    Serial.println("This node is the PRIMARY gateway");
    // Handle relayed messages from other nodes
} else {
    Serial.println("This node is a backup gateway");
}
```

---

#### `getPrimaryGateway()`

Get the node ID of the current primary gateway.

```cpp
uint32_t getPrimaryGateway();
```

**Returns:** Node ID of the primary gateway, or 0 if none elected

**Example:**

```cpp
uint32_t primaryId = mesh.getPrimaryGateway();

if (primaryId != 0) {
    Serial.printf("Primary gateway: %u\n", primaryId);
    
    if (primaryId == mesh.getNodeId()) {
        Serial.println("(That's me!)");
    }
} else {
    Serial.println("No primary gateway elected yet");
}
```

---

#### `getGateways()`

Get a list of all nodes with Internet connectivity.

```cpp
std::vector<uint32_t> getGateways();
```

**Returns:** Vector of node IDs that have Internet access

**Example:**

```cpp
auto gateways = mesh.getGateways();

Serial.printf("Available gateways: %d\n", gateways.size());
for (auto nodeId : gateways) {
    Serial.printf("  - Node %u", nodeId);
    if (nodeId == mesh.getPrimaryGateway()) {
        Serial.print(" [PRIMARY]");
    }
    Serial.println();
}
```

---

#### `isSharedGatewayMode()`

Check if the mesh is operating in Shared Gateway Mode.

```cpp
bool isSharedGatewayMode();
```

**Returns:** `true` if initialized with `initAsSharedGateway()`

**Example:**

```cpp
if (mesh.isSharedGatewayMode()) {
    Serial.println("Running in Shared Gateway Mode");
    Serial.printf("Router connected: %s\n", 
                  WiFi.status() == WL_CONNECTED ? "Yes" : "No");
} else {
    Serial.println("Running in standard mesh mode");
}
```

---

### Callbacks

#### `onLocalInternetChanged()`

Register a callback for Internet connectivity changes.

```cpp
void onLocalInternetChanged(std::function<void(bool available)> callback);
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `callback` | function | Function called when Internet status changes |

**Example:**

```cpp
mesh.onLocalInternetChanged([](bool available) {
    if (available) {
        Serial.println("✓ Internet connection restored");
        // Resume direct sending
    } else {
        Serial.println("✗ Internet connection lost");
        // Data will be routed through mesh
    }
});
```

---

#### `onGatewayChanged()`

Register a callback for primary gateway changes.

```cpp
void onGatewayChanged(std::function<void(uint32_t newGatewayId)> callback);
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `callback` | function | Function called when primary gateway changes |

**Example:**

```cpp
mesh.onGatewayChanged([](uint32_t newGatewayId) {
    Serial.printf("Primary gateway changed to: %u\n", newGatewayId);
    
    if (newGatewayId == mesh.getNodeId()) {
        Serial.println("This node is now the primary gateway!");
        // Start handling relayed messages
    }
});
```

---

## Configuration

### SharedGatewayConfig Structure

The `SharedGatewayConfig` structure provides fine-grained control over Shared Gateway Mode behavior.

```cpp
#include "painlessmesh/gateway.hpp"

painlessmesh::gateway::SharedGatewayConfig config;
```

### Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enabled` | bool | false | Enable shared gateway functionality |
| `routerSSID` | TSTRING | "" | Router SSID (set automatically by initAsSharedGateway) |
| `routerPassword` | TSTRING | "" | Router password |
| `internetCheckInterval` | uint32_t | 30000 | Interval between Internet checks (ms) |
| `internetCheckHost` | TSTRING | "8.8.8.8" | Host to ping for connectivity check |
| `internetCheckPort` | uint16_t | 53 | Port for connectivity check |
| `internetCheckTimeout` | uint32_t | 5000 | Timeout for connectivity check (ms) |
| `messageRetryCount` | uint8_t | 3 | Retries before failover |
| `retryInterval` | uint32_t | 1000 | Interval between retries (ms) |
| `duplicateTrackingTimeout` | uint32_t | 60000 | How long to track message IDs (ms) |
| `maxTrackedMessages` | uint16_t | 500 | Maximum messages to track |
| `gatewayHeartbeatInterval` | uint32_t | 15000 | Primary gateway heartbeat interval (ms) |
| `gatewayFailureTimeout` | uint32_t | 45000 | Time to consider gateway failed (ms) |
| `participateInElection` | bool | true | Whether node can become primary |
| `relayedMessagePriority` | uint8_t | 0 | Priority for relayed messages (0=highest) |
| `maintainPermanentConnection` | bool | true | Keep router connection active |

### Configuration Examples

#### Low-Latency Configuration (Fast Failover)

```cpp
painlessmesh::gateway::SharedGatewayConfig config;
config.internetCheckInterval = 10000;      // Check every 10 seconds
config.internetCheckTimeout = 2000;        // 2 second timeout
config.gatewayHeartbeatInterval = 5000;    // Heartbeat every 5 seconds
config.gatewayFailureTimeout = 15000;      // Fail after 15 seconds

mesh.initAsSharedGateway("Mesh", "pass", "Router", "pass", 
                         &userScheduler, 5555, config);
```

#### Memory-Constrained Configuration (ESP8266)

```cpp
painlessmesh::gateway::SharedGatewayConfig config;
config.maxTrackedMessages = 100;           // Limit tracked messages
config.duplicateTrackingTimeout = 30000;   // Shorter tracking window
config.internetCheckInterval = 60000;      // Less frequent checks

mesh.initAsSharedGateway("Mesh", "pass", "Router", "pass", 
                         &userScheduler, 5555, config);
```

#### Non-Participating Node (Backup Only)

```cpp
painlessmesh::gateway::SharedGatewayConfig config;
config.participateInElection = false;      // Never become primary

mesh.initAsSharedGateway("Mesh", "pass", "Router", "pass", 
                         &userScheduler, 5555, config);
```

### Configuration Validation

Validate configuration before use:

```cpp
painlessmesh::gateway::SharedGatewayConfig config;
config.enabled = true;
config.routerSSID = "MyRouter";
config.internetCheckInterval = 500;  // Too low!

auto result = config.validate();
if (!result.valid) {
    Serial.printf("Config error: %s\n", result.errorMessage.c_str());
    // Output: "internetCheckInterval must be at least 1000ms"
}
```

---

## Package Types

Shared Gateway Mode uses three internal package types for coordination. These are automatically handled by the mesh but are documented here for advanced users implementing custom handlers.

### GatewayDataPackage (Type ID: 620)

Used to route Internet-bound data through the mesh when local Internet is unavailable.

```cpp
class GatewayDataPackage : public plugin::SinglePackage {
public:
    uint32_t messageId;      // Unique message identifier
    uint32_t originNode;     // Node that created the message
    uint32_t timestamp;      // Creation timestamp (mesh time)
    uint8_t priority;        // 0=CRITICAL, 1=HIGH, 2=NORMAL, 3=LOW
    TSTRING destination;     // URL/endpoint
    TSTRING payload;         // Application data
    TSTRING contentType;     // MIME type (default: "application/json")
    uint8_t retryCount;      // Number of relay attempts
    bool requiresAck;        // Whether to send acknowledgment
};
```

**Message ID Generation:**

```cpp
// Generate unique message ID
uint32_t msgId = GatewayDataPackage::generateMessageId(mesh.getNodeId());
```

**Manual Package Creation:**

```cpp
painlessmesh::gateway::GatewayDataPackage pkg;
pkg.messageId = GatewayDataPackage::generateMessageId(mesh.getNodeId());
pkg.originNode = mesh.getNodeId();
pkg.timestamp = mesh.getNodeTime();
pkg.priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_HIGH);
pkg.destination = "https://api.example.com/data";
pkg.payload = "{\"sensor\": 42}";
pkg.contentType = "application/json";
pkg.requiresAck = true;
pkg.dest = primaryGatewayId;
pkg.from = mesh.getNodeId();

mesh.sendPackage(&pkg);
```

---

### GatewayAckPackage (Type ID: 621)

Sent by the gateway back to the origin node to confirm delivery status.

```cpp
class GatewayAckPackage : public plugin::SinglePackage {
public:
    uint32_t messageId;      // Message ID being acknowledged
    uint32_t originNode;     // Original sender node
    bool success;            // Whether delivery succeeded
    uint16_t httpStatus;     // HTTP status code (0 if N/A)
    TSTRING error;           // Error message if failed
    uint32_t timestamp;      // Acknowledgment timestamp
};
```

**Handling Acknowledgments:**

```cpp
mesh.onPackage(protocol::GATEWAY_ACK, [](protocol::Variant& variant) {
    auto ack = variant.to<painlessmesh::gateway::GatewayAckPackage>();
    
    Serial.printf("Ack for message %u: %s\n", 
                  ack.messageId, 
                  ack.success ? "Success" : "Failed");
    
    if (!ack.success) {
        Serial.printf("Error: %s\n", ack.error.c_str());
    } else {
        Serial.printf("HTTP Status: %d\n", ack.httpStatus);
    }
    
    return false;  // Don't stop propagation
});
```

---

### GatewayHeartbeatPackage (Type ID: 622)

Broadcast periodically by the primary gateway to inform all nodes of its status.

```cpp
class GatewayHeartbeatPackage : public plugin::BroadcastPackage {
public:
    bool isPrimary;          // Is this the primary gateway?
    bool hasInternet;        // Internet currently available?
    int8_t routerRSSI;       // Router signal strength (dBm)
    uint32_t uptime;         // Gateway uptime (seconds)
    uint32_t timestamp;      // Heartbeat timestamp
};
```

**Helper Methods:**

```cpp
// Check if gateway is healthy
bool isHealthy() const;  // Returns isPrimary && hasInternet

// Check if signal strength is acceptable (> -70 dBm)
bool hasAcceptableSignal() const;
```

**Monitoring Heartbeats:**

```cpp
mesh.onPackage(protocol::GATEWAY_HEARTBEAT, [](protocol::Variant& variant) {
    auto heartbeat = variant.to<painlessmesh::gateway::GatewayHeartbeatPackage>();
    
    Serial.printf("Gateway %u heartbeat:\n", heartbeat.from);
    Serial.printf("  Primary: %s\n", heartbeat.isPrimary ? "Yes" : "No");
    Serial.printf("  Internet: %s\n", heartbeat.hasInternet ? "Yes" : "No");
    Serial.printf("  RSSI: %d dBm\n", heartbeat.routerRSSI);
    Serial.printf("  Uptime: %u seconds\n", heartbeat.uptime);
    Serial.printf("  Healthy: %s\n", heartbeat.isHealthy() ? "Yes" : "No");
    
    return false;
});
```

---

## Advanced Usage

### Failover Scenarios

#### Scenario 1: Local Internet Failure

```
┌────────┐                           ┌─────────┐         ┌────────────┐
│ Node A │                           │ Node B  │         │  Internet  │
│(Sender)│                           │(Gateway)│         │  Service   │
└───┬────┘                           └────┬────┘         └─────┬──────┘
    │                                     │                    │
    │ 1. sendToInternet() - local fails   │                    │
    │                                     │                    │
    │ 2. GatewayDataPackage (Type 620)    │                    │
    │────────────────────────────────────→│                    │
    │                                     │ 3. HTTP POST       │
    │                                     │───────────────────→│
    │                                     │                    │
    │                                     │ 4. HTTP 200 OK     │
    │                                     │←───────────────────│
    │                                     │                    │
    │ 5. GatewayAckPackage (Type 621)     │                    │
    │←────────────────────────────────────│                    │
    │                                     │                    │
    │ 6. callback(true, 200, "")          │                    │
```

#### Scenario 2: Primary Gateway Failure

```
Timeline:
─────────────────────────────────────────────────────────────────────
0s     Primary Gateway (Node A) broadcasting heartbeats
       └─→ All nodes track Node A as primary

15s    Node A sends heartbeat (normal)

30s    Node A crashes! No more heartbeats...

45s    Nodes detect missing heartbeat (gatewayFailureTimeout)
       └─→ Election triggered

50s    Node B wins election (highest RSSI)
       └─→ Node B broadcasts as new primary

55s    All nodes acknowledge Node B as primary
       └─→ Normal operation resumes
```

### Election Protocol

The gateway election uses a deterministic algorithm to ensure consistent winner selection across all nodes:

1. **Eligibility**: Only nodes with Internet connectivity can participate
2. **Selection Criteria**:
   - **Highest RSSI wins** (better router signal)
   - **If RSSI tie, highest Node ID wins** (deterministic tiebreaker)
3. **Split-Brain Prevention**: If multiple nodes claim primary, nodes defer to the one with higher priority

**Election State Machine:**

```
                ┌─────────────────────────────────────┐
                │                IDLE                 │
                │   (monitoring primary heartbeats)   │
                └─────────────────┬───────────────────┘
                                  │
                    Primary timeout detected
                                  │
                                  ▼
                ┌─────────────────────────────────────┐
                │         ELECTION_RUNNING            │
                │  (collecting candidates, 5 seconds) │
                └─────────────────┬───────────────────┘
                                  │
                    Election duration elapsed
                                  │
                                  ▼
                ┌─────────────────────────────────────┐
                │            COOLDOWN                 │
                │  (preventing rapid re-elections)    │
                └─────────────────┬───────────────────┘
                                  │
                    Cooldown period elapsed
                                  │
                                  ▼
                              [IDLE]
```

### Custom Message Handling

Register handlers for gateway packages:

```cpp
// Handle relayed data (as gateway)
mesh.onPackage(protocol::GATEWAY_DATA, [](protocol::Variant& variant) {
    auto pkg = variant.to<painlessmesh::gateway::GatewayDataPackage>();
    
    Serial.printf("Relayed request from %u to %s\n", 
                  pkg.originNode, pkg.destination.c_str());
    
    // Custom handling (e.g., add authentication)
    // The default handler will forward to Internet
    
    return false;  // Let default handler process
});

// Monitor gateway changes
mesh.onPackage(protocol::GATEWAY_HEARTBEAT, [](protocol::Variant& variant) {
    auto hb = variant.to<painlessmesh::gateway::GatewayHeartbeatPackage>();
    
    if (hb.isPrimary && !hb.hasInternet) {
        Serial.println("Warning: Primary gateway lost Internet!");
    }
    
    return false;
});
```

### Message Tracking and Deduplication

The `GatewayMessageHandler` class prevents duplicate message processing:

```cpp
painlessmesh::GatewayMessageHandler handler;

// Configure from SharedGatewayConfig
handler.configure(config);

// Handle incoming message
bool shouldProcess = handler.handleIncomingMessage(pkg);
if (!shouldProcess) {
    Serial.println("Duplicate message dropped");
    return;
}

// Check if we should send acknowledgment
if (pkg.requiresAck && handler.shouldSendAcknowledgment(pkg.messageId, pkg.originNode)) {
    sendAck(pkg);
    handler.markAcknowledgmentSent(pkg.messageId, pkg.originNode);
}

// Get metrics
auto metrics = handler.getMetrics();
Serial.printf("Processed: %u, Duplicates: %u (%d%%)\n",
              metrics.messagesProcessed,
              metrics.duplicatesDetected,
              metrics.getDuplicateRate());
```

---

## Troubleshooting

### Common Issues

#### Issue: Node fails to connect to router

**Symptoms:**
- `initAsSharedGateway()` returns false
- `WiFi.status()` != `WL_CONNECTED`

**Causes:**
- Incorrect router credentials
- Router out of range
- Channel conflict

**Solutions:**

```cpp
bool success = mesh.initAsSharedGateway(...);

if (!success) {
    Serial.println("Shared Gateway init failed!");
    Serial.printf("WiFi Status: %d\n", WiFi.status());
    
    // Check WiFi status codes:
    // WL_IDLE_STATUS = 0
    // WL_NO_SSID_AVAIL = 1
    // WL_SCAN_COMPLETED = 2
    // WL_CONNECTED = 3
    // WL_CONNECT_FAILED = 4
    // WL_CONNECTION_LOST = 5
    // WL_DISCONNECTED = 6
    
    // Fallback to mesh-only mode
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}
```

---

#### Issue: Internet health check always fails

**Symptoms:**
- `hasLocalInternet()` returns false despite router connection
- `getInternetStatus().lastError` shows "Connection refused"

**Causes:**
- Firewall blocking outbound connections
- DNS not working
- Check host unreachable

**Solutions:**

```cpp
// Try alternate check hosts
painlessmesh::gateway::SharedGatewayConfig config;
config.internetCheckHost = "1.1.1.1";  // Cloudflare DNS
config.internetCheckPort = 53;

// Or use HTTP endpoint
config.internetCheckHost = "www.google.com";
config.internetCheckPort = 80;

// Increase timeout
config.internetCheckTimeout = 10000;
```

---

#### Issue: Frequent gateway elections

**Symptoms:**
- `onGatewayChanged()` fires frequently
- Logs show repeated election cycles

**Causes:**
- Heartbeat interval too short
- Network congestion
- Unstable Internet connection

**Solutions:**

```cpp
// Increase heartbeat interval and failure timeout
painlessmesh::gateway::SharedGatewayConfig config;
config.gatewayHeartbeatInterval = 30000;   // 30 seconds
config.gatewayFailureTimeout = 90000;      // 90 seconds

// Or disable election participation for unstable nodes
config.participateInElection = false;
```

---

#### Issue: Duplicate messages delivered

**Symptoms:**
- Same data appears multiple times
- `GatewayMetrics.duplicatesDetected` is 0 but duplicates exist

**Causes:**
- `duplicateTrackingTimeout` too short
- `maxTrackedMessages` limit reached
- Messages not using `messageId`

**Solutions:**

```cpp
// Increase tracking capacity
painlessmesh::gateway::SharedGatewayConfig config;
config.duplicateTrackingTimeout = 120000;  // 2 minutes
config.maxTrackedMessages = 1000;

// Ensure messages have unique IDs
pkg.messageId = GatewayDataPackage::generateMessageId(mesh.getNodeId());
```

---

#### Issue: High memory usage on ESP8266

**Symptoms:**
- `ESP.getFreeHeap()` drops significantly
- Crashes or reboots

**Solutions:**

```cpp
// Memory-optimized configuration
painlessmesh::gateway::SharedGatewayConfig config;
config.maxTrackedMessages = 50;            // Minimum viable
config.duplicateTrackingTimeout = 20000;   // Short window
config.internetCheckInterval = 60000;      // Less frequent

// Monitor memory
Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
Serial.printf("Tracked messages: %u\n", handler.getTrackedMessageCount());

// Manual cleanup
handler.cleanup();  // Remove expired entries
```

---

### Debug Logging

Enable detailed logging for troubleshooting:

```cpp
// Enable all debug types
mesh.setDebugMsgTypes(ERROR | STARTUP | MESH_STATUS | CONNECTION | 
                      SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);

// Gateway-specific logging happens automatically through Log() calls
// Look for:
// - "GatewayElectionManager:" - Election events
// - "GatewayMessageHandler:" - Message processing
// - "InternetHealthChecker:" - Connectivity checks
```

---

## Performance

### Memory Footprint

#### SharedGatewayConfig

| Platform | Minimum | With Typical Content |
|----------|---------|---------------------|
| ESP8266 | ~80 bytes | ~150-250 bytes |
| ESP32 | ~80 bytes | ~150-250 bytes |
| PC/Test | ~140 bytes | ~200-300 bytes |

#### GatewayDataPackage

| Platform | Base | With 1KB Payload |
|----------|------|------------------|
| ESP8266 | ~74 bytes | ~1100 bytes |
| ESP32 | ~74 bytes | ~1100 bytes |

#### MessageTracker (per message)

| Platform | Per Entry | 500 Messages |
|----------|-----------|--------------|
| ESP8266 | ~40 bytes | ~20 KB |
| ESP32 | ~40 bytes | ~20 KB |

### Recommendations by Platform

#### ESP8266 (~80KB RAM)

```cpp
// Conservative settings
config.maxTrackedMessages = 100;           // ~4KB for tracker
config.duplicateTrackingTimeout = 30000;   // 30 second window
config.internetCheckInterval = 60000;      // Check every minute

// Keep payloads under 1KB
// Leave 40KB+ for application
```

#### ESP32 (~320KB RAM)

```cpp
// Standard settings work well
config.maxTrackedMessages = 500;           // ~20KB for tracker
config.duplicateTrackingTimeout = 60000;   // 60 second window
config.internetCheckInterval = 30000;      // Check every 30 seconds

// Can handle larger payloads (up to 4KB)
// Leave 200KB+ for application
```

### Bandwidth Considerations

| Traffic Type | Interval | Size | Impact |
|--------------|----------|------|--------|
| Gateway Heartbeat | 15s | ~100 bytes | Low |
| Internet Check | 30s | N/A (local TCP) | None |
| GatewayDataPackage | Varies | 200-2000 bytes | Medium |
| GatewayAckPackage | Per message | ~80 bytes | Low |

---

## Migration

### From Bridge Mode to Shared Gateway Mode

#### Step 1: Update Dependencies

Ensure you have painlessMesh v1.9.0 or later.

#### Step 2: Modify Initialization

**Before (Bridge Mode):**

```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
mesh.stationManual(ROUTER_SSID, ROUTER_PASSWORD);
```

**After (Shared Gateway Mode):**

```cpp
mesh.initAsSharedGateway(
    MESH_PREFIX, MESH_PASSWORD,
    ROUTER_SSID, ROUTER_PASSWORD,
    &userScheduler, MESH_PORT
);
```

#### Step 3: Update Internet Sending

**Before:**

```cpp
// Only bridge node could send to Internet
if (mesh.isBridge()) {
    httpClient.POST(url, data);
}
```

**After:**

```cpp
// Any node can send to Internet with automatic failover
mesh.sendToInternet(url, data, [](bool success, uint16_t status, String error) {
    // Handle result
});
```

#### Step 4: Handle Gateway Events

```cpp
// Monitor gateway changes
mesh.onGatewayChanged([](uint32_t newGateway) {
    Serial.printf("New primary gateway: %u\n", newGateway);
});

// Monitor Internet connectivity
mesh.onLocalInternetChanged([](bool available) {
    Serial.printf("Internet: %s\n", available ? "Available" : "Lost");
});
```

#### Step 5: Update All Nodes

**Important:** All nodes in the mesh should be updated to use Shared Gateway Mode for full functionality. Mixed-mode networks (some bridge, some shared gateway) are not recommended.

### Configuration Migration

| Bridge Mode Setting | Shared Gateway Equivalent |
|---------------------|--------------------------|
| `stationManual()` | `routerSSID`, `routerPassword` in config |
| Bridge election | `participateInElection`, `gatewayFailureTimeout` |
| N/A | `internetCheckInterval`, `duplicateTrackingTimeout` |

---

## Related Documentation

- [Design Document](../design/SHARED_GATEWAY_DESIGN.md) - Full technical design
- [Implementation Plan](../design/SHARED_GATEWAY_IMPLEMENTATION_PLAN.md) - Development roadmap
- [Core API Reference](core-api.md) - Base painlessMesh API
- [Example Code](../../examples/sharedGateway/sharedGateway.ino) - Working example

---

## Changelog

| Version | Changes |
|---------|---------|
| v1.9.0 | Initial release of Shared Gateway Mode |

---

*Last updated: November 2025*
