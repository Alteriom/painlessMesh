# Bridge Discovery Issue Resolution

## Issue Description

Users reported that regular mesh nodes could not discover bridge nodes when using the bridge failover examples. The symptom was:

- Regular nodes show "No primary bridge available!" 
- Regular nodes show "Known bridges: 0"
- Nodes form separate meshes instead of connecting together
- Bridge broadcasts were not being received by regular nodes

### Typical Log Output (Before Fix)

**Bridge Node (working correctly):**
```
17:05:40.280 -> Status: {"nodeId":4185597361,"uptime":120,"freeHeap":207556,"isBridge":true,"hasInternet":true}
17:05:43.172 -> I am bridge: YES
17:05:43.172 -> Internet available: YES
```

**Regular Node (not discovering bridge):**
```
17:05:29.620 -> CONNECTION: scanComplete(): num = 1
17:05:29.620 -> CONNECTION: Found 0 nodes
17:05:34.033 -> I am bridge: NO
17:05:34.033 -> Known bridges: 0
17:05:34.033 -> No primary bridge available!
```

## Root Cause Analysis

The issue occurred because of a WiFi channel mismatch:

1. **Bridge nodes** connect to the router first and adopt the **router's WiFi channel** (e.g., channel 6)
2. **Regular nodes** initialized with default parameters used **channel 1** (the default)
3. WiFi scanning on channel 1 cannot detect APs on channel 6
4. Result: Regular nodes never discover the bridge

### Technical Details

When initializing a mesh node:

```cpp
// Default initialization (INCORRECT for bridge discovery)
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
// Uses default channel=1 from function signature
```

The function signature is:
```cpp
void init(TSTRING ssid, TSTRING password, uint16_t port = 5555,
          WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1,
          //                                                     ^^^^ Default is 1
          uint8_t hidden = 0, uint8_t maxconn = MAX_CONN, ...)
```

### Why This Happens

1. Bridge calls `initAsBridge()` which:
   - Connects to router (e.g., on channel 6)
   - Detects router's channel
   - Initializes mesh AP on detected channel (channel 6)

2. Regular node calls `mesh.init()` without channel parameter:
   - Uses default channel=1
   - Creates AP on channel 1
   - Scans only channel 1 for mesh nodes
   - **Cannot see bridge on channel 6**

3. In `painlessMeshSTA.cpp`, scan filtering code:
```cpp
for (auto i = 0; i < num; ++i) {
    if (WiFi.channel(i) != mesh->_meshChannel) {
        continue;  // Skip APs on different channels
    }
    // ... process AP
}
```

## The Fix

### Solution: Use Channel Auto-Detection

Regular nodes must explicitly use `channel=0` to enable automatic channel detection:

```cpp
// CORRECT initialization for bridge discovery
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
//                                                                            ^
//                                                      channel=0 for auto-detect
```

### How Auto-Detection Works

When `channel=0` is specified:

1. Node calls `scanForMeshChannel()` on startup
2. Scans **all WiFi channels** (1-13) looking for the mesh SSID
3. Detects which channel the mesh is operating on
4. Sets `mesh->_meshChannel` to the detected channel
5. Joins the mesh on the correct channel

Code in `painlessMeshSTA.cpp`:
```cpp
void ICACHE_FLASH_ATTR StationScan::stationScan() {
  // If channel is 0, auto-detect the mesh channel first
  if (channel == 0 && mesh->_meshChannel == 0) {
    Log(STARTUP, "stationScan(): Auto-detecting mesh channel...\n");
    uint8_t detectedChannel = scanForMeshChannel(ssid, hidden);
    if (detectedChannel > 0) {
      mesh->_meshChannel = detectedChannel;
      channel = detectedChannel;
      Log(STARTUP, "stationScan(): Mesh channel auto-detected: %d\n", detectedChannel);
    }
  }
  // ... continue with scan on correct channel
}
```

## Fixed Examples

The following examples have been updated to use channel auto-detection:

### 1. bridge_failover.ino

**Before:**
```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
```

**After:**
```cpp
// Initialize as regular node with auto-channel detection
// channel=0 enables automatic mesh channel detection
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
```

### 2. multi_bridge/regular_node.ino

**Before:**
```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
```

**After:**
```cpp
// channel=0 enables automatic mesh channel detection
// This is required to discover bridges that operate on the router's channel
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
```

### 3. bridgeAwareSensorNode/bridgeAwareSensorNode.ino

**Before:**
```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
```

**After:**
```cpp
// channel=0 enables automatic mesh channel detection
// This is required to discover bridges that operate on the router's channel
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
```

## Expected Log Output (After Fix)

**Regular Node (successfully discovering bridge):**
```
CONNECTION: stationScan(): Auto-detecting mesh channel...
CONNECTION: scanForMeshChannel(): Scanning all channels for mesh 'FishFarmMesh'...
CONNECTION: scanForMeshChannel(): Found mesh on channel 6 (RSSI: -45)
STARTUP: stationScan(): Mesh channel auto-detected: 6
CONNECTION: scanComplete(): Found 1 nodes
--- Bridge Status ---
I am bridge: NO
Internet available: YES
Known bridges: 1
  Bridge 4185597361: Internet=YES, RSSI=-46 dBm, LastSeen=500 ms ago
Primary bridge: 4185597361 (RSSI: -46 dBm)
```

## Migration Guide for Existing Code

If you have custom code based on the old examples:

### Check Your Initialization

Look for lines like:
```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
```

### Update to Include Channel Parameter

Change to:
```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
```

### When to Use Channel Auto-Detection

Use `channel=0` (auto-detect) when:
- ✅ Your mesh includes bridge nodes connected to routers
- ✅ You don't control which channel the router uses
- ✅ Nodes might join the mesh at different times
- ✅ You want maximum flexibility

Use fixed channel (e.g., `channel=1`) when:
- ❌ All nodes are regular nodes (no bridges)
- ❌ You control the WiFi environment completely
- ❌ You need to minimize startup time (skip channel scan)
- ❌ You have specific channel requirements (e.g., interference avoidance)

## Verification

To verify your fix is working:

1. **Check startup logs** for auto-detection messages:
   ```
   STARTUP: stationScan(): Auto-detecting mesh channel...
   CONNECTION: scanForMeshChannel(): Found mesh on channel X
   ```

2. **Monitor bridge discovery**:
   ```
   Known bridges: 1
   Primary bridge: XXXXXX (RSSI: -XX dBm)
   ```

3. **Confirm mesh connectivity**:
   - Bridge node shows connected regular nodes
   - Regular nodes show bridge in bridge list
   - Messages successfully route between nodes

## Related Documentation

- [Bridge Failover Example README](examples/bridge_failover/README.md) - Complete bridge failover documentation
- [BRIDGE_TO_INTERNET.md](BRIDGE_TO_INTERNET.md) - General bridge configuration guide
- [Channel Synchronization](docs/CHANNEL_SYNCHRONIZATION.md) - Advanced channel management

## Affected Versions

- **Issue Present**: All versions prior to this fix
- **Fixed In**: Next release after PR merge
- **Workaround**: Manually specify `channel=0` in affected examples

## Split-Brain Race Condition Fix

### Additional Issue Discovered

During testing, a second issue was identified: when multiple nodes with `INITIAL_BRIDGE = false` start simultaneously, they can both become bridges and not see each other (split-brain scenario).

**Root Cause**:
1. Both nodes initialize as regular nodes
2. After 60-second startup period, both detect "no bridge exists"
3. Both trigger elections with short 1-3 second random delay
4. If mesh hasn't formed yet, each runs an isolated election
5. Each wins its own election and becomes a bridge
6. Once bridges, they stop scanning for mesh nodes

**Solution: Configurable Election Timing**

Added new API methods to allow users to tune election timing:

```cpp
// Set startup delay before first election check
// Longer delay allows more time for mesh formation
mesh.setElectionStartupDelay(90000);  // 90 seconds (default: 60 seconds)

// Set random delay range for elections
// Longer delays reduce simultaneous election risk
mesh.setElectionRandomDelay(10000, 30000);  // 10-30 seconds (default: 1-3 seconds)
```

**Recommended Settings for Simultaneous Startups**:
```cpp
mesh.setElectionStartupDelay(90000);      // 90 second startup delay
mesh.setElectionRandomDelay(10000, 30000); // 10-30 second random delay
```

**Alternative Solutions**:
- Stagger node startup by 10-20 seconds
- Use pre-designated bridge mode (`INITIAL_BRIDGE = true` on one node)

## Summary

Two issues were addressed:

1. **Channel Mismatch**: Bridge discovery failed because regular nodes scanned on channel 1 while bridges operated on router's channel. Fixed by using `channel=0` for automatic channel detection.

2. **Split-Brain Race Condition**: Multiple nodes starting simultaneously could both become bridges. Fixed by adding configurable election timing parameters to allow sufficient time for mesh formation before elections start.

Both fixes ensure reliable bridge failover operation in all deployment scenarios.
