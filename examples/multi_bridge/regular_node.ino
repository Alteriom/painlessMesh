//************************************************************
// Multi-Bridge Example: Regular Node
//
// This example demonstrates a regular mesh node in a multi-bridge
// network. The node automatically discovers and uses available bridges
// for Internet connectivity.
//
// Features demonstrated:
// - Automatic bridge discovery with channel auto-detection
// - Bridge selection awareness
// - Message routing to best available bridge
// - Failover handling
//
// Important: Regular nodes use channel=0 for auto-detection to
// discover bridges operating on the router's WiFi channel.
//************************************************************

#include "painlessMesh.h"

#define MESH_PREFIX     "MultiBridgeMesh"
#define MESH_PASSWORD   "meshpassword"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Simulated sensor data
float temperature = 22.5;
float humidity = 45.0;

// Task to send sensor data
Task taskSendMessage(5000, TASK_FOREVER, [](){
  // Simulate sensor readings
  temperature += (random(-10, 10) / 10.0);
  humidity += (random(-50, 50) / 10.0);
  
  String msg = "Temperature: " + String(temperature, 1) + "°C, Humidity: " + String(humidity, 1) + "%";
  
  Serial.println("\n--- Sending Sensor Data ---");
  
  // Get recommended bridge
  uint32_t recommendedBridge = mesh.getRecommendedBridge();
  
  if (recommendedBridge != 0) {
    Serial.printf("Recommended Bridge: %u\n", recommendedBridge);
    Serial.printf("Message: %s\n", msg.c_str());
    
    // Send to specific bridge
    mesh.sendSingle(recommendedBridge, msg);
    Serial.println("✓ Sent to bridge");
  } else {
    Serial.println("⚠️ No bridge available!");
    Serial.println("Message queued for later delivery");
    // In production, you would queue this message
  }
  Serial.println("---------------------------\n");
});

// Task to display network status
Task taskNetworkStatus(15000, TASK_FOREVER, [](){
  Serial.println("\n=== Network Status ===");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Connected Nodes: %d\n", mesh.getNodeList().size());
  
  // Show all active bridges
  auto activeBridges = mesh.getActiveBridges();
  Serial.printf("Active Bridges: %d\n", activeBridges.size());
  
  if (activeBridges.empty()) {
    Serial.println("  ⚠️ NO BRIDGES AVAILABLE");
  } else {
    for (auto bridgeId : activeBridges) {
      Serial.printf("  - Bridge: %u\n", bridgeId);
    }
  }
  
  // Check Internet connectivity
  bool hasInternet = mesh.hasInternetConnection();
  Serial.printf("Internet Available: %s\n", hasInternet ? "YES" : "NO");
  
  Serial.println("======================\n");
});

void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("📨 Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("🔗 New Connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("🔄 Connections changed. Total nodes: %d\n", mesh.getNodeList().size());
}

void bridgeStatusCallback(uint32_t bridgeNodeId, bool hasInternet) {
  Serial.printf("\n🌉 Bridge Status Update: Bridge %u - Internet %s\n", 
                bridgeNodeId, hasInternet ? "CONNECTED" : "OFFLINE");
  
  if (hasInternet) {
    Serial.println("✓ Internet connection available");
  } else {
    Serial.println("⚠️ Bridge lost Internet - checking for alternatives...");
    
    auto activeBridges = mesh.getActiveBridges();
    if (activeBridges.empty()) {
      Serial.println("⚠️ No alternative bridges available!");
    } else {
      Serial.printf("✓ %d alternative bridge(s) available\n", activeBridges.size());
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n=== Multi-Bridge: REGULAR NODE ===\n");
  
  // Initialize as regular mesh node with auto-channel detection
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // channel=0 enables automatic mesh channel detection
  // This is required to discover bridges that operate on the router's channel
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
  
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onBridgeStatusChanged(&bridgeStatusCallback);
  
  // Add tasks
  userScheduler.addTask(taskSendMessage);
  userScheduler.addTask(taskNetworkStatus);
  taskSendMessage.enable();
  taskNetworkStatus.enable();
  
  Serial.println("\n=== Regular Node Ready ===");
  Serial.println("This node will automatically discover and use available bridges");
  Serial.println("In multi-bridge mode, it will use the highest priority bridge");
  Serial.println("If primary fails, it automatically switches to secondary\n");
}

void loop() {
  mesh.update();
}
