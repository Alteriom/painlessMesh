#ifndef MESH_EVENT_PUBLISHER_HPP
#define MESH_EVENT_PUBLISHER_HPP

#include "painlessMesh.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

/**
 * MeshEventPublisher - Publish real-time mesh events
 * 
 * Publishes mesh state change events conforming to @alteriom/mqtt-schema v0.5.0
 * mesh_event.schema.json specification.
 * 
 * Event Types:
 * - node_join: New node entered mesh
 * - node_leave: Node left mesh (clean disconnect)
 * - connection_lost: Direct connection failed
 * - connection_restored: Connection recovered
 * 
 * @author Alteriom Development Team
 * @date October 12, 2025
 */
class MeshEventPublisher {
private:
  painlessMesh& mesh;
  PubSubClient& mqttClient;
  String meshId;
  String eventTopic;
  String firmwareVersion;
  
public:
  MeshEventPublisher(painlessMesh& mesh, PubSubClient& mqttClient, 
                     const String& meshId, const String& topic = "alteriom/mesh/events",
                     const String& fwVersion = "GW 2.3.4")
    : mesh(mesh), mqttClient(mqttClient), meshId(meshId), 
      eventTopic(topic), firmwareVersion(fwVersion) {}
  
  /**
   * Publish node join event
   * Called when a new node connects to the mesh
   */
  void publishNodeJoin(uint32_t nodeId) {
    JsonDocument doc;  // ArduinoJson v7: automatic sizing
    
    // Envelope
    doc["schema_version"] = 1;
    doc["device_id"] = generateDeviceId(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = firmwareVersion;
    
    // Event
    doc["event"] = "mesh_event";
    doc["event_type"] = "node_join";
    doc["mesh_id"] = meshId;
    
    JsonArray affected = doc["affected_nodes"].to<JsonArray>();
    affected.add(generateDeviceId(nodeId));
    
    JsonObject details = doc["details"].to<JsonObject>();
    details["total_nodes"] = mesh.getNodeList().size() + 1;
    details["timestamp"] = getISO8601Timestamp();
    
    publishEvent(doc, "Node Join");
  }
  
  /**
   * Publish node leave event
   * Called when a node disconnects from the mesh
   */
  void publishNodeLeave(uint32_t nodeId) {
    JsonDocument doc;  // ArduinoJson v7: automatic sizing
    
    // Envelope
    doc["schema_version"] = 1;
    doc["device_id"] = generateDeviceId(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = firmwareVersion;
    
    // Event
    doc["event"] = "mesh_event";
    doc["event_type"] = "node_leave";
    doc["mesh_id"] = meshId;
    
    JsonArray affected = doc["affected_nodes"].to<JsonArray>();
    affected.add(generateDeviceId(nodeId));
    
    JsonObject details = doc["details"].to<JsonObject>();
    details["reason"] = "connection_lost";
    details["last_seen"] = getISO8601Timestamp();
    details["total_nodes"] = mesh.getNodeList().size() + 1;
    
    publishEvent(doc, "Node Leave");
  }
  
  /**
   * Publish connection lost event
   * Called when a direct connection fails
   */
  void publishConnectionLost(uint32_t nodeId) {
    JsonDocument doc;  // ArduinoJson v7: automatic sizing
    
    doc["schema_version"] = 1;
    doc["device_id"] = generateDeviceId(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = firmwareVersion;
    
    doc["event"] = "mesh_event";
    doc["event_type"] = "connection_lost";
    doc["mesh_id"] = meshId;
    
    JsonArray affected = doc["affected_nodes"].to<JsonArray>();
    affected.add(generateDeviceId(nodeId));
    
    JsonObject details = doc["details"].to<JsonObject>();
    details["reason"] = "timeout";
    details["timestamp"] = getISO8601Timestamp();
    
    publishEvent(doc, "Connection Lost");
  }
  
  /**
   * Publish connection restored event
   * Called when a connection is re-established
   */
  void publishConnectionRestored(uint32_t nodeId) {
    JsonDocument doc;  // ArduinoJson v7: automatic sizing
    
    doc["schema_version"] = 1;
    doc["device_id"] = generateDeviceId(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = firmwareVersion;
    
    doc["event"] = "mesh_event";
    doc["event_type"] = "connection_restored";
    doc["mesh_id"] = meshId;
    
    JsonArray affected = doc["affected_nodes"].to<JsonArray>();
    affected.add(generateDeviceId(nodeId));
    
    JsonObject details = doc["details"].to<JsonObject>();
    details["timestamp"] = getISO8601Timestamp();
    
    publishEvent(doc, "Connection Restored");
  }
  
  /**
   * Publish network split event
   * Called when mesh partitions into separate segments
   */
  void publishNetworkSplit() {
    JsonDocument doc;  // ArduinoJson v7: automatic sizing
    
    doc["schema_version"] = 1;
    doc["device_id"] = generateDeviceId(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = firmwareVersion;
    
    doc["event"] = "mesh_event";
    doc["event_type"] = "network_split";
    doc["mesh_id"] = meshId;
    
    JsonArray affected = doc["affected_nodes"].to<JsonArray>();
    // TODO: Detect which nodes are in different partitions
    
    JsonObject details = doc["details"].to<JsonObject>();
    details["timestamp"] = getISO8601Timestamp();
    details["partition_count"] = 2;  // TODO: Calculate actual partitions
    
    publishEvent(doc, "Network Split");
  }
  
  /**
   * Publish network merged event
   * Called when mesh partitions rejoin
   */
  void publishNetworkMerged() {
    JsonDocument doc;  // ArduinoJson v7: automatic sizing
    
    doc["schema_version"] = 1;
    doc["device_id"] = generateDeviceId(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = firmwareVersion;
    
    doc["event"] = "mesh_event";
    doc["event_type"] = "network_merged";
    doc["mesh_id"] = meshId;
    
    JsonArray affected = doc["affected_nodes"].to<JsonArray>();
    // TODO: List nodes that rejoined
    
    JsonObject details = doc["details"].to<JsonObject>();
    details["timestamp"] = getISO8601Timestamp();
    
    publishEvent(doc, "Network Merged");
  }
  
private:
  /**
   * Internal method to publish event to MQTT
   */
  void publishEvent(const JsonDocument& doc, const char* eventName) {
    if (!mqttClient.connected()) {
      Serial.printf("[Event] ⚠️ MQTT not connected, cannot publish %s\n", eventName);
      return;
    }
    
    String payload;
    serializeJson(doc, payload);
    
    bool success = mqttClient.publish(eventTopic.c_str(), payload.c_str());
    
    if (success) {
      Serial.printf("[Event] ✅ Published: %s (node: %s)\n", 
                    eventName,
                    doc["affected_nodes"][0].as<const char*>());
    } else {
      Serial.printf("[Event] ❌ Failed to publish: %s\n", eventName);
    }
  }
  
  /**
   * Generate device ID in Alteriom format (ALT-XXXXXXXXXXXX)
   */
  String generateDeviceId(uint32_t nodeId) {
    char deviceId[20];
    snprintf(deviceId, sizeof(deviceId), "ALT-%012X", nodeId);
    return String(deviceId);
  }
  
  /**
   * Get current timestamp in ISO 8601 format
   * TODO: Use proper NTP time if available
   */
  String getISO8601Timestamp() {
    // For now, use epoch-based timestamp
    // In production, this should use actual NTP-synced time
    unsigned long ms = millis();
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "2025-10-12T%02lu:%02lu:%02luZ",
             (ms / 3600000) % 24,
             (ms / 60000) % 60,
             (ms / 1000) % 60);
    return String(timestamp);
  }
};

#endif  // MESH_EVENT_PUBLISHER_HPP
