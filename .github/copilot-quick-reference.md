# Copilot Quick Reference for painlessMesh

## Instant Code Snippets

### New Alteriom Package (Copy-Paste Ready)
```cpp
namespace alteriom {
class YourPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    uint32_t yourField = 0;
    TSTRING yourText = "";
    
    YourPackage() : BroadcastPackage(203) {} // Use next available ID
    
    YourPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
        yourField = jsonObj["field"];
        yourText = jsonObj["text"].as<TSTRING>();
    }
    
    JsonObject addTo(JsonObject&& jsonObj) const {
        jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
        jsonObj["field"] = yourField;
        jsonObj["text"] = yourText;
        return jsonObj;
    }
    
#if ARDUINOJSON_VERSION_MAJOR < 7
    size_t jsonObjectSize() const { 
        return JSON_OBJECT_SIZE(noJsonFields + 2) + yourText.length(); 
    }
#endif
};
}
```

### Arduino Setup (Essential Boilerplate)
```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_password"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;
using namespace alteriom;

void setup() {
    Serial.begin(115200);
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
}

void loop() {
    mesh.update();
}

void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    JsonObject obj = doc.as<JsonObject>();
    
    switch((uint8_t)obj["type"]) {
        case 200: /* SensorPackage */ break;
        case 201: /* CommandPackage */ break;
        case 202: /* StatusPackage */ break;
    }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
}

void changedConnectionCallback() {
    Serial.printf("Connections changed. Nodes: %d\n", mesh.getNodeList().size());
}
```

### Test Template (Instant Test Creation)
```cpp
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;
using namespace painlessmesh;
logger::LogClass Log;

SCENARIO("YourPackage serialization works") {
    GIVEN("A YourPackage with test data") {
        auto pkg = YourPackage();
        pkg.from = 12345;
        pkg.yourField = 42;
        pkg.yourText = "test";
        
        REQUIRE(pkg.type == 203); // Your type ID
        
        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<YourPackage>();
            
            THEN("Values should be preserved") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.yourField == pkg.yourField);
                REQUIRE(pkg2.yourText == pkg.yourText);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}
```

## Quick Commands

### Build & Test
```bash
cmake -G Ninja . && ninja
./bin/catch_alteriom_packages
run-parts --regex catch_ bin/
```

### Common File Locations
- New packages: `examples/alteriom/alteriom_sensor_package.hpp`
- Arduino examples: `examples/alteriom/*.ino`
- Tests: `test/catch/catch_*.cpp`
- Core library: `src/painlessmesh/` (read-only)

## Type ID Registry
- 200: SensorPackage (temperature, humidity, pressure)
- 201: CommandPackage (device commands)
- 202: StatusPackage (health monitoring)
- 203+: Available for new Alteriom packages

## Memory Guidelines
- ESP32: ~320KB RAM - Use doubles, longer strings OK
- ESP8266: ~80KB RAM - Use floats, keep strings short
- Always check `mesh.sendBroadcast()` return value
- Prefer uint16_t/uint8_t over uint32_t when possible

## Common Patterns

### Periodic Tasks
```cpp
Task taskSendData(30000, TASK_FOREVER, &sendDataFunction);
userScheduler.addTask(taskSendData);
taskSendData.enable();
```

### Safe JSON Parsing
```cpp
DynamicJsonDocument doc(1024);
DeserializationError error = deserializeJson(doc, msg);
if (error) {
    Serial.printf("JSON error: %s\n", error.c_str());
    return;
}
```

### Logging Levels
```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);
```