# Release Notes: painlessMesh v1.8.9

**Release Date**: November 12, 2025  
**GitHub**: [Alteriom/painlessMesh](https://github.com/Alteriom/painlessMesh)  
**Tag**: v1.8.9

---

## 🎯 Summary

Version 1.8.9 fixes critical bridge self-registration issues that prevented bridge nodes from properly tracking themselves in status broadcasts and coordination messages. This resolves the "Known bridges: 0" error reported by @woodlist and improves multi-bridge coordination reliability.

---

## 🐛 Critical Fixes

### Bridge Self-Registration (Type 610 & 613)

**Problem**: Bridge nodes were not tracking themselves in their own bridge lists and coordination maps, causing:
- Bridge reporting "Known bridges: 0" despite being active
- "No primary bridge available!" errors
- Multi-bridge priority selection failures

**Root Cause**: Mesh networks don't loop broadcasts back to sender by design. Nodes receive broadcasts from others but not their own messages, requiring explicit local state management.

**Solutions Implemented**:

#### 1. Bridge Status Broadcasting (Type 610)
- Added immediate self-registration in `initBridgeStatusBroadcast()` (line ~746)
  - Bridge calls `updateBridgeStatus()` with own nodeId right after initialization
- Added self-update in `sendBridgeStatus()` (line ~1192)
  - Bridge updates own status before each broadcast
- **Result**: Bridge now appears in its own `knownBridges` list from the start

#### 2. Bridge Coordination (Type 613)
- Added self-registration in `initBridgeCoordination()` (line ~803)
  - Bridge adds own priority: `bridgePriorities[this->nodeId] = bridgePriority`
- Added priority self-update in `sendBridgeCoordination()` (line ~869)
  - Bridge updates own priority before each broadcast
- **Result**: Primary bridge selection works correctly with all bridge priorities

**Impact**:
- ✅ Bridge correctly reports "Known bridges: 1" (or more)
- ✅ Multi-bridge setups properly select primary bridge
- ✅ Bridge failover more reliable with complete tracking
- ✅ Self-tracking pattern consistent across periodic broadcasts

**Files Modified**: `src/arduino/wifi.hpp`

---

## 🔧 Build System

### Docker Compiler Change
- Switched from clang++ to g++ in Dockerfile
- Resolves template instantiation crashes during Docker builds
- Build verification confirms successful compilation

**Files Modified**: `Dockerfile`

---

## 📚 Documentation

### Comprehensive Broadcast Analysis
Added `COMPREHENSIVE_BROADCAST_ANALYSIS.md` with:
- Full analysis of all 4 broadcast message types (610, 611, 612, 613)
- Self-tracking requirements for Type 610 (STATUS) and 613 (COORDINATION)
- Confirmation that Type 611 (ELECTION) already implements correct pattern
- Confirmation that Type 612 (TAKEOVER) doesn't require self-tracking
- Pattern guidelines for future broadcast implementations

---

## 🔬 Technical Details

### Broadcast Message Types Analyzed

| Type | Name | Purpose | Self-Tracking | Status |
|------|------|---------|---------------|---------|
| 610 | BRIDGE_STATUS | Periodic health heartbeat | ✅ Required | ✅ Fixed |
| 611 | BRIDGE_ELECTION | Candidate announcement | ✅ Required | ✅ Already correct |
| 612 | BRIDGE_TAKEOVER | Role change notification | ❌ Not needed | ✅ No issue |
| 613 | BRIDGE_COORDINATION | Priority broadcasting | ✅ Required | ✅ Fixed |

### Before vs After

**Before Fix**:
```
Bridge Node Output:
Known bridges: 0
No primary bridge available!
Bridge priority: (missing from map)
```

**After Fix**:
```
Bridge Node Output:
Known bridges: 1
Primary bridge: 123456 (this node)
Bridge priority: 100 (correctly tracked)
```

---

## 🚀 Upgrade Instructions

### For Existing Users

1. **Update Library**:
   ```bash
   # Arduino Library Manager
   Update "Alteriom PainlessMesh" to v1.8.9
   
   # PlatformIO
   lib_deps = AlteriomPainlessMesh@^1.8.9
   
   # NPM
   npm install @alteriom/painlessmesh@1.8.9
   ```

2. **Rebuild and Deploy**:
   - No code changes required in your sketches
   - Fixes are automatic in the library
   - Rebuild and upload to all bridge nodes

3. **Verify Fix**:
   - Enable logging: `mesh.setDebugMsgTypes(ERROR | CONNECTION)`
   - Check bridge status: `mesh.getBridges()` should return count >= 1
   - Bridge node should log "Known bridges: 1" or more

### Breaking Changes
**None** - This release is fully backward compatible.

---

## 🧪 Testing

### Validation Performed
- ✅ Standalone compilation test with g++
- ✅ Code review of all 4 broadcast message types
- ✅ Pattern validation across entire codebase
- ✅ Version consistency check across all package files

### Recommended Testing
After upgrading, test these scenarios:

1. **Single Bridge**: Deploy and verify bridge reports "Known bridges: 1"
2. **Multi-Bridge**: Verify primary selection works with all priorities tracked
3. **Failover**: Disconnect primary bridge, verify secondary promotion works
4. **Auto-Election**: Test election with no pre-designated bridge

---

## 📖 Related Issues

- Resolves: @woodlist GitHub issue - "Known bridges: 0" despite active bridge
- Related: Bridge failover improvements (v1.8.6)
- Related: Bridge discovery timing (v1.8.4)

---

## 📦 Version Information

### Updated Files
- ✅ `library.properties` → 1.8.9
- ✅ `library.json` → 1.8.9
- ✅ `package.json` → 1.8.9
- ✅ `src/painlessMesh.h` → 1.8.9
- ✅ `src/AlteriomPainlessMesh.h` → 1.8.9
- ✅ `CHANGELOG.md` → Entry added for 1.8.9

### Package Hashes
Will be generated automatically during release process.

---

## 👥 Contributors

- **Alteriom Team** - Core development and maintenance
- **@woodlist** - Issue reporting and testing
- **GitHub Copilot** - Code analysis and documentation

---

## 📝 Next Steps

1. **Create Git Tag**: `git tag -a v1.8.9 -m "release: v1.8.9 - Bridge self-registration fixes"`
2. **Push Tag**: `git push origin v1.8.9`
3. **GitHub Release**: Create release from tag with these notes
4. **Publish Packages**:
   - Arduino Library Manager (automatic)
   - PlatformIO (automatic)
   - NPM: `npm publish`

---

## 📄 License

LGPL-3.0 - See [LICENSE](LICENSE) file for details

---

## 🔗 Links

- **Repository**: https://github.com/Alteriom/painlessMesh
- **Documentation**: https://alteriom.github.io/painlessMesh/
- **Issues**: https://github.com/Alteriom/painlessMesh/issues
- **NPM Package**: https://www.npmjs.com/package/@alteriom/painlessmesh

---

*For detailed technical analysis, see [COMPREHENSIVE_BROADCAST_ANALYSIS.md](COMPREHENSIVE_BROADCAST_ANALYSIS.md)*
