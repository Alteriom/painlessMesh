#include <Arduino.h>
#include "painlessMesh.h"
#include <PubSubClient.h>
#include "mqtt_command_bridge.hpp"
#include "mesh_topology_reporter.hpp"
#include "mesh_event_publisher.hpp"

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
MeshTopologyReporter* topologyReporter;
MeshEventPublisher* eventPublisher;

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
    JsonDocument status;  // ArduinoJson v7: automatic sizing
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

// Task for topology updates (full topology every 60 seconds)
Task taskTopology(TASK_SECOND * 60, TASK_FOREVER, []() {
  if (mqttClient.connected() && topologyReporter != nullptr) {
    Serial.println("[Topology] Publishing full mesh topology...");
    
    String topology = topologyReporter->generateFullTopology();
    bool success = mqttClient.publish("alteriom/mesh/MESH-001/topology", topology.c_str());
    
    if (success) {
      Serial.printf("[Topology] ✅ Published (%d bytes)\n", topology.length());
    } else {
      Serial.println("[Topology] ❌ Failed to publish (message too large or MQTT error)");
    }
  }
});

// Task for incremental topology updates (check every 5 seconds)
Task taskTopologyIncremental(TASK_SECOND * 5, TASK_FOREVER, []() {
  if (mqttClient.connected() && topologyReporter != nullptr) {
    if (topologyReporter->hasTopologyChanged()) {
      Serial.println("[Topology] Topology changed, publishing incremental update...");
      
      String topology = topologyReporter->generateFullTopology();
      bool success = mqttClient.publish("alteriom/mesh/MESH-001/topology", topology.c_str());
      
      if (success) {
        Serial.printf("[Topology] ✅ Published incremental update (%d bytes)\n", topology.length());
      } else {
        Serial.println("[Topology] ❌ Failed to publish incremental update");
      }
    }
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
  
  // Initialize topology reporter and event publisher first
  topologyReporter = new MeshTopologyReporter(mesh, "MESH-001", "GW 2.3.4");
  eventPublisher = new MeshEventPublisher(mesh, mqttClient, "MESH-001", "alteriom/mesh/MESH-001/events", "GW 2.3.4");
  
  Serial.println("\n✅ Topology Reporter initialized");
  Serial.println("✅ Event Publisher initialized");
  
  // Initialize bidirectional MQTT-mesh bridge with topology reporter
  commandBridge = new MqttCommandBridge(mesh, mqttClient, topologyReporter);
  commandBridge->setMqttCallback();
  commandBridge->setMeshCallback();
  commandBridge->begin();
  
  // Add background tasks
  userScheduler.addTask(taskMqttReconnect);
  userScheduler.addTask(taskGatewayStatus);
  userScheduler.addTask(taskTopology);
  userScheduler.addTask(taskTopologyIncremental);
  
  taskMqttReconnect.enable();
  taskGatewayStatus.enable();
  taskTopology.enable();
  taskTopologyIncremental.enable();
  
  // Mesh callbacks with schema-compliant event publishing
  mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("[Mesh] 🔗 New node connected: %u\n", nodeId);
    
    // Publish schema-compliant node join event
    if (eventPublisher != nullptr) {
      eventPublisher->publishNodeJoin(nodeId);
    }
    
    // Publish incremental topology update
    if (mqttClient.connected() && topologyReporter != nullptr) {
      String topology = topologyReporter->generateIncrementalUpdate(nodeId, true);
      mqttClient.publish("alteriom/mesh/MESH-001/topology", topology.c_str());
      Serial.println("[Topology] Published incremental update (node join)");
    }
  });
  
  mesh.onDroppedConnection([](uint32_t nodeId) {
    Serial.printf("[Mesh] ⚠️ Node disconnected: %u\n", nodeId);
    
    // Publish schema-compliant node leave event
    if (eventPublisher != nullptr) {
      eventPublisher->publishNodeLeave(nodeId);
    }
    
    // Publish incremental topology update
    if (mqttClient.connected() && topologyReporter != nullptr) {
      String topology = topologyReporter->generateIncrementalUpdate(nodeId, false);
      mqttClient.publish("alteriom/mesh/MESH-001/topology", topology.c_str());
      Serial.println("[Topology] Published incremental update (node leave)");
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
  Serial.println("    - alteriom/mesh/MESH-001/topology (schema v0.5.0)");
  Serial.println("    - alteriom/mesh/MESH-001/events (schema v0.5.0)");
  Serial.println("\nSchema Compliance:");
  Serial.println("  ✅ @alteriom/mqtt-schema v0.5.0");
  Serial.println("  ✅ mesh_topology.schema.json");
  Serial.println("  ✅ mesh_event.schema.json");
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
