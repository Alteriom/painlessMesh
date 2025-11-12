## 🐛 Bug Fixes

### Bridge Failover Auto-Election (Issue #117)

Fixed critical issue where mesh networks would remain bridgeless indefinitely when all nodes started without a designated initial bridge.

**Before:** No initial bridge → no election → mesh stays bridgeless forever  
**After:** No initial bridge → 60s startup → monitoring detects absence → election triggered → best RSSI node becomes bridge

#### Implementation Details
- Added periodic monitoring task (30s interval) that detects absence of healthy bridge
- Activates after 60s startup grace period to allow network stabilization
- Randomized election delay (1-3s) prevents thundering herd problem
- Respects existing safeguards: election state, 60s cooldown, router visibility

Fully backward compatible - pre-designated bridge mode continues to work as before.

**Core fix:** `src/arduino/wifi.hpp`

## 📝 Documentation

### Bridge Failover Example

Enhanced documentation to clarify two deployment modes:

- **Auto-Election Mode**: All nodes regular (`INITIAL_BRIDGE=false`), RSSI-based election after 60s
- **Pre-Designated Mode**: Traditional single initial bridge setup

Updated:
- `examples/bridge_failover/bridge_failover.ino` - Enhanced header comments
- `examples/bridge_failover/README.md` - Comprehensive auto-election documentation

## 🔧 Housekeeping

- Synchronized `package-lock.json` version to 1.8.5

## 📦 Installation

### PlatformIO
```ini
lib_deps = alteriom/AlteriomPainlessMesh@^1.8.6
```

### NPM
```bash
npm install @alteriom/painlessmesh@1.8.6
```

### Arduino Library Manager
Search for "AlteriomPainlessMesh" and update through the IDE

## 🎯 Use Cases

This release is valuable for:
- Dynamic mesh networks with changing node positions
- Fault-tolerant deployments requiring automatic bridge recovery
- IoT systems with minimal human oversight
- Development environments without pre-configured bridges

## 🙏 Credits

Special thanks to @woodlist for reporting the issue and providing detailed logs.

## 📚 Resources

- [Full Release Notes](https://github.com/Alteriom/painlessMesh/blob/main/docs/releases/RELEASE_NOTES_v1.8.6.md)
- [Bridge Failover Example](https://github.com/Alteriom/painlessMesh/tree/main/examples/bridge_failover)
- [Issue #117](https://github.com/Alteriom/painlessMesh/issues/117)
- [Pull Request #118](https://github.com/Alteriom/painlessMesh/pull/118)

**Full Changelog**: https://github.com/Alteriom/painlessMesh/compare/v1.8.5...v1.8.6
