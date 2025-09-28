#define _TASK_DO_NOT_YIELD 1
#include <Arduino.h>
#include <painlessMesh.h>

#define MESH_SSID "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

painlessMesh painMesh;
Scheduler TaskScheduler;
uint8_t known_nodes = 0;

void receivedData(uint32_t from, String &msg);
void connect();
void newConnection(uint32_t nodeId);
void dropConnection(uint32_t nodeId);
void changedConnection();

void setup() {
  Serial.begin(115200);
  Serial.println("Start");
  connect();
}

void loop() { painMesh.update(); }

void receivedData(uint32_t from, String &msg) {
  Serial.print("Received from node ");
  Serial.print(from);
  Serial.print(": ");
  Serial.println(msg);
}

void connect() {
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);

  painMesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC |
                            COMMUNICATION | GENERAL | MSG_TYPES | REMOTE |
                            S_TIME | DEBUG);  // all types on

  painMesh.setRoot(true);
  painMesh.setContainsRoot(true);
  painMesh.init(MESH_SSID, MESH_PASSWORD, &TaskScheduler, MESH_PORT);
  painMesh.onReceive(&receivedData);
  painMesh.onNewConnection(&newConnection);
  painMesh.onChangedConnections(&changedConnection);
  painMesh.onDroppedConnection(&dropConnection);
}

void newConnection(uint32_t nodeId) {
  Serial.printf(PSTR("--> mesh::newConnection: New Connection, nodeId = %u\n"),
                nodeId);
}

void dropConnection(uint32_t nodeId) {
  Serial.printf(
      PSTR("--> mesh::dropConnection: Drop Connection, nodeId = %u\n"), nodeId);
}

void changedConnection() {
  Serial.printf(PSTR("mesh::changedConnection-> Know Node Before:%d\n"),
                known_nodes);
  Serial.printf(PSTR("mesh::changedConnection-> Know Nodes:%d\n"), known_nodes);
}
