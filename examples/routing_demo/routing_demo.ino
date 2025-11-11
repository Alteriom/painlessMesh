/**
 * Multi-Hop Routing Demo
 * 
 * This example demonstrates the new multi-hop routing capabilities of painlessMesh.
 * It shows how to:
 * - Calculate hop count to any node in the mesh
 * - Get the complete routing table
 * - Find the path from this node to any other node
 * 
 * The functions work automatically with any mesh topology - linear chains,
 * star configurations, or complex multi-hop meshes.
 */

#include "painlessMesh.h"

#define MESH_PREFIX     "routingDemo"
#define MESH_PASSWORD   "meshPassword"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Task to periodically display routing information
Task taskShowRouting(10000, TASK_FOREVER, []() {
  Serial.println("\n=== Routing Information ===");
  
  // Get list of all nodes in the mesh
  auto nodeList = mesh.getNodeList(false);  // false = don't include self
  Serial.printf("Mesh contains %d nodes (plus this node)\n", nodeList.size());
  
  // Display hop count to each node
  Serial.println("\nHop Counts:");
  for (auto nodeId : nodeList) {
    int hops = mesh.getHopCount(nodeId);
    Serial.printf("  Node %u: %d hop%s\n", nodeId, hops, hops == 1 ? "" : "s");
  }
  
  // Display complete routing table
  Serial.println("\nRouting Table (Destination -> Next Hop):");
  auto routingTable = mesh.getRoutingTable();
  for (auto& entry : routingTable) {
    Serial.printf("  To %u -> via %u", entry.first, entry.second);
    if (entry.first == entry.second) {
      Serial.print(" (direct connection)");
    }
    Serial.println();
  }
  
  // Example: Show path to first node in list
  if (!nodeList.empty()) {
    uint32_t targetNode = nodeList.front();
    auto path = mesh.getPathToNode(targetNode);
    
    if (!path.empty()) {
      Serial.printf("\nPath to node %u:\n  ", targetNode);
      for (size_t i = 0; i < path.size(); i++) {
        Serial.printf("%u", path[i]);
        if (i < path.size() - 1) {
          Serial.print(" -> ");
        }
      }
      Serial.printf("\n  (Total: %d hop%s)\n", path.size() - 1, 
                    path.size() - 1 == 1 ? "" : "s");
    } else {
      Serial.printf("\nNode %u is unreachable\n", targetNode);
    }
  }
  
  Serial.println("========================\n");
});

void setup() {
  Serial.begin(115200);
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Setup callbacks
  mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("\nNew connection: Node %u joined the mesh\n", nodeId);
  });
  
  mesh.onDroppedConnection([](uint32_t nodeId) {
    Serial.printf("\nConnection dropped: Node %u left the mesh\n", nodeId);
  });
  
  mesh.onChangedConnections([]() {
    Serial.println("\nMesh topology changed - routing tables updated");
  });
  
  // Add routing display task
  userScheduler.addTask(taskShowRouting);
  taskShowRouting.enable();
  
  Serial.println("\nRouting Demo Started");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
}

void loop() {
  mesh.update();
}
