# Boolean Field Naming Convention

## Overview

This document establishes the standard naming conventions for boolean fields in Alteriom packages, particularly in `StatusPackage`. Consistent naming patterns make the code self-documenting and help developers understand field semantics at a glance.

## Three Naming Patterns

### Pattern 1: `*Set` Suffix

**Purpose**: Indicates that required configuration data has been provided (typically for sensitive data like passwords or secrets).

**When to use**:
- Field represents whether configuration data exists
- Typically used for passwords, secrets, API keys, server URLs
- Does NOT indicate if the feature is active or working

**Examples**:
```cpp
bool deviceSecretSet = false;  // Has device secret been configured?
bool wifiPasswordSet = false;  // Has WiFi password been provided?
bool meshPasswordSet = false;  // Has mesh password been provided?
bool otaServerSet = false;     // Has OTA server URL been configured?
bool mqttBrokerSet = false;    // Has MQTT broker been configured?
```

**Semantic meaning**:
- `true` = Configuration data has been provided
- `false` = Configuration data is missing or not yet provided
- Does NOT indicate the feature is enabled or currently working

### Pattern 2: `*Enabled` Suffix

**Purpose**: Indicates that a feature is currently active or turned on.

**When to use**:
- Field represents a feature toggle (on/off)
- User or system can enable/disable the feature
- Feature state is controllable and intentional

**Examples**:
```cpp
bool displayEnabled = false;           // Is display feature enabled?
bool deepSleepEnabled = false;         // Is deep sleep mode enabled?
bool mqttHourlyRetryEnabled = false;   // Is hourly retry feature enabled?
bool otaEnabled = false;               // Are OTA updates enabled?
bool encryptionEnabled = false;        // Is data encryption enabled?
bool encodingEnabled = false;          // Is data encoding enabled?
bool logTimestampEnabled = false;      // Are log timestamps enabled?
```

**Semantic meaning**:
- `true` = Feature is currently active/turned on
- `false` = Feature is currently inactive/turned off
- Independent of whether required configuration exists

### Pattern 3: Runtime State (`is*` Prefix or `*Connected`)

**Purpose**: Indicates current runtime status or operational state (not configuration).

**When to use**:
- Field represents current operational status
- Status changes at runtime based on system behavior
- Not directly controlled by configuration

**Examples**:
```cpp
bool isConfigured = false;    // Has device completed configuration?
bool mqttConnected = false;   // Currently connected to MQTT broker?
bool meshIsRoot = false;      // Is this node currently the mesh root?
bool isOnline = false;        // Is device currently online?
bool wifiConnected = false;   // Currently connected to WiFi?
```

**Semantic meaning**:
- `true` = Currently in this state
- `false` = Not currently in this state
- Reflects actual runtime conditions, not configuration

## Combining Patterns

A feature may legitimately have multiple boolean fields using different patterns:

### Example: OTA (Over-The-Air) Updates

```cpp
bool otaServerSet = false;     // Has OTA server URL been configured? (*Set)
bool otaEnabled = false;       // Are OTA updates enabled? (*Enabled)
bool otaInProgress = false;    // Is an OTA update currently running? (is* / runtime state)
```

**Valid states**:
- `otaServerSet=true, otaEnabled=false` → Server configured but feature disabled
- `otaServerSet=false, otaEnabled=true` → Feature enabled but no server (invalid/warning state)
- `otaServerSet=true, otaEnabled=true` → Fully configured and active
- `otaServerSet=true, otaEnabled=true, otaInProgress=true` → Update in progress

### Example: MQTT Connection

```cpp
bool mqttBrokerSet = false;    // Has MQTT broker been configured? (*Set)
bool mqttEnabled = false;      // Is MQTT feature enabled? (*Enabled)
bool mqttConnected = false;    // Currently connected to broker? (runtime state)
```

## Current StatusPackage Implementation

### Existing `*Set` Fields (Build 8057)
```cpp
bool deviceSecretSet = false;  // Whether device secret is configured
```

**Potential additions** (if needed):
```cpp
bool wifiPasswordSet = false;  // Whether WiFi password is configured
bool meshPasswordSet = false;  // Whether mesh password is configured
bool mqttBrokerSet = false;    // Whether MQTT broker is configured
bool otaServerSet = false;     // Whether OTA server URL is configured
```

### Existing `*Enabled` Fields
```cpp
bool displayEnabled = false;           // Display feature enabled
bool deepSleepEnabled = false;         // Deep sleep feature enabled
bool mqttHourlyRetryEnabled = false;   // Hourly retry feature enabled
```

**Potential additions** (if needed):
```cpp
bool meshEnabled = false;       // Is WiFi mesh feature enabled?
bool otaEnabled = false;        // Are OTA updates enabled?
bool encryptionEnabled = false; // Is encryption enabled?
bool encodingEnabled = false;   // Is data encoding enabled?
```

### Potential Runtime State Fields
Currently, StatusPackage does not have explicit runtime state fields. If needed in the future:

```cpp
bool isConfigured = false;      // Device has complete valid configuration
bool mqttConnected = false;     // Currently connected to MQTT broker
bool meshIsRoot = false;        // Currently acting as mesh root
bool wifiConnected = false;     // Currently connected to WiFi
```

## Best Practices

### DO ✅

1. **Use `*Set` for configuration presence**
   ```cpp
   bool deviceSecretSet = false;  // ✅ Indicates if secret is configured
   ```

2. **Use `*Enabled` for feature toggles**
   ```cpp
   bool displayEnabled = false;   // ✅ Indicates if feature is on/off
   ```

3. **Use `is*` or `*Connected` for runtime state**
   ```cpp
   bool isConfigured = false;     // ✅ Indicates current state
   bool mqttConnected = false;    // ✅ Indicates connection status
   ```

4. **Document field purpose clearly**
   ```cpp
   bool otaServerSet = false;     // Has OTA server URL been configured? (Build XXXX)
   ```

### DON'T ❌

1. **Don't mix patterns without clear semantics**
   ```cpp
   bool wifiSet = false;          // ❌ Ambiguous - set to what? On/off or configured?
   ```

2. **Don't use generic boolean names**
   ```cpp
   bool wifi = false;             // ❌ Unclear meaning
   bool display = false;          // ❌ What about display?
   ```

3. **Don't use `*Set` for feature toggles**
   ```cpp
   bool displaySet = false;       // ❌ Confusing - use displayEnabled instead
   ```

4. **Don't use `*Enabled` for configuration presence**
   ```cpp
   bool deviceSecretEnabled = false;  // ❌ Confusing - use deviceSecretSet instead
   ```

## Validation

When adding new boolean fields, ask these questions:

1. **Does this field indicate configuration presence?**
   - YES → Use `*Set` suffix
   - Example: `mqttBrokerSet`, `deviceSecretSet`

2. **Does this field toggle a feature on/off?**
   - YES → Use `*Enabled` suffix
   - Example: `displayEnabled`, `otaEnabled`

3. **Does this field reflect current runtime state?**
   - YES → Use `is*` prefix or `*Connected` suffix
   - Example: `isConfigured`, `mqttConnected`

4. **Does this field fit multiple categories?**
   - Consider creating separate fields for each semantic meaning
   - Example: `otaServerSet` AND `otaEnabled` AND `otaInProgress`

## Benefits

1. **Self-Documenting Code**: Field names clearly indicate semantic meaning
2. **Reduced Confusion**: Developers immediately understand what each boolean represents
3. **Better API Design**: Consistent patterns across entire codebase
4. **Easier Onboarding**: New developers can infer meaning from field names
5. **Fewer Bugs**: Clear semantics reduce misunderstandings and implementation errors

## References

- StatusPackage implementation: `examples/alteriom/alteriom_sensor_package.hpp`
- Test validation: `test/catch/catch_alteriom_packages.cpp`
- Time field conventions: See header documentation in `alteriom_sensor_package.hpp`

---

**Document Version**: 1.0  
**Last Updated**: 2025-11-04  
**Status**: Active  
**Applies To**: StatusPackage, EnhancedStatusPackage, and all future Alteriom packages
