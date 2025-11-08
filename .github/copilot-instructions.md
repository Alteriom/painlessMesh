# GitHub Copilot Instructions for painlessMesh

This repository is a fork of the painlessMesh library specifically tailored for Alteriom's needs. painlessMesh is a user-friendly library for creating mesh networks with ESP8266 and ESP32 devices.

## Project Overview

painlessMesh handles routing and network management automatically, allowing developers to focus on their applications. The library uses JSON-based messaging and syncs time across all nodes, making it ideal for coordinated behaviors like synchronized light displays or sensor networks reporting to a central node.

## Code Generation Guidelines

When generating code for this repository, follow these specific patterns:

### Include Statements
- Always include `"painlessMesh.h"` for Arduino projects
- Use `#include "painlessmesh/plugin.hpp"` for custom packages
- Include `"examples/alteriom/alteriom_sensor_package.hpp"` for Alteriom packages
- Add proper header guards: `#ifndef FILENAME_HPP` / `#define FILENAME_HPP` / `#endif`

### Coding Conventions
- Use `TSTRING` instead of `String` for cross-platform compatibility
- Prefix Alteriom-specific classes with `alteriom::` namespace
- Use type IDs 200+ for Alteriom custom packages (200=Sensor, 201=Command, 202=Status)
- Follow existing indentation (2 spaces) and brace placement patterns

### Package Development Templates
When creating Alteriom packages, use these exact patterns:

```cpp
// Single-destination package template
class MyPackage : public painlessmesh::plugin::SinglePackage {
public:
    // Data fields
    uint32_t fieldName = 0;
    TSTRING textField = "";
    
    MyPackage() : SinglePackage(TYPE_ID) {} // Use unique ID 203+
    
    MyPackage(JsonObject jsonObj) : SinglePackage(jsonObj) {
        fieldName = jsonObj["field"];
        textField = jsonObj["text"].as<TSTRING>();
    }
    
    JsonObject addTo(JsonObject&& jsonObj) const {
        jsonObj = SinglePackage::addTo(std::move(jsonObj));
        jsonObj["field"] = fieldName;
        jsonObj["text"] = textField;
        return jsonObj;
    }
    
#if ARDUINOJSON_VERSION_MAJOR < 7
    size_t jsonObjectSize() const { 
        return JSON_OBJECT_SIZE(noJsonFields + FIELD_COUNT) + textField.length(); 
    }
#endif
};

// Broadcast package template  
class MyBroadcastPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    // Same structure but inherit from BroadcastPackage
    MyBroadcastPackage() : BroadcastPackage(TYPE_ID) {}
    // ... rest identical to SinglePackage
};
```

### Arduino Mesh Setup Pattern
```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_password"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
    Serial.begin(115200);
    mesh.setDebugMsgTypes(ERROR | STARTUP);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
}

void loop() {
    mesh.update();
}
```

### Message Handling Pattern
```cpp
void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    JsonObject obj = doc.as<JsonObject>();
    uint8_t msgType = obj["type"];
    
    switch(msgType) {
        case 200: // SensorPackage
            handleSensorData(alteriom::SensorPackage(obj));
            break;
        case 201: // CommandPackage  
            handleCommand(alteriom::CommandPackage(obj));
            break;
        case 202: // StatusPackage
            handleStatus(alteriom::StatusPackage(obj));
            break;
    }
}
```

### Test Generation Pattern
When creating tests for Alteriom packages, follow this template:

```cpp
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;
using namespace painlessmesh;

SCENARIO("MyPackage serialization works correctly") {
    GIVEN("A MyPackage with test data") {
        auto pkg = MyPackage();
        pkg.from = 12345;
        pkg.fieldName = 42;
        pkg.textField = "test_value";
        
        REQUIRE(pkg.type == EXPECTED_TYPE_ID);
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<MyPackage>();
            
            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.fieldName == pkg.fieldName);
                REQUIRE(pkg2.textField == pkg.textField);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}
```

## Key Technologies & Build System

- **C++14** standard with Arduino framework support
- **ESP8266/ESP32** platforms (espressif8266, espressif32)  
- **Boost.Asio** for networking (PC testing)
- **ArduinoJson** for message serialization
- **TaskScheduler** for task management
- **CMake** build system with Ninja generator
- **Catch2** testing framework

### Build Commands
Always use these exact commands for building:
```bash
# Setup dependencies (if needed)
cd test
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arkhipenko/TaskScheduler
cd ..

# Configure and build
cmake -G Ninja .
ninja

# Run tests
./bin/catch_alteriom_packages
run-parts --regex catch_ bin/
```

## Specific Alteriom Implementation Guidance

### Available Alteriom Packages
Use these existing packages for common scenarios:

1. **alteriom::SensorPackage** (Type 200) - Environmental data
   - `temperature`, `humidity`, `pressure` (double)
   - `sensorId`, `timestamp` (uint32_t) 
   - `batteryLevel` (uint8_t)

2. **alteriom::CommandPackage** (Type 201) - Device control
   - `command` (uint8_t), `targetDevice`, `commandId` (uint32_t)
   - `parameters` (TSTRING for JSON)

3. **alteriom::StatusPackage** (Type 202) - Health monitoring  
   - `deviceStatus` (uint8_t), `uptime` (uint32_t)
   - `freeMemory`, `wifiStrength` (uint16_t/uint8_t)
   - `firmwareVersion` (TSTRING)

### Memory Constraints
- ESP32: ~320KB RAM - Use efficient data types, avoid large strings
- ESP8266: ~80KB RAM - Critical memory management, prefer primitives
- MAX_CONN typically 4-10 depending on platform

### Network Patterns
- Use `SinglePackage` for point-to-point commands
- Use `BroadcastPackage` for sensor data and status updates
- Implement exponential backoff for critical messages
- Consider network congestion in high-frequency scenarios

## Error Handling & Debugging

### Common Issues & Solutions
```cpp
// Memory allocation failure
if (!mesh.sendBroadcast(msg)) {
    Log(ERROR, "Failed to send message - check memory/connections\n");
}

// JSON parsing errors
DynamicJsonDocument doc(1024);
DeserializationError error = deserializeJson(doc, msg);
if (error) {
    Log(ERROR, "JSON parse failed: %s\n", error.c_str());
    return;
}

// Network connectivity issues
void newConnectionCallback(uint32_t nodeId) {
    Log(CONNECTION, "New node: %u, total: %d\n", nodeId, mesh.getNodeList().size());
}

void droppedConnectionCallback(uint32_t nodeId) {
    Log(CONNECTION, "Lost node: %u\n", nodeId);
    // Implement reconnection logic if needed
}
```

### Logging Best Practices
```cpp
// Set appropriate log levels for different scenarios
#ifdef DEBUG_BUILD
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | SYNC | COMMUNICATION);
#else
    mesh.setDebugMsgTypes(ERROR | STARTUP);
#endif

// Use structured logging for Alteriom packages
Log(GENERAL, "Sensor[%u]: T=%.1f H=%.1f P=%.1f\n", 
    sensorId, temperature, humidity, pressure);
```

## File Organization Rules

### Directory Structure
```
examples/alteriom/          # Alteriom-specific packages and examples
├── alteriom_sensor_package.hpp    # Package definitions
├── alteriom_sensor_node.ino       # Arduino example
└── README.md                      # Usage documentation

test/catch/                 # Unit tests
├── catch_alteriom_packages.cpp    # Alteriom package tests
└── [other test files]

src/painlessmesh/           # Core library (don't modify without consultation)
├── mesh.hpp, tcp.hpp, protocol.hpp, plugin.hpp
└── [other core files]
```

### When Adding New Files
1. **Arduino sketches**: Place in `examples/alteriom/`
2. **Custom packages**: Add to `examples/alteriom/alteriom_sensor_package.hpp` or create new `.hpp`
3. **Tests**: Create `test/catch/catch_[your_feature].cpp`
4. **Documentation**: Update relevant README files

### CMakeLists.txt Updates
When adding new test files, they're automatically included by the glob pattern:
```cmake
FILE(GLOB TESTFILES test/**/catch_*.cpp)
# Your catch_*.cpp files will be auto-discovered
```

## Linting and Code Quality

### Code Formatting
The repository uses `.clang-format` for consistent code style:
```bash
# Format a single file
clang-format -i src/your_file.cpp

# Format all C++ files (be careful with this)
find src examples -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### Code Quality Checks
- Follow C++14 standard
- Avoid memory leaks (especially important for ESP8266)
- Use RAII patterns for resource management
- Prefer const correctness
- Check for potential buffer overflows

## Security Considerations

### Important Security Rules
1. **Never commit secrets**: No passwords, API keys, or credentials in code
2. **Validate all inputs**: Especially data received from mesh network
3. **Buffer sizes**: Always check buffer bounds, especially with strings
4. **Memory safety**: Be careful with pointers and dynamic allocation
5. **Network security**: Mesh passwords should be strong in production

### Security Scanning
The repository uses GitHub's CodeQL for security analysis:
- Runs automatically on pull requests
- Checks for common vulnerabilities
- Review and address any security alerts

### Secure Coding Patterns
```cpp
// Good: Check buffer sizes
if (msg.length() < MAX_MESSAGE_SIZE) {
    processMessage(msg);
}

// Good: Validate JSON parsing
DeserializationError error = deserializeJson(doc, msg);
if (error) {
    Log(ERROR, "Invalid JSON: %s\n", error.c_str());
    return;
}

// Good: Check memory availability
if (ESP.getFreeHeap() < MIN_FREE_HEAP) {
    Log(ERROR, "Low memory, skipping operation\n");
    return;
}

// Bad: Trusting unvalidated input
// processCommand(obj["cmd"]); // No validation!
```

## Contributing Workflow

### Making Changes
1. Create a feature branch from `develop`
2. Make your changes following the conventions in this guide
3. Build and test locally: `cmake -G Ninja . && ninja && run-parts --regex catch_ bin/`
4. Run security scans if modifying core code
5. Create a pull request to `develop` (not `main`)
6. Address review comments and ensure CI passes
7. Once approved, reviewer will merge the PR

### Pull Request Checklist
- [ ] Code follows style guidelines (check with clang-format)
- [ ] All tests pass locally
- [ ] New tests added for new functionality
- [ ] Documentation updated if needed
- [ ] No security vulnerabilities introduced
- [ ] Memory usage considered (especially for ESP8266)
- [ ] Backward compatibility maintained

### Git Flow
This project follows [git flow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow):
- `main` - Stable releases only
- `develop` - Integration branch for features
- `feature/*` - Feature branches (merge to `develop`)
- `hotfix/*` - Urgent fixes (merge to both `main` and `develop`)

## Additional Resources

### File-Specific Instructions
Check `.github/instructions/` for detailed guidance on:
- Testing (`testing.instructions.md`)
- Alteriom packages (`alteriom-packages.instructions.md`)
- Workflows (`workflows.instructions.md`)

### Documentation
- Repository root: `.instructions.md` - Quick start guide
- Quick reference: `.github/copilot-quick-reference.md`
- Troubleshooting: `.github/copilot-troubleshooting.md`
- Development guide: `.github/README-DEVELOPMENT.md`
- Online docs: https://alteriom.github.io/painlessMesh/