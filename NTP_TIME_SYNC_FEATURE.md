# NTP Time Synchronization Feature

## Overview

The NTP Time Synchronization feature enables bridge nodes with Internet connectivity to distribute authoritative NTP time to all nodes in the mesh network. This eliminates the need for each node to query NTP servers individually, saving bandwidth, power, and reducing network congestion.

## Type ID: 614 (TIME_SYNC_NTP)

## Architecture

```
Internet
   |
   | NTP Query
   v
Bridge Node ---------> Regular Node 1
   |                      |
   | Broadcast            | Update Time
   | Type 614             | Sync RTC
   |                      |
   +----------------> Regular Node 2
   |                      |
   +----------------> Regular Node 3
                          |
                          v
                     Application Code
```

## Package Structure

### NTPTimeSyncPackage (Type 614)

```cpp
class NTPTimeSyncPackage : public painlessmesh::plugin::BroadcastPackage {
 public:
  uint32_t ntpTime = 0;     // Unix timestamp from NTP server (seconds)
  uint16_t accuracy = 0;    // Milliseconds uncertainty/precision
  TSTRING source = "";      // NTP server source (e.g., "pool.ntp.org")
  uint32_t timestamp = 0;   // Collection timestamp (millis())
  uint16_t messageType = 614;  // MQTT Schema message_type
};
```

### JSON Format

```json
{
  "type": 614,
  "from": 123456,
  "routing": 2,
  "ntpTime": 1699564800,
  "accuracy": 50,
  "source": "pool.ntp.org",
  "timestamp": 12345678,
  "message_type": 614
}
```

## Implementation Guide

### Bridge Node (Sender)

The bridge node queries NTP and broadcasts time to the mesh:

```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

painlessMesh mesh;

// Periodic task to broadcast NTP time
Task taskBroadcastNTP(60000, TASK_FOREVER, [](){
  auto pkg = NTPTimeSyncPackage();
  pkg.from = mesh.getNodeId();
  pkg.ntpTime = mesh.getNodeTime() / 1000000;  // Convert to seconds
  pkg.accuracy = 50;  // 50ms uncertainty
  pkg.source = "pool.ntp.org";
  pkg.timestamp = millis();
  
  mesh.sendBroadcast(pkg.toJson());
});

void setup() {
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.stationManual(ROUTER_SSID, ROUTER_PASSWORD);  // Bridge mode
  
  userScheduler.addTask(taskBroadcastNTP);
  taskBroadcastNTP.enable();
}
```

### Regular Node (Receiver)

Regular nodes receive and apply NTP time:

```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

void receivedCallback(uint32_t from, String& msg) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, msg);
  JsonObject obj = doc.as<JsonObject>();
  
  if (obj["type"] == 614) {
    auto pkg = NTPTimeSyncPackage(obj);
    
    // Verify sender is a bridge (optional but recommended)
    if (isBridgeNode(from)) {
      // Apply time synchronization
      mesh.setTimeFromNTP(pkg.ntpTime);
      
      // Optional: Sync RTC module if available
      #ifdef HAS_RTC
      rtc.setTime(pkg.ntpTime);
      #endif
      
      Serial.printf("Time synced from %s (±%ums)\n", 
        pkg.source.c_str(), pkg.accuracy);
    }
  }
}

void setup() {
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
}
```

## Field Details

### ntpTime (uint32_t)
- Unix timestamp in seconds since epoch (1970-01-01 00:00:00 UTC)
- Range: 0 to 4,294,967,295 (year 2106)
- Example: 1699564800 = 2023-11-09 20:00:00 UTC

### accuracy (uint16_t)
- Time uncertainty in milliseconds
- Range: 0 to 65,535ms (0 to 65.5 seconds)
- Typical values:
  - 10-50ms: Good NTP connection
  - 50-200ms: Average NTP connection
  - 200-1000ms: Poor/distant NTP server
  - 1000-5000ms: Very poor connection or local time source

### source (TSTRING)
- NTP server hostname or IP address
- Examples:
  - "pool.ntp.org"
  - "time.google.com"
  - "time.nist.gov"
  - "192.168.1.1" (local router)
  - Empty string if no NTP available

### timestamp (uint32_t)
- When the time was collected/broadcast (millis())
- Used for calculating staleness of time data
- Wraps around every 49.7 days

## Best Practices

### For Bridge Nodes

1. **Update Frequency**: Broadcast every 30-60 seconds
   - Too frequent: Wastes bandwidth
   - Too infrequent: Nodes may drift

2. **NTP Query Strategy**: Query NTP less frequently than broadcast
   - Query NTP every 5-15 minutes
   - Cache and reuse recent NTP time
   - Update accuracy field based on staleness

3. **Error Handling**: Handle NTP failures gracefully
   - Stop broadcasting if NTP unavailable for >15 minutes
   - Or increase accuracy value to indicate uncertainty

### For Regular Nodes

1. **Validation**: Verify sender is bridge before applying time
   ```cpp
   bool isBridgeNode(uint32_t nodeId) {
     // Check if node is in bridge list
     // Or check if node has Internet connectivity flag
   }
   ```

2. **Staleness Check**: Don't apply very old time data
   ```cpp
   uint32_t age = millis() - pkg.timestamp;
   if (age > 120000) {  // Older than 2 minutes
     Serial.println("Time data too old, ignoring");
     return;
   }
   ```

3. **Accuracy Threshold**: Only apply if accuracy is acceptable
   ```cpp
   if (pkg.accuracy > 1000) {  // Worse than 1 second
     Serial.println("Time accuracy too poor, ignoring");
     return;
   }
   ```

4. **RTC Integration**: Sync RTC for offline operation
   ```cpp
   if (rtcEnabled && pkg.accuracy < 500) {
     rtc.setTime(pkg.ntpTime);
     Serial.println("RTC synced with NTP time");
   }
   ```

## Benefits

### Network Efficiency
- **Reduced NTP queries**: Only bridge queries NTP, not every node
- **Lower bandwidth**: One NTP query serves entire mesh
- **Less congestion**: Fewer nodes competing for Internet access

### Power Savings
- **No WiFi switching**: Nodes stay on mesh, don't need router connection
- **Lower power**: No NTP protocol overhead per node
- **Longer battery life**: Especially important for sensor nodes

### Improved Accuracy
- **Authoritative source**: Bridge has better NTP access than distant nodes
- **Lower latency**: Mesh broadcast faster than Internet NTP
- **Better synchronization**: All nodes sync to same time source

### Offline Operation
- **RTC sync**: Nodes can sync RTC modules for offline timekeeping
- **Time continuity**: Time available even when bridge loses Internet
- **Graceful degradation**: Mesh continues with cached time

## Security Considerations

### Trust and Validation

1. **Bridge Authentication**: Verify time source is trusted bridge
   - Use bridge node whitelist
   - Check sender's bridge status flag
   - Validate against multiple bridges if available

2. **Replay Attack Prevention**: Check timestamp freshness
   ```cpp
   static uint32_t lastTimestamp = 0;
   if (pkg.timestamp <= lastTimestamp) {
     // Potential replay attack or clock rollback
     return;
   }
   lastTimestamp = pkg.timestamp;
   ```

3. **Sanity Checks**: Validate time is reasonable
   ```cpp
   const uint32_t MIN_TIME = 1609459200;  // 2021-01-01
   const uint32_t MAX_TIME = 2147483647;  // 2038-01-19
   
   if (pkg.ntpTime < MIN_TIME || pkg.ntpTime > MAX_TIME) {
     Serial.println("Time out of valid range");
     return;
   }
   ```

4. **Large Jump Detection**: Reject suspicious time changes
   ```cpp
   uint32_t currentTime = getCurrentTime();
   int32_t timeDelta = pkg.ntpTime - currentTime;
   
   if (abs(timeDelta) > 86400) {  // More than 1 day
     Serial.println("Time change too large, manual intervention needed");
     return;
   }
   ```

## Example Applications

### Sensor Network
- All sensors use synchronized timestamps
- Data correlation across nodes is accurate
- Event ordering is consistent

### Coordinated Actions
- Multiple nodes can trigger actions at specific times
- Light shows with precise timing
- Scheduled operations across mesh

### Data Logging
- Consistent timestamps for all log entries
- Time-series data from multiple sources can be merged
- Historical analysis is accurate

### Security Systems
- Event timestamps are reliable for audit logs
- Video/sensor correlation across devices
- Alarm scheduling with accurate time

## Testing

### Unit Tests

The feature includes comprehensive unit tests in `test/catch/catch_alteriom_packages.cpp`:

- Basic serialization/deserialization
- JSON field validation
- Different NTP sources
- Edge cases (0 values, max values)
- Long source hostnames
- High accuracy values

Run tests:
```bash
cd painlessMesh
cmake -G Ninja .
ninja
./bin/catch_alteriom_packages
```

### Integration Testing

Test with example sketches:

1. Upload `ntpTimeSyncBridge.ino` to bridge node with Internet
2. Upload `ntpTimeSyncNode.ino` to regular mesh nodes
3. Monitor serial output to verify:
   - Bridge broadcasts NTP time
   - Nodes receive and apply time
   - Timestamps are consistent

## Troubleshooting

### Bridge Not Broadcasting

**Symptoms**: No NTP broadcasts seen by nodes

**Solutions**:
1. Check Internet connectivity: `ping 8.8.8.8`
2. Verify NTP server is reachable
3. Check broadcast task is enabled
4. Increase debug level: `mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION)`

### Nodes Not Receiving Time

**Symptoms**: Nodes connected but not syncing time

**Solutions**:
1. Verify receiver callback is registered: `mesh.onReceive(&receivedCallback)`
2. Check message type parsing: `obj["type"] == 614`
3. Ensure JSON buffer is large enough: `DynamicJsonDocument doc(1024)`
4. Monitor for JSON parse errors

### Poor Time Accuracy

**Symptoms**: High accuracy values (>500ms)

**Solutions**:
1. Use closer NTP server (local or regional)
2. Check network latency to NTP server
3. Reduce NTP query frequency
4. Consider using multiple NTP sources and averaging

### Time Drift

**Symptoms**: Time gradually becomes inaccurate

**Solutions**:
1. Increase broadcast frequency (30-60 seconds)
2. Verify NTP queries are successful
3. Check for mesh network stability issues
4. Use RTC module for drift compensation

## Related Features

- **Bridge Status (Type 610)**: Indicates bridge connectivity status
- **RTC Integration**: Offline timekeeping when NTP unavailable
- **Mesh Time Sync**: Built-in mesh time synchronization protocol

## Version History

- **v1.8.1**: Initial implementation of NTP time sync feature
- Type ID 614 allocated for TIME_SYNC_NTP
- Examples and documentation added

## References

- Issue: Enhancement: Bridge-to-Mesh NTP Time Distribution
- Examples: `examples/ntpTimeSyncBridge/` and `examples/ntpTimeSyncNode/`
- Tests: `test/catch/catch_alteriom_packages.cpp`
- Package: `examples/alteriom/alteriom_sensor_package.hpp`
