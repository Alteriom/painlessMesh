/**
 * @file mqttTopologyTest.ino
 * @brief Hardware test sketch for painlessMesh topology reporting with MQTT
 * 
 * This sketch tests mesh topology reporting according to @alteriom/mqtt-schema v0.5.0
 * 
 * HARDWARE REQUIREMENTS:
 * - 3x ESP32 boards (or ESP8266, but ESP32 recommended)
 * - MQTT broker accessible on network (e.g., Mosquitto)
 * - USB cables for programming and Serial monitoring
 * 
 * TEST CONFIGURATION:
 * - Node 1: Gateway (this sketch with IS_GATEWAY = true)
 * - Node 2: Sensor node (this sketch with IS_GATEWAY = false, role = "sensor")
 * - Node 3: Repeater node (this sketch with IS_GATEWAY = false, role = "repeater")
 * 
 * MQTT TOPICS TESTED:
 * - alteriom/mesh/MESH-001/topology (full topology every 60s, incremental every 5s)
 * - alteriom/mesh/MESH-001/events (node join/leave events)
 * - alteriom/mesh/MESH-001/topology/response (GET_TOPOLOGY command response)
 * - alteriom/mesh/MESH-001/command (receive GET_TOPOLOGY commands)
 * 
 * SCHEMA COMPLIANCE:
 * - Device ID format: ALT-XXXXXXXXXXXX (uppercase hex)
 * - Quality metrics: 0-100 range
 * - RSSI values: negative dBm
 * - Timestamps: ISO 8601 format
 * - Envelope fields: schema_version, device_id, device_type, timestamp, firmware_version
 * 
 * TEST PROCEDURE:
 * 1. Configure WiFi credentials (WIFI_SSID, WIFI_PASSWORD)
 * 2. Configure MQTT broker (MQTT_BROKER, MQTT_PORT)
 * 3. Flash Node 1 with IS_GATEWAY = true
 * 4. Flash Node 2 with IS_GATEWAY = false, NODE_ROLE = "sensor"
 * 5. Flash Node 3 with IS_GATEWAY = false, NODE_ROLE = "repeater"
 * 6. Open Serial monitors for all 3 nodes
 * 7. Wait for mesh formation (watch for "Node joined" messages)
 * 8. Verify full topology published every 60s
 * 9. Verify incremental updates on node join/leave
 * 10. Send GET_TOPOLOGY command and verify response
 * 
 * EXPECTED BEHAVIOR:
 * - All nodes join mesh within 30 seconds
 * - Gateway publishes full topology every 60 seconds
 * - Gateway publishes incremental updates within 5 seconds of node join/leave
 * - All messages conform to @alteriom/mqtt-schema v0.5.0
 * - Connection quality metrics are calculated correctly
 * 
 * @author Alteriom Development Team
 * @date October 14, 2025
 * @version 1.0.0
 */

#include "painlessMesh.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ============================================================================
// CONFIGURATION - MODIFY THESE FOR YOUR SETUP
// ============================================================================

// Mesh Configuration
#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "alteriom2025"
#define MESH_PORT       5555

// WiFi Configuration (for gateway only)
#define WIFI_SSID       "YourWiFiSSID"
#define WIFI_PASSWORD   "YourWiFiPassword"

// MQTT Configuration (for gateway only)
#define MQTT_BROKER     "192.168.1.100"  // Your MQTT broker IP
#define MQTT_PORT       1883
#define MQTT_USER       ""  // Leave empty if no auth
#define MQTT_PASS       ""  // Leave empty if no auth

// Node Configuration - CHANGE THIS FOR EACH NODE
#define IS_GATEWAY      true              // Set to false for sensor/repeater nodes
#define NODE_ROLE       "gateway"         // "gateway", "sensor", or "repeater"
#define MESH_ID         "MESH-001"
#define FIRMWARE_VERSION "TEST 1.0.0"

// Topology Publishing Configuration
#define FULL_TOPOLOGY_INTERVAL    60000   // Full topology every 60 seconds
#define INCREMENTAL_INTERVAL      5000    // Check for changes every 5 seconds
#define COMMAND_CHECK_INTERVAL    1000    // Check for commands every 1 second

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

Scheduler userScheduler;
painlessMesh mesh;

#if IS_GATEWAY
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
#endif

// Task declarations
Task taskSendFullTopology(FULL_TOPOLOGY_INTERVAL, TASK_FOREVER, &sendFullTopology);
Task taskCheckIncrementalUpdate(INCREMENTAL_INTERVAL, TASK_FOREVER, &checkIncrementalUpdate);
Task taskCheckCommands(COMMAND_CHECK_INTERVAL, TASK_FOREVER, &checkCommands);

// Topology state tracking
struct NodeState {
  uint32_t nodeId;
  String role;
  bool online;
  unsigned long lastSeen;
  int connectionCount;
};

std::map<uint32_t, NodeState> lastTopology;
unsigned long lastTopologyUpdate = 0;

// ============================================================================
// DEVICE ID GENERATION
// ============================================================================

/**
 * Generate Alteriom-compliant device ID from chip ID
 * Format: ALT-XXXXXXXXXXXX (uppercase hex)
 */
String getDeviceId() {
  uint64_t chipid = ESP.getEfuseMac();  // ESP32 unique ID
  char deviceId[17];
  snprintf(deviceId, sizeof(deviceId), "ALT-%012llX", chipid);
  return String(deviceId);
}

// ============================================================================
// ISO 8601 TIMESTAMP
// ============================================================================

/**
 * Get current timestamp in ISO 8601 format
 * Note: Uses mesh time, not real-world time (no NTP in this test)
 */
String getTimestamp() {
  uint32_t meshTime = mesh.getNodeTime();
  // Convert microseconds to seconds
  uint32_t seconds = meshTime / 1000000;
  
  // Simple ISO 8601 format (no actual date, just offset from boot)
  char timestamp[30];
  snprintf(timestamp, sizeof(timestamp), "2025-10-14T%02d:%02d:%02dZ", 
           (seconds / 3600) % 24, 
           (seconds / 60) % 60, 
           seconds % 60);
  return String(timestamp);
}

// ============================================================================
// CONNECTION QUALITY CALCULATION
// ============================================================================

/**
 * Calculate connection quality (0-100) based on mesh metrics
 * This is a simplified calculation for testing
 */
int calculateQuality(uint32_t nodeId) {
  // In a real implementation, this would use:
  // - RSSI measurements
  // - Packet loss rate
  // - Latency measurements
  // For testing, we'll use a simulated value based on node ID
  
  // Simulate quality degradation with distance (hop count)
  SimpleList<uint32_t> nodeList = mesh.getNodeList();
  int nodeCount = nodeList.size();
  
  // Base quality: 95 for direct connections
  int quality = 95;
  
  // Reduce quality based on mesh size (simulate congestion)
  if (nodeCount > 3) quality -= (nodeCount - 3) * 5;
  
  // Add some variation based on node ID
  quality -= (nodeId % 10);
  
  // Clamp to 0-100 range
  return std::max(0, std::min(100, quality));
}

/**
 * Simulate RSSI (WiFi signal strength in dBm)
 * Real implementation would read actual WiFi RSSI
 */
int calculateRSSI(uint32_t nodeId) {
  // RSSI ranges from -30 (excellent) to -90 (poor)
  int rssi = -40 - (nodeId % 50);  // Simulated based on node ID
  return std::max(-90, std::min(-30, rssi));
}

/**
 * Simulate round-trip latency in milliseconds
 * Real implementation would measure actual ping times
 */
int calculateLatency(uint32_t nodeId) {
  // Latency increases with hop count
  int baseLatency = 10;
  int variation = nodeId % 20;
  return baseLatency + variation;
}

// ============================================================================
// MQTT PUBLISHING (GATEWAY ONLY)
// ============================================================================

#if IS_GATEWAY

/**
 * Connect to MQTT broker
 */
void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker...");
    
    String clientId = "AlteriomGateway-" + getDeviceId();
    
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println(" connected!");
      
      // Subscribe to command topic
      String commandTopic = "alteriom/mesh/" + String(MESH_ID) + "/command";
      mqttClient.subscribe(commandTopic.c_str());
      Serial.println("Subscribed to: " + commandTopic);
    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

/**
 * Publish message to MQTT with envelope fields
 */
bool publishMQTT(const String& topic, JsonDocument& doc) {
  // Add envelope fields
  doc["schema_version"] = 1;
  doc["device_id"] = getDeviceId();
  doc["device_type"] = NODE_ROLE;
  doc["timestamp"] = getTimestamp();
  doc["firmware_version"] = FIRMWARE_VERSION;
  
  // Serialize to JSON
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Publish
  bool success = mqttClient.publish(topic.c_str(), jsonString.c_str());
  
  if (success) {
    Serial.println("✅ Published to " + topic);
    Serial.println("   Size: " + String(jsonString.length()) + " bytes");
  } else {
    Serial.println("❌ Failed to publish to " + topic);
  }
  
  return success;
}

/**
 * Publish mesh event (node join/leave)
 */
void publishMeshEvent(const String& eventType, uint32_t nodeId, const String& reason = "") {
  JsonDocument doc;
  
  doc["event"] = "mesh_event";
  doc["mesh_id"] = MESH_ID;
  doc["event_type"] = eventType;
  
  JsonArray affectedNodes = doc["affected_nodes"].to<JsonArray>();
  char nodeIdStr[17];
  snprintf(nodeIdStr, sizeof(nodeIdStr), "ALT-%012X", nodeId);
  affectedNodes.add(nodeIdStr);
  
  if (reason != "") {
    JsonObject details = doc["details"].to<JsonObject>();
    details["reason"] = reason;
    details["last_seen"] = getTimestamp();
  }
  
  String topic = "alteriom/mesh/" + String(MESH_ID) + "/events";
  publishMQTT(topic, doc);
}

#endif

// ============================================================================
// TOPOLOGY REPORTING
// ============================================================================

/**
 * Build and send full mesh topology
 */
void sendFullTopology() {
#if IS_GATEWAY
  Serial.println("\n📊 Generating FULL topology report...");
  
  if (!mqttClient.connected()) {
    connectMQTT();
    return;
  }
  
  JsonDocument doc;
  
  doc["event"] = "mesh_topology";
  doc["mesh_id"] = MESH_ID;
  doc["gateway_node_id"] = getDeviceId();
  doc["update_type"] = "full";
  
  // Get node list
  SimpleList<uint32_t> nodeList = mesh.getNodeList();
  
  // Add nodes array
  JsonArray nodes = doc["nodes"].to<JsonArray>();
  
  // Add gateway node
  JsonObject gatewayNode = nodes.add<JsonObject>();
  gatewayNode["node_id"] = getDeviceId();
  gatewayNode["role"] = NODE_ROLE;
  gatewayNode["status"] = "online";
  gatewayNode["last_seen"] = getTimestamp();
  gatewayNode["firmware_version"] = FIRMWARE_VERSION;
  gatewayNode["uptime_seconds"] = millis() / 1000;
  gatewayNode["free_memory_kb"] = ESP.getFreeHeap() / 1024;
  gatewayNode["connection_count"] = nodeList.size();
  
  // Add other nodes
  for (auto nodeId : nodeList) {
    JsonObject node = nodes.add<JsonObject>();
    
    char nodeIdStr[17];
    snprintf(nodeIdStr, sizeof(nodeIdStr), "ALT-%012X", nodeId);
    node["node_id"] = nodeIdStr;
    
    // Simulate role based on node ID
    node["role"] = (nodeId % 2 == 0) ? "sensor" : "repeater";
    node["status"] = "online";
    node["last_seen"] = getTimestamp();
    node["firmware_version"] = FIRMWARE_VERSION;
    node["uptime_seconds"] = 0;  // Unknown for remote nodes
    node["free_memory_kb"] = 0;  // Unknown for remote nodes
    node["connection_count"] = 1;  // Simplified
  }
  
  // Add connections array
  JsonArray connections = doc["connections"].to<JsonArray>();
  
  for (auto nodeId : nodeList) {
    JsonObject conn = connections.add<JsonObject>();
    conn["from_node"] = getDeviceId();
    
    char nodeIdStr[17];
    snprintf(nodeIdStr, sizeof(nodeIdStr), "ALT-%012X", nodeId);
    conn["to_node"] = nodeIdStr;
    
    conn["quality"] = calculateQuality(nodeId);
    conn["latency_ms"] = calculateLatency(nodeId);
    conn["rssi"] = calculateRSSI(nodeId);
    conn["hop_count"] = 1;  // Direct connection to gateway
  }
  
  // Add metrics
  JsonObject metrics = doc["metrics"].to<JsonObject>();
  metrics["total_nodes"] = nodeList.size() + 1;  // +1 for gateway
  metrics["online_nodes"] = nodeList.size() + 1;
  metrics["network_diameter"] = 1;  // Single hop for this test
  
  int avgQuality = 0;
  for (auto nodeId : nodeList) {
    avgQuality += calculateQuality(nodeId);
  }
  if (nodeList.size() > 0) {
    avgQuality /= nodeList.size();
  }
  metrics["avg_connection_quality"] = avgQuality;
  metrics["messages_per_second"] = 0.0;  // Not tracked in this test
  
  String topic = "alteriom/mesh/" + String(MESH_ID) + "/topology";
  publishMQTT(topic, doc);
  
  // Update last topology state
  lastTopology.clear();
  for (auto nodeId : nodeList) {
    NodeState state;
    state.nodeId = nodeId;
    state.role = (nodeId % 2 == 0) ? "sensor" : "repeater";
    state.online = true;
    state.lastSeen = millis();
    state.connectionCount = 1;
    lastTopology[nodeId] = state;
  }
  lastTopologyUpdate = millis();
  
  Serial.println("✅ Full topology published");
  Serial.println("   Nodes: " + String(nodeList.size() + 1));
  Serial.println("   Connections: " + String(nodeList.size()));
  
#endif
}

/**
 * Check for topology changes and send incremental update
 */
void checkIncrementalUpdate() {
#if IS_GATEWAY
  if (!mqttClient.connected()) {
    connectMQTT();
    return;
  }
  
  SimpleList<uint32_t> currentNodes = mesh.getNodeList();
  bool hasChanges = false;
  
  // Check for new nodes
  std::vector<uint32_t> newNodes;
  for (auto nodeId : currentNodes) {
    if (lastTopology.find(nodeId) == lastTopology.end()) {
      newNodes.push_back(nodeId);
      hasChanges = true;
    }
  }
  
  // Check for removed nodes
  std::vector<uint32_t> removedNodes;
  for (auto& pair : lastTopology) {
    uint32_t nodeId = pair.first;
    bool found = false;
    for (auto currentId : currentNodes) {
      if (currentId == nodeId) {
        found = true;
        break;
      }
    }
    if (!found) {
      removedNodes.push_back(nodeId);
      hasChanges = true;
    }
  }
  
  if (hasChanges) {
    Serial.println("\n🔄 Topology changed! Generating incremental update...");
    
    JsonDocument doc;
    
    doc["event"] = "mesh_topology";
    doc["mesh_id"] = MESH_ID;
    doc["gateway_node_id"] = getDeviceId();
    doc["update_type"] = "incremental";
    
    JsonArray nodes = doc["nodes"].to<JsonArray>();
    JsonArray connections = doc["connections"].to<JsonArray>();
    
    // Add new nodes
    for (auto nodeId : newNodes) {
      JsonObject node = nodes.add<JsonObject>();
      
      char nodeIdStr[17];
      snprintf(nodeIdStr, sizeof(nodeIdStr), "ALT-%012X", nodeId);
      node["node_id"] = nodeIdStr;
      node["role"] = (nodeId % 2 == 0) ? "sensor" : "repeater";
      node["status"] = "online";
      node["last_seen"] = getTimestamp();
      
      JsonObject conn = connections.add<JsonObject>();
      conn["from_node"] = getDeviceId();
      conn["to_node"] = nodeIdStr;
      conn["quality"] = calculateQuality(nodeId);
      conn["latency_ms"] = calculateLatency(nodeId);
      conn["rssi"] = calculateRSSI(nodeId);
      conn["hop_count"] = 1;
      
      Serial.println("   + Node joined: " + String(nodeIdStr));
      
      // Publish event
      publishMeshEvent("node_join", nodeId);
      
      // Update state
      NodeState state;
      state.nodeId = nodeId;
      state.role = (nodeId % 2 == 0) ? "sensor" : "repeater";
      state.online = true;
      state.lastSeen = millis();
      state.connectionCount = 1;
      lastTopology[nodeId] = state;
    }
    
    // Mark removed nodes as offline
    for (auto nodeId : removedNodes) {
      JsonObject node = nodes.add<JsonObject>();
      
      char nodeIdStr[17];
      snprintf(nodeIdStr, sizeof(nodeIdStr), "ALT-%012X", nodeId);
      node["node_id"] = nodeIdStr;
      node["status"] = "offline";
      node["last_seen"] = getTimestamp();
      
      Serial.println("   - Node left: " + String(nodeIdStr));
      
      // Publish event
      publishMeshEvent("node_leave", nodeId, "timeout");
      
      // Remove from state
      lastTopology.erase(nodeId);
    }
    
    String topic = "alteriom/mesh/" + String(MESH_ID) + "/topology";
    publishMQTT(topic, doc);
    
    Serial.println("✅ Incremental update published");
  }
#endif
}

/**
 * Check for incoming MQTT commands
 */
void checkCommands() {
#if IS_GATEWAY
  mqttClient.loop();
#endif
}

// ============================================================================
// MQTT COMMAND HANDLER (GATEWAY ONLY)
// ============================================================================

#if IS_GATEWAY

/**
 * Handle incoming MQTT messages (commands)
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("\n📥 MQTT message received:");
  Serial.println("   Topic: " + String(topic));
  
  // Parse JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  
  if (error) {
    Serial.println("   ❌ JSON parse error: " + String(error.c_str()));
    return;
  }
  
  String event = doc["event"];
  String command = doc["command"];
  String correlationId = doc["correlation_id"];
  
  Serial.println("   Event: " + event);
  Serial.println("   Command: " + command);
  Serial.println("   Correlation ID: " + correlationId);
  
  // Handle GET_TOPOLOGY command (command ID 300)
  if (event == "command" && command == "get_topology") {
    Serial.println("   🔍 Processing GET_TOPOLOGY command...");
    
    unsigned long startTime = millis();
    
    // Build topology response
    JsonDocument responseDoc;
    responseDoc["event"] = "command_response";
    responseDoc["command"] = "get_topology";
    responseDoc["correlation_id"] = correlationId;
    responseDoc["success"] = true;
    
    // Build topology result
    JsonObject result = responseDoc["result"].to<JsonObject>();
    result["event"] = "mesh_topology";
    result["mesh_id"] = MESH_ID;
    result["gateway_node_id"] = getDeviceId();
    result["update_type"] = "full";
    
    SimpleList<uint32_t> nodeList = mesh.getNodeList();
    
    // Add nodes
    JsonArray nodes = result["nodes"].to<JsonArray>();
    
    JsonObject gatewayNode = nodes.add<JsonObject>();
    gatewayNode["node_id"] = getDeviceId();
    gatewayNode["role"] = NODE_ROLE;
    gatewayNode["status"] = "online";
    gatewayNode["connection_count"] = nodeList.size();
    
    for (auto nodeId : nodeList) {
      JsonObject node = nodes.add<JsonObject>();
      char nodeIdStr[17];
      snprintf(nodeIdStr, sizeof(nodeIdStr), "ALT-%012X", nodeId);
      node["node_id"] = nodeIdStr;
      node["role"] = (nodeId % 2 == 0) ? "sensor" : "repeater";
      node["status"] = "online";
    }
    
    // Add connections
    JsonArray connections = result["connections"].to<JsonArray>();
    for (auto nodeId : nodeList) {
      JsonObject conn = connections.add<JsonObject>();
      conn["from_node"] = getDeviceId();
      char nodeIdStr[17];
      snprintf(nodeIdStr, sizeof(nodeIdStr), "ALT-%012X", nodeId);
      conn["to_node"] = nodeIdStr;
      conn["quality"] = calculateQuality(nodeId);
      conn["hop_count"] = 1;
    }
    
    // Add metrics
    JsonObject metrics = result["metrics"].to<JsonObject>();
    metrics["total_nodes"] = nodeList.size() + 1;
    metrics["online_nodes"] = nodeList.size() + 1;
    
    // Calculate latency
    unsigned long latency = millis() - startTime;
    responseDoc["latency_ms"] = latency;
    
    // Publish response
    String responseTopic = "alteriom/mesh/" + String(MESH_ID) + "/topology/response";
    publishMQTT(responseTopic, responseDoc);
    
    Serial.println("   ✅ GET_TOPOLOGY response sent");
    Serial.println("   Latency: " + String(latency) + "ms");
  }
}

#endif

// ============================================================================
// MESH CALLBACKS
// ============================================================================

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("✅ New connection: %u\n", nodeId);
  
#if IS_GATEWAY
  // Trigger incremental update on next check
  lastTopologyUpdate = 0;
#endif
}

void changedConnectionCallback() {
  Serial.println("🔄 Connections changed");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("⏰ Time adjusted: %d us\n", offset);
}

void droppedConnectionCallback(uint32_t nodeId) {
  Serial.printf("❌ Connection dropped: %u\n", nodeId);
  
#if IS_GATEWAY
  // Trigger incremental update on next check
  lastTopologyUpdate = 0;
#endif
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("================================");
  Serial.println("Alteriom Mesh Topology Test");
  Serial.println("================================");
  Serial.println("Device ID: " + getDeviceId());
  Serial.println("Role: " + String(NODE_ROLE));
  Serial.println("Firmware: " + String(FIRMWARE_VERSION));
  Serial.println("Gateway: " + String(IS_GATEWAY ? "YES" : "NO"));
  Serial.println("================================\n");
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Set callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onDroppedConnection(&droppedConnectionCallback);
  
#if IS_GATEWAY
  // Gateway: Connect to WiFi
  Serial.println("Connecting to WiFi: " + String(WIFI_SSID));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n✅ WiFi connected!");
  Serial.println("   IP: " + WiFi.localIP().toString());
  
  // Connect to MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(2048);  // Increase buffer for large topology messages
  connectMQTT();
  
  // Add topology reporting tasks
  userScheduler.addTask(taskSendFullTopology);
  userScheduler.addTask(taskCheckIncrementalUpdate);
  userScheduler.addTask(taskCheckCommands);
  
  taskSendFullTopology.enable();
  taskCheckIncrementalUpdate.enable();
  taskCheckCommands.enable();
  
  Serial.println("✅ Topology reporting enabled");
  Serial.println("   Full topology: every " + String(FULL_TOPOLOGY_INTERVAL/1000) + "s");
  Serial.println("   Incremental check: every " + String(INCREMENTAL_INTERVAL/1000) + "s");
#endif
  
  Serial.println("\n🚀 Setup complete! Mesh is running...\n");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  mesh.update();
  
#if IS_GATEWAY
  // Keep MQTT connected
  if (!mqttClient.connected()) {
    connectMQTT();
  }
#endif
}
