#ifndef _MQTT_STATUS_BRIDGE_HPP_
#define _MQTT_STATUS_BRIDGE_HPP_

/**
 * Phase 2: MQTT Status Bridge
 * 
 * Professional monitoring solution for painlessMesh networks.
 * Publishes comprehensive mesh status to MQTT topics for integration
 * with Grafana, InfluxDB, Prometheus, Home Assistant, and other tools.
 * 
 * Features:
 * - Node list with states
 * - Mesh topology JSON
 * - Performance metrics
 * - Active alerts
 * - Per-node health monitoring
 * 
 * MQTT Topics:
 * - mesh/status/nodes         - Node list with basic info
 * - mesh/status/topology      - Complete mesh structure
 * - mesh/status/metrics       - Performance statistics
 * - mesh/status/alerts        - Active alert conditions
 * - mesh/status/node/{id}     - Per-node detailed status
 */

#include <Arduino.h>
#include <PubSubClient.h>

class MqttStatusBridge {
private:
  painlessMesh& mesh;
  PubSubClient& mqttClient;
  
  // Configuration
  uint32_t publishInterval = 30000;  // Default 30 seconds
  bool enableTopologyPublish = true;
  bool enableMetricsPublish = true;
  bool enableAlertsPublish = true;
  bool enablePerNodePublish = false;  // Can be expensive for large meshes
  
  // Topic prefix
  String topicPrefix = "mesh/status/";
  
  // Last publish time
  unsigned long lastPublishTime = 0;
  
  // Task scheduler
  Task* publishTask = nullptr;

public:
  /**
   * Constructor
   * @param mesh Reference to painlessMesh instance
   * @param mqttClient Reference to PubSubClient instance
   */
  MqttStatusBridge(painlessMesh& mesh, PubSubClient& mqttClient) 
    : mesh(mesh), mqttClient(mqttClient) {
  }
  
  /**
   * Set the publish interval in milliseconds
   * @param interval Interval in milliseconds (default: 30000)
   */
  void setPublishInterval(uint32_t interval) {
    publishInterval = interval;
  }
  
  /**
   * Set the MQTT topic prefix
   * @param prefix Topic prefix (default: "mesh/status/")
   */
  void setTopicPrefix(const String& prefix) {
    topicPrefix = prefix;
  }
  
  /**
   * Enable or disable topology publishing
   */
  void enableTopology(bool enable) {
    enableTopologyPublish = enable;
  }
  
  /**
   * Enable or disable metrics publishing
   */
  void enableMetrics(bool enable) {
    enableMetricsPublish = enable;
  }
  
  /**
   * Enable or disable alerts publishing
   */
  void enableAlerts(bool enable) {
    enableAlertsPublish = enable;
  }
  
  /**
   * Enable or disable per-node publishing
   * Warning: This can be expensive for large meshes (50+ nodes)
   */
  void enablePerNode(bool enable) {
    enablePerNodePublish = enable;
  }
  
  /**
   * Initialize the bridge and start publishing
   */
  void begin() {
    // Create periodic task to publish status
    publishTask = &mesh.addTask(TASK_MILLISECOND * publishInterval, 
                                TASK_FOREVER, 
                                [this]() { this->publishStatus(); });
    publishTask->enable();
    
    Serial.println("MQTT Status Bridge started");
    Serial.printf("Publishing every %d ms to %s*\n", publishInterval, topicPrefix.c_str());
  }
  
  /**
   * Stop publishing
   */
  void stop() {
    if (publishTask != nullptr) {
      publishTask->disable();
    }
  }
  
  /**
   * Manually trigger a status publish
   */
  void publishNow() {
    publishStatus();
  }

private:
  /**
   * Main publishing function - collects and publishes all enabled status data
   */
  void publishStatus() {
    if (!mqttClient.connected()) {
      Serial.println("MQTT not connected, skipping status publish");
      return;
    }
    
    unsigned long startTime = millis();
    
    // Always publish node list
    publishNodeList();
    
    // Optional: Publish topology
    if (enableTopologyPublish) {
      publishTopology();
    }
    
    // Optional: Publish metrics
    if (enableMetricsPublish) {
      publishMetrics();
    }
    
    // Optional: Publish alerts
    if (enableAlertsPublish) {
      publishAlerts();
    }
    
    // Optional: Publish per-node status (expensive!)
    if (enablePerNodePublish) {
      publishPerNodeStatus();
    }
    
    unsigned long elapsed = millis() - startTime;
    Serial.printf("Status published in %lu ms\n", elapsed);
  }
  
  /**
   * Publish list of nodes with basic information
   * Topic: mesh/status/nodes
   */
  void publishNodeList() {
    auto nodes = mesh.getNodeList(true);  // Include self
    
    // Build JSON: {"nodes": [123, 456, 789], "count": 3, "timestamp": 12345}
    String payload = "{\"nodes\":[";
    String nodesList;
    for (size_t i = 0; i < nodes.size(); i++) {
      if (i > 0) nodesList += ",";
      nodesList += String(nodes[i]);
    }
    payload += nodesList;
    payload += "],\"count\":";
    payload += String(nodes.size());
    payload += ",\"timestamp\":";
    payload += String(millis());
    payload += "}";
    
    String topic = topicPrefix + "nodes";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Publish mesh topology as JSON
   * Topic: mesh/status/topology
   */
  void publishTopology() {
    String topology = mesh.subConnectionJson(false);
    String topic = topicPrefix + "topology";
    mqttClient.publish(topic.c_str(), topology.c_str());
  }
  
  /**
   * Publish performance metrics
   * Topic: mesh/status/metrics
   */
  void publishMetrics() {
    auto nodes = mesh.getNodeList(true);
    
    // Build JSON with mesh-wide metrics
    String payload = "{";
    payload += "\"nodeCount\":";
    payload += String(nodes.size());
    payload += ",\"rootNodeId\":";
    payload += String(mesh.getNodeId());
    payload += ",\"uptime\":";
    payload += String(millis() / 1000);
    
#if defined(ESP32) || defined(ESP8266)
    payload += ",\"freeHeap\":";
    payload += String(ESP.getFreeHeap());
#ifdef ESP32
    payload += ",\"freeHeapKB\":";
    payload += String(ESP.getFreeHeap() / 1024);
#endif
#endif
    
    payload += ",\"timestamp\":";
    payload += String(millis());
    payload += "}";
    
    String topic = topicPrefix + "metrics";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Publish active alerts
   * Topic: mesh/status/alerts
   * 
   * This is a placeholder - in a real implementation, you would track
   * node health and publish alerts when issues are detected.
   */
  void publishAlerts() {
    // Example: Check for nodes that haven't responded recently
    auto nodes = mesh.getNodeList(true);
    
    String payload = "{\"alerts\":[";
    bool hasAlerts = false;
    
    // Example alert logic - customize based on your needs
    if (nodes.size() == 0) {
      if (hasAlerts) payload += ",";
      payload += "{\"type\":\"NO_NODES\",\"severity\":\"warning\",\"message\":\"No nodes in mesh\"}";
      hasAlerts = true;
    }
    
#if defined(ESP32) || defined(ESP8266)
    // Low memory alert
    if (ESP.getFreeHeap() < 10000) {
      if (hasAlerts) payload += ",";
      payload += "{\"type\":\"LOW_MEMORY\",\"severity\":\"critical\",\"message\":\"Free heap below 10KB\"}";
      hasAlerts = true;
    }
#endif
    
    payload += "],\"timestamp\":";
    payload += String(millis());
    payload += "}";
    
    String topic = topicPrefix + "alerts";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Publish detailed status for each node
   * Topic: mesh/status/node/{nodeId}
   * 
   * Warning: This can generate a lot of MQTT traffic for large meshes!
   */
  void publishPerNodeStatus() {
    auto nodes = mesh.getNodeList(false);  // Don't include self
    
    for (auto nodeId : nodes) {
      String payload = "{";
      payload += "\"nodeId\":";
      payload += String(nodeId);
      payload += ",\"connected\":";
      payload += mesh.isConnected(nodeId) ? "true" : "false";
      payload += ",\"timestamp\":";
      payload += String(millis());
      payload += "}";
      
      String topic = topicPrefix + "node/" + String(nodeId);
      mqttClient.publish(topic.c_str(), payload.c_str());
    }
    
    // Also publish self
    String payload = "{";
    payload += "\"nodeId\":";
    payload += String(mesh.getNodeId());
    payload += ",\"isSelf\":true";
#if defined(ESP32) || defined(ESP8266)
    payload += ",\"freeHeap\":";
    payload += String(ESP.getFreeHeap());
#endif
    payload += ",\"timestamp\":";
    payload += String(millis());
    payload += "}";
    
    String topic = topicPrefix + "node/" + String(mesh.getNodeId());
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
};

#endif // _MQTT_STATUS_BRIDGE_HPP_
