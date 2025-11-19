# RTC Integration Example

This example demonstrates how to integrate Real-Time Clock (RTC) modules with painlessMesh for accurate offline timekeeping.

## Problem Statement

When Internet or bridge connectivity is unavailable, mesh nodes lose access to accurate time synchronization. This is problematic for applications that require valid timestamps, such as:

- Fish farm alarm systems (regulatory compliance)
- Environmental monitoring with offline data logging
- Industrial IoT with intermittent connectivity
- Remote sensor networks

## Solution

painlessMesh RTC integration provides:

✅ **Accurate timestamps during offline periods**  
✅ **Automatic NTP sync when Internet available**  
✅ **Graceful fallback to mesh time**  
✅ **Support for common RTC modules**

## Supported RTC Modules

- **DS3231** - High accuracy I2C RTC with temperature compensation
- **DS1307** - Basic I2C RTC
- **PCF8523** - Low power I2C RTC
- **PCF8563** - Ultra-low power I2C RTC
- **ESP32 Internal RTC** - Built-in ESP32 RTC (requires external battery backup)

## Hardware Requirements

### For DS3231 (used in this example)
- ESP32 or ESP8266 board
- DS3231 RTC module
- Connections:
  - SDA → GPIO 21 (ESP32) or GPIO 4 (ESP8266)
  - SCL → GPIO 22 (ESP32) or GPIO 5 (ESP8266)
  - VCC → 3.3V
  - GND → GND

### Library Requirements
```
painlessMesh
RTClib (Adafruit)
```

Install via Arduino Library Manager:
- Tools → Manage Libraries
- Search for "RTClib" by Adafruit
- Click Install

## How It Works

### 1. RTC Interface Implementation

You implement the `RTCInterface` for your specific RTC hardware:

```cpp
class DS3231Interface : public painlessmesh::rtc::RTCInterface {
  // Implement begin(), isAvailable(), getUnixTime(), setUnixTime(), getType()
};
```

### 2. Enable RTC

```cpp
DS3231Interface rtcInterface(&rtc);
mesh.enableRTC(&rtcInterface);
```

### 3. Automatic Time Management

```cpp
mesh.onBridgeStatusChanged([](uint32_t bridgeNodeId, bool hasInternet) {
  if (hasInternet) {
    // Sync RTC from NTP when Internet available
    uint32_t ntpTime = getNTPTime();
    mesh.syncRTCFromNTP(ntpTime);
  }
  // Offline: RTC maintains accurate time
});
```

### 4. Get Accurate Time

```cpp
// Prefers RTC, falls back to mesh time
uint32_t timestamp = mesh.getAccurateTime();
```

## API Reference

### Mesh Methods

#### `bool enableRTC(rtc::RTCInterface* rtcInterface)`
Enable RTC integration with a user-provided interface.

**Returns:** `true` if RTC initialized successfully, `false` otherwise

#### `void disableRTC()`
Disable RTC integration.

#### `bool syncRTCFromNTP(uint32_t ntpTimestamp)`
Sync RTC from NTP/Internet time source.

**Parameters:**
- `ntpTimestamp` - Unix timestamp from NTP

**Returns:** `true` if sync successful, `false` otherwise

#### `uint32_t getAccurateTime()`
Get accurate time with RTC fallback.

**Returns:** Unix timestamp (seconds) from RTC, or mesh time (microseconds) if RTC unavailable

#### `bool hasRTC()`
Check if RTC is enabled and available.

**Returns:** `true` if RTC can be used, `false` otherwise

#### `rtc::RTCType getRTCType()`
Get RTC module type.

**Returns:** `RTCType` enum value

#### `uint32_t getTimeSinceRTCSync()`
Get time since last RTC sync.

**Returns:** Milliseconds since last sync, or 0 if never synced

#### `void onRTCSyncComplete(rtcSyncCompleteCallback_t callback)`
Set callback for RTC sync completion.

### RTC Types

```cpp
enum RTCType {
  RTC_NONE = 0,
  RTC_DS3231 = 1,
  RTC_DS1307 = 2,
  RTC_PCF8523 = 3,
  RTC_PCF8563 = 4,
  RTC_ESP32_INTERNAL = 5
};
```

## Usage Example

```cpp
#include "painlessMesh.h"
#include <RTClib.h>

painlessMesh mesh;
RTC_DS3231 rtc;
DS3231Interface rtcInterface(&rtc);

void setup() {
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onBridgeStatusChanged(&bridgeStatusCallback);
  mesh.onRTCSyncComplete(&rtcSyncCompleteCallback);
  
  if (mesh.enableRTC(&rtcInterface)) {
    Serial.println("RTC enabled!");
  }
}

void bridgeStatusCallback(uint32_t bridgeNodeId, bool hasInternet) {
  if (hasInternet && mesh.hasRTC()) {
    uint32_t ntpTime = getNTPTime();  // Your implementation
    mesh.syncRTCFromNTP(ntpTime);
  }
}

void rtcSyncCompleteCallback(uint32_t timestamp) {
  Serial.printf("RTC synced to: %u\n", timestamp);
}
```

## Testing

### Verify RTC Time Persistence
1. Upload sketch to node
2. Disconnect Internet/bridge
3. Power cycle the node
4. Verify timestamps remain accurate (±2 seconds)

### Verify NTP Sync
1. Connect bridge to Internet
2. Monitor serial output for sync messages
3. Verify RTC time updated correctly

### Verify Offline Operation
1. Disconnect Internet
2. Wait several hours
3. Verify timestamps continue to increment accurately

## Troubleshooting

### "Couldn't find RTC"
- Check I2C connections (SDA/SCL)
- Verify RTC module has power
- Try I2C scanner sketch to detect device

### "RTC lost power"
- RTC battery needs replacement
- Time will be synced from NTP when available

### Time Not Syncing
- Verify bridge has Internet connectivity
- Check NTP server is accessible
- Ensure `syncRTCFromNTP()` called with valid timestamp

## Time Authority

painlessMesh v1.8.12+ includes **time authority** support to prevent nodes from adopting incorrect time from nodes without accurate time sources.

### How It Works

Nodes with time authority (RTC or Internet) are prioritized during mesh time synchronization:
- Nodes **without** time authority will adopt time from nodes **with** time authority
- Nodes **with** time authority will **NOT** adopt time from nodes without
- When both nodes have same authority status, existing subnet/node ID logic applies

### Setting Time Authority

Time authority is automatically set when:
- RTC is enabled via `enableRTC()` (time authority = true)
- RTC is disabled via `disableRTC()` (time authority = false)

For bridge nodes with Internet, set time authority manually:

```cpp
void bridgeStatusCallback(uint32_t bridgeNodeId, bool hasInternet) {
  // If THIS node is the bridge, update time authority
  if (bridgeNodeId == mesh.getNodeId()) {
    if (hasInternet) {
      mesh.setTimeAuthority(true);  // Internet available
    } else if (!mesh.hasRTC()) {
      mesh.setTimeAuthority(false); // No Internet and no RTC
    }
  }
}
```

### Checking Time Authority

```cpp
if (mesh.getTimeAuthority()) {
  Serial.println("This node has authoritative time source");
}
```

### Use Cases

**Scenario 1: Mixed RTC nodes**
- Node A: Has RTC (time authority = true)
- Node B: No RTC (time authority = false)
- Result: Node B adopts time from Node A ✓

**Scenario 2: Bridge with Internet**
- Node A: Bridge with Internet (time authority = true)
- Node B: Regular node (time authority = false)
- Result: Node B adopts time from bridge ✓

**Scenario 3: Network split**
- Subnet A: All nodes have RTC
- Subnet B: No nodes have RTC
- Result: When subnets reconnect, Subnet B adopts from Subnet A ✓

## Best Practices

1. **Always check RTC availability** before relying on timestamps
2. **Sync regularly** when Internet available (recommended: every 24 hours)
3. **Monitor battery** on RTC modules for continuous operation
4. **Implement fallback** to mesh time if RTC fails
5. **Log sync events** for debugging and maintenance
6. **Set time authority** for bridge nodes when Internet is available
7. **Use RTC on at least one node** in offline deployments for accurate timestamps

## Regulatory Compliance

For systems requiring timestamp accuracy (e.g., fish farm alarms):

- RTC provides ±2 second accuracy during offline periods
- Timestamps remain valid for alarm reporting
- Sync logs provide audit trail
- Battery backup ensures continuous operation

## See Also

- [Bridge Status Feature](../../BRIDGE_STATUS_FEATURE.md) - Internet connectivity detection
- [Bridge Architecture](../../BRIDGE_ARCHITECTURE_IMPLEMENTATION.md) - Mesh bridge setup
- [painlessMesh Documentation](https://gitlab.com/painlessMesh/painlessMesh) - Main library docs
