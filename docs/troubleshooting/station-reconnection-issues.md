# Station Reconnection Issues in Bridge Mode

## Problem Description

When using `stationManual()` to create a bridge node that connects to a router while also maintaining a mesh network, users may experience issues where the station (router) connection drops during mesh initialization and fails to reconnect automatically.

### Symptoms

- Initial connection to router succeeds
- Mesh network initializes successfully
- Station connection drops with `ARDUINO_EVENT_WIFI_STA_DISCONNECTED` events
- Station never reconnects despite reconnection logic being triggered
- Serial output shows repeated disconnect events without successful reconnection

### Example Serial Output

```
=== WiFi Connectivity Diagnostics ===
Step 1: Connecting to router to detect channel...
✓ Successfully connected to router!
  Router Channel: 6 ← Auto-detected!
  Router IP: 192.168.18.11

Step 2: Initializing mesh on channel 6...
STARTUP: init(): 1
STARTUP: init(): Mesh channel set to 6
STARTUP: AP tcp server established on port 5555
STARTUP: stationManual(): Connecting to MyRouter
STARTUP: stationManual(): Connection initiated
✓ Mesh initialized

Step 3: Waiting for station reconnection...
CONNECTION: eventSTADisconnectedHandler: ARDUINO_EVENT_WIFI_STA_DISCONNECTED
CONNECTION: eraseClosedConnections():
CONNECTION: eventSTADisconnectedHandler: ARDUINO_EVENT_WIFI_STA_DISCONNECTED
CONNECTION: eraseClosedConnections():
⚠ Station not connected yet (may connect later)
```

## Root Cause

The issue was in the `connectToAP()` method in `src/painlessMeshSTA.cpp`. When operating in manual mode (for router connections via `stationManual()`):

1. Station disconnects during mesh initialization (normal behavior in AP+STA mode)
2. Disconnect callback triggers `yieldConnectToAP()` to reconnect
3. `connectToAP()` checks if router SSID is in the scan results
4. Router SSID is NOT in scan results (scan only looks for mesh nodes on mesh channel)
5. Function returns without calling `WiFi.begin()` to reconnect
6. Station remains disconnected indefinitely

### Code Analysis

**Before Fix** (lines 192-195 in `painlessMeshSTA.cpp`):

```cpp
} else if (aps.empty() || !ssid.equals(aps.begin()->ssid)) {
    task.enableDelayed(SCAN_INTERVAL);
    return;  // ← Just delays, never attempts reconnection!
}
```

The problem: This conditional assumes the router SSID will appear in the `aps` list from `stationScan()`. However, `stationScan()` only scans on the mesh channel for mesh nodes, not for routers which may be on the same or different channel.

## Solution

**After Fix** (v1.8.1+):

```cpp
} else {
    // For manual router connections, reconnect directly using WiFi.begin()
    // Don't rely on scan results since router may be on different channel
    Log(CONNECTION, 
        "connectToAP(): Manual connection - attempting to reconnect to %s\n",
        ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    task.enableDelayed(SCAN_INTERVAL);
    return;
}
```

The fix:
- Removes dependency on scan results for manual connections
- Calls `WiFi.begin()` directly to reconnect to the router
- Lets ESP hardware auto-detect the router's channel (as designed)
- Adds clear logging to show reconnection attempts

## Verification

After applying the fix, the expected behavior is:

```
Step 3: Waiting for station reconnection...
CONNECTION: eventSTADisconnectedHandler: ARDUINO_EVENT_WIFI_STA_DISCONNECTED
CONNECTION: eraseClosedConnections():
CONNECTION: connectToAP(): Manual connection - attempting to reconnect to MyRouter
✓ Station reconnected successfully!
  IP Address: 192.168.18.11
```

## Workaround (for older versions)

If you're using a version before v1.8.1, you can work around this issue by implementing explicit reconnection logic:

```cpp
void setup() {
  // ... mesh initialization ...
  
  // Add a task to monitor and reconnect station
  userScheduler.addTask(Task(5000, TASK_FOREVER, [](){
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Station disconnected, reconnecting...");
      WiFi.begin(ROUTER_SSID, ROUTER_PASSWORD);
    }
  }));
}
```

## Related Issues

- Issue #21: Original user report from @woodlist
- Issue #50: Bug tracking for stationManual() channel issues
- Issue #59: Bridge-centric architecture proposal
- PR #[number]: Fix implementation

## Affected Versions

- **Affected**: v1.5.0 - v1.8.0
- **Fixed**: v1.8.1+

## Platforms

This issue affects all ESP platforms:
- ESP32 (all variants including ESP32-C6, ESP32-S3)
- ESP8266

## Additional Notes

### Why Does Station Disconnect During Mesh Init?

When the ESP switches from pure STA mode to AP+STA mode during mesh initialization, the WiFi subsystem may briefly disconnect from the station to reconfigure. This is normal behavior and the library should automatically reconnect.

### Channel Matching

Remember that in AP+STA mode, both the AP (mesh) and STA (router connection) **must use the same WiFi channel**. This is a hardware limitation. The fix ensures reconnection works regardless of channel, but both interfaces will still operate on the same channel.

### Best Practice

For production bridge nodes, consider using the "Station First" pattern to auto-detect the router's channel before initializing the mesh:

```cpp
void setup() {
  // Step 1: Connect to router first to detect its channel
  WiFi.mode(WIFI_STA);
  WiFi.begin(ROUTER_SSID, ROUTER_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  uint8_t detectedChannel = WiFi.channel();
  Serial.printf("Router channel: %d\n", detectedChannel);
  
  WiFi.disconnect();
  delay(1000);
  
  // Step 2: Initialize mesh on the detected channel
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT, 
            WIFI_AP_STA, detectedChannel);
  mesh.stationManual(ROUTER_SSID, ROUTER_PASSWORD);
}
```

This approach guarantees channel compatibility and more reliable connections.
