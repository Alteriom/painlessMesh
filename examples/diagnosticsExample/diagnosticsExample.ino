/**
 * painlessMesh Diagnostics API Example
 * 
 * This example demonstrates the enhanced diagnostics API for bridge operations.
 * It shows how to:
 * - Enable diagnostics tracking
 * - Get bridge status information
 * - Monitor election history
 * - Test bridge connectivity
 * - Generate diagnostic reports
 * - Export topology for visualization
 * 
 * Hardware: ESP32 or ESP8266
 * 
 * Usage:
 * 1. Upload to multiple devices
 * 2. Configure one device as a bridge (see bridgeExample)
 * 3. Regular nodes will use this diagnostics API to monitor mesh health
 * 4. Open Serial Monitor to see diagnostic information
 */

#include "painlessMesh.h"

#define   MESH_PREFIX     "DiagnosticsMesh"
#define   MESH_PASSWORD   "diagnostic123"
#define   MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Task to periodically print diagnostics
Task taskPrintDiagnostics(30000, TASK_FOREVER, []() {
  Serial.println("\n" + mesh.getDiagnosticReport());
});

// Task to test bridge connectivity
Task taskTestBridge(60000, TASK_FOREVER, []() {
  Serial.println("\n=== Testing Bridge Connectivity ===");
  auto result = mesh.testBridgeConnectivity();
  
  if (result.success) {
    Serial.printf("✓ Bridge test PASSED: %s\n", result.message.c_str());
    Serial.printf("  Latency: %u ms\n", result.latencyMs);
    Serial.printf("  Internet available: %s\n", result.internetReachable ? "Yes" : "No");
  } else {
    Serial.printf("✗ Bridge test FAILED: %s\n", result.message.c_str());
  }
});

// Task to monitor bridge status
Task taskMonitorBridge(15000, TASK_FOREVER, []() {
  auto status = mesh.getBridgeStatus();
  
  Serial.println("\n=== Bridge Status ===");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Role: %s\n", status.role.c_str());
  Serial.printf("Is Bridge: %s\n", status.isBridge ? "Yes" : "No");
  Serial.printf("Internet Connected: %s\n", status.internetConnected ? "Yes" : "No");
  
  if (status.bridgeNodeId != 0) {
    Serial.printf("Bridge Node: %u (RSSI: %d dBm)\n", 
                  status.bridgeNodeId, status.bridgeRSSI);
    Serial.printf("Time since bridge change: %u seconds\n", 
                  status.timeSinceBridgeChange / 1000);
  } else {
    Serial.println("No bridge available");
  }
});

// Task to show election history
Task taskShowElections(120000, TASK_FOREVER, []() {
  auto history = mesh.getElectionHistory();
  
  if (history.size() > 0) {
    Serial.println("\n=== Election History ===");
    for (const auto& election : history) {
      Serial.printf("Winner: %u (RSSI: %d dBm, %u candidates) - %s\n",
                    election.winnerNodeId, election.winnerRSSI,
                    election.candidateCount, election.reason.c_str());
    }
  }
});

// Task to export topology for visualization
Task taskExportTopology(300000, TASK_FOREVER, []() {
  Serial.println("\n=== Mesh Topology (GraphViz DOT format) ===");
  Serial.println(mesh.exportTopologyDOT());
  Serial.println("Copy above to http://www.webgraphviz.com/ to visualize");
});

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection: %u\n", nodeId);
  
  // Get last bridge change when new node connects
  auto event = mesh.getLastBridgeChange();
  if (event.timestamp > 0) {
    Serial.printf("Last bridge change: %u -> %u (%s)\n",
                  event.oldBridgeId, event.newBridgeId, event.reason.c_str());
  }
}

void changedConnectionCallback() {
  Serial.println("Connections changed");
  Serial.printf("Mesh nodes: %d\n", mesh.getNodeList().size());
  
  // Show Internet path for this node
  auto path = mesh.getInternetPath(mesh.getNodeId());
  if (path.size() > 0) {
    Serial.print("Path to Internet: ");
    for (auto nodeId : path) {
      Serial.printf("%u -> ", nodeId);
    }
    Serial.println("Internet");
  }
}

void bridgeStatusChangedCallback(uint32_t bridgeNodeId, bool hasInternet) {
  Serial.printf("\n!!! Bridge Status Changed !!!\n");
  Serial.printf("Bridge %u Internet: %s\n", 
                bridgeNodeId, hasInternet ? "Connected" : "Disconnected");
  
  // Run immediate connectivity test
  auto result = mesh.testBridgeConnectivity();
  Serial.printf("Connectivity test: %s\n", result.message.c_str());
}

void setup() {
  Serial.begin(115200);
  
  Serial.println("\n\n=== painlessMesh Diagnostics Example ===");
  Serial.println("Initializing mesh network...");
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Enable diagnostics tracking
  Serial.println("Enabling diagnostics...");
  mesh.enableDiagnostics(true);
  
  // Setup callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onBridgeStatusChanged(&bridgeStatusChangedCallback);
  
  // Add diagnostic tasks
  userScheduler.addTask(taskPrintDiagnostics);
  userScheduler.addTask(taskTestBridge);
  userScheduler.addTask(taskMonitorBridge);
  userScheduler.addTask(taskShowElections);
  userScheduler.addTask(taskExportTopology);
  
  // Enable tasks
  taskPrintDiagnostics.enable();
  taskTestBridge.enable();
  taskMonitorBridge.enable();
  taskShowElections.enable();
  taskExportTopology.enable();
  
  Serial.println("Setup complete!");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
}

void loop() {
  mesh.update();
}
