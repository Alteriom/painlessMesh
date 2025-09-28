# Frequently Asked Questions

## General Questions

### Q: What is painlessMesh?

**A:** painlessMesh is a library that makes it easy to create mesh networks with ESP8266 and ESP32 devices. It automatically handles:
- Node discovery and connection
- Message routing between nodes
- Network topology management
- Time synchronization across all nodes
- Self-healing when nodes join or leave

### Q: How many nodes can I have in a mesh?

**A:** The practical limit depends on your ESP model and memory constraints:
- **ESP8266**: 10-20 nodes typically, limited by ~80KB RAM
- **ESP32**: 20-50+ nodes, limited by ~320KB RAM
- Network diameter should stay under 5-7 hops for good performance
- Each node can connect to 2-10 other nodes depending on memory

### Q: What's the range of the mesh network?

**A:** Each WiFi connection has typical range of:
- **Indoor**: 30-50 meters
- **Outdoor (line of sight)**: 100-200 meters  
- **Through walls**: 10-30 meters depending on construction

The mesh extends this by hopping through intermediate nodes. Total range depends on your network topology.

### Q: Do I need a WiFi router for the mesh to work?

**A:** No! painlessMesh creates its own network. Nodes communicate directly with each other without needing internet or a router. However, you can add bridge nodes that connect the mesh to external networks.

## Technical Questions

### Q: How does message routing work?

**A:** painlessMesh uses a tree topology with automatic routing:

1. **Broadcast messages** are sent to all nodes via flood routing
2. **Single messages** use shortest-path routing to specific nodes  
3. **Neighbor messages** go only to directly connected nodes
4. Routing tables update automatically as topology changes

See [Message Routing](../architecture/routing.md) for details.

### Q: Are messages guaranteed to be delivered?

**A:** painlessMesh provides **best-effort delivery**:
- TCP connections provide reliability between directly connected nodes
- No end-to-end delivery guarantees across multiple hops
- Network partitions or node failures can cause message loss
- For critical messages, implement application-level acknowledgments

### Q: How accurate is time synchronization?

**A:** Time sync accuracy is typically:
- **Direct connections**: ±1-5 milliseconds
- **Multi-hop**: ±10-50 milliseconds depending on network load
- **Stability**: Good for coordinating actions within ~100ms windows
- Clock drift is corrected automatically every few minutes

### Q: Can I mix ESP8266 and ESP32 in the same mesh?

**A:** Yes! ESP8266 and ESP32 devices work together seamlessly in the same mesh. However:
- ESP8266 nodes will have fewer connections due to memory limits
- ESP32 nodes may become hubs due to their higher capacity
- Message size limits should account for ESP8266 constraints

## Development Questions

### Q: Which Arduino libraries do I need?

**A:** painlessMesh requires:
```cpp
// Core dependencies (install these)
#include "painlessMesh.h"
#include "ArduinoJson.h"        // v6.x
#include "TaskScheduler.h"      // v3.x

// Platform libraries (built-in)
#include "WiFi.h"              // ESP32
#include "ESP8266WiFi.h"       // ESP8266
```

### Q: Can I use painlessMesh with other WiFi libraries?

**A:** painlessMesh manages WiFi internally and may conflict with other WiFi code. If you need external WiFi:
- Use bridge nodes that connect the mesh to external networks
- Avoid calling WiFi functions directly in mesh nodes
- Consider time-division approaches (mesh mode vs. WiFi mode)

### Q: How do I send sensor data efficiently?

**A:** Use the plugin system for type-safe, efficient messaging:

```cpp
// Define custom package
class SensorPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    float temperature, humidity;
    uint32_t timestamp;
    
    SensorPackage() : BroadcastPackage(100) {}
    // ... implement serialization methods
};

// Send data
SensorPackage sensor;
sensor.temperature = readTemperature();
sensor.humidity = readHumidity();
sensor.timestamp = mesh.getNodeTime();
mesh.sendPackage(&sensor);
```

### Q: How do I handle different message types?

**A:** Use the plugin system with type-specific handlers:

```cpp
// Register handlers for different types
mesh.onPackage(SENSOR_DATA, handleSensorData);
mesh.onPackage(COMMAND_MSG, handleCommand);
mesh.onPackage(STATUS_MSG, handleStatus);

// Or use the raw callback with type checking
mesh.onReceive([](uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    
    int msgType = doc["type"];
    switch(msgType) {
        case SENSOR_DATA:
            // Handle sensor data
            break;
        case COMMAND_MSG:
            // Handle commands
            break;
    }
});
```

## Network Design Questions

### Q: What's the best network topology?

**A:** For most applications, aim for a **balanced tree**:
- Avoid long chains (high latency)
- Avoid star topologies (bottlenecks at center)
- Distribute connections evenly
- Place high-capacity nodes (ESP32) as hubs
- Keep network diameter under 5-7 hops

### Q: How do I make the mesh more reliable?

**A:** Follow these best practices:

1. **Power supply**: Use stable, adequate power sources
2. **Placement**: Ensure good WiFi coverage between nodes
3. **Memory management**: Monitor and optimize memory usage
4. **Error handling**: Implement callbacks for connection events
5. **Redundancy**: Design for node failures
6. **Testing**: Test with realistic loads and distances

### Q: Can nodes sleep or use deep sleep?

**A:** Node sleep modes affect mesh connectivity:

- **Light sleep**: Node stays connected but may miss messages
- **Deep sleep**: Node disconnects from mesh entirely
- **Modem sleep**: WiFi radio sleeps between messages (automatic)

For battery-powered nodes:
```cpp
void enterSleepMode() {
    // Send status before sleeping
    StatusPackage status;
    status.sleepDuration = 300; // 5 minutes
    mesh.sendPackage(&status);
    
    // Give time for message to send
    delay(1000);
    
    // Disconnect cleanly
    mesh.stop();
    
    // Deep sleep
    ESP.deepSleep(300e6); // 5 minutes in microseconds
}
```

## Performance Questions

### Q: Why are my messages slow?

**A:** Common causes of slow message delivery:

1. **Network topology**: Long chains or bottlenecked hubs
2. **Message frequency**: Too many messages causing congestion  
3. **Message size**: Large messages take longer to transmit
4. **CPU load**: Other tasks interfering with mesh processing
5. **WiFi interference**: Congested 2.4GHz band

### Q: How can I optimize performance?

**A:** Performance optimization strategies:

```cpp
// 1. Reduce message frequency
Task slowTask(30000, TASK_FOREVER, &sendData); // Every 30s instead of 1s

// 2. Compact message format
String msg = "{\"t\":" + String(temp) + ",\"h\":" + String(hum) + "}";

// 3. Batch multiple values
String msg = "{\"sensors\":[" + temp + "," + hum + "," + pressure + "]}";

// 4. Use appropriate routing
mesh.sendSingle(targetNode, msg); // Instead of broadcast when possible

// 5. Optimize task timing
mesh.update(); // Call regularly but don't block
yield(); // Allow other tasks to run
```

### Q: How much memory does painlessMesh use?

**A:** Typical memory usage:
- **Core library**: ~20-30KB
- **Per connection**: ~5-10KB
- **Message buffers**: ~2-5KB per connection
- **JSON processing**: ~1-2KB per message

Monitor memory usage:
```cpp
void checkMemory() {
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Largest block: %u bytes\n", ESP.getMaxAllocHeap());
}
```

## Security Questions

### Q: Is painlessMesh secure?

**A:** Current security features are basic:
- **Network password**: WPA2 protection for WiFi connections
- **No message encryption**: Messages are sent in plaintext
- **No authentication**: Any device with the password can join

### Q: How can I improve security?

**A:** Security enhancement options:

```cpp
// 1. Change default passwords regularly
#define MESH_PASSWORD "UniquePassword123!" // Use strong passwords

// 2. Implement application-level encryption
String encryptMessage(String plaintext) {
    // Add your encryption here
    return encryptedMessage;
}

// 3. Validate message sources
mesh.onReceive([](uint32_t from, String& msg) {
    if (!isAuthorizedNode(from)) {
        Serial.printf("Ignoring message from unauthorized node: %u\n", from);
        return;
    }
    processMessage(msg);
});

// 4. Use message authentication
String createSecureMessage(String data) {
    String timestamp = String(mesh.getNodeTime());
    String signature = calculateHMAC(data + timestamp + secretKey);
    return "{\"data\":\"" + data + "\",\"time\":" + timestamp + 
           ",\"sig\":\"" + signature + "\"}";
}
```

### Q: Can someone intercept my mesh messages?

**A:** Yes, without additional encryption:
- WiFi traffic can be intercepted with standard tools
- Messages are JSON plaintext by default
- Network password only protects WiFi association
- Consider application-level encryption for sensitive data

## Integration Questions

### Q: Can I connect the mesh to the internet?

**A:** Yes, using bridge nodes:

```cpp
// Bridge node connects to both mesh and internet
void setup() {
    // Connect to mesh
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    
    // Also connect to home WiFi
    WiFi.begin("HomeWiFi", "password");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    Serial.println("Bridge node: Connected to both mesh and internet");
}

// Forward mesh data to internet services
mesh.onReceive([](uint32_t from, String& msg) {
    // Forward to HTTP server, MQTT broker, etc.
    httpClient.POST("http://myserver.com/api/data", msg);
});
```

### Q: Can I use MQTT with painlessMesh?

**A:** Yes, through bridge nodes or by running MQTT alongside the mesh:

```cpp
#include "PubSubClient.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Bridge mesh messages to MQTT
mesh.onReceive([](uint32_t from, String& msg) {
    String topic = "mesh/node/" + String(from);
    mqttClient.publish(topic.c_str(), msg.c_str());
});

// Bridge MQTT messages to mesh
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg = String((char*)payload);
    mesh.sendBroadcast(msg);
}
```

### Q: How do I integrate with home automation systems?

**A:** Common integration approaches:

1. **HTTP REST API**: Bridge node exposes REST endpoints
2. **MQTT**: Bridge publishes sensor data to MQTT broker
3. **Home Assistant**: Use MQTT discovery or custom integration
4. **Node-RED**: Connect via MQTT or HTTP
5. **Direct integration**: Custom protocol over TCP/WebSocket

## Alteriom-Specific Questions

### Q: What are Alteriom extensions?

**A:** Alteriom provides pre-built packages for common IoT use cases:

- **SensorPackage**: Environmental sensor data (temperature, humidity, pressure)
- **CommandPackage**: Device control commands  
- **StatusPackage**: Device health monitoring

See [Alteriom Overview](../alteriom/overview.md) for details.

### Q: Can I use Alteriom packages in my own projects?

**A:** Yes! The Alteriom packages are examples you can:
- Use directly in your projects
- Modify for your specific needs
- Use as templates for your own packages

```cpp
#include "examples/alteriom/alteriom_sensor_package.hpp"
using namespace alteriom;

// Use directly
SensorPackage sensor;
sensor.temperature = 25.0;
mesh.sendPackage(&sensor);

// Or extend for your needs
class MySensorPackage : public SensorPackage {
public:
    float lightLevel = 0.0; // Add custom field
    // ... implement serialization
};
```

## Troubleshooting Questions

### Q: My ESP keeps crashing. What should I check?

**A:** Common crash causes:

1. **Memory issues**: Monitor heap usage
2. **Power supply**: Ensure stable, adequate power
3. **Stack overflow**: Reduce recursion, large local variables
4. **Watchdog timeout**: Add `yield()` calls in long loops
5. **Hardware issues**: Bad connections, damaged ESP

Enable crash debugging:
```cpp
// ESP32
#include "esp_system.h"
esp_core_dump_init();

// ESP8266  
Serial.println("Last reset reason: " + ESP.getResetReason());
```

### Q: Why can't I see debug messages?

**A:** Debug message troubleshooting:

```cpp
void setup() {
    Serial.begin(115200);
    delay(1000); // Wait for serial to initialize
    
    // Enable debug messages BEFORE mesh.init()
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    
    Serial.println("Setup completed"); // Test message
}
```

Check:
- Correct baud rate (115200)
- Serial cable supports data (not just power)
- Debug messages enabled before mesh initialization
- Serial monitor connected to correct COM port

### Q: Messages work sometimes but not always. Why?

**A:** Intermittent message issues usually indicate:

1. **Memory pressure**: Messages dropped when low memory
2. **Network congestion**: Too many messages simultaneously  
3. **Connection instability**: Nodes connecting/disconnecting
4. **Power issues**: Voltage drops during transmission
5. **WiFi interference**: Other 2.4GHz devices

Add reliability checks:
```cpp
bool sendReliableMessage(String msg) {
    bool sent = mesh.sendBroadcast(msg);
    if (!sent) {
        Serial.println("Failed to send - will retry");
        delay(1000);
        return mesh.sendBroadcast(msg); // Retry once
    }
    return true;
}
```

## Getting More Help

If your question isn't answered here:

1. Check [Common Issues](common-issues.md) for detailed troubleshooting
2. Search the [GitHub Issues](https://github.com/Alteriom/painlessMesh/issues)
3. Post in the [Community Forum](https://groups.google.com/forum/#!forum/painlessmesh-user)
4. Review the [API Documentation](../api/core-api.md)
5. Look at [Example Code](../tutorials/basic-examples.md) for working implementations

When asking for help, include:
- Hardware details (ESP32/ESP8266 model)
- Library versions
- Complete code example
- Serial output with debug enabled
- Network topology description