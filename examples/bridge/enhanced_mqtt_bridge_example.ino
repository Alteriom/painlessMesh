/**
 * Enhanced MQTT Bridge Example - painlessMesh v1.7.7
 * 
 * Demonstrates the enhanced MQTT bridge with:
 * - Command handlers for requesting metrics and health checks
 * - Aggregated network statistics
 * - Real-time metric collection from mesh nodes
 * - Health monitoring with alerting
 * 
 * This gateway node bridges between MQTT and the mesh network,
 * enabling remote monitoring and control.
 * 
 * Hardware: ESP32 with WiFi (ESP8266 also supported)
 * 
 * MQTT Topics:
 * Subscribe (Commands):
 *   - mesh/command/request_metrics {"node_id": 0}  // 0 = all nodes
 *   - mesh/command/request_health {"node_id": 12345}
 *   - mesh/command/get_aggregated {}
 * 
 * Publish (Data):
 *   - mesh/metrics/{node_id}      // Individual node metrics
 *   - mesh/health/{node_id}       // Individual node health
 *   - mesh/aggregated/metrics     // Aggregated mesh-wide metrics
 *   - mesh/aggregated/health      // Aggregated mesh health summary
 *   - mesh/alerts/critical        // Critical health alerts
 */

#include "painlessMesh.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "enhanced_mqtt_bridge.hpp"

// Mesh Configuration
#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_password"
#define MESH_PORT       5555

// WiFi Station Configuration (for MQTT connection)
#define STATION_SSID    "your_wifi_ssid"
#define STATION_PASSWORD "your_wifi_password"

// MQTT Configuration
#define MQTT_BROKER     "192.168.1.100"  // Your MQTT broker IP
#define MQTT_PORT       1883
#define MQTT_USER       "mqtt_user"       // Leave empty if no auth
#define MQTT_PASSWORD   "mqtt_password"   // Leave empty if no auth

Scheduler userScheduler;
painlessMesh mesh;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
EnhancedMqttBridge* bridge = nullptr;

// Task for reconnecting to MQTT
Task taskReconnectMQTT(5000, TASK_FOREVER, &reconnectMQTT);

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Enhanced MQTT Bridge for painlessMesh v1.7.7 ===");
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Configure mesh callbacks
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onDroppedConnection(&droppedConnectionCallback);
  
  // Connect to WiFi for MQTT
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(STATION_SSID, STATION_PASSWORD);
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
  
  // Configure MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(2048);  // Increase buffer for large messages
  
  // Connect to MQTT
  connectMQTT();
  
  // Create and initialize enhanced bridge
  bridge = new EnhancedMqttBridge(mesh, mqttClient);
  bridge->setTopicPrefix("mesh/");
  bridge->setDeviceId("GATEWAY-" + String(mesh.getNodeId(), HEX));
  bridge->enableAggregation(true);
  bridge->setAggregationInterval(60000);  // 60 seconds
  bridge->begin();
  
  // Add MQTT reconnect task
  userScheduler.addTask(taskReconnectMQTT);
  taskReconnectMQTT.enable();
  
  Serial.printf("Gateway Node ID: %u\n", mesh.getNodeId());
  Serial.println("Enhanced MQTT Bridge ready!");
  Serial.println("\nAvailable MQTT commands:");
  Serial.println("  mesh/command/request_metrics {\"node_id\": 0}");
  Serial.println("  mesh/command/request_health {\"node_id\": 12345}");
  Serial.println("  mesh/command/get_aggregated {}");
}

void loop() {
  mesh.update();
  
  // Handle MQTT
  if (mqttClient.connected()) {
    mqttClient.loop();
  }
  
  // Update bridge (handles aggregation)
  if (bridge) {
    bridge->update();
  }
}

/**
 * MQTT callback - receives commands from MQTT
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert to String
  String topicStr = String(topic);
  String payloadStr = "";
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }
  
  Serial.printf("\n[MQTT] Message on topic: %s\n", topic);
  Serial.printf("Payload: %s\n", payloadStr.c_str());
  
  // Forward to bridge for handling
  if (bridge) {
    bridge->handleMQTTMessage(topicStr, payloadStr);
  }
}

/**
 * Connect to MQTT broker
 */
void connectMQTT() {
  Serial.print("Connecting to MQTT broker...");
  
  String clientId = "painlessMesh-" + String(mesh.getNodeId());
  
  bool connected = false;
  if (strlen(MQTT_USER) > 0) {
    connected = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD);
  } else {
    connected = mqttClient.connect(clientId.c_str());
  }
  
  if (connected) {
    Serial.println(" Connected!");
    
    // Subscribe to command topics
    mqttClient.subscribe("mesh/command/request_metrics");
    mqttClient.subscribe("mesh/command/request_health");
    mqttClient.subscribe("mesh/command/get_aggregated");
    
    Serial.println("Subscribed to command topics");
    
    // Publish connection announcement
    String topic = "mesh/gateway/status";
    String payload = "{\"status\":\"connected\",\"node_id\":" + String(mesh.getNodeId()) + "}";
    mqttClient.publish(topic.c_str(), payload.c_str());
  } else {
    Serial.printf(" Failed! (rc=%d)\n", mqttClient.state());
    Serial.println("Will retry in 5 seconds...");
  }
}

/**
 * Reconnect to MQTT if disconnected
 */
void reconnectMQTT() {
  if (!mqttClient.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.println("\nMQTT disconnected, attempting to reconnect...");
    connectMQTT();
  }
}

// Mesh callbacks
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("\n[Mesh] New connection: %u\n", nodeId);
  Serial.printf("Total nodes in mesh: %d\n", mesh.getNodeList().size() + 1);
  
  // Publish to MQTT
  if (mqttClient.connected()) {
    String topic = "mesh/events/connection";
    String payload = "{\"event\":\"new_connection\",\"node_id\":" + String(nodeId) + 
                    ",\"total_nodes\":" + String(mesh.getNodeList().size() + 1) + "}";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
}

void changedConnectionCallback() {
  Serial.println("\n[Mesh] Topology changed");
  
  // Publish to MQTT
  if (mqttClient.connected()) {
    auto nodes = mesh.getNodeList();
    String topic = "mesh/events/topology_change";
    String payload = "{\"event\":\"topology_changed\",\"node_count\":" + 
                    String(nodes.size() + 1) + "}";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
}

void droppedConnectionCallback(uint32_t nodeId) {
  Serial.printf("\n[Mesh] Connection dropped: %u\n", nodeId);
  
  // Publish to MQTT
  if (mqttClient.connected()) {
    String topic = "mesh/events/disconnection";
    String payload = "{\"event\":\"connection_dropped\",\"node_id\":" + String(nodeId) + "}";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
}
