---
applies_to:
  - "examples/alteriom/**/*.hpp"
  - "examples/alteriom/**/*.ino"
---

# Alteriom Package Development Instructions

When working with Alteriom-specific packages and examples, follow these conventions:

## Package Development

### Package Type Selection
Choose the appropriate base class:
- **SinglePackage**: Point-to-point communication to specific node
- **BroadcastPackage**: Send to all nodes in the mesh

### Type ID Assignment
- 200: SensorPackage (temperature, humidity, pressure)
- 201: CommandPackage (device commands)
- 202: StatusPackage (basic health monitoring)
- 203-399: Available for new Alteriom packages
- 400+: Extended packages (MetricsPackage, HealthCheckPackage, etc.)

Always use the next available ID and document it!

### Package Template

```cpp
namespace alteriom {

class YourPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    // Data fields - use appropriate types
    uint32_t yourId = 0;
    double yourValue = 0.0;
    TSTRING yourText = "";
    uint8_t yourFlag = 0;
    
    // Default constructor with type ID
    YourPackage() : BroadcastPackage(203) {} // Next available ID
    
    // Constructor from JSON
    YourPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
        yourId = jsonObj["id"];
        yourValue = jsonObj["value"];
        yourText = jsonObj["text"].as<TSTRING>();
        yourFlag = jsonObj["flag"];
    }
    
    // Serialization to JSON
    JsonObject addTo(JsonObject&& jsonObj) const {
        jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
        jsonObj["id"] = yourId;
        jsonObj["value"] = yourValue;
        jsonObj["text"] = yourText;
        jsonObj["flag"] = yourFlag;
        return jsonObj;
    }
    
    // Size calculation for pre-ArduinoJson 7
#if ARDUINOJSON_VERSION_MAJOR < 7
    size_t jsonObjectSize() const { 
        return JSON_OBJECT_SIZE(noJsonFields + 4) + yourText.length();
    }
#endif
};

} // namespace alteriom
```

## Field Type Guidelines

### Memory-Efficient Types
```cpp
// ESP8266 with ~80KB RAM - prefer smaller types
uint8_t status;           // 0-255
uint16_t count;           // 0-65535
float temperature;        // 4 bytes
uint32_t timestamp;       // 4 bytes

// ESP32 with ~320KB RAM - can use larger types
double preciseMeasurement; // 8 bytes
uint32_t largeCounter;     // 4 bytes
```

### String Usage
```cpp
// Always use TSTRING for cross-platform compatibility
TSTRING deviceName = "";
TSTRING firmwareVersion = "";
TSTRING parameters = "";

// Keep strings short on ESP8266
// Prefer fixed-length fields or enums when possible
```

## Arduino Example Pattern

```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_password"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;
using namespace alteriom;

// Task for periodic operations
Task taskSendData(30000, TASK_FOREVER, [](){
    // Create and send your package
    auto pkg = YourPackage();
    pkg.from = mesh.getNodeId();
    pkg.yourId = getDeviceId();
    pkg.yourValue = readSensor();
    
    // Serialize and send
    String msg = pkg.toJson();
    mesh.sendBroadcast(msg);
});

void setup() {
    Serial.begin(115200);
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    
    userScheduler.addTask(taskSendData);
    taskSendData.enable();
}

void loop() {
    mesh.update();
}

void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) {
        Serial.printf("JSON parse failed: %s\n", error.c_str());
        return;
    }
    
    JsonObject obj = doc.as<JsonObject>();
    uint8_t msgType = obj["type"];
    
    switch(msgType) {
        case 200: handleSensor(SensorPackage(obj)); break;
        case 201: handleCommand(CommandPackage(obj)); break;
        case 202: handleStatus(StatusPackage(obj)); break;
        // Add your package types here
    }
}
```

## Best Practices

### 1. Error Handling
```cpp
// Always check message sending
if (!mesh.sendBroadcast(msg)) {
    Serial.println("Failed to send - network issue or memory");
}

// Validate JSON parsing
if (error) {
    Serial.printf("Parse error: %s\n", error.c_str());
    return;
}

// Check required fields
if (!obj.containsKey("type") || !obj.containsKey("from")) {
    Serial.println("Missing required fields");
    return;
}
```

### 2. Memory Management
```cpp
// Use appropriate buffer sizes
DynamicJsonDocument doc(256);  // Small packages
DynamicJsonDocument doc(512);  // Medium packages
DynamicJsonDocument doc(1024); // Large packages only

// Monitor free memory on ESP8266
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
```

### 3. Rate Limiting
```cpp
// Prevent network congestion
static unsigned long lastBroadcast = 0;
const unsigned long MIN_INTERVAL = 1000; // 1 second

if (millis() - lastBroadcast > MIN_INTERVAL) {
    mesh.sendBroadcast(msg);
    lastBroadcast = millis();
}
```

### 4. Logging
```cpp
// Structured logging for debugging
Serial.printf("[PKG:%u] Sensor[%u]: T=%.1f H=%.1f\n", 
    msgType, sensorId, temperature, humidity);

// Use mesh debug levels
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | SYNC);
```

## Package Documentation

Always document your packages:

```cpp
/**
 * @brief Your package description
 * 
 * Detailed explanation of what this package is for,
 * when to use it, and any special considerations.
 * 
 * Type ID: 203
 * Base class: BroadcastPackage
 * 
 * Fields:
 * - yourId: Description of this field
 * - yourValue: Description and units
 * - yourText: Description and expected format
 * - yourFlag: Bit flags or enum values
 * 
 * Example usage:
 * @code
 * auto pkg = YourPackage();
 * pkg.yourId = 123;
 * pkg.yourValue = 45.6;
 * mesh.sendBroadcast(pkg.toJson());
 * @endcode
 */
```

## Testing Your Package

1. Create test in `test/catch/catch_alteriom_packages.cpp`
2. Test serialization round-trip
3. Verify all fields preserve values
4. Test edge cases (empty strings, zeros, max values)
5. Build and run: `ninja && ./bin/catch_alteriom_packages`

## Common Mistakes to Avoid

❌ Using `String` instead of `TSTRING`  
❌ Forgetting to add field to `addTo()` method  
❌ Not calculating correct `jsonObjectSize()`  
❌ Reusing type IDs from other packages  
❌ Missing the `#if ARDUINOJSON_VERSION_MAJOR < 7` guard  
❌ Not adding `noJsonFields` to size calculation  
❌ Forgetting `using namespace alteriom;` in examples  
❌ Not validating JSON parsing errors  

## File Locations

- **Package definitions**: `examples/alteriom/alteriom_sensor_package.hpp`
- **Arduino examples**: `examples/alteriom/*.ino`
- **Package tests**: `test/catch/catch_alteriom_packages.cpp`
- **Documentation**: Update README with new package info
