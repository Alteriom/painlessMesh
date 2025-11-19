# Channel Synchronization in painlessMesh

## Problem Statement

When a mesh network operates on one channel and a node promotes to bridge via election, it may connect to a router operating on a different channel. This creates a channel mismatch where:

1. Bridge node switches to router's channel (e.g., channel 6)
2. Other mesh nodes remain on original channel (e.g., channel 1)
3. Bridge takeover announcements sent on new channel are not heard by nodes on old channel
4. Nodes cannot find the mesh network and become isolated

## Solution Overview

The solution has two complementary parts:

### Part 1: Automatic Channel Re-detection

Nodes automatically detect when they can't find the mesh on their current channel and trigger a full channel scan to locate it.

**Implementation:** `src/painlessMeshSTA.cpp` and `src/painlessMeshSTA.h`

**Key Components:**
- `consecutiveEmptyScans` counter tracks scans with no mesh nodes found
- `EMPTY_SCAN_THRESHOLD` constant (6 scans) determines when to trigger re-scan
- Full channel scan using `scanForMeshChannel()` when threshold reached
- Automatic channel update and AP restart when mesh found on different channel

**Flow:**
```
1. Node scans on current channel (e.g., channel 1)
2. Finds no mesh nodes → increment consecutiveEmptyScans
3. Repeat for EMPTY_SCAN_THRESHOLD scans (~30 seconds with fast scanning)
4. Trigger scanForMeshChannel() to scan ALL channels (1-13)
5. If mesh found on different channel:
   a. Update mesh->_meshChannel to detected channel
   b. Restart AP on new channel
   c. Reset consecutiveEmptyScans counter
6. Continue normal scanning on new channel
```

**Safeguards:**
- Only triggers when WiFi.status() != WL_CONNECTED (not when stably connected)
- Only triggers when channel > 0 (channel 0 already means auto-detect)
- Resets counter when mesh nodes are found (prevents false triggers)

### Part 2: Dual-Channel Takeover Announcements

Bridge promotion sends takeover announcements on both the old and new channels to ensure all nodes are notified.

**Implementation:** `src/arduino/wifi.hpp` in `promoteToBridge()` method

**Flow:**
```
1. Node wins bridge election
2. Send takeover announcement on CURRENT channel (e.g., channel 1)
   → Nodes still on channel 1 receive this announcement
3. Wait 1 second for announcement to propagate
4. Stop mesh and reinitialize as bridge via initAsBridge()
   → Connects to router, detects router's channel (e.g., channel 6)
   → Initializes mesh on channel 6
5. Schedule follow-up takeover announcement on NEW channel (channel 6)
   → Sent 3 seconds after initialization
   → Nodes that switched to channel 6 receive this announcement
```

**Benefits:**
- Early announcement notifies nodes on old channel before bridge switches
- Delayed announcement notifies nodes that switched early or quickly found new channel
- Ensures complete mesh awareness regardless of node timing

## Expected Behavior

### Scenario: Bridge Promotion with Channel Change

**Setup:**
- Node1: Regular mesh node on channel 1
- Node2: Regular mesh node on channel 1 (weak router signal)
- Router: Operating on channel 6

**Sequence:**
1. Node2 starts mesh on channel 1 (default, no router connection)
2. Node1 joins mesh on channel 1
3. Node1 wins bridge election (better router signal)
4. Node1 sends "Becoming bridge" announcement on channel 1
5. Node2 receives announcement
6. Node1 connects to router on channel 6, initializes mesh on channel 6
7. Node1 sends follow-up "I'm the bridge" announcement on channel 6
8. Node2 can't find mesh on channel 1 for 6 consecutive scans
9. Node2 triggers full channel scan, finds mesh on channel 6
10. Node2 updates to channel 6, restarts AP on channel 6
11. Node2 reconnects to Node1 on channel 6
12. Mesh network is now unified on channel 6

**Timeline:**
- T+0s: Node1 wins election, sends announcement on channel 1
- T+1s: Node1 stops mesh on channel 1
- T+2s: Node1 initializes mesh on channel 6
- T+5s: Node1 sends follow-up announcement on channel 6
- T+30s: Node2 can't find mesh on channel 1 (fast scanning)
- T+30s: Node2 triggers channel re-scan
- T+31s: Node2 finds mesh on channel 6
- T+32s: Node2 restarts AP on channel 6
- T+35s: Node2 reconnects to mesh on channel 6

## Configuration

### Adjusting Re-scan Threshold

The `EMPTY_SCAN_THRESHOLD` is defined in `src/painlessMeshSTA.h`:

```cpp
static const uint16_t EMPTY_SCAN_THRESHOLD = 6; // ~30 seconds at default SCAN_INTERVAL
```

**Trade-offs:**
- **Lower value** (e.g., 3): Faster channel detection, but more susceptible to false triggers
- **Higher value** (e.g., 10): More stable, but slower response to channel changes

### Scan Timing

Default scan intervals defined in `src/painlessmesh/configuration.hpp`:

```cpp
#define SCAN_INTERVAL 30 * TASK_SECOND  // AP scan period in ms
```

When no mesh nodes found, scanning switches to fast mode:
```cpp
task.setInterval(0.5 * SCAN_INTERVAL);  // 15 seconds
```

With EMPTY_SCAN_THRESHOLD=6 and fast scanning:
- Time to trigger re-scan: 6 × 15s = 90 seconds (~1.5 minutes)

## Debugging

### Log Messages

**Channel Re-detection:**
```
CONNECTION: connectToAP(): No mesh nodes found for 6 scans, triggering channel re-detection
CONNECTION: scanForMeshChannel(): Scanning all channels for mesh 'YourMesh'...
CONNECTION: scanForMeshChannel(): Found mesh on channel 6 (RSSI: -45)
CONNECTION: connectToAP(): Mesh found on different channel 6 (was 1), updating...
CONNECTION: connectToAP(): Restarting AP from channel 1 to channel 6
CONNECTION: connectToAP(): AP restarted on channel 6
```

**Bridge Promotion:**
```
STARTUP: === Becoming Bridge Node ===
STARTUP: Sending takeover announcement on current channel before switching...
STARTUP: ✓ Takeover announcement sent on channel 1
STARTUP: Step 1: Connecting to router YourRouter...
STARTUP: ✓ Router connected on channel 6
STARTUP: Step 2: Initializing mesh on channel 6...
STARTUP: ✓ Bridge promotion complete on channel 6
STARTUP: Sending follow-up takeover announcement on new channel 6
STARTUP: ✓ Follow-up takeover announcement sent
```

### Common Issues

**Issue:** Nodes don't switch channels
- **Check:** Ensure `channel=0` in init() for auto-detection
- **Check:** Verify nodes are not connected via stationManual() to router
- **Check:** Increase log level to see scanning activity

**Issue:** Channel switching takes too long
- **Solution:** Reduce EMPTY_SCAN_THRESHOLD in painlessMeshSTA.h
- **Trade-off:** May increase false triggers during temporary network instability

**Issue:** Bridge promotion doesn't work
- **Check:** Verify router credentials are configured
- **Check:** Ensure router has good signal strength (> -80 dBm)
- **Check:** Confirm router is on a valid channel (1-13 for 2.4GHz)

## Testing

### Manual Testing Procedure

1. Setup two ESP32/ESP8266 nodes with painlessMesh
2. Configure router on channel 6
3. Start both nodes without router credentials (will use channel 1)
4. Configure one node with router credentials and trigger election
5. Monitor serial output for channel switching messages
6. Verify both nodes eventually operate on channel 6
7. Verify mesh connectivity is maintained

### Expected Results

- Bridge node switches to router channel within 5 seconds
- Non-bridge nodes detect channel change within 90 seconds
- All nodes reconnect on new channel
- No loss of mesh connectivity (except during transition)

## Related Files

- `src/painlessMeshSTA.h` - StationScan class definition with channel re-detection
- `src/painlessMeshSTA.cpp` - Channel re-detection implementation
- `src/arduino/wifi.hpp` - Bridge promotion with dual announcements
- `test/catch/catch_channel_resync.cpp` - Unit tests for channel synchronization
- `BRIDGE_TO_INTERNET.md` - Bridge setup documentation

## References

- Issue #137: Bridge takeover announcements not heard across channels
- WiFi channels: 1-13 for 2.4GHz (channels 12-13 restricted in some regions)
- ESP32/ESP8266 can only operate on one channel at a time in AP+STA mode
