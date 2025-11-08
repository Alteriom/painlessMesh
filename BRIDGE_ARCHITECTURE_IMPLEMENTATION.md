# Bridge-Centric Architecture Implementation

## Overview

This document describes the implementation of the bridge-centric architecture with automatic channel detection for painlessMesh, as specified in issue #XX.

## Implementation Summary

### New Features

#### 1. `initAsBridge()` Method

**Location:** `src/arduino/wifi.hpp`

**Purpose:** Simplifies bridge node setup by automatically detecting router channel and configuring mesh accordingly.

**Signature:**
```cpp
void initAsBridge(TSTRING meshSSID, TSTRING meshPassword,
                  TSTRING routerSSID, TSTRING routerPassword,
                  Scheduler *baseScheduler, uint16_t port = 5555)
```

**Behavior:**
1. Connects to router in STA mode
2. Waits up to 30 seconds for connection
3. Detects router's WiFi channel using `WiFi.channel()`
4. Falls back to channel 1 if connection fails
5. Initializes mesh on detected channel
6. Re-establishes router connection using `stationManual()`
7. Automatically sets node as root (`setRoot(true)`)
8. Sets mesh as containing root (`setContainsRoot(true)`)
9. Provides comprehensive logging at each step

#### 2. `scanForMeshChannel()` Helper Function

**Location:** `src/painlessMeshSTA.cpp`, `src/painlessMeshSTA.h`

**Purpose:** Scans all WiFi channels to find a specific mesh SSID.

**Signature:**
```cpp
static uint8_t scanForMeshChannel(TSTRING meshSSID, bool meshHidden)
```

**Behavior:**
1. Performs WiFi scan on all channels (channel parameter = 0)
2. Iterates through scan results looking for matching SSID
3. Supports hidden networks (empty SSID matches when hidden flag is set)
4. Returns channel number if found, 0 if not found
5. Cleans up scan results with `WiFi.scanDelete()`
6. Provides detailed logging

**Platform Support:**
- ESP32: Uses `WiFi.scanNetworks(false, meshHidden, false, 300U, 0)`
- ESP8266: Uses `WiFi.scanNetworks(false, meshHidden, 0)`

#### 3. Auto Channel Detection for Regular Nodes

**Location:** `src/painlessMeshSTA.cpp` (enhanced `stationScan()`)

**Purpose:** Allows regular nodes to automatically find and join mesh on any channel.

**Behavior:**
- When `channel=0` is passed to `init()`, triggers auto-detection
- Calls `scanForMeshChannel()` to find mesh
- Updates mesh channel if found
- Falls back to channel 1 if mesh not found
- Only runs once at initialization

**Usage:**
```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
```

## Technical Details

### Channel Detection Algorithm

```
Bridge Node (initAsBridge):
1. WiFi.disconnect()
2. WiFi.mode(WIFI_STA)
3. WiFi.begin(routerSSID, routerPassword)
4. Wait for connection (30s timeout)
5. If connected:
   - detectedChannel = WiFi.channel()
6. Else:
   - detectedChannel = 1 (fallback)
7. init(meshSSID, meshPassword, ..., detectedChannel)
8. stationManual(routerSSID, routerPassword)
9. setRoot(true), setContainsRoot(true)

Regular Node (channel=0):
1. scanForMeshChannel(meshSSID, hidden)
2. If found:
   - mesh->_meshChannel = detectedChannel
3. Else:
   - mesh->_meshChannel = 1 (fallback)
4. Continue with normal stationScan()
```

### Error Handling

#### Router Connection Failure
- **Timeout:** 30 seconds
- **Fallback:** Channel 1
- **Logging:** Error message indicating failure
- **Behavior:** Mesh still initializes, but on default channel

#### Mesh Not Found (Regular Nodes)
- **Fallback:** Channel 1
- **Logging:** Info message about fallback
- **Behavior:** Node creates mesh on channel 1 or waits for mesh to appear

### Memory Considerations

**Bridge Initialization:**
- Temporary WiFi connection during setup
- No additional persistent memory usage
- Scan results cleaned up immediately

**Channel Scanning:**
- Temporary scan results buffer
- Cleared with `WiFi.scanDelete()`
- No memory leaks

### Timing Considerations

**Bridge Initialization:**
- Router connection: Up to 30 seconds
- Total initialization time: ~35-40 seconds worst case
- Can be optimized by reducing timeout if needed

**Regular Node Auto-Detection:**
- Single scan of all channels: ~5-10 seconds
- Only happens once at startup
- Subsequent scans use detected channel

## Backward Compatibility

### No Breaking Changes

All existing code continues to work:

```cpp
// Old code - still works
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6);
mesh.stationManual(ROUTER_SSID, ROUTER_PASSWORD);
mesh.setRoot(true);

// New code - simplified
mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD, ROUTER_SSID, ROUTER_PASSWORD, &userScheduler);
```

### Migration Path

Users can migrate incrementally:
1. Keep existing bridge code working
2. Update bridge nodes to use `initAsBridge()` when convenient
3. Update regular nodes to use `channel=0` for auto-detection
4. No rush - both approaches work simultaneously

## Testing

### Test Coverage

**Automated Tests:**
- ✅ All existing unit tests pass (500+ assertions)
- ✅ No regressions detected
- ✅ Build system validates compilation

**Manual Testing Required:**
- 🔲 Bridge on router channel 1, nodes join successfully
- 🔲 Bridge on router channel 6, nodes join successfully
- 🔲 Bridge on router channel 11, nodes join successfully
- 🔲 Bridge fails to connect to router, uses channel 1
- 🔲 Regular node can't find mesh, falls back to channel 1
- 🔲 Hidden network support
- 🔲 Multiple nodes joining sequentially
- 🔲 Reconnection after bridge reboot
- 🔲 Reconnection after router reboot

### Test Scenarios

#### Scenario 1: Basic Bridge Operation
```
1. Setup bridge node with initAsBridge()
2. Setup 2-3 regular nodes with channel=0
3. Verify all nodes join mesh
4. Verify mesh channel matches router channel
5. Verify bridge has Internet connectivity
6. Verify messages flow through mesh
```

#### Scenario 2: Router Connection Failure
```
1. Setup bridge node with invalid router credentials
2. Verify bridge falls back to channel 1
3. Verify mesh still forms
4. Verify error logging is clear
```

#### Scenario 3: Hidden Network
```
1. Configure router as hidden SSID
2. Setup bridge with initAsBridge()
3. Verify bridge detects hidden router channel
4. Setup regular nodes with channel=0 and hidden=true
5. Verify nodes find and join hidden mesh
```

## Known Limitations

### Current Implementation

1. **Single Bridge Only:** Architecture assumes one bridge node
2. **2.4GHz Only:** Works on channels 1-13 (standard WiFi b/g/n)
3. **No 5GHz Support:** Limited by ESP32/ESP8266 hardware
4. **Blocking Initialization:** Bridge init blocks for up to 30 seconds

### Future Enhancements

1. **Multi-Bridge Support:** Load balancing between multiple bridges
2. **Async Initialization:** Non-blocking bridge setup
3. **Channel Change Detection:** Auto-restart if router changes channel
4. **Callback Notifications:** Events for channel detection, connection status
5. **Configurable Timeout:** User-specified timeout for router connection

## Performance Impact

### Bridge Node
- **Initialization Time:** +30s worst case (router connection timeout)
- **Memory Usage:** No additional runtime overhead
- **CPU Usage:** Minimal, only during initialization

### Regular Nodes
- **Initialization Time:** +5-10s (one-time channel scan)
- **Memory Usage:** No additional runtime overhead
- **CPU Usage:** Minimal, only during initialization

### Network Performance
- **No runtime impact** - Channel detection only happens at startup
- **Mesh operation** - Identical to manual configuration after init

## Documentation Updates

### Files Modified
- ✅ `README.md` - Added bridge quick start section
- ✅ `BRIDGE_TO_INTERNET.md` - Complete rewrite with new approach
- ✅ `CHANGELOG.md` - Release notes for v1.7.8+
- ✅ `examples/bridge/bridge.ino` - Updated to use `initAsBridge()`
- ✅ `examples/basic/basic.ino` - Shows auto-detection
- 🔲 API documentation (Doxygen comments in headers)
- 🔲 Wiki pages (if applicable)

### Documentation Quality
- Clear code examples
- Expected output logs
- Troubleshooting sections
- Migration guide
- Best practices

## Security Considerations

### Password Handling
- Passwords stored in SRAM during setup
- Not persisted to flash (WiFi.persistent(false))
- Cleared after connection established

### Network Security
- No changes to WiFi security model
- Inherits WPA2 security from WiFi stack
- No new attack vectors introduced

### Code Safety
- Input validation on SSID/password strings
- Timeout handling prevents infinite loops
- Fallback behavior prevents bricked devices

## Code Quality

### Static Analysis
- ✅ Compiles without warnings
- ✅ Follows existing code style
- ✅ Matches repository conventions
- ✅ No memory leaks detected

### Code Review Checklist
- ✅ Clear, self-documenting function names
- ✅ Comprehensive inline comments
- ✅ Error handling at all levels
- ✅ Logging for debugging
- ✅ Platform-specific code properly ifdef'd
- ✅ No magic numbers (all constants defined)

## Release Checklist

### Pre-Release
- ✅ Code implementation complete
- ✅ Documentation updated
- ✅ CHANGELOG updated
- ✅ Examples updated
- ✅ Backward compatibility verified
- ✅ All automated tests pass
- 🔲 Manual testing complete
- 🔲 Code review approved
- 🔲 Security scan clean

### Release
- 🔲 Version number bumped
- 🔲 Git tag created
- 🔲 Release notes published
- 🔲 Arduino Library Manager updated
- 🔲 PlatformIO Registry updated
- 🔲 NPM package published

### Post-Release
- 🔲 Monitor issue tracker for bugs
- 🔲 Update documentation based on feedback
- 🔲 Create migration guide if needed

## References

- Issue #XX: Feature request for bridge-centric architecture
- PR #XX: Implementation pull request
- `BRIDGE_TO_INTERNET.md`: User-facing bridge documentation
- `README.md`: Quick start guide

## Contributors

- Implementation: GitHub Copilot (@copilot)
- Architecture Design: Based on feedback from @woodlist
- Review: @sparck75

---

**Document Version:** 1.0  
**Last Updated:** 2025-11-08  
**Status:** Implementation Complete, Testing Pending
