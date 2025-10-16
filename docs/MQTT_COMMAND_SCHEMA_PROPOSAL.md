# MQTT Command Schema Proposal for @alteriom/mqtt-schema v0.5.0

**Date:** October 12, 2025  
**Proposed By:** Alteriom Firmware Team  
**Target Version:** @alteriom/mqtt-schema v0.5.0  
**Status:** PROPOSAL

---

## Executive Summary

This proposal adds standardized command and command response schemas to @alteriom/mqtt-schema to enable bidirectional control flow between MQTT clients (web apps, automation systems) and IoT devices (sensors, gateways).

**Current State (v0.4.0):**
- ✅ Sensor data telemetry (sensor_data.schema.json)
- ✅ Gateway metrics and status
- ✅ Control responses (control_response.schema.json) - **EXISTS BUT LIMITED**
- ❌ Device control commands - **MISSING**

**Proposed Addition:**
- New schema: `command.schema.json` - Standardized command structure
- Enhanced schema: `command_response.schema.json` - Extend control_response with correlation tracking

---

## Use Case

**Scenario**: Web dashboard needs to trigger immediate sensor reading

**Current Workaround**: Custom JSON format, no validation
```json
{
  "type": "command",
  "node_id": 1693975713,
  "command": "read_sensors"
}
```
❌ Not schema-validated  
❌ No timestamp  
❌ No correlation tracking  
❌ No response linking

**Proposed Standard**:
```json
{
  "schema_version": 1,
  "device_id": "ALT-441D64F804A0",
  "device_type": "sensor",
  "timestamp": "2025-10-12T14:30:00Z",
  "firmware_version": "SN 2.3.4",
  "event": "command",
  "command": "read_sensors",
  "correlation_id": "cmd-1728745800-001",
  "parameters": {
    "immediate": true,
    "sensors": ["temperature", "humidity"]
  }
}
```
✅ Schema-validated  
✅ Envelope compliance  
✅ Correlation tracking  
✅ Typed parameters  

---

## Proposed Schema: command.schema.json

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://schemas.alteriom.io/mqtt/v1/command.schema.json",
  "title": "Device Command v1",
  "description": "Command message sent from MQTT client to IoT device for control operations",
  "allOf": [{"$ref": "envelope.schema.json"}],
  "type": "object",
  "required": ["event", "command"],
  "properties": {
    "event": {
      "type": "string",
      "const": "command",
      "description": "Event type discriminator"
    },
    "command": {
      "type": "string",
      "minLength": 1,
      "maxLength": 64,
      "pattern": "^[a-z][a-z0-9_]*$",
      "description": "Command name in snake_case (e.g., read_sensors, set_interval, restart)",
      "examples": [
        "read_sensors",
        "set_interval",
        "enable_sensor",
        "update_config",
        "restart",
        "get_status"
      ]
    },
    "correlation_id": {
      "type": "string",
      "minLength": 1,
      "maxLength": 128,
      "pattern": "^[A-Za-z0-9_-]+$",
      "description": "Unique identifier for tracking command → response lifecycle"
    },
    "parameters": {
      "type": "object",
      "description": "Command-specific parameters (validated by device)",
      "additionalProperties": true,
      "examples": [
        {"interval": 30000},
        {"sensor": "temperature", "enabled": true},
        {"immediate": true, "sensors": ["temperature", "humidity"]}
      ]
    },
    "timeout_ms": {
      "type": "integer",
      "minimum": 1000,
      "maximum": 300000,
      "default": 5000,
      "description": "Command execution timeout in milliseconds"
    },
    "priority": {
      "type": "string",
      "enum": ["low", "normal", "high", "urgent"],
      "default": "normal",
      "description": "Command priority for queue management"
    }
  },
  "additionalProperties": true
}
```

### Field Specifications

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema_version` | integer | ✅ | Always 1 (from envelope) |
| `device_id` | string | ✅ | Target device ID (from envelope) |
| `device_type` | string | ✅ | "sensor" or "gateway" (from envelope) |
| `timestamp` | string (ISO 8601) | ✅ | Command creation time (from envelope) |
| `firmware_version` | string | ✅ | Sender firmware version (from envelope) |
| `event` | "command" | ✅ | Event type discriminator |
| `command` | string | ✅ | Command name (snake_case) |
| `correlation_id` | string | ❌ | Unique tracking ID (recommended) |
| `parameters` | object | ❌ | Command-specific parameters |
| `timeout_ms` | integer | ❌ | Execution timeout (default: 5000ms) |
| `priority` | enum | ❌ | Queue priority (default: "normal") |

---

## Enhanced Schema: command_response.schema.json

**Note**: This extends/replaces the existing `control_response.schema.json`

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://schemas.alteriom.io/mqtt/v1/command_response.schema.json",
  "title": "Command Response v1",
  "description": "Response message from IoT device after executing a command",
  "allOf": [{"$ref": "envelope.schema.json"}],
  "type": "object",
  "required": ["event", "success"],
  "properties": {
    "event": {
      "type": "string",
      "const": "command_response",
      "description": "Event type discriminator"
    },
    "command": {
      "type": "string",
      "minLength": 1,
      "maxLength": 64,
      "description": "Original command name that was executed"
    },
    "correlation_id": {
      "type": "string",
      "minLength": 1,
      "maxLength": 128,
      "pattern": "^[A-Za-z0-9_-]+$",
      "description": "Matches correlation_id from original command"
    },
    "success": {
      "type": "boolean",
      "description": "Whether command execution succeeded"
    },
    "result": {
      "type": ["object", "array", "string", "number", "boolean", "null"],
      "description": "Command execution result data"
    },
    "message": {
      "type": "string",
      "maxLength": 256,
      "description": "Human-readable status message"
    },
    "error_code": {
      "type": "string",
      "maxLength": 64,
      "description": "Machine-readable error code (e.g., TIMEOUT, INVALID_PARAMS)"
    },
    "latency_ms": {
      "type": "integer",
      "minimum": 0,
      "description": "Time taken to execute command in milliseconds"
    }
  },
  "additionalProperties": true
}
```

### Field Specifications

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema_version` | integer | ✅ | Always 1 (from envelope) |
| `device_id` | string | ✅ | Responding device ID (from envelope) |
| `device_type` | string | ✅ | "sensor" or "gateway" (from envelope) |
| `timestamp` | string (ISO 8601) | ✅ | Response creation time (from envelope) |
| `firmware_version` | string | ✅ | Device firmware version (from envelope) |
| `event` | "command_response" | ✅ | Event type discriminator |
| `command` | string | ❌ | Original command name |
| `correlation_id` | string | ❌ | Links to original command (strongly recommended) |
| `success` | boolean | ✅ | Execution success/failure |
| `result` | any | ❌ | Command result data |
| `message` | string | ❌ | Human-readable message |
| `error_code` | string | ❌ | Machine-readable error code |
| `latency_ms` | integer | ❌ | Execution time |

---

## Complete Examples

### Example 1: Read Sensors Command

**MQTT Topic**: `alteriom/nodes/ALT-441D64F804A0/commands`

**Command Payload**:
```json
{
  "schema_version": 1,
  "device_id": "ALT-441D64F804A0",
  "device_type": "sensor",
  "timestamp": "2025-10-12T14:30:00Z",
  "firmware_version": "WEB 1.0.0",
  "event": "command",
  "command": "read_sensors",
  "correlation_id": "cmd-1728745800-001",
  "parameters": {
    "immediate": true,
    "sensors": ["temperature", "humidity", "pressure"]
  },
  "timeout_ms": 5000,
  "priority": "high"
}
```

**Response Topic**: `alteriom/nodes/ALT-441D64F804A0/responses`

**Response Payload**:
```json
{
  "schema_version": 1,
  "device_id": "ALT-441D64F804A0",
  "device_type": "sensor",
  "timestamp": "2025-10-12T14:30:02Z",
  "firmware_version": "SN 2.3.4",
  "event": "command_response",
  "command": "read_sensors",
  "correlation_id": "cmd-1728745800-001",
  "success": true,
  "result": {
    "sensors_read": ["temperature", "humidity", "pressure"],
    "values": {
      "temperature": 23.5,
      "humidity": 45,
      "pressure": 1013
    }
  },
  "message": "Sensors read successfully",
  "latency_ms": 1250
}
```

### Example 2: Set Interval Command

**Command**:
```json
{
  "schema_version": 1,
  "device_id": "ALT-441D64F804A0",
  "device_type": "sensor",
  "timestamp": "2025-10-12T14:35:00Z",
  "firmware_version": "WEB 1.0.0",
  "event": "command",
  "command": "set_interval",
  "correlation_id": "cmd-1728746100-002",
  "parameters": {
    "interval": 60000
  }
}
```

**Response**:
```json
{
  "schema_version": 1,
  "device_id": "ALT-441D64F804A0",
  "device_type": "sensor",
  "timestamp": "2025-10-12T14:35:01Z",
  "firmware_version": "SN 2.3.4",
  "event": "command_response",
  "command": "set_interval",
  "correlation_id": "cmd-1728746100-002",
  "success": true,
  "result": {
    "old_interval": 30000,
    "new_interval": 60000
  },
  "message": "Reading interval updated to 60s",
  "latency_ms": 450
}
```

### Example 3: Command Error

**Command**:
```json
{
  "schema_version": 1,
  "device_id": "ALT-441D64F804A0",
  "device_type": "sensor",
  "timestamp": "2025-10-12T14:40:00Z",
  "firmware_version": "WEB 1.0.0",
  "event": "command",
  "command": "enable_sensor",
  "correlation_id": "cmd-1728746400-003",
  "parameters": {
    "sensor": "gps",
    "enabled": true
  }
}
```

**Error Response**:
```json
{
  "schema_version": 1,
  "device_id": "ALT-441D64F804A0",
  "device_type": "sensor",
  "timestamp": "2025-10-12T14:40:01Z",
  "firmware_version": "SN 2.3.4",
  "event": "command_response",
  "command": "enable_sensor",
  "correlation_id": "cmd-1728746400-003",
  "success": false,
  "result": null,
  "message": "GPS sensor not available on this device",
  "error_code": "SENSOR_NOT_AVAILABLE",
  "latency_ms": 120
}
```

### Example 4: Command Timeout

**Gateway-Generated Error Response** (when device doesn't respond):
```json
{
  "schema_version": 1,
  "device_id": "ALT-6825DD341CA4",
  "device_type": "gateway",
  "timestamp": "2025-10-12T14:45:05Z",
  "firmware_version": "GW 2.3.4",
  "event": "command_response",
  "command": "restart",
  "correlation_id": "cmd-1728746700-004",
  "success": false,
  "result": {
    "target_device": "ALT-441D64F804A0",
    "timeout_ms": 5000
  },
  "message": "Command timeout - device did not respond within 5000ms",
  "error_code": "TIMEOUT",
  "latency_ms": 5000
}
```

---

## Standard Command Names

To promote interoperability, we recommend these standard command names:

### Device Control
- `restart` - Reboot device
- `factory_reset` - Reset to factory defaults
- `sleep` - Enter low-power mode
- `wake` - Exit low-power mode

### Sensor Operations
- `read_sensors` - Trigger immediate sensor reading
- `enable_sensor` - Enable/disable specific sensor
- `calibrate_sensor` - Calibrate sensor
- `set_interval` - Update reading interval

### Configuration
- `get_config` - Request current configuration
- `set_config` - Update configuration
- `save_config` - Persist config to flash
- `get_status` - Request device status

### Network
- `scan_networks` - Scan WiFi/LoRa networks
- `connect` - Connect to network
- `disconnect` - Disconnect from network

### Firmware
- `start_ota` - Begin OTA update
- `cancel_ota` - Cancel OTA update
- `get_version` - Request firmware version

### Diagnostics
- `run_diagnostics` - Run self-test
- `get_logs` - Retrieve device logs
- `clear_logs` - Clear log storage

**Note**: Devices may implement custom commands with `custom_` prefix (e.g., `custom_led_pattern`)

---

## Error Codes Reference

Standard error codes for `error_code` field:

| Code | Description |
|------|-------------|
| `UNKNOWN_COMMAND` | Command name not recognized |
| `INVALID_PARAMS` | Invalid or missing parameters |
| `TIMEOUT` | Command execution timeout |
| `DEVICE_BUSY` | Device busy, cannot execute |
| `PERMISSION_DENIED` | Insufficient permissions |
| `SENSOR_NOT_AVAILABLE` | Sensor not present/enabled |
| `CONFIG_ERROR` | Configuration update failed |
| `NETWORK_ERROR` | Network operation failed |
| `FIRMWARE_ERROR` | Firmware operation failed |
| `STORAGE_ERROR` | Storage operation failed |
| `HARDWARE_ERROR` | Hardware failure detected |
| `RATE_LIMITED` | Too many commands |

---

## Migration Path

### From control_response.schema.json to command_response.schema.json

**Old Format** (control_response.schema.json):
```json
{
  "schema_version": 1,
  "device_id": "ALT-441D64F804A0",
  "device_type": "sensor",
  "timestamp": "2025-10-12T14:30:02Z",
  "firmware_version": "SN 2.3.4",
  "command": "read_sensors",
  "status": "ok",
  "message": "Sensors read successfully",
  "result": {"temperature": 23.5}
}
```

**New Format** (command_response.schema.json):
```json
{
  "schema_version": 1,
  "device_id": "ALT-441D64F804A0",
  "device_type": "sensor",
  "timestamp": "2025-10-12T14:30:02Z",
  "firmware_version": "SN 2.3.4",
  "event": "command_response",
  "command": "read_sensors",
  "correlation_id": "cmd-1728745800-001",
  "success": true,
  "message": "Sensors read successfully",
  "result": {"temperature": 23.5},
  "latency_ms": 1250
}
```

**Key Changes**:
1. Added `event: "command_response"` discriminator
2. Replaced `status: "ok"|"error"` with `success: boolean`
3. Added `correlation_id` for request tracking
4. Added `latency_ms` for performance monitoring
5. Added `error_code` for machine-readable errors

**Backward Compatibility**:
- Old `control_response.schema.json` can coexist
- Validators can accept both formats during transition
- Recommend deprecating `control_response` in v0.6.0

---

## Implementation Checklist

### For @alteriom/mqtt-schema Repository

- [ ] Create `schemas/command.schema.json`
- [ ] Create `schemas/command_response.schema.json`
- [ ] Update `mqtt_v1_bundle.json` to include new schemas
- [ ] Generate TypeScript types from schemas
- [ ] Add Ajv validators for command and command_response
- [ ] Update `classifyAndValidate()` to recognize `event: "command"`
- [ ] Add type guards: `isCommandMessage()`, `isCommandResponseMessage()`
- [ ] Update README.md with command examples
- [ ] Add validation tests for command schemas
- [ ] Update CHANGELOG.md for v0.5.0
- [ ] Deprecation notice for `control_response.schema.json` (remove in v0.6.0)

### For alteriom-firmware Repository

- [ ] Update `package.json` to `@alteriom/mqtt-schema@^0.5.0`
- [ ] Implement `mqtt_command_bridge.cpp` with schema validation
- [ ] Update `mqtt_spec_builder.cpp` to support command messages
- [ ] Update `mqtt_spec_builder.cpp` to support command_response messages
- [ ] Add command name constants to `mqtt_spec_builder.h`
- [ ] Update documentation with command examples
- [ ] Add integration tests for command flow
- [ ] Update CHANGELOG.md

---

## Benefits

1. **Standardization**: All commands follow same structure
2. **Validation**: JSON schema validation catches errors early
3. **Traceability**: Correlation IDs link commands to responses
4. **Monitoring**: Latency tracking enables performance analysis
5. **Interoperability**: Standard command names work across vendors
6. **Error Handling**: Machine-readable error codes
7. **Type Safety**: TypeScript types generated from schemas
8. **Documentation**: Self-documenting via JSON schema

---

## Questions for Schema Maintainers

1. **Naming Convention**: Should we use `command` or `control` for the event type?
   - Proposal: `event: "command"` for requests, `event: "command_response"` for responses
   - Alternative: `event: "control"` / `event: "control_response"`

2. **Priority Field**: Should priority be standardized or left to implementation?
   - Current proposal: Standard enum with optional usage

3. **Timeout Field**: Should timeout be in command or handled by client?
   - Current proposal: Optional field in command message

4. **Backward Compatibility**: Deprecate `control_response.schema.json` now or later?
   - Current proposal: Deprecate in v0.5.0, remove in v0.6.0

5. **Custom Commands**: Should we enforce `custom_` prefix or allow any name?
   - Current proposal: Recommend `custom_` prefix, but allow any valid snake_case

---

## References

- [@alteriom/mqtt-schema v0.4.0](https://www.npmjs.com/package/@alteriom/mqtt-schema)
- [JSON Schema Draft 2020-12](https://json-schema.org/draft/2020-12/schema)
- [MQTT Topics Best Practices](https://www.hivemq.com/blog/mqtt-essentials-part-5-mqtt-topics-best-practices/)
- [alteriom-firmware MQTT_BRIDGE_IMPROVEMENTS.md](../analysis/MQTT_BRIDGE_IMPROVEMENTS.md)

---

**Submitted By**: Alteriom Firmware Development Team  
**Contact**: [GitHub Issues](https://github.com/Alteriom/alteriom-mqtt-schema/issues)  
**Date**: October 12, 2025
