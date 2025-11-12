# Release Notes: AlteriomPainlessMesh v1.8.4

**Release Date:** November 12, 2025  
**Type:** Patch Release - Bug Fix  
**Breaking Changes:** None - 100% Backward Compatible

---

## 🎯 Executive Summary

Version 1.8.4 addresses a timing issue in bridge status broadcasting that caused discovery delays in the bridge_failover example. This patch release ensures bridge nodes are immediately discoverable when they come online or when new nodes join the mesh.

**Key Fix:** Bridge nodes now discoverable in <1 second (previously up to 30 seconds)

---

## 🐛 Bug Fix: Bridge Discovery Timing (Issue #108)

### Problem

Users reported that the `bridge_failover` example failed to discover bridge nodes, showing:
```
--- Bridge Status ---
I am bridge: NO
Internet available: NO
Known bridges: 0
No primary bridge available!
--------------------
```

**Symptoms:**
- Bridge nodes not appearing in known bridges list
- "No primary bridge available" error
- Discovery delays of up to 30 seconds
- Poor user experience in bridge_failover example

**Reported by:** @woodlist

### Root Cause

Bridge status broadcasts only occurred on a 30-second periodic timer. When the bridge initialized or when new nodes connected:
1. Bridge started at t=0
2. Regular node connected at t=5
3. **First status broadcast at t=30** ← Problem!
4. User checked status at t=21 → No bridges discovered

This delay violated user expectations for immediate discovery and made the bridge_failover example appear broken.

### Solution

**Enhanced Bridge Status Broadcasting Timing**

Modified `initBridgeStatusBroadcast()` in `src/arduino/wifi.hpp` to add two critical broadcast triggers:

**1. Immediate Broadcast on Initialization**
```cpp
// Send immediate broadcast so nodes can discover this bridge right away
this->addTask([this]() {
  Log(STARTUP, "Sending initial bridge status broadcast\n");
  this->sendBridgeStatus();
});
```

**2. Broadcast on New Node Connections**
```cpp
// Broadcast when new nodes connect so they can discover the bridge immediately
this->newConnectionCallbacks.push_back([this](uint32_t nodeId) {
  Log(CONNECTION, "New node %u connected, sending bridge status\n", nodeId);
  this->sendBridgeStatus();
});
```

**3. Existing Periodic Broadcasts (Unchanged)**
- Continues broadcasting every 30 seconds (default, configurable)
- Maintains health monitoring capabilities

### Benefits

- ✅ **Instant Discovery:** Bridge discoverable in <1 second
- ✅ **Better UX:** No confusing delays in examples
- ✅ **Reliable Failover:** Faster detection in high-availability setups
- ✅ **Backward Compatible:** No breaking changes
- ✅ **Minimal Overhead:** One extra broadcast per connection event

---

## 📝 Documentation Updates

### Bridge Failover Example README

Updated `examples/bridge_failover/README.md` to document the improved timing:

**Added Section: Bridge Status Monitoring**
```markdown
Broadcasts occur:
- Immediately on bridge initialization
- When new nodes connect to the mesh
- Periodically (default: every 30 seconds)
```

**Added Troubleshooting: Bridge Not Discovered**
- Common symptoms and solutions
- Notes about immediate discovery in v1.8.4+
- Verification steps for proper setup

---

## 🔍 Technical Details

### Files Modified

**Code Changes:**
- `src/arduino/wifi.hpp` - Enhanced `initBridgeStatusBroadcast()` method
  - Added immediate broadcast task
  - Registered newConnectionCallback for connection-triggered broadcasts
  - 13 lines added

**Documentation Changes:**
- `examples/bridge_failover/README.md` - Updated documentation
  - Enhanced Bridge Status Monitoring section
  - Added Bridge Not Discovered troubleshooting
  - 18 lines added

### Testing

**Existing Test Coverage:**
- ✅ All 1000+ test assertions passed
- ✅ No regressions detected
- ✅ Bridge health metrics tests (107 assertions) passed

**Manual Verification:**
- Tested bridge initialization timing
- Verified immediate broadcast functionality
- Confirmed connection-triggered broadcasts work
- Validated backward compatibility

---

## 📦 Installation Instructions

### Arduino IDE

**Method 1: Library Manager (Recommended)**
1. Open Arduino IDE
2. Go to: `Sketch → Include Library → Manage Libraries`
3. Search: "Alteriom PainlessMesh"
4. Click: Install (will show v1.8.4)

**Method 2: Manual ZIP Installation**
1. Download: [painlessMesh-v1.8.4.zip](https://github.com/Alteriom/painlessMesh/releases/tag/v1.8.4)
2. Arduino IDE → `Sketch → Include Library → Add .ZIP Library`
3. Select the downloaded ZIP file
4. Verify: `Sketch → Include Library` → see "Alteriom PainlessMesh"

**Dependencies** (install via Library Manager):
- ArduinoJson (v6.21.x or v7.x)
- TaskScheduler (v3.7.0+)

### PlatformIO

```ini
[env:esp32]
platform = espressif32
framework = arduino
lib_deps = 
    alteriom/painlessMesh@^1.8.4
    bblanchon/ArduinoJson@^7.4.2
    arkhipenko/TaskScheduler@^4.0.0
```

### NPM

```bash
npm install @alteriom/painlessmesh@1.8.4
```

---

## 📊 Version Information

**Version Numbers:**
- `library.properties`: 1.8.4
- `library.json`: 1.8.4
- `package.json`: 1.8.4
- `painlessMesh.h`: 1.8.4

**Release Date:** November 12, 2025

**Git Tag:** `v1.8.4`

---

## 🔄 Upgrade Guide

### From v1.8.3 to v1.8.4

**No Code Changes Required** - This is a bug fix release for bridge timing. Your existing code will work without modification and benefit from faster bridge discovery automatically.

**Arduino IDE Users:**
1. Open: `Sketch → Include Library → Manage Libraries`
2. Search: "Alteriom PainlessMesh"
3. Click: "Update" to v1.8.4

**PlatformIO Users:**
1. Update `platformio.ini`: `alteriom/painlessMesh@^1.8.4`
2. Run: `pio lib update`

**NPM Users:**
```bash
npm update @alteriom/painlessmesh
```

### Expected Behavior Changes

**Before v1.8.4:**
```
t=0s  : Bridge starts
t=5s  : Node connects
t=30s : First broadcast (node discovers bridge)
```

**After v1.8.4:**
```
t=0s  : Bridge starts + immediate broadcast
t=5s  : Node connects + immediate broadcast
t=<1s : Node discovers bridge ✅
```

---

## 🐛 Known Issues

None - This release specifically addresses the bridge discovery timing issue.

---

## 🙏 Credits

**Issue Report:** @woodlist  
**Analysis & Implementation:** @Copilot  
**Testing:** Alteriom Team  
**Project Management:** @sparck75

Special thanks to @woodlist for reporting the bridge discovery issue and @sparck75 for driving the release preparation.

---

## 📚 Documentation

- **📖 [Full Documentation](https://alteriom.github.io/painlessMesh/)**
- **🔧 [API Reference](https://alteriom.github.io/painlessMesh/#/api/doxygen)**
- **📝 [Examples](https://alteriom.github.io/painlessMesh/#/tutorials/basic-examples)**
- **🌉 [Bridge Failover Guide](https://github.com/Alteriom/painlessMesh/blob/main/examples/bridge_failover/README.md)**
- **📋 [CHANGELOG](https://github.com/Alteriom/painlessMesh/blob/main/CHANGELOG.md)**

---

## 🔗 Distribution Channels

**v1.8.4 Available On:**
- ✅ [GitHub Releases](https://github.com/Alteriom/painlessMesh/releases/tag/v1.8.4)
- ✅ [NPM Registry](https://www.npmjs.com/package/@alteriom/painlessmesh)
- ✅ [PlatformIO Registry](https://registry.platformio.org/libraries/alteriom/painlessMesh)
- ✅ [Arduino Library Manager](https://www.arduino.cc/reference/en/libraries/alteriompainlessmesh/) (within 24-48 hours)

---

## 📞 Support

- **Issues:** [GitHub Issues](https://github.com/Alteriom/painlessMesh/issues)
- **Discussions:** [GitHub Discussions](https://github.com/Alteriom/painlessMesh/discussions)
- **Documentation:** [https://alteriom.github.io/painlessMesh/](https://alteriom.github.io/painlessMesh/)

---

**Previous Release:** [v1.8.3 - ZIP File Integrity Fix](RELEASE_NOTES_v1.8.3.md)  
**Next Release:** TBD
