/**
 * @file ntpTimeSyncBridge.ino
 * @brief Example: Bridge node with NTP time synchronization
 * 
 * This example demonstrates a bridge node that:
 * 1. Connects to WiFi router (bridge mode)
 * 2. Queries NTP server for accurate time
 * 3. Broadcasts NTP time to mesh nodes via NTPTimeSyncPackage
 * 
 * Regular mesh nodes can receive this broadcast and sync their local time,
 * eliminating the need for each node to query NTP individually.
 */

#include "painlessMesh.h"
#include "alteriom_sensor_package.hpp"

// Mesh configuration
#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_mesh_password"
#define MESH_PORT       5555

// WiFi router configuration (for bridge)
#define ROUTER_SSID     "your_router_ssid"
#define ROUTER_PASSWORD "your_router_password"

// NTP configuration
#define NTP_SERVER      "pool.ntp.org"
#define NTP_OFFSET      0      // Timezone offset in seconds (0 = UTC)
#define NTP_INTERVAL    60000  // Update interval in milliseconds

Scheduler userScheduler;
painlessMesh mesh;

using namespace alteriom;

// Task to broadcast NTP time periodically
Task taskBroadcastNTP(NTP_INTERVAL, TASK_FOREVER, [](){
  // Create NTP time sync package
  auto pkg = NTPTimeSyncPackage();
  pkg.from = mesh.getNodeId();
  
  // Get NTP time from mesh (if available)
  pkg.ntpTime = mesh.getNodeTime() / 1000000;  // Convert to seconds
  pkg.accuracy = 50;  // Estimate 50ms uncertainty
  pkg.source = NTP_SERVER;
  pkg.timestamp = millis();
  
  // Serialize and broadcast to mesh
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  pkg.addTo(std::move(obj));
  
  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);
  
  Serial.printf("Broadcast NTP time: %u from %s (accuracy: %ums)\n", 
    pkg.ntpTime, pkg.source.c_str(), pkg.accuracy);
});

void setup() {
  Serial.begin(115200);
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Configure as bridge (connects to router)
  mesh.stationManual(ROUTER_SSID, ROUTER_PASSWORD);
  mesh.setHostname("AlteriomBridge");
  
  // Set root node (optional, helps with mesh stability)
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  
  // Add NTP broadcast task
  userScheduler.addTask(taskBroadcastNTP);
  taskBroadcastNTP.enable();
  
  Serial.println("Bridge node with NTP time sync initialized");
  Serial.printf("Broadcasting NTP time every %d seconds\n", NTP_INTERVAL / 1000);
}

void loop() {
  mesh.update();
}
