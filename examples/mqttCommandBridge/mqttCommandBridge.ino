#include <Arduino.h>
#include "painlessMesh.h"
#include <PubSubClient.h>
#include "examples/bridge/mqtt_command_bridge.hpp"

// Mesh configuration
#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

// WiFi configuration
#define STATION_SSID     "YourWiFi"
#define STATION_PASSWORD "YourPassword"

// MQTT configuration
#define MQTT_BROKER_IP   "192.168.1.100"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID   "painlessMesh-commandBridge"

Scheduler userScheduler;
painlessMesh mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
MqttCommandBridge* commandBridge;

// Task for MQTT reconnection
Task taskMqttReconnect(TASK_SECOND * 5, TASK_FOREVER, []() {
  if (!mqttClient.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("MQTT reconnected!");
      commandBridge->begin(); // Re-subscribe to topics
    } else {
      Serial.printf("MQTT reconnect failed, rc=%d\n", mqttClient.state());
    }
  }
});

// Task for periodic gateway status
Task taskGatewayStatus(TASK_MINUTE, TASK_FOREVER, []() {
  if (mqttClient.connected()) {
    DynamicJsonDocument status(512);
    status["gateway_id"] = mesh.getNodeId();
    status["status"] = "online";
    status["uptime"] = millis() / 1000;
    status["free_memory"] = ESP.getFreeHeap();
    status["connected_nodes"] = mesh.getNodeList().size();
    status["wifi_rssi"] = WiFi.RSSI();
    
    String payload;
    serializeJson(status, payload);
    
    mqttClient.publish("mesh/gateway/status", payload.c_str());
  }
});

// Task for topology updates
Task taskTopology(TASK_SECOND * 30, TASK_FOREVER, []() {
  if (mqttClient.connected()) {
    auto nodeList = mesh.getNodeList();
    
    DynamicJsonDocument topology(2048);
    topology["gateway_id"] = mesh.getNodeId();
    topology["node_count"] = nodeList.size();
    
    JsonArray nodes = topology.createNestedArray("nodes");
    for (auto node : nodeList) {
      nodes.add(node);
    }
    
    String payload;
    serializeJson(topology, payload);
    
    mqttClient.publish("mesh/topology", payload.c_str());
  }
});

void setup() {
  Serial.begin(115200);
  
  Serial.println("\n\n=================================");
  Serial.println("MQTT Command Bridge for painlessMesh");
  Serial.println("=================================\n");
  
  // Initialize mesh as bridge (root node)
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  
  // Connect to WiFi station
  Serial.printf("Connecting to WiFi: %s\n", STATION_SSID);
  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  
  // Wait for WiFi connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi connected!\n");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  } else {
    Serial.println("\nWiFi connection failed!");
    Serial.println("Bridge will continue with mesh-only mode");
  }
  
  // Configure MQTT
  mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
  mqttClient.setBufferSize(2048); // Increase buffer for larger messages
  
  // Connect to MQTT broker
  Serial.printf("\nConnecting to MQTT broker: %s:%d\n", MQTT_BROKER_IP, MQTT_BROKER_PORT);
  
  int mqttAttempts = 0;
  while (!mqttClient.connected() && mqttAttempts < 10) {
    Serial.print("Attempting MQTT connection...");
    
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected!");
    } else {
      Serial.printf("failed, rc=%d\n", mqttClient.state());
      Serial.println("Error codes: -4=timeout, -3=conn lost, -2=conn failed, -1=disconnected");
      delay(2000);
      mqttAttempts++;
    }
  }
  
  if (!mqttClient.connected()) {
    Serial.println("WARNING: MQTT connection failed, bridge will retry");
  }
  
  // Initialize bidirectional MQTT-mesh bridge
  commandBridge = new MqttCommandBridge(mesh, mqttClient);
  commandBridge->setMqttCallback();
  commandBridge->setMeshCallback();
  commandBridge->begin();
  
  // Add background tasks
  userScheduler.addTask(taskMqttReconnect);
  userScheduler.addTask(taskGatewayStatus);
  userScheduler.addTask(taskTopology);
  
  taskMqttReconnect.enable();
  taskGatewayStatus.enable();
  taskTopology.enable();
  
  // Mesh callbacks
  mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("New node connected: %u\n", nodeId);
    
    if (mqttClient.connected()) {
      DynamicJsonDocument event(256);
      event["event"] = "node_connected";
      event["node_id"] = nodeId;
      event["timestamp"] = millis();
      
      String payload;
      serializeJson(event, payload);
      
      mqttClient.publish("mesh/events", payload.c_str());
    }
  });
  
  mesh.onDroppedConnection([](uint32_t nodeId) {
    Serial.printf("Node disconnected: %u\n", nodeId);
    
    if (mqttClient.connected()) {
      DynamicJsonDocument event(256);
      event["event"] = "node_disconnected";
      event["node_id"] = nodeId;
      event["timestamp"] = millis();
      
      String payload;
      serializeJson(event, payload);
      
      mqttClient.publish("mesh/events", payload.c_str());
    }
  });
  
  Serial.println("\n=== MQTT Command Bridge Ready ===");
  Serial.printf("Gateway ID: %u\n", mesh.getNodeId());
  Serial.printf("Connected Nodes: %d\n", mesh.getNodeList().size());
  Serial.println("\nMQTT Topics:");
  Serial.println("  Commands IN:");
  Serial.println("    - mesh/command/{nodeId}");
  Serial.println("    - mesh/command/broadcast");
  Serial.println("    - mesh/config/{nodeId}/get");
  Serial.println("    - mesh/config/{nodeId}/set");
  Serial.println("  Messages OUT:");
  Serial.println("    - mesh/response/{nodeId}");
  Serial.println("    - mesh/status/{nodeId}");
  Serial.println("    - mesh/sensor/{nodeId}");
  Serial.println("    - mesh/health/{nodeId}");
  Serial.println("    - mesh/gateway/status");
  Serial.println("    - mesh/topology");
  Serial.println("    - mesh/events");
  Serial.println("\nBridge is now listening for commands...\n");
}

void loop() {
  mesh.update();
  
  // Process MQTT messages
  if (mqttClient.connected()) {
    mqttClient.loop();
  }
  
  // Handle WiFi reconnection
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt > 30000) {
      Serial.println("WiFi disconnected, attempting reconnect...");
      WiFi.reconnect();
      lastReconnectAttempt = millis();
    }
  }
}
