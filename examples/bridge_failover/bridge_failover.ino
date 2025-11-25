//************************************************************
// Bridge Failover with RSSI-Based Election Example
//
// This example demonstrates automatic bridge failover in painlessMesh.
// When the primary bridge loses Internet connectivity, nodes automatically
// hold an election to select a new bridge based on router signal strength.
//
// Hardware Required:
// - ESP32 or ESP8266
// - WiFi router with Internet connection
//
// Setup Options:
//
// OPTION A - Auto-Election Mode (Recommended):
// 1. Configure your mesh credentials (MESH_PREFIX, MESH_PASSWORD)
// 2. Configure your router credentials (ROUTER_SSID, ROUTER_PASSWORD)
// 3. Keep INITIAL_BRIDGE = false on ALL nodes
// 4. Flash multiple nodes with this sketch
// 5. After startup (~60 seconds), nodes will automatically elect a bridge
//    based on best router signal strength (RSSI)
//
// OPTION B - Pre-Designated Bridge Mode:
// 1. Configure your mesh credentials (MESH_PREFIX, MESH_PASSWORD)
// 2. Configure your router credentials (ROUTER_SSID, ROUTER_PASSWORD)
// 3. Set INITIAL_BRIDGE = true on ONE node (your designated bridge)
// 4. Keep INITIAL_BRIDGE = false on all other nodes
// 5. Flash the nodes - designated bridge starts immediately
// 6. If designated bridge fails, others will hold election
//
// Features Demonstrated:
// - Automatic bridge election when no bridge exists
// - Automatic bridge failure detection via heartbeats
// - RSSI-based election protocol
// - Deterministic winner selection with tiebreakers
// - Seamless promotion to bridge role
// - Bridge takeover announcements
// - Automatic mesh channel detection for bridge discovery
//
// Important Note:
// Regular nodes MUST use channel auto-detection (channel=0) to discover
// bridges that are operating on the router's WiFi channel. The example
// automatically uses channel=0 for this purpose.
//
//************************************************************

#include "painlessMesh.h"

// Mesh configuration
#define MESH_PREFIX     "FishFarmMesh"
#define MESH_PASSWORD   "securepass"
#define MESH_PORT       5555

// Router configuration (for bridge connectivity)
#define ROUTER_SSID     "YourRouterSSID"
#define ROUTER_PASSWORD "YourRouterPassword"

// Set to true to make this node the initial bridge
// Set to false for regular nodes that can become bridges via election
#define INITIAL_BRIDGE  false

Scheduler userScheduler;
painlessMesh mesh;

// Task to send periodic status messages
Task taskSendStatus(30000, TASK_FOREVER, [](){
  DynamicJsonDocument doc(256);
  JsonObject obj = doc.to<JsonObject>();
  
  obj["nodeId"] = mesh.getNodeId();
  obj["uptime"] = millis() / 1000;
  obj["freeHeap"] = ESP.getFreeHeap();
  obj["isBridge"] = mesh.isBridge();
  obj["hasInternet"] = mesh.hasInternetConnection();
  
  // Add bridge information if available
  auto primaryBridge = mesh.getPrimaryBridge();
  if (primaryBridge) {
    obj["primaryBridge"] = primaryBridge->nodeId;
    obj["bridgeRSSI"] = primaryBridge->routerRSSI;
  }
  
  String msg;
  serializeJson(doc, msg);
  
  Serial.printf("Status: %s\n", msg.c_str());
});

void bridgeStatusCallback(uint32_t bridgeNodeId, bool hasInternet) {
  Serial.printf("⚠️  Bridge %u status changed: Internet %s\n", 
                bridgeNodeId, hasInternet ? "Connected" : "Disconnected");
  
  if (!hasInternet) {
    Serial.println("Bridge lost Internet - election may start");
  }
}

void bridgeRoleCallback(bool isBridge, String reason) {
  if (isBridge) {
    Serial.printf("🎯 PROMOTED TO BRIDGE: %s\n", reason.c_str());
    Serial.println("This node is now the primary bridge!");
  } else {
    Serial.printf("Regular node mode: %s\n", reason.c_str());
  }
}

void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections. Nodes: %d\n", mesh.getNodeList().size());
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Time adjusted: %u. Offset: %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Bridge Failover Example ===\n");

  if (INITIAL_BRIDGE) {
    Serial.println("Mode: INITIAL BRIDGE");
    Serial.println("This node will start as the primary bridge\n");
    
    // Initialize as bridge with automatic channel detection
    bool bridgeSuccess = mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                                           ROUTER_SSID, ROUTER_PASSWORD,
                                           &userScheduler, MESH_PORT);
    
    if (!bridgeSuccess) {
      Serial.println("✗ Failed to initialize as bridge!");
      Serial.println("Router unreachable - falling back to regular node with failover");
      
      // Fallback: Initialize as regular node with bridge failover enabled
      // This allows automatic promotion to bridge when router becomes available
      // channel=0 enables automatic mesh channel detection
      mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
      mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
      mesh.enableBridgeFailover(true);
      mesh.setElectionTimeout(5000);
      
      Serial.println("✓ Running as regular node - will auto-promote when router available");
    }
  } else {
    Serial.println("Mode: REGULAR NODE (Failover Enabled)");
    Serial.println("This node can become bridge via election\n");
    
    // Initialize as regular node with auto-channel detection
    // channel=0 enables automatic mesh channel detection
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
    
    // Configure for automatic bridge failover
    mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
    mesh.enableBridgeFailover(true);
    mesh.setElectionTimeout(5000);  // 5 second election window
    
    // Optional: Set minimum RSSI for isolated bridge elections (default: -80 dBm)
    // This prevents nodes with poor signal from becoming bridges when isolated
    // mesh.setMinimumBridgeRSSI(-80);  // Uncomment to customize threshold
    
    // Optional: Configure election timing to prevent split-brain when nodes start simultaneously
    // Longer startup delay allows more time for mesh formation before elections begin
    // mesh.setElectionStartupDelay(90000);  // 90 seconds (default: 60 seconds)
    
    // Optional: Increase random delay to reduce simultaneous election risk
    // Longer delays provide more mesh discovery time when multiple nodes detect missing bridge
    // mesh.setElectionRandomDelay(10000, 30000);  // 10-30 seconds (default: 1-3 seconds)
  }

  // Register callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onBridgeStatusChanged(&bridgeStatusCallback);
  mesh.onBridgeRoleChanged(&bridgeRoleCallback);

  // Configure logging
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

  // Add status reporting task
  userScheduler.addTask(taskSendStatus);
  taskSendStatus.enable();
  
  Serial.printf("\nNode ID: %u\n", mesh.getNodeId());
  Serial.println("Setup complete!\n");
}

void loop() {
  mesh.update();
  
  // Monitor bridge status every 10 seconds
  static unsigned long lastBridgeCheck = 0;
  if (millis() - lastBridgeCheck > 10000) {
    lastBridgeCheck = millis();
    
    Serial.println("\n--- Bridge Status ---");
    Serial.printf("I am bridge: %s\n", mesh.isBridge() ? "YES" : "NO");
    Serial.printf("Internet available: %s\n", mesh.hasInternetConnection() ? "YES" : "NO");
    
    auto bridges = mesh.getBridges();
    Serial.printf("Known bridges: %d\n", bridges.size());
    
    for (const auto& bridge : bridges) {
      Serial.printf("  Bridge %u: Internet=%s, RSSI=%d dBm, LastSeen=%u ms ago\n",
                    bridge.nodeId,
                    bridge.internetConnected ? "YES" : "NO",
                    bridge.routerRSSI,
                    millis() - bridge.lastSeen);
    }
    
    auto primary = mesh.getPrimaryBridge();
    if (primary) {
      Serial.printf("Primary bridge: %u (RSSI: %d dBm)\n", 
                    primary->nodeId, primary->routerRSSI);
    } else {
      Serial.println("No primary bridge available!");
    }
    Serial.println("--------------------\n");
  }
}
