# Your First Mesh Network

This tutorial builds on the [Quick Start Guide](quickstart.md) to create a more comprehensive mesh network with multiple types of nodes and practical functionality.

## Overview

We'll build a mesh network with three types of nodes:
1. **Sensor Node** - Collects and broadcasts environmental data
2. **Controller Node** - Receives sensor data and controls devices
3. **Bridge Node** - Connects mesh to external networks (WiFi/Internet)

## Node 1: Sensor Node

This node simulates environmental sensors and broadcasts data to the mesh.

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Sensor simulation
float temperature = 22.5;
float humidity = 65.0;
uint32_t sensorId = 1001;

// Task to send sensor data every 30 seconds
Task taskSendSensor(30000, TASK_FOREVER, [](){
    // Simulate sensor readings with some variation
    temperature += random(-10, 10) / 10.0;
    humidity += random(-50, 50) / 10.0;
    
    // Keep values in reasonable ranges
    temperature = constrain(temperature, 15.0, 35.0);
    humidity = constrain(humidity, 30.0, 90.0);
    
    // Create JSON message
    String msg = "{";
    msg += "\"type\":\"sensor\",";
    msg += "\"nodeId\":" + String(mesh.getNodeId()) + ",";
    msg += "\"sensorId\":" + String(sensorId) + ",";
    msg += "\"temperature\":" + String(temperature, 1) + ",";
    msg += "\"humidity\":" + String(humidity, 1) + ",";
    msg += "\"timestamp\":" + String(mesh.getNodeTime());
    msg += "}";
    
    mesh.sendBroadcast(msg);
    Serial.printf("Sent sensor data: T=%.1f°C, H=%.1f%%\n", temperature, humidity);
});

void receivedCallback(uint32_t from, String &msg) {
    Serial.printf("Sensor Node: Received from %u: %s\n", from, msg.c_str());
    
    // Sensor nodes can respond to commands
    if (msg.indexOf("\"command\":\"read_sensor\"") > 0) {
        // Force immediate sensor reading
        taskSendSensor.forceNextIteration();
        Serial.println("Forced sensor reading requested");
    }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("Sensor Node: New connection to %u\n", nodeId);
}

void changedConnectionCallback() {
    Serial.printf("Sensor Node: Changed connections. Nodes: %s\n", 
                  mesh.subConnectionJson().c_str());
}

void setup() {
    Serial.begin(115200);
    Serial.println("=== Sensor Node Starting ===");
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    
    userScheduler.addTask(taskSendSensor);
    taskSendSensor.enable();
    
    Serial.printf("Sensor Node initialized. Node ID: %u\n", mesh.getNodeId());
}

void loop() {
    mesh.update();
}
```

## Node 2: Controller Node

This node receives sensor data and can send commands to other nodes.

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Storage for sensor data
struct SensorData {
    uint32_t nodeId;
    uint32_t sensorId;
    float temperature;
    float humidity;
    uint32_t timestamp;
    bool valid;
};

SensorData sensors[10]; // Store data from up to 10 sensors
int sensorCount = 0;

// Task to request sensor data every 60 seconds
Task taskRequestData(60000, TASK_FOREVER, [](){
    String cmd = "{";
    cmd += "\"type\":\"command\",";
    cmd += "\"command\":\"read_sensor\",";
    cmd += "\"from\":" + String(mesh.getNodeId());
    cmd += "}";
    
    mesh.sendBroadcast(cmd);
    Serial.println("Requested sensor data from all nodes");
});

// Task to display collected data every 45 seconds
Task taskDisplayData(45000, TASK_FOREVER, [](){
    Serial.println("=== Collected Sensor Data ===");
    for (int i = 0; i < sensorCount; i++) {
        if (sensors[i].valid) {
            Serial.printf("Node %u (Sensor %u): T=%.1f°C, H=%.1f%%, Age=%u seconds\n",
                         sensors[i].nodeId, sensors[i].sensorId,
                         sensors[i].temperature, sensors[i].humidity,
                         (mesh.getNodeTime() - sensors[i].timestamp) / 1000000);
        }
    }
    Serial.println("============================");
});

void storeSensorData(uint32_t nodeId, uint32_t sensorId, float temp, float hum, uint32_t timestamp) {
    // Find existing entry or create new one
    int index = -1;
    for (int i = 0; i < sensorCount; i++) {
        if (sensors[i].nodeId == nodeId && sensors[i].sensorId == sensorId) {
            index = i;
            break;
        }
    }
    
    if (index == -1 && sensorCount < 10) {
        index = sensorCount++;
    }
    
    if (index != -1) {
        sensors[index].nodeId = nodeId;
        sensors[index].sensorId = sensorId;
        sensors[index].temperature = temp;
        sensors[index].humidity = hum;
        sensors[index].timestamp = timestamp;
        sensors[index].valid = true;
    }
}

void receivedCallback(uint32_t from, String &msg) {
    Serial.printf("Controller: Received from %u: %s\n", from, msg.c_str());
    
    // Parse sensor data
    if (msg.indexOf("\"type\":\"sensor\"") > 0) {
        // Simple JSON parsing (in production, use ArduinoJson library)
        int tempStart = msg.indexOf("\"temperature\":") + 14;
        int tempEnd = msg.indexOf(",", tempStart);
        float temperature = msg.substring(tempStart, tempEnd).toFloat();
        
        int humStart = msg.indexOf("\"humidity\":") + 11;
        int humEnd = msg.indexOf(",", humStart);
        float humidity = msg.substring(humStart, humEnd).toFloat();
        
        int sensorStart = msg.indexOf("\"sensorId\":") + 11;
        int sensorEnd = msg.indexOf(",", sensorStart);
        uint32_t sensorId = msg.substring(sensorStart, sensorEnd).toInt();
        
        int timeStart = msg.indexOf("\"timestamp\":") + 12;
        int timeEnd = msg.indexOf("}", timeStart);
        uint32_t timestamp = msg.substring(timeStart, timeEnd).toInt();
        
        storeSensorData(from, sensorId, temperature, humidity, timestamp);
        
        // Trigger actions based on sensor data
        if (temperature > 30.0) {
            Serial.println("WARNING: High temperature detected!");
            // Could send cooling command here
        }
        if (humidity > 80.0) {
            Serial.println("WARNING: High humidity detected!");
            // Could trigger ventilation here
        }
    }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("Controller: New connection to %u\n", nodeId);
}

void changedConnectionCallback() {
    Serial.printf("Controller: Changed connections. Nodes: %s\n", 
                  mesh.subConnectionJson().c_str());
}

void setup() {
    Serial.begin(115200);
    Serial.println("=== Controller Node Starting ===");
    
    // Initialize sensor storage
    for (int i = 0; i < 10; i++) {
        sensors[i].valid = false;
    }
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    
    userScheduler.addTask(taskRequestData);
    userScheduler.addTask(taskDisplayData);
    taskRequestData.enable();
    taskDisplayData.enable();
    
    Serial.printf("Controller Node initialized. Node ID: %u\n", mesh.getNodeId());
}

void loop() {
    mesh.update();
}
```

## Node 3: Bridge Node

This node connects the mesh to WiFi/Internet for external connectivity.

```cpp
#include "painlessMesh.h"
#include <WiFi.h>  // Use <ESP8266WiFi.h> for ESP8266

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

// External WiFi credentials
#define STATION_SSID     "YourHomeWiFi"
#define STATION_PASSWORD "YourWiFiPassword"

Scheduler userScheduler;
painlessMesh mesh;

// Task to report bridge status
Task taskBridgeStatus(30000, TASK_FOREVER, [](){
    String status = "{";
    status += "\"type\":\"bridge_status\",";
    status += "\"nodeId\":" + String(mesh.getNodeId()) + ",";
    status += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    status += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
    status += "\"mesh_connections\":" + String(mesh.getNodeList().size()) + ",";
    status += "\"timestamp\":" + String(mesh.getNodeTime());
    status += "}";
    
    mesh.sendBroadcast(status);
    Serial.printf("Bridge Status: WiFi=%s, RSSI=%d, Mesh Nodes=%d\n",
                  WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
                  WiFi.RSSI(), mesh.getNodeList().size());
});

void receivedCallback(uint32_t from, String &msg) {
    Serial.printf("Bridge: Received from %u: %s\n", from, msg.c_str());
    
    // Forward sensor data to external server (example)
    if (msg.indexOf("\"type\":\"sensor\"") > 0 && WiFi.status() == WL_CONNECTED) {
        // Here you could forward to HTTP/MQTT/etc
        Serial.println("Would forward sensor data to external server");
    }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("Bridge: New mesh connection to %u\n", nodeId);
}

void changedConnectionCallback() {
    Serial.printf("Bridge: Mesh topology changed. Nodes: %s\n", 
                  mesh.subConnectionJson().c_str());
}

void setup() {
    Serial.begin(115200);
    Serial.println("=== Bridge Node Starting ===");
    
    // Initialize mesh
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    
    // Connect to external WiFi
    WiFi.begin(STATION_SSID, STATION_PASSWORD);
    Serial.printf("Connecting to WiFi: %s", STATION_SSID);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    
    userScheduler.addTask(taskBridgeStatus);
    taskBridgeStatus.enable();
    
    Serial.printf("Bridge Node initialized. Node ID: %u\n", mesh.getNodeId());
}

void loop() {
    mesh.update();
}
```

## Deployment Steps

1. **Upload Sensor Node code** to your first ESP32/ESP8266
2. **Upload Controller Node code** to your second device  
3. **Upload Bridge Node code** to your third device (update WiFi credentials first!)
4. **Power up all devices** and open Serial Monitors to watch them connect
5. **Observe the mesh in action**:
   - Sensor nodes broadcast environmental data
   - Controller receives and processes sensor data
   - Bridge provides external connectivity and status updates

## What You'll See

### Sensor Node Output
```
=== Sensor Node Starting ===
Sensor Node initialized. Node ID: 123456789
Sensor Node: New connection to 987654321
Sent sensor data: T=23.2°C, H=67.4%
```

### Controller Node Output
```
=== Controller Node Starting ===
Controller Node initialized. Node ID: 987654321
Requested sensor data from all nodes
Controller: Received from 123456789: {"type":"sensor",...}
=== Collected Sensor Data ===
Node 123456789 (Sensor 1001): T=23.2°C, H=67.4%, Age=5 seconds
```

### Bridge Node Output
```
=== Bridge Node Starting ===
Connecting to WiFi: YourHomeWiFi....
WiFi connected! IP: 192.168.1.100
Bridge Node initialized. Node ID: 555444333
Bridge Status: WiFi=Connected, RSSI=-45, Mesh Nodes=2
```

## Key Features Demonstrated

- **Automatic Discovery**: Nodes find each other automatically
- **Message Broadcasting**: Sensor data reaches all interested nodes
- **JSON Communication**: Structured data exchange
- **Command System**: Controller can request data from sensors
- **External Connectivity**: Bridge connects mesh to Internet
- **Self-Healing**: Network continues working if nodes disconnect

## Extending Your Mesh

### Add More Sensors
- Temperature/humidity sensors (DHT22)
- Motion detectors (PIR)
- Light sensors (photoresistor)
- Soil moisture sensors

### Add Actuators
- LED strips for notifications
- Servo motors for mechanical control
- Relays for switching devices
- Buzzers for alerts

### External Integrations
- MQTT broker connectivity
- HTTP REST API calls
- Home Assistant integration
- Cloud database logging

## Next Steps

- Explore [Custom Packages](../tutorials/custom-packages.md) for type-safe messaging
- Learn about [Alteriom Extensions](../alteriom/overview.md) for pre-built sensor packages
- Dive into [Advanced Topics](../advanced/performance.md) for optimization
- Check out [Troubleshooting](../troubleshooting/common-issues.md) if you encounter issues

Congratulations! You now have a working multi-node mesh network with real-world functionality.