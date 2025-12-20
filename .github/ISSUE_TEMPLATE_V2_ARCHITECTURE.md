# Architectural Improvements for painlessMesh v2.x

## Overview

This issue tracks architectural improvements to address underlying flow and reliability issues in painlessMesh, moving beyond timing-based fixes to more robust solutions.

## Background

Recent fixes (v1.9.x series) have addressed immediate crashes by adjusting timing constants (AsyncClient deletion delays, retry delays, etc.). While these fixes are necessary for hardware compatibility (especially ESP32-C6 RISC-V), they are band-aids over deeper architectural issues.

## Proposed Architectural Improvements for v2.x

### 1. Channel Management & Bridge Election

**Current Issue**: Bridge election can force all nodes to switch channels, causing network disruption.

**Proposed Solution**:
- Prioritize bridges that are already on the same channel as the current mesh
- Implement channel-aware bridge election algorithm
- Only trigger channel switches when absolutely necessary
- Add bridge preference scoring based on:
  - Channel match with current mesh
  - Internet connectivity quality
  - RSSI strength
  - Node stability/uptime

**Implementation Considerations**:
- Backward compatibility with v1.x election protocol
- Gradual rollout with feature flag
- Testing with mixed-version networks

### 2. Message Delivery Reliability

**Current Issue**: Message delivery depends on timing and connection stability, with no robust guarantee mechanism.

**Proposed Solution**:
- Implement proper message acknowledgment protocol
- Add message queuing with retry logic at application layer
- Implement delivery confirmation callbacks
- Add message priority levels (critical, normal, low)
- Persistent queue for critical messages (survive reboots)

**API Changes** (backward compatible):
```cpp
// New API for reliable delivery
uint32_t msgId = mesh.sendReliable(dest, msg, 
    [](uint32_t msgId, bool delivered) {
        // Delivery confirmation callback
    },
    3,  // max retries
    5000 // retry timeout ms
);

// Enhanced sendToInternet with delivery tracking
uint32_t msgId = mesh.sendToInternet(url, data,
    [](uint32_t msgId, int httpStatus, String response) {
        // Delivery confirmation with response
    },
    MessagePriority::HIGH
);
```

### 3. Connection State Management

**Current Issue**: Connection establishment and teardown lacks proper state machine, leading to race conditions.

**Proposed Solution**:
- Implement formal state machine for connection lifecycle:
  - DISCONNECTED
  - CONNECTING
  - CONNECTED
  - AUTHENTICATING
  - ESTABLISHED
  - DISCONNECTING
  - ERROR
- Add state transition validation
- Implement proper cleanup for each state
- Add connection health monitoring

### 4. AsyncClient Lifecycle Management

**Current Issue**: AsyncClient deletion timing is critical and error-prone, requiring hardware-specific delays.

**Proposed Solution**:
- Implement reference-counted AsyncClient wrapper
- Add proper lifecycle callbacks from AsyncTCP
- Use RAII patterns for automatic resource management
- Abstract hardware-specific timing into platform layer

**Implementation**:
```cpp
class ManagedAsyncClient {
    std::shared_ptr<AsyncClient> client;
    PlatformTimings timings; // ESP32, ESP32-C6, ESP8266 specific
    
    ~ManagedAsyncClient() {
        // RAII-based cleanup with proper timing
    }
};
```

### 5. Network Topology Stability

**Current Issue**: Rapid topology changes cause cascading failures and reconnections.

**Proposed Solution**:
- Add topology change dampening
- Implement exponential backoff for reconnection attempts
- Add node stability scoring
- Prefer stable nodes for routing

### 6. Internet Gateway Management

**Current Issue**: Gateway selection and failover is simplistic.

**Proposed Solution**:
- Implement gateway health monitoring
- Add automatic gateway failover
- Support multiple concurrent gateways
- Implement load balancing across gateways
- Add gateway quality metrics (latency, bandwidth, reliability)

### 7. Testing & Validation

**Enhancements Needed**:
- Add integration tests for channel switching scenarios
- Add stress tests for high connection churn
- Add tests for bridge election with various topologies
- Add hardware-in-the-loop tests for ESP32-C6 specific behavior
- Add performance benchmarks

## Implementation Plan

### Phase 1: Research & Design (v2.0-alpha)
- Detailed design documents for each component
- Prototype implementations
- Community feedback
- Breaking change assessment

### Phase 2: Core Architecture (v2.0-beta1)
- Connection state machine
- Channel-aware bridge election
- Message delivery framework

### Phase 3: Reliability Features (v2.0-beta2)
- Reliable message delivery
- Gateway failover
- Topology stability

### Phase 4: Platform Abstraction (v2.0-beta3)
- Hardware-specific timing abstraction
- AsyncClient lifecycle management
- Testing and validation

### Phase 5: Release (v2.0.0)
- Documentation
- Migration guide
- Deprecation warnings for v1.x patterns

## Breaking Changes

Expected breaking changes in v2.x:
- Connection callback signatures may change
- Some timing constants moved to platform-specific config
- Bridge election protocol changes (with backward compatibility mode)
- Message format may be extended (with version negotiation)

## Migration Path

- v1.x will remain supported with critical bug fixes
- v2.x will offer backward compatibility mode
- Migration guide will provide step-by-step upgrade path
- Gradual migration: v2.x nodes can coexist with v1.x nodes during transition

## Success Metrics

- 90% reduction in timing-related issues
- Improved message delivery reliability (>99.9% for critical messages)
- Faster network convergence after topology changes
- Better performance on ESP32-C6 and future hardware
- Reduced support burden from architectural issues

## Related Issues

- #324 - ESP32-C6 heap corruption (timing-based fix in v1.9.15)
- Previous timing-related fixes in v1.9.x series

## Community Input

Community feedback is welcome on:
1. Priority of improvements
2. API design preferences
3. Breaking change tolerance
4. Migration path requirements
5. Additional architectural issues to address

---

**Note**: This is a planning issue for v2.x. Current v1.9.x fixes remain necessary for immediate stability while this architectural work proceeds.
