//************************************************************
// Reliable sensor logging with delivery confirmation (issue #379)
//
// Demonstrates the per-message acknowledgment API:
// 1. every 10 seconds a sensor reading is sent to a gateway node
// 2. the reading stays in a small retry buffer until the gateway
//    confirms delivery (delivered == true)
// 3. unconfirmed readings are retried up to MAX_ATTEMPTS times
//
// Run one node with GATEWAY defined (or adapt gatewayId discovery to
// your setup) and one or more sensor nodes.
//************************************************************
#include <painlessMesh.h>

#define MESH_SSID "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

// How long to wait for a delivery confirmation
#define ACK_TIMEOUT_MS 3000
// How often a reading is retried before it is dropped
#define MAX_ATTEMPTS 3

Scheduler userScheduler;
painlessMesh mesh;

// The node we log readings to. In a real deployment you would discover
// this via a broadcastannouncement or hardcode your gateway's node id.
uint32_t gatewayId = 0;

// A single buffered reading awaiting confirmation
struct PendingReading {
  String payload;
  uint8_t attempts = 0;
  bool inFlight = false;
};
PendingReading pending;

void sendReading();
Task taskSendReading(TASK_SECOND * 10, TASK_FOREVER, &sendReading);

void transmitPending() {
  if (gatewayId == 0 || pending.payload.length() == 0 || pending.inFlight)
    return;

  pending.attempts++;
  pending.inFlight = true;
  bool queued = mesh.sendSingle(
      gatewayId, pending.payload,
      [](uint32_t nodeId, bool delivered, uint32_t latencyMs) {
        pending.inFlight = false;
        if (delivered) {
          Serial.printf("Reading confirmed by %u in %u ms\n", nodeId,
                        latencyMs);
          pending.payload = "";
          pending.attempts = 0;
        } else if (pending.attempts < MAX_ATTEMPTS) {
          Serial.printf("No ack from %u, retrying (attempt %u)\n", nodeId,
                        pending.attempts + 1);
          transmitPending();
        } else {
          Serial.printf("Dropping reading after %u attempts\n",
                        pending.attempts);
          pending.payload = "";
          pending.attempts = 0;
        }
      },
      ACK_TIMEOUT_MS);

  if (!queued) {
    // No route to the gateway right now; try again on the next reading
    pending.inFlight = false;
  }
}

void sendReading() {
  if (pending.payload.length() > 0) {
    // Previous reading still unconfirmed — in a real application you
    // might queue multiple readings instead of overwriting
    Serial.println("Previous reading still pending, overwriting");
  }
  pending.payload =
      String("{\"sensor\":\"temp\",\"value\":") + String(random(15, 30)) +
      String(",\"node\":") + String(mesh.getNodeId()) + String("}");
  pending.attempts = 0;
  transmitPending();
}

void receivedCallback(uint32_t from, String &msg) {
  // The gateway simply prints what it receives. The acknowledgment is
  // sent automatically by the library — no application code needed.
  Serial.printf("Gateway received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New connection: %u\n", nodeId);
  // Naive gateway discovery for this example: treat the first node we
  // see as the gateway. Replace with your own discovery logic.
  if (gatewayId == 0) gatewayId = nodeId;
}

void setup() {
  Serial.begin(115200);

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);

  userScheduler.addTask(taskSendReading);
  taskSendReading.enable();
}

void loop() {
  // Ack timeouts are processed inside update() — no extra calls needed.
  // mesh.pendingAcks() tells you how many messages are still unconfirmed.
  mesh.update();
}
