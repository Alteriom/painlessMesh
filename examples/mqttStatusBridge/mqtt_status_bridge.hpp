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
 * - Performance metrics (Alteriom MQTT Schema v1 compliant)
 * - Active alerts
 * - Per-node health monitoring
 * 
 * MQTT Topics:
 * - mesh/status/nodes         - Node list with basic info
 * - mesh/status/topology      - Complete mesh structure
 * - mesh/status/metrics       - Gateway metrics (schema v1 compliant)
 * - mesh/status/alerts        - Active alert conditions
 * - mesh/status/node/{id}     - Per-node detailed status
 * 
 * Compliance:
 * - Uses @alteriom/mqtt-schema v1 for gateway_metrics messages
 * - Includes required envelope fields (schema_version, device_id, etc.)
 * - ISO 8601 timestamps
 */

#include <Arduino.h>
#include <painlessMesh.h>
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
  
  // Device identification (for Alteriom schema compliance)
  String deviceId = "";
  String firmwareVersion = "1.0.0";
  
  // Last publish time
  unsigned long lastPublishTime = 0;
  
  // Task scheduler
  Task publishTask;

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
   * Set device ID for Alteriom schema compliance
   * @param id Device identifier (defaults to mesh node ID if not set)
   */
  void setDeviceId(const String& id) {
    deviceId = id;
  }
  
  /**
   * Set firmware version for Alteriom schema compliance
   * @param version Firmware version string (default: "1.0.0")
   */
  void setFirmwareVersion(const String& version) {
    firmwareVersion = version;
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
    publishTask.set(TASK_MILLISECOND * publishInterval, 
                   TASK_FOREVER, 
                   [this]() { this->publishStatus(); });
    mesh.mScheduler.addTask(publishTask);
    publishTask.enable();
    
    Serial.println("MQTT Status Bridge started");
    Serial.printf("Publishing every %d ms to %s*\n", publishInterval, topicPrefix.c_str());
  }
  
  /**
   * Stop publishing
   */
  void stop() {
    publishTask.disable();
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
   * 
   * Schema-compliant with proposed mesh_node_list.schema.json v1
   * Uses envelope + nodes array format for interoperability
   */
  void publishNodeList() {
    auto nodes = mesh.getNodeList(true);  // Include self
    
    // Get device ID
    String devId = deviceId.length() > 0 ? deviceId : String(mesh.getNodeId());
    
    // Build ISO 8601 timestamp
    String timestamp = buildISO8601Timestamp();
    
    // Build schema-compliant message with envelope
    String payload = "{";
    // Envelope fields (required by proposed schema)
    payload += "\"schema_version\":1";
    payload += ",\"device_id\":\"" + devId + "\"";
    payload += ",\"device_type\":\"gateway\"";
    payload += ",\"timestamp\":\"" + timestamp + "\"";
    payload += ",\"firmware_version\":\"" + firmwareVersion + "\"";
    
    // Nodes array (required by proposed schema)
    payload += ",\"nodes\":[";
    bool firstNode = true;
    for (auto nodeId : nodes) {
      if (!firstNode) payload += ",";
      firstNode = false;
      payload += "{";
      payload += "\"node_id\":\"" + String(nodeId) + "\"";
      payload += ",\"status\":\"online\"";  // All listed nodes are online
      payload += ",\"last_seen\":\"" + timestamp + "\"";
      payload += "}";
    }
    payload += "]";
    
    // Additional fields (optional in proposed schema)
    payload += ",\"node_count\":" + String(nodes.size());
    payload += ",\"mesh_id\":\"" + String(mesh.getNodeId()) + "\"";
    payload += "}";
    
    String topic = topicPrefix + "nodes";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Publish mesh topology as JSON
   * Topic: mesh/status/topology
   * 
   * Schema-compliant with proposed mesh_topology.schema.json v1
   * Uses envelope + connections array format
   * 
   * Note: painlessMesh native topology is complex. This implementation
   * creates a simplified schema-compliant version. For full topology details,
   * use the native format via a separate topic if needed.
   */
  void publishTopology() {
    auto nodes = mesh.getNodeList(true);
    
    // Get device ID
    String devId = deviceId.length() > 0 ? deviceId : String(mesh.getNodeId());
    
    // Build ISO 8601 timestamp
    String timestamp = buildISO8601Timestamp();
    
    // Build schema-compliant message with envelope
    String payload = "{";
    // Envelope fields (required by proposed schema)
    payload += "\"schema_version\":1";
    payload += ",\"device_id\":\"" + devId + "\"";
    payload += ",\"device_type\":\"gateway\"";
    payload += ",\"timestamp\":\"" + timestamp + "\"";
    payload += ",\"firmware_version\":\"" + firmwareVersion + "\"";
    
    // Connections array (required by proposed schema)
    // Simplified: Each node connected to root (gateway)
    payload += ",\"connections\":[";
    bool first = true;
    for (auto nodeId : nodes) {
      // Skip self (root node)
      if (nodeId == mesh.getNodeId()) continue;
      
      if (!first) payload += ",";
      first = false;
      
      payload += "{";
      payload += "\"from_node\":\"" + String(nodeId) + "\"";
      payload += ",\"to_node\":\"" + devId + "\"";
      payload += ",\"link_quality\":0.9";  // Default - can be enhanced
      payload += ",\"hop_count\":1";  // Simplified - assume direct connection
      payload += "}";
    }
    payload += "]";
    
    // Additional fields (optional in proposed schema)
    payload += ",\"root_node\":\"" + devId + "\"";
    payload += ",\"total_connections\":" + String(nodes.size() > 0 ? nodes.size() - 1 : 0);
    payload += "}";
    
    String topic = topicPrefix + "topology";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Publish performance metrics
   * Topic: mesh/status/metrics
   * 
   * Compliant with @alteriom/mqtt-schema v1 gateway_metrics format
   */
  void publishMetrics() {
    auto nodes = mesh.getNodeList(true);
    
    // Get device ID (use mesh node ID if not explicitly set)
    String devId = deviceId.length() > 0 ? deviceId : String(mesh.getNodeId());
    
    // Build ISO 8601 timestamp (simplified - use actual RTC/NTP if available)
    // Format: YYYY-MM-DDTHH:MM:SSZ
    // Note: This uses millis() as fallback. For production, use:
    // - NTP time sync via mesh.getNodeTime() or WiFi NTP client
    // - RTC module for accurate timestamps
    String timestamp;
    #ifdef USE_NTP_TIME
    // If NTP is available, use it
    timestamp = getFormattedTimeISO8601();
    #else
    // Fallback: Use epoch + millis for relative time
    // This is a simplified timestamp - recommend NTP sync for production
    unsigned long totalSeconds = millis() / 1000;
    unsigned long days = totalSeconds / 86400;
    unsigned long hours = (totalSeconds / 3600) % 24;
    unsigned long minutes = (totalSeconds / 60) % 60;
    unsigned long seconds = totalSeconds % 60;
    char timeStr[32];
    // Use Unix epoch start (1970-01-01) + elapsed time
    sprintf(timeStr, "1970-01-%02luT%02lu:%02lu:%02luZ", 
            1 + days, hours, minutes, seconds);
    timestamp = timeStr;
    #endif
    
    // Build Alteriom schema v1 compliant gateway_metrics message
    String payload = "{";
    // Envelope fields (required by schema)
    payload += "\"schema_version\":1";
    payload += ",\"device_id\":\"" + devId + "\"";
    payload += ",\"device_type\":\"gateway\"";
    payload += ",\"timestamp\":\"" + timestamp + "\"";
    payload += ",\"firmware_version\":\"" + firmwareVersion + "\"";
    
    // Metrics object (required by gateway_metrics schema)
    payload += ",\"metrics\":{";
    payload += "\"uptime_s\":" + String(millis() / 1000);
    payload += ",\"mesh_nodes\":" + String(nodes.size());
    
#if defined(ESP32) || defined(ESP8266)
    uint32_t totalHeap = ESP.getFreeHeap();
    // Rough estimate of memory usage percentage (assuming typical ESP heap size)
    uint32_t estimatedTotalHeap = 320000; // Adjust based on actual hardware
#ifdef ESP8266
    estimatedTotalHeap = 80000;
#endif
    float memoryUsagePct = 100.0 - ((float)totalHeap / estimatedTotalHeap * 100.0);
    if (memoryUsagePct < 0) memoryUsagePct = 0;
    if (memoryUsagePct > 100) memoryUsagePct = 100;
    
    payload += ",\"memory_usage_pct\":" + String(memoryUsagePct, 1);
#endif
    
    payload += ",\"connected_devices\":" + String(nodes.size());
    payload += "}"; // End metrics object
    payload += "}"; // End message
    
    String topic = topicPrefix + "metrics";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Publish active alerts
   * Topic: mesh/status/alerts
   * 
   * Schema-compliant with proposed mesh_alert.schema.json v1
   * Uses envelope + alerts array format
   * 
   * Tracks node health and publishes alerts when issues are detected.
   */
  void publishAlerts() {
    auto nodes = mesh.getNodeList(true);
    
    // Get device ID
    String devId = deviceId.length() > 0 ? deviceId : String(mesh.getNodeId());
    
    // Build ISO 8601 timestamp
    String timestamp = buildISO8601Timestamp();
    
    // Build schema-compliant message with envelope
    String payload = "{";
    // Envelope fields (required by proposed schema)
    payload += "\"schema_version\":1";
    payload += ",\"device_id\":\"" + devId + "\"";
    payload += ",\"device_type\":\"gateway\"";
    payload += ",\"timestamp\":\"" + timestamp + "\"";
    payload += ",\"firmware_version\":\"" + firmwareVersion + "\"";
    
    // Alerts array (required by proposed schema)
    payload += ",\"alerts\":[";
    bool hasAlerts = false;
    
    // Alert: No nodes in mesh
    if (nodes.size() <= 1) {  // Only gateway
      if (hasAlerts) payload += ",";
      payload += "{";
      payload += "\"alert_type\":\"node_offline\"";
      payload += ",\"severity\":\"warning\"";
      payload += ",\"message\":\"No sensor nodes in mesh network\"";
      payload += ",\"alert_id\":\"alert-no-nodes\"";
      payload += "}";
      hasAlerts = true;
    }
    
#if defined(ESP32) || defined(ESP8266)
    // Alert: Low memory on gateway
    size_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 10000) {
      if (hasAlerts) payload += ",";
      payload += "{";
      payload += "\"alert_type\":\"low_memory\"";
      payload += ",\"severity\":\"critical\"";
      payload += ",\"message\":\"Gateway free heap below 10KB\"";
      payload += ",\"node_id\":\"" + devId + "\"";
      payload += ",\"metric_value\":" + String(freeHeap / 1024.0);
      payload += ",\"threshold\":10.0";
      payload += ",\"alert_id\":\"alert-low-mem-gw\"";
      payload += "}";
      hasAlerts = true;
    }
#endif
    
    payload += "]";
    
    // Additional fields (optional in proposed schema)
    payload += ",\"alert_count\":" + String(hasAlerts ? 1 : 0);
    payload += "}";
    
    String topic = topicPrefix + "alerts";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  
  /**
   * Publish detailed status for each node
   * Topic: mesh/status/node/{nodeId}
   * 
   * Schema-compliant with sensor_status.schema.json v1 (existing in @alteriom/mqtt-schema)
   * Uses envelope + status enum format
   * 
   * Warning: This can generate a lot of MQTT traffic for large meshes!
   * Enable only if per-node status is required.
   */
  void publishPerNodeStatus() {
    auto nodes = mesh.getNodeList(true);  // Include all nodes
    
    // Build ISO 8601 timestamp
    String timestamp = buildISO8601Timestamp();
    
    for (auto nodeId : nodes) {
      // Build schema-compliant message with envelope
      String payload = "{";
      // Envelope fields (required by sensor_status schema)
      payload += "\"schema_version\":1";
      payload += ",\"device_id\":\"" + String(nodeId) + "\"";
      
      // Determine device type - gateway or sensor
      if (nodeId == mesh.getNodeId()) {
        payload += ",\"device_type\":\"gateway\"";
      } else {
        payload += ",\"device_type\":\"sensor\"";
      }
      
      payload += ",\"timestamp\":\"" + timestamp + "\"";
      payload += ",\"firmware_version\":\"" + firmwareVersion + "\"";
      
      // Status field (required by sensor_status schema)
      bool isConnected = mesh.isConnected(nodeId);
      payload += ",\"status\":\"" + String(isConnected ? "online" : "offline") + "\"";
      
      // Optional fields in sensor_status schema
      // Note: signal_strength would require RSSI data from mesh
      // battery_level would require node to report this data
      
      payload += "}";
      
      String topic = topicPrefix + "node/" + String(nodeId);
      mqttClient.publish(topic.c_str(), payload.c_str());
    }
  }
  
private:
  /**
   * Helper function to build ISO 8601 timestamp
   * Shared across all publishing methods for consistency
   */
  String buildISO8601Timestamp() {
    #ifdef USE_NTP_TIME
    // If NTP is available, use it
    return getFormattedTimeISO8601();
    #else
    // Fallback: Use epoch + millis for relative time
    unsigned long totalSeconds = millis() / 1000;
    unsigned long days = totalSeconds / 86400;
    unsigned long hours = (totalSeconds / 3600) % 24;
    unsigned long minutes = (totalSeconds / 60) % 60;
    unsigned long seconds = totalSeconds % 60;
    char timeStr[32];
    sprintf(timeStr, "1970-01-%02luT%02lu:%02lu:%02luZ", 
            1 + days, hours, minutes, seconds);
    return String(timeStr);
    #endif
  }
};

#endif // _MQTT_STATUS_BRIDGE_HPP_
