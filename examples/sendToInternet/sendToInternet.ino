//************************************************************
// sendToInternet Example - WhatsApp/Callmebot Integration
//
// This example demonstrates how to use mesh.sendToInternet() to send
// messages to Internet endpoints (like WhatsApp via Callmebot) from
// ANY node in the mesh network - not just the bridge/gateway.
//
// IMPORTANT:
// Regular mesh nodes do NOT have direct IP routing to the Internet.
// The sendToInternet() API routes your data THROUGH a gateway node
// that has Internet access. This is different from making direct
// HTTP requests (which would fail on regular mesh nodes).
//
// How it works:
// 1. Node calls mesh.sendToInternet() with destination URL and payload
// 2. The mesh routes the request to a gateway node with Internet
// 3. Gateway makes the actual HTTP request to the destination
// 4. Gateway sends acknowledgment back through the mesh
// 5. Your callback is invoked with the result
//
// Use Cases:
// - Fish farm sensors sending O2 alarms to WhatsApp
// - Industrial IoT sending alerts to cloud APIs
// - Smart home sensors reporting to home automation servers
//
// Prerequisites:
//
// 1. GATEWAY SETUP (Choose one approach):
//
//    Option A - Dedicated Bridge (Recommended):
//    - Use initAsBridge() on ONE node (see examples/bridge/bridge.ino)
//    - The bridge node automatically handles Internet routing
//
//    Option B - Shared Gateway (All nodes have router access):
//    - Use initAsSharedGateway() on ALL nodes (see examples/sharedGateway/sharedGateway.ino)
//    - Requires ROUTER_SSID and ROUTER_PASSWORD on every node
//    - All nodes connect directly to the router for Internet access
//
//    Option C - Failover Bridge (High Availability):
//    - Use bridge_failover example unchanged (see examples/bridge_failover/bridge_failover.ino)
//    - Automatically elects backup bridges if primary fails
//    - Works as-is without any modifications needed!
//
// 2. SENDING NODE SETUP:
//    - Call mesh.enableSendToInternet() AFTER mesh.init() on nodes that SEND requests.
//    - Bridge nodes automatically handle routing via initAsBridge().
//    - This example shows how to enable it in the setup() function below.
//
// For Callmebot WhatsApp API:
// - Get your API key from https://www.callmebot.com/blog/free-api-whatsapp-messages/
// - Format: https://api.callmebot.com/whatsapp.php?phone=PHONE&apikey=KEY&text=MESSAGE
// - CallMeBot is a free service with a rate limit, and it does not say in
//   the HTTP status whether it will deliver: callmebot.h reads its reply.
//   Send it alerts, not a message every minute.
//
// What painlessMesh does and does not do for you:
// - It issues each request once. It retries only when the request cannot
//   have reached the server (the connection was refused, say) or the server
//   said to come back (HTTP 429/503, honouring Retry-After). A timeout after
//   the request was sent is NOT retried: resending could deliver it twice.
// - Every attempt carries the same X-Request-Id / Idempotency-Key header.
// - The callback gets the HTTP status and the start of the response body.
//   Whether that reply means "delivered" is the service's language, so the
//   sketch decides -- here with callmebot::judge().
//
//************************************************************
#include "painlessMesh.h"

#include "callmebot.h"

// ============================================
// Mesh Network Configuration
// ============================================
#define MESH_PREFIX     "SensorMesh"
#define MESH_PASSWORD   "meshPassword123"
#define MESH_PORT       5555

// ============================================
// Router Configuration (for bridge/gateway)
// ============================================
#define ROUTER_SSID     "YourRouterSSID"
#define ROUTER_PASSWORD "YourRouterPassword"

// ============================================
// Callmebot/WhatsApp Configuration
// ============================================
// Get your API key from: https://www.callmebot.com/blog/free-api-whatsapp-messages/
#define WHATSAPP_PHONE  "+1234567890"    // Your phone number with country code
#define WHATSAPP_APIKEY "your_api_key"   // Your Callmebot API key

// ============================================
// Cloud API Configuration
// ============================================
// The endpoint the periodic sensor readings are POSTed to. Replace it with
// your own; the placeholder below does not resolve, so until you do the
// sketch skips the cloud send and says so rather than reporting
// "connection refused" every minute (issue #450).
// Example endpoints:
// - ThingsBoard: "https://demo.thingsboard.io/api/v1/YOUR_TOKEN/telemetry"
// - AWS IoT:     "https://YOUR_ENDPOINT.iot.us-east-1.amazonaws.com/topics/sensors"
// - Custom API:  "https://api.yourserver.com/sensors/data"
#define CLOUD_URL       "https://api.example.com/sensors"

// ============================================
// Sensor Simulation Configuration
// ============================================
// These define the ranges for simulated sensor values
#define TEMP_MIN        20.0   // Minimum temperature (°C)
#define TEMP_RANGE      10.0   // Temperature range (20-30°C)
#define HUMIDITY_MIN    40.0   // Minimum humidity (%)
#define HUMIDITY_RANGE  40.0   // Humidity range (40-80%)
#define O2_MIN          5.0    // Lowest simulated O2 level (mg/L)
#define O2_MAX          10.0   // Highest simulated O2 level (mg/L)
#define O2_ALARM_THRESHOLD 6.0 // O2 level below this raises the alarm
#define O2_ALARM_CLEAR     6.5 // ...and it clears only above this (hysteresis)

// ============================================
// Alert pacing
// ============================================
// One WhatsApp per alarm, not one per reading, and never more often than
// this per node. A reading every minute that alerted every time it was low
// is how a sketch runs into CallMeBot's rate limit.
#define ALERT_MIN_INTERVAL_MS (10UL * 60UL * 1000UL)

// ============================================
// Mode Selection
// ============================================
// Set to true to make this node a bridge with Internet access
// Set to false for regular mesh nodes that will send via the bridge
#define IS_BRIDGE_NODE  false

// ============================================
// Task Scheduler and Mesh Instance
// ============================================
Scheduler userScheduler;
painlessMesh mesh;

// ============================================
// Function Prototypes
// ============================================
bool sendAlertToWhatsApp(String message);
void sendSensorDataToCloud();
void receivedCallback(uint32_t from, String& msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
String urlEncode(const String& str);

// ============================================
// Task Definitions
// ============================================
// Task to periodically send sensor data (simulated)
Task taskSendSensorData(60000, TASK_FOREVER, &sendSensorDataToCloud);

// The startup WhatsApp used to be a single 30 s one-shot. A regular node that
// took longer than that to find the mesh -- following a bridge to another
// channel takes about a minute -- fired it with no gateway in sight and never
// tried again (issue #450). It now retries until a gateway with Internet is
// known, then disables itself.
void sendStartupAlert();
Task taskStartupAlert(30000, TASK_FOREVER, &sendStartupAlert);
String startupMsg;

// Alert state
bool alertInFlight = false;          // a WhatsApp request is on its way
uint32_t lastAlertMs = 0;            // when the last one was handed over
uint32_t alertHoldMs = 0;            // how long to wait after it
bool o2Alarm = false;                // the alarm is raised
float o2Level = 8.0;                 // simulated reading, a slow random walk

// ============================================
// URL Encoding Helper
// ============================================

/**
 * URL-encode a string for safe transmission in URLs
 * 
 * Encodes special characters to their percent-encoded equivalents.
 * This is required for WhatsApp messages containing special characters.
 * 
 * @param str The string to encode
 * @return URL-encoded string
 */
String urlEncode(const String& str) {
  String encoded = "";
  for (size_t i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      // Safe characters - no encoding needed
      encoded += c;
    } else if (c == ' ') {
      encoded += "%20";
    } else {
      // Encode other characters as %XX
      char hex[4];
      snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
      encoded += hex;
    }
  }
  return encoded;
}

// ============================================
// sendToInternet() Usage Example
// ============================================

/**
 * Send a WhatsApp message via Callmebot using sendToInternet()
 *
 * This function demonstrates how to use mesh.sendToInternet() to send
 * data to an Internet endpoint. The request is automatically routed
 * through a gateway node that has Internet access.
 *
 * @param message The message to send via WhatsApp
 * @return true when the request was handed to the mesh
 */
bool sendAlertToWhatsApp(String message) {
  // Check if Internet is available via any gateway
  if (!mesh.hasInternetConnection()) {
    Serial.println("❌ No Internet available - no gateway with Internet found");
    Serial.println("   Make sure at least one node is a bridge with router access");
    return false;
  }
  if (alertInFlight) {
    Serial.println("(a WhatsApp message is still on its way; not sending another)");
    return false;
  }
  if (lastAlertMs != 0 && millis() - lastAlertMs < alertHoldMs) {
    Serial.printf("(WhatsApp paused for another %lu s after the last message)\n",
                  (unsigned long)((alertHoldMs - (millis() - lastAlertMs)) / 1000));
    return false;
  }

  // URL-encode the message for safe transmission
  String encodedMessage = urlEncode(message);

  // Build the Callmebot WhatsApp API URL
  // Format: https://api.callmebot.com/whatsapp.php?phone=PHONE&apikey=KEY&text=MESSAGE
  String url = "https://api.callmebot.com/whatsapp.php";
  url += "?phone=" + urlEncode(WHATSAPP_PHONE);
  url += "&apikey=" + String(WHATSAPP_APIKEY);
  url += "&text=" + encodedMessage;

  Serial.println("\n📱 Sending WhatsApp message via sendToInternet()...");
  Serial.printf("   Message: %s\n", message.c_str());
  // Logs end up pasted into bug reports; the key stays out of them.
  Serial.printf("   URL: %s\n", callmebot::redactApiKey(url).c_str());

  // Use sendToInternet() to route the request through a gateway.
  //
  // This callback receives the whole InternetResult: the HTTP status, the
  // start of CallMeBot's reply, whether the library could retry it and how
  // many requests it took. painlessMesh says what HTTP says; callmebot::judge()
  // says what CallMeBot meant -- a "Too many requests" page can arrive under
  // HTTP 201, and HTTP 208 has meant "not delivered".
  alertInFlight = true;
  lastAlertMs = millis();
  alertHoldMs = ALERT_MIN_INTERVAL_MS;
  uint32_t msgId = mesh.sendToInternet(
    url,
    "",  // No payload needed for GET request - params are in URL
    [](const painlessmesh::InternetResult& result) {
      alertInFlight = false;
      const auto reply = callmebot::judge(result.httpStatus, result.response);
      if (reply.accepted) {
        Serial.printf("✅ WhatsApp message queued by CallMeBot (request %u, HTTP %u, %u attempt(s))\n",
                      result.messageId, result.httpStatus, result.attempts);
      } else {
        Serial.printf("❌ WhatsApp message not sent (request %u): %s\n",
                      result.messageId, reply.meaning);
        Serial.printf("   HTTP %u after %u attempt(s)\n", result.httpStatus, result.attempts);
        if (result.error.length() > 0) {
          Serial.printf("   Error: %s\n", result.error.c_str());
        }
        if (result.response.length() > 0) {
          Serial.printf("   CallMeBot said: %s\n", result.response.c_str());
        }
      }
      if (reply.holdOffMs > alertHoldMs) {
        alertHoldMs = reply.holdOffMs;
      }
    },
    static_cast<uint8_t>(painlessmesh::gateway::GatewayPriority::PRIORITY_HIGH)
  );

  if (msgId > 0) {
    Serial.printf("   Handed to the mesh as request %u; the result follows when the gateway answers\n", msgId);
    return true;
  }
  // The callback reports why, from the scheduler, in a moment.
  Serial.println("   Not handed to the mesh");
  return false;
}

/**
 * Send sensor data to a cloud API
 * 
 * This demonstrates sending JSON sensor data to a REST API endpoint.
 * In a real application, you would replace the URL with your actual
 * cloud API endpoint (AWS, Azure, ThingsBoard, etc.)
 */
void sendSensorDataToCloud() {
  // Simulate sensor readings using configured ranges
  float temperature = TEMP_MIN + random(0, (int)(TEMP_RANGE * 10)) / 10.0;
  float humidity = HUMIDITY_MIN + random(0, (int)(HUMIDITY_RANGE * 10)) / 10.0;
  // O2 drifts rather than jumping, so an alarm is an episode, not a coin toss
  o2Level += random(-3, 4) / 10.0;
  if (o2Level < O2_MIN) o2Level = O2_MIN;
  if (o2Level > O2_MAX) o2Level = O2_MAX;
  
  // Create JSON payload
  String payload = "{";
  payload += "\"nodeId\":" + String(mesh.getNodeId()) + ",";
  payload += "\"temperature\":" + String(temperature, 1) + ",";
  payload += "\"humidity\":" + String(humidity, 1) + ",";
  payload += "\"o2Level\":" + String(o2Level, 1) + ",";
  payload += "\"timestamp\":" + String(millis());
  payload += "}";
  
  Serial.println("\n📊 Sending sensor data to cloud...");
  Serial.printf("   Payload: %s\n", payload.c_str());
  
  // Alert once when O2 falls below the threshold; re-arm once it recovers.
  // An alert that could not be sent (no gateway yet, paused after a refusal)
  // is tried again at the next reading while the alarm lasts.
  if (!o2Alarm && o2Level < O2_ALARM_THRESHOLD) {
    String alertMsg = "⚠️ ALARM: O2 level critical at " + String(o2Level, 1) + " mg/L! Node: " + String(mesh.getNodeId());
    o2Alarm = sendAlertToWhatsApp(alertMsg);
  } else if (o2Alarm && o2Level > O2_ALARM_CLEAR) {
    Serial.printf("   O2 back to %.1f mg/L; alarm cleared\n", o2Level);
    o2Alarm = false;
  }
  
  // Only send if Internet is available
  if (!mesh.hasInternetConnection()) {
    Serial.println("   ⚠️ No Internet - data not sent (would be queued in production)");
    return;
  }
  
  // Send to the cloud API configured at the top of the sketch
  String cloudUrl = CLOUD_URL;
  if (cloudUrl.indexOf("example.com") >= 0) {
    Serial.println("   (CLOUD_URL is still the placeholder; set your endpoint to send readings)");
    return;
  }

  uint32_t msgId = mesh.sendToInternet(
    cloudUrl,
    payload,
    [](bool success, uint16_t httpStatus, String error) {
      if (success) {
        Serial.printf("   ✅ Cloud data sent! HTTP: %d\n", httpStatus);
      } else {
        Serial.printf("   ❌ Cloud send failed: %s\n", error.c_str());
      }
    }
  );
  
  Serial.printf("   Message ID: %u\n", msgId);
}

// ============================================
// Mesh Callbacks
// ============================================

/**
 * Send the startup notification once a gateway with Internet is known.
 *
 * Runs every 30 s from taskStartupAlert until the message has been handed to
 * sendToInternet(), then disables itself.
 */
void sendStartupAlert() {
  if (!mesh.hasInternetConnection()) {
    Serial.println("(startup WhatsApp waiting for a gateway with Internet; retrying in 30 s)");
    return;
  }
  if (sendAlertToWhatsApp(startupMsg)) {
    taskStartupAlert.disable();
  }
}

void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("📨 Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("✓ New connection: Node %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("🔄 Mesh topology changed. Nodes: %d\n", mesh.getNodeList().size());
}

// ============================================
// Setup Function
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n");
  Serial.println("================================================");
  Serial.println("   painlessMesh - sendToInternet Example");
  Serial.println("   WhatsApp/Callmebot Integration Demo");
  Serial.println("================================================\n");
  
  // Configure debug output
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  bool success = false;
  
#if IS_BRIDGE_NODE
  // Initialize as bridge with router connection
  Serial.println("Mode: BRIDGE (Gateway with Internet access)\n");
  success = mesh.initAsBridge(
    MESH_PREFIX, MESH_PASSWORD,
    ROUTER_SSID, ROUTER_PASSWORD,
    &userScheduler, MESH_PORT
  );
  
  if (success) {
    Serial.println("✓ Bridge initialized - this node has Internet access");
    Serial.println("  Other nodes can use sendToInternet() through this gateway\n");
  } else {
    Serial.println("✗ Bridge init failed - falling back to regular mesh");
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  }
#else
  // Initialize as regular mesh node
  Serial.println("Mode: REGULAR NODE (sends to Internet via gateway)\n");
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  success = true;
#endif
  
  // IMPORTANT: Enable the sendToInternet() API
  mesh.enableSendToInternet();
  
  // Register callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  
  // Start periodic sensor data task
  userScheduler.addTask(taskSendSensorData);
  taskSendSensorData.enable();
  
  // Print startup info
  Serial.println("================================================");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Is Bridge: %s\n", mesh.isBridge() ? "YES" : "NO");
  Serial.println("================================================\n");
  
  // Send a startup notification via WhatsApp (demonstrates sendToInternet).
  // First attempt in 30 s, then every 30 s until a gateway with Internet is
  // known: a bridge can use its own uplink at once, a regular node has to
  // find the mesh first.
  // A boot tag makes every start's text different: a service that drops a
  // repeat of the last message (or answers it with HTTP 208) will not
  // swallow this one because the previous boot sent the same words.
  startupMsg = "🚀 Node " + String(mesh.getNodeId()) + " started (boot " +
               String((uint32_t)random(0x10000), HEX) + ")";
  Serial.println("Will send the startup WhatsApp as soon as a gateway with Internet is known (first try in 30 s)...\n");
  userScheduler.addTask(taskStartupAlert);
  taskStartupAlert.enableDelayed();
}

// ============================================
// Main Loop
// ============================================
void loop() {
  mesh.update();
}
