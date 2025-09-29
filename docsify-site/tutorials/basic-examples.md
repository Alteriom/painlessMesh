# Basic Examples

This tutorial provides a collection of basic painlessMesh examples, from simple message passing to more advanced scenarios. Each example builds upon previous concepts and demonstrates key mesh networking principles.

## Example 1: Simple Broadcast

The most basic mesh example - nodes that broadcast messages to all other nodes.

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "secretPassword"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Send a message every 5 seconds
Task taskSendMessage(5000, TASK_FOREVER, [](){
    String msg = "Hello from node " + String(mesh.getNodeId());
    mesh.sendBroadcast(msg);
    Serial.printf("Sent: %s\n", msg.c_str());
});

void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
}

void setup() {
    Serial.begin(115200);
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    
    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();
    
    Serial.printf("Node ID: %u\n", mesh.getNodeId());
}

void loop() {
    mesh.update();
}
```

**Key Concepts:**
- All nodes broadcast the same message type
- Every node receives every broadcast message
- Node ID uniquely identifies each device

## Example 2: Point-to-Point Messaging

Sending messages to specific nodes instead of broadcasting.

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "secretPassword"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

uint32_t targetNode = 0; // Will be set to first discovered node

// Send targeted message every 10 seconds
Task taskSendTargeted(10000, TASK_FOREVER, [](){
    if (targetNode != 0) {
        String msg = "Direct message from " + String(mesh.getNodeId());
        bool sent = mesh.sendSingle(targetNode, msg);
        Serial.printf("Sent to %u: %s [%s]\n", targetNode, msg.c_str(), 
                     sent ? "OK" : "FAILED");
    }
});

void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
    
    // Echo back to sender
    String response = "Echo: " + msg;
    mesh.sendSingle(from, response);
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
    
    // Set first connected node as target
    if (targetNode == 0) {
        targetNode = nodeId;
        Serial.printf("Set target node: %u\n", targetNode);
    }
}

void droppedConnectionCallback(uint32_t nodeId) {
    Serial.printf("Dropped connection: %u\n", nodeId);
    
    // Clear target if it disconnected
    if (targetNode == nodeId) {
        targetNode = 0;
        
        // Find new target from remaining connections
        auto nodes = mesh.getNodeList();
        if (!nodes.empty()) {
            targetNode = *nodes.begin();
            Serial.printf("New target node: %u\n", targetNode);
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onDroppedConnection(&droppedConnectionCallback);
    
    userScheduler.addTask(taskSendTargeted);
    taskSendTargeted.enable();
    
    Serial.printf("Node ID: %u\n", mesh.getNodeId());
}

void loop() {
    mesh.update();
}
```

**Key Concepts:**
- Use `sendSingle()` for point-to-point messages
- Track connected nodes using callbacks
- Handle node disconnections gracefully

## Example 3: JSON Message Format

Structured data exchange using JSON messages.

```cpp
#include "painlessMesh.h"
#include "ArduinoJson.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "secretPassword"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

int messageCounter = 0;

// Send structured data every 15 seconds
Task taskSendData(15000, TASK_FOREVER, [](){
    DynamicJsonDocument doc(200);
    
    doc["type"] = "sensor_data";
    doc["nodeId"] = mesh.getNodeId();
    doc["counter"] = messageCounter++;
    doc["temperature"] = random(150, 350) / 10.0; // 15.0 to 35.0
    doc["humidity"] = random(300, 800) / 10.0;    // 30.0 to 80.0
    doc["timestamp"] = mesh.getNodeTime();
    
    String message;
    serializeJson(doc, message);
    
    mesh.sendBroadcast(message);
    Serial.printf("Sent: %s\n", message.c_str());
});

void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
    
    // Parse JSON message
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) {
        Serial.printf("JSON parsing failed: %s\n", error.c_str());
        return;
    }
    
    // Process different message types
    String msgType = doc["type"];
    
    if (msgType == "sensor_data") {
        uint32_t nodeId = doc["nodeId"];
        float temperature = doc["temperature"];
        float humidity = doc["humidity"];
        uint32_t timestamp = doc["timestamp"];
        
        Serial.printf("Sensor data from node %u: T=%.1f°C, H=%.1f%% (age: %u µs)\n",
                     nodeId, temperature, humidity, 
                     mesh.getNodeTime() - timestamp);
        
        // Respond with acknowledgment
        sendAcknowledgment(from, doc["counter"]);
    }
    else if (msgType == "acknowledgment") {
        int counter = doc["counter"];
        Serial.printf("Received acknowledgment from %u for message %d\n", from, counter);
    }
}

void sendAcknowledgment(uint32_t targetNode, int counter) {
    DynamicJsonDocument doc(100);
    
    doc["type"] = "acknowledgment";
    doc["nodeId"] = mesh.getNodeId();
    doc["counter"] = counter;
    
    String message;
    serializeJson(doc, message);
    
    mesh.sendSingle(targetNode, message);
}

void setup() {
    Serial.begin(115200);
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    
    userScheduler.addTask(taskSendData);
    taskSendData.enable();
    
    Serial.printf("Node ID: %u\n", mesh.getNodeId());
}

void loop() {
    mesh.update();
}
```

**Key Concepts:**
- Use ArduinoJson for structured data
- Handle JSON parsing errors gracefully
- Implement message acknowledgments
- Add timestamps for data age validation

## Example 4: Multi-Function Node

A node that handles multiple types of messages and functions.

```cpp
#include "painlessMesh.h"
#include "ArduinoJson.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "secretPassword"
#define MESH_PORT       5555

// LED pin (built-in LED on most ESP boards)
#define LED_PIN 2

Scheduler userScheduler;
painlessMesh mesh;

bool ledState = false;
int statusCounter = 0;

// Send status every 30 seconds
Task taskSendStatus(30000, TASK_FOREVER, [](){
    DynamicJsonDocument doc(300);
    
    doc["type"] = "status";
    doc["nodeId"] = mesh.getNodeId();
    doc["counter"] = statusCounter++;
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["connections"] = mesh.getNodeList().size();
    doc["ledState"] = ledState;
    doc["timestamp"] = mesh.getNodeTime();
    
    String message;
    serializeJson(doc, message);
    
    mesh.sendBroadcast(message);
    Serial.printf("Status sent: uptime=%ds, heap=%d, connections=%d\n",
                 (int)(millis()/1000), ESP.getFreeHeap(), mesh.getNodeList().size());
});

// Blink LED every 2 seconds
Task taskBlinkLED(2000, TASK_FOREVER, [](){
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
});

void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) return;
    
    String msgType = doc["type"];
    
    if (msgType == "status") {
        handleStatusMessage(from, doc);
    }
    else if (msgType == "command") {
        handleCommandMessage(from, doc);
    }
    else if (msgType == "ping") {
        handlePingMessage(from, doc);
    }
}

void handleStatusMessage(uint32_t from, JsonDocument& doc) {
    int uptime = doc["uptime"];
    int freeHeap = doc["freeHeap"];
    int connections = doc["connections"];
    bool remoteLedState = doc["ledState"];
    
    Serial.printf("Status from %u: uptime=%ds, heap=%d, LED=%s\n",
                 from, uptime, freeHeap, remoteLedState ? "ON" : "OFF");
    
    // Check if remote node needs help
    if (freeHeap < 10000) {
        Serial.printf("Warning: Node %u has low memory!\n", from);
        sendHelpOffer(from);
    }
}

void handleCommandMessage(uint32_t from, JsonDocument& doc) {
    String command = doc["command"];
    
    Serial.printf("Command from %u: %s\n", from, command.c_str());
    
    if (command == "led_on") {
        ledState = true;
        digitalWrite(LED_PIN, HIGH);
        sendCommandResponse(from, "led_on", "OK");
    }
    else if (command == "led_off") {
        ledState = false;
        digitalWrite(LED_PIN, LOW);
        sendCommandResponse(from, "led_off", "OK");
    }
    else if (command == "get_status") {
        // Force immediate status send
        taskSendStatus.forceNextIteration();
    }
    else {
        sendCommandResponse(from, command, "UNKNOWN_COMMAND");
    }
}

void handlePingMessage(uint32_t from, JsonDocument& doc) {
    uint32_t pingTime = doc["timestamp"];
    
    // Send pong response
    DynamicJsonDocument response(100);
    response["type"] = "pong";
    response["nodeId"] = mesh.getNodeId();
    response["pingTime"] = pingTime;
    response["pongTime"] = mesh.getNodeTime();
    
    String message;
    serializeJson(response, message);
    mesh.sendSingle(from, message);
    
    Serial.printf("Responded to ping from %u\n", from);
}

void sendCommandResponse(uint32_t targetNode, String command, String result) {
    DynamicJsonDocument doc(150);
    
    doc["type"] = "command_response";
    doc["nodeId"] = mesh.getNodeId();
    doc["command"] = command;
    doc["result"] = result;
    doc["timestamp"] = mesh.getNodeTime();
    
    String message;
    serializeJson(doc, message);
    mesh.sendSingle(targetNode, message);
}

void sendHelpOffer(uint32_t targetNode) {
    DynamicJsonDocument doc(100);
    
    doc["type"] = "help_offer";
    doc["nodeId"] = mesh.getNodeId();
    doc["message"] = "I can help with processing";
    
    String message;
    serializeJson(doc, message);
    mesh.sendSingle(targetNode, message);
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u (total: %d)\n", nodeId, mesh.getNodeList().size());
    
    // Send welcome message to new node
    DynamicJsonDocument doc(100);
    doc["type"] = "welcome";
    doc["nodeId"] = mesh.getNodeId();
    doc["message"] = "Welcome to the mesh!";
    
    String message;
    serializeJson(doc, message);
    mesh.sendSingle(nodeId, message);
}

void droppedConnectionCallback(uint32_t nodeId) {
    Serial.printf("Lost connection: %u (remaining: %d)\n", 
                 nodeId, mesh.getNodeList().size());
}

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onDroppedConnection(&droppedConnectionCallback);
    
    userScheduler.addTask(taskSendStatus);
    userScheduler.addTask(taskBlinkLED);
    taskSendStatus.enable();
    taskBlinkLED.enable();
    
    Serial.printf("Multi-function node started. ID: %u\n", mesh.getNodeId());
}

void loop() {
    mesh.update();
}
```

**Key Concepts:**
- Handle multiple message types in one node
- Implement command/response patterns
- Monitor network health and offer assistance
- Combine mesh communication with hardware control

## Example 5: Network Discovery and Mapping

Discover and map the entire mesh network topology.

```cpp
#include "painlessMesh.h"
#include "ArduinoJson.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "secretPassword"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

std::map<uint32_t, std::set<uint32_t>> networkTopology;
std::map<uint32_t, String> nodeInfo;

// Request topology information every 60 seconds
Task taskDiscoverTopology(60000, TASK_FOREVER, [](){
    DynamicJsonDocument doc(100);
    
    doc["type"] = "topology_request";
    doc["requesterId"] = mesh.getNodeId();
    doc["timestamp"] = mesh.getNodeTime();
    
    String message;
    serializeJson(doc, message);
    
    mesh.sendBroadcast(message);
    Serial.println("Sent topology discovery request");
});

// Print network map every 90 seconds
Task taskPrintNetworkMap(90000, TASK_FOREVER, [](){
    printNetworkTopology();
});

void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) return;
    
    String msgType = doc["type"];
    
    if (msgType == "topology_request") {
        sendTopologyResponse(from);
    }
    else if (msgType == "topology_response") {
        processTopologyResponse(from, doc);
    }
}

void sendTopologyResponse(uint32_t requester) {
    DynamicJsonDocument doc(512);
    
    doc["type"] = "topology_response";
    doc["nodeId"] = mesh.getNodeId();
    doc["requester"] = requester;
    
    // Include my connections
    JsonArray connections = doc.createNestedArray("connections");
    auto nodeList = mesh.getNodeList();
    for (uint32_t nodeId : nodeList) {
        connections.add(nodeId);
    }
    
    // Include node information
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["version"] = "1.0";
    
    String message;
    serializeJson(doc, message);
    
    mesh.sendSingle(requester, message);
    Serial.printf("Sent topology response to %u\n", requester);
}

void processTopologyResponse(uint32_t from, JsonDocument& doc) {
    Serial.printf("Received topology from %u\n", from);
    
    // Store node information
    int uptime = doc["uptime"];
    int freeHeap = doc["freeHeap"];
    String version = doc["version"];
    
    char info[100];
    snprintf(info, sizeof(info), "uptime:%ds heap:%d ver:%s", 
             uptime, freeHeap, version.c_str());
    nodeInfo[from] = String(info);
    
    // Store connections
    networkTopology[from].clear();
    JsonArray connections = doc["connections"];
    for (JsonVariant connection : connections) {
        uint32_t connectedNode = connection.as<uint32_t>();
        networkTopology[from].insert(connectedNode);
    }
    
    Serial.printf("Node %u has %d connections\n", from, connections.size());
}

void printNetworkTopology() {
    Serial.println("=== Network Topology ===");
    
    if (networkTopology.empty()) {
        Serial.println("No topology data available");
        return;
    }
    
    // Print each node and its connections
    for (auto& [nodeId, connections] : networkTopology) {
        Serial.printf("Node %u (%s)\n", nodeId, 
                     nodeInfo.count(nodeId) ? nodeInfo[nodeId].c_str() : "no info");
        
        if (connections.empty()) {
            Serial.println("  No connections");
        } else {
            Serial.print("  Connected to: ");
            for (uint32_t connectedNode : connections) {
                Serial.printf("%u ", connectedNode);
            }
            Serial.println();
        }
    }
    
    // Calculate network statistics
    int totalNodes = networkTopology.size() + 1; // +1 for this node  
    int totalConnections = 0;
    for (auto& [nodeId, connections] : networkTopology) {
        totalConnections += connections.size();
    }
    totalConnections += mesh.getNodeList().size(); // This node's connections
    totalConnections /= 2; // Each connection counted twice
    
    Serial.printf("Network Summary: %d nodes, %d connections\n", 
                 totalNodes, totalConnections);
    Serial.println("========================");
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
    
    // Immediate topology request for new node
    DynamicJsonDocument doc(100);
    doc["type"] = "topology_request";
    doc["requesterId"] = mesh.getNodeId();
    doc["timestamp"] = mesh.getNodeTime();
    
    String message;
    serializeJson(doc, message);
    mesh.sendSingle(nodeId, message);
}

void droppedConnectionCallback(uint32_t nodeId) {
    Serial.printf("Lost connection: %u\n", nodeId);
    
    // Remove from topology map
    networkTopology.erase(nodeId);
    nodeInfo.erase(nodeId);
    
    // Also remove from other nodes' connection lists
    for (auto& [otherNode, connections] : networkTopology) {
        connections.erase(nodeId);
    }
}

void setup() {
    Serial.begin(115200);
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onDroppedConnection(&droppedConnectionCallback);
    
    userScheduler.addTask(taskDiscoverTopology);
    userScheduler.addTask(taskPrintNetworkMap);
    taskDiscoverTopology.enable();
    taskPrintNetworkMap.enable();
    
    Serial.printf("Network discovery node started. ID: %u\n", mesh.getNodeId());
}

void loop() {
    mesh.update();
}
```

**Key Concepts:**
- Actively discover network topology
- Store and analyze network structure
- Track node information and capabilities
- Visualize mesh connectivity

## Common Patterns and Best Practices

### Error Handling
```cpp
void receivedCallback(uint32_t from, String& msg) {
    // Always validate JSON parsing
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) {
        Serial.printf("JSON error from %u: %s\n", from, error.c_str());
        return; // Don't process invalid JSON
    }
    
    // Validate required fields exist
    if (!doc.containsKey("type")) {
        Serial.printf("Missing 'type' field from %u\n", from);
        return;
    }
    
    // Process message...
}
```

### Memory Management
```cpp
void sendLargeMessage() {
    // Use appropriate document size
    DynamicJsonDocument doc(1024); // Size for your data
    
    // ... populate document
    
    String message;
    size_t messageSize = serializeJson(doc, message);
    
    if (message.length() > 1000) {
        Serial.println("Warning: Large message may cause issues");
    }
    
    mesh.sendBroadcast(message);
}
```

### Task Management
```cpp
void setup() {
    // Create tasks but don't enable immediately
    Task task1(5000, TASK_FOREVER, &function1);
    Task task2(10000, TASK_FOREVER, &function2);
    
    userScheduler.addTask(task1);
    userScheduler.addTask(task2);
    
    // Enable tasks after mesh is initialized
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    
    task1.enable();
    task2.enable();
}
```

## Next Steps

These examples provide a foundation for building more complex mesh applications. Consider exploring:

- [Custom Packages Tutorial](custom-packages.md) for type-safe messaging
- [Sensor Networks Tutorial](sensor-networks.md) for IoT applications  
- [Alteriom Extensions](../alteriom/overview.md) for production-ready packages
- [Performance Optimization](../advanced/performance.md) for scaling up

Each example can be extended with additional features like data persistence, external connectivity, or advanced routing strategies based on your specific needs.