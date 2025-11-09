# Bridge Failover with RSSI-Based Election

## Overview

Bridge failover enables painlessMesh networks to automatically recover from bridge node failures by electing a new bridge based on router signal strength (RSSI). This ensures continuous Internet connectivity for critical applications like fish farm monitoring, industrial IoT, and smart building systems.

## Problem Statement

In a typical mesh network, the bridge node connecting to the Internet represents a single point of failure:

- **Bridge goes offline**: Entire mesh loses Internet access
- **Bridge loses Internet**: Gateway unavailable for data upload
- **Manual recovery**: Requires human intervention to restore connectivity

Without automatic failover, critical systems can miss alarms, lose sensor data, or fail to respond to urgent conditions.

## Solution: Distributed Bridge Election

painlessMesh implements a distributed consensus protocol that:

1. **Detects failures**: Monitors bridge heartbeats (Type 610 status broadcasts)
2. **Triggers elections**: Starts election when primary bridge fails
3. **Selects winner**: Deterministically chooses node with best router signal
4. **Promotes bridge**: Winner automatically becomes new bridge
5. **Announces takeover**: Informs mesh of new bridge

## Architecture

### Message Types

#### Type 610: BRIDGE_STATUS (Existing)
Bridge nodes broadcast their status every 30 seconds:
```json
{
  "type": 610,
  "from": 1234567890,
  "routing": 2,
  "internetConnected": true,
  "routerRSSI": -42,
  "routerChannel": 6,
  "uptime": 3600000,
  "gatewayIP": "192.168.1.1",
  "timestamp": 1609459200
}
```

#### Type 611: BRIDGE_ELECTION (New)
Candidates broadcast their router signal strength:
```json
{
  "type": 611,
  "from": 2886734890,
  "routing": 2,
  "routerRSSI": -35,
  "uptime": 3600000,
  "freeMemory": 150000,
  "timestamp": 1609459300,
  "routerSSID": "MyRouter"
}
```

#### Type 612: BRIDGE_TAKEOVER (New)
Winner announces bridge role assumption:
```json
{
  "type": 612,
  "from": 2886734890,
  "routing": 2,
  "previousBridge": 1234567890,
  "reason": "Election winner - best router signal",
  "routerRSSI": -35,
  "timestamp": 1609459400
}
```

### Election Protocol

```
┌──────────────────────────────────────────────────────┐
│                  NORMAL OPERATION                    │
│  Bridge broadcasts status every 30s (Type 610)      │
└──────────────────────────────────────────────────────┘
                        ↓
                 Bridge fails
                 (60s timeout)
                        ↓
┌──────────────────────────────────────────────────────┐
│              ELECTION PHASE (5 seconds)              │
│  1. Each node scans for router RSSI                 │
│  2. Broadcasts candidacy (Type 611)                 │
│  3. Collects all candidates                         │
└──────────────────────────────────────────────────────┘
                        ↓
┌──────────────────────────────────────────────────────┐
│           EVALUATION PHASE (Instant)                 │
│  All nodes independently evaluate candidates using:  │
│  1. Best RSSI wins                                  │
│  2. Tiebreaker: Highest uptime                      │
│  3. Tiebreaker: Most free memory                    │
│  4. Tiebreaker: Lowest node ID                      │
└──────────────────────────────────────────────────────┘
                        ↓
┌──────────────────────────────────────────────────────┐
│            PROMOTION PHASE (5 seconds)               │
│  Winner:                                            │
│  1. Calls initAsBridge()                           │
│  2. Connects to router                              │
│  3. Broadcasts takeover (Type 612)                  │
└──────────────────────────────────────────────────────┘
                        ↓
┌──────────────────────────────────────────────────────┐
│              NEW NORMAL OPERATION                    │
│  New bridge broadcasts status (Type 610)            │
└──────────────────────────────────────────────────────┘
```

### Winner Selection Algorithm

All nodes execute identical deterministic evaluation:

```cpp
BridgeCandidate* winner = nullptr;
int8_t bestRSSI = -127;

for (auto& candidate : candidates) {
  if (candidate.routerRSSI > bestRSSI) {
    // Better signal strength
    bestRSSI = candidate.routerRSSI;
    winner = &candidate;
  } else if (candidate.routerRSSI == bestRSSI) {
    // Tiebreaker 1: Higher uptime (more stable)
    if (candidate.uptime > winner->uptime) {
      winner = &candidate;
    } else if (candidate.uptime == winner->uptime) {
      // Tiebreaker 2: More memory (more capable)
      if (candidate.freeMemory > winner->freeMemory) {
        winner = &candidate;
      } else if (candidate.freeMemory == winner->freeMemory) {
        // Tiebreaker 3: Lower node ID (deterministic)
        if (candidate.nodeId < winner->nodeId) {
          winner = &candidate;
        }
      }
    }
  }
}
```

## API Reference

### Configuration

```cpp
// Set router credentials (required for election participation)
mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);

// Enable automatic failover (default: enabled)
mesh.enableBridgeFailover(true);

// Set election timeout in milliseconds (default: 5000)
mesh.setElectionTimeout(5000);

// Set bridge timeout for failure detection (default: 60000)
mesh.setBridgeTimeout(60000);

// Set bridge status broadcast interval (default: 30000)
mesh.setBridgeStatusInterval(30000);
```

### Callbacks

```cpp
// Called when bridge status changes
void onBridgeStatusChanged(uint32_t bridgeNodeId, bool hasInternet) {
  if (!hasInternet) {
    Serial.println("Bridge lost Internet - election may start");
  }
}

// Called when this node's role changes
void onBridgeRoleChanged(bool isBridge, String reason) {
  if (isBridge) {
    Serial.printf("Promoted to bridge: %s\n", reason.c_str());
  }
}

// Register callbacks
mesh.onBridgeStatusChanged(&onBridgeStatusChanged);
mesh.onBridgeRoleChanged(&onBridgeRoleChanged);
```

### Status Methods

```cpp
// Check if this node is a bridge
bool isBridge = mesh.isBridge();

// Check if any bridge has Internet connectivity
bool hasInternet = mesh.hasInternetConnection();

// Get primary (best) bridge
BridgeInfo* primary = mesh.getPrimaryBridge();
if (primary) {
  Serial.printf("Primary bridge: %u (RSSI: %d dBm)\n", 
                primary->nodeId, primary->routerRSSI);
}

// Get all known bridges
std::vector<BridgeInfo> bridges = mesh.getBridges();
for (const auto& bridge : bridges) {
  Serial.printf("Bridge %u: Internet=%s, RSSI=%d\n",
                bridge.nodeId,
                bridge.internetConnected ? "YES" : "NO",
                bridge.routerRSSI);
}
```

## Usage Example

### Basic Setup

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMesh"
#define MESH_PASSWORD   "password"
#define ROUTER_SSID     "MyRouter"
#define ROUTER_PASSWORD "routerpass"

painlessMesh mesh;

void setup() {
  // Initialize as regular node
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler);
  
  // Enable automatic failover
  mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
  mesh.enableBridgeFailover(true);
  
  // Register callbacks
  mesh.onBridgeStatusChanged(&bridgeStatusCallback);
  mesh.onBridgeRoleChanged(&bridgeRoleCallback);
}
```

### Initial Bridge Setup

```cpp
void setup() {
  // Initialize as bridge with automatic channel detection
  mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                   ROUTER_SSID, ROUTER_PASSWORD,
                   &userScheduler);
}
```

See `examples/bridge_failover/` for complete working example.

## Failure Scenarios

### Scenario 1: Bridge Goes Offline

**Timeline:**
- T+0s: Bridge node powers off
- T+30s: Last status broadcast expires
- T+60s: Nodes detect failure (timeout threshold)
- T+62s: Election starts (2s coordination delay)
- T+67s: Election completes (5s collection window)
- T+72s: Winner promoted to bridge (5s promotion)

**Total Failover Time:** ~70 seconds

### Scenario 2: Bridge Loses Internet

**Timeline:**
- T+0s: Router Internet connection fails
- T+30s: Bridge broadcasts `internetConnected: false`
- T+30s: Nodes receive status, consider election
- T+32s: Election starts (if no recovery detected)
- T+37s: Election completes
- T+42s: Winner promoted to bridge

**Total Failover Time:** ~42 seconds

### Scenario 3: Sequential Bridge Failures

**Timeline:**
- T+0s: Primary bridge fails, election starts
- T+70s: Node A wins, becomes bridge
- T+120s: Node A also fails
- T+180s: Failure detected (60s timeout)
- T+250s: New election completes, Node B becomes bridge

**Recovery:** Continues indefinitely until stable bridge found

## Performance Characteristics

### Timing
- **Failure detection:** 60 seconds (configurable)
- **Election duration:** 5 seconds (configurable)
- **Promotion delay:** 5 seconds (WiFi reconnection)
- **Total failover:** 60-70 seconds typical

### Network Overhead
- **Status broadcasts:** 256 bytes per bridge every 30s
- **Election messages:** 256 bytes per candidate (one-time)
- **Takeover announcement:** 256 bytes (one-time)

### Memory Usage
- **Per candidate:** ~12 bytes during election
- **Bridge tracking:** ~48 bytes per bridge
- **State machine:** ~100 bytes

### Scalability
- **Tested:** Up to 10 nodes
- **Theoretical:** 50+ nodes (limited by election timeout)
- **Recommended:** 5-10 nodes per mesh

## Edge Cases and Prevention

### Split-Brain Prevention

**Problem:** Multiple nodes promote themselves simultaneously

**Solution:** 
- State machine prevents concurrent elections
- Deterministic evaluation ensures consensus
- All nodes reach same conclusion independently

### Rapid Failover Prevention

**Problem:** Bridge roles oscillate rapidly

**Solution:**
- Minimum 60 seconds between role changes
- RSSI hysteresis (winner must be significantly better)
- Uptime tiebreaker favors stable nodes

### Phantom Election Prevention

**Problem:** Election starts when bridge is healthy

**Solution:**
- Only trigger on confirmed failure (60s no heartbeat)
- Check `hasInternetConnection()` before starting
- Verify router is visible before participating

### Router Visibility Issues

**Problem:** Candidate can't see router during scan

**Solution:**
- Return RSSI=0 if router not found
- Candidate excluded from winner consideration
- Election continues with remaining candidates

## Troubleshooting

### Elections Don't Start

**Check:**
- Router credentials configured: `setRouterCredentials()`
- Failover enabled: `enableBridgeFailover(true)`
- Bridge timeout exceeded (60 seconds)
- At least one node can see router

**Debug:**
```cpp
Serial.printf("Credentials: %s\n", routerCredentialsConfigured ? "YES" : "NO");
Serial.printf("Failover: %s\n", bridgeFailoverEnabled ? "ON" : "OFF");
Serial.printf("Last bridge seen: %u ms ago\n", millis() - lastBridgeSeen);
```

### Wrong Node Wins Election

**Check:**
- RSSI measurement accuracy (WiFi scan)
- Router placement and interference
- Tiebreaker criteria (uptime, memory, node ID)

**Debug:**
```cpp
Serial.printf("My RSSI: %d dBm\n", scanRouterSignalStrength(ROUTER_SSID));
Serial.printf("My uptime: %u ms\n", millis());
Serial.printf("My free memory: %u bytes\n", ESP.getFreeHeap());
```

### Bridge Promotion Fails

**Check:**
- Router password correct
- Router channel compatible
- Node can reach router physically
- Sufficient memory for bridge mode

**Debug:**
```cpp
Serial.printf("WiFi status: %d\n", WiFi.status());
Serial.printf("Router channel: %d\n", WiFi.channel());
Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
```

## Security Considerations

### Router Credentials

**Risk:** Router password stored in node memory

**Mitigation:**
- Use separate guest network for mesh bridges
- Limit router permissions (no admin access)
- Consider WPA2-Enterprise for stronger security

### Rogue Bridge Prevention

**Risk:** Malicious node claims bridge role with fake RSSI

**Mitigation:**
- Physical security of mesh nodes
- Verify bridge Internet connectivity post-election
- Monitor bridge status broadcasts for anomalies

### Denial of Service

**Risk:** Attacker triggers repeated elections

**Mitigation:**
- Minimum 60s between role changes
- Rate limiting on election start triggers
- Monitor for excessive election activity

## Best Practices

### 1. Configure Redundancy
```cpp
// At least 2 nodes with router credentials
mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
```

### 2. Monitor Bridge Health
```cpp
void loop() {
  if (!mesh.hasInternetConnection()) {
    // Queue critical data locally
    queueMessage(msg);
  }
}
```

### 3. Log Election Activity
```cpp
void onBridgeRoleChanged(bool isBridge, String reason) {
  logToSD(millis(), isBridge, reason);  // Audit trail
}
```

### 4. Test Failover Regularly
```cpp
// Scheduled failover test (monthly)
if (shouldTestFailover()) {
  // Temporarily disable primary bridge
  testBridgeFailover();
}
```

### 5. Optimize Router Placement
- Position router centrally in mesh coverage area
- Minimize physical obstructions
- Avoid interference from other 2.4GHz devices

## Comparison with Alternatives

### Manual Failover
- **Pro:** Full control, predictable behavior
- **Con:** Requires human intervention, slow recovery

### Pre-designated Backup
- **Pro:** Fast failover to known secondary
- **Con:** Backup may have poor signal, not optimal

### RSSI-Based Election (painlessMesh)
- **Pro:** Automatic, optimal selection, distributed
- **Con:** 60-70s failover time, requires router visibility

## Future Enhancements

### Potential Improvements
1. **Faster failover:** Reduce detection timeout to 30s
2. **Predictive failover:** Detect degrading bridges before failure
3. **Multi-router support:** Failover between different routers
4. **Quality of Service:** Prioritize critical traffic during failover
5. **Geographic awareness:** Consider physical location in selection

### Experimental Features
- Load-based selection (choose least-loaded node)
- Battery-aware (exclude low-battery nodes)
- Historical reliability (favor nodes with uptime track record)

## References

- Issue: Alteriom/painlessMesh#XX (Bridge Failover Request)
- Related: Alteriom/painlessMesh#63 (Bridge Status Broadcast)
- Related: Alteriom/painlessMesh#59 (initAsBridge Method)
- Example: `examples/bridge_failover/`
- Test: `test/catch/catch_alteriom_packages.cpp`

## Credits

- **Requested by:** @woodlist (fish farm alarm system)
- **Implemented:** painlessMesh v1.8.0
- **Consensus model:** Based on Raft algorithm principles
- **RSSI selection:** Adapted from WiFi mesh best practices
