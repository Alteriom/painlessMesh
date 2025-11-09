/**
 * Bridge Health Monitoring Example
 * 
 * Demonstrates the new bridge health metrics API for monitoring
 * bridge connectivity, signal quality, traffic, and performance.
 * 
 * Features:
 * - Periodic metrics collection and logging
 * - MQTT integration for publishing metrics
 * - Prometheus-style metrics export
 * - Manual metrics reset and JSON export
 * 
 * Hardware: ESP32 or ESP8266
 * 
 * Based on painlessMesh v1.7.7+ bridge health monitoring feature
 */

#include "painlessMesh.h"

// Mesh network configuration
#define   MESH_PREFIX     "BridgeMesh"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

// Bridge node configuration
#define   STATION_SSID    "YourWiFiSSID"
#define   STATION_PASSWORD "YourWiFiPassword"

// Metrics reporting interval (60 seconds)
#define   METRICS_INTERVAL 60000

Scheduler userScheduler;
painlessMesh mesh;

// MQTT client (if using MQTT integration)
// #include <PubSubClient.h>
// WiFiClient espClient;
// PubSubClient mqttClient(espClient);

/**
 * Metrics callback - called periodically with current metrics
 * This is where you'd publish to MQTT, send to logging service, etc.
 */
void metricsCallback(painlessmesh::BridgeHealthMetrics metrics) {
  Serial.println("\n========== Bridge Health Metrics ==========");
  
  // Connectivity
  Serial.println("Connectivity:");
  Serial.printf("  Uptime: %u seconds\n", metrics.uptimeSeconds);
  Serial.printf("  Internet Uptime: %u seconds\n", metrics.internetUptimeSeconds);
  Serial.printf("  Total Disconnects: %u\n", metrics.totalDisconnects);
  
  // Signal Quality
  Serial.println("\nSignal Quality:");
  Serial.printf("  Current RSSI: %d dBm\n", metrics.currentRSSI);
  Serial.printf("  Average RSSI: %d dBm\n", metrics.avgRSSI);
  Serial.printf("  Min RSSI: %d dBm\n", metrics.minRSSI);
  Serial.printf("  Max RSSI: %d dBm\n", metrics.maxRSSI);
  
  // Traffic
  Serial.println("\nTraffic:");
  Serial.printf("  Bytes RX: %llu\n", metrics.bytesRx);
  Serial.printf("  Bytes TX: %llu\n", metrics.bytesTx);
  Serial.printf("  Messages RX: %u\n", metrics.messagesRx);
  Serial.printf("  Messages TX: %u\n", metrics.messagesTx);
  Serial.printf("  Messages Dropped: %u\n", metrics.messagesDropped);
  
  // Performance
  Serial.println("\nPerformance:");
  Serial.printf("  Avg Latency: %u ms\n", metrics.avgLatencyMs);
  Serial.printf("  Packet Loss: %u%%\n", metrics.packetLossPercent);
  Serial.printf("  Mesh Nodes: %u\n", metrics.meshNodeCount);
  
  Serial.println("==========================================\n");
  
  // MQTT Integration Example
  // Uncomment if using MQTT
  /*
  if (mqttClient.connected()) {
    String json = mesh.getHealthMetricsJSON();
    mqttClient.publish("bridge/metrics", json.c_str());
    Serial.println("Metrics published to MQTT");
  }
  */
}

/**
 * Export metrics in Prometheus format
 * Call this from a web server endpoint for Prometheus scraping
 */
String exportPrometheus() {
  auto metrics = mesh.getBridgeHealthMetrics();
  
  String output = "";
  
  // Connectivity metrics
  output += "# HELP bridge_uptime_seconds Bridge uptime in seconds\n";
  output += "# TYPE bridge_uptime_seconds counter\n";
  output += "bridge_uptime_seconds " + String(metrics.uptimeSeconds) + "\n";
  
  output += "# HELP bridge_internet_uptime_seconds Internet connectivity uptime\n";
  output += "# TYPE bridge_internet_uptime_seconds counter\n";
  output += "bridge_internet_uptime_seconds " + String(metrics.internetUptimeSeconds) + "\n";
  
  output += "# HELP bridge_disconnects_total Total number of disconnections\n";
  output += "# TYPE bridge_disconnects_total counter\n";
  output += "bridge_disconnects_total " + String(metrics.totalDisconnects) + "\n";
  
  // Signal quality metrics
  output += "# HELP bridge_rssi_dbm WiFi signal strength in dBm\n";
  output += "# TYPE bridge_rssi_dbm gauge\n";
  output += "bridge_rssi_dbm{type=\"current\"} " + String(metrics.currentRSSI) + "\n";
  output += "bridge_rssi_dbm{type=\"average\"} " + String(metrics.avgRSSI) + "\n";
  output += "bridge_rssi_dbm{type=\"min\"} " + String(metrics.minRSSI) + "\n";
  output += "bridge_rssi_dbm{type=\"max\"} " + String(metrics.maxRSSI) + "\n";
  
  // Traffic metrics
  output += "# HELP bridge_bytes_total Total bytes transferred\n";
  output += "# TYPE bridge_bytes_total counter\n";
  output += "bridge_bytes_total{direction=\"rx\"} " + String((unsigned long)metrics.bytesRx) + "\n";
  output += "bridge_bytes_total{direction=\"tx\"} " + String((unsigned long)metrics.bytesTx) + "\n";
  
  output += "# HELP bridge_messages_total Total messages processed\n";
  output += "# TYPE bridge_messages_total counter\n";
  output += "bridge_messages_total{direction=\"rx\"} " + String(metrics.messagesRx) + "\n";
  output += "bridge_messages_total{direction=\"tx\"} " + String(metrics.messagesTx) + "\n";
  output += "bridge_messages_total{status=\"dropped\"} " + String(metrics.messagesDropped) + "\n";
  
  // Performance metrics
  output += "# HELP bridge_latency_ms Average message latency in milliseconds\n";
  output += "# TYPE bridge_latency_ms gauge\n";
  output += "bridge_latency_ms " + String(metrics.avgLatencyMs) + "\n";
  
  output += "# HELP bridge_packet_loss_percent Packet loss percentage\n";
  output += "# TYPE bridge_packet_loss_percent gauge\n";
  output += "bridge_packet_loss_percent " + String(metrics.packetLossPercent) + "\n";
  
  output += "# HELP bridge_mesh_nodes Total nodes in mesh network\n";
  output += "# TYPE bridge_mesh_nodes gauge\n";
  output += "bridge_mesh_nodes " + String(metrics.meshNodeCount) + "\n";
  
  return output;
}

/**
 * Manual metrics check task
 * Demonstrates how to manually get metrics on demand
 */
Task taskManualMetricsCheck(10000, TASK_FOREVER, [](){
  // Get metrics JSON for logging or transmission
  String json = mesh.getHealthMetricsJSON();
  Serial.println("JSON Metrics: " + json);
});

void setup() {
  Serial.begin(115200);
  
  // Mesh initialization
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Configure as a bridge (root) node
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  
  // Connect to WiFi station for Internet access
  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname("BridgeNode");
  
  // Set up periodic health metrics callback (every 60 seconds)
  mesh.onHealthMetricsUpdate(metricsCallback, METRICS_INTERVAL);
  
  // Optional: Add manual metrics check task
  // userScheduler.addTask(taskManualMetricsCheck);
  // taskManualMetricsCheck.enable();
  
  Serial.println("Bridge with health monitoring started");
  Serial.println("Metrics will be reported every 60 seconds");
}

void loop() {
  mesh.update();
  
  // If you have a web server for Prometheus endpoint:
  // server.on("/metrics", HTTP_GET, [](){
  //   server.send(200, "text/plain", exportPrometheus());
  // });
}
