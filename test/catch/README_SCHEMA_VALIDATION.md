# Schema Validation Tests

**File:** `catch_topology_schema.cpp`  
**Purpose:** Validate @alteriom/mqtt-schema v0.5.0 compliance for mesh topology and event messages  
**Created:** October 13, 2025

---

## Overview

This test suite validates that painlessMesh topology and event messages conform to the @alteriom/mqtt-schema v0.5.0 specification:
- `mesh_topology.schema.json`
- `mesh_event.schema.json`

### What's Tested

#### ✅ Topology Messages
- **Envelope Fields:** schema_version, device_id, device_type, timestamp, firmware_version
- **Event Discriminator:** event="mesh_topology"
- **Mesh Identification:** mesh_id, gateway_node_id
- **Device ID Format:** ALT-XXXXXXXXXXXX (12 uppercase hex digits)
- **Nodes Array:** node_id, role, status, last_seen, firmware_version, uptime_seconds, free_memory_kb, connection_count
- **Connections Array:** from_node, to_node, quality, latency_ms, rssi, hop_count
- **Metrics Object:** total_nodes, online_nodes, network_diameter, avg_connection_quality, messages_per_second
- **Update Types:** "full" and "incremental"

#### ✅ Event Messages
- **Envelope Fields:** All required fields present and correct types
- **Event Discriminator:** event="mesh_event"
- **Event Types:** node_join, node_leave, connection_lost, connection_restored, network_split, network_merged
- **Affected Nodes:** Array with proper device ID format
- **Details Object:** Event-specific details (reason, last_seen, etc.)

#### ✅ Data Validation
- **Quality Ranges:** 0-100 for connection quality
- **RSSI Ranges:** -100 to 0 dBm (negative values)
- **Latency Ranges:** Positive milliseconds < 10s
- **Message Size:** Full topology < 16KB, incremental < 2KB
- **Node Roles:** gateway, sensor, repeater
- **Node Statuses:** online, offline, unknown

---

## Running the Tests

### Option 1: Docker (Recommended)

**Prerequisites:**
- Docker Desktop installed and running
- Windows PowerShell

**Run all tests:**
```powershell
.\docker-test.ps1 test
```

**Run only schema validation tests:**
```powershell
.\docker-test.ps1 shell
# Inside container:
cd /workspace
ninja
./bin/catch_topology_schema
```

### Option 2: Local Build (Linux/Mac)

**Prerequisites:**
- CMake 3.10+
- Ninja build system
- GCC/Clang compiler
- Git (for dependencies)

**Build and run:**
```bash
# Clone dependencies (first time only)
cd test
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arkhipenko/TaskScheduler

# Build
cd ..
cmake -G Ninja .
ninja

# Run all tests
run-parts --regex catch_ bin/

# Run only schema validation
./bin/catch_topology_schema
```

### Option 3: Local Build (Windows)

**Prerequisites:**
- CMake 3.10+ (available in `cmake-3.27.7-windows-x86_64/`)
- Ninja build system
- MinGW or Visual Studio
- Git

**Build and run:**
```powershell
# Clone dependencies (first time only)
cd test
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arkhipenko/TaskScheduler

# Build
cd ..
.\cmake-3.27.7-windows-x86_64\bin\cmake.exe -G Ninja .
ninja

# Run
.\bin\catch_topology_schema.exe
```

---

## Test Structure

### Scenarios

1. **Full Topology Validation**
   - Tests complete topology message structure
   - Validates all required fields
   - Checks device ID format
   - Verifies nodes and connections arrays

2. **Incremental Update Validation**
   - Tests incremental topology updates
   - Validates node join/leave messages
   - Checks update_type="incremental"

3. **Connection Objects Validation**
   - Tests connection structure
   - Validates quality, latency, RSSI fields
   - Checks device ID format in connections

4. **Event Message Validation**
   - Tests node_join event structure
   - Tests node_leave event structure
   - Tests connection_lost event structure
   - Validates event_type field

5. **Enumeration Validation**
   - Tests event types (6 types)
   - Tests node roles (3 roles)
   - Tests node statuses (3 statuses)

6. **Range Validation**
   - Quality: 0-100
   - RSSI: -100 to 0 dBm
   - Latency: positive ms values

7. **Message Size Validation**
   - Full topology < 16KB
   - Incremental < 2KB

### Helper Functions

- `validateEnvelope(JsonObject)` - Checks all envelope fields
- `validateDeviceIdFormat(String)` - Validates ALT-XXXXXXXXXXXX format

---

## Expected Output

### Successful Test Run

```
===============================================================================
All tests passed (150+ assertions in 10 test cases)
```

### Test Failure Example

```
test/catch/catch_topology_schema.cpp:98: FAILED:
  REQUIRE( obj["schema_version"] == 1 )
with expansion:
  0 == 1
```

---

## Integration with CI

### GitHub Actions

Add to `.github/workflows/test.yml`:
```yaml
- name: Run Schema Validation Tests
  run: |
    docker-compose up --abort-on-container-exit painlessmesh-test
```

### Manual Verification

Before merging PR, ensure:
1. ✅ All schema validation tests pass
2. ✅ No new compilation warnings
3. ✅ Device ID format is consistent (ALT-XXXXXXXXXXXX)
4. ✅ All envelope fields present in messages
5. ✅ Message sizes within limits

---

## Troubleshooting

### Issue: Docker build fails

**Solution:** Ensure Docker Desktop is running and you have internet access to pull base images.

### Issue: CMake not found

**Solution:** 
- Windows: Use `.\cmake-3.27.7-windows-x86_64\bin\cmake.exe`
- Linux/Mac: Install via package manager (`apt install cmake` or `brew install cmake`)

### Issue: ArduinoJson errors

**Solution:** Ensure ArduinoJson is cloned in `test/ArduinoJson/`:
```bash
cd test
git clone https://github.com/bblanchon/ArduinoJson.git
```

### Issue: Test crashes on startup

**Solution:** Check that painlessMesh mock is properly initialized. The test creates a minimal mesh instance for validation.

### Issue: Device ID format fails

**Solution:** Verify that device IDs are:
- Exactly 16 characters
- Start with "ALT-"
- Followed by 12 uppercase hex digits (0-9, A-F)

---

## Schema Compliance Checklist

Use this checklist to verify schema compliance:

### mesh_topology.schema.json

- [ ] Envelope fields present (5 fields)
- [ ] event="mesh_topology"
- [ ] mesh_id field present
- [ ] gateway_node_id in ALT-XXXXXXXXXXXX format
- [ ] nodes array with required fields (8 per node)
- [ ] connections array with required fields (6 per connection)
- [ ] metrics object with 5 aggregates
- [ ] update_type is "full" or "incremental"
- [ ] Device IDs in uppercase hex format
- [ ] Quality values 0-100
- [ ] RSSI values negative dBm
- [ ] Message size < 16KB (full) or < 2KB (incremental)

### mesh_event.schema.json

- [ ] Envelope fields present (5 fields)
- [ ] event="mesh_event"
- [ ] event_type is valid (6 types)
- [ ] mesh_id field present
- [ ] affected_nodes array present
- [ ] Device IDs in affected_nodes in ALT-XXXXXXXXXXXX format
- [ ] details object present
- [ ] Event-specific fields in details (reason, timestamp, etc.)

---

## Related Files

- **Implementation:** `examples/bridge/mesh_topology_reporter.hpp`
- **Implementation:** `examples/bridge/mesh_event_publisher.hpp`
- **Documentation:** `docs/MQTT_SCHEMA_COMPLIANCE.md`
- **Documentation:** `docs/MQTT_BRIDGE_COMMANDS.md`
- **Integration:** `examples/mqttCommandBridge/mqttCommandBridge.ino`
- **Command Handler:** `examples/bridge/mqtt_command_bridge.hpp`

---

## Next Steps

After schema validation tests pass:

1. **Task 15:** Create hardware test sketch (`examples/mqttTopologyTest/`)
2. **Task 18:** Create visualization guide (`docs/MESH_TOPOLOGY_GUIDE.md`)
3. **Release:** Tag v1.7.0 with mesh topology support

---

**Author:** Alteriom Development Team  
**Schema Version:** @alteriom/mqtt-schema v0.5.0  
**Last Updated:** October 13, 2025
