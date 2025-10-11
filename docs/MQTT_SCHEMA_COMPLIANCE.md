# Alteriom MQTT Schema Compliance

## Overview

The Phase 2 MQTT Status Bridge is compliant with the **@alteriom/mqtt-schema v1** specification for gateway metrics messages. This ensures interoperability with other Alteriom services and monitoring tools.

## Schema Package

- **Package:** `@alteriom/mqtt-schema` v0.3.2+
- **Registry:** npm (https://www.npmjs.com/package/@alteriom/mqtt-schema)
- **Documentation:** https://github.com/Alteriom/alteriom-mqtt-schema

## Compliance Status

### Gateway Metrics (mesh/status/metrics)

✅ **Fully Compliant** with `gateway_metrics.schema.json` v1

**Required Envelope Fields:**
- `schema_version`: 1 (integer)
- `device_id`: Mesh node ID or custom identifier
- `device_type`: "gateway"
- `timestamp`: ISO 8601 format (YYYY-MM-DDTHH:MM:SSZ)
- `firmware_version`: String (configurable, default: "1.0.0")

**Metrics Object:**
- `uptime_s`: Uptime in seconds
- `mesh_nodes`: Number of connected mesh nodes
- `memory_usage_pct`: Memory usage percentage
- `connected_devices`: Number of connected devices

**Example Message:**
```json
{
  "schema_version": 1,
  "device_id": "123456",
  "device_type": "gateway",
  "timestamp": "2024-01-01T12:34:56Z",
  "firmware_version": "1.0.0",
  "metrics": {
    "uptime_s": 3600,
    "mesh_nodes": 5,
    "memory_usage_pct": 45.2,
    "connected_devices": 5
  }
}
```

### Other Topics

The following topics are **not yet schema-compliant** (custom format):
- `mesh/status/nodes` - Custom format (node list)
- `mesh/status/topology` - painlessMesh native topology JSON
- `mesh/status/alerts` - Custom alert format
- `mesh/status/node/{id}` - Custom per-node format

**Future Work:** These could be aligned with sensor_status or custom schemas if needed.

## Configuration

### Setting Device Information

```cpp
MqttStatusBridge bridge(mesh, mqttClient);

// Set device ID (defaults to mesh node ID)
bridge.setDeviceId("gateway-001");

// Set firmware version (defaults to "1.0.0")
bridge.setFirmwareVersion("2.1.3");

bridge.begin();
```

### Timestamp Generation

The implementation generates timestamps in ISO 8601 format compliant with the schema requirement.

**Production Recommendations:**
- **Preferred:** Use NTP time sync via WiFi or mesh time synchronization
- **Alternative:** Use RTC (Real-Time Clock) module for accurate timestamps
- **Fallback:** Current implementation uses Unix epoch (1970-01-01) + millis()

**Format:** `YYYY-MM-DDTHH:MM:SSZ` (ISO 8601 with Zulu time)

**Example Production Implementation:**
```cpp
// With NTP (recommended)
#include <WiFi.h>
#include <time.h>

String getISO8601Timestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "1970-01-01T00:00:00Z";  // Fallback
  }
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buffer);
}

// Configure NTP in setup()
configTime(0, 0, "pool.ntp.org");
```

## Validation

To validate messages against the schema:

```javascript
// Node.js example
const { validators } = require('@alteriom/mqtt-schema');

const message = JSON.parse(mqttPayload);
const result = validators.gatewayMetrics(message);

if (!result.valid) {
  console.error('Validation errors:', result.errors);
}
```

## Benefits of Compliance

1. **Interoperability**: Works with other Alteriom services
2. **Validation**: Messages can be validated against published schemas
3. **Type Safety**: TypeScript types available from the schema package
4. **Documentation**: Schema serves as API documentation
5. **Evolution**: Forward-compatible with schema versioning

## Dependencies

Add to `package.json`:
```json
{
  "devDependencies": {
    "@alteriom/mqtt-schema": "^0.3.2",
    "ajv": "^8.17.0",
    "ajv-formats": "^2.1.1"
  }
}
```

## Migration Notes

If you have existing MQTT consumers expecting the old format:

**Old Format:**
```json
{
  "nodeCount": 5,
  "rootNodeId": 123456,
  "uptime": 3600,
  "freeHeap": 45000,
  "timestamp": 1234567890
}
```

**New Format (Schema v1):**
```json
{
  "schema_version": 1,
  "device_id": "123456",
  "device_type": "gateway",
  "timestamp": "2024-01-01T12:34:56Z",
  "firmware_version": "1.0.0",
  "metrics": {
    "uptime_s": 3600,
    "mesh_nodes": 5,
    "memory_usage_pct": 45.2,
    "connected_devices": 5
  }
}
```

**Migration Path:**
1. Update MQTT consumers to handle both formats during transition
2. Use `schema_version` field to detect new format
3. Gradually migrate all publishers to new format
4. Remove old format support once migration complete

## References

- Schema Package: https://www.npmjs.com/package/@alteriom/mqtt-schema
- Schema Repository: https://github.com/Alteriom/alteriom-mqtt-schema
- JSON Schema Spec: https://json-schema.org/
- Phase 2 Guide: [PHASE2_GUIDE.md](PHASE2_GUIDE.md)
- Implementation: [mqtt_status_bridge.hpp](../examples/bridge/mqtt_status_bridge.hpp)

## Future Enhancements

Potential schema alignment for other message types:
- Align node list with sensor_status schema
- Create custom mesh_topology schema
- Standardize alert format across Alteriom ecosystem
- Add validation helpers in the bridge class

---

**Last Updated:** October 2024  
**Schema Version:** v1  
**Package Version:** @alteriom/mqtt-schema@0.3.2
