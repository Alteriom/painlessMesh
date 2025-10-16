#include "painlessMesh.h"
#include "alteriom_sensor_package.hpp"

// Define LED_BUILTIN for ESP32 boards that don't have it predefined
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

using namespace alteriom;

Scheduler userScheduler;
painlessMesh mesh;

// Command handlers
void handleCommand(const CommandPackage& cmd);
void sendStatusResponse(uint32_t commandId, String message, uint8_t status = 0);
void sendConfiguration();
void sendStatus();

// Command definitions (matching mqtt_command_bridge.hpp)
enum Commands {
  RESET = 1,
  SLEEP = 2,
  WAKE = 3,
  LED_CONTROL = 10,
  RELAY_SWITCH = 11,
  PWM_SET = 12,
  GET_CONFIG = 100,
  SET_CONFIG = 101,
  GET_STATUS = 200,
  GET_METRICS = 201
};

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  mesh.onReceive([](uint32_t from, String& msg) {
    Serial.printf("Received from %u\n", from);
    
    // Parse message
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) {
      Serial.printf("JSON parse error: %s\n", error.c_str());
      return;
    }
    
    uint8_t msgType = doc["type"];
    
    if (msgType == 201) {
      // CommandPackage
      JsonObject obj = doc.as<JsonObject>();
      CommandPackage cmd(obj);
      
      // Check if command is for this node
      if (cmd.targetDevice == 0 || cmd.targetDevice == mesh.getNodeId()) {
        handleCommand(cmd);
      }
    }
  });
  
  mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
  });
  
  mesh.onChangedConnections([]() {
    Serial.printf("Connections changed - nodes: %d\n", mesh.getNodeList().size());
  });
  
  Serial.printf("Mesh node initialized: %u\n", mesh.getNodeId());
  Serial.println("Ready to receive commands");
}

void loop() {
  mesh.update();
}

/**
 * @brief Handle incoming command
 */
void handleCommand(const CommandPackage& cmd) {
  Serial.printf("Command received: cmd=%d, id=%u\n", cmd.command, cmd.commandId);
  
  switch(cmd.command) {
    case RESET:
      sendStatusResponse(cmd.commandId, "Rebooting...");
      delay(1000);
      ESP.restart();
      break;
      
    case SLEEP: {
      JsonDocument params;
      deserializeJson(params, cmd.parameters);
      uint32_t duration = params["duration_ms"];
      
      sendStatusResponse(cmd.commandId, "Entering sleep for " + String(duration) + "ms");
      delay(100);
      
      ESP.deepSleep(duration * 1000);
      break;
    }
    
    case LED_CONTROL: {
      JsonDocument params;
      deserializeJson(params, cmd.parameters);
      
      bool state = params["state"];
      uint8_t brightness = params["brightness"] | 255;
      
      digitalWrite(LED_BUILTIN, state ? LOW : HIGH);
      
      String msg = "LED " + String(state ? "ON" : "OFF");
      if (brightness < 255) {
        msg += " (brightness: " + String(brightness) + ")";
      }
      
      sendStatusResponse(cmd.commandId, msg);
      Serial.println(msg);
      break;
    }
    
    case RELAY_SWITCH: {
      JsonDocument params;
      deserializeJson(params, cmd.parameters);
      
      uint8_t channel = params["channel"];
      bool state = params["state"];
      
      // Control relay (GPIO configuration needed)
      String msg = "Relay " + String(channel) + " " + String(state ? "ON" : "OFF");
      sendStatusResponse(cmd.commandId, msg);
      Serial.println(msg);
      break;
    }
    
    case PWM_SET: {
      JsonDocument params;
      deserializeJson(params, cmd.parameters);
      
      uint8_t pin = params["pin"];
      uint16_t duty = params["duty"];
      
      // Set PWM (example - adjust for your hardware)
      String msg = "PWM pin " + String(pin) + " set to " + String(duty);
      sendStatusResponse(cmd.commandId, msg);
      Serial.println(msg);
      break;
    }
    
    case GET_CONFIG:
      sendConfiguration();
      break;
      
    case GET_STATUS:
      sendStatus();
      break;
      
    case GET_METRICS: {
      // Send enhanced status
      EnhancedStatusPackage metrics;
      metrics.from = mesh.getNodeId();
      metrics.deviceStatus = 0;
      metrics.uptime = millis() / 1000;
      metrics.freeMemory = ESP.getFreeHeap() / 1024;
      metrics.wifiStrength = WiFi.RSSI();
      metrics.firmwareVersion = "1.0.0";
      
      auto nodeList = mesh.getNodeList();
      metrics.nodeCount = nodeList.size();
      metrics.connectionCount = nodeList.size();
      
      auto variant = painlessmesh::protocol::Variant(&metrics);
      String msg;
      variant.printTo(msg);
      
      mesh.sendBroadcast(msg);
      Serial.println("Metrics sent");
      break;
    }
    
    case SET_CONFIG: {
      JsonDocument params;
      deserializeJson(params, cmd.parameters);
      
      JsonObject config = params.as<JsonObject>();
      
      if (config["deviceName"].is<String>()) {
        String name = config["deviceName"];
        Serial.printf("Config: deviceName=%s\n", name.c_str());
      }
      
      if (config["sampleRate"].is<uint32_t>()) {
        uint32_t rate = config["sampleRate"];
        Serial.printf("Config: sampleRate=%u\n", rate);
      }
      
      sendStatusResponse(cmd.commandId, "Configuration updated");
      break;
    }
    
    default:
      sendStatusResponse(cmd.commandId, "Unknown command: " + String(cmd.command), 2);
      Serial.printf("Unknown command: %d\n", cmd.command);
  }
}

/**
 * @brief Send status response
 */
void sendStatusResponse(uint32_t commandId, String message, uint8_t status) {
  StatusPackage response;
  response.from = mesh.getNodeId();
  response.deviceStatus = status;
  response.uptime = millis() / 1000;
  response.freeMemory = ESP.getFreeHeap() / 1024;
  response.wifiStrength = WiFi.RSSI();
  response.firmwareVersion = "1.0.0";
  response.responseToCommand = commandId;
  response.responseMessage = message;
  
  auto variant = painlessmesh::protocol::Variant(&response);
  String msg;
  variant.printTo(msg);
  
  mesh.sendBroadcast(msg);
  Serial.printf("Response sent: %s\n", message.c_str());
}

/**
 * @brief Send configuration
 */
void sendConfiguration() {
  JsonDocument config;
  config["deviceName"] = "Sensor-" + String(mesh.getNodeId());
  config["firmwareVersion"] = "1.0.0";
  config["role"] = "sensor";
  config["sampleRate"] = 60000;
  config["ledEnabled"] = true;
  
  String configStr;
  serializeJson(config, configStr);
  
  sendStatusResponse(0, configStr);
  Serial.println("Configuration sent");
}

/**
 * @brief Send basic status
 */
void sendStatus() {
  sendStatusResponse(0, "OK");
  Serial.println("Status sent");
}
