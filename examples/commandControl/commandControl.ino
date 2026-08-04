//************************************************************
// Command/control with delivery confirmation (issue #379)
//
// A controller node periodically broadcasts a command to all nodes and
// tracks, per node, whether the command was delivered. Nodes execute
// the command in their onReceive callback; the acknowledgment is sent
// automatically by the library.
//
// The broadcast ack overload requires includeSelf to be passed
// explicitly:
//   mesh.sendBroadcast(msg, false, callback, timeoutMs);
// The callback fires once per mesh node (excluding this node).
//************************************************************
#include <painlessMesh.h>

#define MESH_SSID "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

// Define CONTROLLER on exactly one node to make it send the commands
// #define CONTROLLER

Scheduler userScheduler;
painlessMesh mesh;

uint32_t commandsSent = 0;
uint32_t nodesConfirmed = 0;
uint32_t nodesMissed = 0;

void sendCommand();
Task taskSendCommand(TASK_SECOND * 15, TASK_FOREVER, &sendCommand);

void sendCommand() {
#ifdef CONTROLLER
  String cmd = String("{\"cmd\":\"setInterval\",\"seconds\":") +
               String(random(5, 60)) + String("}");
  ++commandsSent;
  nodesConfirmed = 0;
  nodesMissed = 0;

  bool queued = mesh.sendBroadcast(
      cmd, false,
      [](uint32_t nodeId, bool delivered, uint32_t latencyMs) {
        if (delivered) {
          ++nodesConfirmed;
          Serial.printf("Node %u confirmed command in %u ms\n", nodeId,
                        latencyMs);
        } else {
          ++nodesMissed;
          Serial.printf("Node %u did NOT confirm — resend or alert\n",
                        nodeId);
          // A real controller would retry with sendSingle(nodeId, ...)
        }
        if (mesh.pendingAcks() == 0)
          Serial.printf("Command %u done: %u confirmed, %u missed\n",
                        commandsSent, nodesConfirmed, nodesMissed);
      },
      5000);

  if (!queued)
    Serial.println("No nodes connected, command not sent");
#endif
}

void receivedCallback(uint32_t from, String &msg) {
  // Executing the command counts as delivery; the ack is automatic
  Serial.printf("Executing command from %u: %s\n", from, msg.c_str());
}

void setup() {
  Serial.begin(115200);

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);

  userScheduler.addTask(taskSendCommand);
  taskSendCommand.enable();
}

void loop() { mesh.update(); }
