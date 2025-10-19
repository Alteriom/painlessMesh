//************************************************************
// Phase 2: MQTT Status Bridge Example
//
// This example demonstrates the MQTT Status Bridge feature,
// which publishes comprehensive mesh status to MQTT topics
// for professional monitoring with tools like:
// - Grafana
// - InfluxDB
// - Prometheus
// - Home Assistant
// - Node-RED
//
// MQTT Topics Published:
// - mesh/status/nodes      - List of all nodes in mesh
// - mesh/status/topology   - Complete mesh topology JSON
// - mesh/status/metrics    - Performance metrics
// - mesh/status/alerts     - Active alerts
// - mesh/status/node/{id}  - Per-node detailed status (optional)
//
// Requirements:
// - ESP32 or ESP8266
// - WiFi connection to external network (for MQTT broker)
// - MQTT broker (e.g., Mosquitto, HiveMQ, AWS IoT)
//************************************************************

#include <Arduino.h>
#include "painlessTaskOptions.h"  // Must be first to configure TaskScheduler
#include <TaskScheduler.h>  // Required for LDF to find TaskScheduler dependency
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "mqtt_status_bridge.hpp"

// Mesh configuration
#define   MESH_PREFIX     "AlteriomMesh"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

// External WiFi configuration (for MQTT connection)
#define   STATION_SSID     "YourAP_SSID"
#define   STATION_PASSWORD "YourAP_PWD"

// MQTT broker configuration
#define   MQTT_BROKER_IP   "192.168.1.100"  // Change to your MQTT broker IP
#define   MQTT_BROKER_PORT 1883
#define   MQTT_CLIENT_ID   "painlessMesh_StatusBridge"

#define HOSTNAME "MQTT_Status_Bridge"

// Prototypes
void receivedCallback(const uint32_t &from, const String &msg);
void mqttCallback(char* topic, byte* payload, unsigned int length);
IPAddress getlocalIP();

// Global objects
IPAddress myIP(0, 0, 0, 0);
IPAddress mqttBroker(192, 168, 1, 100);  // Change to your MQTT broker IP

Scheduler userScheduler;
painlessMesh mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttBroker, MQTT_BROKER_PORT, mqttCallback, wifiClient);

// Phase 2: MQTT Status Bridge instance
MqttStatusBridge* statusBridge = nullptr;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Phase 2: MQTT Status Bridge Example ===\n");

  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6);
  mesh.onReceive(&receivedCallback);

  // Connect to external WiFi for MQTT
  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname(HOSTNAME);

  // Bridge node should be root
  mesh.setRoot(true);
  mesh.setContainsRoot(true);

  Serial.println("Mesh initialized");
  Serial.println("Waiting for WiFi and MQTT connection...");
}

void loop() {
  mesh.update();
  mqttClient.loop();

  // Check for WiFi connection and connect to MQTT
  if (myIP != getlocalIP()) {
    myIP = getlocalIP();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(myIP.toString());

    // Connect to MQTT broker
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("MQTT connected!");
      Serial.print("Broker: ");
      Serial.print(mqttBroker.toString());
      Serial.print(":");
      Serial.println(MQTT_BROKER_PORT);
      
      // Subscribe to mesh command topics (optional)
      mqttClient.subscribe("mesh/command/#");
      mqttClient.publish("mesh/status/bridge", "Bridge online!");

      // Phase 2: Initialize and start MQTT Status Bridge
      if (statusBridge == nullptr) {
        statusBridge = new MqttStatusBridge(mesh, mqttClient, userScheduler);
        
        // Configure the bridge
        statusBridge->setPublishInterval(30000);  // Publish every 30 seconds
        statusBridge->setTopicPrefix("mesh/status/");
        
        // Enable features (all enabled by default)
        statusBridge->enableTopology(true);   // Publish mesh topology
        statusBridge->enableMetrics(true);    // Publish performance metrics
        statusBridge->enableAlerts(true);     // Publish active alerts
        statusBridge->enablePerNode(false);   // Per-node status (can be expensive)
        
        // Start publishing
        statusBridge->begin();
        
        Serial.println("\n=== MQTT Status Bridge Configured ===");
        Serial.println("Publishing to topics:");
        Serial.println("  - mesh/status/nodes");
        Serial.println("  - mesh/status/topology");
        Serial.println("  - mesh/status/metrics");
        Serial.println("  - mesh/status/alerts");
        Serial.println("Publish interval: 30 seconds\n");
      }
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.println(mqttClient.state());
    }
  }

  // Reconnect to MQTT if connection is lost
  static unsigned long lastReconnectAttempt = 0;
  if (!mqttClient.connected() && myIP != IPAddress(0, 0, 0, 0)) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.println("Attempting MQTT reconnection...");
      if (mqttClient.connect(MQTT_CLIENT_ID)) {
        Serial.println("MQTT reconnected!");
        mqttClient.subscribe("mesh/command/#");
      }
    }
  }
}

void receivedCallback(const uint32_t &from, const String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
  
  // Forward mesh messages to MQTT
  String topic = "mesh/from/" + String(from);
  if (mqttClient.connected()) {
    mqttClient.publish(topic.c_str(), msg.c_str());
  }
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  // Parse payload
  char* cleanPayload = (char*)malloc(length + 1);
  memcpy(cleanPayload, payload, length);
  cleanPayload[length] = '\0';
  String msg = String(cleanPayload);
  free(cleanPayload);

  Serial.printf("MQTT received on %s: %s\n", topic, msg.c_str());

  String topicStr = String(topic);
  
  // Handle mesh commands
  if (topicStr.startsWith("mesh/command/")) {
    String command = topicStr.substring(13);  // Remove "mesh/command/"
    
    if (command == "getNodes") {
      // Get node list
      auto nodes = mesh.getNodeList(true);
      String nodeList = "{\"nodes\":[";
      bool firstNode = true;
      for (auto nodeId : nodes) {
        if (!firstNode) nodeList += ",";
        firstNode = false;
        nodeList += String(nodeId);
      }
      nodeList += "]}";
      mqttClient.publish("mesh/response/nodes", nodeList.c_str());
    }
    else if (command == "getTopology") {
      // Get mesh topology
      String topology = mesh.subConnectionJson(false);
      mqttClient.publish("mesh/response/topology", topology.c_str());
    }
    else if (command == "publishNow") {
      // Trigger immediate status publish
      if (statusBridge != nullptr) {
        statusBridge->publishNow();
        mqttClient.publish("mesh/response/command", "Status published");
      }
    }
    else if (command == "broadcast") {
      // Broadcast message to all mesh nodes
      mesh.sendBroadcast(msg);
      mqttClient.publish("mesh/response/command", "Message broadcasted");
    }
  }
}

IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}
