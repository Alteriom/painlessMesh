//************************************************************
// Shared Gateway Example
//
// This example demonstrates the Shared Gateway Mode feature of painlessMesh.
// Unlike the Bridge Mode where only one node connects to the router,
// Shared Gateway Mode allows ALL nodes to connect to the same WiFi router
// while maintaining mesh connectivity.
//
// Key Features:
// - All nodes operate in AP+STA mode simultaneously
// - All nodes connect to the same WiFi router
// - Mesh and router operate on the same channel for reliability
// - Automatic router reconnection if connection is lost
// - Each node can independently access the Internet
//
// Use Cases:
// - Industrial sensor networks where all nodes are within router range
// - Smart building systems requiring redundant Internet access
// - Fish farm monitoring where any node can send critical alarms to cloud
// - Remote monitoring stations with shared WiFi infrastructure
//
// Hardware Requirements:
// - ESP32 or ESP8266
// - WiFi router within range of all nodes
//
// How It Works:
// 1. Each node connects to the router to detect its WiFi channel
// 2. Mesh network is initialized on the same channel as the router
// 3. Router connection is re-established using stationManual()
// 4. Periodic monitoring ensures automatic reconnection if router is lost
//
// For more details, see docs/design/SHARED_GATEWAY_DESIGN.md
//
//************************************************************
#include "painlessMesh.h"

// ============================================
// Mesh Network Configuration
// ============================================
// These settings must match across all nodes in your mesh network
#define MESH_PREFIX     "SharedGatewayMesh"  // Mesh network name (SSID prefix)
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
// Task Scheduler and Mesh Instance
// ============================================
Scheduler userScheduler;
painlessMesh mesh;

// ============================================
// Function Prototypes
// ============================================
void sendStatusBroadcast();
void receivedCallback(uint32_t from, String& msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);

// ============================================
// Task Definitions
// ============================================
// Task to broadcast status every 10 seconds
Task taskBroadcastStatus(10000, TASK_FOREVER, &sendStatusBroadcast);

// ============================================
// Callback Functions
// ============================================

/**
 * Send a status broadcast to all nodes in the mesh
 * 
 * This demonstrates how data can be shared across the mesh network.
 * In a real application, you might send sensor readings, alarms,
 * or other application-specific data.
 */
void sendStatusBroadcast() {
  // Create a JSON status message
  // ArduinoJson v7 uses JsonDocument with automatic sizing
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  
  // Basic node identification
  obj["nodeId"] = mesh.getNodeId();
  obj["type"] = "status";
  
  // Router connection information
  bool routerConnected = (WiFi.status() == WL_CONNECTED);
  obj["routerConnected"] = routerConnected;
  
  if (routerConnected) {
    obj["routerIP"] = WiFi.localIP().toString();
    obj["routerRSSI"] = WiFi.RSSI();
    obj["routerChannel"] = WiFi.channel();
  }
  
  // Mesh network information
  obj["meshNodes"] = mesh.getNodeList().size();
  obj["sharedGatewayMode"] = mesh.isSharedGatewayMode();
  
  // System health information
  obj["uptime"] = millis() / 1000;  // Uptime in seconds
  obj["freeHeap"] = ESP.getFreeHeap();
  
  // Serialize and send
  String msg;
  serializeJson(doc, msg);
  
  // Broadcast to all mesh nodes
  mesh.sendBroadcast(msg);
  
  // Print status to Serial for debugging
  Serial.println("\n========== STATUS BROADCAST ==========");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Router: %s\n", routerConnected ? "Connected" : "Disconnected");
  if (routerConnected) {
    Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("  Channel: %d\n", WiFi.channel());
  }
  Serial.printf("Mesh Nodes: %d\n", mesh.getNodeList().size());
  Serial.printf("Shared Gateway Mode: %s\n", mesh.isSharedGatewayMode() ? "Yes" : "No");
  Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
  Serial.println("=======================================\n");
}

/**
 * Callback for received mesh messages
 * 
 * @param from The node ID of the sender
 * @param msg The message content (typically JSON)
 */
void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
  
  // Parse the message to extract useful information
  // ArduinoJson v7 uses JsonDocument with automatic sizing
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, msg);
  
  if (error) {
    Serial.printf("JSON parse error: %s\n", error.c_str());
    return;
  }
  
  // Check if this is a status message from another shared gateway node
  if (doc["type"] == "status") {
    bool theirRouterConnected = doc["routerConnected"] | false;
    Serial.printf("  -> Node %u router status: %s\n", 
                  from, theirRouterConnected ? "Connected" : "Disconnected");
  }
}

/**
 * Callback when a new node joins the mesh
 * 
 * @param nodeId The ID of the newly connected node
 */
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("\n✓ New Connection: Node %u joined the mesh\n", nodeId);
  Serial.printf("  Total nodes in mesh: %d\n", mesh.getNodeList().size() + 1);
}

/**
 * Callback when mesh topology changes
 * 
 * This is called when nodes join, leave, or when connections are reorganized
 */
void changedConnectionCallback() {
  Serial.println("\n⚡ Mesh topology changed");
  
  // Print current node list
  auto nodes = mesh.getNodeList();
  Serial.printf("Current mesh contains %d other node(s):\n", nodes.size());
  
  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf("  - Node %u\n", *node);
    node++;
  }
}

/**
 * Callback when mesh time is synchronized
 * 
 * painlessMesh maintains synchronized time across all nodes.
 * This callback is triggered when time adjustments occur.
 * 
 * @param offset The time adjustment in microseconds
 */
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Time synchronized. Offset: %d us\n", offset);
}

// ============================================
// Setup Function
// ============================================
void setup() {
  Serial.begin(115200);
  
  // Wait for serial monitor to connect (useful for debugging)
  delay(1000);
  
  Serial.println("\n");
  Serial.println("================================================");
  Serial.println("   painlessMesh - Shared Gateway Example");
  Serial.println("================================================\n");
  
  // Configure mesh debugging output
  // Available types: ERROR, STARTUP, MESH_STATUS, CONNECTION, SYNC,
  //                  COMMUNICATION, GENERAL, MSG_TYPES, REMOTE
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  Serial.println("Initializing Shared Gateway Mode...\n");
  Serial.printf("  Mesh SSID: %s\n", MESH_PREFIX);
  Serial.printf("  Mesh Port: %d\n", MESH_PORT);
  Serial.printf("  Router SSID: %s\n", ROUTER_SSID);
  Serial.println("");
  
  // ============================================
  // Initialize as Shared Gateway
  // ============================================
  // This method will:
  // 1. Connect to the router to detect its WiFi channel
  // 2. Initialize the mesh network on the same channel
  // 3. Re-establish the router connection
  // 4. Set up automatic router reconnection monitoring
  //
  // All nodes using this mode will be on the same channel as the router,
  // ensuring reliable mesh communication AND router connectivity.
  
  bool success = mesh.initAsSharedGateway(
    MESH_PREFIX,      // Mesh network name
    MESH_PASSWORD,    // Mesh network password
    ROUTER_SSID,      // Router SSID to connect to
    ROUTER_PASSWORD,  // Router password
    &userScheduler,   // Task scheduler for mesh operations
    MESH_PORT         // TCP port for mesh communication
  );
  
  if (success) {
    Serial.println("✓ Shared Gateway Mode initialized successfully!\n");
  } else {
    Serial.println("✗ Failed to initialize Shared Gateway Mode!");
    Serial.println("  Check your router credentials and ensure router is in range.");
    Serial.println("  The mesh will still operate, but without router connectivity.\n");
    
    // Fallback: Initialize as regular mesh node
    // This allows the device to still participate in the mesh
    // even if router connection fails
    Serial.println("Falling back to regular mesh mode...\n");
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  }
  
  // ============================================
  // Register Callbacks
  // ============================================
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  // ============================================
  // Enable Status Broadcasting Task
  // ============================================
  userScheduler.addTask(taskBroadcastStatus);
  taskBroadcastStatus.enable();
  
  // ============================================
  // Print Startup Summary
  // ============================================
  Serial.println("================================================");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Shared Gateway Mode: %s\n", mesh.isSharedGatewayMode() ? "Active" : "Inactive");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Router IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Router Channel: %d\n", WiFi.channel());
  }
  Serial.println("================================================\n");
  Serial.println("Status broadcasts will be sent every 10 seconds.\n");
}

// ============================================
// Main Loop
// ============================================
void loop() {
  // Update the mesh - this must be called regularly
  // It handles all mesh network operations including:
  // - Maintaining connections
  // - Processing incoming messages
  // - Running scheduled tasks
  // - Time synchronization
  mesh.update();
}
