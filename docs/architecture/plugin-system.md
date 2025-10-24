# Plugin System Architecture

The painlessMesh plugin system provides a type-safe, extensible framework for creating custom message types and handlers. This document explains how the plugin system works internally and how to extend it.

## Overview

The plugin system enables:
- **Type-safe messaging** between nodes
- **Custom package definitions** for specific use cases
- **Automatic serialization/deserialization** to/from JSON
- **Message routing control** (broadcast, single, neighbor)
- **Event-driven processing** with callbacks

## Architecture Components

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │ SensorPackage   │  │ CommandPackage  │  │ StatusPackage│ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    Plugin Framework                         │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │ SinglePackage   │  │BroadcastPackage │  │NeighbourPkg  │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    Protocol Layer                           │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │PackageInterface │  │    Variant      │  │ PackageHandler│ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    Mesh Core                                │
└─────────────────────────────────────────────────────────────┘
```

## Core Interfaces

### PackageInterface

The base interface all packages must implement:

```cpp
namespace painlessmesh {
namespace protocol {

class PackageInterface {
public:
    uint32_t from;      // Source node ID
    router::Type routing; // Routing strategy
    int type;           // Package type identifier
    
    // Serialization to JSON
    virtual JsonObject addTo(JsonObject&& jsonObj) const = 0;
    
    // Size calculation for buffer allocation
    virtual size_t jsonObjectSize() const = 0;
};

}} // namespace painlessmesh::protocol
```

### Package Base Classes

#### SinglePackage
For point-to-point messages to specific nodes:

```cpp
class SinglePackage : public protocol::PackageInterface {
public:
    uint32_t dest;      // Destination node ID
    int noJsonFields = 4; // Base field count
    
    SinglePackage(int type) : routing(router::SINGLE), type(type) {}
    
    SinglePackage(JsonObject jsonObj) {
        from = jsonObj["from"];
        dest = jsonObj["dest"]; 
        type = jsonObj["type"];
        routing = static_cast<router::Type>(jsonObj["routing"].as<int>());
    }
    
    JsonObject addTo(JsonObject&& jsonObj) const override {
        jsonObj["from"] = from;
        jsonObj["dest"] = dest;
        jsonObj["routing"] = static_cast<int>(routing);
        jsonObj["type"] = type;
        return jsonObj;
    }
};
```

#### BroadcastPackage  
For messages to all nodes in the mesh:

```cpp
class BroadcastPackage : public protocol::PackageInterface {
public:
    int noJsonFields = 3; // Base field count
    
    BroadcastPackage(int type) : routing(router::BROADCAST), type(type) {}
    
    BroadcastPackage(JsonObject jsonObj) {
        from = jsonObj["from"];
        type = jsonObj["type"];
        routing = static_cast<router::Type>(jsonObj["routing"].as<int>());
    }
    
    JsonObject addTo(JsonObject&& jsonObj) const override {
        jsonObj["from"] = from;
        jsonObj["routing"] = static_cast<int>(routing);
        jsonObj["type"] = type;
        return jsonObj;
    }
};
```

#### NeighbourPackage
For messages to directly connected nodes only:

```cpp
class NeighbourPackage : public plugin::SinglePackage {
public:
    NeighbourPackage(int type) : SinglePackage(type) {
        routing = router::NEIGHBOUR;
    }
    
    NeighbourPackage(JsonObject jsonObj) : SinglePackage(jsonObj) {}
};
```

## Type System

### Package Type IDs

Each package type needs a unique identifier:

```cpp
// Core painlessMesh types: 1-12 (reserved)
enum CoreTypes {
    MSG_TYPE = 1,
    TIME_SYNC = 2,
    NODE_SYNC = 3,
    // ... other core types
};

// Custom types: 20+ (recommended)
enum CustomTypes {
    SENSOR_DATA = 20,
    DEVICE_COMMAND = 21,
    STATUS_REPORT = 22,
    // ... your custom types
};

// Alteriom types: 200+ (for Alteriom extensions)
enum AlteriomTypes {
    ALTERIOM_SENSOR = 200,
    ALTERIOM_COMMAND = 400,
    ALTERIOM_STATUS = 202
};
```

### Routing Types

```cpp
namespace router {
enum Type {
    SINGLE = 0,     // Point-to-point
    BROADCAST = 1,  // To all nodes  
    NEIGHBOUR = 2   // To direct neighbors only
};
}
```

## Message Processing Flow

### Outbound Messages

```
Application
    ↓
Create Package → Serialize to JSON → Queue for Transmission
    ↓                   ↓                       ↓
Set Fields          addTo()                Send via TCP
```

### Inbound Messages  

```
TCP Reception → JSON Parse → Type Lookup → Deserialize → Callback
     ↓              ↓           ↓             ↓           ↓
Raw Message    JsonObject   Type ID      Package     User Handler
```

## Variant System

The `Variant` class provides type-safe serialization and deserialization:

```cpp
namespace protocol {

class Variant {
public:
    // Create from package
    Variant(const PackageInterface* pkg);
    
    // Deserialize to specific type
    template<typename T>
    T to() const;
    
    // Get routing information
    uint32_t dest() const;
    router::Type routing() const;
    int type() const;
    
    // Serialize to string
    void printTo(String& output) const;
};

}
```

### Usage Example

```cpp
// Sending
SensorPackage sensor;
sensor.temperature = 25.0;
sensor.humidity = 60.0;

protocol::Variant variant(&sensor);
String message;
variant.printTo(message);
// Send message via mesh

// Receiving  
void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    JsonObject obj = doc.as<JsonObject>();
    
    int msgType = obj["type"];
    if (msgType == SENSOR_DATA) {
        protocol::Variant variant(obj);
        SensorPackage received = variant.to<SensorPackage>();
        // Process sensor data
    }
}
```

## Creating Custom Packages

### Step 1: Define Package Class

```cpp
class WeatherPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    // Data fields
    float temperature = 0.0;
    float humidity = 0.0;
    float pressure = 0.0;
    uint32_t timestamp = 0;
    TSTRING location = "";
    
    // Constructor with type ID
    WeatherPackage() : BroadcastPackage(25) {} // Use unique ID
    
    // Deserialization constructor
    WeatherPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
        temperature = jsonObj["temp"];
        humidity = jsonObj["hum"];
        pressure = jsonObj["pres"];
        timestamp = jsonObj["time"];
        location = jsonObj["loc"].as<TSTRING>();
    }
    
    // Serialization method
    JsonObject addTo(JsonObject&& jsonObj) const override {
        jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
        jsonObj["temp"] = temperature;
        jsonObj["hum"] = humidity;
        jsonObj["pres"] = pressure;
        jsonObj["time"] = timestamp;
        jsonObj["loc"] = location;
        return jsonObj;
    }

#if ARDUINOJSON_VERSION_MAJOR < 7
    // Size calculation for buffer allocation
    size_t jsonObjectSize() const override { 
        return JSON_OBJECT_SIZE(noJsonFields + 5) + location.length(); 
    }
#endif
};
```

### Step 2: Register Handler

```cpp
void setup() {
    // Initialize mesh
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    
    // Register handler for weather packages
    mesh.onPackage(25, [](protocol::Variant& variant) {
        WeatherPackage weather = variant.to<WeatherPackage>();
        
        Serial.printf("Weather from %u: T=%.1f°C, H=%.1f%%, P=%.1f hPa at %s\n",
                     weather.from, weather.temperature, weather.humidity, 
                     weather.pressure, weather.location.c_str());
        
        return false; // Don't stop propagation
    });
}
```

### Step 3: Send Packages

```cpp
void sendWeatherData() {
    WeatherPackage weather;
    weather.from = mesh.getNodeId();
    weather.temperature = readTemperature();
    weather.humidity = readHumidity();
    weather.pressure = readPressure();
    weather.timestamp = mesh.getNodeTime();
    weather.location = "Sensor Station Alpha";
    
    mesh.sendPackage(&weather);
}
```

## Advanced Features

### Task Integration

The plugin system integrates with TaskScheduler:

```cpp
// Add recurring task
auto task = mesh.addTask(30000, TASK_FOREVER, [](){
    WeatherPackage weather;
    // ... populate data
    mesh.sendPackage(&weather);
});

// One-time task
mesh.addTask([](){
    StatusPackage status;
    // ... populate status
    mesh.sendPackage(&status);
});
```

### Message Filtering

```cpp
// Handler that filters messages
mesh.onPackage(SENSOR_DATA, [](protocol::Variant& variant) {
    SensorPackage sensor = variant.to<SensorPackage>();
    
    // Only process recent data
    if (mesh.getNodeTime() - sensor.timestamp > 60000000) { // 60 seconds
        return false; // Ignore old data
    }
    
    // Process valid sensor data
    processSensorData(sensor);
    return false;
});
```

### Conditional Routing

```cpp
class ConditionalPackage : public painlessmesh::plugin::SinglePackage {
public:
    bool urgent = false;
    
    ConditionalPackage() : SinglePackage(30) {}
    
    // Override routing based on urgency
    router::Type getRouting() const {
        return urgent ? router::BROADCAST : router::SINGLE;
    }
};
```

## Memory Management

### ArduinoJson Integration

```cpp
// Efficient buffer sizing
size_t jsonObjectSize() const override {
    size_t baseSize = JSON_OBJECT_SIZE(noJsonFields + customFieldCount);
    size_t stringSize = stringField1.length() + stringField2.length(); 
    return baseSize + stringSize;
}

// Use appropriate document size
DynamicJsonDocument doc(package.jsonObjectSize() + 100); // Add safety margin
```

### String Handling

```cpp
// Use TSTRING for cross-platform compatibility
TSTRING deviceName = "WeatherStation01";

// Efficient string operations
void updateName(const TSTRING& newName) {
    deviceName = newName;
    deviceName.reserve(32); // Pre-allocate for efficiency
}
```

## Error Handling

### Serialization Errors

```cpp
JsonObject addTo(JsonObject&& jsonObj) const override {
    try {
        jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
        
        // Validate data before serialization
        if (temperature < -50 || temperature > 100) {
            Serial.println("Warning: Temperature out of range");
        }
        
        jsonObj["temp"] = temperature;
        return jsonObj;
    } catch (...) {
        Serial.println("Error serializing weather package");
        return jsonObj;
    }
}
```

### Deserialization Validation

```cpp
WeatherPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    // Validate required fields exist
    if (jsonObj.containsKey("temp")) {
        temperature = jsonObj["temp"];
    } else {
        Serial.println("Missing temperature field");
        temperature = 0.0;
    }
    
    // Range validation
    if (temperature < -50 || temperature > 100) {
        Serial.printf("Invalid temperature: %.1f\n", temperature);
        temperature = 0.0;
    }
}
```

## Best Practices

### Package Design

1. **Keep packages small** - Minimize memory usage
2. **Use appropriate routing** - Don't broadcast when single-cast suffices
3. **Include timestamps** - Enable data age validation
4. **Validate inputs** - Check ranges and formats
5. **Version your schemas** - Plan for future changes

### Type Management

1. **Use unique type IDs** - Avoid conflicts with other packages
2. **Document type assignments** - Maintain a registry
3. **Group related types** - Use ranges for related functionality
4. **Reserve ranges** - Plan for future expansion

### Performance

1. **Pre-calculate sizes** - Implement `jsonObjectSize()` accurately
2. **Minimize string operations** - Use fixed-size fields when possible
3. **Batch operations** - Send multiple readings in one package
4. **Cache frequently used objects** - Avoid repeated allocations

## Integration with Alteriom

The Alteriom extensions demonstrate advanced plugin usage:

```cpp
namespace alteriom {

class SensorPackage : public painlessmesh::plugin::BroadcastPackage {
    // Environmental sensor data
    // Type ID: 200
};

class CommandPackage : public painlessmesh::plugin::SinglePackage {
    // Device control commands  
    // Type ID: 201
};

class StatusPackage : public painlessmesh::plugin::BroadcastPackage {
    // Device health and status
    // Type ID: 202
};

}
```

See [Alteriom Extensions](../alteriom/overview.md) for detailed usage examples.

## Next Steps

- Learn about [Message Routing](routing.md) algorithms
- Explore [Alteriom Packages](../alteriom/sensor-packages.md) for real-world examples  
- See [Performance Optimization](../advanced/performance.md) for efficiency tips
- Check [Custom Packages Tutorial](../tutorials/custom-packages.md) for hands-on examples