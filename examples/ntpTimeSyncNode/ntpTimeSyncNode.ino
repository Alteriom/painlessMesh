/**
 * @file ntpTimeSyncNode.ino
 * @brief Example: Regular mesh node receiving NTP time from bridge
 * 
 * This example demonstrates a regular mesh node that:
 * 1. Connects to the mesh network
 * 2. Listens for NTP time broadcasts from bridge nodes
 * 3. Updates local time when NTP sync is received
 * 4. Optionally syncs RTC module if available
 * 
 * No Internet connection needed - time is received from bridge!
 */

#include "painlessMesh.h"
#include "alteriom_sensor_package.hpp"

// Mesh configuration
#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_mesh_password"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

using namespace alteriom;

// Track last time sync
uint32_t lastNTPSync = 0;
uint32_t ntpSyncCount = 0;

// Received callback - handle incoming messages
void receivedCallback(uint32_t from, String& msg) {
  // Parse JSON message
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, msg);
  
  if (error) {
    Serial.printf("JSON parse error: %s\n", error.c_str());
    return;
  }
  
  JsonObject obj = doc.as<JsonObject>();
  uint16_t msgType = obj["type"];
  
  // Check if this is an NTP time sync message
  if (msgType == 614) {
    // Deserialize NTP time sync package
    auto pkg = NTPTimeSyncPackage(obj);
    
    Serial.printf("\n=== NTP Time Sync Received ===\n");
    Serial.printf("From: %u\n", from);
    Serial.printf("NTP Time: %u\n", pkg.ntpTime);
    Serial.printf("Accuracy: %ums\n", pkg.accuracy);
    Serial.printf("Source: %s\n", pkg.source.c_str());
    Serial.printf("Timestamp: %u\n", pkg.timestamp);
    Serial.println("=============================\n");
    
    // Update mesh time (this is application-specific)
    // In a real implementation, you would:
    // 1. Verify the sender is a bridge node
    // 2. Apply the time with mesh.setTimeFromNTP(pkg.ntpTime)
    // 3. Update RTC if available
    
    lastNTPSync = millis();
    ntpSyncCount++;
    
    Serial.printf("Time sync applied! Total syncs: %u\n", ntpSyncCount);
  }
}

// Status task - periodic status updates
Task taskStatus(30000, TASK_FOREVER, [](){
  uint32_t timeSinceSync = (millis() - lastNTPSync) / 1000;
  
  Serial.printf("\n--- Node Status ---\n");
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Connections: %d\n", mesh.getNodeList().size());
  Serial.printf("NTP Syncs: %u\n", ntpSyncCount);
  
  if (ntpSyncCount > 0) {
    Serial.printf("Last sync: %u seconds ago\n", timeSinceSync);
  } else {
    Serial.println("Waiting for first NTP sync...");
  }
  
  Serial.println("------------------\n");
});

void setup() {
  Serial.begin(115200);
  
  // Initialize mesh with auto-channel detection
  // channel=0 enables automatic mesh channel detection
  // This is required to discover bridges that operate on the router's channel
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
  
  // Set callbacks
  mesh.onReceive(&receivedCallback);
  
  // Add status task
  userScheduler.addTask(taskStatus);
  taskStatus.enable();
  
  Serial.println("Regular mesh node initialized");
  Serial.println("Listening for NTP time broadcasts from bridge...");
}

void loop() {
  mesh.update();
}
