#ifndef _ENHANCED_MQTT_BRIDGE_HPP_
#define _ENHANCED_MQTT_BRIDGE_HPP_

/**
 * Enhanced MQTT Bridge for painlessMesh v1.7.7+
 * 
 * Extends the basic MQTT bridge with:
 * - Command handlers for requesting specific metrics
 * - Aggregated network statistics
 * - Support for MetricsPackage (Type 204) and HealthCheckPackage (Type 605)
 * - Real-time metric requests and responses
 * - Network-wide health aggregation
 * 
 * MQTT Topics:
 * Subscribe:
 * - mesh/command/request_metrics  - Request metrics from specific node or all nodes
 * - mesh/command/request_health   - Request health check from specific node or all nodes
 * - mesh/command/get_aggregated   - Get aggregated statistics
 * 
 * Publish:
 * - mesh/metrics/{node_id}        - Individual node metrics
 * - mesh/health/{node_id}         - Individual node health
 * - mesh/aggregated/metrics       - Aggregated mesh-wide metrics
 * - mesh/aggregated/health        - Aggregated mesh-wide health summary
 * - mesh/response/metrics         - Metrics response to command
 * - mesh/response/health          - Health response to command
 */

#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

// Command types for metric requests
#define CMD_REQUEST_METRICS     210
#define CMD_REQUEST_HEALTH      211
#define CMD_METRICS_RESPONSE    212
#define CMD_HEALTH_RESPONSE     213

class EnhancedMqttBridge {
private:
  painlessMesh& mesh;
  PubSubClient& mqttClient;
  
  // Configuration
  String topicPrefix = "mesh/";
  String deviceId = "";
  String firmwareVersion = "1.7.7";
  
  // Aggregation settings
  bool enableAggregation = true;
  uint32_t aggregationInterval = 60000;  // 60 seconds
  unsigned long lastAggregationTime = 0;
  
  // Metric storage for aggregation
  struct NodeMetrics {
    uint32_t nodeId;
    uint32_t timestamp;
    uint8_t cpuUsage;
    uint32_t freeHeap;
    uint16_t currentThroughput;
    uint8_t connectionQuality;
    int8_t wifiRSSI;
  };
  
  struct NodeHealth {
    uint32_t nodeId;
    uint32_t timestamp;
    uint8_t healthStatus;
    uint16_t problemFlags;
    uint8_t memoryHealth;
    uint8_t networkHealth;
    uint8_t performanceHealth;
  };
  
  // Storage (limited to prevent memory issues)
  #define MAX_STORED_NODES 20
  NodeMetrics metricsCache[MAX_STORED_NODES];
  NodeHealth healthCache[MAX_STORED_NODES];
  uint8_t metricsCacheSize = 0;
  uint8_t healthCacheSize = 0;
  
  // Callback for processing received mesh messages
  std::function<void(uint32_t, String&)> originalReceiveCallback = nullptr;

public:
  /**
   * Constructor
   */
  EnhancedMqttBridge(painlessMesh& mesh, PubSubClient& mqttClient)
    : mesh(mesh), mqttClient(mqttClient) {
  }
  
  /**
   * Initialize the enhanced bridge
   */
  void begin() {
    // Set device ID if not already set
    if (deviceId.length() == 0) {
      deviceId = "ALT-" + String(mesh.getNodeId(), HEX);
      deviceId.toUpperCase();
    }
    
    // Subscribe to command topics
    subscribeMQTTCommands();
    
    // Set up mesh message handler
    setupMeshCallbacks();
    
    Serial.println("Enhanced MQTT Bridge v1.7.7 started");
    Serial.printf("Device ID: %s\n", deviceId.c_str());
    Serial.println("Command topics subscribed:");
    Serial.printf("  - %scommand/request_metrics\n", topicPrefix.c_str());
    Serial.printf("  - %scommand/request_health\n", topicPrefix.c_str());
    Serial.printf("  - %scommand/get_aggregated\n", topicPrefix.c_str());
  }
  
  /**
   * Update function - call in loop()
   */
  void update() {
    // Check if it's time to publish aggregated statistics
    if (enableAggregation && (millis() - lastAggregationTime > aggregationInterval)) {
      publishAggregatedMetrics();
      publishAggregatedHealth();
      lastAggregationTime = millis();
    }
  }
  
  /**
   * Set topic prefix
   */
  void setTopicPrefix(const String& prefix) {
    topicPrefix = prefix;
  }
  
  /**
   * Set device ID
   */
  void setDeviceId(const String& id) {
    deviceId = id;
  }
  
  /**
   * Enable/disable aggregation
   */
  void enableAggregation(bool enable) {
    enableAggregation = enable;
  }
  
  /**
   * Set aggregation interval
   */
  void setAggregationInterval(uint32_t interval) {
    aggregationInterval = interval;
  }
  
  /**
   * Handle incoming MQTT messages
   * Call this from your MQTT callback
   */
  void handleMQTTMessage(String& topic, String& payload) {
    Serial.printf("MQTT message received on topic: %s\n", topic.c_str());
    
    if (topic == topicPrefix + "command/request_metrics") {
      handleRequestMetrics(payload);
    } else if (topic == topicPrefix + "command/request_health") {
      handleRequestHealth(payload);
    } else if (topic == topicPrefix + "command/get_aggregated") {
      handleGetAggregated(payload);
    }
  }

private:
  /**
   * Subscribe to MQTT command topics
   */
  void subscribeMQTTCommands() {
    String topic = topicPrefix + "command/request_metrics";
    mqttClient.subscribe(topic.c_str());
    
    topic = topicPrefix + "command/request_health";
    mqttClient.subscribe(topic.c_str());
    
    topic = topicPrefix + "command/get_aggregated";
    mqttClient.subscribe(topic.c_str());
  }
  
  /**
   * Set up mesh callbacks to intercept packages
   */
  void setupMeshCallbacks() {
    mesh.onReceive([this](uint32_t from, String& msg) {
      this->handleMeshMessage(from, msg);
    });
  }
  
  /**
   * Handle mesh messages and extract metrics/health packages
   */
  void handleMeshMessage(uint32_t from, String& msg) {
    // Parse message to determine type
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) {
      Serial.printf("JSON parse error from node %u: %s\n", from, error.c_str());
      return;
    }
    
    JsonObject obj = doc.as<JsonObject>();
    uint8_t msgType = obj["type"];
    
    // Handle different package types
    switch(msgType) {
      case 204:  // MetricsPackage (SENSOR_METRICS per schema v0.7.2+)
        handleMetricsPackage(from, obj);
        break;
      case 605:  // HealthCheckPackage (MESH_METRICS per schema v0.7.2+)
        handleHealthPackage(from, obj);
        break;
      case CMD_METRICS_RESPONSE:
        handleMetricsResponse(from, obj);
        break;
      case CMD_HEALTH_RESPONSE:
        handleHealthResponse(from, obj);
        break;
    }
    
    // Call original callback if set
    if (originalReceiveCallback) {
      originalReceiveCallback(from, msg);
    }
  }
  
  /**
   * Handle MetricsPackage received from mesh
   */
  void handleMetricsPackage(uint32_t from, JsonObject& obj) {
    Serial.printf("Received MetricsPackage from node %u\n", from);
    
    // Store in cache for aggregation
    storeMetrics(from, obj);
    
    // Publish to MQTT
    publishMetricsToMQTT(from, obj);
  }
  
  /**
   * Handle HealthCheckPackage received from mesh
   */
  void handleHealthPackage(uint32_t from, JsonObject& obj) {
    Serial.printf("Received HealthCheckPackage from node %u\n", from);
    
    // Store in cache for aggregation
    storeHealth(from, obj);
    
    // Publish to MQTT
    publishHealthToMQTT(from, obj);
    
    // Check for critical health and alert
    uint8_t healthStatus = obj["health"];
    if (healthStatus == 0) {
      publishCriticalAlert(from, obj);
    }
  }
  
  /**
   * Store metrics in cache for aggregation
   */
  void storeMetrics(uint32_t nodeId, JsonObject& obj) {
    // Find existing entry or add new
    int index = findMetricsCacheIndex(nodeId);
    if (index == -1) {
      if (metricsCacheSize < MAX_STORED_NODES) {
        index = metricsCacheSize++;
      } else {
        // Cache full, replace oldest
        index = 0;
      }
    }
    
    metricsCache[index].nodeId = nodeId;
    metricsCache[index].timestamp = obj["ts"];
    metricsCache[index].cpuUsage = obj["cpu"];
    metricsCache[index].freeHeap = obj["heap"];
    metricsCache[index].currentThroughput = obj["throughput"];
    metricsCache[index].connectionQuality = obj["connQual"];
    metricsCache[index].wifiRSSI = obj["rssi"];
  }
  
  /**
   * Store health in cache for aggregation
   */
  void storeHealth(uint32_t nodeId, JsonObject& obj) {
    // Find existing entry or add new
    int index = findHealthCacheIndex(nodeId);
    if (index == -1) {
      if (healthCacheSize < MAX_STORED_NODES) {
        index = healthCacheSize++;
      } else {
        // Cache full, replace oldest
        index = 0;
      }
    }
    
    healthCache[index].nodeId = nodeId;
    healthCache[index].timestamp = obj["ts"];
    healthCache[index].healthStatus = obj["health"];
    healthCache[index].problemFlags = obj["problems"];
    healthCache[index].memoryHealth = obj["memHealth"];
    healthCache[index].networkHealth = obj["netHealth"];
    healthCache[index].performanceHealth = obj["perfHealth"];
  }
  
  /**
   * Find metrics cache index for node
   */
  int findMetricsCacheIndex(uint32_t nodeId) {
    for (int i = 0; i < metricsCacheSize; i++) {
      if (metricsCache[i].nodeId == nodeId) {
        return i;
      }
    }
    return -1;
  }
  
  /**
   * Find health cache index for node
   */
  int findHealthCacheIndex(uint32_t nodeId) {
    for (int i = 0; i < healthCacheSize; i++) {
      if (healthCache[i].nodeId == nodeId) {
        return i;
      }
    }
    return -1;
  }
  
  /**
   * Publish metrics to MQTT
   */
  void publishMetricsToMQTT(uint32_t from, JsonObject& obj) {
    String topic = topicPrefix + "metrics/" + String(from);
    
    // Build payload
    String payload;
    serializeJson(obj, payload);
    
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Publish health to MQTT
   */
  void publishHealthToMQTT(uint32_t from, JsonObject& obj) {
    String topic = topicPrefix + "health/" + String(from);
    
    // Build payload
    String payload;
    serializeJson(obj, payload);
    
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Publish critical health alert
   */
  void publishCriticalAlert(uint32_t from, JsonObject& obj) {
    String topic = topicPrefix + "alerts/critical";
    
    String payload = "{";
    payload += "\"alert_type\":\"critical_health\",";
    payload += "\"node_id\":" + String(from) + ",";
    payload += "\"health_status\":" + String((uint8_t)obj["health"]) + ",";
    payload += "\"problem_flags\":" + String((uint16_t)obj["problems"]) + ",";
    payload += "\"recommendations\":\"" + obj["recommend"].as<String>() + "\",";
    payload += "\"timestamp\":\"" + buildISO8601Timestamp() + "\"";
    payload += "}";
    
    mqttClient.publish(topic.c_str(), payload.c_str());
    Serial.printf("CRITICAL ALERT: Node %u health is critical!\n", from);
  }
  
  /**
   * Handle request metrics command from MQTT
   */
  void handleRequestMetrics(String& payload) {
    Serial.println("Handling request_metrics command");
    
    // Parse payload to get target node (0 = all nodes)
    DynamicJsonDocument doc(256);
    deserializeJson(doc, payload);
    uint32_t targetNode = doc["node_id"] | 0;
    
    // Create command package
    CommandPackage cmd;
    cmd.from = mesh.getNodeId();
    cmd.dest = targetNode;
    cmd.command = CMD_REQUEST_METRICS;
    cmd.commandId = millis();
    
    // Send to mesh
    String msg = cmd.toJsonString();
    if (targetNode == 0) {
      mesh.sendBroadcast(msg);
      Serial.println("Metrics requested from all nodes");
    } else {
      mesh.sendSingle(targetNode, msg);
      Serial.printf("Metrics requested from node %u\n", targetNode);
    }
  }
  
  /**
   * Handle request health command from MQTT
   */
  void handleRequestHealth(String& payload) {
    Serial.println("Handling request_health command");
    
    // Parse payload to get target node (0 = all nodes)
    DynamicJsonDocument doc(256);
    deserializeJson(doc, payload);
    uint32_t targetNode = doc["node_id"] | 0;
    
    // Create command package
    CommandPackage cmd;
    cmd.from = mesh.getNodeId();
    cmd.dest = targetNode;
    cmd.command = CMD_REQUEST_HEALTH;
    cmd.commandId = millis();
    
    // Send to mesh
    String msg = cmd.toJsonString();
    if (targetNode == 0) {
      mesh.sendBroadcast(msg);
      Serial.println("Health check requested from all nodes");
    } else {
      mesh.sendSingle(targetNode, msg);
      Serial.printf("Health check requested from node %u\n", targetNode);
    }
  }
  
  /**
   * Handle get aggregated command from MQTT
   */
  void handleGetAggregated(String& payload) {
    Serial.println("Handling get_aggregated command");
    
    // Immediately publish current aggregated stats
    publishAggregatedMetrics();
    publishAggregatedHealth();
  }
  
  /**
   * Handle metrics response from node
   */
  void handleMetricsResponse(uint32_t from, JsonObject& obj) {
    Serial.printf("Received metrics response from node %u\n", from);
    
    // Publish to response topic
    String topic = topicPrefix + "response/metrics";
    String payload;
    serializeJson(obj, payload);
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Handle health response from node
   */
  void handleHealthResponse(uint32_t from, JsonObject& obj) {
    Serial.printf("Received health response from node %u\n", from);
    
    // Publish to response topic
    String topic = topicPrefix + "response/health";
    String payload;
    serializeJson(obj, payload);
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Publish aggregated metrics
   */
  void publishAggregatedMetrics() {
    if (metricsCacheSize == 0) {
      Serial.println("No metrics to aggregate");
      return;
    }
    
    // Calculate aggregated statistics
    uint32_t totalCPU = 0;
    uint32_t totalHeap = 0;
    uint32_t totalThroughput = 0;
    uint32_t totalQuality = 0;
    int32_t totalRSSI = 0;
    uint32_t minHeap = 0xFFFFFFFF;
    uint32_t maxHeap = 0;
    uint8_t minQuality = 100;
    uint8_t maxQuality = 0;
    
    for (int i = 0; i < metricsCacheSize; i++) {
      totalCPU += metricsCache[i].cpuUsage;
      totalHeap += metricsCache[i].freeHeap;
      totalThroughput += metricsCache[i].currentThroughput;
      totalQuality += metricsCache[i].connectionQuality;
      totalRSSI += metricsCache[i].wifiRSSI;
      
      if (metricsCache[i].freeHeap < minHeap) minHeap = metricsCache[i].freeHeap;
      if (metricsCache[i].freeHeap > maxHeap) maxHeap = metricsCache[i].freeHeap;
      if (metricsCache[i].connectionQuality < minQuality) minQuality = metricsCache[i].connectionQuality;
      if (metricsCache[i].connectionQuality > maxQuality) maxQuality = metricsCache[i].connectionQuality;
    }
    
    // Build aggregated payload
    String topic = topicPrefix + "aggregated/metrics";
    String payload = "{";
    payload += "\"node_count\":" + String(metricsCacheSize) + ",";
    payload += "\"avg_cpu\":" + String(totalCPU / metricsCacheSize) + ",";
    payload += "\"avg_heap\":" + String(totalHeap / metricsCacheSize) + ",";
    payload += "\"min_heap\":" + String(minHeap) + ",";
    payload += "\"max_heap\":" + String(maxHeap) + ",";
    payload += "\"total_throughput\":" + String(totalThroughput) + ",";
    payload += "\"avg_quality\":" + String(totalQuality / metricsCacheSize) + ",";
    payload += "\"min_quality\":" + String(minQuality) + ",";
    payload += "\"max_quality\":" + String(maxQuality) + ",";
    payload += "\"avg_rssi\":" + String(totalRSSI / metricsCacheSize) + ",";
    payload += "\"timestamp\":\"" + buildISO8601Timestamp() + "\"";
    payload += "}";
    
    mqttClient.publish(topic.c_str(), payload.c_str());
    Serial.printf("Published aggregated metrics for %d nodes\n", metricsCacheSize);
  }
  
  /**
   * Publish aggregated health
   */
  void publishAggregatedHealth() {
    if (healthCacheSize == 0) {
      Serial.println("No health data to aggregate");
      return;
    }
    
    // Count nodes by health status
    uint8_t healthyCount = 0;
    uint8_t warningCount = 0;
    uint8_t criticalCount = 0;
    
    // Aggregate problem flags
    uint16_t aggregatedProblems = 0;
    
    // Average health scores
    uint32_t totalMemHealth = 0;
    uint32_t totalNetHealth = 0;
    uint32_t totalPerfHealth = 0;
    
    for (int i = 0; i < healthCacheSize; i++) {
      // Count by status
      if (healthCache[i].healthStatus == 2) healthyCount++;
      else if (healthCache[i].healthStatus == 1) warningCount++;
      else criticalCount++;
      
      // Aggregate problems
      aggregatedProblems |= healthCache[i].problemFlags;
      
      // Sum health scores
      totalMemHealth += healthCache[i].memoryHealth;
      totalNetHealth += healthCache[i].networkHealth;
      totalPerfHealth += healthCache[i].performanceHealth;
    }
    
    // Determine overall mesh health
    uint8_t meshHealth = 2;  // Healthy
    if (criticalCount > 0) meshHealth = 0;  // Critical
    else if (warningCount > 0) meshHealth = 1;  // Warning
    
    // Build aggregated payload
    String topic = topicPrefix + "aggregated/health";
    String payload = "{";
    payload += "\"node_count\":" + String(healthCacheSize) + ",";
    payload += "\"mesh_health\":" + String(meshHealth) + ",";
    payload += "\"healthy_nodes\":" + String(healthyCount) + ",";
    payload += "\"warning_nodes\":" + String(warningCount) + ",";
    payload += "\"critical_nodes\":" + String(criticalCount) + ",";
    payload += "\"aggregated_problems\":" + String(aggregatedProblems) + ",";
    payload += "\"avg_memory_health\":" + String(totalMemHealth / healthCacheSize) + ",";
    payload += "\"avg_network_health\":" + String(totalNetHealth / healthCacheSize) + ",";
    payload += "\"avg_performance_health\":" + String(totalPerfHealth / healthCacheSize) + ",";
    payload += "\"timestamp\":\"" + buildISO8601Timestamp() + "\"";
    payload += "}";
    
    mqttClient.publish(topic.c_str(), payload.c_str());
    Serial.printf("Published aggregated health: %d healthy, %d warning, %d critical\n", 
                  healthyCount, warningCount, criticalCount);
  }
  
  /**
   * Build ISO 8601 timestamp
   */
  String buildISO8601Timestamp() {
    // Simplified - in production use NTP time
    uint32_t uptime = millis() / 1000;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "1970-01-01T00:%02d:%02dZ", 
             (uptime / 60) % 60, uptime % 60);
    return String(buffer);
  }
};

#endif  // _ENHANCED_MQTT_BRIDGE_HPP_
