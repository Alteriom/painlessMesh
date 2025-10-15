# Using AlteriomPainlessMesh in Your PlatformIO Project

## Quick Start

### Step 1: Add to platformio.ini

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    https://github.com/Alteriom/painlessMesh#copilot/start-phase-2-implementation
```

Or for ESP8266:

```ini
[env:esp8266]
platform = espressif8266
board = nodemcuv2
framework = arduino
lib_deps = 
    https://github.com/Alteriom/painlessMesh#copilot/start-phase-2-implementation
```

### Step 2: Include in your code

```cpp
#include <painlessMesh.h>

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init("MESH_SSID", "MESH_PASSWORD", &userScheduler, 5555);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
}

void loop() {
  mesh.update();
}

void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.println("Connections changed");
}
```

### Step 3: Build

```bash
pio run
```

## Troubleshooting

### Error: "cannot resolve directory"

**Solution:** This is now FIXED! The library has:
- ✅ Explicit `srcDir: "src"` in library.json
- ✅ All source files in src/ directory
- ✅ No duplicate library.json files
- ✅ Proper header references

### Error: "UnboundLocalError: dir"

**Solution:** This was caused by npm build scripts. Now FIXED:
- ✅ Build scripts renamed to `dev:build` (won't auto-run)
- ✅ npm link works without triggering cmake

### Error: "header not found"

**Solution:** Use the correct include:

```cpp
// ✅ Correct
#include <painlessMesh.h>

// ❌ Wrong
#include <AlteriomPainlessMesh.h>  // Alternative header, use painlessMesh.h instead
```

## Library Structure (Verified ✓)

```
painlessMesh/
├── library.json              ✓ Root metadata with srcDir
├── library.properties        ✓ Arduino IDE compatibility
├── src/                      ✓ All source files here
│   ├── painlessMesh.h        ✓ Main header
│   ├── painlessMeshSTA.cpp   ✓ Implementation
│   ├── scheduler.cpp
│   ├── wifi.cpp
│   └── painlessmesh/         ✓ Library modules
└── examples/                 ✓ 20 example sketches
```

## Validation Status

Run validation script to verify:

```bash
python scripts/validate_library_structure.py
```

**Current Status:** ✅ 8/8 checks passed (100%)

## Alteriom Extensions

This fork includes additional features:

### SensorPackage (Type 200)
```cpp
#include "examples/alteriom/alteriom_sensor_package.hpp"

alteriom::SensorPackage sensor;
sensor.temperature = 22.5;
sensor.humidity = 55.0;
sensor.pressure = 1013.25;
mesh.sendBroadcast(sensor.to_string());
```

### CommandPackage (Type 201)
```cpp
alteriom::CommandPackage cmd;
cmd.command = 1;  // LED_CONTROL
cmd.targetDevice = targetNodeId;
cmd.parameters = "{\"state\":true}";
mesh.sendSingle(targetNodeId, cmd.to_string());
```

### StatusPackage (Type 202)
```cpp
alteriom::StatusPackage status;
status.deviceStatus = 1;  // Online
status.uptime = millis() / 1000;
status.freeMemory = ESP.getFreeHeap() / 1024;
mesh.sendBroadcast(status.to_string());
```

## MQTT Integration

For MQTT gateway functionality, see:
- `examples/mqttBridge/mqttBridge.ino`
- `docs/MESH_TOPOLOGY_GUIDE.md` - Visualization examples
- `docs/MQTT_BRIDGE_COMMANDS.md` - Command reference

## Schema Compliance

This library implements **@alteriom/mqtt-schema v0.5.0**:
- ✅ Device ID format: `ALT-XXXXXXXXXXXX`
- ✅ Envelope fields (schema_version, device_id, timestamp, etc.)
- ✅ Mesh topology reporting
- ✅ Command/response tracking with correlation IDs

## Support

- **Issues:** https://github.com/Alteriom/painlessMesh/issues
- **Pull Request:** #17 (Phase 2 Implementation)
- **Documentation:** `docs/` directory
- **Examples:** `examples/` directory (20 sketches)

---

**Last Validated:** October 15, 2025  
**Branch:** copilot/start-phase-2-implementation  
**Status:** ✅ Ready for PlatformIO use
