# Internet Access in painlessMesh - Quick Reference

## Quick Answer

**Q: Why can't my mesh nodes access the internet?**

**A: Only the bridge node has internet access. Regular mesh nodes must forward data through the bridge.**

```text
Internet
   |
Router ← Your WiFi
   |
Bridge Node ← Only this node can access internet
   |
Mesh Network ← These nodes cannot access internet directly
 / | \
Node1 Node2 Node3
```

## Common Error Messages

If you see these errors on regular mesh nodes, you're trying to access the internet directly:

```
[HTTPS] GET... failed, error: connection refused
WiFi.status() != WL_CONNECTED
HTTP request timeout
Connection failed
```

## Quick Fix

### ❌ Wrong Approach (Doesn't Work)

```cpp
// Regular mesh node trying to access internet
HTTPClient http;
http.begin("http://api.example.com/data");
http.POST(sensorData);  // FAILS!
```

### ✅ Correct Approach (Works)

```cpp
// Regular mesh node sends to bridge
mesh.sendSingle(bridgeNodeId, sensorData);

// Bridge node forwards to internet
void receivedCallback(uint32_t from, String& msg) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://api.example.com/data");
        http.POST(msg);  // WORKS!
        http.end();
    }
}
```

## Architecture Patterns

### Pattern 1: Single Bridge (Basic)

One designated node is the bridge:

```cpp
// ==== BRIDGE NODE ====
mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                  ROUTER_SSID, ROUTER_PASSWORD,
                  &userScheduler, 5555);

// ==== SENSOR NODES ====
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, 5555);
```

### Pattern 2: Bridge Failover (High Availability)

Multiple nodes can become bridge automatically:

```cpp
// All nodes have router credentials
mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
mesh.enableBridgeFailover(true);

mesh.onBridgeRoleChanged(&bridgeRoleCallback);
```

### Pattern 3: Multi-Bridge (Load Balancing)

Multiple simultaneous bridges:

```cpp
// Primary bridge (priority 10)
mesh.initAsBridge(..., 10);

// Secondary bridge (priority 5)
mesh.initAsBridge(..., 5);
```

## By Use Case

### HTTP/HTTPS Requests

```cpp
// Bridge forwards HTTP requests
void receivedCallback(uint32_t from, String& msg) {
    HTTPClient http;
    http.begin("https://api.example.com/endpoint");
    http.addHeader("Content-Type", "application/json");
    http.POST(msg);
    http.end();
}
```

### MQTT Publishing

```cpp
// Bridge publishes to MQTT
#include "PubSubClient.h"
PubSubClient mqtt(wifiClient);

void receivedCallback(uint32_t from, String& msg) {
    String topic = "mesh/sensor/" + String(from);
    mqtt.publish(topic.c_str(), msg.c_str());
}
```

### WhatsApp Notifications

```cpp
// Bridge sends WhatsApp messages
#include "Callmebot_ESP32.h"
Callmebot_ESP32 whatsapp;

void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    
    if (doc["alarm"] == true) {
        String alert = "Alarm from sensor " + String(from);
        whatsapp.sendMessage(alert);
    }
}
```

### Cloud Services (AWS, Azure, GCP)

```cpp
// Bridge forwards to cloud
void receivedCallback(uint32_t from, String& msg) {
    // AWS IoT Core
    awsClient.publish("iot/sensor/data", msg);
    
    // Azure IoT Hub
    azureClient.sendEvent(msg);
    
    // Google Cloud IoT
    googleClient.publishTelemetry(msg);
}
```

### Email Notifications

```cpp
// Bridge sends email
#include "ESP_Mail_Client.h"

void receivedCallback(uint32_t from, String& msg) {
    SMTPData smtpData;
    smtpData.setLogin(SMTP_HOST, SMTP_PORT, EMAIL, PASSWORD);
    smtpData.setMessage("Sensor Alert", msg);
    MailClient.sendMail(smtpData);
}
```

## Technical Explanation

### Why Only Bridge Has Internet?

ESP8266/ESP32 WiFi hardware operates on a single channel:

**Bridge Node (WIFI_AP_STA):**
- Creates mesh Access Point (AP) on channel X
- Connects to router Station (STA) on channel X
- Both on same channel = Internet access ✅

**Regular Node (WIFI_AP):**
- Creates mesh Access Point (AP) on channel X
- No router connection
- No internet access ❌

### Can I Make All Nodes Bridges?

Technically yes, but **not recommended** because:

1. **Memory overhead**: +5-10KB RAM per node
2. **Performance degradation**: All compete for router
3. **Router limits**: Max clients (typically 10-32)
4. **Power consumption**: Extra WiFi connection
5. **Loses mesh benefits**: Defeats purpose of mesh

## Complete Examples

### Sensor Network with Cloud Upload

**Bridge:**
```cpp
#include "painlessMesh.h"
#include "HTTPClient.h"

painlessMesh mesh;

void setup() {
    mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                      ROUTER_SSID, ROUTER_PASSWORD,
                      &userScheduler, 5555);
    mesh.onReceive(&receivedCallback);
}

void receivedCallback(uint32_t from, String& msg) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("https://cloud.example.com/api/sensor");
        http.addHeader("Authorization", "Bearer " + API_KEY);
        http.addHeader("Content-Type", "application/json");
        
        int httpCode = http.POST(msg);
        if (httpCode == HTTP_CODE_OK) {
            Serial.println("Data uploaded to cloud");
        }
        http.end();
    }
}
```

**Sensor Node:**
```cpp
#include "painlessMesh.h"

painlessMesh mesh;
#define BRIDGE_NODE_ID 1234567890

void setup() {
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, 5555);
}

void loop() {
    mesh.update();
    sendSensorData();
}

void sendSensorData() {
    static unsigned long lastSend = 0;
    if (millis() - lastSend < 60000) return;
    lastSend = millis();
    
    String data = "{\"sensor\":\"temp\",\"value\":" + 
                  String(readTemperature()) + ",\"nodeId\":" + 
                  String(mesh.getNodeId()) + "}";
    
    mesh.sendSingle(BRIDGE_NODE_ID, data);
}
```

## Troubleshooting Checklist

- [ ] Verify which node is the bridge
- [ ] Check bridge has `initAsBridge()` or router credentials
- [ ] Confirm bridge shows `WiFi.status() == WL_CONNECTED`
- [ ] Verify regular nodes use `mesh.init()` without router
- [ ] Check sensor nodes send to bridge, not directly to internet
- [ ] Verify bridge forwards received messages to internet
- [ ] Test bridge internet connection with simple HTTP request
- [ ] Check serial output for connection errors
- [ ] Monitor memory usage on bridge node
- [ ] Verify firewall/router allows bridge's outbound connections

## More Information

- **[Common Architecture Mistakes](common-architecture-mistakes.md)** - Detailed guide
- **[BRIDGE_TO_INTERNET.md](../../BRIDGE_TO_INTERNET.md)** - Complete setup guide
- **[Bridge Failover](../BRIDGE_FAILOVER.md)** - High availability
- **[FAQ](faq.md)** - Common questions
- **[examples/mqttBridge/](../../examples/mqttBridge/)** - Working example

## Still Having Issues?

When asking for help, provide:

1. Which node is the bridge? (show setup code)
2. Which nodes are sensors? (show setup code)
3. Serial output from bridge (with debug enabled)
4. Serial output from sensor node (with debug enabled)
5. Complete error message
6. What you're trying to access (HTTP API, MQTT, etc.)

Post in:
- [GitHub Issues](https://github.com/Alteriom/painlessMesh/issues)
- [Community Forum](https://groups.google.com/forum/#!forum/painlessmesh-user)
