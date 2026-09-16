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

### Reading the reply: the whole result

`success` means HTTP 200, 201, 202 or 204 -- what HTTP calls a success. It
does not mean the service did what you wanted: CallMeBot, for one, answers
"Too many requests" under HTTP 201. Pass a callback that takes an
`InternetResult` to get the start of the response body as well, and decide
in your sketch:

```cpp
#include "callmebot.h"  // from this example

mesh.sendToInternet(url, "", [](const painlessmesh::InternetResult& result) {
  // result.messageId, result.success, result.httpStatus, result.error,
  // result.response (the body, as one line), result.retryable, result.attempts
  const auto reply = callmebot::judge(result.httpStatus, result.response);
  if (reply.accepted) {
    Serial.println("CallMeBot queued the message");
  } else {
    Serial.printf("Not sent: %s -- CallMeBot said: %s\n", reply.meaning,
                  result.response.c_str());
  }
});
```

`callmebot.h` is part of the example, not the library: it knows CallMeBot's
wording ("Message queued", "Too many requests", "Your Account is Paused",
"APIKey is invalid")
and that its HTTP 208 has not meant a delivery. The desktop test suite checks
it against the CallMeBot-shaped test point in `test/mock-http-server/`.

### Retries and duplicates

The library issues each request **once**, and retries only when a retry
cannot deliver it twice:

| What happened | Retried? |
|---|---|
| Connection refused, or a send failure before the request went out | yes |
| Gateway without Internet, a host that does not resolve | no -- a retry within seconds fails the same way |
| HTTP 429 or 503 (the server did not take it) | yes, not before `Retry-After` |
| Read timeout, connection lost after sending | **no** -- the server may have it |
| Any other status, including every 2xx and 500 | no |

A request that was not retried reports `retryable == false`; a transport error
says "(the request may have reached the server; not retried)". Resending is
then your call, knowing it may arrive twice. Every attempt at one call carries
the same `X-Request-Id` and `Idempotency-Key` header, so a service that honours
idempotency keys drops the copy, and a log can tell a retry from a second send.

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
3. Print `result.response` in the callback: CallMeBot says why in the body,
   and HTTP 200/201 alone does not mean it sent anything
4. URL-encode special characters in the message

### Gateway shows "no internet access" but WiFi is connected

**Symptom:** Error message "Router has no internet access - check WAN connection"

**Cause:** The gateway node is connected to WiFi but the router itself has no internet connectivity. This is detected through DNS resolution testing.

**Solutions:**
1. **Check router WAN connection:**
   - Verify router's internet LED indicator
   - Check router's WAN port cable connection
   - Ensure modem is powered and connected

2. **Check ISP service:**
   - Verify your internet service is active
   - Test internet on other devices connected to same router
   - Contact ISP if service is down

3. **Router configuration:**
   - Verify router has obtained WAN IP address
   - Check router's internet connection status page
   - Restart router if necessary

**Technical Details:**
The gateway performs two connectivity checks:
1. `WiFi.status() == WL_CONNECTED` - Verifies WiFi association
2. DNS resolution test - Verifies actual internet routing

If WiFi is connected but DNS fails, it indicates the router has no upstream internet connection. This prevents unnecessary HTTP request timeouts and provides early error detection.

### Understanding HTTP Status Codes

The callback provides `httpStatus` to indicate the result:

**SUCCESS (success = true):**
- `200 OK` - Standard success (most common for WhatsApp API)
- `201 Created` - Resource successfully created
- `202 Accepted` - Request accepted for processing
- `204 No Content` - Successful with no response body

**FAILURE (success = false):**
- `203 Non-Authoritative Information` - **Cached/proxied response, NOT actual delivery**

⚠️ **The status alone does not say whether WhatsApp got the message.** CallMeBot answers a refusal (for example "Too many requests") with HTTP 201 or HTTP 203, the same error page under both, and it has answered HTTP 208 to messages that never arrived (issues #450 and #452). The library reports HTTP's meaning -- 201 is a success, 203 and 208 are not -- and hands your callback the start of the body in `result.response`. The sketch's `callmebot::judge()` reads that body: `✅ WhatsApp message queued by CallMeBot` only when CallMeBot said "Message queued", and otherwise `❌ WhatsApp message not sent` followed by what CallMeBot said.

**The sketch paces its alerts.** CallMeBot is free and rate-limited. The sketch sends one WhatsApp when O2 falls below the threshold, not one per reading, never more than one every 10 minutes per node, and waits 15 minutes after "Too many requests" or a paused account. Several nodes with the same API key share CallMeBot's limit; raise `ALERT_MIN_INTERVAL_MS` accordingly.

Also replace `CLOUD_URL` at the top of the sketch: the placeholder `api.example.com` does not resolve, and until you change it the sketch skips the cloud send and says so instead of reporting `connection refused` every minute.

## Files

- `sendToInternet.ino` - Main example sketch for ESP32/ESP8266
- `mock_server_test/mock_server_test.ino` - Bridge testing with mock HTTP server
- `pc_node/pc_mesh_node.cpp` - **NEW:** PC-based mesh node for testing regular node → bridge flow
- `README.md` - This documentation
- `pc_node/PC_NODE_README.md` - **NEW:** Documentation for PC mesh node testing

### Related examples

- [`mock_server_test/`](mock_server_test/) - Bridge testing against the mock
  HTTP server. It lives in its own sketch folder because the Arduino toolchain
  merges every `.ino` in a folder into one translation unit, so two sketches
  side by side collide on `setup()`/`loop()`.

## Testing from Regular Nodes

### PC Mesh Node Emulator (NEW!)

Want to test `sendToInternet()` from a **regular mesh node** (not a bridge) without needing multiple ESP devices?

The **PC Mesh Node** allows you to:
- ✅ Run a mesh node on Windows/Linux/macOS
- ✅ Test the complete flow: PC Node → Bridge → Internet
- ✅ Debug with full PC development tools
- ✅ Fast iteration (no upload times!)

**Quick Start:**
```bash
cd examples/sendToInternet/pc_node

# Build
cmake . && make

# Run (connect to your ESP bridge)
./pc_mesh_node 192.168.1.100 5555
```

**Full Documentation:** [pc_node/PC_NODE_README.md](pc_node/PC_NODE_README.md)

**What It Tests:**
- Regular mesh node sending HTTP requests through bridge
- Request routing through mesh network
- Response callback handling
- Multiple HTTP status codes (200, 404, 500, etc.)
- JSON payload transmission

**Addresses Issue #337:** This solution provides the "bridge emulator" or mesh node emulator requested by @woodlist for testing node-to-internet traffic through the bridge.

## Related Examples

- [sharedGateway](../sharedGateway/) - All nodes with direct Internet access
- [bridge_failover](../bridge_failover/) - Automatic gateway failover
- [mqttBridge](../mqttBridge/) - MQTT integration
- [Mock HTTP Server](../../test/mock-http-server/) - Local testing endpoint
