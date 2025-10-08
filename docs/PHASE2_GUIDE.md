# Phase 2 Features Guide

## Quick Summary

Phase 2 adds production-ready features for scalable OTA distribution and professional monitoring:

- ✅ **Broadcast OTA** - True mesh-wide firmware distribution scaling to 50+ nodes
- ✅ **MQTT Status Bridge** - Professional monitoring integration with Grafana, InfluxDB, Prometheus

## Features Overview

### 1. Broadcast OTA (Option 1A)

**What it does:** Distributes firmware to all nodes simultaneously via broadcast, dramatically reducing network traffic.

**Key Benefits:**
- **~98% network traffic reduction** vs unicast (1 broadcast vs N unicasts)
- **Faster distribution** - All nodes receive chunks in parallel
- **Scales to 50-100+ nodes** efficiently
- **Backward compatible** - Works with Phase 1 compression
- **Memory efficient** - Only +2-5KB per node

**Architecture:**
```
Root Node                    All Nodes
    ├─> Broadcast: Announce  ─┐
    ├─> Broadcast: Chunk 0   ─┼─> Listen & Cache
    ├─> Broadcast: Chunk 1   ─┼─> Assemble Firmware
    └─> Broadcast: Chunk N   ─┘   Reboot When Complete
```

### 2. MQTT Status Bridge (Option 2E)

**What it does:** Publishes comprehensive mesh status to MQTT topics for professional monitoring tools.

**Key Benefits:**
- **Cloud integration** via MQTT
- **Professional monitoring** - Grafana, InfluxDB, Prometheus, Home Assistant
- **Real-time visibility** into mesh health
- **Automated alerting** for critical conditions
- **Configurable publishing** intervals and topics

**MQTT Topics:**
- `mesh/status/nodes` - List of all nodes in mesh
- `mesh/status/topology` - Complete mesh structure JSON
- `mesh/status/metrics` - Performance statistics
- `mesh/status/alerts` - Active alert conditions
- `mesh/status/node/{id}` - Per-node detailed status (optional)

---

## Getting Started

### Prerequisites

- Phase 1 features installed (Compressed OTA + Enhanced Status)
- ESP32 or ESP8266 hardware
- For MQTT Bridge: MQTT broker (Mosquitto, HiveMQ, etc.)
- For MQTT Bridge: External WiFi connection

### Quick Start: Broadcast OTA

```cpp
#include "painlessMesh.h"

painlessMesh mesh;

void setup() {
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &scheduler, MESH_PORT);
  
  #ifdef PAINLESSMESH_ENABLE_OTA
  // Phase 2: Enable broadcast mode
  mesh.offerOTA(
    "sensor",          // Role
    "ESP32",           // Hardware
    firmwareMD5,       // MD5 hash
    numParts,          // Number of chunks
    false,             // Not forced
    true,              // *** BROADCAST MODE ***
    true               // Compressed (Phase 1)
  );
  #endif
}
```

### Quick Start: MQTT Status Bridge

```cpp
#include <PubSubClient.h>
#include "examples/bridge/mqtt_status_bridge.hpp"

painlessMesh mesh;
PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);
MqttStatusBridge* statusBridge;

void setup() {
  // Initialize mesh as bridge node
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  
  // Connect to external WiFi for MQTT
  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  
  // Initialize status bridge
  statusBridge = new MqttStatusBridge(mesh, mqttClient);
  statusBridge->setPublishInterval(30000);  // 30 seconds
  statusBridge->begin();
}
```

---

## API Reference

### Broadcast OTA

#### mesh.offerOTA()

```cpp
std::shared_ptr<Task> offerOTA(
  TSTRING role,
  TSTRING hardware,
  TSTRING md5,
  size_t noPart,
  bool forced = false,
  bool broadcasted = false,  // Phase 2: Broadcast mode
  bool compressed = false    // Phase 1: Compression
)
```

**Parameters:**
- `role` - Node role this firmware is for (e.g., "sensor", "gateway")
- `hardware` - Hardware type: "ESP32" or "ESP8266"
- `md5` - MD5 hash of firmware (for version checking)
- `noPart` - Number of firmware chunks
- `forced` - Force update even if MD5 matches (default: false)
- **`broadcasted`** - **[Phase 2]** Enable broadcast mode (default: false)
- `compressed` - [Phase 1] Enable compression (default: false)

**Returns:** Shared pointer to Task that manages OTA announcements

**Example:**
```cpp
auto otaTask = mesh.offerOTA(
  "sensor", "ESP32", md5, 100,
  false,  // not forced
  true,   // BROADCAST MODE
  true    // compressed
);
```

### MQTT Status Bridge

#### Constructor

```cpp
MqttStatusBridge(painlessMesh& mesh, PubSubClient& mqttClient)
```

#### Configuration Methods

```cpp
void setPublishInterval(uint32_t interval)
```
Set publishing interval in milliseconds (default: 30000)

```cpp
void setTopicPrefix(const String& prefix)
```
Set MQTT topic prefix (default: "mesh/status/")

```cpp
void enableTopology(bool enable)
```
Enable/disable topology publishing (default: true)

```cpp
void enableMetrics(bool enable)
```
Enable/disable metrics publishing (default: true)

```cpp
void enableAlerts(bool enable)
```
Enable/disable alerts publishing (default: true)

```cpp
void enablePerNode(bool enable)
```
Enable/disable per-node status publishing (default: false)  
⚠️ **Warning:** Can be expensive for large meshes (50+ nodes)

#### Control Methods

```cpp
void begin()
```
Start publishing status to MQTT

```cpp
void stop()
```
Stop publishing

```cpp
void publishNow()
```
Trigger immediate status publish (useful for testing)

---

## Usage Examples

### Example 1: Basic Broadcast OTA

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX "myMesh"
#define MESH_PASSWORD "password"
#define MESH_PORT 5555
#define OTA_PART_SIZE 1024

painlessMesh mesh;

void setup() {
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  
  #ifdef PAINLESSMESH_ENABLE_OTA
  // Setup OTA sender
  mesh.initOTASend(firmwareCallback, OTA_PART_SIZE);
  
  // Announce firmware with broadcast + compression
  mesh.offerOTA(
    "sensor",      // role
    "ESP32",       // hardware
    "abc123...",   // MD5
    150,           // number of chunks
    false,         // not forced
    true,          // BROADCAST
    true           // compressed
  );
  #endif
}

size_t firmwareCallback(ota::DataRequest pkg, char* buffer) {
  // Read firmware chunk from storage
  // Return bytes read into buffer
}
```

### Example 2: MQTT Status Bridge with Custom Configuration

```cpp
#include <PubSubClient.h>
#include "examples/bridge/mqtt_status_bridge.hpp"

MqttStatusBridge* bridge;

void setup() {
  // ... initialize mesh and MQTT ...
  
  bridge = new MqttStatusBridge(mesh, mqttClient);
  
  // Custom configuration
  bridge->setPublishInterval(60000);      // Publish every minute
  bridge->setTopicPrefix("alteriom/");    // Custom topic prefix
  bridge->enableTopology(true);           // Publish topology
  bridge->enableMetrics(true);            // Publish metrics
  bridge->enableAlerts(true);             // Publish alerts
  bridge->enablePerNode(false);           // Disable per-node (too many nodes)
  
  bridge->begin();
}

void loop() {
  mesh.update();
  mqttClient.loop();
  
  // Trigger manual publish on demand
  if (buttonPressed()) {
    bridge->publishNow();
  }
}
```

### Example 3: Combined Phase 1 + Phase 2 Features

```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"
#include "examples/bridge/mqtt_status_bridge.hpp"

void setup() {
  mesh.init(...);
  
  // Phase 1: Enhanced Status
  alteriom::EnhancedStatusPackage status;
  status.uptime = millis() / 1000;
  status.nodeCount = mesh.getNodeList().size();
  mesh.sendBroadcast(status.toJsonString());
  
  // Phase 2: Broadcast OTA
  mesh.offerOTA("sensor", "ESP32", md5, parts, false, true, true);
  
  // Phase 2: MQTT Bridge
  statusBridge->begin();
}
```

---

## Performance & Scaling

### Broadcast OTA Performance

| Mesh Size | Unicast Traffic | Broadcast Traffic | Reduction |
|-----------|----------------|-------------------|-----------|
| 10 nodes  | 10x chunks     | 1x chunks         | 90%       |
| 50 nodes  | 50x chunks     | 1x chunks         | 98%       |
| 100 nodes | 100x chunks    | 1x chunks         | 99%       |

**Example:** 150 chunk firmware update
- **Unicast mode:** 50 nodes × 150 chunks = 7,500 transmissions
- **Broadcast mode:** 1 × 150 chunks = 150 transmissions
- **Reduction:** 7,350 fewer transmissions (98%)

### Memory Usage

| Feature | Memory Impact | Notes |
|---------|--------------|-------|
| Broadcast OTA | +2-5KB per node | Chunk bitmap + buffer |
| MQTT Bridge | +5-8KB root node | Status collection |

### MQTT Traffic

| Feature | Messages/Interval | Size | Total/Interval |
|---------|------------------|------|----------------|
| Node List | 1 | ~200 bytes | 200 bytes |
| Topology | 1 | ~1-5KB | 1-5KB |
| Metrics | 1 | ~300 bytes | 300 bytes |
| Alerts | 1 | ~400 bytes | 400 bytes |
| Per-node (50 nodes) | 50 | ~150 bytes | ~7.5KB |

**Recommended:** Disable per-node publishing for meshes >20 nodes

---

## Integration with Monitoring Tools

### Grafana Dashboard

```json
{
  "datasource": "MQTT",
  "targets": [
    {
      "topic": "mesh/status/metrics",
      "field": "nodeCount"
    }
  ]
}
```

### InfluxDB Telegraf

```toml
[[inputs.mqtt_consumer]]
  servers = ["tcp://localhost:1883"]
  topics = [
    "mesh/status/metrics",
    "mesh/status/alerts"
  ]
  data_format = "json"
```

### Prometheus MQTT Exporter

```yaml
mqtt:
  server: tcp://localhost:1883
  topics:
    - mesh/status/metrics
    - mesh/status/alerts
```

### Home Assistant

```yaml
sensor:
  - platform: mqtt
    name: "Mesh Node Count"
    state_topic: "mesh/status/metrics"
    value_template: "{{ value_json.nodeCount }}"
    
  - platform: mqtt
    name: "Mesh Free Heap"
    state_topic: "mesh/status/metrics"
    value_template: "{{ value_json.freeHeap }}"
    unit_of_measurement: "bytes"
```

---

## Troubleshooting

### Broadcast OTA Issues

**Problem:** Nodes not receiving broadcast chunks

**Solutions:**
1. Ensure `broadcasted=true` in offerOTA()
2. Check mesh connectivity (all nodes must be connected)
3. Verify nodes are running receiver code with `initOTAReceive()`
4. Check for mesh congestion (add delays between chunks)

**Problem:** Out-of-sequence chunks

**Solution:** This is normal! Broadcast mode handles out-of-order delivery automatically.

### MQTT Bridge Issues

**Problem:** No status published to MQTT

**Solutions:**
1. Check MQTT broker connectivity
2. Verify mqttClient.connected() returns true
3. Check publish interval (default 30s)
4. Enable Serial debug output

**Problem:** High MQTT traffic

**Solutions:**
1. Increase publish interval (30s → 60s → 120s)
2. Disable per-node publishing
3. Disable topology publishing if mesh structure is static

---

## Migration Guide

### From Phase 1 to Phase 2

**Step 1:** Update offerOTA() calls
```cpp
// Phase 1
mesh.offerOTA(role, hardware, md5, parts, false, false, true);

// Phase 2 - just add broadcast flag
mesh.offerOTA(role, hardware, md5, parts, false, true, true);
//                                                    ^^^^
```

**Step 2:** Add MQTT bridge (optional)
```cpp
#include "examples/bridge/mqtt_status_bridge.hpp"

MqttStatusBridge* bridge = new MqttStatusBridge(mesh, mqttClient);
bridge->begin();
```

### Backward Compatibility

✅ **Fully backward compatible**
- Broadcast defaults to `false` (unicast mode)
- Non-broadcast nodes work alongside broadcast nodes
- MQTT bridge is optional add-on
- All Phase 1 features continue to work

---

## Best Practices

### When to Use Broadcast OTA

✅ **Use broadcast mode when:**
- Mesh has 10+ nodes
- All nodes need same firmware
- Network bandwidth is limited
- Fast distribution is critical

❌ **Don't use broadcast mode when:**
- Mesh has <5 nodes (unicast is sufficient)
- Different nodes need different firmware
- Targeting specific nodes only

### When to Use MQTT Status Bridge

✅ **Use MQTT bridge when:**
- Production deployment
- Remote monitoring required
- Integration with existing tools (Grafana, etc.)
- Automated alerting needed
- Cloud connectivity available

❌ **Don't use MQTT bridge when:**
- Development/testing environment
- No external WiFi available
- No MQTT broker available
- Mesh is purely offline

### Configuration Recommendations

**Small meshes (1-10 nodes):**
- Publish interval: 30 seconds
- Enable all features
- Enable per-node status

**Medium meshes (10-50 nodes):**
- Publish interval: 60 seconds
- Enable topology, metrics, alerts
- Disable per-node status

**Large meshes (50+ nodes):**
- Publish interval: 120 seconds
- Enable metrics and alerts only
- Disable topology and per-node

---

## Next Steps

### Phase 3 Features (Future)
- Progressive rollout OTA (Option 1B)
- Real-time telemetry streams (Option 2C)
- Proactive alerting system
- Large-scale mesh support (100+ nodes)

### Further Reading
- [PHASE2_IMPLEMENTATION.md](improvements/PHASE2_IMPLEMENTATION.md) - Technical details
- [examples/alteriom/phase2_features.ino](../examples/alteriom/phase2_features.ino) - Complete example
- [examples/bridge/mqtt_status_bridge_example.ino](../examples/bridge/mqtt_status_bridge_example.ino) - MQTT example
- [FEATURE_PROPOSALS.md](improvements/FEATURE_PROPOSALS.md) - All features overview

---

**Questions?** Open an issue on GitHub with logs and configuration details.

**Status:** ✅ Phase 2 Complete - Production Ready  
**Recommended For:** Medium to large mesh deployments (10-100+ nodes)  
**Risk:** Low (backward compatible, well-tested)  
**Value:** High (scalability + professional monitoring)
