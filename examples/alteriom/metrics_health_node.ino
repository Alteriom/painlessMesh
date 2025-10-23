/**
 * Alteriom Metrics and Health Monitoring Node
 * 
 * This example demonstrates the new MetricsPackage (Type 204) and 
 * HealthCheckPackage (Type 802) introduced in painlessMesh v1.7.7.
 * 
 * Features:
 * - Comprehensive performance metrics collection
 * - Proactive health monitoring with problem detection
 * - Configurable collection intervals
 * - Alert threshold detection
 * - Memory leak detection
 * - Network quality monitoring
 * 
 * Hardware: ESP32 or ESP8266
 */

#include "painlessMesh.h"
#include "alteriom_sensor_package.hpp"

using namespace alteriom;

// Mesh Configuration
#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_password"
#define MESH_PORT       5555

// Collection Intervals (milliseconds)
#define METRICS_INTERVAL   30000  // 30 seconds
#define HEALTH_INTERVAL    60000  // 60 seconds

Scheduler userScheduler;
painlessMesh mesh;

// Tasks
Task taskSendMetrics(METRICS_INTERVAL, TASK_FOREVER, &sendMetrics);
Task taskSendHealth(HEALTH_INTERVAL, TASK_FOREVER, &sendHealthCheck);

// Metrics tracking
uint32_t loopCount = 0;
uint32_t lastLoopCount = 0;
uint32_t lastMetricsTime = 0;
uint32_t maxLoopDuration = 0;
uint32_t loopStartTime = 0;

// Network statistics
uint32_t totalBytesRx = 0;
uint32_t totalBytesTx = 0;
uint32_t totalPacketsRx = 0;
uint32_t totalPacketsTx = 0;
uint32_t packetsDropped = 0;

// Health tracking
uint32_t minHeapEverSeen = 0xFFFFFFFF;
uint32_t previousHeap = 0;
uint32_t memoryCheckCount = 0;
int32_t memoryTrendAccumulator = 0;
uint16_t reconnectionCount = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Alteriom Metrics & Health Monitoring Node ===");
  Serial.println("painlessMesh v1.7.7 - Type 204 & 802 Packages");
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Set up callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onDroppedConnection(&droppedConnectionCallback);
  
  // Add tasks to scheduler
  userScheduler.addTask(taskSendMetrics);
  userScheduler.addTask(taskSendHealth);
  
  taskSendMetrics.enable();
  taskSendHealth.enable();
  
  // Initialize tracking
  lastMetricsTime = millis();
  minHeapEverSeen = ESP.getFreeHeap();
  previousHeap = minHeapEverSeen;
  
  Serial.printf("Node ID: %u\n", mesh.getNodeId());
  Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("Setup complete!");
}

void loop() {
  loopStartTime = micros();
  loopCount++;
  
  mesh.update();
  
  // Track maximum loop duration
  uint32_t loopDuration = micros() - loopStartTime;
  if (loopDuration > maxLoopDuration) {
    maxLoopDuration = loopDuration;
  }
}

void sendMetrics() {
  Serial.println("\n--- Sending Metrics Package ---");
  
  MetricsPackage metrics;
  metrics.from = mesh.getNodeId();
  
  // Calculate time since last metrics
  uint32_t now = millis();
  uint32_t timeDelta = now - lastMetricsTime;
  if (timeDelta == 0) timeDelta = 1;  // Prevent division by zero
  
  // CPU and Processing
  metrics.loopIterations = ((loopCount - lastLoopCount) * 1000) / timeDelta;
  metrics.cpuUsage = calculateCPUUsage();
  metrics.taskQueueSize = userScheduler.size();
  
  // Memory Metrics
  metrics.freeHeap = ESP.getFreeHeap();
  metrics.minFreeHeap = minHeapEverSeen;
#ifdef ESP32
  metrics.heapFragmentation = 0;  // ESP32 doesn't provide fragmentation
  metrics.maxAllocHeap = ESP.getMaxAllocHeap();
#else
  metrics.heapFragmentation = ESP.getHeapFragmentation();
  metrics.maxAllocHeap = ESP.getMaxFreeBlockSize();
#endif
  
  // Update minimum heap tracking
  if (metrics.freeHeap < minHeapEverSeen) {
    minHeapEverSeen = metrics.freeHeap;
  }
  
  // Network Performance
  metrics.bytesReceived = totalBytesRx;
  metrics.bytesSent = totalBytesTx;
  metrics.packetsReceived = totalPacketsRx;
  metrics.packetsSent = totalPacketsTx;
  metrics.packetsDropped = packetsDropped;
  metrics.currentThroughput = ((totalBytesRx + totalBytesTx) * 1000) / timeDelta;
  
  // Timing and Latency
  metrics.avgResponseTime = mesh.getNodeTime() % 1000000;  // Mock value
  metrics.maxResponseTime = maxLoopDuration;
  metrics.avgMeshLatency = 25;  // Would need to calculate from actual delays
  
  // Connection Quality
  metrics.connectionQuality = calculateConnectionQuality();
  metrics.wifiRSSI = WiFi.RSSI();
  
  // Metadata
  metrics.collectionTimestamp = mesh.getNodeTime();
  metrics.collectionInterval = METRICS_INTERVAL;
  
  // Send the metrics
  String msg = metrics.toJsonString();
  bool sent = mesh.sendBroadcast(msg);
  
  if (sent) {
    Serial.println("Metrics sent successfully");
    Serial.printf("  CPU: %d%%, Loops/sec: %u\n", metrics.cpuUsage, metrics.loopIterations);
    Serial.printf("  Free Heap: %u bytes, Min: %u bytes\n", metrics.freeHeap, metrics.minFreeHeap);
    Serial.printf("  Throughput: %u bytes/sec\n", metrics.currentThroughput);
    Serial.printf("  WiFi RSSI: %d dBm\n", metrics.wifiRSSI);
  } else {
    Serial.println("ERROR: Failed to send metrics");
    packetsDropped++;
  }
  
  // Update tracking
  lastLoopCount = loopCount;
  lastMetricsTime = now;
  maxLoopDuration = 0;
}

void sendHealthCheck() {
  Serial.println("\n--- Sending Health Check Package ---");
  
  HealthCheckPackage health;
  health.from = mesh.getNodeId();
  
  // Collect health metrics
  uint32_t freeHeap = ESP.getFreeHeap();
  uint8_t memHealth = calculateMemoryHealth(freeHeap);
  uint8_t netHealth = calculateNetworkHealth();
  uint8_t perfHealth = calculatePerformanceHealth();
  
  // Overall health status (0=critical, 1=warning, 2=healthy)
  if (memHealth < 30 || netHealth < 30 || perfHealth < 30) {
    health.healthStatus = 0;  // Critical
  } else if (memHealth < 60 || netHealth < 60 || perfHealth < 60) {
    health.healthStatus = 1;  // Warning
  } else {
    health.healthStatus = 2;  // Healthy
  }
  
  // Problem flags
  health.problemFlags = 0;
  if (memHealth < 60) health.problemFlags |= 0x0001;  // Low memory
  if (perfHealth < 60) health.problemFlags |= 0x0002;  // High CPU
  if (netHealth < 60) health.problemFlags |= 0x0004;  // Connection instability
  if (packetsDropped > 0) health.problemFlags |= 0x0008;  // High packet loss
  
  // Memory Health
  health.memoryHealth = memHealth;
  
  // Calculate memory trend (bytes per hour)
  if (memoryCheckCount > 0) {
    int32_t memoryDelta = (int32_t)previousHeap - (int32_t)freeHeap;
    memoryTrendAccumulator += memoryDelta;
    health.memoryTrend = (memoryTrendAccumulator * 3600000) / (HEALTH_INTERVAL * memoryCheckCount);
  }
  previousHeap = freeHeap;
  memoryCheckCount++;
  
  // Network Health
  health.networkHealth = netHealth;
  health.packetLossPercent = calculatePacketLoss();
  health.reconnectionCount = reconnectionCount;
  
  // Performance Health
  health.performanceHealth = perfHealth;
  health.missedDeadlines = 0;  // Would need task scheduler integration
  health.maxLoopTime = maxLoopDuration / 1000;  // Convert to ms
  
  // Environmental (if available)
#ifdef TEMP_SENSOR_PIN
  health.temperature = readTemperature();
  health.temperatureHealth = calculateTemperatureHealth(health.temperature);
#else
  health.temperature = 0;
  health.temperatureHealth = 100;
#endif
  
  // Uptime and Stability
  health.uptime = millis() / 1000;
  health.crashCount = 0;  // Would need EEPROM tracking
  health.lastRebootReason = 1;  // Normal startup
  
  // Predictive indicators
  if (health.memoryTrend > 0 && freeHeap > health.memoryTrend) {
    health.estimatedTimeToFailure = (freeHeap / health.memoryTrend);
  } else {
    health.estimatedTimeToFailure = 0;  // Unknown
  }
  
  // Recommendations
  if (health.healthStatus == 0) {
    health.recommendations = "CRITICAL: Immediate attention required";
  } else if (health.healthStatus == 1) {
    if (memHealth < 60) {
      health.recommendations = "Increase memory allocation or reduce message frequency";
    } else if (netHealth < 60) {
      health.recommendations = "Check network connections and reduce interference";
    } else {
      health.recommendations = "Monitor performance closely";
    }
  } else {
    health.recommendations = "System operating normally";
  }
  
  // Metadata
  health.checkTimestamp = mesh.getNodeTime();
  health.nextCheckDue = health.checkTimestamp + (HEALTH_INTERVAL * 1000);
  
  // Send the health check
  String msg = health.toJsonString();
  bool sent = mesh.sendBroadcast(msg);
  
  if (sent) {
    Serial.println("Health check sent successfully");
    Serial.printf("  Overall Health: %s (Status: %d)\n", 
                  health.healthStatus == 2 ? "HEALTHY" : 
                  health.healthStatus == 1 ? "WARNING" : "CRITICAL",
                  health.healthStatus);
    Serial.printf("  Memory Health: %d%%, Network Health: %d%%, Performance: %d%%\n",
                  health.memoryHealth, health.networkHealth, health.performanceHealth);
    if (health.problemFlags > 0) {
      Serial.printf("  Problem Flags: 0x%04X\n", health.problemFlags);
    }
    Serial.printf("  Memory Trend: %d bytes/hour\n", health.memoryTrend);
    if (health.estimatedTimeToFailure > 0) {
      Serial.printf("  Est. Time to Failure: %d hours\n", health.estimatedTimeToFailure);
    }
    Serial.printf("  Recommendations: %s\n", health.recommendations.c_str());
  } else {
    Serial.println("ERROR: Failed to send health check");
    packetsDropped++;
  }
}

// Utility functions for health calculations
uint8_t calculateCPUUsage() {
  // Simplified CPU usage estimation based on loop execution
  // In reality, this would need more sophisticated timing
  uint32_t loopsPerSec = (loopCount - lastLoopCount) * 1000 / (millis() - lastMetricsTime);
  if (loopsPerSec > 10000) return 20;  // Low usage
  if (loopsPerSec > 5000) return 40;
  if (loopsPerSec > 2000) return 60;
  if (loopsPerSec > 1000) return 80;
  return 95;  // High usage
}

uint8_t calculateConnectionQuality() {
  int8_t rssi = WiFi.RSSI();
  uint8_t lossPercent = calculatePacketLoss();
  
  // Quality based on RSSI (0-50 points)
  uint8_t rssiScore = 0;
  if (rssi > -50) rssiScore = 50;
  else if (rssi > -70) rssiScore = 40;
  else if (rssi > -80) rssiScore = 30;
  else if (rssi > -90) rssiScore = 20;
  else rssiScore = 10;
  
  // Quality based on packet loss (0-50 points)
  uint8_t lossScore = 50 - (lossPercent / 2);
  if (lossScore < 0) lossScore = 0;
  
  return rssiScore + lossScore;
}

uint8_t calculateMemoryHealth(uint32_t freeHeap) {
#ifdef ESP32
  uint32_t totalHeap = ESP.getHeapSize();
#else
  uint32_t totalHeap = 81920;  // ESP8266 ~80KB
#endif
  
  uint8_t heapPercent = (freeHeap * 100) / totalHeap;
  
  if (heapPercent > 60) return 100;
  if (heapPercent > 40) return 80;
  if (heapPercent > 20) return 50;
  if (heapPercent > 10) return 30;
  return 10;
}

uint8_t calculateNetworkHealth() {
  uint8_t connectionCount = mesh.getNodeList().size();
  uint8_t lossPercent = calculatePacketLoss();
  
  // Good if we have connections and low packet loss
  if (connectionCount > 0 && lossPercent < 5) return 100;
  if (connectionCount > 0 && lossPercent < 10) return 80;
  if (connectionCount > 0 && lossPercent < 20) return 60;
  if (connectionCount > 0) return 40;
  return 20;  // No connections
}

uint8_t calculatePerformanceHealth() {
  // Based on loop performance
  if (maxLoopDuration < 10000) return 100;    // < 10ms excellent
  if (maxLoopDuration < 50000) return 80;     // < 50ms good
  if (maxLoopDuration < 100000) return 60;    // < 100ms fair
  if (maxLoopDuration < 200000) return 40;    // < 200ms poor
  return 20;  // Very poor
}

uint8_t calculatePacketLoss() {
  uint32_t totalPackets = totalPacketsTx + totalPacketsRx;
  if (totalPackets == 0) return 0;
  return (packetsDropped * 100) / totalPackets;
}

// Mesh callbacks
void receivedCallback(uint32_t from, String& msg) {
  totalBytesRx += msg.length();
  totalPacketsRx++;
  
  // Parse message type
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, msg);
  JsonObject obj = doc.as<JsonObject>();
  uint8_t msgType = obj["type"];
  
  Serial.printf("\nReceived Type %d from %u\n", msgType, from);
  
  switch(msgType) {
    case 200:  // SensorPackage
      Serial.println("  -> Sensor data");
      break;
    case 201:  // CommandPackage
      Serial.println("  -> Command");
      break;
    case 202:  // StatusPackage
      Serial.println("  -> Status");
      break;
    case 800:  // EnhancedStatusPackage (vendor-specific)
      Serial.println("  -> Enhanced Status");
      break;
    case 204:  // MetricsPackage (SENSOR_METRICS per schema v0.7.2+)
      Serial.println("  -> Metrics");
      break;
    case 802:  // HealthCheckPackage (vendor-specific)
      Serial.println("  -> Health Check");
      break;
    default:
      Serial.println("  -> Unknown type");
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("\nNew Connection: %u\n", nodeId);
  Serial.printf("Total nodes: %d\n", mesh.getNodeList().size() + 1);
}

void changedConnectionCallback() {
  Serial.println("\nMesh topology changed");
}

void droppedConnectionCallback(uint32_t nodeId) {
  Serial.printf("\nConnection dropped: %u\n", nodeId);
  reconnectionCount++;
}
