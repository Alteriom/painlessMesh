# MQTT Schema Proposals for painlessMesh Topics

## Overview

This document proposes schema extensions to @alteriom/mqtt-schema to support painlessMesh mesh network monitoring topics that are currently using custom formats.

## Current Status

**Schema v1 Compliant:**
- ✅ `mesh/status/metrics` - Uses `gateway_metrics.schema.json`

**Needs Schema Compliance:**
- ⚠️ `mesh/status/nodes` - Node list (needs new schema)
- ⚠️ `mesh/status/topology` - Mesh structure (needs new schema)
- ⚠️ `mesh/status/alerts` - Alert messages (needs new schema)
- ⚠️ `mesh/status/node/{id}` - Per-node status (can use `sensor_status.schema.json`)

---

## Proposed Schema 1: mesh_node_list.schema.json

**Purpose:** Report list of active nodes in the mesh network

**Topic:** `mesh/status/nodes`

**Proposed Schema:**
```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://schemas.alteriom.io/mqtt/v1/mesh_node_list.schema.json",
  "title": "Mesh Node List v1",
  "allOf": [{"$ref": "envelope.schema.json"}],
  "type": "object",
  "required": ["nodes"],
  "properties": {
    "nodes": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["node_id"],
        "properties": {
          "node_id": {
            "type": "string",
            "description": "Unique node identifier"
          },
          "status": {
            "type": "string",
            "enum": ["online", "offline", "unreachable"],
            "description": "Current node status"
          },
          "last_seen": {
            "type": "string",
            "format": "date-time",
            "description": "Last communication timestamp"
          },
          "signal_strength": {
            "type": "integer",
            "minimum": -200,
            "maximum": 0,
            "description": "Signal strength in dBm"
          }
        },
        "additionalProperties": true
      }
    },
    "node_count": {
      "type": "integer",
      "minimum": 0,
      "description": "Total number of nodes"
    },
    "mesh_id": {
      "type": "string",
      "description": "Mesh network identifier"
    }
  },
  "additionalProperties": true
}
```

**Example Message:**
```json
{
  "schema_version": 1,
  "device_id": "gateway-001",
  "device_type": "gateway",
  "timestamp": "2024-10-11T18:00:00Z",
  "firmware_version": "2.1.0",
  "nodes": [
    {
      "node_id": "123456",
      "status": "online",
      "last_seen": "2024-10-11T18:00:00Z",
      "signal_strength": -45
    },
    {
      "node_id": "789012",
      "status": "online",
      "last_seen": "2024-10-11T17:59:58Z",
      "signal_strength": -62
    }
  ],
  "node_count": 2,
  "mesh_id": "AlteriomMesh"
}
```

---

## Proposed Schema 2: mesh_topology.schema.json

**Purpose:** Report mesh network topology (connections between nodes)

**Topic:** `mesh/status/topology`

**Proposed Schema:**
```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://schemas.alteriom.io/mqtt/v1/mesh_topology.schema.json",
  "title": "Mesh Network Topology v1",
  "allOf": [{"$ref": "envelope.schema.json"}],
  "type": "object",
  "required": ["connections"],
  "properties": {
    "connections": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["from_node", "to_node"],
        "properties": {
          "from_node": {
            "type": "string",
            "description": "Source node ID"
          },
          "to_node": {
            "type": "string",
            "description": "Destination node ID"
          },
          "link_quality": {
            "type": "number",
            "minimum": 0,
            "maximum": 1,
            "description": "Link quality score (0-1)"
          },
          "latency_ms": {
            "type": "integer",
            "minimum": 0,
            "description": "Link latency in milliseconds"
          },
          "hop_count": {
            "type": "integer",
            "minimum": 1,
            "description": "Number of hops in path"
          }
        },
        "additionalProperties": true
      }
    },
    "root_node": {
      "type": "string",
      "description": "Root node ID (gateway/bridge)"
    },
    "total_connections": {
      "type": "integer",
      "minimum": 0,
      "description": "Total number of connections"
    }
  },
  "additionalProperties": true
}
```

**Example Message:**
```json
{
  "schema_version": 1,
  "device_id": "gateway-001",
  "device_type": "gateway",
  "timestamp": "2024-10-11T18:00:00Z",
  "firmware_version": "2.1.0",
  "connections": [
    {
      "from_node": "123456",
      "to_node": "gateway-001",
      "link_quality": 0.95,
      "latency_ms": 12,
      "hop_count": 1
    },
    {
      "from_node": "789012",
      "to_node": "123456",
      "link_quality": 0.82,
      "latency_ms": 25,
      "hop_count": 2
    }
  ],
  "root_node": "gateway-001",
  "total_connections": 2
}
```

---

## Proposed Schema 3: mesh_alert.schema.json

**Purpose:** Report alerts and warnings from mesh network

**Topic:** `mesh/status/alerts`

**Proposed Schema:**
```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://schemas.alteriom.io/mqtt/v1/mesh_alert.schema.json",
  "title": "Mesh Network Alert v1",
  "allOf": [{"$ref": "envelope.schema.json"}],
  "type": "object",
  "required": ["alerts"],
  "properties": {
    "alerts": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["alert_type", "severity", "message"],
        "properties": {
          "alert_type": {
            "type": "string",
            "enum": [
              "low_memory",
              "node_offline",
              "connection_lost",
              "high_latency",
              "packet_loss",
              "firmware_mismatch",
              "configuration_error",
              "security_warning",
              "other"
            ],
            "description": "Type of alert"
          },
          "severity": {
            "type": "string",
            "enum": ["critical", "warning", "info"],
            "description": "Alert severity level"
          },
          "message": {
            "type": "string",
            "description": "Human-readable alert message"
          },
          "node_id": {
            "type": "string",
            "description": "Related node ID (if applicable)"
          },
          "metric_value": {
            "type": "number",
            "description": "Related metric value (if applicable)"
          },
          "threshold": {
            "type": "number",
            "description": "Threshold that triggered alert"
          },
          "alert_id": {
            "type": "string",
            "description": "Unique alert identifier"
          }
        },
        "additionalProperties": true
      }
    },
    "alert_count": {
      "type": "integer",
      "minimum": 0,
      "description": "Total number of active alerts"
    }
  },
  "additionalProperties": true
}
```

**Example Message:**
```json
{
  "schema_version": 1,
  "device_id": "gateway-001",
  "device_type": "gateway",
  "timestamp": "2024-10-11T18:00:00Z",
  "firmware_version": "2.1.0",
  "alerts": [
    {
      "alert_type": "low_memory",
      "severity": "warning",
      "message": "Node 123456 has low free memory",
      "node_id": "123456",
      "metric_value": 15.2,
      "threshold": 20.0,
      "alert_id": "alert-001"
    },
    {
      "alert_type": "node_offline",
      "severity": "critical",
      "message": "Node 345678 is unreachable",
      "node_id": "345678",
      "alert_id": "alert-002"
    }
  ],
  "alert_count": 2
}
```

---

## Existing Schema Usage: Per-Node Status

**Topic:** `mesh/status/node/{id}`

**Use Existing:** `sensor_status.schema.json` (already in @alteriom/mqtt-schema v1)

**Implementation:**
```json
{
  "schema_version": 1,
  "device_id": "123456",
  "device_type": "sensor",
  "timestamp": "2024-10-11T18:00:00Z",
  "firmware_version": "2.1.0",
  "status": "online",
  "battery_level": 85,
  "signal_strength": -45
}
```

---

## Implementation Priority

### Phase 2A (Immediate - Use Existing Schemas)

1. **Per-Node Status** (`mesh/status/node/{id}`)
   - ✅ Use existing `sensor_status.schema.json`
   - Implement immediately with schema v1 compliance
   - Each mesh node publishes its own status

### Phase 2B (Near-term - Needs Schema Proposal)

2. **Node List** (`mesh/status/nodes`)
   - Propose `mesh_node_list.schema.json` to @alteriom/mqtt-schema
   - Implement once schema accepted
   - Gateway publishes aggregated node list

3. **Alerts** (`mesh/status/alerts`)
   - Propose `mesh_alert.schema.json` to @alteriom/mqtt-schema
   - Implement once schema accepted
   - Gateway publishes active alerts

4. **Topology** (`mesh/status/topology`)
   - Propose `mesh_topology.schema.json` to @alteriom/mqtt-schema
   - Implement once schema accepted
   - Gateway publishes network structure

---

## Benefits of Schema Compliance

**For All Topics:**
- ✅ Interoperability across Alteriom ecosystem
- ✅ TypeScript type definitions available
- ✅ Automated validation with ajv
- ✅ Consistent message format
- ✅ Forward compatibility
- ✅ Professional monitoring tool integration

**For Mesh Networks:**
- ✅ Standardized node reporting
- ✅ Centralized alert management
- ✅ Network topology visualization
- ✅ Multi-mesh aggregation support
- ✅ Historical data analysis

---

## Next Steps

### For @alteriom/mqtt-schema Package Maintainers:

1. **Review Proposed Schemas:**
   - `mesh_node_list.schema.json`
   - `mesh_topology.schema.json`
   - `mesh_alert.schema.json`

2. **Provide Feedback:**
   - Suggest modifications to align with Alteriom standards
   - Confirm field names and types
   - Validate against operational requirements

3. **Add to Package:**
   - Include in next @alteriom/mqtt-schema release (v0.4.0+)
   - Update validation rules
   - Add TypeScript type definitions

### For painlessMesh Implementation:

1. **Immediate (Phase 2A):**
   - ✅ Implement per-node status with `sensor_status.schema.json`
   - ✅ Each node publishes to `mesh/status/node/{id}`
   - ✅ Use existing envelope fields and status enum

2. **Near-term (Phase 2B):**
   - Implement proposed schemas (with "x-" prefix if needed before official acceptance)
   - Create validation examples
   - Document integration patterns

3. **Long-term:**
   - Migrate to official schemas once added to @alteriom/mqtt-schema
   - Remove "x-" prefix from schema IDs
   - Update documentation

---

## Schema Proposal Format

**For submission to @alteriom/mqtt-schema:**

```json
{
  "title": "Schema Name",
  "version": "1.0.0",
  "rationale": "Why this schema is needed",
  "use_cases": ["Use case 1", "Use case 2"],
  "schema": { /* JSON Schema */ },
  "examples": [ /* Example messages */ ],
  "validation_notes": "Special validation requirements"
}
```

---

**Document Version:** 1.0  
**Date:** October 2024  
**Status:** Proposal for @alteriom/mqtt-schema maintainers  
**Contact:** painlessMesh contributors
