//************************************************************
// Queued Alarms Example - Message Queueing for Offline Mode
//
// Demonstrates priority-based message queueing for critical alarms
// when Internet connection is unavailable. Perfect for IoT systems
// that cannot afford to lose critical data.
//
// Use Case: Fish farm dissolved oxygen monitoring
// - CRITICAL alarms (low O2) must never be lost
// - Queue messages during Internet outages
// - Automatic delivery when connection restored
//
// Hardware: ESP32 or ESP8266
//************************************************************

#include "painlessMesh.h"

// Mesh configuration
#define   MESH_PREFIX     "FishFarmMesh"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

// Router credentials for bridge node
#define   ROUTER_SSID     "YourWiFiSSID"
#define   ROUTER_PASSWORD "YourWiFiPassword"

// Sensor thresholds (mg/L for dissolved oxygen)
#define   CRITICAL_O2_THRESHOLD  3.0
#define   WARNING_O2_THRESHOLD   5.0

// Queue configuration
#define   MAX_QUEUE_SIZE  500
#define   QUEUE_PRUNE_AGE (24 * 60 * 60 * 1000)  // 24 hours in ms

Scheduler userScheduler;
painlessMesh mesh;

bool offlineMode = false;
uint32_t lastO2Check = 0;
uint32_t lastQueuePrune = 0;

// Simulated sensor reading (replace with actual sensor code)
float readDissolvedOxygenSensor() {
  // In real application, read from actual sensor
  // For demo, simulate varying O2 levels
  static float o2Level = 7.0;
  o2Level += random(-20, 20) / 10.0;  // +/- 2.0 mg/L variation
  o2Level = constrain(o2Level, 2.0, 10.0);
  return o2Level;
}

// Send critical O2 alarm
void sendCriticalAlarm(float o2Level) {
  // Create alarm message (in real app, use JSON)
  String payload = String("{\"type\":\"CRITICAL_ALARM\",\"sensor\":\"O2\",\"value\":") 
                   + String(o2Level, 2) + ",\"threshold\":" 
                   + String(CRITICAL_O2_THRESHOLD, 2) + ",\"tankId\":\"TANK_A\",\"nodeId\":" 
                   + mesh.getNodeId() + ",\"timestamp\":" + mesh.getNodeTime() + "}";
  
  if (offlineMode || !mesh.hasInternetConnection()) {
    // CRITICAL: Queue for guaranteed delivery
    uint32_t msgId = mesh.queueMessage(
      payload,
      "mqtt://cloud.farm.com/alarms/critical",
      PRIORITY_CRITICAL
    );
    
    if (msgId) {
      Serial.printf("🚨 CRITICAL O2 ALARM: %.2f mg/L - QUEUED #%u\n", o2Level, msgId);
    } else {
      Serial.printf("🚨 CRITICAL O2 ALARM: %.2f mg/L - QUEUE FAILED!\n", o2Level);
    }
  } else {
    // Send immediately via bridge (in real app, use MQTT client)
    Serial.printf("🚨 CRITICAL O2 ALARM: %.2f mg/L - SENT IMMEDIATELY\n", o2Level);
    // mqttClient.publish("alarms/critical", payload.c_str());
  }
}

// Send warning alarm
void sendWarningAlarm(float o2Level) {
  String payload = String("{\"type\":\"WARNING\",\"sensor\":\"O2\",\"value\":") 
                   + String(o2Level, 2) + ",\"threshold\":" 
                   + String(WARNING_O2_THRESHOLD, 2) + ",\"nodeId\":" 
                   + mesh.getNodeId() + "}";
  
  if (offlineMode) {
    uint32_t msgId = mesh.queueMessage(
      payload,
      "mqtt://cloud.farm.com/alarms/warning",
      PRIORITY_HIGH
    );
    
    if (msgId) {
      Serial.printf("⚠️  WARNING O2: %.2f mg/L - QUEUED #%u\n", o2Level, msgId);
    } else {
      Serial.printf("⚠️  WARNING O2: %.2f mg/L - QUEUE FULL\n", o2Level);
    }
  } else {
    Serial.printf("⚠️  WARNING O2: %.2f mg/L - SENT\n", o2Level);
  }
}

// Send normal telemetry
void sendNormalTelemetry(float o2Level) {
  String payload = String("{\"sensor\":\"O2\",\"value\":") + String(o2Level, 2) 
                   + ",\"nodeId\":" + mesh.getNodeId() + "}";
  
  if (offlineMode) {
    // Low priority - queue only if space available
    uint32_t msgId = mesh.queueMessage(
      payload,
      "mqtt://cloud.farm.com/telemetry",
      PRIORITY_LOW
    );
    
    if (msgId) {
      Serial.printf("📊 Telemetry: %.2f mg/L - queued #%u\n", o2Level, msgId);
    } else {
      Serial.printf("📊 Telemetry: %.2f mg/L - dropped (queue full)\n", o2Level);
    }
  } else {
    Serial.printf("📊 Telemetry: %.2f mg/L\n", o2Level);
  }
}

// Check O2 sensor and send appropriate message
void checkO2Sensor() {
  float o2Level = readDissolvedOxygenSensor();
  
  if (o2Level < CRITICAL_O2_THRESHOLD) {
    sendCriticalAlarm(o2Level);
  } else if (o2Level < WARNING_O2_THRESHOLD) {
    sendWarningAlarm(o2Level);
  } else {
    sendNormalTelemetry(o2Level);
  }
}

// Bridge status callback - Internet connectivity changed
void bridgeStatusCallback(uint32_t bridgeNodeId, bool hasInternet) {
  if (!hasInternet) {
    offlineMode = true;
    Serial.println("\n⚠️  OFFLINE MODE ACTIVATED");
    Serial.printf("  Bridge node %u lost Internet\n", bridgeNodeId);
    Serial.printf("  Queue size: %u messages\n", mesh.getQueuedMessageCount());
    
    uint32_t critical = mesh.getQueuedMessageCount(PRIORITY_CRITICAL);
    if (critical > 0) {
      Serial.printf("  ⚠️  %u CRITICAL messages queued!\n", critical);
    }
  } else {
    offlineMode = false;
    Serial.println("\n✅ ONLINE MODE - Internet restored");
    Serial.printf("  Bridge node %u has Internet\n", bridgeNodeId);
    
    // Flush queued messages
    uint32_t queuedCount = mesh.getQueuedMessageCount();
    if (queuedCount > 0) {
      Serial.printf("  Flushing %u queued messages...\n", queuedCount);
      
      auto messages = mesh.flushMessageQueue();
      for (auto& msg : messages) {
        // In real application, send via MQTT or HTTP
        Serial.printf("  Sending queued message #%u (priority=%d, attempts=%u)\n",
                     msg.id, msg.priority, msg.attempts);
        
        // Simulate sending (in real app, check if send succeeded)
        bool sent = true;  // Replace with: mqttClient.publish(...)
        
        if (sent) {
          mesh.removeQueuedMessage(msg.id);
        } else {
          // Increment attempt counter
          mesh.incrementQueuedMessageAttempts(msg.id);
          
          // Remove if too many attempts
          if (msg.attempts >= 3) {
            Serial.printf("  ❌ Message #%u failed after 3 attempts, removing\n", msg.id);
            mesh.removeQueuedMessage(msg.id);
          }
        }
      }
      
      Serial.printf("  ✅ Queue flushed (%u messages sent)\n", queuedCount);
    }
  }
}

// Queue state callback - Monitor queue health
void queueStateCallback(QueueState state, uint32_t messageCount) {
  switch (state) {
    case QUEUE_EMPTY:
      Serial.println("ℹ️  Queue empty");
      break;
    case QUEUE_NORMAL:
      Serial.printf("ℹ️  Queue normal (%u messages)\n", messageCount);
      break;
    case QUEUE_75_PERCENT:
      Serial.printf("⚠️  Queue 75%% full (%u messages)\n", messageCount);
      break;
    case QUEUE_FULL:
      Serial.printf("🚨 Queue FULL (%u messages) - dropping LOW priority\n", messageCount);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Fish Farm O2 Monitoring with Message Queue ===\n");
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // For bridge node: set router credentials
  // Uncomment if this is the bridge node
  // mesh.stationManual(ROUTER_SSID, ROUTER_PASSWORD);
  // mesh.setHostname("FishFarmBridge");
  
  // Enable message queue
  mesh.enableMessageQueue(true, MAX_QUEUE_SIZE);
  Serial.printf("Message queue enabled (capacity: %u)\n", MAX_QUEUE_SIZE);
  
  // Set callbacks
  mesh.onBridgeStatusChanged(&bridgeStatusCallback);
  mesh.onQueueStateChanged(&queueStateCallback);
  
  Serial.println("Setup complete. Monitoring O2 levels...\n");
  
  // Initialize random for sensor simulation
  randomSeed(analogRead(0));
}

void loop() {
  mesh.update();
  
  // Check O2 sensor every 10 seconds
  if (millis() - lastO2Check > 10000) {
    lastO2Check = millis();
    checkO2Sensor();
    
    // Print queue status
    uint32_t queueSize = mesh.getQueuedMessageCount();
    if (queueSize > 0) {
      Serial.printf("  [Queue: %u messages", queueSize);
      
      uint32_t critical = mesh.getQueuedMessageCount(PRIORITY_CRITICAL);
      if (critical > 0) {
        Serial.printf(" (%u CRITICAL)", critical);
      }
      
      Serial.println("]");
    }
  }
  
  // Prune old messages every hour
  if (millis() - lastQueuePrune > 3600000) {
    lastQueuePrune = millis();
    uint32_t pruned = mesh.pruneQueue(QUEUE_PRUNE_AGE);
    if (pruned > 0) {
      Serial.printf("ℹ️  Pruned %u old messages from queue\n", pruned);
    }
  }
}
