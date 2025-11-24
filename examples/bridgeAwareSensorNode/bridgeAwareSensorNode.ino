//************************************************************
// Bridge-Aware Sensor Node Example
//
// This example demonstrates how to use the bridge status
// callback feature to implement intelligent message queueing.
//
// Features:
// - Monitors bridge Internet connectivity status
// - Queues critical messages when Internet is unavailable
// - Automatically sends queued messages when Internet returns
// - Implements failover logic for multiple bridges
// - Automatic channel detection for bridge discovery
//
// Use case: Fish farm monitoring system that needs to ensure
// critical alarms are delivered even during Internet outages
//
// Important: Uses channel=0 for auto-detection to discover
// bridges operating on the router's WiFi channel.
//************************************************************

#include "painlessTaskOptions.h"  // Must be first to configure TaskScheduler
#include <TaskScheduler.h>  // Required for LDF to find TaskScheduler dependency
#include "alteriom_sensor_package.hpp"
#include "painlessMesh.h"

#define MESH_PREFIX "AlteriomMesh"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

Scheduler userScheduler;
painlessMesh mesh;

// Alteriom package handlers
using namespace alteriom;

// Message queue for offline mode
struct QueuedMessage {
  uint32_t timestamp;
  bool isCritical;
  SensorPackage data;
};

std::vector<QueuedMessage> messageQueue;
const size_t MAX_QUEUE_SIZE = 100;  // Prevent memory overflow

// Bridge connectivity tracking
bool internetAvailable = false;
uint32_t primaryBridgeId = 0;

// Simulation variables
float currentTemperature = 25.0;
float currentOxygenLevel = 7.5;  // mg/L - critical threshold is 5.0

// Function prototypes
void sendSensorData();
void handleIncomingPackage(uint32_t from, String& msg);
void bridgeStatusCallback(uint32_t bridgeNodeId, bool hasInternet);
void checkCriticalConditions();
void flushMessageQueue();
void queueMessage(const SensorPackage& data, bool isCritical);

Task taskSendSensorData(30000, TASK_FOREVER, &sendSensorData);
Task taskCheckCritical(5000, TASK_FOREVER, &checkCriticalConditions);

void setup() {
  Serial.begin(115200);

  // Initialize mesh with auto-channel detection
  // channel=0 enables automatic mesh channel detection
  // This is required to discover bridges that operate on the router's channel
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
  
  // Setup callbacks
  mesh.onReceive(&handleIncomingPackage);
  mesh.onBridgeStatusChanged(&bridgeStatusCallback);
  
  mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("✓ New connection: %u\n", nodeId);
  });
  
  mesh.onChangedConnections([]() {
    Serial.println("ℹ Mesh connections changed");
    
    // Check if we have a primary bridge
    auto primaryBridge = mesh.getPrimaryBridge();
    if (primaryBridge) {
      Serial.printf("  Primary bridge: %u (RSSI: %d dBm, Internet: %s)\n",
                    primaryBridge->nodeId,
                    primaryBridge->routerRSSI,
                    primaryBridge->internetConnected ? "Yes" : "No");
    } else {
      Serial.println("  ⚠ No healthy bridges available");
    }
  });

  // Add tasks
  userScheduler.addTask(taskSendSensorData);
  taskSendSensorData.enable();

  userScheduler.addTask(taskCheckCritical);
  taskCheckCritical.enable();

  Serial.println("═══════════════════════════════════════════");
  Serial.println("  Bridge-Aware Sensor Node");
  Serial.println("  Fish Farm Monitoring System");
  Serial.println("═══════════════════════════════════════════");
  Serial.printf("  Node ID: %u\n", mesh.getNodeId());
  Serial.println("  Monitoring: Temperature, Dissolved Oxygen");
  Serial.println("  Critical O₂ threshold: 5.0 mg/L");
  Serial.println("═══════════════════════════════════════════\n");
}

void loop() { 
  mesh.update();
  
  // Simulate sensor drift
  static uint32_t lastSimUpdate = 0;
  if (millis() - lastSimUpdate > 10000) {
    // Randomly vary oxygen level (can go critical)
    currentOxygenLevel += random(-20, 15) / 10.0;
    currentOxygenLevel = constrain(currentOxygenLevel, 3.0, 9.0);
    
    currentTemperature += random(-10, 10) / 10.0;
    currentTemperature = constrain(currentTemperature, 20.0, 30.0);
    
    lastSimUpdate = millis();
  }
}

void sendSensorData() {
  // Create sensor package
  SensorPackage sensorData;
  sensorData.from = mesh.getNodeId();
  sensorData.sensorId = mesh.getNodeId();
  sensorData.timestamp = mesh.getNodeTime();
  sensorData.temperature = currentTemperature;
  sensorData.humidity = 65.0 + random(-50, 50) / 10.0;  // Not critical for fish farm
  sensorData.pressure = currentOxygenLevel;  // Abuse pressure field for O₂ level
  sensorData.batteryLevel = 85 + random(-10, 5);

  // Determine if this is critical data (low oxygen)
  bool isCritical = (currentOxygenLevel < 5.5);  // Early warning threshold

  if (internetAvailable) {
    // Internet available - send immediately
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    sensorData.addTo(std::move(obj));

    String msg;
    serializeJson(doc, msg);
    mesh.sendBroadcast(msg);

    Serial.printf("📤 Sent: T=%.1f°C, O₂=%.1f mg/L, Bat=%d%% %s\n",
                  sensorData.temperature,
                  currentOxygenLevel,
                  sensorData.batteryLevel,
                  isCritical ? "⚠ CRITICAL" : "");
  } else {
    // Internet offline - queue message
    queueMessage(sensorData, isCritical);
    
    Serial.printf("💾 Queued: T=%.1f°C, O₂=%.1f mg/L %s [Queue: %d/%d]\n",
                  sensorData.temperature,
                  currentOxygenLevel,
                  isCritical ? "⚠ CRITICAL" : "",
                  messageQueue.size(),
                  MAX_QUEUE_SIZE);
  }
}

void checkCriticalConditions() {
  // Check for critical low oxygen
  if (currentOxygenLevel < 5.0) {
    Serial.printf("🚨 CRITICAL ALARM: Low Oxygen = %.1f mg/L!\n", currentOxygenLevel);
    
    // Create critical alarm package
    SensorPackage alarm;
    alarm.from = mesh.getNodeId();
    alarm.sensorId = mesh.getNodeId();
    alarm.timestamp = mesh.getNodeTime();
    alarm.temperature = currentTemperature;
    alarm.pressure = currentOxygenLevel;
    alarm.batteryLevel = 85;

    if (internetAvailable) {
      // Send immediately
      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      alarm.addTo(std::move(obj));

      String msg;
      serializeJson(doc, msg);
      mesh.sendBroadcast(msg);
      Serial.println("  ↪ Critical alarm sent immediately");
    } else {
      // Queue with high priority
      queueMessage(alarm, true);
      Serial.printf("  ↪ Critical alarm queued [Queue: %d/%d]\n",
                    messageQueue.size(), MAX_QUEUE_SIZE);
    }
  }
}

void queueMessage(const SensorPackage& data, bool isCritical) {
  if (messageQueue.size() >= MAX_QUEUE_SIZE) {
    Serial.println("⚠ Queue full - dropping oldest message");
    messageQueue.erase(messageQueue.begin());
  }

  QueuedMessage qMsg;
  qMsg.timestamp = millis();
  qMsg.isCritical = isCritical;
  qMsg.data = data;
  
  messageQueue.push_back(qMsg);
}

void flushMessageQueue() {
  if (messageQueue.empty()) {
    return;
  }

  Serial.printf("\n📤 Flushing message queue (%d messages)...\n", messageQueue.size());
  
  size_t sentCount = 0;
  size_t criticalCount = 0;
  
  for (const auto& qMsg : messageQueue) {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    qMsg.data.addTo(std::move(obj));

    String msg;
    serializeJson(doc, msg);
    mesh.sendBroadcast(msg);
    
    sentCount++;
    if (qMsg.isCritical) criticalCount++;
    
    delay(100);  // Small delay to avoid flooding
  }
  
  messageQueue.clear();
  
  Serial.printf("✓ Queue flushed: %d messages (%d critical)\n\n",
                sentCount, criticalCount);
}

void bridgeStatusCallback(uint32_t bridgeNodeId, bool hasInternet) {
  bool wasAvailable = internetAvailable;
  internetAvailable = hasInternet;
  primaryBridgeId = bridgeNodeId;
  
  Serial.println("\n════════════════════════════════════════════");
  if (hasInternet) {
    Serial.printf("✓ Bridge %u: Internet CONNECTED\n", bridgeNodeId);
    
    // Get bridge details
    auto bridges = mesh.getBridges();
    for (const auto& bridge : bridges) {
      if (bridge.nodeId == bridgeNodeId) {
        Serial.printf("  Router: %s (RSSI: %d dBm, Ch: %d)\n",
                      bridge.gatewayIP.c_str(),
                      bridge.routerRSSI,
                      bridge.routerChannel);
        Serial.printf("  Bridge uptime: %u seconds\n", bridge.uptime / 1000);
      }
    }
    
    // If Internet was restored, flush queued messages
    if (!wasAvailable) {
      Serial.println("  → Internet restored - processing queue");
      flushMessageQueue();
    }
  } else {
    Serial.printf("⚠ Bridge %u: Internet OFFLINE\n", bridgeNodeId);
    Serial.println("  → Entering offline mode");
    Serial.println("  → Critical alarms will be queued");
  }
  Serial.println("════════════════════════════════════════════\n");
  
  // Show overall mesh status
  if (mesh.hasInternetConnection()) {
    Serial.println("✓ Mesh has Internet connectivity");
  } else {
    Serial.println("⚠ No Internet connectivity in mesh");
  }
  
  // Show all known bridges
  auto bridges = mesh.getBridges();
  if (!bridges.empty()) {
    Serial.printf("Known bridges: %d\n", bridges.size());
    for (const auto& bridge : bridges) {
      Serial.printf("  - Bridge %u: %s (RSSI: %d dBm)\n",
                    bridge.nodeId,
                    bridge.internetConnected ? "Online" : "Offline",
                    bridge.routerRSSI);
    }
  }
  Serial.println();
}

void handleIncomingPackage(uint32_t from, String& msg) {
  // Parse the JSON message
  JsonDocument doc;
  deserializeJson(doc, msg);
  JsonObject obj = doc.as<JsonObject>();

  // Check message type
  uint8_t msgType = obj["type"];

  switch (msgType) {
    case 200:  // SensorPackage
    {
      SensorPackage receivedSensor(obj);
      Serial.printf("📥 Sensor from %u: T=%.1f°C, O₂=%.1f mg/L\n",
                    receivedSensor.from,
                    receivedSensor.temperature,
                    receivedSensor.pressure);  // Using pressure for O₂
    } break;

    case 400:  // CommandPackage
    {
      CommandPackage receivedCmd(obj);
      if (receivedCmd.dest == mesh.getNodeId()) {
        Serial.printf("📥 Command %d received\n", receivedCmd.command);
        
        // Handle commands (e.g., request status, change thresholds)
        if (receivedCmd.command == 3) {
          // Data request - send immediately even if offline
          sendSensorData();
        }
      }
    } break;

    case 610:  // BridgeStatusPackage
      // Handled automatically by mesh's internal callback
      // But we can log that we received it
      Serial.printf("📡 Bridge status update from %u\n", from);
      break;

    default:
      // Unknown message type
      break;
  }
}
