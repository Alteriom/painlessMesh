# OTA Commands and API Reference

## Overview

This document provides a complete reference for OTA (Over-The-Air) firmware update commands in painlessMesh, including Phase 2 broadcast mode enhancements and Alteriom MQTT schema compliance for firmware status reporting.

## Table of Contents

1. [OTA Command API](#ota-command-api)
2. [Broadcast OTA (Phase 2)](#broadcast-ota-phase-2)
3. [Firmware Status Reporting](#firmware-status-reporting)
4. [MQTT Schema Compliance](#mqtt-schema-compliance)
5. [Complete Examples](#complete-examples)
6. [Troubleshooting](#troubleshooting)

---

## OTA Command API

### mesh.offerOTA()

Announce and distribute firmware updates to mesh nodes.

```cpp
std::shared_ptr<Task> offerOTA(
  TSTRING role,
  TSTRING hardware,
  TSTRING md5,
  size_t noPart,
  bool forced = false,
  bool broadcasted = false,  // Phase 2 feature
  bool compressed = false    // Phase 1 feature
)
```

#### Parameters

| Parameter | Type | Description | Default |
|-----------|------|-------------|---------|
| `role` | TSTRING | Target node role (e.g., "sensor", "gateway") | Required |
| `hardware` | TSTRING | Hardware type: "ESP32" or "ESP8266" | Required |
| `md5` | TSTRING | MD5 hash of firmware binary for version checking | Required |
| `noPart` | size_t | Total number of firmware chunks | Required |
| `forced` | bool | Force update even if MD5 matches current version | false |
| `broadcasted` | bool | **[Phase 2]** Enable broadcast distribution mode | false |
| `compressed` | bool | [Phase 1] Enable compression (40-60% bandwidth savings) | false |

#### Returns

`std::shared_ptr<Task>` - Shared pointer to task managing OTA announcements

#### Behavior

**Unicast Mode (broadcasted=false):**
- Each node requests chunks individually
- Root node responds to each request
- Network traffic: O(N × F) where N=nodes, F=firmware size
- Best for: 1-10 nodes

**Broadcast Mode (broadcasted=true):**
- Root node broadcasts chunks once
- All nodes receive simultaneously
- Network traffic: O(F) - independent of node count
- Best for: 10-100+ nodes
- ~98% traffic reduction for 50-node mesh

#### Example

```cpp
// Phase 2 Broadcast OTA with compression
auto otaTask = mesh.offerOTA(
  "sensor",        // Role
  "ESP32",         // Hardware
  firmwareMD5,     // MD5 hash
  numChunks,       // Number of chunks
  false,           // Not forced
  true,            // BROADCAST MODE (Phase 2)
  true             // Compressed (Phase 1)
);

// Monitor OTA progress
otaTask->setCallback([]() {
  Serial.println("OTA announcement sent");
});
```

### mesh.initOTAReceive()

Initialize OTA receiver on a node to accept firmware updates.

```cpp
void initOTAReceive(
  TSTRING role,
  std::function<void(size_t current, size_t total)> progressCallback = nullptr,
  bool acceptCompressed = true,
  bool acceptBroadcast = true
)
```

#### Parameters

| Parameter | Type | Description | Default |
|-----------|------|-------------|---------|
| `role` | TSTRING | This node's role for matching firmware | Required |
| `progressCallback` | function | Callback for progress updates (current, total) | nullptr |
| `acceptCompressed` | bool | Accept compressed firmware (Phase 1) | true |
| `acceptBroadcast` | bool | Accept broadcast OTA (Phase 2) | true |

#### Example

```cpp
mesh.initOTAReceive(
  "sensor",
  [](size_t current, size_t total) {
    Serial.printf("OTA Progress: %d/%d (%d%%)\n", 
                  current, total, (current * 100) / total);
  },
  true,  // Accept compressed
  true   // Accept broadcast
);
```

---

## Broadcast OTA (Phase 2)

### Architecture

Broadcast OTA distributes firmware to all nodes simultaneously instead of individually, dramatically reducing network traffic and update time.

**Message Flow:**

```
1. Root Node Announces Update:
   Root → ALL: Broadcast Announce {role, hardware, md5, noPart, broadcasted=true}

2. Root Node Broadcasts Chunks:
   Root → ALL: Broadcast Data(chunk 0)
   Root → ALL: Broadcast Data(chunk 1)
   Root → ALL: Broadcast Data(chunk 2)
   ...
   Root → ALL: Broadcast Data(chunk N)

3. All Nodes Process:
   Each node:
   - Receives broadcasts
   - Assembles chunks
   - Verifies MD5
   - Flashes firmware
   - Reboots into new version
```

### Performance Comparison

| Mesh Size | Unicast Transmissions | Broadcast Transmissions | Reduction | Time Improvement |
|-----------|----------------------|------------------------|-----------|------------------|
| 10 nodes  | 1,500                | 150                    | 90%       | ~10x faster      |
| 50 nodes  | 7,500                | 150                    | 98%       | ~50x faster      |
| 100 nodes | 15,000               | 150                    | 99%       | ~100x faster     |

*Assuming 150 firmware chunks*

### Memory Requirements

**Per Node:**
- Chunk buffer: ~1-2KB
- Tracking bitmap: ~1KB (for 150 chunks)
- Total overhead: +2-5KB

**Scalability:**
- Tested: Up to 100 nodes
- Theoretical: 200+ nodes with proper rate limiting
- Recommended: 10-100 nodes for optimal performance

### Implementation Details

The broadcast mode is implemented through a minimal change to `src/painlessmesh/ota.hpp`:

```cpp
static Data replyTo(const DataRequest& req, TSTRING data, size_t partNo) {
  Data d;
  // ... initialize fields ...
  
  // Phase 2: Set BROADCAST routing when broadcasted flag is true
  if (req.broadcasted) {
    d.routing = router::BROADCAST;
  }
  return d;
}
```

### Best Practices

1. **Rate Limiting:** Don't broadcast chunks faster than nodes can process
2. **Chunk Size:** Use 1024-2048 byte chunks for optimal balance
3. **Network Stability:** Ensure mesh is stable before starting OTA
4. **Monitoring:** Use progress callbacks to track update status
5. **Fallback:** Keep unicast mode available for small deployments

---

## Firmware Status Reporting

### Alteriom MQTT Schema v1 Compliance

The firmware update process can report status using the Alteriom MQTT schema `firmware_status.schema.json` v1.

#### Schema Structure

```json
{
  "schema_version": 1,
  "device_id": "node-123456",
  "device_type": "sensor",
  "timestamp": "2024-10-11T16:30:00Z",
  "firmware_version": "1.0.0",
  "status": "downloading",
  "from_version": "1.0.0",
  "to_version": "2.0.0",
  "progress_pct": 45.5,
  "error": null
}
```

#### Status Values

| Status | Description | Required Fields |
|--------|-------------|----------------|
| `pending` | Update queued, not started | None |
| `downloading` | Downloading firmware chunks | `progress_pct` recommended |
| `flashing` | Writing firmware to flash | `progress_pct` recommended |
| `verifying` | Verifying firmware integrity | None |
| `rebooting` | Rebooting into new firmware | None |
| `completed` | Update successful | `to_version` |
| `failed` | Update failed | `error` required |

#### Example Implementation

```cpp
// Report OTA progress via MQTT
void reportOTAStatus(String status, float progress = -1, String error = "") {
  DynamicJsonDocument doc(512);
  
  // Envelope fields (required)
  doc["schema_version"] = 1;
  doc["device_id"] = String(mesh.getNodeId());
  doc["device_type"] = "sensor";
  doc["timestamp"] = getCurrentISO8601Timestamp();
  doc["firmware_version"] = FIRMWARE_VERSION;
  
  // Firmware status fields
  doc["status"] = status;
  doc["from_version"] = OLD_VERSION;
  doc["to_version"] = NEW_VERSION;
  
  if (progress >= 0) {
    doc["progress_pct"] = progress;
  }
  
  if (error.length() > 0) {
    doc["error"] = error;
  } else {
    doc["error"] = nullptr;
  }
  
  String payload;
  serializeJson(doc, payload);
  mqttClient.publish("device/firmware/status", payload.c_str());
}

// Usage in OTA progress callback
mesh.initOTAReceive("sensor", [](size_t current, size_t total) {
  float progress = (current * 100.0) / total;
  
  if (current == 0) {
    reportOTAStatus("downloading", 0);
  } else if (current < total) {
    reportOTAStatus("downloading", progress);
  } else {
    reportOTAStatus("flashing", 100);
  }
});
```

---

## MQTT Schema Compliance

### Gateway Metrics

The MQTT Status Bridge publishes gateway metrics in full compliance with Alteriom MQTT schema v1.

**Schema:** `gateway_metrics.schema.json` v1

**Required Fields:**
- Envelope: `schema_version`, `device_id`, `device_type`, `timestamp`, `firmware_version`
- Metrics: `uptime_s` (minimum required)

**Published Message:**
```json
{
  "schema_version": 1,
  "device_id": "gateway-001",
  "device_type": "gateway",
  "timestamp": "2024-10-11T16:30:00Z",
  "firmware_version": "2.1.0",
  "metrics": {
    "uptime_s": 3600,
    "mesh_nodes": 12,
    "memory_usage_pct": 45.2,
    "connected_devices": 12
  }
}
```

### Validation

To validate messages against the schema:

```javascript
// Node.js validation example
const { validators } = require('@alteriom/mqtt-schema');

const message = JSON.parse(mqttPayload);
const result = validators.gatewayMetrics(message);

if (!result.valid) {
  console.error('Validation errors:', result.errors);
}
```

---

## Complete Examples

### Example 1: Basic OTA Sender

```cpp
#include <painlessMesh.h>

#define MESH_PREFIX     "mesh"
#define MESH_PASSWORD   "password"
#define MESH_PORT       5555

painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  
  // Calculate firmware chunks
  File firmware = SD.open("/firmware.bin");
  size_t fileSize = firmware.size();
  size_t numChunks = (fileSize + 1023) / 1024;  // 1KB chunks
  String md5 = calculateMD5(firmware);
  
  // Offer OTA with Phase 2 broadcast
  auto otaTask = mesh.offerOTA(
    "sensor",
    "ESP32",
    md5,
    numChunks,
    false,  // not forced
    true,   // BROADCAST
    true    // compressed
  );
  
  Serial.println("Broadcasting OTA update...");
}

void loop() {
  mesh.update();
}
```

### Example 2: OTA Receiver with Status Reporting

```cpp
#include <painlessMesh.h>
#include <PubSubClient.h>

painlessMesh mesh;
PubSubClient mqttClient;

void reportOTAStatus(String status, float progress = -1) {
  // Implementation as shown above
}

void setup() {
  Serial.begin(115200);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  
  // Initialize OTA receiver with progress reporting
  mesh.initOTAReceive(
    "sensor",
    [](size_t current, size_t total) {
      float pct = (current * 100.0) / total;
      Serial.printf("OTA: %d/%d (%.1f%%)\n", current, total, pct);
      
      // Report via MQTT
      if (current == 0) {
        reportOTAStatus("downloading", 0);
      } else if (current < total) {
        reportOTAStatus("downloading", pct);
      } else {
        reportOTAStatus("flashing", 100);
      }
    },
    true,  // accept compressed
    true   // accept broadcast
  );
}

void loop() {
  mesh.update();
  mqttClient.loop();
}
```

### Example 3: Full MQTT Bridge with Schema Compliance

```cpp
#include <painlessMesh.h>
#include <PubSubClient.h>
#include "examples/bridge/mqtt_status_bridge.hpp"

painlessMesh mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
MqttStatusBridge* statusBridge;

void setup() {
  Serial.begin(115200);
  
  // Initialize mesh as bridge node
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  mesh.stationManual(WIFI_SSID, WIFI_PASSWORD);
  
  // Connect to MQTT broker
  mqttClient.setServer(MQTT_BROKER, 1883);
  mqttClient.connect("painlessMesh-bridge");
  
  // Initialize schema-compliant MQTT status bridge
  statusBridge = new MqttStatusBridge(mesh, mqttClient);
  statusBridge->setDeviceId("gateway-001");
  statusBridge->setFirmwareVersion("2.1.0");
  statusBridge->setPublishInterval(30000);  // 30 seconds
  statusBridge->enableMetrics(true);        // Schema v1 compliant
  statusBridge->enableTopology(true);
  statusBridge->enableAlerts(true);
  statusBridge->begin();
  
  Serial.println("MQTT Status Bridge started");
  Serial.println("Publishing schema-compliant gateway metrics");
}

void loop() {
  mesh.update();
  mqttClient.loop();
}
```

---

## Troubleshooting

### Common Issues

#### 1. OTA Not Starting

**Symptom:** Nodes don't respond to OTA announcements

**Solutions:**
- Verify role matches between sender and receiver
- Check hardware type (ESP32 vs ESP8266)
- Ensure nodes have OTA initialized with `initOTAReceive()`
- Check MD5 is different from current firmware

#### 2. Broadcast OTA Slow/Failing

**Symptom:** Broadcast mode slower than expected or nodes miss chunks

**Solutions:**
- Reduce broadcast rate (add delays between chunks)
- Check mesh stability (`mesh.getNodeList()` should be stable)
- Verify sufficient memory on nodes (check `ESP.getFreeHeap()`)
- Consider smaller chunk size for congested meshes
- Reduce number of nodes or use unicast for <10 nodes

#### 3. Schema Validation Failures

**Symptom:** MQTT consumers reject messages

**Solutions:**
- Verify `schema_version` is exactly 1 (integer)
- Check `device_id` matches pattern `^[A-Za-z0-9_-]+$` (no spaces)
- Ensure `device_type` is exactly "gateway" or "sensor"
- Validate timestamp is ISO 8601: `YYYY-MM-DDTHH:MM:SSZ`
- Check `firmware_version` is not empty and ≤40 characters
- Ensure `metrics` object exists for gateway_metrics
- Verify `uptime_s` is present and non-negative integer

#### 4. Timestamp Issues

**Symptom:** Timestamps rejected or incorrect

**Solutions:**
- Use NTP time sync for accurate timestamps
- Implement RTC module for offline accuracy
- Fallback implementation uses Unix epoch + millis()
- Ensure format: `1970-01-15T12:34:56Z` (must include date and Z suffix)
- Validate with regex: `^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$`

### Debug Commands

```cpp
// Enable OTA debug logging
mesh.setDebugMsgTypes(ERROR | STARTUP | OTA);

// Check OTA status
Serial.printf("Accepting OTA: %s\n", mesh.isAcceptingOTA() ? "YES" : "NO");

// Monitor memory during OTA
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

// Validate MQTT message locally
#include <ArduinoJson.h>
DynamicJsonDocument doc(1024);
deserializeJson(doc, mqttPayload);
// Check required fields manually
bool valid = doc.containsKey("schema_version") &&
             doc["schema_version"] == 1 &&
             doc.containsKey("metrics");
```

---

## References

- **painlessMesh OTA Plugin:** [src/painlessmesh/ota.hpp](../src/painlessmesh/ota.hpp)
- **MQTT Status Bridge:** [examples/bridge/mqtt_status_bridge.hpp](../examples/bridge/mqtt_status_bridge.hpp)
- **Alteriom MQTT Schema:** https://www.npmjs.com/package/@alteriom/mqtt-schema
- **Phase 2 Guide:** [PHASE2_GUIDE.md](PHASE2_GUIDE.md)
- **Schema Compliance:** [MQTT_SCHEMA_COMPLIANCE.md](MQTT_SCHEMA_COMPLIANCE.md)
- **Phase 2 Implementation:** [improvements/PHASE2_IMPLEMENTATION.md](improvements/PHASE2_IMPLEMENTATION.md)

---

**Last Updated:** October 2024  
**Schema Version:** v1  
**painlessMesh Version:** 1.6.1+  
**Phase:** 2 (Broadcast OTA + MQTT Status Bridge)
