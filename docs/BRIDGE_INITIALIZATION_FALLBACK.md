# Bridge Initialization and Fallback Patterns

## Overview

This document describes the behavior of `initAsBridge()` when router connection fails, and demonstrates recommended fallback patterns that give library users control over error handling and recovery strategies.

## Design Philosophy

As a library, painlessMesh provides the building blocks for mesh networking but **does not dictate application-level recovery strategies**. When bridge initialization fails, the library:

1. **Returns failure status** (`false`) instead of forcing actions like restart
2. **Provides clear error logging** to help diagnose issues
3. **Leaves mesh state clean** for user-controlled recovery
4. **Allows graceful fallback** to regular mesh node

This design enables flexible deployment patterns:
- Single-bridge networks with manual recovery
- Multi-bridge networks with automatic redundancy
- Hybrid approaches with controlled failover behavior

## Bridge Initialization Behavior

### Success Path

When `initAsBridge()` successfully connects to the router:

```cpp
bool initAsBridge(TSTRING meshSSID, TSTRING meshPassword,
                  TSTRING routerSSID, TSTRING routerPassword,
                  Scheduler *baseScheduler, uint16_t port = 5555);
```

**Steps:**
1. Disconnect from any existing WiFi connections
2. Connect to router in STA mode (30 second timeout)
3. Detect router's WiFi channel
4. Initialize mesh AP on same channel as router
5. Re-establish router connection using stationManual
6. Set node as root/bridge
7. Start bridge status broadcasting
8. **Return `true`**

**Result:** Device functions as bridge on router's channel, maintaining both router and mesh connectivity.

### Failure Path

When router connection fails (timeout after 30 seconds):

**Steps:**
1. Log error: "Failed to connect to router"
2. Log error: "Cannot become bridge without router connection"
3. Log error: "Bridge initialization aborted - remaining as regular node"
4. **Return `false` without initializing mesh**

**Result:** Device is not initialized as bridge OR regular node. User code must decide next steps.

### Why No Automatic Recovery?

The library does not automatically restart or force fallback because:

1. **User control**: Application knows its deployment context and requirements
2. **Network stability**: Avoid restart loops that could destabilize mesh
3. **Flexibility**: Different use cases need different recovery strategies
4. **Predictability**: Library behavior should be deterministic and explicit

## Recommended Fallback Patterns

### Pattern 1: Fallback to Regular Node (Basic)

**Use Case:** Single bridge network where bridge is not critical to mesh operation.

```cpp
bool bridgeSuccess = mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                                       ROUTER_SSID, ROUTER_PASSWORD,
                                       &userScheduler, MESH_PORT);

if (!bridgeSuccess) {
  Serial.println("✗ Failed to initialize as bridge!");
  Serial.println("Router unreachable - falling back to regular mesh node");
  
  // Fallback: Initialize as regular mesh node
  // Device can still participate in mesh without bridge functionality
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  Serial.println("✓ Initialized as regular mesh node");
  Serial.println("Note: To function as a bridge, fix router and restart");
}
```

**Benefits:**
- Device remains part of mesh network
- Can receive messages from other nodes
- Can participate in mesh topology
- Manual intervention needed to restore bridge role

### Pattern 2: Fallback with Auto-Promotion (Recommended)

**Use Case:** Networks where any node can become bridge when router becomes available.

```cpp
bool bridgeSuccess = mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                                       ROUTER_SSID, ROUTER_PASSWORD,
                                       &userScheduler, MESH_PORT);

if (!bridgeSuccess) {
  Serial.println("✗ Failed to initialize as bridge!");
  Serial.println("Router unreachable - enabling bridge failover");
  
  // Fallback: Regular node with automatic bridge promotion
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
  mesh.enableBridgeFailover(true);
  mesh.setElectionTimeout(5000);
  
  Serial.println("✓ Running as regular node");
  Serial.println("Will auto-promote to bridge when router available");
}
```

**Benefits:**
- Automatic recovery when router comes online
- No manual intervention needed
- Participates in bridge elections
- Maintains mesh connectivity throughout

### Pattern 3: Multi-Bridge Redundancy

**Use Case:** Critical networks with multiple potential bridges.

**Primary Bridge:**
```cpp
bool bridgeSuccess = mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                                       ROUTER_SSID, ROUTER_PASSWORD,
                                       &userScheduler, MESH_PORT, 10); // Priority 10

if (!bridgeSuccess) {
  Serial.println("✗ Primary bridge init failed!");
  Serial.println("Router unreachable - secondary should be active");
  
  // Fallback: Regular node (secondary bridge takes over)
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  Serial.println("✓ Running as regular node");
  Serial.println("Secondary bridge should provide connectivity");
}
```

**Secondary Bridge:**
```cpp
// Secondary bridge configuration (priority 5)
// If primary fails, secondary provides redundancy
```

**Benefits:**
- Immediate redundancy via secondary bridge
- Graceful degradation of primary
- No single point of failure
- Maintains Internet connectivity for mesh

### Pattern 4: Retry with Exponential Backoff

**Use Case:** Environments with intermittent router availability.

```cpp
const int MAX_RETRIES = 3;
int retryCount = 0;
int retryDelay = 5000; // Start with 5 seconds

while (retryCount < MAX_RETRIES) {
  bool bridgeSuccess = mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                                         ROUTER_SSID, ROUTER_PASSWORD,
                                         &userScheduler, MESH_PORT);
  
  if (bridgeSuccess) {
    Serial.println("✓ Bridge initialized successfully");
    break;
  }
  
  retryCount++;
  if (retryCount < MAX_RETRIES) {
    Serial.printf("Retry %d/%d in %d seconds...\n", 
                  retryCount, MAX_RETRIES, retryDelay/1000);
    delay(retryDelay);
    retryDelay *= 2; // Exponential backoff
  }
}

if (retryCount >= MAX_RETRIES) {
  Serial.println("Max retries reached - falling back to regular node");
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}
```

**Benefits:**
- Handles temporary router outages
- Exponential backoff prevents network flooding
- Eventually falls back if persistent failure
- Configurable retry strategy

### Pattern 5: User-Controlled Restart

**Use Case:** Explicit bridge nodes that require bridge functionality to operate.

```cpp
bool bridgeSuccess = mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                                       ROUTER_SSID, ROUTER_PASSWORD,
                                       &userScheduler, MESH_PORT);

if (!bridgeSuccess) {
  Serial.println("✗ Failed to initialize as bridge!");
  Serial.println("This device must function as a bridge");
  Serial.println("Restarting in 30 seconds to retry...");
  delay(30000);
  ESP.restart(); // User's choice to restart
}
```

**Benefits:**
- Clear intent that bridge role is required
- User controls restart timing
- Can implement watchdog or LED indicators
- Appropriate for dedicated bridge hardware

## Channel Discovery and Mesh Formation

### Two Nodes on Different Channels

When two mesh nodes start on different channels:

1. **Station Scan Discovery:**
   - Each node scans for mesh SSID on all channels
   - Nodes discover each other via beacon frames
   - Connection negotiation determines channel

2. **Channel Selection:**
   - Node with more connections typically maintains its channel
   - New node switches to join existing network
   - Root node (bridge) has priority in channel selection

3. **Bridge Channel Priority:**
   - Bridge nodes maintain router's channel
   - Regular nodes switch to bridge's channel to join
   - This ensures bridge maintains router connection

### Bridge Appearance and Channel Changes

When a new bridge appears in an existing mesh:

1. **Bridge Broadcasts Status (Type 610):**
   - Announces presence on its channel (router's channel)
   - Other nodes receive status updates

2. **Nodes Evaluate Connectivity:**
   - If bridge is on different channel, nodes must decide:
     - Stay on current mesh channel?
     - Switch to bridge's channel for Internet access?

3. **Channel Re-synchronization:**
   - Nodes may perform channel re-sync if mesh fragmented
   - After 6 consecutive empty scans, trigger re-sync
   - Re-scan all channels to find mesh

4. **Avoiding Channel Chase Loops:**
   - Election mechanism includes channel check
   - Nodes defer election if approaching re-sync threshold
   - Prevents oscillation between channels

## Best Practices

### 1. Choose Appropriate Fallback Pattern

- **Critical Infrastructure:** Use Pattern 3 (Multi-Bridge Redundancy)
- **General IoT:** Use Pattern 2 (Auto-Promotion)
- **Testing/Development:** Use Pattern 1 (Basic Fallback)
- **Dedicated Bridges:** Use Pattern 5 (User-Controlled Restart)

### 2. Monitor Bridge Status

```cpp
mesh.onBridgeStatusChanged([](uint32_t bridgeNodeId, bool hasInternet) {
  Serial.printf("Bridge %u: Internet %s\n", 
                bridgeNodeId, hasInternet ? "UP" : "DOWN");
});
```

### 3. Handle Bridge Role Changes

```cpp
mesh.onBridgeRoleChanged([](bool isBridge, String reason) {
  if (isBridge) {
    Serial.printf("Promoted to bridge: %s\n", reason.c_str());
  } else {
    Serial.printf("Demoted from bridge: %s\n", reason.c_str());
  }
});
```

### 4. Avoid Restart Loops

- Implement maximum retry counts
- Use exponential backoff for retries
- Monitor consecutive failures and adjust strategy
- Consider manual intervention threshold

### 5. Document Deployment Strategy

Clearly document in your application code:
- Which fallback pattern is used
- Why that pattern was chosen
- Expected behavior during failures
- Manual recovery procedures if needed

## Testing Recommendations

### Test Scenario 1: Router Unreachable at Startup

1. Configure node as bridge
2. Make router unreachable (wrong password, powered off, etc.)
3. Verify node falls back gracefully
4. Check node can join mesh as regular node
5. Verify no restart loops occur

### Test Scenario 2: Router Becomes Available Later

1. Start with router unreachable
2. Node falls back to regular node with failover enabled
3. Power on router
4. Verify node detects router and promotes to bridge
5. Check channel switching works correctly

### Test Scenario 3: Multi-Bridge Failover

1. Start primary and secondary bridges
2. Make primary's router unreachable
3. Verify primary falls back to regular node
4. Check secondary bridge continues providing connectivity
5. Restore primary's router and verify it resumes bridge role

### Test Scenario 4: Channel Chase Prevention

1. Start mesh on channel 6
2. Add bridge on channel 11
3. Verify nodes handle channel mismatch
4. Check no continuous restart or channel oscillation
5. Verify eventual mesh convergence on single channel

## Summary

painlessMesh provides flexible bridge initialization with graceful failure handling:

- **Library returns status**, doesn't force actions
- **Users choose recovery strategy** based on their requirements
- **Multiple fallback patterns** for different use cases
- **Channel management** prevents network instability
- **Comprehensive callbacks** for monitoring and control

This design philosophy ensures painlessMesh can be used in diverse deployments while maintaining network stability and giving users full control over their mesh behavior.
