# painlessMesh Development Guide for Alteriom

This guide provides comprehensive information for developing with the painlessMesh library in the Alteriom context.

## Quick Start

### Environment Setup
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y cmake ninja-build libboost-dev libboost-system-dev

# Clone and setup the repository
git clone https://github.com/Alteriom/painlessMesh.git
cd painlessMesh

# Setup submodules (if they exist, otherwise manually clone dependencies)
git submodule init && git submodule update || {
    cd test
    git clone https://github.com/bblanchon/ArduinoJson.git
    git clone https://github.com/arkhipenko/TaskScheduler
    cd ..
}

# Build the project
cmake -G Ninja .
ninja

# Run tests
./bin/catch_base64
./bin/catch_plugin
./bin/catch_protocol
```

## Project Structure for Alteriom

```
painlessMesh/
├── src/                          # Core library source
│   ├── painlessmesh/            # Main mesh implementation
│   │   ├── mesh.hpp             # Core mesh functionality
│   │   ├── tcp.hpp              # TCP connection handling
│   │   ├── protocol.hpp         # Message protocols
│   │   ├── plugin.hpp           # Plugin system
│   │   └── router.hpp           # Message routing
│   ├── plugin/                  # Plugin implementations
│   │   ├── performance.hpp      # Performance monitoring
│   │   └── remote.hpp           # Remote management
│   └── scheduler.cpp            # Task scheduling
├── test/                        # Test infrastructure
│   ├── catch/                   # Unit tests (Catch2)
│   ├── boost/                   # Integration tests (Boost.Asio)
│   ├── ArduinoJson/            # JSON library (submodule)
│   └── TaskScheduler/          # Task scheduler (submodule)
├── examples/                    # Example implementations
└── .github/                     # GitHub configuration
    ├── copilot-instructions.md  # Copilot guidance
    └── README-DEVELOPMENT.md    # This file
```

## Building Custom Packages for Alteriom

### 1. Define Custom Message Types

Create custom package classes for Alteriom-specific communications:

```cpp
// Example: Sensor data package
class AlteriomSensorPackage : public plugin::BroadcastPackage {
public:
    double temperature = 0.0;
    double humidity = 0.0;
    uint32_t sensorId = 0;
    
    AlteriomSensorPackage() : BroadcastPackage(100) {} // Custom type ID
    
    AlteriomSensorPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
        temperature = jsonObj["temp"];
        humidity = jsonObj["hum"];
        sensorId = jsonObj["id"];
    }
    
    JsonObject addTo(JsonObject&& jsonObj) const {
        jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
        jsonObj["temp"] = temperature;
        jsonObj["hum"] = humidity;
        jsonObj["id"] = sensorId;
        return jsonObj;
    }
};
```

### 2. Implement Custom Plugins

```cpp
// Example: Alteriom data collection plugin
class AlteriomDataCollector : public plugin::PackageHandler<Connection> {
public:
    void init(Scheduler& scheduler) {
        // Setup periodic data collection
        addTask(scheduler, 0, 30000, [this]() { // Every 30 seconds
            collectAndBroadcastData();
        });
        
        // Handle incoming sensor data
        onPackage(100, [this](protocol::Variant& variant) {
            auto pkg = variant.to<AlteriomSensorPackage>();
            processIncomingSensorData(pkg);
            return true;
        });
    }
    
private:
    void collectAndBroadcastData() {
        AlteriomSensorPackage pkg;
        pkg.from = mesh->getNodeId();
        pkg.temperature = readTemperature();
        pkg.humidity = readHumidity();
        pkg.sensorId = getDeviceId();
        
        sendPackage(&pkg);
    }
    
    void processIncomingSensorData(const AlteriomSensorPackage& pkg) {
        // Store or forward sensor data
        logSensorData(pkg);
    }
};
```

### 3. Integration with Main Mesh

```cpp
#include "painlessMesh.h"
#include "alteriom_plugins.h"

painlessMesh mesh;
AlteriomDataCollector dataCollector;

void setup() {
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &scheduler, MESH_PORT);
    mesh.onReceive([](uint32_t from, String& msg) {
        // Handle standard mesh messages
    });
    
    // Initialize Alteriom-specific functionality
    dataCollector.init(scheduler);
}

void loop() {
    mesh.update();
}
```

## Testing Strategy for Alteriom

### Unit Testing
Follow the pattern in `test/catch/catch_plugin.cpp`:

```cpp
SCENARIO("Alteriom sensor package serialization") {
    GIVEN("A sensor package with data") {
        auto pkg = AlteriomSensorPackage();
        pkg.temperature = 23.5;
        pkg.humidity = 45.0;
        pkg.sensorId = 12345;
        
        WHEN("Converting to JSON and back") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<AlteriomSensorPackage>();
            
            THEN("Data should be preserved") {
                REQUIRE(pkg2.temperature == pkg.temperature);
                REQUIRE(pkg2.humidity == pkg.humidity);
                REQUIRE(pkg2.sensorId == pkg.sensorId);
            }
        }
    }
}
```

### Integration Testing
Use patterns from `test/boost/tcp_integration.cpp` for network testing:

```cpp
SCENARIO("Alteriom mesh data collection") {
    // Setup multiple mesh nodes
    // Test data propagation
    // Verify coordination behavior
}
```

## Performance Considerations

### Memory Management
- ESP32: ~320KB RAM, ESP8266: ~80KB RAM
- Use appropriate data types (avoid unnecessary strings)
- Monitor mesh size (MAX_CONN limits)
- Consider message frequency and size

### Network Optimization
- Use Single packages for point-to-point communication
- Use Broadcast packages for coordination messages
- Implement proper back-off strategies for retransmission
- Monitor network congestion

### Time Synchronization
- Critical for coordinated Alteriom behaviors
- Use mesh time sync capabilities
- Account for network delays in timing-sensitive operations

## Debugging and Monitoring

### Logging
```cpp
using namespace painlessmesh::logger;
Log.setLogLevel(DEBUG); // Set appropriate level
Log(GENERAL, "Alteriom sensor data: temp=%.1f\n", temperature);
```

### Performance Monitoring
```cpp
#include "plugin/performance.hpp"

// Monitor specific operations
performance::monitor("sensor_collection", [&]() {
    collectSensorData();
});
```

## MCP Server Configuration

The repository includes an MCP server configuration (`mcp-server.json`) that provides:
- Filesystem access for code navigation
- Bash execution for building and testing
- Git integration for version control

This enables enhanced GitHub Copilot functionality with access to project files, build capabilities, and version control operations.

## Deployment for Alteriom

### Build Configuration
Update `library.json` and `CMakeLists.txt` for Alteriom-specific requirements:
- Custom dependencies
- Alteriom-specific compile flags
- Custom examples and tests

### Documentation
- Document Alteriom-specific APIs
- Provide integration examples
- Update README with Alteriom use cases

## Contributing to Alteriom painlessMesh

1. Follow the existing code style (`.clang-format`)
2. Write comprehensive tests for new functionality
3. Update documentation for API changes
4. Consider backward compatibility
5. Test on both ESP32 and ESP8266 platforms
6. Validate memory usage and performance impact