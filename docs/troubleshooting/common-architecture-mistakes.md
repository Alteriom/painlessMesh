# Common Architecture Mistakes

This document explains common misunderstandings about painlessMesh architecture and how to fix them.

## Mistake #1: Expecting Regular Nodes to Have Internet Access

### The Problem

**What users expect:**
```cpp
// Regular mesh node trying to make HTTP requests
void sendSensorData() {
    HTTPClient http;
    http.begin("http://api.example.com/sensor");
    http.POST("{\"temp\":25.5}");  // ❌ This fails!
    http.end();
}
```

**Error message:**
```
[HTTPS] GET... failed, error: connection refused
```

### Why This Fails

painlessMesh creates a **mesh network**, not a mesh-routed internet gateway. Only the bridge node connects to your WiFi router.

**Architecture:**
```text
Internet
   |
Router (WiFi - Channel 6)
   |
Bridge Node (WIFI_AP_STA mode)
   |            ← Only this node has internet!
   |
Mesh Network (Channel 6)
 / | \
Node1 Node2 Node3  ← These nodes do NOT have internet!
```

**Technical explanation:**

1. **Bridge node** (`WIFI_AP_STA` mode):
   - Acts as Access Point (AP) for mesh
   - Acts as Station (STA) connected to router
   - Has internet access via router connection
   - Can make HTTP/HTTPS requests

2. **Regular nodes** (`WIFI_AP` mode):
   - Only act as Access Points for mesh
   - No router connection
   - No internet access
   - Cannot make HTTP/HTTPS requests directly

ESP8266/ESP32 WiFi hardware can only operate on one channel at a time. Regular nodes use their WiFi radio to create the mesh AP - they cannot simultaneously connect to your router.

### The Solution

**Pattern 1: Forward through bridge (Recommended)**

```cpp
// ==== BRIDGE NODE ====
#include "painlessMesh.h"
#include "HTTPClient.h"

painlessMesh mesh;

void setup() {
    // Initialize as bridge with internet access
    mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                      ROUTER_SSID, ROUTER_PASSWORD,
                      &userScheduler, 5555);
    
    mesh.onReceive(&receivedCallback);
}

void receivedCallback(uint32_t from, String& msg) {
    // Parse message from mesh nodes
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    
    // Forward to internet service
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://api.example.com/sensor");
        http.addHeader("Content-Type", "application/json");
        int httpCode = http.POST(msg);
        
        if (httpCode > 0) {
            Serial.printf("Data forwarded to cloud: %d\n", httpCode);
        } else {
            Serial.printf("HTTP POST failed: %s\n", http.errorToString(httpCode).c_str());
        }
        
        http.end();
    } else {
        Serial.println("No internet connection - data not forwarded");
    }
}

// ==== REGULAR SENSOR NODE ====
#include "painlessMesh.h"

painlessMesh mesh;
uint32_t bridgeNodeId = 0;

void setup() {
    // Regular node - no router connection
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, 5555);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
}

void loop() {
    mesh.update();
    
    // Read sensor
    float temperature = readTemperatureSensor();
    
    // Send to bridge (NOT directly to internet!)
    if (bridgeNodeId != 0) {
        String msg = "{\"sensor\":\"temp\",\"value\":" + String(temperature) + "}";
        mesh.sendSingle(bridgeNodeId, msg);
        Serial.println("Data sent to bridge for forwarding");
    }
}

void newConnectionCallback(uint32_t nodeId) {
    // You could implement bridge discovery here
    // For now, configure bridge ID manually or via broadcast discovery
}
```

**Pattern 2: Bridge failover (High availability)**

```cpp
// All nodes configured with router credentials
mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
mesh.enableBridgeFailover(true);

mesh.onBridgeRoleChanged(&bridgeRoleCallback);

void bridgeRoleCallback(bool isBridge, String reason) {
    if (isBridge) {
        Serial.printf("I am now bridge: %s\n", reason.c_str());
        // This node now has internet access
        startForwardingToCloud();
    } else {
        Serial.println("I am a regular node - no internet access");
        // This node must forward through bridge
    }
}
```

See [BRIDGE_FAILOVER.md](../BRIDGE_FAILOVER.md) for details.

## Mistake #2: Thinking All Nodes Should Be Bridges

### The Problem

Some users try to make every node a bridge:

```cpp
// ❌ Bad idea: Making all nodes bridges
void setup() {
    // Every node connects to router
    mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                      ROUTER_SSID, ROUTER_PASSWORD,
                      &userScheduler, 5555);
}
```

### Why This Is Problematic

1. **Memory overhead**: Each router connection uses 5-10KB RAM
2. **Performance**: All nodes compete for router bandwidth
3. **Complexity**: Loses benefits of mesh architecture
4. **Channel conflicts**: All nodes must match router channel
5. **Connection limit**: Router has max client limit

### The Solution

**Use the bridge-forwarding pattern:**

- **1 bridge node**: Connects to router and mesh
- **N regular nodes**: Connect to mesh only, forward data to bridge
- **Bridge forwards**: Takes messages and forwards to internet

This is the intended architecture and scales much better.

## Mistake #3: Not Identifying the Bridge Node

### The Problem

Regular nodes send data but don't know which node is the bridge:

```cpp
// ❌ How do I know which node is the bridge?
void sendData() {
    String msg = "{\"data\":\"value\"}";
    mesh.sendBroadcast(msg);  // Wasteful - sends to everyone!
}
```

### The Solution

**Option 1: Bridge discovery via status broadcasts**

```cpp
// Bridge node broadcasts status
#include "examples/alteriom/alteriom_sensor_package.hpp"
using namespace alteriom;

// Bridge node
Task taskBridgeStatus(30000, TASK_FOREVER, []() {
    if (mesh.isRoot()) {
        BridgeStatusPackage status;
        status.from = mesh.getNodeId();
        status.internetConnected = (WiFi.status() == WL_CONNECTED);
        status.routerRSSI = WiFi.RSSI();
        
        mesh.sendBroadcast(status.toJsonString());
    }
});

// Regular node
uint32_t bridgeNodeId = 0;

void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    
    if (doc["type"] == 610) {  // BridgeStatusPackage
        bridgeNodeId = from;
        Serial.printf("Bridge node discovered: %u\n", bridgeNodeId);
    }
}

void sendDataToBridge() {
    if (bridgeNodeId != 0) {
        mesh.sendSingle(bridgeNodeId, myData);
    } else {
        Serial.println("Bridge not yet discovered");
    }
}
```

**Option 2: Hardcode bridge ID (simple approach)**

```cpp
// Configure bridge ID on all nodes
#define BRIDGE_NODE_ID 1234567890

void sendDataToBridge() {
    mesh.sendSingle(BRIDGE_NODE_ID, myData);
}
```

**Option 3: Use root node as bridge**

```cpp
void setup() {
    mesh.setRoot(true);  // Bridge should be root
    mesh.setContainsRoot(true);  // Tell all nodes
}

void sendDataToBridge() {
    // Send to root node (which is the bridge)
    auto nodeList = mesh.getNodeList();
    for (auto nodeId : nodeList) {
        // Check if this node is root
        // (painlessMesh automatically routes toward root)
    }
}
```

## Mistake #4: Using Wrong WiFi Mode

### The Problem

```cpp
// ❌ Regular node trying to use AP+STA mode
void setup() {
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, 5555, 
              WIFI_AP_STA);  // This is for bridges!
    
    // Then trying to connect to router
    WiFi.begin(ROUTER_SSID, ROUTER_PASSWORD);  // Conflicts with mesh!
}
```

### The Solution

**Bridge nodes:**
```cpp
// Use initAsBridge() which handles AP+STA mode correctly
mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                  ROUTER_SSID, ROUTER_PASSWORD,
                  &userScheduler, 5555);
```

**Regular nodes:**
```cpp
// Use default WIFI_AP mode (or let init() choose)
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, 5555);
// No router connection needed
```

## Mistake #5: Blocking Code on Regular Nodes Waiting for Internet

### The Problem

```cpp
// ❌ Regular node waiting forever for internet that never comes
void sendToCloud() {
    HTTPClient http;
    http.begin("http://api.example.com");
    http.setTimeout(30000);  // Waits 30 seconds
    http.POST(data);  // Will always timeout!
}
```

### The Solution

**Check your node role:**

```cpp
bool isBridgeNode = false;

void setup() {
    if (shouldBeBridge()) {
        mesh.initAsBridge(...);
        isBridgeNode = true;
    } else {
        mesh.init(...);
        isBridgeNode = false;
    }
}

void sendData() {
    if (isBridgeNode && WiFi.status() == WL_CONNECTED) {
        // Bridge can send directly to internet
        http.POST("http://api.example.com", data);
    } else {
        // Regular node forwards to bridge
        mesh.sendSingle(bridgeNodeId, data);
    }
}
```

## Real-World Example: WhatsApp Notifications

This is a real issue reported by a user trying to use the Callmebot-ESP32 library with painlessMesh.

### Original (Incorrect) Approach

```cpp
// ❌ Regular mesh node trying to send WhatsApp messages
#include "Callmebot_ESP32.h"

Callmebot_ESP32 whatsapp;

void sendAlarm() {
    // This fails on regular nodes: "connection refused"
    whatsapp.sendMessage("Sensor alarm!");  // ❌ No internet!
}
```

### Corrected Approach

```cpp
// ==== BRIDGE NODE ====
#include "painlessMesh.h"
#include "Callmebot_ESP32.h"

Callmebot_ESP32 whatsapp;

void setup() {
    // Bridge has internet access
    mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                      ROUTER_SSID, ROUTER_PASSWORD,
                      &userScheduler, 5555);
    
    whatsapp.begin(PHONE_NUMBER, API_KEY);
    mesh.onReceive(&receivedCallback);
}

void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    
    // Check for alarm messages from mesh nodes
    if (doc["alarm"] == true) {
        String alertMsg = "ALARM from sensor " + String(from);
        alertMsg += ": " + doc["message"].as<String>();
        
        // Bridge can send WhatsApp messages
        if (WiFi.status() == WL_CONNECTED) {
            whatsapp.sendMessage(alertMsg);
            Serial.println("WhatsApp alert sent!");
        }
    }
}

// ==== SENSOR NODE ====
void checkSensorAndAlert() {
    float oxygenLevel = readO2Sensor();
    
    if (oxygenLevel < CRITICAL_THRESHOLD) {
        // Send alarm to bridge (not directly to WhatsApp!)
        String alarm = "{\"alarm\":true,\"sensor\":\"O2\",";
        alarm += "\"value\":" + String(oxygenLevel) + ",";
        alarm += "\"message\":\"Critical O2 level!\"}";
        
        mesh.sendSingle(bridgeNodeId, alarm);
        Serial.println("Alarm sent to bridge for WhatsApp notification");
    }
}
```

## Summary: Architecture Best Practices

1. ✅ **One bridge node**: Connects to router and mesh, has internet access
2. ✅ **Regular nodes**: Connect to mesh only, NO internet access
3. ✅ **Forward pattern**: Regular nodes send data to bridge, bridge forwards to internet
4. ✅ **Bridge discovery**: Implement mechanism for nodes to find the bridge
5. ✅ **Error handling**: Bridge checks internet connectivity before forwarding
6. ✅ **Failover option**: Use bridge failover for high availability (v1.8.0+)

## Additional Resources

- [BRIDGE_TO_INTERNET.md](../../BRIDGE_TO_INTERNET.md) - Complete bridge setup guide
- [BRIDGE_FAILOVER.md](../BRIDGE_FAILOVER.md) - Automatic failover for reliability
- [examples/mqttBridge/](../../examples/mqttBridge/) - Working example of bridge pattern
- [Mesh Architecture](../architecture/mesh-architecture.md) - Understanding mesh design
- [FAQ](faq.md) - Common questions and answers
