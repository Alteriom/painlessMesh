#ifndef MQTT_COMMAND_BRIDGE_HPP
#define MQTT_COMMAND_BRIDGE_HPP

#include "painlessMesh.h"
#include <PubSubClient.h>
#include "alteriom_sensor_package.hpp"
#include "mesh_topology_reporter.hpp"

using namespace alteriom;

/**
 * @brief Bidirectional MQTT-Mesh Command Bridge
 * 
 * Enables MQTT control of mesh devices and mesh status reporting to MQTT.
 * Supports command forwarding, configuration management, and response routing.
 */
class MqttCommandBridge {
private:
  painlessMesh& mesh;
  PubSubClient& mqtt;
  String gatewayId;
  MeshTopologyReporter* topologyReporter;
  
  // Command definitions matching MQTT_BRIDGE_COMMANDS.md
  enum Commands {
    // Device Control (1-99)
    RESET = 1,
    SLEEP = 2,
    WAKE = 3,
    LED_CONTROL = 10,
    RELAY_SWITCH = 11,
    PWM_SET = 12,
    SENSOR_ENABLE = 20,
    SENSOR_CALIBRATE = 21,
    
    // Configuration (100-199)
    GET_CONFIG = 100,
    SET_CONFIG = 101,
    RESET_CONFIG = 102,
    SAVE_CONFIG = 103,
    SET_SAMPLE_RATE = 110,
    SET_DEVICE_NAME = 111,
    
    // Status (200-255)
    GET_STATUS = 200,
    GET_METRICS = 201,
    GET_DIAGNOSTICS = 202,
    START_MONITORING = 210,
    STOP_MONITORING = 211,
    
    // Topology (300-399)
    GET_TOPOLOGY = 300
  };
  
public:
  MqttCommandBridge(painlessMesh& m, PubSubClient& mq, MeshTopologyReporter* reporter = nullptr) 
    : mesh(m), mqtt(mq), topologyReporter(reporter) {
    gatewayId = String(mesh.getNodeId());
  }
  
  /**
   * @brief Set topology reporter instance
   */
  void setTopologyReporter(MeshTopologyReporter* reporter) {
    topologyReporter = reporter;
  }
  
  /**
   * @brief Initialize the bridge with topic subscriptions and callbacks
   */
  void begin() {
    // Subscribe to command topics
    String cmdTopic = "mesh/command/" + gatewayId;
    mqtt.subscribe(cmdTopic.c_str());
    mqtt.subscribe("mesh/command/broadcast");
    
    String configGetTopic = "mesh/config/" + gatewayId + "/get";
    mqtt.subscribe(configGetTopic.c_str());
    
    String configSetTopic = "mesh/config/" + gatewayId + "/set";
    mqtt.subscribe(configSetTopic.c_str());
    
    Serial.printf("MQTT Command Bridge initialized - Gateway ID: %s\n", gatewayId.c_str());
    Serial.println("Subscribed topics:");
    Serial.printf("  - %s\n", cmdTopic.c_str());
    Serial.println("  - mesh/command/broadcast");
    Serial.printf("  - %s\n", configGetTopic.c_str());
    Serial.printf("  - %s\n", configSetTopic.c_str());
  }
  
  /**
   * @brief Set MQTT callback for incoming messages
   */
  void setMqttCallback() {
    mqtt.setCallback([this](char* topic, byte* payload, unsigned int length) {
      this->onMqttMessage(topic, payload, length);
    });
  }
  
  /**
   * @brief Set mesh received callback for outgoing messages
   */
  void setMeshCallback() {
    mesh.onReceive([this](uint32_t from, String& msg) {
      this->onMeshMessage(from, msg);
    });
  }
  
  /**
   * @brief Handle incoming MQTT messages
   */
  void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    
    // Parse JSON payload
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
      Serial.printf("JSON parse error: %s\n", error.c_str());
      return;
    }
    
    Serial.printf("MQTT message on %s\n", topic);
    
    if (topicStr.indexOf("/command/") >= 0) {
      forwardCommandToMesh(doc);
    }
    else if (topicStr.endsWith("/get")) {
      handleConfigRequest(doc);
    }
    else if (topicStr.endsWith("/set")) {
      handleConfigUpdate(doc);
    }
  }
  
  /**
   * @brief Forward MQTT command to mesh network
   */
  void forwardCommandToMesh(JsonDocument& doc) {
    // Create CommandPackage
    CommandPackage cmd;
    cmd.command = doc["command"];
    cmd.targetDevice = doc["targetDevice"];
    cmd.commandId = doc["commandId"] | millis(); // Generate if missing
    cmd.parameters = doc["parameters"].as<String>();
    cmd.from = mesh.getNodeId();
    cmd.dest = cmd.targetDevice;
    
    Serial.printf("Command: cmd=%d, target=%u, id=%u\n", 
                 cmd.command, cmd.targetDevice, cmd.commandId);
    
    // Route based on target
    if (cmd.targetDevice == 0 || cmd.targetDevice == mesh.getNodeId()) {
      // Execute locally on gateway
      executeCommand(cmd);
    } else {
      // Forward to mesh node
      auto variant = painlessmesh::protocol::Variant(&cmd);
      String message;
      variant.printTo(message);
      
      mesh.sendSingle(cmd.targetDevice, message);
      Serial.printf("Command forwarded to node %u\n", cmd.targetDevice);
    }
  }
  
  /**
   * @brief Execute command locally on gateway
   */
  void executeCommand(const CommandPackage& cmd) {
    Serial.printf("Executing command: %d\n", cmd.command);
    
    StatusPackage response;
    response.from = mesh.getNodeId();
    response.responseToCommand = cmd.commandId;
    response.deviceStatus = 0; // OK
    
    switch(cmd.command) {
      case RESET:
        response.responseMessage = "Rebooting...";
        sendResponse(response);
        delay(1000);
        ESP.restart();
        break;
        
      case SLEEP: {
        JsonDocument params;
        deserializeJson(params, cmd.parameters);
        uint32_t duration = params["duration_ms"];
        
        response.responseMessage = "Entering sleep for " + String(duration) + "ms";
        sendResponse(response);
        
        ESP.deepSleep(duration * 1000);
        break;
      }
      
      case LED_CONTROL: {
        JsonDocument params;
        deserializeJson(params, cmd.parameters);
        
        bool state = params["state"];
        uint8_t brightness = params["brightness"] | 255;
        
        // Control LED (use GPIO2 if LED_BUILTIN not defined on ESP32)
        #ifndef LED_BUILTIN
        #define LED_BUILTIN 2
        #endif
        digitalWrite(LED_BUILTIN, state ? LOW : HIGH);
        
        response.responseMessage = "LED " + String(state ? "ON" : "OFF");
        sendResponse(response);
        break;
      }
      
      case GET_CONFIG:
        sendConfiguration();
        break;
        
      case GET_STATUS:
        sendStatus();
        break;
        
      case GET_METRICS:
        sendMetrics();
        break;
        
      case GET_TOPOLOGY:
        sendTopology(cmd.commandId);
        break;
        
      default:
        response.deviceStatus = 2; // Error
        response.responseMessage = "Unknown command: " + String(cmd.command);
        sendResponse(response);
    }
  }
  
  /**
   * @brief Handle incoming mesh messages and forward to MQTT
   */
  void onMeshMessage(uint32_t from, String& msg) {
    // Parse message
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) return;
    
    uint8_t msgType = doc["type"];
    
    // Forward mesh messages to MQTT based on type
    switch(msgType) {
      case 200: // SensorPackage
        publishToMqtt("mesh/sensor/" + String(from), msg);
        break;
        
      case 202: // StatusPackage
        // Check if it's a command response (ArduinoJson v7 compatible)
        if (doc["respTo"].is<uint32_t>() && doc["respTo"].as<uint32_t>() > 0) {
          publishToMqtt("mesh/response/" + String(from), msg);
        } else {
          publishToMqtt("mesh/status/" + String(from), msg);
        }
        break;
        
      case 203: // EnhancedStatusPackage
        publishToMqtt("mesh/health/" + String(from), msg);
        break;
    }
  }
  
  /**
   * @brief Send status response via MQTT
   */
  void sendResponse(const StatusPackage& response) {
    auto variant = painlessmesh::protocol::Variant(&response);
    String message;
    variant.printTo(message);
    
    String topic = "mesh/response/" + String(response.from);
    publishToMqtt(topic, message);
  }
  
  /**
   * @brief Send configuration via MQTT
   */
  void sendConfiguration() {
    JsonDocument config;
    config["schema_version"] = 1;
    config["device_id"] = gatewayId;
    config["timestamp"] = getCurrentISO8601Timestamp();
    
    JsonObject cfg = config["config"].to<JsonObject>();
    cfg["deviceName"] = "Gateway-" + gatewayId;
    cfg["firmwareVersion"] = "1.7.0-alteriom";
    cfg["role"] = "gateway";
    cfg["meshPrefix"] = "AlteriomMesh";
    cfg["nodeCount"] = mesh.getNodeList().size();
    
    String payload;
    serializeJson(config, payload);
    
    String topic = "mesh/config/" + gatewayId;
    publishToMqtt(topic, payload);
  }
  
  /**
   * @brief Send basic status via MQTT
   */
  void sendStatus() {
    StatusPackage status;
    status.from = mesh.getNodeId();
    status.deviceStatus = 0;
    status.uptime = millis() / 1000;
    status.freeMemory = ESP.getFreeHeap() / 1024;
    status.wifiStrength = WiFi.RSSI();
    status.firmwareVersion = "1.7.0-alteriom";
    
    sendResponse(status);
  }
  
  /**
   * @brief Send enhanced metrics via MQTT
   */
  void sendMetrics() {
    EnhancedStatusPackage metrics;
    metrics.from = mesh.getNodeId();
    metrics.deviceStatus = 0;
    metrics.uptime = millis() / 1000;
    metrics.freeMemory = ESP.getFreeHeap() / 1024;
    metrics.wifiStrength = WiFi.RSSI();
    metrics.firmwareVersion = "1.7.0-alteriom";
    
    // Mesh statistics
    auto nodeList = mesh.getNodeList();
    metrics.nodeCount = nodeList.size();
    metrics.connectionCount = mesh.getNodeList().size();
    
    auto variant = painlessmesh::protocol::Variant(&metrics);
    String message;
    variant.printTo(message);
    
    String topic = "mesh/health/" + String(metrics.from);
    publishToMqtt(topic, message);
  }
  
  /**
   * @brief Send mesh topology via MQTT (Command 300)
   * @param commandId The command ID for correlation
   */
  void sendTopology(uint32_t commandId) {
    if (!topologyReporter) {
      Serial.println("⚠️  Topology reporter not initialized");
      
      StatusPackage response;
      response.from = mesh.getNodeId();
      response.responseToCommand = commandId;
      response.deviceStatus = 2; // Error
      response.responseMessage = "Topology reporter not available";
      sendResponse(response);
      return;
    }
    
    Serial.printf("📊 Generating topology for command %u\n", commandId);
    String topology = topologyReporter->generateFullTopology();
    
    // Parse to add correlation_id
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, topology);
    
    if (error) {
      Serial.printf("❌ Failed to parse topology: %s\n", error.c_str());
      return;
    }
    
    // Add correlation_id at root level
    doc["correlation_id"] = String(commandId);
    
    String payload;
    serializeJson(doc, payload);
    
    // Publish to response topic
    String topic = "alteriom/mesh/MESH-001/topology/response";
    publishToMqtt(topic, payload);
    
    Serial.printf("✅ Topology sent (cmd=%u, size=%d bytes)\n", commandId, payload.length());
  }
  
  /**
   * @brief Handle configuration request
   */
  void handleConfigRequest(JsonDocument& doc) {
    sendConfiguration();
  }
  
  /**
   * @brief Handle configuration update
   */
  void handleConfigUpdate(JsonDocument& doc) {
    JsonObject config = doc["config"];
    
    // Apply configuration updates (ArduinoJson v7 compatible)
    if (config["deviceName"].is<const char*>()) {
      String newName = config["deviceName"];
      Serial.printf("Config update: deviceName=%s\n", newName.c_str());
      // Store to preferences/EEPROM in production
    }
    
    if (config["meshPrefix"].is<const char*>()) {
      String newPrefix = config["meshPrefix"];
      Serial.printf("Config update: meshPrefix=%s (requires restart)\n", newPrefix.c_str());
    }
    
    // Send acknowledgment
    JsonDocument ack;
    ack["status"] = "success";
    ack["device_id"] = gatewayId;
    ack["message"] = "Configuration updated";
    ack["timestamp"] = getCurrentISO8601Timestamp();
    
    String payload;
    serializeJson(ack, payload);
    
    String topic = "mesh/response/" + gatewayId;
    publishToMqtt(topic, payload);
  }
  
  /**
   * @brief Publish message to MQTT broker
   */
  void publishToMqtt(const String& topic, const String& payload) {
    if (mqtt.connected()) {
      bool success = mqtt.publish(topic.c_str(), payload.c_str());
      if (success) {
        Serial.printf("MQTT published: %s\n", topic.c_str());
      } else {
        Serial.printf("MQTT publish failed: %s\n", topic.c_str());
      }
    } else {
      Serial.println("MQTT not connected, cannot publish");
    }
  }
  
private:
  /**
   * @brief Get current ISO8601 timestamp
   * Note: Simple implementation, enhance with NTP for production
   */
  String getCurrentISO8601Timestamp() {
    unsigned long seconds = millis() / 1000;
    char timestamp[25];
    snprintf(timestamp, sizeof(timestamp), 
             "1970-01-01T%02lu:%02lu:%02luZ",
             (seconds / 3600) % 24,
             (seconds / 60) % 60,
             seconds % 60);
    return String(timestamp);
  }
};

#endif // MQTT_COMMAND_BRIDGE_HPP
