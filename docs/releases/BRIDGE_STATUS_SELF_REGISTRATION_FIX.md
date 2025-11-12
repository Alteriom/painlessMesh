# Bridge Status Self-Registration Fix

## Issue Reported by @woodlist

**Problem**: After successfully promoting to bridge via election, the node reports:
```
I am bridge: YES
Internet available: NO
Known bridges: 0
No primary bridge available! ❌
```

## Root Cause

When a node becomes a bridge (either via `initAsBridge()` or election promotion), it broadcasts bridge status messages to the mesh network, but **it does not add itself to its own `knownBridges` list**.

### Why This Happens

1. Node wins election and promotes to bridge ✅
2. Node calls `initBridgeStatusBroadcast()` ✅  
3. Node sends bridge status broadcasts ✅
4. **Other nodes receive broadcasts and update their `knownBridges` ✅**
5. **Bridge node itself never receives its own broadcast ❌**
6. Bridge node's `knownBridges` remains empty ❌
7. `getPrimaryBridge()` returns `nullptr` because list is empty ❌

### Code Flow

```cpp
// When bridge promotes
promoteToBridge() 
  → initAsBridge()
    → initBridgeStatusBroadcast()
      → sendBridgeStatus() // Broadcasts to network
        → sendBroadcast(msg) // Bridge doesn't receive its own broadcasts
```

## The Fix

### Changes to `src/arduino/wifi.hpp`

#### 1. Modified `initBridgeStatusBroadcast()`

Added self-registration during bridge initialization:

```cpp
void initBridgeStatusBroadcast() {
    using namespace logger;
    
    if (!this->isBridge() || !this->bridgeStatusBroadcastEnabled) {
      return;
    }
    
    Log(STARTUP, "initBridgeStatusBroadcast(): Setting up bridge status broadcast\n");
    
    // NEW: Register ourselves as a bridge in the knownBridges list
    // This ensures the bridge knows about itself and reports correct status
    this->addTask([this]() {
      bool hasInternet = (WiFi.status() == WL_CONNECTED) && 
                         (WiFi.localIP() != IPAddress(0, 0, 0, 0));
      
      this->updateBridgeStatus(
        this->nodeId,               // bridgeNodeId
        hasInternet,                // internetConnected
        WiFi.RSSI(),                // routerRSSI
        WiFi.channel(),             // routerChannel
        millis(),                   // uptime
        WiFi.gatewayIP().toString(),// gatewayIP
        this->getNodeTime()         // timestamp
      );
      
      Log(STARTUP, "initBridgeStatusBroadcast(): Registered self as bridge (nodeId: %u)\n", 
          this->nodeId);
    });
    
    // ... rest of method unchanged
}
```

#### 2. Modified `sendBridgeStatus()`

Added self-update before broadcasting:

```cpp
void sendBridgeStatus() {
    using namespace logger;
    
    if (!this->bridgeStatusBroadcastEnabled) {
      return;
    }
    
    // ... create JSON message ...
    
    bool hasInternet = (WiFi.status() == WL_CONNECTED) && 
                       (WiFi.localIP() != IPAddress(0, 0, 0, 0));
    
    int8_t rssi = WiFi.RSSI();
    uint8_t channel = WiFi.channel();
    uint32_t uptime = millis();
    TSTRING gatewayIP = WiFi.gatewayIP().toString();
    
    // ... add to JSON ...
    
    Log(GENERAL, "sendBridgeStatus(): Broadcasting status (Internet: %s)\n",
        hasInternet ? "Connected" : "Disconnected");
    Log(GENERAL, "sendBridgeStatus(): WiFi status=%d, localIP=%s, gatewayIP=%s\n",
        WiFi.status(), WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str());
    
    // NEW: Update our own bridge status in knownBridges list
    // This ensures the bridge reports itself correctly when queried
    this->updateBridgeStatus(this->nodeId, hasInternet, rssi, channel, 
                            uptime, gatewayIP, this->getNodeTime());
    
    this->sendBroadcast(msg);
}
```

## Expected Behavior After Fix

### Before Fix
```
--- Bridge Status ---
I am bridge: YES
Internet available: NO
Known bridges: 0
No primary bridge available! ❌
--------------------
```

### After Fix
```
--- Bridge Status ---
I am bridge: YES
Internet available: NO ⚠️ (May be YES if router has internet)
Known bridges: 1
Primary bridge: 3394043125 (RSSI: -36 dBm) ✅
--------------------
```

## Technical Details

### Why Two Registration Points?

1. **`initBridgeStatusBroadcast()`** - Initial registration when bridge first starts
   - Ensures bridge is in `knownBridges` immediately after promotion
   - Provides accurate status for early queries

2. **`sendBridgeStatus()`** - Periodic updates
   - Keeps bridge info current in `knownBridges`
   - Updates RSSI, uptime, internet connectivity dynamically
   - Ensures consistency between broadcast and local state

### Internet Connectivity

The "Internet available: NO" in @woodlist's log may be accurate depending on:
- Router has active internet connection
- DHCP has assigned valid IP address (not 0.0.0.0)
- `WiFi.localIP()` returns valid address

Check with:
```cpp
Log(GENERAL, "sendBridgeStatus(): WiFi status=%d, localIP=%s, gatewayIP=%s\n",
    WiFi.status(), WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str());
```

This log line was present in the code but not visible in the serial output @woodlist provided.

## Impact

### Fixed Issues
✅ Bridge nodes now correctly report themselves in `knownBridges`  
✅ `getPrimaryBridge()` returns valid bridge pointer for bridge nodes  
✅ Bridge status displays accurately show "Known bridges: 1" minimum  
✅ Regular nodes can immediately discover newly promoted bridges  

### Backward Compatibility
✅ No breaking changes to existing API  
✅ Compatible with existing bridge setups  
✅ Works with both manual `initAsBridge()` and election-based promotion  
✅ Multi-bridge configurations continue to function normally  

## Testing

### Manual Test Procedure
1. Flash bridge_failover example to ESP32/ESP8266
2. Start all nodes without pre-configured bridge
3. Wait for 60-second grace period
4. Observe election and promotion
5. Check bridge status output

Expected to see:
- "Known bridges: 1" or more
- Valid primary bridge with node ID and RSSI
- No "No primary bridge available!" error

### Unit Tests
Existing tests in `test/catch/catch_diagnostics_api.cpp` verify:
- `updateBridgeStatus()` adds bridges correctly
- `getPrimaryBridge()` returns correct bridge based on RSSI/health
- Bridge tracking and health monitoring work as expected

## Files Modified
- `src/arduino/wifi.hpp`
  - `initBridgeStatusBroadcast()` - Added self-registration
  - `sendBridgeStatus()` - Added periodic self-update

## Related Issues
- Fixes "No primary bridge available" on bridge nodes
- Related to bridge election feature (v1.8.6)
- Complements bridge failover implementation

## Credits
**Reported by**: @woodlist  
**Analysis**: GitHub Copilot  
**Fix**: Self-registration pattern for bridge tracking  

---

**Version**: To be included in v1.8.7+  
**Date**: November 12, 2025  
**Status**: Implementation complete, pending testing
