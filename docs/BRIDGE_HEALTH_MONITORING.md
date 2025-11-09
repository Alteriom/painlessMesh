# Bridge Health Monitoring & Metrics Collection

## Overview

The Bridge Health Monitoring feature provides comprehensive operational visibility into bridge health, connectivity quality, and performance metrics. This is essential for production deployments, troubleshooting, and capacity planning.

## Features

✅ **Connectivity Metrics** - Track uptime, internet connectivity, and disconnection events  
✅ **Signal Quality** - Monitor WiFi signal strength (RSSI) statistics  
✅ **Traffic Metrics** - Measure bytes and messages transmitted/received  
✅ **Performance Metrics** - Calculate latency and packet loss  
✅ **JSON Export** - Easy integration with monitoring tools  
✅ **Prometheus Support** - Export metrics for Prometheus scraping  
✅ **Periodic Callbacks** - Automated metric collection and reporting  

## API Reference

### BridgeHealthMetrics Structure

```cpp
struct BridgeHealthMetrics {
  // Connectivity
  uint32_t uptimeSeconds;           // Total uptime in seconds
  uint32_t internetUptimeSeconds;   // Time with Internet connection
  uint32_t totalDisconnects;        // Number of disconnection events
  uint32_t currentUptime;           // Current uptime in milliseconds
  
  // Signal Quality
  int8_t currentRSSI;               // Current WiFi signal strength (dBm)
  int8_t avgRSSI;                   // Average signal strength
  int8_t minRSSI;                   // Minimum observed signal strength
  int8_t maxRSSI;                   // Maximum observed signal strength
  
  // Traffic
  uint64_t bytesRx;                 // Total bytes received
  uint64_t bytesTx;                 // Total bytes transmitted
  uint32_t messagesRx;              // Total messages received
  uint32_t messagesTx;              // Total messages transmitted
  uint32_t messagesQueued;          // Messages currently queued
  uint32_t messagesDropped;         // Messages that failed to send
  
  // Performance
  uint32_t avgLatencyMs;            // Average message latency
  uint8_t packetLossPercent;        // Packet loss percentage (0-100)
  uint32_t meshNodeCount;           // Number of nodes in mesh
};
```

### Methods

#### getBridgeHealthMetrics()

Get current health metrics snapshot.

```cpp
BridgeHealthMetrics metrics = mesh.getBridgeHealthMetrics();

Serial.printf("Uptime: %u s\n", metrics.uptimeSeconds);
Serial.printf("Messages RX: %u\n", metrics.messagesRx);
Serial.printf("Avg Latency: %u ms\n", metrics.avgLatencyMs);
```

#### resetHealthMetrics()

Reset all counters to zero. Useful for periodic monitoring windows.

```cpp
mesh.resetHealthMetrics();
```

**Note:** This resets counters (messages, bytes, disconnects) but keeps current state metrics (RSSI, node count).

#### getHealthMetricsJSON()

Export metrics as JSON string for easy integration.

```cpp
String json = mesh.getHealthMetricsJSON();
mqttClient.publish("bridge/metrics", json.c_str());
```

**JSON Format:**
```json
{
  "connectivity": {
    "uptimeSeconds": 3600,
    "internetUptimeSeconds": 3540,
    "totalDisconnects": 2,
    "currentUptime": 3600000
  },
  "signalQuality": {
    "currentRSSI": -65,
    "avgRSSI": -68,
    "minRSSI": -75,
    "maxRSSI": -60
  },
  "traffic": {
    "bytesRx": 1048576,
    "bytesTx": 524288,
    "messagesRx": 1234,
    "messagesTx": 567,
    "messagesQueued": 0,
    "messagesDropped": 5
  },
  "performance": {
    "avgLatencyMs": 45,
    "packetLossPercent": 1,
    "meshNodeCount": 8
  }
}
```

#### onHealthMetricsUpdate()

Register a periodic callback for automated monitoring.

```cpp
mesh.onHealthMetricsUpdate([](BridgeHealthMetrics metrics) {
  // Process metrics
  Serial.printf("Nodes: %u, Latency: %u ms\n", 
                metrics.meshNodeCount, metrics.avgLatencyMs);
}, 60000);  // Every 60 seconds
```

## Integration Examples

### MQTT Publishing

```cpp
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void metricsCallback(BridgeHealthMetrics metrics) {
  if (mqttClient.connected()) {
    String json = mesh.getHealthMetricsJSON();
    mqttClient.publish("bridge/metrics", json.c_str());
  }
}

void setup() {
  // ... mesh setup ...
  mesh.onHealthMetricsUpdate(metricsCallback, 60000);
}
```

### Prometheus Exporter

```cpp
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

String exportPrometheus() {
  auto metrics = mesh.getBridgeHealthMetrics();
  
  String output = "";
  output += "# HELP bridge_uptime_seconds Bridge uptime\n";
  output += "# TYPE bridge_uptime_seconds counter\n";
  output += "bridge_uptime_seconds " + String(metrics.uptimeSeconds) + "\n";
  
  output += "# HELP bridge_rssi_dbm WiFi signal strength\n";
  output += "# TYPE bridge_rssi_dbm gauge\n";
  output += "bridge_rssi_dbm " + String(metrics.currentRSSI) + "\n";
  
  // Add more metrics...
  
  return output;
}

void setup() {
  // ... mesh setup ...
  
  server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", exportPrometheus());
  });
  
  server.begin();
}
```

### Grafana Dashboard

Use the Prometheus exporter above with Grafana to create monitoring dashboards:

1. Configure Prometheus to scrape your bridge node: `http://bridge-ip/metrics`
2. Import metrics into Grafana
3. Create panels for:
   - Uptime and availability
   - Signal strength trends
   - Traffic throughput
   - Latency and packet loss
   - Mesh topology changes

### Cloud Logging (AWS CloudWatch, Azure Monitor, etc.)

```cpp
void metricsCallback(BridgeHealthMetrics metrics) {
  String json = mesh.getHealthMetricsJSON();
  
  // Send to CloudWatch
  httpClient.post("/cloudwatch", json);
  
  // Send to Azure Monitor
  httpClient.post("/azure-monitor", json);
}
```

## Use Cases

### Production Monitoring

Monitor bridge health in production deployments:
- Track uptime and availability
- Detect connectivity issues early
- Analyze signal quality trends
- Capacity planning based on traffic patterns

### Troubleshooting

Debug network issues:
- Identify sources of packet loss
- Locate signal strength problems
- Diagnose latency spikes
- Track disconnection patterns

### Performance Optimization

Optimize mesh performance:
- Analyze traffic patterns
- Identify bottlenecks
- Optimize node placement based on RSSI
- Fine-tune routing strategies

## Best Practices

### Monitoring Intervals

- **Development:** 10-30 seconds for rapid feedback
- **Production:** 60-300 seconds to reduce overhead
- **Critical Systems:** 30-60 seconds with alerting

### Metric Storage

- Use time-series databases (InfluxDB, Prometheus)
- Retain raw metrics for 7-30 days
- Aggregate to hourly/daily for long-term storage
- Set up automated retention policies

### Alerting

Set up alerts for:
- Packet loss > 5%
- Latency > 200ms
- Signal strength < -80 dBm
- Frequent disconnections (> 3/hour)
- Node count drops

### Resource Management

On ESP8266 (limited RAM):
- Use longer intervals (120-300 seconds)
- Minimize callback complexity
- Consider offloading processing to gateway

On ESP32 (more RAM):
- Can use shorter intervals (30-60 seconds)
- More sophisticated processing possible
- Support multiple monitoring integrations

## Example: Complete Monitoring Setup

See `examples/bridge/bridge_health_monitoring_example.ino` for a complete working example with:
- Periodic metrics logging
- MQTT integration
- Prometheus export endpoint
- Manual metrics queries

## Version History

- **v1.8.0** - Initial release
  - Basic metrics collection
  - JSON export
  - Periodic callbacks
  - MQTT and Prometheus examples

## See Also

- [Bridge Architecture](BRIDGE_ARCHITECTURE_IMPLEMENTATION.md)
- [Bridge Status Feature](BRIDGE_STATUS_FEATURE.md)
- [MQTT Bridge Examples](../examples/bridge/)
