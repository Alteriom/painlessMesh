#ifndef MESH_TOPOLOGY_REPORTER_HPP
#define MESH_TOPOLOGY_REPORTER_HPP

#include "painlessMesh.h"
#include <ArduinoJson.h>

/**
 * MeshTopologyReporter - Generate schema-compliant topology messages
 * 
 * Generates mesh topology messages conforming to @alteriom/mqtt-schema v0.5.0
 * mesh_topology.schema.json specification.
 * 
 * Features:
 * - Full topology snapshots with nodes, connections, and metrics
 * - Incremental updates for node join/leave events
 * - Change detection to avoid redundant publishes
 * - Network metrics aggregation
 * 
 * @author Alteriom Development Team
 * @date October 12, 2025
 */
class MeshTopologyReporter {
private:
  painlessMesh& mesh;
  String meshId;
  String lastTopologyHash;  // Detect changes
  String firmwareVersion;
  
public:
  MeshTopologyReporter(painlessMesh& mesh, const String& meshId, const String& fwVersion = "GW 2.3.4") 
    : mesh(mesh), meshId(meshId), firmwareVersion(fwVersion) {}
  
  /**
   * Generate full topology message (schema v0.5.0 compliant)
   * 
   * Returns JSON string ready for MQTT publish to:
   * alteriom/mesh/{mesh_id}/topology
   */
  String generateFullTopology() {
    DynamicJsonDocument doc(8192);  // Large buffer for full topology
    
    // Envelope fields (from @alteriom/mqtt-schema)
    doc["schema_version"] = 1;
    doc["device_id"] = generateDeviceId(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = firmwareVersion;
    
    // Event discriminator
    doc["event"] = "mesh_topology";
    
    // Mesh identification
    doc["mesh_id"] = meshId;
    doc["gateway_node_id"] = generateDeviceId(mesh.getNodeId());
    
    // Nodes array
    JsonArray nodes = doc.createNestedArray("nodes");
    addNodeInfo(nodes, mesh.getNodeId(), "gateway");  // Self
    
    auto nodeList = mesh.getNodeList(false);
    for (auto nodeId : nodeList) {
      addNodeInfo(nodes, nodeId, "sensor");  // Assume sensor role
    }
    
    // Connections array
    JsonArray connections = doc.createNestedArray("connections");
    addConnectionInfo(connections);
    
    // Network metrics
    JsonObject metrics = doc.createNestedObject("metrics");
    metrics["total_nodes"] = nodeList.size() + 1;  // +1 for gateway
    metrics["online_nodes"] = nodeList.size() + 1;
    metrics["network_diameter"] = calculateNetworkDiameter();
    metrics["avg_connection_quality"] = calculateAvgQuality();
    metrics["messages_per_second"] = calculateThroughput();
    
    doc["update_type"] = "full";
    
    String output;
    serializeJson(doc, output);
    return output;
  }
  
  /**
   * Generate incremental topology update
   * 
   * @param nodeId Node that joined or left
   * @param joined true if node joined, false if left
   */
  String generateIncrementalUpdate(uint32_t nodeId, bool joined) {
    DynamicJsonDocument doc(2048);
    
    // Envelope fields
    doc["schema_version"] = 1;
    doc["device_id"] = generateDeviceId(mesh.getNodeId());
    doc["device_type"] = "gateway";
    doc["timestamp"] = getISO8601Timestamp();
    doc["firmware_version"] = firmwareVersion;
    
    doc["event"] = "mesh_topology";
    doc["mesh_id"] = meshId;
    doc["gateway_node_id"] = generateDeviceId(mesh.getNodeId());
    
    // Node that changed
    JsonArray nodes = doc.createNestedArray("nodes");
    if (joined) {
      addNodeInfo(nodes, nodeId, "sensor");
    } else {
      // For node leaving, mark as offline
      JsonObject node = nodes.createNestedObject();
      node["node_id"] = generateDeviceId(nodeId);
      node["status"] = "offline";
    }
    
    // Update connections
    JsonArray connections = doc.createNestedArray("connections");
    // Add connections related to this node
    addConnectionInfoForNode(connections, nodeId);
    
    // Updated metrics
    auto nodeList = mesh.getNodeList(false);
    JsonObject metrics = doc.createNestedObject("metrics");
    metrics["total_nodes"] = nodeList.size() + 1;
    metrics["online_nodes"] = nodeList.size() + 1;
    
    doc["update_type"] = "incremental";
    
    String output;
    serializeJson(doc, output);
    return output;
  }
  
  /**
   * Check if topology has changed since last check
   * Updates internal hash if changed
   */
  bool hasTopologyChanged() {
    String currentHash = calculateTopologyHash();
    if (currentHash != lastTopologyHash) {
      lastTopologyHash = currentHash;
      return true;
    }
    return false;
  }
  
private:
  /**
   * Add node information to JSON array
   */
  void addNodeInfo(JsonArray& nodes, uint32_t nodeId, const char* role) {
    JsonObject node = nodes.createNestedObject();
    node["node_id"] = generateDeviceId(nodeId);
    node["role"] = role;
    node["status"] = "online";
    node["last_seen"] = getISO8601Timestamp();
    
    // Gateway node has additional info
    if (nodeId == mesh.getNodeId()) {
      node["firmware_version"] = firmwareVersion;
      node["uptime_seconds"] = millis() / 1000;
#if defined(ESP32) || defined(ESP8266)
      node["free_memory_kb"] = ESP.getFreeHeap() / 1024;
#else
      node["free_memory_kb"] = 0;
#endif
      node["connection_count"] = mesh.getNodeList().size();
    } else {
      // For other nodes, use placeholder values
      // TODO: Get actual values from node status messages
      node["firmware_version"] = "SN 2.3.4";
      node["uptime_seconds"] = 0;
      node["free_memory_kb"] = 0;
      node["connection_count"] = 0;
    }
  }
  
  /**
   * Add connection information to JSON array
   */
  void addConnectionInfo(JsonArray& connections) {
    // Get direct connections with quality metrics
    auto connDetails = mesh.getConnectionDetails();
    
    String gatewayId = generateDeviceId(mesh.getNodeId());
    
    for (auto& conn : connDetails) {
      JsonObject connObj = connections.createNestedObject();
      connObj["from_node"] = gatewayId;
      connObj["to_node"] = generateDeviceId(conn.nodeId);
      connObj["quality"] = conn.quality;
      connObj["latency_ms"] = conn.avgLatency;
      connObj["rssi"] = conn.rssi;
      connObj["hop_count"] = conn.hopCount;
    }
    
    // TODO: Add indirect connections from mesh topology
    // For now, we only report direct connections from gateway
  }
  
  /**
   * Add connection information for a specific node
   */
  void addConnectionInfoForNode(JsonArray& connections, uint32_t nodeId) {
    auto connDetails = mesh.getConnectionDetails();
    String gatewayId = generateDeviceId(mesh.getNodeId());
    String targetId = generateDeviceId(nodeId);
    
    for (auto& conn : connDetails) {
      if (conn.nodeId == nodeId) {
        JsonObject connObj = connections.createNestedObject();
        connObj["from_node"] = gatewayId;
        connObj["to_node"] = targetId;
        connObj["quality"] = conn.quality;
        connObj["latency_ms"] = conn.avgLatency;
        connObj["rssi"] = conn.rssi;
        connObj["hop_count"] = conn.hopCount;
      }
    }
  }
  
  /**
   * Calculate network diameter (max hop count)
   */
  int calculateNetworkDiameter() {
    int maxHops = 0;
    auto nodeList = mesh.getNodeList(false);
    
    for (auto nodeId : nodeList) {
      int hops = mesh.getHopCount(nodeId);
      if (hops > maxHops) {
        maxHops = hops;
      }
    }
    
    return maxHops;
  }
  
  /**
   * Calculate average connection quality
   */
  float calculateAvgQuality() {
    auto connDetails = mesh.getConnectionDetails();
    
    if (connDetails.empty()) {
      return 0.0;
    }
    
    int totalQuality = 0;
    for (auto& conn : connDetails) {
      totalQuality += conn.quality;
    }
    
    return (float)totalQuality / connDetails.size();
  }
  
  /**
   * Calculate network throughput (messages per second)
   */
  float calculateThroughput() {
    // TODO: Implement actual throughput tracking
    // For now return placeholder
    return 12.4;
  }
  
  /**
   * Calculate topology hash for change detection
   */
  String calculateTopologyHash() {
    auto nodeList = mesh.getNodeList(true);
    String hash = String(nodeList.size());
    for (auto nodeId : nodeList) {
      hash += String(nodeId);
    }
    return hash;
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

#endif  // MESH_TOPOLOGY_REPORTER_HPP
