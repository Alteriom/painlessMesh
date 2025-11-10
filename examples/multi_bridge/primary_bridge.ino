//************************************************************
// Multi-Bridge Example: Primary Bridge Node
//
// This example demonstrates a primary bridge (priority 10) in a
// multi-bridge deployment. Use with secondary_bridge.ino for
// redundancy and load balancing.
//
// Features demonstrated:
// - Primary bridge with highest priority
// - Multi-bridge coordination
// - Load reporting and bridge discovery
// - Automatic role management
//************************************************************

#include "painlessMesh.h"

#define MESH_PREFIX     "MultiBridgeMesh"
#define MESH_PASSWORD   "meshpassword"
#define MESH_PORT       5555

// Primary router connection (high-speed Internet)
#define ROUTER_SSID     "PrimaryRouter"
#define ROUTER_PASSWORD "routerpass"

Scheduler userScheduler;
painlessMesh mesh;

// Task to display bridge coordination status
Task taskBridgeStatus(10000, TASK_FOREVER, [](){
  Serial.println("\n=== Primary Bridge Status ===");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Connected Nodes: %d\n", mesh.getNodeList().size());
  
  // Show all active bridges in the mesh
  auto activeBridges = mesh.getActiveBridges();
  Serial.printf("Active Bridges: %d\n", activeBridges.size());
  for (auto bridgeId : activeBridges) {
    Serial.printf("  - Bridge: %u%s\n", bridgeId, 
                  (bridgeId == mesh.getNodeId()) ? " (ME)" : "");
  }
  
  // Show recommended bridge for next message
  uint32_t recommended = mesh.getRecommendedBridge();
  Serial.printf("Recommended Bridge: %u\n", recommended);
  Serial.println("=============================\n");
});

void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections. Node count: %d\n", mesh.getNodeList().size());
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n=== Multi-Bridge: PRIMARY BRIDGE ===\n");
  
  // Initialize mesh as primary bridge with priority 10 (highest)
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Enable multi-bridge coordination mode
  mesh.enableMultiBridge(true);
  
  // Set bridge selection strategy (PRIORITY_BASED is default)
  mesh.setBridgeSelectionStrategy(painlessMesh::PRIORITY_BASED);
  
  // Initialize as bridge with priority 10 (primary)
  mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                    ROUTER_SSID, ROUTER_PASSWORD,
                    &userScheduler, MESH_PORT, 10);  // Priority 10 = Primary
  
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  
  // Add status reporting task
  userScheduler.addTask(taskBridgeStatus);
  taskBridgeStatus.enable();
  
  Serial.println("\n=== Primary Bridge Ready ===");
  Serial.println("This node is the PRIMARY bridge (Priority 10)");
  Serial.println("It will be preferred for all mesh traffic");
  Serial.println("If it fails, the secondary bridge will take over automatically\n");
}

void loop() {
  mesh.update();
}
