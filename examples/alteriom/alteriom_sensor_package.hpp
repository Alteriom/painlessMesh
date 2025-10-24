#ifndef ALTERIOM_SENSOR_PACKAGE_HPP
#define ALTERIOM_SENSOR_PACKAGE_HPP

#include "painlessmesh/plugin.hpp"

namespace alteriom {

/**
 * @brief Sensor data package for broadcasting environmental measurements
 *
 * This package is designed for Alteriom's IoT sensor network requirements,
 * allowing nodes to share environmental data across the mesh.
 */
class SensorPackage : public painlessmesh::plugin::BroadcastPackage {
 public:
  // Temperature in Celsius
  double temperature = 0.0;
  // Relative humidity percentage
  double humidity = 0.0;
  // Atmospheric pressure in hPa
  double pressure = 0.0;
  // Unique sensor identifier
  uint32_t sensorId = 0;
  // Unix timestamp of measurement
  uint32_t timestamp = 0;
  // Battery level percentage
  uint8_t batteryLevel = 0;

  // Type ID 200 for Alteriom sensors
  SensorPackage() : BroadcastPackage(200) {}

  SensorPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    temperature = jsonObj["temp"];
    humidity = jsonObj["hum"];
    pressure = jsonObj["press"];
    sensorId = jsonObj["sid"];
    timestamp = jsonObj["ts"];
    batteryLevel = jsonObj["bat"];
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["temp"] = temperature;
    jsonObj["hum"] = humidity;
    jsonObj["press"] = pressure;
    jsonObj["sid"] = sensorId;
    jsonObj["ts"] = timestamp;
    jsonObj["bat"] = batteryLevel;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const { return JSON_OBJECT_SIZE(noJsonFields + 6); }
#endif
};

/**
 * @brief Command package for controlling Alteriom devices
 *
 * Single-destination package for sending specific commands to individual nodes.
 */
class CommandPackage : public painlessmesh::plugin::SinglePackage {
 public:
  uint8_t command = 0;        // Command type
  uint32_t targetDevice = 0;  // Target device ID
  TSTRING parameters = "";    // Command parameters as JSON string
  uint32_t commandId = 0;     // Unique command identifier for tracking

  CommandPackage() : SinglePackage(201) {}  // Type ID 201 (NOTE: conflicts with schema SENSOR_HEARTBEAT, kept for backward compatibility)

  CommandPackage(JsonObject jsonObj) : SinglePackage(jsonObj) {
    command = jsonObj["cmd"];
    targetDevice = jsonObj["target"];
    parameters = jsonObj["params"].as<TSTRING>();
    commandId = jsonObj["cid"];
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = SinglePackage::addTo(std::move(jsonObj));
    jsonObj["cmd"] = command;
    jsonObj["target"] = targetDevice;
    jsonObj["params"] = parameters;
    jsonObj["cid"] = commandId;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 4) + parameters.length();
  }
#endif
};

/**
 * @brief Status report package for device health monitoring
 */
class StatusPackage : public painlessmesh::plugin::BroadcastPackage {
 public:
  uint8_t deviceStatus = 0;      // Device status flags
  uint32_t uptime = 0;           // Device uptime in seconds
  uint16_t freeMemory = 0;       // Free memory in KB
  uint8_t wifiStrength = 0;      // WiFi signal strength
  TSTRING firmwareVersion = "";  // Current firmware version

  // Command response fields (for MQTT bridge)
  uint32_t responseToCommand = 0;  // CommandId this is responding to
  TSTRING responseMessage = "";    // Success/error message

  StatusPackage() : BroadcastPackage(202) {}  // Type ID 202 for Alteriom status

  StatusPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    deviceStatus = jsonObj["status"];
    uptime = jsonObj["uptime"];
    freeMemory = jsonObj["mem"];
    wifiStrength = jsonObj["wifi"];
    firmwareVersion = jsonObj["fw"].as<TSTRING>();
    responseToCommand = jsonObj["respTo"] | 0;
    responseMessage = jsonObj["respMsg"].as<TSTRING>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["status"] = deviceStatus;
    jsonObj["uptime"] = uptime;
    jsonObj["mem"] = freeMemory;
    jsonObj["wifi"] = wifiStrength;
    jsonObj["fw"] = firmwareVersion;
    if (responseToCommand > 0) {
      jsonObj["respTo"] = responseToCommand;
      jsonObj["respMsg"] = responseMessage;
    }
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 7) + firmwareVersion.length() +
           responseMessage.length();
  }
#endif
};

/**
 * @brief Enhanced status package with comprehensive health metrics (Phase 1)
 *
 * This is an extended version of StatusPackage that includes additional
 * mesh statistics, performance metrics, and alerting capabilities.
 * Type ID 203 is used to distinguish from the basic StatusPackage (202).
 */
class EnhancedStatusPackage : public painlessmesh::plugin::BroadcastPackage {
 public:
  // Device Health (from original StatusPackage)
  uint8_t deviceStatus = 0;      // Device status flags
  uint32_t uptime = 0;           // Device uptime in seconds
  uint16_t freeMemory = 0;       // Free memory in KB
  uint8_t wifiStrength = 0;      // WiFi signal strength
  TSTRING firmwareVersion = "";  // Current firmware version
  TSTRING firmwareMD5 = "";      // Firmware hash for OTA verification

  // Mesh Statistics
  uint16_t nodeCount = 0;         // Number of known nodes in mesh
  uint8_t connectionCount = 0;    // Number of direct connections
  uint32_t messagesReceived = 0;  // Total messages received
  uint32_t messagesSent = 0;      // Total messages sent
  uint32_t messagesDropped = 0;   // Total messages dropped/failed

  // Performance Metrics
  uint16_t avgLatency = 0;     // Average message latency in ms
  uint8_t packetLossRate = 0;  // Packet loss rate percentage (0-100)
  uint16_t throughput = 0;     // Network throughput in bytes/sec

  // Warnings/Alerts
  uint8_t alertFlags = 0;  // Bit flags for various alert conditions
  TSTRING lastError = "";  // Last error message for diagnostics

  EnhancedStatusPackage()
      : BroadcastPackage(604) {}  // Type ID 604 for enhanced status (MESH_STATUS per mqtt-schema v0.7.2+)

  EnhancedStatusPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    deviceStatus = jsonObj["status"];
    uptime = jsonObj["uptime"];
    freeMemory = jsonObj["mem"];
    wifiStrength = jsonObj["wifi"];
    firmwareVersion = jsonObj["fw"].as<TSTRING>();
    firmwareMD5 = jsonObj["fwMD5"].as<TSTRING>();

    nodeCount = jsonObj["nodes"];
    connectionCount = jsonObj["conns"];
    messagesReceived = jsonObj["msgRx"];
    messagesSent = jsonObj["msgTx"];
    messagesDropped = jsonObj["msgDrop"];

    avgLatency = jsonObj["latency"];
    packetLossRate = jsonObj["loss"];
    throughput = jsonObj["throughput"];

    alertFlags = jsonObj["alerts"];
    lastError = jsonObj["lastErr"].as<TSTRING>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));

    // Device Health
    jsonObj["status"] = deviceStatus;
    jsonObj["uptime"] = uptime;
    jsonObj["mem"] = freeMemory;
    jsonObj["wifi"] = wifiStrength;
    jsonObj["fw"] = firmwareVersion;
    jsonObj["fwMD5"] = firmwareMD5;

    // Mesh Statistics
    jsonObj["nodes"] = nodeCount;
    jsonObj["conns"] = connectionCount;
    jsonObj["msgRx"] = messagesReceived;
    jsonObj["msgTx"] = messagesSent;
    jsonObj["msgDrop"] = messagesDropped;

    // Performance Metrics
    jsonObj["latency"] = avgLatency;
    jsonObj["loss"] = packetLossRate;
    jsonObj["throughput"] = throughput;

    // Alerts
    jsonObj["alerts"] = alertFlags;
    jsonObj["lastErr"] = lastError;

    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 18) + firmwareVersion.length() +
           firmwareMD5.length() + lastError.length();
  }
#endif
};

/**
 * @brief Detailed performance metrics package for monitoring (Phase 2)
 *
 * Provides comprehensive performance data including CPU usage, memory trends,
 * network throughput, and other metrics useful for dashboards and monitoring.
 * Type ID 204 for Alteriom metrics.
 */
class MetricsPackage : public painlessmesh::plugin::BroadcastPackage {
 public:
  // CPU and Processing
  uint8_t cpuUsage = 0;           // CPU usage percentage (0-100)
  uint32_t loopIterations = 0;    // Loop iterations per second
  uint16_t taskQueueSize = 0;     // Number of pending tasks
  
  // Memory Metrics
  uint32_t freeHeap = 0;          // Free heap memory in bytes
  uint32_t minFreeHeap = 0;       // Minimum free heap since boot
  uint32_t heapFragmentation = 0; // Heap fragmentation percentage
  uint32_t maxAllocHeap = 0;      // Largest allocatable block
  
  // Network Performance
  uint32_t bytesReceived = 0;     // Total bytes received
  uint32_t bytesSent = 0;         // Total bytes sent
  uint16_t packetsReceived = 0;   // Total packets received
  uint16_t packetsSent = 0;       // Total packets sent
  uint16_t packetsDropped = 0;    // Packets dropped
  uint16_t currentThroughput = 0; // Current throughput in bytes/sec
  
  // Timing and Latency
  uint32_t avgResponseTime = 0;   // Average response time in microseconds
  uint32_t maxResponseTime = 0;   // Maximum response time in microseconds
  uint16_t avgMeshLatency = 0;    // Average mesh latency in milliseconds
  
  // Connection Quality
  uint8_t connectionQuality = 0;  // Overall connection quality (0-100)
  int8_t wifiRSSI = 0;           // WiFi RSSI in dBm
  
  // Collection metadata
  uint32_t collectionTimestamp = 0; // When metrics were collected
  uint32_t collectionInterval = 0;  // Interval between collections in ms
  
  // MQTT Schema v0.7.2+ message_type for faster classification
  uint16_t messageType = 204;       // SENSOR_METRICS (aligns with schema v0.7.2+)

  MetricsPackage() : BroadcastPackage(204) {}

  MetricsPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    cpuUsage = jsonObj["cpu"];
    loopIterations = jsonObj["loops"];
    taskQueueSize = jsonObj["tasks"];
    
    freeHeap = jsonObj["heap"];
    minFreeHeap = jsonObj["minHeap"];
    heapFragmentation = jsonObj["fragHeap"];
    maxAllocHeap = jsonObj["maxHeap"];
    
    bytesReceived = jsonObj["bytesRx"];
    bytesSent = jsonObj["bytesTx"];
    packetsReceived = jsonObj["pktsRx"];
    packetsSent = jsonObj["pktsTx"];
    packetsDropped = jsonObj["pktsDrop"];
    currentThroughput = jsonObj["throughput"];
    
    avgResponseTime = jsonObj["avgResp"];
    maxResponseTime = jsonObj["maxResp"];
    avgMeshLatency = jsonObj["avgLat"];
    
    connectionQuality = jsonObj["connQual"];
    wifiRSSI = jsonObj["rssi"];
    
    collectionTimestamp = jsonObj["ts"];
    collectionInterval = jsonObj["interval"];
    messageType = jsonObj["message_type"] | 204;
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    
    // CPU and Processing
    jsonObj["cpu"] = cpuUsage;
    jsonObj["loops"] = loopIterations;
    jsonObj["tasks"] = taskQueueSize;
    
    // Memory Metrics
    jsonObj["heap"] = freeHeap;
    jsonObj["minHeap"] = minFreeHeap;
    jsonObj["fragHeap"] = heapFragmentation;
    jsonObj["maxHeap"] = maxAllocHeap;
    
    // Network Performance
    jsonObj["bytesRx"] = bytesReceived;
    jsonObj["bytesTx"] = bytesSent;
    jsonObj["pktsRx"] = packetsReceived;
    jsonObj["pktsTx"] = packetsSent;
    jsonObj["pktsDrop"] = packetsDropped;
    jsonObj["throughput"] = currentThroughput;
    
    // Timing and Latency
    jsonObj["avgResp"] = avgResponseTime;
    jsonObj["maxResp"] = maxResponseTime;
    jsonObj["avgLat"] = avgMeshLatency;
    
    // Connection Quality
    jsonObj["connQual"] = connectionQuality;
    jsonObj["rssi"] = wifiRSSI;
    
    // Metadata
    jsonObj["ts"] = collectionTimestamp;
    jsonObj["interval"] = collectionInterval;
    jsonObj["message_type"] = messageType;  // MQTT Schema v0.7.1+
    
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 22);
  }
#endif
};

/**
 * @brief Health check package for proactive problem detection (Phase 2)
 *
 * Provides early warning indicators and health status to detect issues
 * before they cause failures. Used for predictive maintenance and alerting.
 * Type ID 605 for Alteriom health checks (MESH_METRICS per mqtt-schema v0.7.2+).
 */
class HealthCheckPackage : public painlessmesh::plugin::BroadcastPackage {
 public:
  // Overall Health Status (0=critical, 1=warning, 2=healthy)
  uint8_t healthStatus = 2;
  
  // Problem Indicators (bit flags)
  uint16_t problemFlags = 0;  // Bit flags for specific problems
  /*
   * Problem flag bits:
   * 0x0001 - Low memory warning
   * 0x0002 - High CPU usage
   * 0x0004 - Connection instability
   * 0x0008 - High packet loss
   * 0x0010 - Network congestion
   * 0x0020 - Low battery (if applicable)
   * 0x0040 - Thermal warning
   * 0x0080 - Mesh partition detected
   * 0x0100 - OTA in progress
   * 0x0200 - Configuration error
   */
  
  // Memory Health
  uint8_t memoryHealth = 100;      // Memory health score (0-100)
  uint32_t memoryTrend = 0;        // Bytes/hour memory loss (for leak detection)
  
  // Network Health
  uint8_t networkHealth = 100;     // Network health score (0-100)
  uint8_t packetLossPercent = 0;   // Packet loss percentage
  uint8_t reconnectionCount = 0;   // Reconnections in last hour
  
  // Performance Health
  uint8_t performanceHealth = 100; // Performance health score (0-100)
  uint32_t missedDeadlines = 0;    // Missed task deadlines
  uint16_t maxLoopTime = 0;        // Maximum loop execution time in ms
  
  // Environmental (if sensors available)
  int8_t temperature = 0;          // Device temperature in Celsius
  uint8_t temperatureHealth = 100; // Temperature health score
  
  // Uptime and Stability
  uint32_t uptime = 0;             // Uptime in seconds
  uint16_t crashCount = 0;         // Crash/restart count
  uint32_t lastRebootReason = 0;   // Last reboot reason code
  
  // Predictive indicators
  uint16_t estimatedTimeToFailure = 0; // Estimated hours until failure (0=unknown)
  TSTRING recommendations = "";    // Recommended actions
  
  // Check metadata
  uint32_t checkTimestamp = 0;     // When check was performed
  uint32_t nextCheckDue = 0;       // When next check is due
  
  // MQTT Schema v0.7.2+ message_type for faster classification
  uint16_t messageType = 605;      // MESH_METRICS type code (mesh performance health per mqtt-schema v0.7.2+)

  HealthCheckPackage() : BroadcastPackage(605) {}

  HealthCheckPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    healthStatus = jsonObj["health"];
    problemFlags = jsonObj["problems"];
    
    memoryHealth = jsonObj["memHealth"];
    memoryTrend = jsonObj["memTrend"];
    
    networkHealth = jsonObj["netHealth"];
    packetLossPercent = jsonObj["loss"];
    reconnectionCount = jsonObj["reconn"];
    
    performanceHealth = jsonObj["perfHealth"];
    missedDeadlines = jsonObj["missed"];
    maxLoopTime = jsonObj["maxLoop"];
    
    temperature = jsonObj["temp"];
    temperatureHealth = jsonObj["tempHealth"];
    
    uptime = jsonObj["uptime"];
    crashCount = jsonObj["crashes"];
    lastRebootReason = jsonObj["reboot"];
    
    estimatedTimeToFailure = jsonObj["ettf"];
    recommendations = jsonObj["recommend"].as<TSTRING>();
    
    checkTimestamp = jsonObj["ts"];
    nextCheckDue = jsonObj["nextCheck"];
    messageType = jsonObj["message_type"] | 605;
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    
    jsonObj["health"] = healthStatus;
    jsonObj["problems"] = problemFlags;
    
    jsonObj["memHealth"] = memoryHealth;
    jsonObj["memTrend"] = memoryTrend;
    
    jsonObj["netHealth"] = networkHealth;
    jsonObj["loss"] = packetLossPercent;
    jsonObj["reconn"] = reconnectionCount;
    
    jsonObj["perfHealth"] = performanceHealth;
    jsonObj["missed"] = missedDeadlines;
    jsonObj["maxLoop"] = maxLoopTime;
    
    jsonObj["temp"] = temperature;
    jsonObj["tempHealth"] = temperatureHealth;
    
    jsonObj["uptime"] = uptime;
    jsonObj["crashes"] = crashCount;
    jsonObj["reboot"] = lastRebootReason;
    
    jsonObj["ettf"] = estimatedTimeToFailure;
    jsonObj["recommend"] = recommendations;
    
    jsonObj["ts"] = checkTimestamp;
    jsonObj["nextCheck"] = nextCheckDue;
    jsonObj["message_type"] = messageType;  // MQTT Schema v0.7.1+
    
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 20) + recommendations.length();
  }
#endif
};

}  // namespace alteriom

#endif  // ALTERIOM_SENSOR_PACKAGE_HPP