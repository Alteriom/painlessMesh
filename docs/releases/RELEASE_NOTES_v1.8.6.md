# painlessMesh v1.8.6 Release Notes

**Release Date:** November 12, 2025  
**Type:** Patch Release  
**Breaking Changes:** None - 100% Backward Compatible

---

## 🎯 Executive Summary

Version 1.8.6 fixes a critical issue in the bridge failover mechanism where mesh networks would remain bridgeless indefinitely when all nodes started without a designated initial bridge. This release introduces automatic bridge election monitoring that ensures meshes always establish internet connectivity when configured for bridge failover.

---

## 🐛 Bug Fixes

### Bridge Failover Auto-Election (Issue #117)

**Problem:** The bridge failover example didn't work when all nodes had `INITIAL_BRIDGE=false`. The mesh would start but never elect a bridge, leaving the network without internet connectivity indefinitely.

**Symptoms:**
```
--- Bridge Status ---
I am bridge: NO
Internet available: NO
Known bridges: 0
No primary bridge available!
--------------------
```

**Root Cause:** Bridge elections only triggered when an *existing* bridge lost connectivity. When the mesh had no bridge from startup, no election was ever initiated.

**Solution:** Added periodic monitoring task (30-second interval) that:
- Detects absence of healthy bridges in the mesh
- Triggers bridge election after 60-second startup grace period
- Uses randomized election delay (1-3 seconds) to prevent thundering herd
- Respects all existing safeguards (election state, cooldown, router visibility)

**Behavior Changes:**

| Scenario | Before v1.8.6 | After v1.8.6 |
|----------|---------------|--------------|
| No initial bridge | Mesh stays bridgeless forever | Election triggered after 60s, best RSSI node becomes bridge |
| Pre-designated bridge | Works normally | Works normally (unchanged) |
| Bridge failure | Election triggered | Election triggered (unchanged) |

**Backward Compatibility:** Fully backward compatible. Pre-designated bridge mode continues to work exactly as before.

**Core Changes:** `src/arduino/wifi.hpp`

---

## 📝 Documentation Improvements

### Bridge Failover Example Clarification

Enhanced documentation for `examples/bridge_failover/` to clearly explain two deployment modes:

**Auto-Election Mode** (New Documentation)
- All nodes start with `INITIAL_BRIDGE=false`
- After 60-second startup grace period, monitoring begins
- Best RSSI node automatically elected as bridge
- Ideal for dynamic deployments where optimal bridge selection matters

**Pre-Designated Mode** (Existing)
- One node starts with `INITIAL_BRIDGE=true`
- Traditional setup with guaranteed initial bridge
- Ideal for fixed deployments with known bridge location

Updated files:
- `examples/bridge_failover/bridge_failover.ino` - Enhanced header comments
- `examples/bridge_failover/README.md` - Comprehensive mode documentation

---

## 🔧 Housekeeping

- Synchronized `package-lock.json` version to 1.8.5

---

## 🔄 Upgrade Guide

### From v1.8.5 to v1.8.6

This is a seamless upgrade with no breaking changes:

1. **Update Package Version:**
   ```bash
   # PlatformIO
   pio pkg update alteriom/AlteriomPainlessMesh@^1.8.6
   
   # NPM
   npm update @alteriom/painlessmesh
   
   # Arduino Library Manager
   # Update through IDE's Library Manager
   ```

2. **No Code Changes Required** - All existing code works without modification

3. **Optional: Enable Auto-Election Mode**
   ```cpp
   // For auto-election mode, set all nodes to:
   #define INITIAL_BRIDGE false
   
   // Network will automatically elect bridge after 60s startup
   ```

---

## 🎯 Use Cases

This release is particularly valuable for:

### 1. Dynamic Mesh Networks
- Mobile or portable mesh deployments
- Networks where node positions change
- Optimal bridge selection based on WiFi signal strength

### 2. Fault-Tolerant Deployments
- Meshes that must establish connectivity without manual intervention
- Systems requiring automatic bridge recovery
- IoT deployments with minimal human oversight

### 3. Development & Testing
- Quick prototyping without pre-configuring bridge nodes
- Testing scenarios with varying network topologies
- Validation of bridge election algorithms

---

## 📊 Technical Details

### Monitoring Task Implementation

```cpp
// Periodic check every 30 seconds
this->addTask(30000, TASK_FOREVER, [this]() {
  // Skip if: not enabled, already bridge, or in startup grace period
  if (!bridgeFailoverEnabled || !routerCredentialsConfigured) return;
  if (this->isBridge() || millis() < 60000) return;
  
  // Check for healthy bridge
  bool hasHealthyBridge = false;
  for (const auto& bridge : this->getBridges()) {
    if (bridge.isHealthy(bridgeTimeoutMs) && bridge.internetConnected) {
      hasHealthyBridge = true;
      break;
    }
  }
  
  // Trigger election with randomized delay if no bridge found
  if (!hasHealthyBridge) {
    uint32_t delay = random(1000, 3000);
    this->addTask(delay, TASK_ONCE, [this]() {
      this->startBridgeElection();
    });
  }
});
```

### Safeguards

1. **60-second startup grace period** - Allows network to stabilize
2. **Randomized election delay** - Prevents simultaneous elections
3. **Respects election state** - Won't trigger if election already in progress
4. **Cooldown period** - Enforces 60-second minimum between elections
5. **Router visibility check** - Only nodes that can see router participate

---

## 🙏 Credits

Special thanks to @woodlist for reporting Issue #117 and providing detailed logs that helped identify the root cause.

---

## 📦 Distribution

This release is available through:
- **GitHub Releases**: [v1.8.6](https://github.com/Alteriom/painlessMesh/releases/tag/v1.8.6)
- **NPM**: `npm install @alteriom/painlessmesh@1.8.6`
- **PlatformIO**: `alteriom/AlteriomPainlessMesh@^1.8.6`
- **Arduino Library Manager**: Search for "AlteriomPainlessMesh"

---

## 📚 Additional Resources

- [Bridge Failover Example](https://github.com/Alteriom/painlessMesh/tree/main/examples/bridge_failover)
- [Bridge Failover Documentation](https://github.com/Alteriom/painlessMesh/blob/main/examples/bridge_failover/README.md)
- [Issue #117 - Bug Report](https://github.com/Alteriom/painlessMesh/issues/117)
- [Pull Request #118 - Implementation](https://github.com/Alteriom/painlessMesh/pull/118)
- [Full Changelog](https://github.com/Alteriom/painlessMesh/blob/main/CHANGELOG.md)

---

## 🐛 Found a Bug?

Please report issues at: https://github.com/Alteriom/painlessMesh/issues

---

**Full Changelog**: [v1.8.5...v1.8.6](https://github.com/Alteriom/painlessMesh/compare/v1.8.5...v1.8.6)
