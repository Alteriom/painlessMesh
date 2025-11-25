# Issue Resolution: Bridge Discovery Failure

## Issue Summary

**Original Issue**: Users reported that two ESP32 nodes running the bridge_failover example could not discover each other. One node became a bridge and connected to the router, while the second node remained isolated, showing "Known bridges: 0" despite scanning and finding the mesh SSID.

**GitHub Issue**: Bridge Failover with RSSI-Based Election Example

## Root Cause

The issue was caused by a WiFi channel mismatch between bridge and regular nodes:

1. **Bridge Node Behavior**:
   - `initAsBridge()` connects to router first
   - Detects router's WiFi channel (e.g., channel 6)
   - Initializes mesh AP on detected channel
   - Operates on channel 6

2. **Regular Node Behavior (INCORRECT)**:
   - `mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT)` 
   - Uses default parameter: `channel=1`
   - Creates AP and scans only on channel 1
   - Cannot discover bridge operating on channel 6

3. **Scanning Logic**:
   - Code in `painlessMeshSTA.cpp` filters scan results by channel
   - APs on different channels are skipped
   - Regular nodes never discover the bridge

## Technical Details

### Default Channel Parameter

The `mesh.init()` function signature has a default channel parameter:

```cpp
void init(TSTRING ssid, TSTRING password, uint16_t port = 5555,
          WiFiMode_t connectMode = WIFI_AP_STA, 
          uint8_t channel = 1,  // ← Default is 1, not 0!
          uint8_t hidden = 0, uint8_t maxconn = MAX_CONN, ...)
```

### Channel Filtering in Scan

From `painlessMeshSTA.cpp`, line 106-108:

```cpp
for (auto i = 0; i < num; ++i) {
    if (WiFi.channel(i) != mesh->_meshChannel) {
        continue;  // Skip APs on different channels
    }
    // Process matching APs...
}
```

### Auto-Detection Logic

When `channel=0` is specified, the code performs auto-detection (lines 44-58):

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
  // ...
}
```

## Solution

### Code Fix

Change regular node initialization from:

```cpp
// INCORRECT - uses default channel=1
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
```

To:

```cpp
// CORRECT - uses channel=0 for auto-detection
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
```

### Files Modified

1. **examples/bridge_failover/bridge_failover.ino**
   - Fixed regular node initialization (line ~155)
   - Fixed fallback initialization (line ~142)
   - Added explanatory comments

2. **examples/multi_bridge/regular_node.ino**
   - Fixed regular node initialization (line ~120)
   - Added explanatory comments

3. **examples/bridgeAwareSensorNode/bridgeAwareSensorNode.ino**
   - Fixed mesh initialization (line ~66)
   - Added explanatory comments

4. **examples/ntpTimeSyncNode/ntpTimeSyncNode.ino**
   - Fixed mesh initialization (line ~94)
   - Added explanatory comments

5. **examples/bridge_failover/README.md**
   - Added "Channel Detection (Automatic)" section
   - Updated troubleshooting guide
   - Documented critical requirement for channel=0

6. **CHANGELOG.md**
   - Added entry in [Unreleased] section
   - Documented root cause, solution, and affected examples

7. **BRIDGE_DISCOVERY_FIX.md** (New)
   - Comprehensive technical explanation
   - Migration guide for existing code
   - Before/after log examples
   - Verification steps

## Testing & Verification

### Expected Behavior After Fix

**Regular Node Startup Logs:**
```
CONNECTION: stationScan(): Auto-detecting mesh channel...
CONNECTION: scanForMeshChannel(): Scanning all channels for mesh 'FishFarmMesh'...
CONNECTION: scanForMeshChannel(): Found mesh on channel 6 (RSSI: -45)
STARTUP: stationScan(): Mesh channel auto-detected: 6
CONNECTION: scanComplete(): Found 1 nodes
```

**Bridge Status Check:**
```
--- Bridge Status ---
I am bridge: NO
Internet available: YES
Known bridges: 1
  Bridge 4185597361: Internet=YES, RSSI=-46 dBm, LastSeen=500 ms ago
Primary bridge: 4185597361 (RSSI: -46 dBm)
```

### Verification Steps

1. ✅ Bridge node successfully connects to router
2. ✅ Bridge operates on router's channel (e.g., channel 6)
3. ✅ Regular nodes auto-detect mesh channel
4. ✅ Regular nodes discover and connect to bridge
5. ✅ Bridge status shows in regular node's bridge list
6. ✅ Mesh network forms correctly
7. ✅ Messages route between nodes

## Impact Assessment

### Affected Examples

Examples that work with bridge nodes and have been fixed:
- ✅ `bridge_failover/bridge_failover.ino`
- ✅ `multi_bridge/regular_node.ino`
- ✅ `bridgeAwareSensorNode/bridgeAwareSensorNode.ino`
- ✅ `ntpTimeSyncNode/ntpTimeSyncNode.ino`

### Unaffected Examples

Examples that don't require changes:
- ✅ `basic/basic.ino` - Already uses channel=0
- ✅ `startHere/startHere.ino` - Simple mesh without bridges
- ✅ `echoNode/echoNode.ino` - Simple mesh without bridges
- ✅ Bridge examples (they use `initAsBridge()` which handles channels correctly)

### User Impact

**Who is affected:**
- Users running bridge failover examples
- Users with custom code based on old examples
- Anyone reporting "No primary bridge available!" errors

**Who is NOT affected:**
- Simple mesh networks without bridges
- Existing code that already specifies channel=0
- Bridge nodes (they already handle channels correctly)

## Best Practices Going Forward

### When to Use Channel=0 (Auto-Detect)

✅ Use `channel=0` when:
- Mesh includes bridge nodes connected to routers
- Router channel is unknown or may change
- Nodes join the mesh at different times
- Maximum compatibility is desired

### When to Use Fixed Channel

❌ Use fixed channel (e.g., `channel=1`) when:
- Pure mesh network with no bridges
- All nodes are equal peers
- You control the WiFi environment completely
- Minimizing startup time is critical

### Code Template for Regular Nodes with Bridges

```cpp
void setup() {
  Serial.begin(115200);
  
  // Initialize with channel auto-detection
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, 
            WIFI_AP_STA, 0);  // channel=0 for auto-detect
  
  // Configure callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onBridgeStatusChanged(&bridgeStatusCallback);
  
  // Optional: Configure for bridge failover
  mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
  mesh.enableBridgeFailover(true);
}
```

## Related Documentation

- **BRIDGE_DISCOVERY_FIX.md** - Comprehensive technical guide
- **examples/bridge_failover/README.md** - Bridge failover documentation
- **BRIDGE_TO_INTERNET.md** - General bridge configuration
- **CHANGELOG.md** - Version history with this fix

## Migration for Existing Users

If you have custom code experiencing this issue:

1. **Locate your initialization**:
   ```cpp
   mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
   ```

2. **Add channel parameter**:
   ```cpp
   mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
   ```

3. **Test thoroughly**:
   - Verify bridge discovery in logs
   - Check bridge status reports
   - Confirm mesh connectivity

## Lessons Learned

1. **Default parameters can cause subtle bugs**: The `channel=1` default seemed reasonable but caused issues with bridge discovery

2. **Documentation is critical**: Need to clearly explain channel behavior in examples that work with bridges

3. **Auto-detection is safer**: For bridge-enabled meshes, channel auto-detection prevents channel mismatch issues

4. **Testing edge cases matters**: This issue only appears when bridges and regular nodes are mixed

## Conclusion

The bridge discovery issue has been fully resolved by updating all bridge-related examples to use `channel=0` for automatic channel detection. This ensures regular nodes can discover bridges regardless of which WiFi channel the router is using.

**Resolution Status**: ✅ COMPLETE

**Version**: Will be included in next release after PR merge

**Affected Users**: Should update to latest examples or manually add `channel=0` parameter
