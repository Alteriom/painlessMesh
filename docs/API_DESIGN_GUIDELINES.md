# API Design Guidelines for Alteriom Packages

This document provides guidelines for designing consistent and maintainable JSON configuration structures in Alteriom packages, particularly for StatusPackage and related message types.

## Table of Contents

- [Overview](#overview)
- [Nesting vs Flat Structure Guidelines](#nesting-vs-flat-structure-guidelines)
- [Current Structure Patterns](#current-structure-patterns)
- [Decision Tree](#decision-tree)
- [Examples](#examples)
- [Best Practices](#best-practices)

## Overview

Alteriom packages use JSON serialization for configuration and status data. This document establishes clear patterns for when to use nested structures versus flat key-value pairs to ensure consistency and maintainability across the codebase.

### Key Principles

1. **Consistency over perfection** - Follow existing patterns in similar sections
2. **Simplicity by default** - Use flat structures unless nesting provides clear benefits
3. **Future-proof** - Consider extensibility when designing structures
4. **Clarity** - Structure should reflect logical grouping

## Nesting vs Flat Structure Guidelines

### Use FLAT Structure When:

- **< 4 total fields** in a configuration section
- **No clear logical subsystems** within the section
- **Simple value types** without complex relationships
- **Low likelihood of expansion** in the future

**Benefits:**
- Simpler code (fewer nested object creations)
- Easier to parse and validate
- More concise JSON output
- Faster serialization/deserialization

### Use NESTED Structure When:

- **3+ fields belong to same logical subsystem**
- **Clear semantic grouping** exists
- **Future extensibility anticipated** for subsystem
- **Subsystem has distinct meaning** separate from parent

**Benefits:**
- Better logical organization
- Easier to add related fields without cluttering parent
- Clear separation of concerns
- More extensible architecture

## Current Structure Patterns

### Flat Sections (No Nesting)

These sections use simple key-value pairs:

#### Display Configuration
```json
"display_config": {
  "enabled": true,
  "brightness": 128,
  "timeout_ms": 30000,
  "timeout_s": 30
}
```

**Rationale:** Only 3-4 fields, all directly related to display, no subsystems.

#### Power Configuration
```json
"power_config": {
  "deep_sleep_enabled": false,
  "deep_sleep_interval_ms": 300000,
  "deep_sleep_interval_s": 300,
  "battery_percent": 85
}
```

**Rationale:** Small number of fields (4), even though battery and sleep are different concerns, nesting would add unnecessary complexity.

#### MQTT Retry Configuration
```json
"mqtt_retry": {
  "max_attempts": 5,
  "circuit_breaker_ms": 60000,
  "circuit_breaker_s": 60,
  "hourly_retry_enabled": true,
  "initial_retry_ms": 1000,
  "initial_retry_s": 1,
  "max_retry_ms": 30000,
  "max_retry_s": 30,
  "backoff_multiplier": 2.0
}
```

**Rationale:** While this has 9 fields with distinct concerns (retry policy vs backoff strategy), it remains flat for simplicity. The retry configuration is cohesive enough that nesting would fragment it without clear benefit.

### Nested Sections (With Subsystems)

These sections use nested objects for logical grouping:

#### Sensor Configuration with Calibration
```json
"sensors": {
  "read_interval_ms": 30000,
  "read_interval_s": 30,
  "transmission_interval_ms": 60000,
  "transmission_interval_s": 60,
  "calibration": {
    "temperature_offset": 0.5,
    "humidity_offset": -2.0,
    "pressure_offset": 0.0
  }
}
```

**Rationale:** Calibration is a distinct subsystem with its own semantic meaning. It's optional, extensible, and conceptually separate from sensor timing configuration.

**Benefits of nesting here:**
- Calibration can be added/removed as a unit
- Easy to add more calibration fields without cluttering main sensors object
- Clear semantic boundary - calibration is a specific tuning operation

#### Organization Metadata
```json
"organization": {
  "organizationId": "org-123",
  "customerId": "cust-456",
  "deviceGroup": "sensors",
  "device_name": "sensor-01",
  "device_location": "warehouse-a",
  "device_secret_set": true
}
```

**Rationale:** Organization metadata is an optional, self-contained subsystem that may not be present on all devices.

## Decision Tree

Use this decision tree when designing new configuration sections:

```
START: New configuration section needed
│
├─ Does section have < 4 fields?
│  ├─ YES → Use FLAT structure
│  └─ NO → Continue
│
├─ Do 3+ fields belong to same logical subsystem?
│  ├─ NO → Use FLAT structure
│  └─ YES → Continue
│
├─ Is subsystem likely to grow in future?
│  ├─ NO → Consider FLAT (unless strong semantic grouping)
│  └─ YES → Continue
│
├─ Would nesting improve clarity significantly?
│  ├─ NO → Use FLAT structure
│  └─ YES → Use NESTED structure
│
END
```

## Examples

### Example 1: Adding OTA Configuration (Flat Approach)

**Scenario:** Adding Over-The-Air update configuration with 3 fields.

```cpp
// C++ Fields
bool otaEnabled = false;
TSTRING otaServer = "";
uint16_t otaPort = 0;

// JSON Serialization (FLAT)
JsonObject ota = jsonObj["ota"].to<JsonObject>();
ota["enabled"] = otaEnabled;
ota["server"] = otaServer;
ota["port"] = otaPort;
```

**Result:**
```json
"ota": {
  "enabled": true,
  "server": "ota.example.com",
  "port": 8080
}
```

**Decision:** Keep FLAT - only 3 fields, no subsystems.

### Example 2: Adding Sensor Thresholds (Nested Approach)

**Scenario:** Adding temperature, humidity, and pressure thresholds to sensor configuration.

```cpp
// C++ Fields (added to existing sensor config)
double tempThresholdMin = -40.0;
double tempThresholdMax = 85.0;
double humidityThresholdMin = 0.0;
double humidityThresholdMax = 100.0;

// JSON Serialization (NESTED under sensors)
JsonObject sensors = jsonObj["sensors"].to<JsonObject>();
// ... existing sensor fields ...

JsonObject thresholds = sensors["thresholds"].to<JsonObject>();
thresholds["temperature_min"] = tempThresholdMin;
thresholds["temperature_max"] = tempThresholdMax;
thresholds["humidity_min"] = humidityThresholdMin;
thresholds["humidity_max"] = humidityThresholdMax;
```

**Result:**
```json
"sensors": {
  "read_interval_ms": 30000,
  "calibration": { ... },
  "thresholds": {
    "temperature_min": -40.0,
    "temperature_max": 85.0,
    "humidity_min": 0.0,
    "humidity_max": 100.0
  }
}
```

**Decision:** Use NESTED - 4+ fields, clear subsystem (alerting/validation logic), logical grouping.

### Example 3: Adding Encoding Configuration (Flat Approach)

**Scenario:** Adding message encoding/compression settings.

```cpp
// C++ Fields
bool compressionEnabled = false;
TSTRING encodingType = "json";
uint8_t compressionLevel = 6;

// JSON Serialization (FLAT)
JsonObject encoding = jsonObj["encoding"].to<JsonObject>();
encoding["compression_enabled"] = compressionEnabled;
encoding["encoding_type"] = encodingType;
encoding["compression_level"] = compressionLevel;
```

**Result:**
```json
"encoding": {
  "compression_enabled": false,
  "encoding_type": "json",
  "compression_level": 6
}
```

**Decision:** Keep FLAT - only 3 fields, cohesive purpose.

## Best Practices

### 1. Be Conservative with Nesting

**Rationale:** Flat structures are simpler to implement and consume.

**Rule:** When in doubt, start flat. Nesting can be added later if needed, but removing nesting is a breaking change.

### 2. Consider Backward Compatibility

When adding fields to existing sections:

```cpp
// Deserialization with backward compatibility
displayTimeout = displayConfig["timeout_ms"] | displayConfig["timeout"] | 0;
```

This allows reading old format (`timeout`) while preferring new format (`timeout_ms`).

### 3. Group Optional Subsystems

**Good:**
```json
"sensors": {
  "read_interval_ms": 30000,
  "calibration": { ... }  // Optional, can be omitted entirely
}
```

**Avoid:**
```json
"sensors": {
  "read_interval_ms": 30000,
  "temperature_offset": 0.0,  // Mixed levels - unclear if calibration is a concept
  "humidity_offset": 0.0,
  "pressure_offset": 0.0
}
```

### 4. Maintain Consistent Field Naming

Follow existing conventions:
- Time fields: Follow [Time Field Naming Convention](../examples/alteriom/alteriom_sensor_package.hpp#L10-L55)
- Boolean fields: Follow [Boolean Naming Convention](BOOLEAN_NAMING_CONVENTION.md)
- Use snake_case for JSON keys
- Use camelCase for C++ field names

### 5. Document Structure Decisions

Add comments explaining nesting choices:

```cpp
// Display configuration (flat - only 3 fields, no subsystems)
JsonObject displayConfig = jsonObj["display_config"].to<JsonObject>();

// Sensor configuration with nested calibration (calibration is distinct subsystem)
JsonObject sensors = jsonObj["sensors"].to<JsonObject>();
JsonObject calibration = sensors["calibration"].to<JsonObject>();
```

### 6. Test Structure Consistency

Create tests to validate structure patterns:

```cpp
TEST(StatusPackage, StructureConsistency) {
    alteriom::StatusPackage pkg;
    pkg.tempOffset = 0.5;
    
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    pkg.addTo(std::move(obj));
    
    // Verify nested structures
    REQUIRE(obj["sensors"]["calibration"].is<JsonObject>());
    
    // Verify flat structures remain flat
    REQUIRE(obj["display_config"]["enabled"].is<bool>());
    REQUIRE_FALSE(obj["display_config"].containsKey("nested_section"));
}
```

## Migration Path

If restructuring becomes necessary:

1. **Add new structure** while maintaining old structure
2. **Support both formats** in deserialization
3. **Deprecate old format** (document in release notes)
4. **Remove old format** in next major version

```cpp
// Example: Supporting both flat and nested
if (jsonObj["network"]["wifi"].is<JsonObject>()) {
    // New nested format
    JsonObject wifi = jsonObj["network"]["wifi"];
    wifiSSID = wifi["ssid"].as<TSTRING>();
} else {
    // Old flat format (deprecated)
    wifiSSID = jsonObj["network"]["wifi_ssid"].as<TSTRING>();
}
```

## Summary

| Criteria | Flat Structure | Nested Structure |
|----------|---------------|------------------|
| **Field Count** | < 4 fields | 3+ fields in subsystem |
| **Logical Grouping** | No clear subsystems | Clear semantic grouping |
| **Extensibility** | Low likelihood of growth | Anticipated expansion |
| **Complexity** | Simple values | Complex relationships |
| **Examples** | display_config, power_config, ota, encoding | sensors.calibration, organization |

**Golden Rule:** When uncertain, prefer flat structures. Nesting should provide clear organizational or extensibility benefits to justify the added complexity.

## References

- [StatusPackage Implementation](../examples/alteriom/alteriom_sensor_package.hpp)
- [Boolean Naming Convention](BOOLEAN_NAMING_CONVENTION.md)
- [Time Field Naming Convention](../examples/alteriom/alteriom_sensor_package.hpp#L10-L55)
- [Test Cases](../test/catch/catch_alteriom_packages.cpp)

## Revision History

- **2025-11-04**: Initial version documenting StatusPackage nesting patterns
