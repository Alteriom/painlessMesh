//************************************************************
// Alteriom Sensor Node Example
//
// This example demonstrates how to create an Alteriom-specific
// sensor node using custom packages with painlessMesh
//************************************************************

#include <TaskScheduler.h>  // Required for LDF to find TaskScheduler dependency
#include "alteriom_sensor_package.hpp"
#include "painlessMesh.h"

#define MESH_PREFIX "AlteriomMesh"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

Scheduler userScheduler;  // to control your personal task
painlessMesh mesh;

// Alteriom package handlers
using namespace alteriom;

// User stub
void sendSensorData();
void handleIncomingPackage(uint32_t from, String& msg);
void handleCommandPackage(CommandPackage& cmd);
void handleStatusRequest();

Task taskSendSensorData(30000, TASK_FOREVER, &sendSensorData);
Task taskSendStatus(60000, TASK_FOREVER, &handleStatusRequest);

void setup() {
  Serial.begin(115200);

  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC |
                        COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&handleIncomingPackage);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Add tasks
  userScheduler.addTask(taskSendSensorData);
  taskSendSensorData.enable();

  userScheduler.addTask(taskSendStatus);
  taskSendStatus.enable();

  // Initialize sensors (placeholder)
  Serial.println("Alteriom Sensor Node initialized");
}

void loop() { mesh.update(); }

void sendSensorData() {
  // Create sensor package
  SensorPackage sensorData;
  sensorData.from = mesh.getNodeId();
  sensorData.sensorId = mesh.getNodeId();  // Use node ID as sensor ID
  sensorData.timestamp = mesh.getNodeTime();

  // Read sensor values (placeholder - replace with actual sensor readings)
  sensorData.temperature =
      23.5 + random(-50, 50) / 10.0;  // Simulate temperature
  sensorData.humidity = 45.0 + random(-100, 100) / 10.0;   // Simulate humidity
  sensorData.pressure = 1013.25 + random(-50, 50) / 10.0;  // Simulate pressure
  sensorData.batteryLevel = 85 + random(-20, 15);          // Simulate battery

  // Serialize and send
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  sensorData.addTo(std::move(obj));

  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);

  Serial.printf("Sent sensor data: T=%.1f, H=%.1f, P=%.1f, Bat=%d%%\n",
                sensorData.temperature, sensorData.humidity,
                sensorData.pressure, sensorData.batteryLevel);
}

void handleStatusRequest() {
  StatusPackage status;
  status.from = mesh.getNodeId();
  status.deviceStatus = 0x01;  // Device operational
  status.uptime = millis() / 1000;
  status.freeMemory = ESP.getFreeHeap() / 1024;  // KB
  status.wifiStrength = 75;                      // Placeholder
  status.firmwareVersion = "1.0.0-alteriom";

  // Serialize and send
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  status.addTo(std::move(obj));

  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);

  Serial.printf("Sent status: uptime=%ds, mem=%dKB\n", status.uptime,
                status.freeMemory);
}

void handleIncomingPackage(uint32_t from, String& msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());

  // Parse the JSON message
  JsonDocument doc;
  deserializeJson(doc, msg);
  JsonObject obj = doc.as<JsonObject>();

  // Check message type
  uint8_t msgType = obj["type"];

  switch (msgType) {
    case 200:  // SensorPackage
    {
      SensorPackage receivedSensor(obj);
      Serial.printf("Sensor data from %u: T=%.1f, H=%.1f\n",
                    receivedSensor.from, receivedSensor.temperature,
                    receivedSensor.humidity);
      // Process sensor data (store, forward, analyze, etc.)
    } break;

    case 201:  // CommandPackage
    {
      CommandPackage receivedCmd(obj);
      if (receivedCmd.dest == mesh.getNodeId()) {
        handleCommandPackage(receivedCmd);
      }
    } break;

    case 202:  // StatusPackage
    {
      StatusPackage receivedStatus(obj);
      Serial.printf("Status from %u: uptime=%ds, mem=%dKB, fw=%s\n",
                    receivedStatus.from, receivedStatus.uptime,
                    receivedStatus.freeMemory,
                    receivedStatus.firmwareVersion.c_str());
    } break;

    default:
      Serial.printf("Unknown message type: %d\n", msgType);
      break;
  }
}

void handleCommandPackage(CommandPackage& cmd) {
  Serial.printf("Received command %d for device %u\n", cmd.command,
                cmd.targetDevice);

  // Process different command types
  switch (cmd.command) {
    case 1:  // Reset command
      Serial.println("Reset command received");
      // Implement reset logic
      break;

    case 2:  // Configuration update
      Serial.printf("Config update: %s\n", cmd.parameters.c_str());
      // Parse and apply configuration
      break;

    case 3:  // Data request
      Serial.println("Data request received");
      sendSensorData();  // Send current sensor data
      break;

    default:
      Serial.printf("Unknown command: %d\n", cmd.command);
      break;
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() { Serial.printf("Changed connections\n"); }

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}