# Shared Gateway Mode - Implementation Plan

> **Status:** Ready for Implementation  
> **Parent Design:** [SHARED_GATEWAY_DESIGN.md](SHARED_GATEWAY_DESIGN.md)  
> **Estimated Total Effort:** 8-13 weeks  
> **Date:** November 2025

---

## Overview

This document breaks down the Shared Gateway Mode feature into discrete, implementable GitHub issues. Each phase is designed to be independently testable and deliverable.

---

## Issue Breakdown

### Phase 1: Core Shared Gateway Initialization

**Issue #1: Add SharedGatewayConfig Structure**
- **Priority:** High
- **Estimated Effort:** 2-3 days
- **Files to Create/Modify:**
  - `src/painlessmesh/gateway.hpp` (NEW)
  - `src/painlessmesh/configuration.hpp` (MODIFY)

**Tasks:**
1. Create `SharedGatewayConfig` struct with all configuration fields:
   ```cpp
   struct SharedGatewayConfig {
       bool enabled = false;
       TSTRING routerSSID = "";
       TSTRING routerPassword = "";
       uint32_t internetCheckInterval = 30000;
       TSTRING internetCheckHost = "8.8.8.8";
       uint16_t internetCheckPort = 53;
       uint32_t internetCheckTimeout = 5000;
       uint8_t messageRetryCount = 3;
       uint32_t retryInterval = 1000;
       uint32_t duplicateTrackingTimeout = 60000;
       uint16_t maxTrackedMessages = 500;
       uint32_t gatewayHeartbeatInterval = 15000;
       uint32_t gatewayFailureTimeout = 45000;
       bool participateInElection = true;
       uint8_t relayedMessagePriority = 0;  // CRITICAL
       bool maintainPermanentConnection = true;
   };
   ```
2. Add validation methods for configuration
3. Add unit tests for configuration validation

**Acceptance Criteria:**
- [ ] SharedGatewayConfig compiles on ESP8266 and ESP32
- [ ] Unit tests pass for all configuration scenarios
- [ ] Memory footprint documented

---

**Issue #2: Implement initAsSharedGateway() Method**
- **Priority:** High
- **Estimated Effort:** 1 week
- **Dependencies:** Issue #1
- **Files to Create/Modify:**
  - `src/painlessMesh.h` (MODIFY)
  - `src/arduino/wifi.hpp` (MODIFY)
  - `src/painlessMeshSTA.cpp` (MODIFY)

**Tasks:**
1. Add `initAsSharedGateway()` method signature to `painlessMesh` class
2. Configure WiFi in AP+STA mode for all nodes
3. Connect to router while maintaining mesh AP
4. Ensure mesh and router operate on same channel
5. Add router connection monitoring
6. Add reconnection logic for router disconnects

**Implementation Details:**
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

**Acceptance Criteria:**
- [ ] All nodes can connect to router and mesh simultaneously
- [ ] Mesh communication works between nodes
- [ ] Router connection is maintained
- [ ] Automatic reconnection on router disconnect
- [ ] Works on both ESP8266 and ESP32

---

**Issue #3: Create Shared Gateway Example**
- **Priority:** Medium
- **Estimated Effort:** 2-3 days
- **Dependencies:** Issue #2
- **Files to Create:**
  - `examples/sharedGateway/sharedGateway.ino` (NEW)
  - `examples/sharedGateway/README.md` (NEW)
  - `examples/sharedGateway/platformio.ini` (NEW)

**Tasks:**
1. Create basic example demonstrating shared gateway mode
2. Document configuration options
3. Add serial output for debugging
4. Include comments explaining each step

**Acceptance Criteria:**
- [ ] Example compiles and runs on ESP32
- [ ] Example compiles and runs on ESP8266
- [ ] README explains setup and usage
- [ ] All nodes connect to router and mesh

---

### Phase 2: Internet Connectivity Monitoring

**Issue #4: Implement Internet Health Check**
- **Priority:** High
- **Estimated Effort:** 3-4 days
- **Dependencies:** Issue #2
- **Files to Create/Modify:**
  - `src/painlessmesh/gateway.hpp` (MODIFY)
  - `src/painlessmesh/mesh.hpp` (MODIFY)

**Tasks:**
1. Implement TCP connection test to configurable host/port
2. Add periodic health check task
3. Track local Internet connectivity status
4. Add `hasLocalInternet()` method
5. Fire callback on connectivity change

**Implementation Details:**
```cpp
// Check if THIS node has direct Internet access
bool hasLocalInternet();

// Callback when local Internet status changes
void onLocalInternetChanged(std::function<void(bool available)> callback);

// Get last Internet check result
InternetStatus getInternetStatus();
```

**Acceptance Criteria:**
- [ ] Health check detects Internet availability
- [ ] Callback fires on connectivity change
- [ ] Works with configurable check interval
- [ ] Minimal memory overhead
- [ ] Does not block mesh operations

---

**Issue #5: Add Internet Status Broadcasting**
- **Priority:** Medium
- **Estimated Effort:** 2-3 days
- **Dependencies:** Issue #4
- **Files to Modify:**
  - `src/painlessmesh/gateway.hpp` (MODIFY)
  - `src/painlessmesh/protocol.hpp` (MODIFY)

**Tasks:**
1. Broadcast Internet status to mesh periodically
2. Track which nodes have Internet access
3. Update known gateway list based on broadcasts
4. Add `getNodesWithInternet()` method

**Acceptance Criteria:**
- [ ] All nodes know which peers have Internet
- [ ] Status updates propagate within 30 seconds
- [ ] Memory efficient tracking (max 20 nodes)

---

### Phase 3: Failover Routing Through Mesh

**Issue #6: Implement GatewayDataPackage**
- **Priority:** High
- **Estimated Effort:** 3-4 days
- **Dependencies:** Issue #5
- **Files to Create/Modify:**
  - `src/painlessmesh/gateway.hpp` (MODIFY)
  - `src/painlessmesh/protocol.hpp` (MODIFY)

**Tasks:**
1. Define `GatewayDataPackage` (Type ID 620)
2. Implement serialization/deserialization
3. Add message ID generation (unique per node)
4. Include origin node, timestamp, payload fields
5. Add unit tests for package serialization

**Package Structure:**
```cpp
class GatewayDataPackage : public SinglePackage {
public:
    uint32_t messageId;      // Unique message ID
    uint32_t originNode;     // Node that created message
    uint32_t timestamp;      // Creation timestamp
    uint8_t priority;        // 0=CRITICAL to 3=LOW
    TSTRING destination;     // URL/endpoint
    TSTRING payload;         // Application data
    TSTRING contentType;     // MIME type
    uint8_t retryCount;      // Relay attempts
    bool requiresAck;        // Send acknowledgment?
};
```

**Acceptance Criteria:**
- [ ] Package serializes/deserializes correctly
- [ ] Message IDs are unique per node
- [ ] Unit tests pass
- [ ] Works with ArduinoJson v6 and v7

---

**Issue #7: Implement GatewayAckPackage**
- **Priority:** High
- **Estimated Effort:** 2 days
- **Dependencies:** Issue #6
- **Files to Modify:**
  - `src/painlessmesh/gateway.hpp` (MODIFY)
  - `src/painlessmesh/protocol.hpp` (MODIFY)

**Tasks:**
1. Define `GatewayAckPackage` (Type ID 621)
2. Include success status, HTTP code, error message
3. Route acknowledgment back to origin node
4. Add unit tests

**Acceptance Criteria:**
- [ ] Ack packages route back to origin
- [ ] Contains delivery status information
- [ ] Unit tests pass

---

**Issue #8: Implement sendToInternet() API**
- **Priority:** High
- **Estimated Effort:** 1 week
- **Dependencies:** Issues #6, #7
- **Files to Modify:**
  - `src/painlessmesh/mesh.hpp` (MODIFY)
  - `src/painlessmesh/gateway.hpp` (MODIFY)

**Tasks:**
1. Add `sendToInternet()` method
2. Try local Internet first
3. On failure, route through mesh to gateway
4. Track pending messages for acknowledgment
5. Call user callback with result
6. Implement retry logic

**API:**
```cpp
uint32_t sendToInternet(
    String destination,
    String payload,
    std::function<void(bool success, uint16_t httpStatus, String error)> callback,
    uint8_t priority = PRIORITY_CRITICAL
);
```

**Acceptance Criteria:**
- [ ] Sends via local Internet when available
- [ ] Falls back to mesh routing when unavailable
- [ ] Callback receives delivery confirmation
- [ ] Supports configurable priority
- [ ] Works on ESP8266 and ESP32

---

**Issue #9: Implement Gateway Message Handler**
- **Priority:** High
- **Estimated Effort:** 3-4 days
- **Dependencies:** Issue #8
- **Files to Modify:**
  - `src/painlessmesh/gateway.hpp` (MODIFY)
  - `src/painlessmesh/mesh.hpp` (MODIFY)

**Tasks:**
1. Add handler for incoming `GatewayDataPackage`
2. Extract payload and forward to Internet
3. Send `GatewayAckPackage` back to origin
4. Add user callback for custom handling

**Acceptance Criteria:**
- [ ] Gateway receives and processes relayed messages
- [ ] Sends to Internet on behalf of origin node
- [ ] Returns acknowledgment with status
- [ ] User can override default handling

---

### Phase 4: Duplicate Prevention and Acknowledgment

**Issue #10: Implement MessageTracker Class**
- **Priority:** High
- **Estimated Effort:** 3-4 days
- **Dependencies:** None (can start parallel)
- **Files to Create:**
  - `src/painlessmesh/message_tracker.hpp` (NEW)

**Tasks:**
1. Create `MessageTracker` class
2. Track processed message IDs with timestamps
3. Implement cleanup of expired entries
4. Add memory-efficient storage (configurable max)
5. Thread-safe operations for ESP32

**Implementation:**
```cpp
class MessageTracker {
public:
    MessageTracker(uint16_t maxMessages = 500, uint32_t timeoutMs = 60000);
    
    bool isProcessed(uint32_t messageId, uint32_t originNode);
    void markProcessed(uint32_t messageId, uint32_t originNode);
    void markAcknowledged(uint32_t messageId, uint32_t originNode);
    void cleanup();
    size_t size() const;
};
```

**Acceptance Criteria:**
- [ ] Prevents duplicate message processing
- [ ] Automatic cleanup of old entries
- [ ] Memory usage within limits
- [ ] Unit tests for all scenarios
- [ ] Works on ESP8266 (limited RAM)

---

**Issue #11: Integrate Duplicate Prevention**
- **Priority:** High
- **Estimated Effort:** 2-3 days
- **Dependencies:** Issues #9, #10
- **Files to Modify:**
  - `src/painlessmesh/gateway.hpp` (MODIFY)

**Tasks:**
1. Check incoming messages against tracker
2. Skip already-processed messages
3. Track sent acknowledgments
4. Add metrics for duplicate detection

**Acceptance Criteria:**
- [ ] Duplicate messages are dropped
- [ ] Single acknowledgment per message
- [ ] Metrics available for monitoring

---

### Phase 5: Gateway Election and Failover

**Issue #12: Implement GatewayHeartbeatPackage**
- **Priority:** High
- **Estimated Effort:** 2 days
- **Dependencies:** Issue #5
- **Files to Modify:**
  - `src/painlessmesh/gateway.hpp` (MODIFY)
  - `src/painlessmesh/protocol.hpp` (MODIFY)

**Tasks:**
1. Define `GatewayHeartbeatPackage` (Type ID 622)
2. Include primary status, Internet availability, RSSI
3. Broadcast periodically from primary gateway
4. Track heartbeats from all gateways

**Acceptance Criteria:**
- [ ] Primary gateway broadcasts heartbeats
- [ ] All nodes track gateway health
- [ ] Heartbeat contains election-relevant data

---

**Issue #13: Implement Gateway Election Protocol**
- **Priority:** High
- **Estimated Effort:** 1 week
- **Dependencies:** Issue #12
- **Files to Modify:**
  - `src/painlessmesh/gateway.hpp` (MODIFY)

**Tasks:**
1. Detect when primary gateway fails (missed heartbeats)
2. Trigger election among nodes with Internet
3. Use deterministic algorithm (RSSI, then node ID)
4. Handle split-brain scenarios
5. Announce new primary to mesh

**Election Algorithm:**
1. Wait for `gatewayFailureTimeout` without heartbeat
2. Each eligible node calculates its score (RSSI)
3. Highest RSSI wins; node ID breaks ties
4. Winner announces primary status
5. Other nodes defer to announced primary

**Acceptance Criteria:**
- [ ] Election triggers on primary failure
- [ ] Deterministic winner selection
- [ ] New primary starts heartbeats
- [ ] Smooth transition without message loss

---

**Issue #14: Implement isPrimaryGateway() and Related APIs**
- **Priority:** Medium
- **Estimated Effort:** 2 days
- **Dependencies:** Issue #13
- **Files to Modify:**
  - `src/painlessmesh/mesh.hpp` (MODIFY)

**Tasks:**
1. Add `isPrimaryGateway()` method
2. Add `getPrimaryGateway()` method
3. Add `onGatewayChanged()` callback
4. Add `getGateways()` for all known gateways

**Acceptance Criteria:**
- [ ] APIs return correct gateway status
- [ ] Callback fires on gateway change
- [ ] Documentation updated

---

### Phase 6: Documentation and Examples

**Issue #15: Update API Documentation**
- **Priority:** Medium
- **Estimated Effort:** 2-3 days
- **Dependencies:** All implementation issues
- **Files to Modify:**
  - `docs/api/shared-gateway.md` (NEW)
  - `README.md` (MODIFY)
  - API documentation files

**Tasks:**
1. Document all new methods
2. Add usage examples
3. Document configuration options
4. Add troubleshooting guide

**Acceptance Criteria:**
- [ ] All new APIs documented
- [ ] Examples are working and tested
- [ ] Doxygen comments complete

---

**Issue #16: Create Advanced Example with HTTP Client**
- **Priority:** Medium
- **Estimated Effort:** 3-4 days
- **Dependencies:** Issue #9
- **Files to Create:**
  - `examples/sharedGatewayHTTP/sharedGatewayHTTP.ino` (NEW)
  - `examples/sharedGatewayHTTP/README.md` (NEW)

**Tasks:**
1. Create example with actual HTTP client
2. Demonstrate failover behavior
3. Show acknowledgment handling
4. Include status monitoring

**Acceptance Criteria:**
- [ ] Example sends real HTTP requests
- [ ] Demonstrates mesh failover
- [ ] Works on ESP32 with WiFiClientSecure

---

## Implementation Order

### Sprint 1 (Weeks 1-2): Foundation
- Issue #1: SharedGatewayConfig Structure
- Issue #2: initAsSharedGateway() Method
- Issue #10: MessageTracker Class (parallel)

### Sprint 2 (Weeks 3-4): Connectivity
- Issue #3: Basic Example
- Issue #4: Internet Health Check
- Issue #5: Internet Status Broadcasting

### Sprint 3 (Weeks 5-7): Core Messaging
- Issue #6: GatewayDataPackage
- Issue #7: GatewayAckPackage
- Issue #8: sendToInternet() API
- Issue #9: Gateway Message Handler

### Sprint 4 (Weeks 8-9): Reliability
- Issue #11: Duplicate Prevention Integration
- Issue #12: GatewayHeartbeatPackage

### Sprint 5 (Weeks 10-12): Election & Polish
- Issue #13: Gateway Election Protocol
- Issue #14: Gateway Status APIs
- Issue #15: Documentation
- Issue #16: Advanced Example

---

## Testing Requirements

### Unit Tests (per issue)
- Configuration validation
- Package serialization/deserialization
- Message tracker operations
- Election algorithm

### Integration Tests
- Multi-node mesh with shared gateway
- Failover scenarios
- Duplicate prevention
- Election convergence

### Hardware Tests
- ESP8266 memory limits
- ESP32 multi-threading
- Real Internet connectivity
- Long-running stability

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Memory overflow on ESP8266 | Configurable limits, aggressive cleanup |
| Channel conflicts | Use router channel detection |
| Election race conditions | Deterministic algorithm, random delays |
| Message loss during failover | Acknowledgment system, retry logic |

---

## Success Metrics

- All 16 issues completed and tested
- Works on ESP8266 with <10KB overhead
- Works on ESP32 with <30KB overhead
- Failover time <5 seconds
- Zero duplicate messages delivered
- 95%+ message delivery rate

---

## Related Documents

- [SHARED_GATEWAY_DESIGN.md](SHARED_GATEWAY_DESIGN.md) - Feature design document
- [BRIDGE_FAILOVER.md](../BRIDGE_FAILOVER.md) - Existing failover documentation
