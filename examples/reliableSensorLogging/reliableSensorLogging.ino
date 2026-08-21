//************************************************************
// Reliable sensor logging with delivery confirmation (issue #379)
//
// Demonstrates the per-message acknowledgment API:
// 1. every 10 seconds a sensor reading is sent to a gateway node
// 2. the reading stays in a small retry buffer until the gateway
//    confirms delivery (delivered == true)
// 3. unconfirmed readings are retried up to MAX_ATTEMPTS times
//
// Run one node with GATEWAY defined and one or more sensor nodes. The
// gateway announces its node id; only sensor nodes generate readings.
//************************************************************
#include <painlessMesh.h>

#define MESH_SSID "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

// Define GATEWAY on exactly one node.
// #define GATEWAY

// How long to wait for a delivery confirmation
#define ACK_TIMEOUT_MS 3000
// How often a reading is retried before it is dropped
#define MAX_ATTEMPTS 3

Scheduler userScheduler;
painlessMesh mesh;

// The node we log readings to. In a real deployment you would discover
// this via a broadcast announcement or hardcode your gateway's node id.
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
#ifdef GATEWAY
void announceGateway();
Task taskAnnounceGateway(TASK_SECOND * 5, TASK_FOREVER, &announceGateway);
#endif

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
  if (pending.inFlight) {
    Serial.println("Previous reading still awaiting acknowledgment, skipping");
    return;
  }
  if (pending.payload.length() > 0) {
    // A previous attempt had no route. Replace that unsent reading; a real
    // application might queue multiple readings instead.
    Serial.println("Previous unsent reading still pending, overwriting");
  }
  pending.payload =
      String("{\"sensor\":\"temp\",\"value\":") + String(random(15, 30)) +
      String(",\"node\":") + String(mesh.getNodeId()) + String("}");
  pending.attempts = 0;
  transmitPending();
}

#ifdef GATEWAY
void announceGateway() {
  mesh.sendBroadcast("GATEWAY_ANNOUNCE");
  Serial.printf("Gateway announcement sent from %u\n", mesh.getNodeId());
}
#endif

void receivedCallback(uint32_t from, String &msg) {
#ifdef GATEWAY
  // The gateway simply prints what it receives. The acknowledgment is
  // sent automatically by the library — no application code needed.
  Serial.printf("Gateway received from %u: %s\n", from, msg.c_str());
#else
  if (msg == "GATEWAY_ANNOUNCE") {
    gatewayId = from;
    Serial.printf("Discovered gateway node %u\n", gatewayId);
  }
#endif
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New connection: %u\n", nodeId);
#ifdef GATEWAY
  // Announce immediately as well as periodically so new sensors do not
  // wait for the next scheduled announcement.
  announceGateway();
#endif
}

void setup() {
  Serial.begin(115200);

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);

#ifdef GATEWAY
  userScheduler.addTask(taskAnnounceGateway);
  taskAnnounceGateway.enable();
  announceGateway();
#else
  userScheduler.addTask(taskSendReading);
  taskSendReading.enable();
#endif
}

void loop() {
  // Ack timeouts are processed inside update() — no extra calls needed.
  // mesh.pendingAcks() tells you how many messages are still unconfirmed.
  mesh.update();
}
