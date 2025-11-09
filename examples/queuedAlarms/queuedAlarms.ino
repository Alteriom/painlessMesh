/**
 * Queued Alarms Example - Fish Farm Oxygen Monitoring
 * 
 * This example demonstrates message queuing for critical IoT applications where
 * message loss during Internet outages is unacceptable.
 * 
 * Scenario: Fish farm dissolved oxygen monitoring with critical alarm delivery
 * - Monitors dissolved oxygen levels in fish tanks
 * - Critical alarms must be delivered even if Internet connection drops
 * - Messages are queued with priority levels and delivered when Internet restored
 * - Persistent storage ensures alarms survive device reboots
 * 
 * Hardware Requirements:
 * - ESP32 or ESP8266
 * - Dissolved oxygen sensor (simulated in this example)
 * - WiFi connection to router with Internet access
 * 
 * Key Features Demonstrated:
 * - Priority-based message queuing (CRITICAL, HIGH, NORMAL, LOW)
 * - Automatic queue flushing when Internet connectivity restored
 * - Persistent storage for critical messages (SPIFFS/LittleFS)
 * - Queue state monitoring and management
 * - Integration with bridge status callbacks
 */

#include "painlessMesh.h"
#include <ArduinoJson.h>

// Mesh configuration
#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_mesh_password"
#define MESH_PORT       5555

// Router configuration (for bridge node with Internet access)
#define ROUTER_SSID     "your_router_ssid"
#define ROUTER_PASSWORD "your_router_password"

// Oxygen thresholds (mg/L)
#define CRITICAL_O2_THRESHOLD  3.0   // Below this = fish die
#define WARNING_O2_THRESHOLD   5.0   // Below this = warning
#define NORMAL_O2_MIN          6.0   // Healthy range
#define NORMAL_O2_MAX          8.0

// Timing
#define SENSOR_READ_INTERVAL   10000  // Read sensor every 10 seconds
#define TELEMETRY_INTERVAL     60000  // Send telemetry every 60 seconds

Scheduler userScheduler;
painlessMesh mesh;

// State tracking
bool offlineMode = false;
uint32_t lastSensorRead = 0;
uint32_t lastTelemetrySent = 0;
float currentO2Level = 7.5;  // Current dissolved oxygen level

// Simulated sensor reading (replace with actual sensor code)
float readDissolvedOxygenSensor() {
  // Simulate sensor reading with some variation
  // In real application, read from actual DO sensor via I2C or analog input
  static float baseValue = 7.5;
  float noise = (random(-100, 100) / 100.0);
  baseValue += noise * 0.1;
  
  // Clamp to realistic range
  if (baseValue < 0) baseValue = 0;
  if (baseValue > 12) baseValue = 12;
  
  return baseValue;
}

// Simulate actual message sending to cloud (MQTT, HTTP, etc.)
bool sendMessageToCloud(const String& payload, const String& destination) {
  // In real application, this would:
  // - Send via MQTT: mqttClient.publish(destination.c_str(), payload.c_str());
  // - Or send via HTTP: httpClient.POST(destination, payload);
  // - Return true if successful, false if failed
  
  Serial.printf("☁️  Sending to cloud [%s]: %s\n", destination.c_str(), payload.c_str());
  
  // Simulate success if we have Internet (not in offline mode)
  return !offlineMode;
}

// Create critical oxygen alarm message
void sendCriticalAlarm(float o2Level) {
  DynamicJsonDocument doc(256);
  doc["type"] = "alarm";
  doc["severity"] = "CRITICAL";
  doc["tankId"] = "TANK_A";
  doc["sensorType"] = "dissolved_oxygen";
  doc["value"] = o2Level;
  doc["threshold"] = CRITICAL_O2_THRESHOLD;
  doc["timestamp"] = mesh.getNodeTime();
  doc["nodeId"] = mesh.getNodeId();
  doc["message"] = "CRITICAL: Dissolved oxygen below survival threshold!";
  
  String payload;
  serializeJson(doc, payload);
  String destination = "mqtt://cloud.farm.com/alarms/critical";
  
  if (offlineMode || !mesh.hasInternetConnection()) {
    // Queue as CRITICAL - must be delivered, never dropped
    uint32_t msgId = mesh.queueMessage(payload, destination, 
                                      painlessmesh::queue::PRIORITY_CRITICAL);
    
    Serial.printf("🚨 CRITICAL O2 ALARM: %.2f mg/L - QUEUED #%u (offline mode)\n", 
                  o2Level, msgId);
    
    // Immediately save to persistent storage for critical messages
    mesh.saveQueueToStorage();
    
    // Also try to alert locally (buzzer, LED, etc.)
    // digitalWrite(ALARM_PIN, HIGH);
    
  } else {
    // Send immediately if online
    if (sendMessageToCloud(payload, destination)) {
      Serial.printf("🚨 CRITICAL O2 ALARM: %.2f mg/L - SENT IMMEDIATELY\n", o2Level);
    } else {
      // Failed to send, queue it
      uint32_t msgId = mesh.queueMessage(payload, destination,
                                        painlessmesh::queue::PRIORITY_CRITICAL);
      Serial.printf("🚨 CRITICAL O2 ALARM: %.2f mg/L - SEND FAILED, QUEUED #%u\n",
                    o2Level, msgId);
    }
  }
}

// Create warning message
void sendWarningAlarm(float o2Level) {
  DynamicJsonDocument doc(256);
  doc["type"] = "alarm";
  doc["severity"] = "WARNING";
  doc["tankId"] = "TANK_A";
  doc["sensorType"] = "dissolved_oxygen";
  doc["value"] = o2Level;
  doc["threshold"] = WARNING_O2_THRESHOLD;
  doc["timestamp"] = mesh.getNodeTime();
  doc["nodeId"] = mesh.getNodeId();
  doc["message"] = "WARNING: Dissolved oxygen below recommended level";
  
  String payload;
  serializeJson(doc, payload);
  String destination = "mqtt://cloud.farm.com/alarms/warning";
  
  if (offlineMode) {
    // Queue as HIGH priority
    uint32_t msgId = mesh.queueMessage(payload, destination,
                                      painlessmesh::queue::PRIORITY_HIGH);
    if (msgId) {
      Serial.printf("⚠️  WARNING O2 ALARM: %.2f mg/L - QUEUED #%u\n", o2Level, msgId);
    }
  } else {
    if (sendMessageToCloud(payload, destination)) {
      Serial.printf("⚠️  WARNING O2 ALARM: %.2f mg/L - SENT\n", o2Level);
    }
  }
}

// Send normal telemetry
void sendNormalTelemetry(float o2Level) {
  DynamicJsonDocument doc(192);
  doc["type"] = "telemetry";
  doc["tankId"] = "TANK_A";
  doc["sensorType"] = "dissolved_oxygen";
  doc["value"] = o2Level;
  doc["timestamp"] = mesh.getNodeTime();
  doc["nodeId"] = mesh.getNodeId();
  
  String payload;
  serializeJson(doc, payload);
  String destination = "mqtt://cloud.farm.com/telemetry";
  
  if (offlineMode) {
    // Queue as LOW priority - may be dropped if queue fills
    uint32_t msgId = mesh.queueMessage(payload, destination,
                                      painlessmesh::queue::PRIORITY_LOW);
    if (msgId) {
      Serial.printf("📊 Telemetry queued #%u: O2=%.2f mg/L\n", msgId, o2Level);
    } else {
      Serial.println("📊 Telemetry dropped (queue full)");
    }
  } else {
    if (sendMessageToCloud(payload, destination)) {
      Serial.printf("📊 Telemetry sent: O2=%.2f mg/L\n", o2Level);
    }
  }
}

// Check oxygen level and send appropriate message
void checkOxygenLevel() {
  currentO2Level = readDissolvedOxygenSensor();
  
  if (currentO2Level < CRITICAL_O2_THRESHOLD) {
    sendCriticalAlarm(currentO2Level);
  } else if (currentO2Level < WARNING_O2_THRESHOLD) {
    sendWarningAlarm(currentO2Level);
  } else {
    // Only send telemetry at specified interval for normal readings
    if (millis() - lastTelemetrySent > TELEMETRY_INTERVAL) {
      sendNormalTelemetry(currentO2Level);
      lastTelemetrySent = millis();
    }
  }
}

// Bridge status changed callback
void bridgeStatusCallback(uint32_t bridgeNodeId, bool hasInternet) {
  Serial.printf("\n=== Bridge Status Update ===\n");
  Serial.printf("Bridge Node: %u\n", bridgeNodeId);
  Serial.printf("Internet: %s\n", hasInternet ? "Connected" : "Disconnected");
  
  if (!hasInternet) {
    offlineMode = true;
    uint32_t queuedCount = mesh.getQueuedMessageCount();
    Serial.printf("⚠️  OFFLINE MODE ACTIVATED\n");
    Serial.printf("   Queue size: %u messages\n", queuedCount);
    Serial.printf("   Critical messages: %u\n", 
                  mesh.getQueuedMessageCount(painlessmesh::queue::PRIORITY_CRITICAL));
  } else {
    offlineMode = false;
    Serial.printf("✓ ONLINE MODE - Internet restored\n");
    
    uint32_t queuedCount = mesh.getQueuedMessageCount();
    if (queuedCount > 0) {
      Serial.printf("   Flushing %u queued messages...\n", queuedCount);
      
      // Flush queue with our send callback
      uint32_t sent = mesh.flushMessageQueue(sendMessageToCloud);
      
      Serial.printf("✓ Sent %u queued messages\n", sent);
      
      // Save queue state after flushing
      mesh.saveQueueToStorage();
    }
  }
  Serial.println("===========================\n");
}

// Queue state changed callback
void queueStateCallback(painlessmesh::queue::QueueState state, uint32_t messageCount) {
  switch (state) {
    case painlessmesh::queue::QUEUE_EMPTY:
      Serial.println("✓ Message queue empty");
      break;
    case painlessmesh::queue::QUEUE_25_PERCENT:
      Serial.printf("ℹ️  Queue 25%% full (%u messages)\n", messageCount);
      break;
    case painlessmesh::queue::QUEUE_50_PERCENT:
      Serial.printf("ℹ️  Queue 50%% full (%u messages)\n", messageCount);
      break;
    case painlessmesh::queue::QUEUE_75_PERCENT:
      Serial.printf("⚠️  Queue 75%% full (%u messages)\n", messageCount);
      break;
    case painlessmesh::queue::QUEUE_FULL:
      Serial.printf("🚨 Queue FULL (%u messages) - dropping LOW priority\n", messageCount);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
  Serial.println("\n\n==============================================");
  Serial.println("Fish Farm Oxygen Monitoring - Queued Alarms");
  Serial.println("==============================================\n");
  
  // Initialize SPIFFS for persistent queue storage
  if (!SPIFFS.begin(true)) {
    Serial.println("⚠️  SPIFFS initialization failed!");
    Serial.println("   Queue will not persist across reboots");
  } else {
    Serial.println("✓ SPIFFS initialized for persistent queue");
  }
  
  // Initialize mesh
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Configure as bridge node (optional - only if this node has router access)
  // Comment out these lines for regular sensor nodes
  mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
  mesh.initBridgeStatusBroadcast();
  mesh.enableBridgeFailover(true);
  
  // Enable message queue with persistent storage
  mesh.enableMessageQueue(
    500,                                    // Max 500 messages
    true,                                   // Enable persistence
    "/painlessmesh/o2_alarm_queue.dat"    // Storage path
  );
  
  // Configure queue settings
  mesh.setMaxQueueRetryAttempts(3);        // Try sending 3 times
  
  // Set callbacks
  mesh.onBridgeStatusChanged(&bridgeStatusCallback);
  mesh.onQueueStateChanged(&queueStateCallback);
  
  // Load any queued messages from previous session
  uint32_t loaded = mesh.loadQueueFromStorage();
  if (loaded > 0) {
    Serial.printf("✓ Restored %u queued messages from storage\n", loaded);
  }
  
  // Print queue statistics
  painlessmesh::queue::QueueStats stats = mesh.getQueueStats();
  Serial.printf("\nQueue Statistics:\n");
  Serial.printf("  Total Queued: %u\n", stats.totalQueued);
  Serial.printf("  Total Sent: %u\n", stats.totalSent);
  Serial.printf("  Total Dropped: %u\n", stats.totalDropped);
  Serial.printf("  Current Size: %u\n", stats.currentSize);
  Serial.printf("  Peak Size: %u\n\n", stats.peakSize);
  
  Serial.println("✓ System ready - monitoring oxygen levels\n");
}

void loop() {
  mesh.update();
  
  // Check oxygen sensor at specified interval
  if (millis() - lastSensorRead > SENSOR_READ_INTERVAL) {
    lastSensorRead = millis();
    checkOxygenLevel();
  }
  
  // Periodically report queue status (every 5 minutes)
  static uint32_t lastStatusReport = 0;
  if (millis() - lastStatusReport > 300000) {
    lastStatusReport = millis();
    
    uint32_t queuedCount = mesh.getQueuedMessageCount();
    if (queuedCount > 0) {
      Serial.printf("\n📊 Queue Status: %u messages pending\n", queuedCount);
      Serial.printf("   CRITICAL: %u\n", 
                    mesh.getQueuedMessageCount(painlessmesh::queue::PRIORITY_CRITICAL));
      Serial.printf("   HIGH: %u\n",
                    mesh.getQueuedMessageCount(painlessmesh::queue::PRIORITY_HIGH));
      Serial.printf("   NORMAL: %u\n",
                    mesh.getQueuedMessageCount(painlessmesh::queue::PRIORITY_NORMAL));
      Serial.printf("   LOW: %u\n\n",
                    mesh.getQueuedMessageCount(painlessmesh::queue::PRIORITY_LOW));
    }
  }
  
  // Prune old messages (older than 24 hours) periodically
  static uint32_t lastPrune = 0;
  if (millis() - lastPrune > 3600000) {  // Every hour
    lastPrune = millis();
    uint32_t pruned = mesh.pruneMessageQueue(24);
    if (pruned > 0) {
      Serial.printf("🗑️  Pruned %u messages older than 24 hours\n", pruned);
    }
  }
}
