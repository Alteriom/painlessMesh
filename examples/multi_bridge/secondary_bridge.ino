//************************************************************
// Multi-Bridge Example: Secondary Bridge Node
//
// This example demonstrates a secondary bridge (priority 5) in a
// multi-bridge deployment. This provides hot standby redundancy
// and can handle load if the primary bridge is busy.
//
// Features demonstrated:
// - Secondary bridge with medium priority
// - Hot standby mode (always ready)
// - Automatic failover if primary fails
// - Load balancing support
//************************************************************

#include "painlessMesh.h"

#define MESH_PREFIX     "MultiBridgeMesh"
#define MESH_PASSWORD   "meshpassword"
#define MESH_PORT       5555

// Secondary router connection (backup Internet or different ISP)
#define ROUTER_SSID     "SecondaryRouter"
#define ROUTER_PASSWORD "routerpass2"

Scheduler userScheduler;
painlessMesh mesh;

// Task to display bridge coordination status
Task taskBridgeStatus(10000, TASK_FOREVER, [](){
  Serial.println("\n=== Secondary Bridge Status ===");
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
  
  if (recommended == mesh.getNodeId()) {
    Serial.println("⚠️  I AM THE ACTIVE BRIDGE (primary likely failed!)");
  } else {
    Serial.println("✓ Standby mode - primary bridge is active");
  }
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

void bridgeStatusCallback(uint32_t bridgeNodeId, bool hasInternet) {
  if (hasInternet) {
    Serial.printf("✓ Bridge %u: Internet connected\n", bridgeNodeId);
  } else {
    Serial.printf("⚠️ Bridge %u: Internet OFFLINE\n", bridgeNodeId);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n=== Multi-Bridge: SECONDARY BRIDGE ===\n");
  
  // Initialize mesh as secondary bridge with priority 5
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Enable multi-bridge coordination mode
  mesh.enableMultiBridge(true);
  
  // Set bridge selection strategy (PRIORITY_BASED is default)
  mesh.setBridgeSelectionStrategy(painlessMesh::PRIORITY_BASED);
  
  // Initialize as bridge with priority 5 (secondary)
  bool bridgeSuccess = mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                                         ROUTER_SSID, ROUTER_PASSWORD,
                                         &userScheduler, MESH_PORT, 5);  // Priority 5 = Secondary
  
  if (!bridgeSuccess) {
    Serial.println("✗ Failed to initialize as secondary bridge!");
    Serial.println("Check router credentials and connectivity.");
    Serial.println("Halting...");
    while(1) delay(1000);  // Halt execution
  }
  
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onBridgeStatusChanged(&bridgeStatusCallback);
  
  // Add status reporting task
  userScheduler.addTask(taskBridgeStatus);
  taskBridgeStatus.enable();
  
  Serial.println("\n=== Secondary Bridge Ready ===");
  Serial.println("This node is the SECONDARY bridge (Priority 5)");
  Serial.println("It operates in hot standby mode");
  Serial.println("Will take over if primary bridge fails\n");
}

void loop() {
  mesh.update();
}
