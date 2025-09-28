# Copilot Troubleshooting Guide for painlessMesh

## Build Issues

### "TaskScheduler.h: No such file"
```bash
cd test
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arkhipenko/TaskScheduler
cd ..
cmake -G Ninja .
```

### "Boost not found"
```bash
sudo apt-get install libboost-dev libboost-system-dev
```

### Ninja build fails
```bash
rm -rf CMakeCache.txt CMakeFiles build.ninja
cmake -G Ninja .
ninja
```

## Runtime Issues

### Mesh connection problems
```cpp
// Check network settings
mesh.setDebugMsgTypes(ERROR | CONNECTION | STARTUP);

// Verify same prefix/password across nodes
#define MESH_PREFIX     "AlteriomMesh"  // Must be identical
#define MESH_PASSWORD   "your_password" // Must be identical
#define MESH_PORT       5555            // Must be identical
```

### Memory allocation failures
```cpp
// ESP8266: Reduce JSON buffer size
DynamicJsonDocument doc(512); // Instead of 1024

// Check available memory
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

// Use static buffers for frequent operations
StaticJsonDocument<200> doc; // For small packages
```

### JSON parsing errors
```cpp
// Always check deserialization
DeserializationError error = deserializeJson(doc, msg);
if (error) {
    Serial.printf("JSON parse failed: %s\n", error.c_str());
    Serial.printf("Message was: %s\n", msg.c_str());
    return;
}

// Validate required fields exist
if (!obj.containsKey("type") || !obj.containsKey("from")) {
    Serial.println("Missing required fields");
    return;
}
```

## Package Development Issues

### "String does not name a type"
```cpp
// Use TSTRING instead of String
TSTRING parameters = "";      // Correct
String parameters = "";       // Wrong in test environment
```

### JSON serialization fails
```cpp
// Ensure all fields are properly serialized
JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["field1"] = field1;  // Don't forget any fields
    jsonObj["field2"] = field2;
    return jsonObj;
}
```

### Test compilation errors
```cpp
// Include correct headers in tests
#include "catch2/catch.hpp"
#include "Arduino.h"                    // Must be first
#include "catch_utils.hpp"
#include "examples/alteriom/alteriom_sensor_package.hpp"

// Declare Log in tests
painlessmesh::logger::LogClass Log;
```

## Common Error Messages & Solutions

### "handlePackage not found"
PackageHandler doesn't have handlePackage method, use callbacks:
```cpp
handler.onPackage(200, [](protocol::Variant& variant) {
    auto pkg = variant.to<SensorPackage>();
    // Process package
    return true;
});
```

### "no matching function for call to as<String>()"
Use TSTRING instead:
```cpp
parameters = jsonObj["params"].as<TSTRING>();  // Correct
parameters = jsonObj["params"].as<String>();   // Wrong
```

### "jsonObjectSize undefined"
Add the macro guard:
```cpp
#if ARDUINOJSON_VERSION_MAJOR < 7
size_t jsonObjectSize() const { 
    return JSON_OBJECT_SIZE(noJsonFields + FIELD_COUNT); 
}
#endif
```

### Mesh stops working after time
```cpp
// Add connection monitoring
void droppedConnectionCallback(uint32_t nodeId) {
    Serial.printf("Lost connection to %u\n", nodeId);
    // Implement reconnection logic if needed
}

mesh.onDroppedConnection(&droppedConnectionCallback);
```

## Performance Issues

### High memory usage
```cpp
// Use appropriate buffer sizes
DynamicJsonDocument doc(256);  // Small packages
DynamicJsonDocument doc(512);  // Medium packages  
DynamicJsonDocument doc(1024); // Large packages only

// Reuse documents where possible
static DynamicJsonDocument reusableDoc(512);
```

### Network congestion
```cpp
// Add delays between broadcasts
static unsigned long lastBroadcast = 0;
if (millis() - lastBroadcast > 1000) { // Max 1 msg/sec
    mesh.sendBroadcast(msg);
    lastBroadcast = millis();
}

// Check send success
if (!mesh.sendBroadcast(msg)) {
    Serial.println("Broadcast failed - network congested?");
}
```

### Slow mesh formation
```cpp
// Increase stability check interval
mesh.setStationTimeoutMs(30000);  // 30 seconds instead of default

// Monitor mesh stability
void changedConnectionCallback() {
    Serial.printf("Network size: %d nodes\n", mesh.getNodeList().size());
    if (mesh.getNodeList().size() < EXPECTED_NODES) {
        Serial.println("Waiting for more nodes...");
    }
}
```

## Debug Output Interpretation

### Connection Messages
```
New AP connection incoming  - Node connecting as AP client
New STA connection incoming - Node connecting as STA client
Closed connection - Normal disconnection
tcp_err(): error trying to connect - Connection failed
```

### Mesh Formation
```
logLevel set to 6 - Debug logging enabled
Starting mesh... - Initialization
AP started - Access point mode active
STA connected - Station mode connected
```

## Quick Fixes Checklist

1. **Build fails**: Check dependencies (Boost, ArduinoJson, TaskScheduler)
2. **Tests fail**: Verify includes and TSTRING usage
3. **No connections**: Check MESH_PREFIX/PASSWORD/PORT consistency
4. **Memory errors**: Reduce buffer sizes, check ESP.getFreeHeap()
5. **JSON errors**: Validate message format and field names
6. **Slow performance**: Reduce broadcast frequency, check network size