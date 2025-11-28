//************************************************************
// Advanced HTTP Client with Shared Gateway Mode Example
//
// This example demonstrates how to use shared gateway mode with painlessMesh
// to send HTTP/HTTPS requests to external APIs. It showcases:
//
// Key Features:
// 1. Shared gateway mode - all nodes connect to the same router
// 2. HTTP/HTTPS POST requests to external APIs
// 3. GatewayDataPackage for mesh routing with message tracking
// 4. GatewayAckPackage for delivery confirmations
// 5. Automatic failover when Internet connection is lost
// 6. Priority levels for messages (CRITICAL, HIGH, NORMAL, LOW)
// 7. Status monitoring and debugging
//
// Architecture:
//
//     ┌─────────────────────────────────────────────────┐
//     │                   Internet                      │
//     │            (api.example.com)                    │
//     └──────────────────────┬──────────────────────────┘
//                            │
//                      WiFi Router
//          ┌─────────────────┼─────────────────┐
//          │                 │                 │
//      Node A            Node B            Node C
//      (HTTP Client)     (HTTP Client)     (HTTP Client)
//      [AP+STA]          [AP+STA]          [AP+STA]
//          │                 │                 │
//          └─────────────────┼─────────────────┘
//                      Mesh Network
//
// Hardware Requirements:
// - ESP32 or ESP8266
// - WiFi router within range of all nodes
// - Internet connectivity for external API access
//
// Use Cases:
// - IoT sensor data upload to cloud services
// - Fish farm monitoring with redundant cloud reporting
// - Industrial sensor networks with cloud aggregation
// - Smart building systems with external API integration
//
//************************************************************

#include "painlessMesh.h"
#include "painlessmesh/gateway.hpp"

// Platform-specific HTTP client includes
#ifdef ESP32
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <WiFiClientSecure.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClientSecure.h>
#endif

// ============================================
// Mesh Network Configuration
// ============================================
// These settings must match across all nodes in your mesh network
#define MESH_PREFIX     "SharedHTTPMesh"     // Mesh network name (SSID prefix)
#define MESH_PASSWORD   "meshPassword123"    // Mesh network password
#define MESH_PORT       5555                 // TCP port for mesh communication

// ============================================
// Router Configuration
// ============================================
// Configure these to match your WiFi router settings
// All nodes in the mesh will connect to this same router
#define ROUTER_SSID     "YourRouterSSID"     // Your WiFi router name
#define ROUTER_PASSWORD "YourRouterPassword" // Your WiFi router password

// ============================================
// HTTP API Configuration
// ============================================
// Configure your target API endpoint
// For HTTPS, use port 443 and enable USE_HTTPS
#define API_ENDPOINT    "http://httpbin.org/post"  // Test endpoint (change for production)
#define API_HOST        "httpbin.org"              // Host for HTTPS certificate validation
#define USE_HTTPS       false                      // Set to true for HTTPS connections
#define HTTP_TIMEOUT    10000                      // HTTP request timeout (milliseconds)

// ============================================
// Timing Configuration
// ============================================
#define SENSOR_INTERVAL     30000   // Interval between sensor readings (milliseconds)
#define STATUS_INTERVAL     10000   // Interval between status reports (milliseconds)
#define RETRY_DELAY         5000    // Delay between retry attempts (milliseconds)
#define MAX_RETRIES         3       // Maximum number of retry attempts
#define INTERNET_CHECK_INTERVAL 30000  // Interval between Internet checks (milliseconds)

// ============================================
// Task Scheduler and Mesh Instance
// ============================================
Scheduler userScheduler;
painlessMesh mesh;

// Gateway packages namespace for convenience
using namespace painlessmesh::gateway;

// ============================================
// Global State Variables
// ============================================
bool internetAvailable = false;        // Current Internet connectivity status
uint32_t lastInternetCheck = 0;        // Timestamp of last Internet check
uint32_t httpRequestCount = 0;         // Total HTTP requests sent
uint32_t httpSuccessCount = 0;         // Successful HTTP requests
uint32_t httpFailCount = 0;            // Failed HTTP requests
uint32_t messageIdCounter = 0;         // Counter for generating unique message IDs

// Track pending acknowledgments
struct PendingRequest {
  uint32_t messageId;
  uint32_t timestamp;
  String destination;
  uint8_t retryCount;
  uint8_t priority;
};
std::vector<PendingRequest> pendingRequests;

// ============================================
// Function Prototypes
// ============================================
void sendSensorData();
void sendStatusReport();
void checkInternetConnectivity();
void handleIncomingMessage(uint32_t from, String& msg);
void processGatewayAck(JsonObject obj);
bool sendHttpRequest(const String& url, const String& payload, int& httpCode);
void retryPendingRequests();
String createSensorPayload();
uint32_t generateMessageId();

// ============================================
// Task Definitions
// ============================================

// Task to send sensor data periodically
Task taskSendSensorData(SENSOR_INTERVAL, TASK_FOREVER, &sendSensorData);

// Task to send status reports
Task taskSendStatus(STATUS_INTERVAL, TASK_FOREVER, &sendStatusReport);

// Task to check Internet connectivity
Task taskCheckInternet(INTERNET_CHECK_INTERVAL, TASK_FOREVER, &checkInternetConnectivity);

// Task to retry pending requests
Task taskRetryRequests(RETRY_DELAY, TASK_FOREVER, &retryPendingRequests);

// ============================================
// Message ID Generation
// ============================================

/**
 * Generate a unique message ID
 * 
 * Combines the node ID with an incrementing counter to ensure
 * uniqueness across the mesh network.
 * 
 * @return Unique message ID
 */
uint32_t generateMessageId() {
  messageIdCounter++;
  // Combine node ID (upper 16 bits) with counter (lower 16 bits)
  return ((mesh.getNodeId() & 0xFFFF) << 16) | (messageIdCounter & 0xFFFF);
}

// ============================================
// Internet Connectivity Check
// ============================================

/**
 * Check if Internet is available by making a test connection
 * 
 * Uses a simple TCP connection to a known host (Google DNS) to verify
 * Internet connectivity. This is more reliable than checking WiFi status.
 */
void checkInternetConnectivity() {
  if (WiFi.status() != WL_CONNECTED) {
    if (internetAvailable) {
      Serial.println("\n⚠️  WiFi disconnected - Internet unavailable");
      internetAvailable = false;
    }
    return;
  }
  
  // Try to connect to Google DNS to verify Internet connectivity
  WiFiClient testClient;
  bool connected = testClient.connect("8.8.8.8", 53);
  testClient.stop();
  
  if (connected != internetAvailable) {
    internetAvailable = connected;
    Serial.printf("\n%s Internet %s\n", 
                  connected ? "✓" : "✗",
                  connected ? "connected" : "disconnected - failover active");
    
    // If Internet just came back, retry pending requests
    if (connected && !pendingRequests.empty()) {
      Serial.printf("  Retrying %d pending requests...\n", pendingRequests.size());
    }
  }
  
  lastInternetCheck = millis();
}

// ============================================
// HTTP Request Functions
// ============================================

/**
 * Send an HTTP POST request to the specified URL
 * 
 * Supports both HTTP and HTTPS connections. For HTTPS, the certificate
 * validation is disabled for simplicity (not recommended for production).
 * 
 * @param url The target URL for the POST request
 * @param payload The JSON payload to send
 * @param httpCode Output parameter for the HTTP response code
 * @return true if the request was successful (2xx response), false otherwise
 */
bool sendHttpRequest(const String& url, const String& payload, int& httpCode) {
  httpRequestCount++;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("  ✗ HTTP request failed: WiFi not connected");
    httpFailCount++;
    return false;
  }
  
  if (!internetAvailable) {
    Serial.println("  ✗ HTTP request failed: Internet not available");
    httpFailCount++;
    return false;
  }
  
  Serial.printf("  → Sending HTTP%s request to: %s\n", 
                USE_HTTPS ? "S" : "", url.c_str());
  
  HTTPClient http;
  bool success = false;
  
#ifdef ESP32
  if (USE_HTTPS) {
    // ESP32 HTTPS with certificate validation disabled
    WiFiClientSecure* secureClient = new WiFiClientSecure();
    secureClient->setInsecure();  // Skip certificate validation (not for production!)
    
    http.begin(*secureClient, url);
    http.setTimeout(HTTP_TIMEOUT);
    http.addHeader("Content-Type", "application/json");
    
    httpCode = http.POST(payload);
    
    if (httpCode > 0) {
      Serial.printf("  ← HTTP Response: %d\n", httpCode);
      if (httpCode >= 200 && httpCode < 300) {
        String response = http.getString();
        Serial.printf("  Response: %.100s%s\n", 
                      response.c_str(), 
                      response.length() > 100 ? "..." : "");
        success = true;
        httpSuccessCount++;
      } else {
        httpFailCount++;
      }
    } else {
      Serial.printf("  ✗ HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
      httpFailCount++;
    }
    
    http.end();
    delete secureClient;
  } else {
    // ESP32 HTTP (non-secure)
    http.begin(url);
    http.setTimeout(HTTP_TIMEOUT);
    http.addHeader("Content-Type", "application/json");
    
    httpCode = http.POST(payload);
    
    if (httpCode > 0) {
      Serial.printf("  ← HTTP Response: %d\n", httpCode);
      if (httpCode >= 200 && httpCode < 300) {
        String response = http.getString();
        Serial.printf("  Response: %.100s%s\n", 
                      response.c_str(), 
                      response.length() > 100 ? "..." : "");
        success = true;
        httpSuccessCount++;
      } else {
        httpFailCount++;
      }
    } else {
      Serial.printf("  ✗ HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
      httpFailCount++;
    }
    
    http.end();
  }
#else
  // ESP8266 implementation
  WiFiClient client;
  
  if (USE_HTTPS) {
    // ESP8266 HTTPS with certificate validation disabled
    WiFiClientSecure secureClient;
    secureClient.setInsecure();  // Skip certificate validation (not for production!)
    
    http.begin(secureClient, url);
    http.setTimeout(HTTP_TIMEOUT);
    http.addHeader("Content-Type", "application/json");
    
    httpCode = http.POST(payload);
    
    if (httpCode > 0) {
      Serial.printf("  ← HTTP Response: %d\n", httpCode);
      if (httpCode >= 200 && httpCode < 300) {
        String response = http.getString();
        Serial.printf("  Response: %.100s%s\n", 
                      response.c_str(), 
                      response.length() > 100 ? "..." : "");
        success = true;
        httpSuccessCount++;
      } else {
        httpFailCount++;
      }
    } else {
      Serial.printf("  ✗ HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
      httpFailCount++;
    }
    
    http.end();
  } else {
    // ESP8266 HTTP (non-secure)
    http.begin(client, url);
    http.setTimeout(HTTP_TIMEOUT);
    http.addHeader("Content-Type", "application/json");
    
    httpCode = http.POST(payload);
    
    if (httpCode > 0) {
      Serial.printf("  ← HTTP Response: %d\n", httpCode);
      if (httpCode >= 200 && httpCode < 300) {
        String response = http.getString();
        Serial.printf("  Response: %.100s%s\n", 
                      response.c_str(), 
                      response.length() > 100 ? "..." : "");
        success = true;
        httpSuccessCount++;
      } else {
        httpFailCount++;
      }
    } else {
      Serial.printf("  ✗ HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
      httpFailCount++;
    }
    
    http.end();
  }
#endif

  return success;
}

// ============================================
// Sensor Data Functions
// ============================================

/**
 * Create a JSON payload with simulated sensor data
 * 
 * In a real application, replace this with actual sensor readings.
 * 
 * @return JSON string containing sensor data
 */
String createSensorPayload() {
  // ArduinoJson v7 uses JsonDocument with automatic sizing
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  
  // Node identification
  obj["nodeId"] = mesh.getNodeId();
  obj["timestamp"] = mesh.getNodeTime();
  
  // Simulated sensor readings (replace with real sensor code)
  obj["temperature"] = 22.5 + (random(100) / 10.0);  // 22.5-32.5°C
  obj["humidity"] = 45.0 + (random(300) / 10.0);     // 45-75%
  obj["pressure"] = 1013.0 + (random(200) / 10.0);   // 1013-1033 hPa
  
  // System health metrics
  obj["freeHeap"] = ESP.getFreeHeap();
  obj["uptime"] = millis() / 1000;
  obj["rssi"] = WiFi.RSSI();
  
  // Mesh status
  obj["meshNodes"] = mesh.getNodeList().size();
  obj["routerConnected"] = (WiFi.status() == WL_CONNECTED);
  obj["internetAvailable"] = internetAvailable;
  
  String payload;
  serializeJson(doc, payload);
  return payload;
}

/**
 * Send sensor data to the cloud API
 * 
 * Creates a sensor payload and sends it via HTTP POST request.
 * If the request fails, it's added to the pending queue for retry.
 */
void sendSensorData() {
  Serial.println("\n======== SENSOR DATA UPLOAD ========");
  
  String payload = createSensorPayload();
  Serial.printf("Payload: %s\n", payload.c_str());
  
  if (!internetAvailable) {
    // Queue the request for later retry
    PendingRequest req;
    req.messageId = generateMessageId();
    req.timestamp = millis();
    req.destination = API_ENDPOINT;
    req.retryCount = 0;
    req.priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_NORMAL);
    
    if (pendingRequests.size() < 10) {  // Limit queue size to conserve memory
      pendingRequests.push_back(req);
      Serial.printf("  ⏳ Request queued for retry (ID: %u, Queue size: %d)\n", 
                    req.messageId, pendingRequests.size());
    } else {
      Serial.println("  ⚠️  Queue full - request dropped");
    }
    Serial.println("====================================\n");
    return;
  }
  
  // Attempt to send HTTP request
  int httpCode;
  bool success = sendHttpRequest(API_ENDPOINT, payload, httpCode);
  
  if (success) {
    Serial.printf("  ✓ Sensor data uploaded successfully (HTTP %d)\n", httpCode);
    
    // Optionally broadcast the acknowledgment to mesh for logging
    JsonDocument ackDoc;
    JsonObject ackObj = ackDoc.to<JsonObject>();
    ackObj["type"] = 621;  // GATEWAY_ACK type
    ackObj["msgId"] = generateMessageId();
    ackObj["origin"] = mesh.getNodeId();
    ackObj["success"] = true;
    ackObj["http"] = httpCode;
    ackObj["ts"] = mesh.getNodeTime();
    
    String ackMsg;
    serializeJson(ackDoc, ackMsg);
    mesh.sendBroadcast(ackMsg, static_cast<uint8_t>(GatewayPriority::PRIORITY_LOW));
  } else {
    // Queue for retry
    PendingRequest req;
    req.messageId = generateMessageId();
    req.timestamp = millis();
    req.destination = API_ENDPOINT;
    req.retryCount = 1;
    req.priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_NORMAL);
    
    if (pendingRequests.size() < 10) {
      pendingRequests.push_back(req);
      Serial.printf("  ⏳ Request queued for retry (ID: %u)\n", req.messageId);
    }
  }
  
  Serial.println("====================================\n");
}

/**
 * Retry pending requests that failed previously
 * 
 * Called periodically to attempt sending queued requests.
 * Requests are removed after MAX_RETRIES attempts.
 */
void retryPendingRequests() {
  if (pendingRequests.empty() || !internetAvailable) {
    return;
  }
  
  // Process one request at a time to avoid blocking
  auto it = pendingRequests.begin();
  if (it != pendingRequests.end()) {
    if (it->retryCount >= MAX_RETRIES) {
      Serial.printf("⚠️  Dropping request %u after %d retries\n", 
                    it->messageId, it->retryCount);
      pendingRequests.erase(it);
      return;
    }
    
    Serial.printf("\n🔄 Retrying request %u (attempt %d/%d)\n", 
                  it->messageId, it->retryCount + 1, MAX_RETRIES);
    
    String payload = createSensorPayload();  // Create fresh payload
    int httpCode;
    bool success = sendHttpRequest(it->destination, payload, httpCode);
    
    if (success) {
      Serial.printf("  ✓ Retry successful (HTTP %d)\n", httpCode);
      pendingRequests.erase(it);
    } else {
      it->retryCount++;
      Serial.printf("  ✗ Retry failed, will try again later\n");
    }
  }
}

// ============================================
// Status Reporting
// ============================================

/**
 * Send a status report to the serial console and mesh
 * 
 * Provides comprehensive information about the node's current state,
 * including connectivity, HTTP statistics, and resource usage.
 */
void sendStatusReport() {
  bool routerConnected = (WiFi.status() == WL_CONNECTED);
  
  Serial.println("\n========== STATUS REPORT ==========");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
  Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
  
  Serial.println("\n--- Connectivity ---");
  Serial.printf("Router: %s\n", routerConnected ? "Connected" : "Disconnected");
  if (routerConnected) {
    Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("  Channel: %d\n", WiFi.channel());
  }
  Serial.printf("Internet: %s\n", internetAvailable ? "Available" : "Unavailable");
  Serial.printf("Shared Gateway Mode: %s\n", mesh.isSharedGatewayMode() ? "Active" : "Inactive");
  
  Serial.println("\n--- Mesh Network ---");
  Serial.printf("Mesh Nodes: %d\n", mesh.getNodeList().size());
  
  Serial.println("\n--- HTTP Statistics ---");
  Serial.printf("Total Requests: %u\n", httpRequestCount);
  Serial.printf("Successful: %u\n", httpSuccessCount);
  Serial.printf("Failed: %u\n", httpFailCount);
  Serial.printf("Pending Retry: %d\n", pendingRequests.size());
  if (httpRequestCount > 0) {
    Serial.printf("Success Rate: %.1f%%\n", 
                  (httpSuccessCount * 100.0) / httpRequestCount);
  }
  
  Serial.println("===================================\n");
  
  // Broadcast status to mesh
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  obj["nodeId"] = mesh.getNodeId();
  obj["type"] = "httpGatewayStatus";
  obj["routerConnected"] = routerConnected;
  obj["internetAvailable"] = internetAvailable;
  obj["httpRequests"] = httpRequestCount;
  obj["httpSuccess"] = httpSuccessCount;
  obj["httpFailed"] = httpFailCount;
  obj["pendingRetry"] = pendingRequests.size();
  obj["freeHeap"] = ESP.getFreeHeap();
  obj["uptime"] = millis() / 1000;
  if (routerConnected) {
    obj["rssi"] = WiFi.RSSI();
  }
  
  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg, static_cast<uint8_t>(GatewayPriority::PRIORITY_LOW));
}

// ============================================
// Mesh Callbacks
// ============================================

/**
 * Callback for received mesh messages
 * 
 * Handles incoming messages from other nodes in the mesh.
 * Processes GatewayAckPackage messages for delivery confirmations.
 * 
 * @param from The node ID of the sender
 * @param msg The message content (typically JSON)
 */
void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("📨 Received from %u: %.100s%s\n", 
                from, msg.c_str(), msg.length() > 100 ? "..." : "");
  
  // Parse the message
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, msg);
  
  if (error) {
    Serial.printf("  ✗ JSON parse error: %s\n", error.c_str());
    return;
  }
  
  JsonObject obj = doc.as<JsonObject>();
  int msgType = obj["type"] | 0;
  
  // Handle GatewayAckPackage (Type 621)
  if (msgType == 621) {
    processGatewayAck(obj);
  }
  // Handle status messages from other nodes
  else if (obj["type"].is<const char*>() && 
           String(obj["type"].as<const char*>()) == "httpGatewayStatus") {
    Serial.printf("  → Status from node %u: Internet=%s, Requests=%d, Success=%d\n",
                  obj["nodeId"].as<uint32_t>(),
                  obj["internetAvailable"].as<bool>() ? "Yes" : "No",
                  obj["httpRequests"].as<int>(),
                  obj["httpSuccess"].as<int>());
  }
}

/**
 * Process a GatewayAckPackage message
 * 
 * Handles acknowledgment messages that confirm delivery of data
 * to external services.
 * 
 * @param obj The JSON object containing the acknowledgment data
 */
void processGatewayAck(JsonObject obj) {
  uint32_t messageId = obj["msgId"] | 0;
  uint32_t originNode = obj["origin"] | 0;
  bool success = obj["success"] | false;
  uint16_t httpStatus = obj["http"] | 0;
  
  Serial.printf("  📬 Gateway ACK: msgId=%u, origin=%u, success=%s, http=%d\n",
                messageId, originNode, success ? "true" : "false", httpStatus);
  
  // Remove from pending requests if this was our message
  for (auto it = pendingRequests.begin(); it != pendingRequests.end(); ) {
    if (it->messageId == messageId) {
      Serial.printf("  ✓ Acknowledged message %u removed from queue\n", messageId);
      it = pendingRequests.erase(it);
    } else {
      ++it;
    }
  }
}

/**
 * Callback when a new node joins the mesh
 */
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("\n✓ New Connection: Node %u joined the mesh\n", nodeId);
  Serial.printf("  Total nodes in mesh: %d\n", mesh.getNodeList().size() + 1);
}

/**
 * Callback when mesh topology changes
 */
void changedConnectionCallback() {
  Serial.println("\n⚡ Mesh topology changed");
  auto nodes = mesh.getNodeList();
  Serial.printf("Current mesh contains %d other node(s)\n", nodes.size());
}

/**
 * Callback when mesh time is synchronized
 */
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("⏱️  Time synchronized. Offset: %d us\n", offset);
}

// ============================================
// Critical Alarm Function
// ============================================

/**
 * Send a critical alarm message with highest priority
 * 
 * Use this for urgent alerts that need immediate delivery.
 * Critical messages bypass the normal queue and are sent immediately.
 * 
 * @param alarmType Type of alarm (e.g., "temperature", "oxygen", "security")
 * @param message Descriptive message about the alarm
 */
void sendCriticalAlarm(const String& alarmType, const String& message) {
  Serial.println("\n🚨 CRITICAL ALARM 🚨");
  Serial.printf("  Type: %s\n", alarmType.c_str());
  Serial.printf("  Message: %s\n", message.c_str());
  
  // Create alarm payload
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  obj["nodeId"] = mesh.getNodeId();
  obj["timestamp"] = mesh.getNodeTime();
  obj["alarmType"] = alarmType;
  obj["message"] = message;
  obj["priority"] = "CRITICAL";
  obj["rssi"] = WiFi.RSSI();
  obj["freeHeap"] = ESP.getFreeHeap();
  
  String payload;
  serializeJson(doc, payload);
  
  // Attempt immediate HTTP send
  if (internetAvailable) {
    int httpCode;
    bool success = sendHttpRequest(API_ENDPOINT, payload, httpCode);
    
    if (!success) {
      // Queue with CRITICAL priority for immediate retry
      PendingRequest req;
      req.messageId = generateMessageId();
      req.timestamp = millis();
      req.destination = API_ENDPOINT;
      req.retryCount = 0;
      req.priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_CRITICAL);
      pendingRequests.insert(pendingRequests.begin(), req);  // Add to front of queue
    }
  }
  
  // Also broadcast to mesh with CRITICAL priority
  JsonDocument meshDoc;
  JsonObject meshObj = meshDoc.to<JsonObject>();
  meshObj["type"] = "criticalAlarm";
  meshObj["nodeId"] = mesh.getNodeId();
  meshObj["alarmType"] = alarmType;
  meshObj["message"] = message;
  meshObj["timestamp"] = mesh.getNodeTime();
  
  String meshMsg;
  serializeJson(meshDoc, meshMsg);
  mesh.sendBroadcast(meshMsg, static_cast<uint8_t>(GatewayPriority::PRIORITY_CRITICAL));
  
  Serial.println("🚨 Alarm sent to cloud and mesh\n");
}

// ============================================
// Setup Function
// ============================================
void setup() {
  Serial.begin(115200);
  
  // Wait for serial monitor
  delay(1000);
  
  Serial.println("\n");
  Serial.println("================================================");
  Serial.println("   painlessMesh - Shared Gateway HTTP Example");
  Serial.println("================================================\n");
  
  Serial.printf("Target API: %s\n", API_ENDPOINT);
  Serial.printf("HTTPS: %s\n", USE_HTTPS ? "Enabled" : "Disabled");
  Serial.println("");
  
  // Configure mesh debugging
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  Serial.println("Initializing Shared Gateway Mode...\n");
  Serial.printf("  Mesh SSID: %s\n", MESH_PREFIX);
  Serial.printf("  Mesh Port: %d\n", MESH_PORT);
  Serial.printf("  Router SSID: %s\n", ROUTER_SSID);
  Serial.println("");
  
  // Initialize as Shared Gateway
  bool success = mesh.initAsSharedGateway(
    MESH_PREFIX,
    MESH_PASSWORD,
    ROUTER_SSID,
    ROUTER_PASSWORD,
    &userScheduler,
    MESH_PORT
  );
  
  if (success) {
    Serial.println("✓ Shared Gateway Mode initialized successfully!\n");
  } else {
    Serial.println("✗ Failed to initialize Shared Gateway Mode!");
    Serial.println("  Falling back to regular mesh mode...\n");
    
    // Fallback to regular mesh
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  }
  
  // Register callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  // Initialize random seed for sensor simulation
  randomSeed(analogRead(0) + mesh.getNodeId());
  
  // Add tasks to scheduler
  userScheduler.addTask(taskSendSensorData);
  userScheduler.addTask(taskSendStatus);
  userScheduler.addTask(taskCheckInternet);
  userScheduler.addTask(taskRetryRequests);
  
  // Enable tasks
  taskSendSensorData.enable();
  taskSendStatus.enable();
  taskCheckInternet.enable();
  taskRetryRequests.enable();
  
  // Initial Internet check
  checkInternetConnectivity();
  
  // Startup summary
  Serial.println("================================================");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Shared Gateway Mode: %s\n", mesh.isSharedGatewayMode() ? "Active" : "Inactive");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Router IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Router Channel: %d\n", WiFi.channel());
    Serial.printf("Router RSSI: %d dBm\n", WiFi.RSSI());
  }
  Serial.printf("Internet: %s\n", internetAvailable ? "Available" : "Unavailable");
  Serial.println("================================================\n");
  
  Serial.println("Tasks Enabled:");
  Serial.printf("  - Sensor data upload every %d seconds\n", SENSOR_INTERVAL / 1000);
  Serial.printf("  - Status report every %d seconds\n", STATUS_INTERVAL / 1000);
  Serial.printf("  - Internet check every %d seconds\n", INTERNET_CHECK_INTERVAL / 1000);
  Serial.printf("  - Retry pending requests every %d seconds\n", RETRY_DELAY / 1000);
  Serial.println("\nReady for operation!\n");
}

// ============================================
// Main Loop
// ============================================
void loop() {
  // Update the mesh
  mesh.update();
  
  // Example: Simulate a critical alarm every 5 minutes (for testing)
  static unsigned long lastAlarmTest = 0;
  if (millis() - lastAlarmTest > 300000) {  // 5 minutes
    lastAlarmTest = millis();
    // Uncomment the line below to test critical alarms
    // sendCriticalAlarm("test", "Periodic test alarm from sensor node");
  }
}
