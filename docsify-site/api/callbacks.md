# API Callbacks

painlessMesh provides several callback functions to handle mesh events and messages. These callbacks allow your application to respond to network changes, receive messages, and handle various mesh events.

## Message Callbacks

### onReceive()

Called when a message is received from another node.

```cpp
void mesh.onReceive(std::function<void(uint32_t from, String& msg)> callback)
```

**Parameters:**
- `from` - Node ID of the sender
- `msg` - The received message string

**Example:**
```cpp
mesh.onReceive([](uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
    
    // Parse message and take action
    if (msg == "turn_on_led") {
        digitalWrite(LED_PIN, HIGH);
    }
});
```

**Advanced Usage:**
```cpp
void receivedCallback(uint32_t from, String& msg) {
    // Log the message
    Serial.printf("Message from node %u: %s\n", from, msg.c_str());
    
    // Parse JSON messages
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, msg);
    
    if (!error) {
        String command = doc["command"];
        int value = doc["value"];
        
        executeCommand(command, value);
    }
    
    // Echo back acknowledgment
    String ack = "ACK:" + msg;
    mesh.sendSingle(from, ack);
}

mesh.onReceive(&receivedCallback);
```

### onPackage()

Type-safe callback for custom package types (requires painlessMesh plugin system).

```cpp
void mesh.onPackage(uint8_t packageType, std::function<bool(protocol::Variant& variant)> callback)
```

**Parameters:**
- `packageType` - The type ID of the package
- `callback` - Function to handle the package, returns true if handled

**Example:**
```cpp
// Define custom package
class SensorPackage : public painlessmesh::plugin::BroadcastPackage {
public:
    float temperature;
    float humidity;
    // ... implementation
};

// Register handler
mesh.onPackage(200, [](protocol::Variant& variant) {
    SensorPackage sensor = variant.to<SensorPackage>();
    Serial.printf("Sensor data: T=%.1f°C, H=%.1f%%\n", 
                  sensor.temperature, sensor.humidity);
    return true; // Package handled
});
```

## Connection Callbacks

### onNewConnection()

Called when a new node connects to the mesh.

```cpp
void mesh.onNewConnection(std::function<void(uint32_t nodeId)> callback)
```

**Parameters:**
- `nodeId` - The node ID of the newly connected node

**Example:**
```cpp
mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("New node connected: %u\n", nodeId);
    Serial.printf("Total nodes in mesh: %d\n", mesh.getNodeList().size() + 1);
    
    // Send welcome message to new node
    String welcome = "Welcome to the mesh!";
    mesh.sendSingle(nodeId, welcome);
    
    // Update network topology display
    updateNetworkDisplay();
});
```

### onChangedConnections()

Called when the mesh topology changes (nodes connect or disconnect).

```cpp
void mesh.onChangedConnections(std::function<void()> callback)
```

**Example:**
```cpp
mesh.onChangedConnections([]() {
    Serial.println("Mesh topology changed");
    
    auto nodes = mesh.getNodeList();
    Serial.printf("Currently connected to %d nodes: ", nodes.size());
    
    for (auto nodeId : nodes) {
        Serial.printf("%u ", nodeId);
    }
    Serial.println();
    
    // Update routing tables or network status
    updateNetworkMap();
});
```

### onDroppedConnection()

Called when a connection to a node is lost.

```cpp
void mesh.onDroppedConnection(std::function<void(uint32_t nodeId)> callback)
```

**Parameters:**
- `nodeId` - The node ID of the disconnected node

**Example:**
```cpp
mesh.onDroppedConnection([](uint32_t nodeId) {
    Serial.printf("Lost connection to node: %u\n", nodeId);
    
    // Clean up any node-specific data
    removeNodeData(nodeId);
    
    // Check if this was a critical node
    if (isCriticalNode(nodeId)) {
        Serial.println("WARNING: Critical node disconnected!");
        triggerAlert();
    }
});
```

## Time Synchronization Callbacks

### onNodeTimeAdjusted()

Called when the node's time is adjusted for mesh synchronization.

```cpp
void mesh.onNodeTimeAdjusted(std::function<void(int32_t offset)> callback)
```

**Parameters:**
- `offset` - Time adjustment in microseconds

**Example:**
```cpp
mesh.onNodeTimeAdjusted([](int32_t offset) {
    Serial.printf("Time adjusted by %d microseconds\n", offset);
    
    // Log significant time adjustments
    if (abs(offset) > 1000000) { // More than 1 second
        Serial.println("WARNING: Large time adjustment detected");
    }
    
    // Update any time-sensitive operations
    recalibrateTimers(offset);
});
```

### onNodeDelayReceived()

Called when delay measurement to a node is completed.

```cpp
void mesh.onNodeDelayReceived(std::function<void(uint32_t nodeId, int32_t delay)> callback)
```

**Parameters:**
- `nodeId` - The node ID
- `delay` - Round-trip delay in microseconds

**Example:**
```cpp
mesh.onNodeDelayReceived([](uint32_t nodeId, int32_t delay) {
    Serial.printf("Delay to node %u: %d µs\n", nodeId, delay);
    
    // Track network performance
    networkDelays[nodeId] = delay;
    
    // Alert on high latency
    if (delay > 100000) { // More than 100ms
        Serial.printf("HIGH LATENCY to node %u: %d µs\n", nodeId, delay);
    }
});
```

## Mesh Events

### onEvent()

Generic event handler for various mesh events.

```cpp
void mesh.onEvent(std::function<void(painlessmesh::protocol::MeshEvent event, 
                                    uint32_t nodeId, 
                                    String data)> callback)
```

**Events:**
- `NODE_JOINED` - New node joined the mesh
- `NODE_LEFT` - Node left the mesh  
- `MESSAGE_SENT` - Message was sent
- `MESSAGE_RECEIVED` - Message was received
- `TIME_SYNC` - Time synchronization occurred

**Example:**
```cpp
mesh.onEvent([](painlessmesh::protocol::MeshEvent event, uint32_t nodeId, String data) {
    switch(event) {
        case painlessmesh::protocol::NODE_JOINED:
            Serial.printf("Event: Node %u joined\n", nodeId);
            break;
            
        case painlessmesh::protocol::NODE_LEFT:
            Serial.printf("Event: Node %u left\n", nodeId);
            break;
            
        case painlessmesh::protocol::MESSAGE_SENT:
            Serial.printf("Event: Message sent to %u\n", nodeId);
            break;
            
        case painlessmesh::protocol::TIME_SYNC:
            Serial.printf("Event: Time sync with %u\n", nodeId);
            break;
    }
});
```

## Advanced Callback Patterns

### Message Filtering

Filter messages by type or content:

```cpp
mesh.onReceive([](uint32_t from, String& msg) {
    // Only process sensor messages
    if (msg.startsWith("SENSOR:")) {
        processSensorData(from, msg.substring(7));
    }
    // Only process commands from authorized nodes
    else if (msg.startsWith("CMD:") && isAuthorizedNode(from)) {
        executeCommand(from, msg.substring(4));
    }
});
```

### State Management

Update application state based on network events:

```cpp
enum NodeRole { SENSOR, CONTROLLER, GATEWAY };
NodeRole myRole = SENSOR;

mesh.onNewConnection([](uint32_t nodeId) {
    // Adjust role based on network size
    int networkSize = mesh.getNodeList().size() + 1;
    
    if (networkSize == 1) {
        myRole = GATEWAY; // First node becomes gateway
    } else if (networkSize <= 3) {
        myRole = CONTROLLER; // Early nodes become controllers
    } else {
        myRole = SENSOR; // Later nodes become sensors
    }
    
    Serial.printf("Role changed to: %s\n", roleToString(myRole));
});
```

### Error Handling

Robust error handling in callbacks:

```cpp
mesh.onReceive([](uint32_t from, String& msg) {
    try {
        // Parse and process message
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, msg);
        
        if (error) {
            Serial.printf("JSON parse error from node %u: %s\n", 
                         from, error.c_str());
            return;
        }
        
        // Validate message content
        if (!doc.containsKey("type")) {
            Serial.printf("Invalid message from node %u: missing type\n", from);
            return;
        }
        
        String type = doc["type"];
        if (type == "sensor") {
            processSensorMessage(doc);
        } else if (type == "command") {
            processCommandMessage(doc);
        } else {
            Serial.printf("Unknown message type from node %u: %s\n", 
                         from, type.c_str());
        }
        
    } catch (const std::exception& e) {
        Serial.printf("Exception processing message from node %u: %s\n", 
                     from, e.what());
    }
});
```

### Performance Monitoring

Monitor callback performance:

```cpp
mesh.onReceive([](uint32_t from, String& msg) {
    uint32_t startTime = micros();
    
    // Process message
    processMessage(from, msg);
    
    uint32_t processingTime = micros() - startTime;
    
    // Log slow processing
    if (processingTime > 10000) { // More than 10ms
        Serial.printf("Slow message processing: %u µs\n", processingTime);
    }
});
```

## Best Practices

### 1. Keep Callbacks Fast

Callbacks should execute quickly to avoid blocking the mesh:

```cpp
// ❌ Bad - blocking operation
mesh.onReceive([](uint32_t from, String& msg) {
    delay(1000); // Blocks mesh operations!
    processMessage(msg);
});

// ✅ Good - queue for later processing
std::queue<String> messageQueue;

mesh.onReceive([](uint32_t from, String& msg) {
    messageQueue.push(msg); // Quick operation
});

void loop() {
    mesh.update();
    
    // Process queued messages
    if (!messageQueue.empty()) {
        processMessage(messageQueue.front());
        messageQueue.pop();
    }
}
```

### 2. Handle Errors Gracefully

Always validate input and handle errors:

```cpp
mesh.onReceive([](uint32_t from, String& msg) {
    if (msg.length() == 0) {
        Serial.println("Received empty message");
        return;
    }
    
    if (msg.length() > MAX_MESSAGE_SIZE) {
        Serial.printf("Message too large: %d bytes\n", msg.length());
        return;
    }
    
    // Process valid message
    processMessage(from, msg);
});
```

### 3. Use Lambda Captures Carefully

Be careful with lambda captures to avoid memory issues:

```cpp
// ❌ Dangerous - captures local variable by reference
void setupMesh() {
    String localData = "important data";
    
    mesh.onReceive([&localData](uint32_t from, String& msg) {
        // localData may be invalid when callback executes!
        processMessage(msg, localData);
    });
} // localData goes out of scope

// ✅ Safe - capture by value or use global/member variables
String globalData = "important data";

mesh.onReceive([](uint32_t from, String& msg) {
    processMessage(msg, globalData);
});
```

### 4. Avoid Recursive Sending

Don't send messages from within message callbacks without guards:

```cpp
// ❌ Dangerous - potential infinite loop
mesh.onReceive([](uint32_t from, String& msg) {
    if (msg == "ping") {
        mesh.sendSingle(from, "pong"); // Could cause ping-pong loop!
    }
});

// ✅ Better - add guards and limits
int maxPingResponses = 3;
std::map<uint32_t, int> pingCounts;

mesh.onReceive([](uint32_t from, String& msg) {
    if (msg == "ping" && pingCounts[from] < maxPingResponses) {
        mesh.sendSingle(from, "pong");
        pingCounts[from]++;
    }
});
```

See also:
- [Core API](core-api.md) - Complete method reference
- [Configuration](configuration.md) - Mesh setup options  
- [Message Types](message-types.md) - Built-in message formats