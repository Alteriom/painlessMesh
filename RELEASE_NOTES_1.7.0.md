# AlteriomPainlessMesh v1.7.0 Release Notes

**Release Date:** October 15, 2025  
**Schema Version:** @alteriom/mqtt-schema v0.5.0  
**GitHub Release:** https://github.com/Alteriom/painlessMesh/releases/tag/v1.7.0

---

## 🎉 What's New in v1.7.0

Version 1.7.0 brings **Phase 2 features** to AlteriomPainlessMesh with a focus on scalability, professional monitoring, and production-ready stability. This release is 100% backward compatible with v1.6.x while adding powerful new capabilities for enterprise IoT deployments.

### 🚀 Major Features

#### 1. Broadcast OTA Distribution

The most significant feature in v1.7.0 is **Broadcast OTA mode**, which enables efficient firmware updates for large mesh networks (50-100+ nodes).

**Key Benefits:**
- **98% Network Traffic Reduction** for 50-node mesh
- **Parallel Distribution** - All nodes receive chunks simultaneously
- **Scales to 100+ Nodes** efficiently
- **Simple API** - One parameter enables broadcast mode

**Example:**
```cpp
// Enable broadcast mode for efficient large-mesh updates
mesh.offerOTA("sensor", "ESP32", md5, parts, false, true, true);
//                                             ^^^^ ^^^^
//                                       broadcast compress
```

**Performance Comparison:**
- **10 nodes:** 90% traffic reduction, ~10x faster
- **50 nodes:** 98% traffic reduction, ~50x faster
- **100 nodes:** 99% traffic reduction, ~100x faster

**Memory Impact:** +2-5KB per node (chunk tracking buffer)

📖 **Documentation:** [docs/PHASE2_GUIDE.md](docs/PHASE2_GUIDE.md)

---

#### 2. MQTT Status Bridge

Professional monitoring solution that publishes comprehensive mesh status to MQTT topics, enabling integration with enterprise monitoring tools.

**Key Features:**
- **5 MQTT Topic Streams:**
  - `mesh/status/topology` - Complete mesh structure JSON
  - `mesh/status/metrics` - Performance statistics
  - `mesh/status/nodes` - Node list with count
  - `mesh/status/alerts` - Active alert conditions
  - `mesh/status/node/{id}` - Per-node detailed status

**Tool Integration Ready:**
- ✅ Grafana - Dashboard visualization
- ✅ InfluxDB - Time-series storage
- ✅ Prometheus - Metrics collection
- ✅ Home Assistant - Home automation
- ✅ Node-RED - Custom flows

**Example:**
```cpp
#include "examples/bridge/mqtt_status_bridge.hpp"

MqttStatusBridge bridge(mesh, mqttClient);
bridge.setPublishInterval(30000);  // 30 seconds
bridge.enableTopology(true);
bridge.enableMetrics(true);
bridge.enableAlerts(true);
bridge.begin();
```

**Memory Impact:** +5-8KB (root node only)

📖 **Documentation:** [docs/PHASE2_GUIDE.md](docs/PHASE2_GUIDE.md)

---

#### 3. Mesh Topology Visualization Guide

Complete 980-line guide with working examples for building web dashboards to visualize mesh topology in real-time.

**Includes:**
- 🎨 **D3.js Force-Directed Graph** (200+ lines) - Interactive network visualization
- 🕸️ **Cytoscape.js Network View** (150+ lines) - Advanced graph rendering
- 🔴 **Node.js + Express + Socket.IO Dashboard** - Real-time updates
- 🐍 **Python Console Monitor** - Rich library for terminal visualization
- 🔄 **Node-RED Flow JSON** - Drag-and-drop flow programming
- 🛠️ **Troubleshooting Guide** - Common issues and performance tuning

All examples are **copy-paste ready** and production-tested.

📊 **Documentation:** [docs/MESH_TOPOLOGY_GUIDE.md](docs/MESH_TOPOLOGY_GUIDE.md)

---

### 🐛 Critical Bug Fixes

#### Fixed: Missing `#include <vector>` Compilation Error

**Impact:** HIGH - All users including painlessMesh.h

**Symptom:**
```
error: 'vector' in namespace 'std' does not name a template type
   std::vector<ConnectionInfo> getConnectionDetails()
        ^~~~~~
```

**Fix:** Added `#include <vector>` to `src/painlessmesh/mesh.hpp` (line 4)

**Affected Code:**
- `getConnectionDetails()` function (line 386)
- `latencySamples` member variable (line 594)

**Documentation:** [VECTOR_INCLUDE_FIX.md](VECTOR_INCLUDE_FIX.md)

---

#### Fixed: PlatformIO SCons Build Errors

**Impact:** HIGH - All PlatformIO users

**Symptom:**
```
Error: Cannot resolve directory for painlessMeshSTA.cpp
UnboundLocalError: cannot access local variable 'dir'
```

**Fixes Applied:**
1. Added explicit `"srcDir": "src"` to library.json
2. Added explicit `"includeDir": "src"` to library.json
3. Removed conflicting `"export": {"include": "src"}` section
4. Fixed header reference in library.properties (painlessMesh.h)

**Validation:** Created automated validation script with 8 checks (100% passing)

**Documentation:**
- [LIBRARY_STRUCTURE_FIX.md](LIBRARY_STRUCTURE_FIX.md)
- [PLATFORMIO_USAGE.md](PLATFORMIO_USAGE.md)
- [SCONS_BUILD_FIX.md](SCONS_BUILD_FIX.md)

---

#### Fixed: npm Link Errors

**Impact:** MEDIUM - Users consuming library via npm

**Symptom:**
```
UnboundLocalError: cannot access local variable 'dir' (Python error during npm link)
```

**Fix:** Renamed npm scripts to prevent auto-execution:
- `"build"` → `"dev:build"`
- `"prebuild"` → `"dev:prebuild"`

**Reason:** npm automatically runs `build` and `prebuild` scripts during `npm link`, which triggered cmake/ninja builds causing Python errors.

---

### 📚 New Documentation (7 Files)

1. **[docs/MESH_TOPOLOGY_GUIDE.md](docs/MESH_TOPOLOGY_GUIDE.md)** (980 lines)
   - Complete visualization guide with 5 working examples
   - D3.js, Cytoscape.js, Node.js, Python, Node-RED
   - Performance considerations and troubleshooting

2. **[docs/PHASE2_GUIDE.md](docs/PHASE2_GUIDE.md)** (~500 lines)
   - Complete Phase 2 API reference
   - Usage examples and performance benchmarks
   - Integration guides for Grafana, InfluxDB, Prometheus
   - Migration guide from Phase 1

3. **[docs/improvements/PHASE2_IMPLEMENTATION.md](docs/improvements/PHASE2_IMPLEMENTATION.md)** (~600 lines)
   - Technical architecture and implementation details
   - MQTT topic schema documentation
   - Performance analysis and testing strategy

4. **[LIBRARY_STRUCTURE_FIX.md](LIBRARY_STRUCTURE_FIX.md)**
   - PlatformIO library structure improvements
   - Validation checklist and testing guide
   - Root cause analysis and solutions

5. **[PLATFORMIO_USAGE.md](PLATFORMIO_USAGE.md)**
   - Quick start guide for PlatformIO users
   - Common issues and solutions
   - Example platformio.ini configurations

6. **[SCONS_BUILD_FIX.md](SCONS_BUILD_FIX.md)**
   - Comprehensive troubleshooting for PlatformIO builds
   - Step-by-step diagnostic procedures
   - Fix verification instructions

7. **[VECTOR_INCLUDE_FIX.md](VECTOR_INCLUDE_FIX.md)**
   - Documentation of missing C++ header fix
   - Testing and verification instructions
   - Impact analysis

---

### 🛠️ New Tools

#### Library Structure Validation Script

**File:** `scripts/validate_library_structure.py` (250+ lines)

**Features:**
- 8 comprehensive validation checks
- Automated PlatformIO compliance verification
- Detects common configuration issues
- Provides actionable recommendations

**Usage:**
```bash
python scripts/validate_library_structure.py
```

**Output:**
```
✓ Library JSON Exists (srcDir: src | includeDir: src)
✓ Library Properties Valid (Primary header: painlessMesh.h)
✓ Source Directory Structure (3 .cpp, 4 .h, 0 .hpp files)
✓ No Root Source Files (No .cpp files in root)
✓ No Duplicate Metadata (Single library.json at: library.json)
✓ Header Files Present (Found: painlessMesh.h, AlteriomPainlessMesh.h)
✓ Examples Directory (Found 20 example sketches)
✓ PlatformIO Compliance (All 5 structure checks passed)

════════════════════════════════════════════════════
VALIDATION SUMMARY: ✅ ALL CHECKS PASSED (8/8)
════════════════════════════════════════════════════
```

---

### 📊 Performance Improvements

#### Broadcast OTA Performance

| Mesh Size | Unicast Transmissions | Broadcast Transmissions | Traffic Reduction | Speed Improvement |
|-----------|----------------------|------------------------|-------------------|-------------------|
| 10 nodes  | 1,500               | 150                    | 90%               | ~10x faster       |
| 50 nodes  | 7,500               | 150                    | 98%               | ~50x faster       |
| 100 nodes | 15,000              | 150                    | 99%               | ~100x faster      |

**Example:** 150-chunk firmware update (typical 512KB firmware)
- **Unicast:** Sequential per node = O(N × F) = 50 × 150 = 7,500 transmissions
- **Broadcast:** Parallel to all = O(F) = 150 transmissions
- **Savings:** 98% reduction (7,350 fewer transmissions)

#### Memory Impact

| Feature | Memory Usage | Impact |
|---------|-------------|--------|
| Broadcast OTA | +2-5KB per node | Chunk tracking buffer |
| MQTT Bridge | +5-8KB (root only) | Bridge state and buffers |
| Total (both features) | +7-13KB | Acceptable for ESP32, tight on ESP8266 |

---

### ⚠️ Breaking Changes

**NONE** - This release is 100% backward compatible with v1.6.x

- All Phase 1 APIs unchanged
- Broadcast OTA defaults to `false` (unicast mode)
- MQTT bridge is optional add-on
- Existing sketches work without modification
- No recompilation required unless adopting new features

---

### 🔄 Migration Guide

#### No Migration Required

Existing v1.6.x code works without changes. To adopt new features:

#### Option 1: Enable Broadcast OTA

```cpp
// v1.6.x code (continues to work)
mesh.offerOTA(role, hw, md5, parts, false, false, true);

// v1.7.0 - add broadcast parameter
mesh.offerOTA(role, hw, md5, parts, false, true, true);
//                                         ^^^^ enable broadcast
```

#### Option 2: Add MQTT Status Bridge

```cpp
// Include the bridge header
#include "examples/bridge/mqtt_status_bridge.hpp"

// In your setup(), create and configure bridge
MqttStatusBridge bridge(mesh, mqttClient);
bridge.setPublishInterval(30000);  // 30 seconds
bridge.enableTopology(true);
bridge.enableMetrics(true);
bridge.enableAlerts(true);
bridge.begin();

// In your loop(), keep mesh and MQTT alive
void loop() {
    mesh.update();
    mqttClient.loop();
}
```

#### Option 3: Use Both Features

```cpp
// Phase 1: Compressed OTA
// Phase 2: Broadcast distribution
mesh.offerOTA(role, hw, md5, parts, false, true, true);

// Phase 2: MQTT Status Bridge
MqttStatusBridge bridge(mesh, mqttClient);
bridge.begin();
```

---

### 🎯 When to Use Each Feature

#### Broadcast OTA

✅ **Use when:**
- Mesh has 10+ nodes
- All nodes need same firmware
- Network bandwidth is limited
- Fast distribution is critical
- Large-scale deployments (50+ nodes)

❌ **Skip when:**
- Small meshes (<5 nodes) - unicast is sufficient
- Different firmware per node - use unicast with role filtering
- Highly unstable networks - unicast is more reliable

#### MQTT Status Bridge

✅ **Use when:**
- Production deployments
- Remote monitoring requirements
- Integration with existing tools (Grafana, InfluxDB)
- Cloud-connected systems
- Enterprise environments
- Automated alerting needs

❌ **Skip when:**
- Development/testing (use Serial monitor)
- Pure offline meshes (no external network)
- Resource-constrained root nodes
- No MQTT infrastructure available

---

### 🔐 Security Considerations

#### MQTT Bridge Security

- **Recommendation:** Use TLS/SSL for MQTT connections in production
- **Authentication:** Enable MQTT broker authentication (username/password)
- **Network Isolation:** Consider VLANs or firewall rules for MQTT traffic
- **Topic ACLs:** Configure topic access control lists on broker

**Example (secure MQTT):**
```cpp
wifiClient.setCACert(ca_cert);  // TLS certificate
mqttClient.setServer(broker, 8883);  // Secure port
mqttClient.connect("bridge", mqtt_user, mqtt_pass);
```

#### OTA Security

- **Firmware Signing:** MD5 verification included (consider SHA256 for higher security)
- **Role-Based Updates:** Use role filtering to target specific device types
- **Test First:** Always test OTA on small subset before full mesh rollout

---

### 📋 Compatibility Matrix

| Component | v1.6.x | v1.7.0 | Compatible |
|-----------|--------|--------|------------|
| Basic OTA | ✅ | ✅ | ✅ Yes |
| Compressed OTA | ✅ | ✅ | ✅ Yes |
| Broadcast OTA | ❌ | ✅ | ✅ Yes (new) |
| Basic Status Packages | ✅ | ✅ | ✅ Yes |
| Enhanced Status Packages | ✅ | ✅ | ✅ Yes |
| MQTT Bridge | ❌ | ✅ | ✅ Yes (new) |
| Custom Packages (200-203) | ✅ | ✅ | ✅ Yes |
| Mesh Topology | ❌ | ✅ | ✅ Yes (new) |

**Platform Support:**
- ✅ ESP32 (all variants)
- ✅ ESP8266 (with memory considerations)
- ✅ Arduino IDE 2.x
- ✅ PlatformIO 6.x
- ✅ espressif32 platform
- ✅ espressif8266 platform

**Dependencies:**
- ArduinoJson ^7.4.2
- TaskScheduler ^4.0.0
- AsyncTCP ^3.4.7 (ESP32)
- ESPAsyncTCP ^2.0.0 (ESP8266)
- @alteriom/mqtt-schema v0.5.0 (dev)

---

### 🧪 Testing Recommendations

#### Before Upgrading

1. **Backup Current Firmware**
   - Keep v1.6.x binaries for rollback
   - Document current mesh configuration

2. **Review Changes**
   - Read [CHANGELOG.md](CHANGELOG.md)
   - Review [PHASE2_GUIDE.md](docs/PHASE2_GUIDE.md)
   - Check [LIBRARY_STRUCTURE_FIX.md](LIBRARY_STRUCTURE_FIX.md)

3. **Test in Development**
   - Deploy to test mesh (2-5 nodes)
   - Verify compilation succeeds
   - Test existing functionality

#### After Upgrading

1. **Verify Compilation**
   ```bash
   pio run --target clean
   pio run
   ```

2. **Test Basic Mesh**
   - Nodes connect successfully
   - Messages route correctly
   - Time sync works

3. **Test New Features (Optional)**
   - Broadcast OTA on small mesh (2-3 nodes)
   - MQTT bridge with local broker
   - Topology visualization

#### Production Rollout

1. **Pilot Phase**
   - Deploy to 10% of mesh (5-10 nodes)
   - Monitor for 24-48 hours
   - Check logs for errors

2. **Gradual Rollout**
   - Expand to 50% of mesh
   - Monitor performance metrics
   - Verify stability

3. **Full Deployment**
   - Complete mesh upgrade
   - Enable advanced features as needed
   - Document configuration

---

### 🐛 Known Issues

#### None Currently Identified

All known issues from v1.6.x have been resolved in v1.7.0. If you encounter any problems:

1. Check [SCONS_BUILD_FIX.md](SCONS_BUILD_FIX.md) for compilation issues
2. Review [docs/troubleshooting/common-issues.md](docs/troubleshooting/common-issues.md)
3. Run validation: `python scripts/validate_library_structure.py`
4. Open issue: https://github.com/Alteriom/painlessMesh/issues

---

### 📖 Complete Documentation

| Topic | Document | Description |
|-------|----------|-------------|
| **Release Overview** | RELEASE_NOTES_1.7.0.md | This document |
| **Changelog** | [CHANGELOG.md](CHANGELOG.md) | Detailed change history |
| **Phase 2 Features** | [docs/PHASE2_GUIDE.md](docs/PHASE2_GUIDE.md) | Complete API and usage guide |
| **Implementation Details** | [docs/improvements/PHASE2_IMPLEMENTATION.md](docs/improvements/PHASE2_IMPLEMENTATION.md) | Technical architecture |
| **Visualization Guide** | [docs/MESH_TOPOLOGY_GUIDE.md](docs/MESH_TOPOLOGY_GUIDE.md) | Dashboard examples |
| **Bug Fixes** | [VECTOR_INCLUDE_FIX.md](VECTOR_INCLUDE_FIX.md) | Compilation fix details |
| **Library Structure** | [LIBRARY_STRUCTURE_FIX.md](LIBRARY_STRUCTURE_FIX.md) | PlatformIO fixes |
| **Build Troubleshooting** | [SCONS_BUILD_FIX.md](SCONS_BUILD_FIX.md) | Comprehensive diagnostics |
| **PlatformIO Usage** | [PLATFORMIO_USAGE.md](PLATFORMIO_USAGE.md) | Quick start guide |
| **MQTT Commands** | [docs/MQTT_BRIDGE_COMMANDS.md](docs/MQTT_BRIDGE_COMMANDS.md) | Command reference |

---

### 🙏 Acknowledgments

This release includes contributions from:
- **Phase 2 Implementation Team** - Broadcast OTA and MQTT bridge design
- **Community Bug Reports** - Compilation and build system issues
- **Documentation Contributors** - Comprehensive guides and examples
- **Testing Team** - Validation and quality assurance

Special thanks to all users who reported issues and provided feedback during development.

---

### 🚀 What's Next: Phase 3

According to [docs/improvements/FEATURE_PROPOSALS.md](docs/improvements/FEATURE_PROPOSALS.md), Phase 3 will include:

- **Progressive Rollout OTA (Option 1B)** - Phased deployment with health checks
- **Real-time Telemetry Streams (Option 2C)** - Continuous metrics streaming
- **Proactive Alerting System** - Automated anomaly detection
- **Large-Scale Optimization** - Support for 200+ node meshes

Follow development: https://github.com/Alteriom/painlessMesh/projects

---

### 📞 Support

**Documentation:** https://alteriom.github.io/painlessMesh/  
**Issues:** https://github.com/Alteriom/painlessMesh/issues  
**Discussions:** https://github.com/Alteriom/painlessMesh/discussions  
**Email:** support@alteriom.com

---

**Released:** October 15, 2025  
**Version:** 1.7.0  
**Schema:** @alteriom/mqtt-schema v0.5.0  
**Status:** ✅ Production Ready  
**Compatibility:** 100% backward compatible with v1.6.x
