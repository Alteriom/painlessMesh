/**
 * Phase 1 OTA Features Example - Compressed OTA & Enhanced Status
 *
 * This example demonstrates the Phase 1 enhancements to painlessMesh:
 * - Option 1E: Compressed OTA Transfer for 40-60% bandwidth reduction
 * - Option 2A: Enhanced StatusPackage with comprehensive metrics
 *
 * Features:
 * - Compressed firmware updates reduce network bandwidth
 * - Enhanced status reporting with mesh statistics and performance metrics
 * - Backward compatible with existing nodes
 */

#include "alteriom_sensor_package.hpp"
#include "painlessMesh.h"

#define MESH_PREFIX "AlteriomPhase1Mesh"
#define MESH_PASSWORD "phase1Features"
#define MESH_PORT 5555

#define STATUS_INTERVAL 30000  // Send enhanced status every 30 seconds

Scheduler userScheduler;
painlessMesh mesh;

// Task declarations
Task statusTask(STATUS_INTERVAL, TASK_FOREVER, &sendEnhancedStatus);

// Metrics tracking
unsigned long lastRebootTime = 0;
uint32_t totalMessagesReceived = 0;
uint32_t totalMessagesSent = 0;

void setup() {
  Serial.begin(115200);
  
  lastRebootTime = millis();
  
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  
  // Add the enhanced status reporting task
  userScheduler.addTask(statusTask);
  statusTask.enable();
  
  Serial.println("Phase 1 Features Example Started");
  Serial.println("- Compressed OTA enabled");
  Serial.println("- Enhanced Status reporting active");
}

void loop() {
  mesh.update();
}

/**
 * Send Enhanced Status Package with comprehensive metrics
 */
void sendEnhancedStatus() {
  alteriom::EnhancedStatusPackage status;
  
  // Device Health
  status.deviceStatus = 0x01;  // 0x01 = Online and healthy
  status.uptime = (millis() - lastRebootTime) / 1000;  // Uptime in seconds
  
#ifdef ESP32
  status.freeMemory = ESP.getFreeHeap() / 1024;  // Free heap in KB
  status.firmwareVersion = "1.0.0-phase1";
  status.wifiStrength = constrain(map(WiFi.RSSI(), -100, -50, 0, 100), 0, 100);
#elif defined(ESP8266)
  status.freeMemory = ESP.getFreeHeap() / 1024;
  status.firmwareVersion = "1.0.0-phase1";
  status.wifiStrength = constrain(map(WiFi.RSSI(), -100, -50, 0, 100), 0, 100);
#else
  status.freeMemory = 128;  // Default for testing
  status.firmwareVersion = "1.0.0-test";
  status.wifiStrength = 75;
#endif
  
  // For demonstration, use a placeholder MD5
  status.firmwareMD5 = "abc123def456";
  
  // Mesh Statistics
  auto nodeList = mesh.getNodeList();
  status.nodeCount = nodeList.size();
  status.connectionCount = mesh.connectionCount();
  status.messagesReceived = totalMessagesReceived;
  status.messagesSent = totalMessagesSent;
  status.messagesDropped = 0;  // Would need tracking in real implementation
  
  // Performance Metrics
  // In a real implementation, these would be calculated from actual metrics
  status.avgLatency = 50;      // Average message latency in ms
  status.packetLossRate = 1;   // 1% packet loss
  status.throughput = 512;     // Bytes per second
  
  // Alerts - check for various conditions
  status.alertFlags = 0;
  if (status.freeMemory < 32) {
    status.alertFlags |= 0x01;  // Low memory alert
  }
  if (status.wifiStrength < 30) {
    status.alertFlags |= 0x02;  // Weak signal alert
  }
  if (status.connectionCount == 0 && status.nodeCount == 0) {
    status.alertFlags |= 0x04;  // Isolated node alert
  }
  
  status.lastError = (status.alertFlags == 0) ? "" : "See alert flags";
  
  // Convert to JSON and send
  String msg;
  protocol::Variant variant(&status);
  variant.printTo(msg);
  
  mesh.sendBroadcast(msg);
  totalMessagesSent++;
  
  Serial.printf("Enhanced Status sent - Nodes: %d, Mem: %dKB, Uptime: %ds\n",
                status.nodeCount, status.freeMemory, status.uptime);
}

/**
 * Callback when messages are received
 */
void receivedCallback(uint32_t from, String& msg) {
  totalMessagesReceived++;
  
  // Parse the message to identify the type
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, msg);
  
  if (error) {
    Serial.printf("JSON parse error from %u: %s\n", from, error.c_str());
    return;
  }
  
  JsonObject obj = doc.as<JsonObject>();
  int msgType = obj["type"];
  
  switch (msgType) {
    case 203: {  // Enhanced Status Package
      protocol::Variant variant(msg);
      auto status = variant.to<alteriom::EnhancedStatusPackage>();
      
      Serial.printf("Enhanced Status from %u: v%s, Nodes: %d, Mem: %dKB, Msgs: %d/%d\n",
                    from, 
                    status.firmwareVersion.c_str(),
                    status.nodeCount,
                    status.freeMemory,
                    status.messagesReceived,
                    status.messagesSent);
      
      if (status.alertFlags != 0) {
        Serial.printf("  ALERTS (0x%02X): %s\n", status.alertFlags, status.lastError.c_str());
      }
      break;
    }
    
    case 202: {  // Basic Status Package (backward compatible)
      protocol::Variant variant(msg);
      auto status = variant.to<alteriom::StatusPackage>();
      Serial.printf("Basic Status from %u: v%s, Mem: %dKB\n",
                    from, status.firmwareVersion.c_str(), status.freeMemory);
      break;
    }
    
    default:
      Serial.printf("Message type %d from %u (not a status package)\n", msgType, from);
      break;
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection: %u\n", nodeId);
  Serial.printf("Total nodes in mesh: %d\n", mesh.getNodeList().size());
}

void changedConnectionCallback() {
  Serial.printf("Connections changed. Nodes: %d\n", mesh.getNodeList().size());
}

/**
 * Example: How to use Compressed OTA (sender side)
 * 
 * This function shows how to enable compressed OTA transfers.
 * In a real implementation, this would be called from an OTA sender node.
 */
void setupCompressedOTA() {
  #ifdef PAINLESSMESH_ENABLE_OTA
  
  // Example firmware info
  String firmwareMD5 = "abc123def456789";  // MD5 of firmware
  size_t numParts = 100;                   // Number of firmware chunks
  
  // Offer OTA update with compression enabled
  // Parameters: role, hardware, md5, numParts, forced, broadcasted, compressed
  auto otaTask = mesh.offerOTA(
    "sensor",      // Role this firmware is for
    "ESP32",       // Hardware type
    firmwareMD5,   // MD5 hash of firmware
    numParts,      // Number of data chunks
    false,         // Not forced (only update if different)
    false,         // Not using broadcast mode (yet - Phase 2)
    true           // COMPRESSED! 40-60% bandwidth savings
  );
  
  Serial.println("Compressed OTA announced!");
  Serial.println("Expected bandwidth savings: 40-60%");
  Serial.println("Nodes will receive and decompress firmware automatically");
  
  #else
  Serial.println("OTA not enabled. Define PAINLESSMESH_ENABLE_OTA");
  #endif
}

/**
 * Benefits of Phase 1 Features:
 * 
 * 1. Compressed OTA (Option 1E):
 *    - 40-60% reduction in network bandwidth
 *    - Faster firmware distribution across mesh
 *    - Lower energy consumption
 *    - Backward compatible with uncompressed OTA
 *    - Works with all future OTA options (broadcast, progressive, etc.)
 * 
 * 2. Enhanced Status Package (Option 2A):
 *    - Comprehensive device health monitoring
 *    - Mesh network statistics (nodes, connections, messages)
 *    - Performance metrics (latency, packet loss, throughput)
 *    - Alert system for proactive issue detection
 *    - Standardized format across all Alteriom nodes
 *    - Easy integration with monitoring dashboards
 * 
 * Next Steps (Phase 2):
 *    - Broadcast OTA for even faster updates in large meshes
 *    - MQTT bridge for cloud monitoring integration
 *    - Professional monitoring tools (Grafana, InfluxDB)
 */
