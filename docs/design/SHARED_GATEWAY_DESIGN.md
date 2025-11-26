# Shared Gateway Mode - Design Document

> **Status:** Proposed  
> **Version:** 1.0.0  
> **Author:** AlteriomPainlessMesh Team  
> **Date:** November 2025  
> **Target Release:** v1.9.0+

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Problem Statement](#problem-statement)
3. [Proposed Solution Architecture](#proposed-solution-architecture)
4. [Technical Design](#technical-design)
5. [Message Flow Diagrams](#message-flow-diagrams)
6. [Configuration Options](#configuration-options)
7. [Memory and Performance Considerations](#memory-and-performance-considerations)
8. [Implementation Phases](#implementation-phases)
9. [Testing Strategy](#testing-strategy)
10. [Risks and Mitigations](#risks-and-mitigations)
11. [Future Enhancements](#future-enhancements)
12. [Open Questions](#open-questions)

---

## Executive Summary

**Shared Gateway Mode** is a proposed painlessMesh feature that enables all nodes in a mesh network to connect to the same WiFi router while maintaining mesh connectivity. Unlike the current architecture where only dedicated bridge nodes can access the Internet, Shared Gateway Mode allows every node to send data directly to the Internet through their own router connection.

### Key Benefits

- **Redundant Internet Access**: All nodes can send data to the Internet, eliminating single points of failure
- **Automatic Failover**: If a node's Internet connection fails, data routes through the mesh to find a working gateway
- **Simplified Deployment**: No dedicated bridge node required; any node can serve as gateway
- **Load Distribution**: Internet traffic can be distributed across multiple nodes
- **Resilient Architecture**: Mesh continues to function even if individual router connections fail

### Use Cases

- **Fish Farm Monitoring**: Critical O2 alarms can reach the cloud through any available path
- **Industrial Sensor Networks**: Sensor data reliably uploaded even during partial network outages
- **Smart Building Systems**: HVAC and security data maintains connectivity through redundant paths
- **Remote Monitoring**: Environmental stations with spotty Internet can relay through nearby nodes

---

## Problem Statement

### Current Architecture Limitations

In the current painlessMesh architecture:

1. **Single Bridge Dependency**: Only nodes initialized with `initAsBridge()` can connect to the Internet
2. **Single Point of Failure**: If the bridge node fails or loses Internet, the entire mesh loses cloud connectivity
3. **Complex Failover**: Bridge failover requires election protocol and significant reconfiguration time (60-70 seconds)

```
Current Architecture:
                     Internet
                        │
                     Router
                        │
               ┌────────┴────────┐
               │  Bridge Node    │ ← Only Internet-capable node
               │  (initAsBridge) │
               └────────┬────────┘
                        │
          ┌─────────────┼─────────────┐
          │             │             │
       Node A        Node B        Node C
      (mesh only)   (mesh only)   (mesh only)
```

### New Use Case: Shared WiFi Channel

Many deployments have all mesh nodes within range of the same WiFi router:

- All nodes connect to the same SSID (e.g., "Factory_WiFi")
- Each node can independently reach the router
- Each node could potentially send data directly to the Internet
- **Challenge**: When a node's direct Internet path fails, it needs failover to mesh routing

```
New Use Case - Shared Gateway Mode:
                     Internet
                        │
                     Router (shared SSID)
          ┌─────────────┼─────────────┐
          │             │             │
       Node A        Node B        Node C
       (STA+AP)      (STA+AP)      (STA+AP)
          │             │             │
          └─────────────┼─────────────┘
                   Mesh Network
```

### Key Challenges

1. **Duplicate Message Prevention**: When Node A's Internet fails and data routes through Node B, we must prevent Node A from also trying to send directly once its connection recovers
2. **Gateway Election**: Need to designate a "primary" gateway to handle relayed messages and prevent multiple nodes from sending the same data
3. **Internet Health Detection**: Each node must monitor its own Internet connectivity
4. **Message Tracking**: Track which messages have been sent to prevent duplicates
5. **Seamless Failover**: Minimize data loss during Internet connectivity transitions

---

## Proposed Solution Architecture

### Overview

Shared Gateway Mode introduces a new initialization mode where all nodes:

1. Connect to the same router as STA (station mode)
2. Participate in mesh as AP (access point mode)
3. Can send data directly to Internet when their connection is available
4. Fall back to mesh routing when their local Internet fails
5. Coordinate through a primary gateway for relayed messages

### Architecture Diagram

```
                        Internet
                           │
                        Router
                   ┌───────┴───────┐
          WiFi STA │               │ WiFi STA
     ┌─────────────┴───────────────┴─────────────┐
     │                                           │
  Node A ←────── Mesh (WiFi AP) ──────→ Node B
  [Primary]                              [Backup]
     │                                     │
     └──────────── Mesh ──────────────→ Node C
                                       [Backup]

Legend:
- WiFi STA: Station connection to router (Internet access)
- WiFi AP: Access point for mesh network
- [Primary]: Elected primary gateway for relayed messages
- [Backup]: Backup gateway, can relay if primary fails
```

### Mode Comparison

| Aspect | Normal Mode | Bridge Mode | Shared Gateway Mode |
|--------|-------------|-------------|---------------------|
| Internet Access | None | Bridge only | All nodes |
| Router Connection | None | Bridge only | All nodes |
| WiFi Mode | AP only | AP+STA (bridge) | AP+STA (all) |
| Failover | None | Election (60-70s) | Instant relay |
| Primary Gateway | N/A | Bridge | Elected |
| Duplicate Prevention | N/A | N/A | Required |

---

## Technical Design

### New Data Structures

#### SharedGatewayConfig Struct

```cpp
/**
 * @brief Configuration for Shared Gateway Mode
 */
struct SharedGatewayConfig {
    // Enable/disable shared gateway mode
    bool enabled = false;
    
    // Router credentials
    TSTRING routerSSID = "";
    TSTRING routerPassword = "";
    
    // Internet connectivity check
    uint32_t internetCheckInterval = 30000;  // Check every 30 seconds
    TSTRING internetCheckHost = "8.8.8.8";   // Google DNS by default
    uint16_t internetCheckPort = 53;         // DNS port
    uint32_t internetCheckTimeout = 5000;    // 5 second timeout
    
    // Failover configuration
    uint8_t messageRetryCount = 3;           // Retries before failover
    uint32_t retryInterval = 1000;           // 1 second between retries
    
    // Duplicate prevention
    uint32_t duplicateTrackingTimeout = 60000; // Track message IDs for 60s
    uint16_t maxTrackedMessages = 500;         // Maximum tracked messages
    
    // Gateway election
    uint32_t gatewayHeartbeatInterval = 15000; // Primary gateway heartbeat
    uint32_t gatewayFailureTimeout = 45000;    // Consider gateway failed after 45s
    bool participateInElection = true;         // Whether this node can be primary
};
```

#### GatewayDataPackage

```cpp
/**
 * @brief Package for routing application data through mesh to gateway
 *
 * When a node's Internet connection fails, it wraps its data in this package
 * and routes it through the mesh to the primary gateway for Internet delivery.
 *
 * Type ID 620 for GATEWAY_DATA
 */
class GatewayDataPackage : public painlessmesh::plugin::SinglePackage {
public:
    uint32_t messageId = 0;       // Unique message identifier
    uint32_t originNode = 0;      // Node that originated the message
    uint32_t timestamp = 0;       // When message was created
    uint8_t priority = 1;         // 0=LOW, 1=NORMAL, 2=HIGH, 3=CRITICAL
    TSTRING destination = "";     // Internet destination (URL/endpoint)
    TSTRING payload = "";         // Application data payload
    TSTRING contentType = "";     // MIME type (application/json, etc.)
    uint8_t retryCount = 0;       // Number of relay attempts
    bool requiresAck = true;      // Whether to send acknowledgment
    
    // MQTT Schema message_type
    uint16_t messageType = 620;   // GATEWAY_DATA
    
    GatewayDataPackage() : SinglePackage(620) {}
    
    GatewayDataPackage(JsonObject jsonObj) : SinglePackage(jsonObj) {
        messageId = jsonObj["msgId"];
        originNode = jsonObj["origin"];
        timestamp = jsonObj["ts"];
        priority = jsonObj["priority"] | 1;
        destination = jsonObj["dest"].as<TSTRING>();
        payload = jsonObj["data"].as<TSTRING>();
        contentType = jsonObj["contentType"].as<TSTRING>();
        retryCount = jsonObj["retry"] | 0;
        requiresAck = jsonObj["ack"] | true;
        messageType = jsonObj["message_type"] | 620;
    }
    
    JsonObject addTo(JsonObject&& jsonObj) const {
        jsonObj = SinglePackage::addTo(std::move(jsonObj));
        jsonObj["msgId"] = messageId;
        jsonObj["origin"] = originNode;
        jsonObj["ts"] = timestamp;
        jsonObj["priority"] = priority;
        jsonObj["dest"] = destination;
        jsonObj["data"] = payload;
        jsonObj["contentType"] = contentType;
        jsonObj["retry"] = retryCount;
        jsonObj["ack"] = requiresAck;
        jsonObj["message_type"] = messageType;
        return jsonObj;
    }
    
#if ARDUINOJSON_VERSION_MAJOR < 7
    size_t jsonObjectSize() const {
        return JSON_OBJECT_SIZE(noJsonFields + 10) + 
               destination.length() + payload.length() + contentType.length();
    }
#endif
};
```

#### GatewayAckPackage

```cpp
/**
 * @brief Acknowledgment package for confirmed Internet delivery
 *
 * Sent by the gateway back to the origin node to confirm that data
 * was successfully sent to the Internet.
 *
 * Type ID 621 for GATEWAY_ACK
 */
class GatewayAckPackage : public painlessmesh::plugin::SinglePackage {
public:
    uint32_t messageId = 0;       // Message ID being acknowledged
    uint32_t originNode = 0;      // Original sender
    bool success = false;         // Whether delivery succeeded
    uint16_t httpStatus = 0;      // HTTP status code (if applicable)
    TSTRING errorMessage = "";    // Error message if failed
    uint32_t timestamp = 0;       // When ack was sent
    
    // MQTT Schema message_type
    uint16_t messageType = 621;   // GATEWAY_ACK
    
    GatewayAckPackage() : SinglePackage(621) {}
    
    GatewayAckPackage(JsonObject jsonObj) : SinglePackage(jsonObj) {
        messageId = jsonObj["msgId"];
        originNode = jsonObj["origin"];
        success = jsonObj["success"] | false;
        httpStatus = jsonObj["http"] | 0;
        errorMessage = jsonObj["error"].as<TSTRING>();
        timestamp = jsonObj["ts"];
        messageType = jsonObj["message_type"] | 621;
    }
    
    JsonObject addTo(JsonObject&& jsonObj) const {
        jsonObj = SinglePackage::addTo(std::move(jsonObj));
        jsonObj["msgId"] = messageId;
        jsonObj["origin"] = originNode;
        jsonObj["success"] = success;
        jsonObj["http"] = httpStatus;
        jsonObj["error"] = errorMessage;
        jsonObj["ts"] = timestamp;
        jsonObj["message_type"] = messageType;
        return jsonObj;
    }
    
#if ARDUINOJSON_VERSION_MAJOR < 7
    size_t jsonObjectSize() const {
        return JSON_OBJECT_SIZE(noJsonFields + 7) + errorMessage.length();
    }
#endif
};
```

#### GatewayHeartbeatPackage

```cpp
/**
 * @brief Heartbeat package from primary gateway
 *
 * The primary gateway broadcasts this periodically to inform all nodes
 * of its status and availability for handling relayed messages.
 *
 * Type ID 622 for GATEWAY_HEARTBEAT
 */
class GatewayHeartbeatPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    bool isPrimary = false;        // Is this the primary gateway?
    bool internetAvailable = true; // Is Internet currently accessible?
    int8_t routerRSSI = 0;         // Router signal strength
    uint32_t uptime = 0;           // Gateway uptime
    uint32_t messagesRelayed = 0;  // Total messages relayed
    uint16_t queueSize = 0;        // Current relay queue size
    uint32_t timestamp = 0;        // Heartbeat timestamp
    
    // MQTT Schema message_type
    uint16_t messageType = 622;    // GATEWAY_HEARTBEAT
    
    GatewayHeartbeatPackage() : BroadcastPackage(622) {}
    
    GatewayHeartbeatPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
        isPrimary = jsonObj["primary"] | false;
        internetAvailable = jsonObj["internet"] | true;
        routerRSSI = jsonObj["rssi"] | 0;
        uptime = jsonObj["uptime"] | 0;
        messagesRelayed = jsonObj["relayed"] | 0;
        queueSize = jsonObj["queue"] | 0;
        timestamp = jsonObj["ts"] | 0;
        messageType = jsonObj["message_type"] | 622;
    }
    
    JsonObject addTo(JsonObject&& jsonObj) const {
        jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
        jsonObj["primary"] = isPrimary;
        jsonObj["internet"] = internetAvailable;
        jsonObj["rssi"] = routerRSSI;
        jsonObj["uptime"] = uptime;
        jsonObj["relayed"] = messagesRelayed;
        jsonObj["queue"] = queueSize;
        jsonObj["ts"] = timestamp;
        jsonObj["message_type"] = messageType;
        return jsonObj;
    }
    
#if ARDUINOJSON_VERSION_MAJOR < 7
    size_t jsonObjectSize() const {
        return JSON_OBJECT_SIZE(noJsonFields + 8);
    }
#endif
};
```

### Message Tracking for Duplicate Prevention

```cpp
/**
 * @brief Tracks processed messages to prevent duplicates
 */
struct TrackedMessage {
    uint32_t messageId;           // Unique message ID
    uint32_t originNode;          // Node that created the message
    uint32_t timestamp;           // When message was first seen
    bool sentToInternet;          // Whether we sent it to Internet
    bool relayedToGateway;        // Whether we relayed through mesh
    bool ackReceived;             // Whether we got acknowledgment
};

class MessageTracker {
private:
    std::list<TrackedMessage> trackedMessages;
    uint16_t maxMessages;
    uint32_t trackingTimeout;
    
public:
    MessageTracker(uint16_t max = 500, uint32_t timeout = 60000) 
        : maxMessages(max), trackingTimeout(timeout) {}
    
    bool isProcessed(uint32_t messageId, uint32_t originNode);
    void markProcessed(uint32_t messageId, uint32_t originNode, 
                       bool sentInternet = false, bool relayed = false);
    void markAcknowledged(uint32_t messageId, uint32_t originNode);
    void cleanup();
    size_t size() const { return trackedMessages.size(); }
};
```

### New API Methods

#### Initialization

```cpp
bool initAsSharedGateway(
    String meshPrefix, 
    String meshPassword,
    String routerSSID,
    String routerPassword,
    Scheduler* userScheduler,
    uint16_t port = 5555,
    SharedGatewayConfig config = SharedGatewayConfig()
);
```

#### Sending Data to Internet

```cpp
uint32_t sendToInternet(
    String destination,
    String payload,
    std::function<void(bool success, uint16_t httpStatus, String error)> callback,
    uint8_t priority = PRIORITY_NORMAL
);
```

#### Gateway Status

```cpp
InternetStatus getInternetStatus();
GatewayInfo* getPrimaryGateway();
bool isPrimaryGateway();
std::vector<GatewayInfo> getGateways();
```

---

## Message Flow Diagrams

### Normal Flow: Direct Internet Access

```
Node A has working Internet connection:

  ┌────────┐                            ┌────────────┐
  │ Node A │                            │  Internet  │
  │ (App)  │                            │  Service   │
  └───┬────┘                            └─────┬──────┘
      │                                       │
      │ 1. sendToInternet(url, data)          │
      │──────────────────────────────────────→│
      │                                       │
      │ 2. HTTP Response (200 OK)             │
      │←──────────────────────────────────────│
      │                                       │
      │ 3. callback(true, 200, "")            │
      │                                       │
```

### Failover Flow: Mesh Routing to Gateway

```
Node A's Internet fails, routes through Node B (primary gateway):

  ┌────────┐        ┌─────────┐         ┌────────────┐
  │ Node A │        │ Node B  │         │  Internet  │
  │ (Sender)│       │(Gateway)│         │  Service   │
  └───┬────┘        └────┬────┘         └─────┬──────┘
      │                  │                     │
      │ 1. sendToInternet() fails locally      │
      │                  │                     │
      │ 2. GatewayDataPackage (Type 620)       │
      │─────────────────→│                     │
      │                  │                     │
      │                  │ 3. HTTP POST        │
      │                  │────────────────────→│
      │                  │                     │
      │                  │ 4. HTTP Response    │
      │                  │←────────────────────│
      │                  │                     │
      │ 5. GatewayAckPackage (Type 621)        │
      │←─────────────────│                     │
      │                  │                     │
      │ 6. callback(true, 200, "")             │
      │                  │                     │
```

---

## Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `sharedGatewayEnabled` | bool | false | Enable/disable shared gateway mode |
| `internetCheckInterval` | uint32_t | 30000 | How often to check Internet connectivity (ms) |
| `internetCheckHost` | String | "8.8.8.8" | Host to ping for Internet check |
| `messageRetryCount` | uint8_t | 3 | Retries before failover to mesh |
| `duplicateTrackingTimeout` | uint32_t | 60000 | How long to track message IDs (ms) |
| `maxTrackedMessages` | uint16_t | 500 | Maximum tracked messages |
| `gatewayHeartbeatInterval` | uint32_t | 15000 | Primary gateway heartbeat frequency (ms) |
| `gatewayFailureTimeout` | uint32_t | 45000 | Time before gateway considered failed (ms) |

---

## Memory and Performance Considerations

### ESP8266 Constraints (~80KB RAM)

| Component | Memory Usage | Recommendation |
|-----------|--------------|----------------|
| Message Tracker | ~12 bytes/message | Limit to 100-200 messages |
| Gateway Info | ~48 bytes/gateway | Track max 5 gateways |
| Relay Queue | ~256 bytes/message | Limit to 10-20 messages |
| Total Overhead | ~5-10KB | Leave 40KB+ for app |

### ESP32 Constraints (~320KB RAM)

| Component | Memory Usage | Recommendation |
|-----------|--------------|----------------|
| Message Tracker | ~12 bytes/message | Can track 500-1000 messages |
| Gateway Info | ~48 bytes/gateway | Track up to 20 gateways |
| Relay Queue | ~256 bytes/message | Queue up to 50 messages |
| Total Overhead | ~15-30KB | Leave 200KB+ for app |

---

## Implementation Phases

### Phase 1: Core Shared Gateway Initialization (2-3 weeks)
- Implement `initAsSharedGateway()` method
- Configure AP+STA mode for all nodes
- Basic mesh connectivity on same channel

### Phase 2: Internet Connectivity Monitoring (1-2 weeks)
- Implement Internet health check
- Track connectivity status per node
- Detect Internet failures

### Phase 3: Failover Routing Through Mesh (2-3 weeks)
- Implement `GatewayDataPackage`
- Route failed sends through mesh
- Primary gateway handles relayed messages

### Phase 4: Duplicate Prevention and Acknowledgment (1-2 weeks)
- Implement message tracking
- Prevent duplicate sends
- Handle acknowledgments

### Phase 5: Gateway Election and Failover (2-3 weeks)
- Implement gateway heartbeat
- Detect gateway failure
- Run deterministic election

### Total Estimated Timeline: 8-13 weeks

---

## Testing Strategy

- Unit tests for message tracking and configuration
- Integration tests for failover scenarios
- Simulator tests for multi-node behavior
- Hardware testing on ESP8266 and ESP32

---

## Risks and Mitigations

1. **Channel Conflict**: Use 5GHz WiFi when available, implement rate limiting
2. **Network Congestion**: Only primary gateway sends heartbeats, adjustable intervals
3. **Race Conditions**: Deterministic evaluation algorithm, state machine
4. **Memory Exhaustion**: Configurable limits, automatic cleanup
5. **False Positives**: Multiple check hosts, consecutive failure threshold

---

## Future Enhancements

- Load balancing across multiple gateways
- Quality of Service prioritization
- End-to-end encryption for relayed data
- Geographic awareness in gateway selection

---

## Open Questions

1. Should all nodes maintain router connection permanently?
2. How to handle partial Internet connectivity?
3. Message queue size and priority when relaying?
4. How to handle mixed-mode networks?
5. Testing without real Internet?

---

## Related Documentation

- [Bridge Failover Guide](../BRIDGE_FAILOVER.md)
- [Bridge to Internet](../../BRIDGE_TO_INTERNET.md)
- [Multi-Bridge Implementation](../implementation/MULTI_BRIDGE_IMPLEMENTATION.md)
- [Message Queue Implementation](../implementation/MESSAGE_QUEUE_IMPLEMENTATION.md)

---

## Appendix B: Message Type Summary

| Type ID | Name | Direction | Purpose |
|---------|------|-----------|---------|
| 620 | GATEWAY_DATA | Node → Gateway | Application data for Internet delivery |
| 621 | GATEWAY_ACK | Gateway → Node | Delivery confirmation |
| 622 | GATEWAY_HEARTBEAT | Gateway → All | Primary gateway status broadcast |

These types are reserved in the 620-629 range for Shared Gateway Mode messages.
