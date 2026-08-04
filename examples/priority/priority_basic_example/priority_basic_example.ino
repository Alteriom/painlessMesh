/**
 * Basic Priority Messaging Example for painlessMesh
 * 
 * Demonstrates how to send messages with different priority levels.
 * 
 * Priority levels:
 * - PRIORITY_CRITICAL (0): Life/safety critical messages (alarms, emergencies)
 * - PRIORITY_HIGH (1): Important commands and status requests
 * - PRIORITY_NORMAL (2): Regular sensor data and routine updates (default)
 * - PRIORITY_LOW (3): Non-essential telemetry and debug logs
 * 
 * Hardware: ESP32 or ESP8266
 */

#include "painlessMesh.h"

#define MESH_PREFIX     "PriorityMeshDemo"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Task to send regular sensor data (NORMAL priority)
Task taskSensorData(5000, TASK_FOREVER, []() {
    String sensorData = "{\"type\":\"sensor\",\"temp\":23.5,\"humidity\":45}";
    
    // Send with NORMAL priority (default)
    mesh.sendBroadcast(sensorData, 2);  // Priority level 2 = NORMAL
    
    Serial.println("Sent sensor data (NORMAL priority)");
});

// Task to send status updates (HIGH priority)
Task taskStatusUpdate(10000, TASK_FOREVER, []() {
    String status = "{\"type\":\"status\",\"online\":true,\"uptime\":" + String(millis()) + "}";
    
    // Send with HIGH priority
    mesh.sendBroadcast(status, 1);  // Priority level 1 = HIGH
    
    Serial.println("Sent status update (HIGH priority)");
});

// Task to send low-priority debug logs
Task taskDebugLog(15000, TASK_FOREVER, []() {
    String debug = "{\"type\":\"debug\",\"freeHeap\":" + String(ESP.getFreeHeap()) + "}";
    
    // Send with LOW priority
    mesh.sendBroadcast(debug, 3);  // Priority level 3 = LOW
    
    Serial.println("Sent debug log (LOW priority)");
});

void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
}

void changedConnectionCallback() {
    Serial.printf("Connections changed\n");
}

void setup() {
    Serial.begin(115200);
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    
    // Add periodic tasks
    userScheduler.addTask(taskSensorData);
    userScheduler.addTask(taskStatusUpdate);
    userScheduler.addTask(taskDebugLog);
    
    taskSensorData.enable();
    taskStatusUpdate.enable();
    taskDebugLog.enable();
    
    Serial.println("Priority messaging demo started");
    Serial.println("Messages sent with different priorities:");
    Serial.println("  CRITICAL (0): Emergency alarms");
    Serial.println("  HIGH (1): Status updates");
    Serial.println("  NORMAL (2): Sensor data");
    Serial.println("  LOW (3): Debug logs");
}

void loop() {
    mesh.update();
}

// Example: Sending a CRITICAL alarm message
void sendCriticalAlarm(String alarmType, String message) {
    String criticalMsg = "{\"type\":\"alarm\",\"alarm\":\"" + alarmType + "\",\"msg\":\"" + message + "\"}";
    
    // Send with CRITICAL priority - will be sent immediately, bypassing queue
    mesh.sendBroadcast(criticalMsg, 0);  // Priority level 0 = CRITICAL
    
    Serial.printf("CRITICAL ALARM SENT: %s - %s\n", alarmType.c_str(), message.c_str());
}

// Example: Sending a direct message with priority
void sendCommandToNode(uint32_t destNode, String command) {
    String cmdMsg = "{\"type\":\"command\",\"cmd\":\"" + command + "\"}";
    
    // Send to specific node with HIGH priority
    mesh.sendSingle(destNode, cmdMsg, 1);  // Priority level 1 = HIGH
    
    Serial.printf("Command sent to node %u with HIGH priority\n", destNode);
}
