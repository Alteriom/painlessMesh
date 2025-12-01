# sendToInternet Example - WhatsApp/Callmebot Integration

This example demonstrates how to use `mesh.sendToInternet()` to send data to Internet endpoints (like WhatsApp via Callmebot) from **any node** in the mesh network.

## ⚠️ Important: Understanding sendToInternet()

Regular mesh nodes do **NOT** have direct IP routing to the Internet. The `sendToInternet()` API routes your data **through a gateway node** that has Internet access.

```
┌─────────────┐                    ┌─────────────┐                    ┌─────────────┐
│ Sensor Node │ ──sendToInternet──▶│   Gateway   │ ──HTTP Request──▶  │  Internet   │
│  (no WiFi)  │                    │ (has WiFi)  │                    │  (Callmebot)│
└─────────────┘                    └─────────────┘                    └─────────────┘
       ▲                                  │                                  │
       │                                  │                                  │
       └──────────── ACK/Result ◀─────────┴──────────── HTTP Response ◀──────┘
```

**This is different from making direct HTTP requests** (which would fail on regular mesh nodes with "connection refused").

## Use Cases

- 🐟 **Fish farm sensors** sending O2 alarms to WhatsApp
- 🏭 **Industrial IoT** sending alerts to cloud APIs
- 🏠 **Smart home sensors** reporting to home automation servers
- 📊 **Remote monitoring** sending data to ThingsBoard, AWS IoT, etc.

## Prerequisites

1. **At least one node must be a bridge/gateway** with Internet access
2. **OR** use `initAsSharedGateway()` so all nodes have Internet
3. Enable the API after mesh init: `mesh.enableSendToInternet()`

## Setup for Callmebot WhatsApp

1. Get your API key from: https://www.callmebot.com/blog/free-api-whatsapp-messages/
2. Update the configuration in the sketch:

```cpp
#define WHATSAPP_PHONE  "+1234567890"    // Your phone with country code
#define WHATSAPP_APIKEY "your_api_key"   // Your Callmebot API key
```

## API Usage

### Basic Usage

```cpp
// Check if Internet is available via any gateway
if (mesh.hasInternetConnection()) {
  
  // Send data to Internet - routed through gateway automatically
  uint32_t msgId = mesh.sendToInternet(
    "https://api.callmebot.com/whatsapp.php?phone=+1234567890&apikey=KEY&text=Hello",
    "",  // Empty payload for GET (params in URL)
    [](bool success, uint16_t httpStatus, String error) {
      if (success) {
        Serial.printf("✅ Sent! HTTP: %d\n", httpStatus);
      } else {
        Serial.printf("❌ Failed: %s\n", error.c_str());
      }
    }
  );
}
```

### Sending JSON to REST API

```cpp
String payload = "{\"temperature\": 25.5, \"humidity\": 60}";

mesh.sendToInternet(
  "https://api.yourserver.com/sensors",
  payload,
  [](bool success, uint16_t httpStatus, String error) {
    Serial.printf("Result: %s, HTTP: %d\n", 
                  success ? "OK" : error.c_str(), httpStatus);
  },
  static_cast<uint8_t>(painlessmesh::gateway::GatewayPriority::PRIORITY_HIGH)
);
```

### Priority Levels

| Value | Enum | Use Case |
|-------|------|----------|
| 0 | `PRIORITY_CRITICAL` | Alarms, emergencies |
| 1 | `PRIORITY_HIGH` | Important alerts |
| 2 | `PRIORITY_NORMAL` | Regular sensor data |
| 3 | `PRIORITY_LOW` | Bulk/background data |

## Hardware Setup

### Option A: Bridge + Sensor Nodes

**Bridge Node** (has Internet):
```cpp
#define IS_BRIDGE_NODE true
// Update ROUTER_SSID and ROUTER_PASSWORD
```

**Sensor Nodes** (no direct Internet):
```cpp
#define IS_BRIDGE_NODE false
```

### Option B: Shared Gateway Mode

All nodes connect to the same router:
```cpp
mesh.initAsSharedGateway(
  MESH_PREFIX, MESH_PASSWORD,
  ROUTER_SSID, ROUTER_PASSWORD,  // Router credentials required!
  &userScheduler, MESH_PORT
);
mesh.enableSendToInternet();
```

## Common Issues

### "No Internet available - no gateway with Internet found"

- Make sure at least one node is initialized as a bridge with router credentials
- Check that the bridge has successfully connected to the router
- Use `mesh.hasInternetConnection()` to verify gateway availability

### "connection refused" when using HTTPClient directly

**DON'T do this on regular mesh nodes:**
```cpp
// This FAILS on regular mesh nodes!
HTTPClient http;
http.begin("https://api.callmebot.com/...");
```

**DO this instead:**
```cpp
// This works - routes through gateway
mesh.sendToInternet("https://api.callmebot.com/...", "", callback);
```

### WhatsApp message not received

1. Verify your Callmebot API key is correct
2. Ensure phone number includes country code (e.g., `+1234567890`)
3. Check HTTP status code in callback (200 = success)
4. URL-encode special characters in the message

## Files

- `sendToInternet.ino` - Main example sketch
- `README.md` - This documentation

## Related Examples

- [sharedGateway](../sharedGateway/) - All nodes with direct Internet access
- [bridge_failover](../bridge_failover/) - Automatic gateway failover
- [mqttBridge](../mqttBridge/) - MQTT integration
