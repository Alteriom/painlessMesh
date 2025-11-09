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
  double temperature = 0.0;  // Temperature in Celsius
  double humidity = 0.0;     // Relative humidity percentage
  double pressure = 0.0;     // Atmospheric pressure in hPa
  uint32_t sensorId = 0;     // Unique sensor identifier
  uint32_t timestamp = 0;    // Unix timestamp of measurement
  uint8_t batteryLevel = 0;  // Battery level percentage

  SensorPackage()
      : BroadcastPackage(200) {}  // Type ID 200 for Alteriom sensors

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

  CommandPackage() : SinglePackage(400) {}  // Type ID 201 for Alteriom commands

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

  StatusPackage() : BroadcastPackage(202) {}  // Type ID 202 for Alteriom status

  StatusPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    deviceStatus = jsonObj["status"];
    uptime = jsonObj["uptime"];
    freeMemory = jsonObj["mem"];
    wifiStrength = jsonObj["wifi"];
    firmwareVersion = jsonObj["fw"].as<TSTRING>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["status"] = deviceStatus;
    jsonObj["uptime"] = uptime;
    jsonObj["mem"] = freeMemory;
    jsonObj["wifi"] = wifiStrength;
    jsonObj["fw"] = firmwareVersion;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 5) + firmwareVersion.length();
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
      : BroadcastPackage(203) {}  // Type ID 203 for enhanced status

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
 * @brief Bridge status package for monitoring Internet connectivity (Type 610 - BRIDGE_STATUS)
 *
 * This package is broadcast by bridge nodes to inform the mesh about their Internet connectivity status.
 * Regular nodes can use this information to decide whether to send data, queue messages, or failover to backup bridges.
 * 
 * Broadcast interval: Configurable, default 30 seconds
 * Timeout threshold: 60 seconds (nodes consider bridge offline if no heartbeat)
 * 
 * Type ID 610 for BRIDGE_STATUS per mqtt-schema v0.7.3+.
 */
class BridgeStatusPackage : public painlessmesh::plugin::BroadcastPackage {
public:
  // Bridge connectivity status
  bool internetConnected = false;  // Is the bridge connected to Internet?
  int8_t routerRSSI = 0;          // Router WiFi signal strength in dBm
  uint8_t routerChannel = 0;      // Router WiFi channel
  uint32_t uptime = 0;            // Bridge uptime in milliseconds
  TSTRING gatewayIP = "";         // Router gateway IP address
  uint32_t timestamp = 0;         // Timestamp of status check
  
  // MQTT Schema v0.7.3+ message_type
  uint16_t messageType = 610;  // BRIDGE_STATUS
  
  BridgeStatusPackage() : BroadcastPackage(610) {}
  
  BridgeStatusPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    internetConnected = jsonObj["internetConnected"] | false;
    routerRSSI = jsonObj["routerRSSI"] | 0;
    routerChannel = jsonObj["routerChannel"] | 0;
    uptime = jsonObj["uptime"] | 0;
    gatewayIP = jsonObj["gatewayIP"].as<TSTRING>();
    timestamp = jsonObj["timestamp"] | 0;
    messageType = jsonObj["message_type"] | 610;
  }
  
  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["internetConnected"] = internetConnected;
    jsonObj["routerRSSI"] = routerRSSI;
    jsonObj["routerChannel"] = routerChannel;
    jsonObj["uptime"] = uptime;
    jsonObj["gatewayIP"] = gatewayIP;
    jsonObj["timestamp"] = timestamp;
    jsonObj["message_type"] = messageType;
    return jsonObj;
  }
  
#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 7) + gatewayIP.length();
  }
#endif
};

}  // namespace alteriom

#endif  // ALTERIOM_SENSOR_PACKAGE_HPP