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
//    - Call mesh.enableSendToInternet() AFTER mesh.init() on ALL nodes
//    - This enables both sending (regular nodes) AND routing (bridge nodes)
//    - This example shows how to enable it in the setup() function below
//
// For Callmebot WhatsApp API:
// - Get your API key from https://www.callmebot.com/blog/free-api-whatsapp-messages/
// - Format: https://api.callmebot.com/whatsapp.php?phone=PHONE&apikey=KEY&text=MESSAGE
//
//************************************************************
#include "painlessMesh.h"
#include <WiFiClientSecure.h>

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
// Sensor Simulation Configuration
// ============================================
// These define the ranges for simulated sensor values
#define TEMP_MIN        20.0   // Minimum temperature (°C)
#define TEMP_RANGE      10.0   // Temperature range (20-30°C)
#define HUMIDITY_MIN    40.0   // Minimum humidity (%)
#define HUMIDITY_RANGE  40.0   // Humidity range (40-80%)
#define O2_MIN          5.0    // Minimum O2 level (mg/L)
#define O2_RANGE        5.0    // O2 range (5-10 mg/L)
#define O2_ALARM_THRESHOLD 6.0 // O2 level below this triggers alarm

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
void sendAlertToWhatsApp(String message);
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
 */
void sendAlertToWhatsApp(String message) {
  // Check if Internet is available via any gateway
  if (!mesh.hasInternetConnection()) {
    Serial.println("❌ No Internet available - no gateway with Internet found");
    Serial.println("   Make sure at least one node is a bridge with router access");
    return;
  }
  
  // URL-encode the message for safe transmission
  String encodedMessage = urlEncode(message);
  
  // Build the Callmebot WhatsApp API URL
  // Format: https://api.callmebot.com/whatsapp.php?phone=PHONE&apikey=KEY&text=MESSAGE
  String url = "https://api.callmebot.com/whatsapp.php";
  url += "?phone=" + String(WHATSAPP_PHONE);
  url += "&apikey=" + String(WHATSAPP_APIKEY);
  url += "&text=" + encodedMessage;
  
  Serial.println("\n📱 Sending WhatsApp message via sendToInternet()...");
  Serial.printf("   Message: %s\n", message.c_str());
  Serial.printf("   URL: %s\n", url.c_str());
  
  // Use sendToInternet() to route the request through a gateway
  // The callback will be invoked when we get a response (or timeout)
  uint32_t msgId = mesh.sendToInternet(
    url,
    "",  // No payload needed for GET request - params are in URL
    [](bool success, uint16_t httpStatus, String error) {
      if (success) {
        Serial.printf("✅ WhatsApp message sent! HTTP Status: %d\n", httpStatus);
      } else {
        Serial.printf("❌ Failed to send WhatsApp: %s (HTTP: %d)\n", error.c_str(), httpStatus);
      }
    },
    static_cast<uint8_t>(painlessmesh::gateway::GatewayPriority::PRIORITY_HIGH)
  );
  
  if (msgId > 0) {
    Serial.printf("   Message queued with ID: %u\n", msgId);
  } else {
    Serial.println("   ❌ Failed to queue message - no gateway available");
  }
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
  float o2Level = O2_MIN + random(0, (int)(O2_RANGE * 10)) / 10.0;
  
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
  
  // Check for alarm conditions using configured threshold
  if (o2Level < O2_ALARM_THRESHOLD) {
    // O2 level critical - send WhatsApp alert!
    String alertMsg = "⚠️ ALARM: O2 level critical at " + String(o2Level, 1) + " mg/L! Node: " + String(mesh.getNodeId());
    sendAlertToWhatsApp(alertMsg);
  }
  
  // Only send if Internet is available
  if (!mesh.hasInternetConnection()) {
    Serial.println("   ⚠️ No Internet - data not sent (would be queued in production)");
    return;
  }
  
  // Send to cloud API (replace with your actual endpoint)
  // Example endpoints:
  // - ThingsBoard: "https://demo.thingsboard.io/api/v1/YOUR_TOKEN/telemetry"
  // - AWS IoT: "https://YOUR_ENDPOINT.iot.us-east-1.amazonaws.com/topics/sensors"
  // - Custom API: "https://api.yourserver.com/sensors/data"
  
  String cloudUrl = "https://api.example.com/sensors";  // Replace with your endpoint
  
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
  
  // Send a startup notification via WhatsApp (demonstrates sendToInternet)
  String startupMsg = "🚀 Node " + String(mesh.getNodeId()) + " started!";
  
  // Delay to allow mesh to connect first
  Serial.println("Will attempt to send startup WhatsApp in 30 seconds...\n");
  mesh.addTask([startupMsg]() {
    sendAlertToWhatsApp(startupMsg);
  }, 30000);  // 30 second delay
}

// ============================================
// Main Loop
// ============================================
void loop() {
  mesh.update();
}
