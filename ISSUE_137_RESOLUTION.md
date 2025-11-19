# Issue #137 Resolution: Bridge Takeover Announcement Channel Synchronization

## Problem Description

When a mesh network node promotes to bridge via election, it connects to a router that may be operating on a different WiFi channel than the mesh. This creates a situation where:

1. The bridge switches to the router's channel (e.g., from channel 1 to channel 6)
2. Other mesh nodes remain on the original channel (channel 1)
3. Bridge takeover announcements sent on the new channel (6) are not heard by nodes still on the old channel (1)
4. Nodes become isolated and cannot reconnect to the mesh

**Original Issue Comment:**
> The second MC powered up with a priory bad signal to router. It trying to create a mesh network on default (1-th) channel. The first node has good RSSI and at once become a bridge on a channel that will be defined later, and likely will be on different channel then the second MC did for mesh network initialization. Will the second MC reset its mesh network channel and seek for bridge on all possible WiFi channels?

## Root Cause Analysis

The issue occurred because:
1. Bridge election happens on the current channel (e.g., channel 1)
2. Winning node calls `promoteToBridge()` which:
   - Stops the mesh
   - Calls `initAsBridge()` to connect to router and detect its channel
   - Initializes mesh on router's channel (e.g., channel 6)
   - Sends takeover announcement
3. By the time the takeover announcement is sent, the bridge is on channel 6
4. Other nodes are still on channel 1 and cannot hear the announcement
5. Nodes continue scanning only on channel 1 and never find the mesh on channel 6

## Solution Overview

The solution consists of two complementary mechanisms:

### 1. Dual-Channel Takeover Announcements

**File:** `src/arduino/wifi.hpp` - `promoteToBridge()` method

The bridge now sends takeover announcements on BOTH the old and new channels:

**Before channel switch:**
```cpp
// Send announcement on current channel (e.g., channel 1)
sendBroadcast(takeoverMessage);
delay(1000);  // Allow propagation

// Stop and switch to router's channel
stop();
initAsBridge(...);  // Now on channel 6
```

**After channel switch:**
```cpp
// Schedule follow-up announcement on new channel (e.g., channel 6)
addTask(3000, TASK_ONCE, [this]() {
    sendBroadcast(followUpMessage);
});
```

**Benefits:**
- Nodes still on old channel hear the first announcement
- Nodes that switch quickly hear the second announcement
- Complete coverage regardless of timing

### 2. Automatic Channel Re-detection

**Files:** `src/painlessMeshSTA.h` and `src/painlessMeshSTA.cpp`

Nodes now automatically detect when the mesh has moved to a different channel:

**Tracking:**
```cpp
// Track consecutive scans with no mesh nodes found
uint16_t consecutiveEmptyScans = 0;
static const uint16_t EMPTY_SCAN_THRESHOLD = 6;  // ~30 seconds
```

**Detection:**
```cpp
if (aps.empty()) {
    consecutiveEmptyScans++;
    
    if (consecutiveEmptyScans >= EMPTY_SCAN_THRESHOLD && 
        WiFi.status() != WL_CONNECTED) {
        // Trigger full channel scan (all channels 1-13)
        uint8_t detectedChannel = scanForMeshChannel(ssid, hidden);
        
        if (detectedChannel > 0 && detectedChannel != mesh->_meshChannel) {
            // Update to new channel and restart AP
            mesh->_meshChannel = detectedChannel;
            channel = detectedChannel;
            restartAPOnNewChannel();
        }
    }
}
```

**Benefits:**
- Automatic - no manual intervention required
- Robust - works even if announcements are missed
- Safe - includes guards against false triggers

### 3. Bridge Election Deferral (Additional Fix - Nov 2025)

**Files:** `src/arduino/wifi.hpp` and `src/painlessMeshSTA.h`

To prevent nodes from trying to become bridges when they should be re-syncing channels, bridge election now checks mesh connectivity state:

**Pre-election Check:**
```cpp
uint16_t emptyScans = stationScan.getConsecutiveEmptyScans();
if (emptyScans >= 3 && WiFi.status() != WL_CONNECTED) {
    // Defer election to allow channel re-sync to complete
    Log(CONNECTION, "Mesh connectivity lost, deferring election...\n");
    return;
}
```

**Problem Addressed:**
- Nodes losing mesh connectivity would immediately try to become bridges
- Router scanning interfered with channel re-detection
- Created unnecessary network churn and duplicate bridges

**Benefits:**
- Prioritizes mesh re-synchronization over bridge promotion
- Prevents unnecessary bridge elections during channel transitions
- Maintains stable mesh topology
- Reduces router connection attempts

## Complete Scenario Flow

### Setup
- Node2: Started on channel 1 with weak router signal
- Node1: On channel 1 with good router signal
- Router: Operating on channel 6

### Timeline

**T+0s: Bridge Election**
- Node1 wins election (best router RSSI)
- Node1 sends takeover announcement on channel 1
- Node2 receives announcement: "Node1 is becoming bridge"

**T+1s: Channel Switch Begins**
- Node1 stops mesh on channel 1
- Node1 connects to router, detects channel 6

**T+2s: Bridge Initialized**
- Node1 initializes mesh on channel 6
- Node1 configured as root/bridge

**T+5s: Follow-up Announcement**
- Node1 sends follow-up announcement on channel 6
- (Node2 still on channel 1, doesn't hear this)

**T+15s-45s: Mesh Scanning Phase**
- Node2 scans channel 1 repeatedly (emptyScans: 1, 2, 3...)
- No mesh nodes found on channel 1
- Fast scanning mode activated (every 15 seconds)

**T+30s: Bridge Monitor Check**
- Node2's bridge monitor detects no healthy bridge
- Triggers bridge election attempt
- **NEW:** Election detects emptyScans = 3, defers election
- Log: "Mesh connectivity lost, deferring election to allow channel re-sync"

**T+90s: Auto Re-detection Triggered**
- Node2 has scanned 6 times on channel 1 (emptyScans threshold reached)
- No mesh nodes found in any scan
- Node2 triggers full channel scan across all channels (1-13)

**T+91s: Channel Found**
- Node2 scans all channels
- Finds mesh on channel 6 (RSSI: -45 dBm)
- Node2 updates: `mesh->_meshChannel = 6`
- Log: "Mesh found on different channel 6 (was 1), updating..."

**T+92s: AP Restarted**
- Node2 restarts its AP on channel 6
- Node2 resumes scanning on channel 6
- Log: "AP restarted on channel 6"

**T+95s: Mesh Reunified**
- Node2 finds Node1's AP on channel 6
- Node2 connects to Node1
- Mesh network fully operational on channel 6
- **No unnecessary bridge election occurred**

## Files Changed

### Code Changes
1. **src/painlessMeshSTA.h** (Original + Nov 2025 Update)
   - Added `consecutiveEmptyScans` counter
   - Added `EMPTY_SCAN_THRESHOLD` constant
   - **NEW:** Added `isChannelResyncNeeded()` helper method
   - **NEW:** Added `getConsecutiveEmptyScans()` accessor

2. **src/painlessMeshSTA.cpp**
   - Modified `connectToAP()` to trigger channel re-scan
   - Automatic channel update and AP restart logic
   - Added `scanForMeshChannel()` to scan all channels

3. **src/arduino/wifi.hpp** (Original + Nov 2025 Update)
   - Modified `promoteToBridge()` to send dual announcements
   - Pre-switch announcement on old channel
   - Post-switch announcement on new channel (delayed)
   - **NEW:** Modified `startBridgeElection()` to check mesh connectivity
   - **NEW:** Defers election if emptyScans ≥ 3 and not connected

### Documentation
1. **docs/CHANNEL_SYNCHRONIZATION.md**
   - Complete technical documentation
   - Configuration options and trade-offs
   - Debugging guide with log examples
   - Testing procedures

2. **BRIDGE_TO_INTERNET.md**
   - Updated with channel re-synchronization section
   - Link to detailed documentation

3. **test/catch/catch_channel_resync.cpp** (Original + Nov 2025 Update)
   - Unit tests for threshold configuration
   - Documentation tests for expected behavior
   - **NEW:** Added test for bridge election deferral behavior

4. **ISSUE_137_RESOLUTION.md** (this file)
   - Complete resolution summary
   - **Updated:** Added section 3 on bridge election deferral

## Configuration

### Adjusting Re-scan Threshold

Edit `src/painlessMeshSTA.h`:

```cpp
static const uint16_t EMPTY_SCAN_THRESHOLD = 6;  // Default: 6 scans
```

**Trade-offs:**
- **Lower** (e.g., 3): Faster detection, more false triggers
- **Higher** (e.g., 10): More stable, slower detection

### Expected Timing

With default settings:
- `SCAN_INTERVAL = 30 seconds`
- Fast scanning when no nodes found: `0.5 * SCAN_INTERVAL = 15 seconds`
- Time to trigger re-scan: `6 × 15s = 90 seconds (~1.5 minutes)`

## Debugging

### Enable Detailed Logging

```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
```

### Key Log Messages

**Channel Re-detection:**
```
CONNECTION: connectToAP(): No mesh nodes found for 6 scans, triggering channel re-detection
CONNECTION: scanForMeshChannel(): Found mesh on channel 6 (RSSI: -45)
CONNECTION: connectToAP(): Mesh found on different channel 6 (was 1), updating...
CONNECTION: connectToAP(): AP restarted on channel 6
```

**Bridge Promotion:**
```
STARTUP: Sending takeover announcement on current channel before switching...
STARTUP: ✓ Takeover announcement sent on channel 1
STARTUP: ✓ Router connected on channel 6
STARTUP: ✓ Bridge promotion complete on channel 6
STARTUP: Sending follow-up takeover announcement on new channel 6
```

**Bridge Election Deferral (Nov 2025 Fix):**
```
CONNECTION: Bridge monitor: No healthy bridge detected, triggering election
CONNECTION: startBridgeElection(): Mesh connectivity lost (3 empty scans), deferring election to allow channel re-sync
CONNECTION: startBridgeElection(): Will retry election in 75 seconds if still needed
```

## Testing Recommendations

### Manual Test Procedure

1. Setup two ESP32/ESP8266 nodes
2. Configure router on channel 6
3. Start both nodes without router credentials (will default to channel 1)
4. Give one node router credentials
5. Trigger bridge election on that node
6. Monitor serial output on both nodes
7. Verify channel synchronization occurs
8. Confirm mesh connectivity restored

### Expected Results

- Bridge switches to channel 6 within 5 seconds
- Non-bridge node detects channel change within 90 seconds
- Both nodes reconnect on channel 6
- Mesh connectivity maintained (brief interruption during transition)

## Benefits

✅ **Automatic**: No manual configuration or intervention required  
✅ **Robust**: Works even if initial announcements are missed  
✅ **Safe**: Guards against false triggers from temporary network issues  
✅ **Documented**: Complete documentation and debugging guidance  
✅ **Tested**: Unit tests validate threshold configuration  

## Future Enhancements (Optional)

Potential improvements for future releases:

1. **Faster Initial Detection**: Reduce threshold for first-time channel detection
2. **Channel Hints**: Include new channel in takeover announcement for faster switching
3. **Prioritized Scanning**: Scan common channels (1, 6, 11) first
4. **Bridge Coordination**: Multiple bridges coordinate channel selection

## Related Documentation

- [Channel Synchronization Documentation](docs/CHANNEL_SYNCHRONIZATION.md) - Complete technical guide
- [Bridge Setup Guide](BRIDGE_TO_INTERNET.md) - Bridge configuration
- [Issue #137](https://github.com/Alteriom/painlessMesh/issues/137) - Original issue

## Credits

**Issue Reporter:** @woodlist  
**Implementation:** GitHub Copilot  
**Review:** @sparck75  

---

**Status:** ✅ Resolved  
**Version:** 1.8.12 (pending)  
**Date:** 2025-11-19
