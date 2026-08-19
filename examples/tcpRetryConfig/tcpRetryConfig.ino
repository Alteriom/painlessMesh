//************************************************************
// Tuning the TCP connection retry behaviour (issue #378)
//
// painlessMesh retries a failed TCP connection with exponential backoff
// before giving up and falling back to a full WiFi reconnect. The defaults
// (5 retries, 1s base delay -> 1s, 2s, 4s, 8s, 8s) are tuned for general
// purpose meshes and deliberately favour reliability over speed.
//
// setTcpRetryConfig() lets you pick a different trade-off. Three ready-made
// profiles are shown below; switch between them with ACTIVE_PROFILE.
//
// Read examples/tcpRetryConfig/README.md before changing these values - the
// defaults exist because 1.9.x raised them to fix real mesh instability.
//************************************************************
#include <painlessMesh.h>

#define   MESH_SSID       "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

// ---------------------------------------------------------------------------
// Profiles
// ---------------------------------------------------------------------------
#define PROFILE_REALTIME  1   // low latency: fail fast, reconnect fast
#define PROFILE_RELIABLE  2   // industrial: many retries, long backoff
#define PROFILE_BATTERY   3   // conserve power: few retries, long block

// >>> Change this line to try a different profile <<<
#define ACTIVE_PROFILE    PROFILE_REALTIME

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void droppedConnectionCallback(uint32_t nodeId);

Scheduler     userScheduler;
painlessMesh  mesh;

Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage);

// Build the retry configuration for the selected profile.
painlessmesh::tcp::TcpRetryConfig buildRetryConfig() {
  painlessmesh::tcp::TcpRetryConfig cfg;  // starts at the library defaults

#if ACTIVE_PROFILE == PROFILE_REALTIME
  // Real-time sensor / LED meshes: a stalled node is worse than a dropped
  // one. Give up on the TCP handshake almost immediately and go straight
  // back to scanning for another parent.
  cfg.maxRetries = 1;                    // default 5
  cfg.retryDelayMs = 200;                // default 1000
  cfg.stabilizationDelayMs = 100;        // default 500
  cfg.exhaustionReconnectDelayMs = 1000; // default 10000
  cfg.failureBlockDurationMs = 5000;     // default 60000

#elif ACTIVE_PROFILE == PROFILE_RELIABLE
  // Industrial / high-reliability meshes: connectivity matters more than
  // how long it takes to get there. Retry patiently and keep a failed peer
  // out of the running for a good while.
  cfg.maxRetries = 10;                     // default 5 (this is the maximum)
  cfg.retryDelayMs = 2000;                 // default 1000
  cfg.stabilizationDelayMs = 1000;         // default 500
  cfg.exhaustionReconnectDelayMs = 30000;  // default 10000
  // Must exceed one full failure cycle (126s of retries + 30s reconnect),
  // otherwise a dead peer leaves the blocklist before we finished failing
  // over and gets re-selected immediately.
  cfg.failureBlockDurationMs = 180000;     // default 60000

#elif ACTIVE_PROFILE == PROFILE_BATTERY
  // Battery-powered nodes: every retry is radio time. Few attempts, spaced
  // widely, and a long blocklist so we do not keep waking up for a peer
  // that is known to be down.
  cfg.maxRetries = 2;                      // default 5
  cfg.retryDelayMs = 3000;                 // default 1000
  cfg.stabilizationDelayMs = 500;          // default 500
  cfg.exhaustionReconnectDelayMs = 60000;  // default 10000
  cfg.failureBlockDurationMs = 300000;     // default 60000

#else
  #error "ACTIVE_PROFILE must be one of PROFILE_REALTIME, PROFILE_RELIABLE, PROFILE_BATTERY"
#endif

  return cfg;
}

void setup() {
  Serial.begin(115200);

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

  // Apply the retry configuration BEFORE init() so the very first connection
  // attempt already uses it.
  mesh.setTcpRetryConfig(buildRetryConfig());

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);

  // Read the configuration back. Values outside safe operating bounds are
  // clamped by the setter, so this prints what is actually in effect - not
  // necessarily what was requested.
  painlessmesh::tcp::TcpRetryConfig active = mesh.getTcpRetryConfig();
  Serial.println();
  Serial.println(F("Effective TCP retry configuration:"));
  Serial.printf("  maxRetries                 = %u\n",
                (unsigned)active.maxRetries);
  Serial.printf("  retryDelayMs               = %u\n",
                (unsigned)active.retryDelayMs);
  Serial.printf("  stabilizationDelayMs       = %u\n",
                (unsigned)active.stabilizationDelayMs);
  Serial.printf("  exhaustionReconnectDelayMs = %u\n",
                (unsigned)active.exhaustionReconnectDelayMs);
  Serial.printf("  failureBlockDurationMs     = %u\n",
                (unsigned)active.failureBlockDurationMs);

  // Worst-case time spent retrying before falling back to a WiFi reconnect.
  uint32_t worstCase = 0;
  for (uint8_t i = 0; i < active.maxRetries; ++i) {
    worstCase += painlessmesh::tcp::retryBackoffDelay(active, i);
  }
  Serial.printf("  -> worst-case retry time   = %u ms\n", (unsigned)worstCase);
  Serial.printf("  -> plus reconnect delay    = %u ms\n",
                (unsigned)(worstCase + active.exhaustionReconnectDelayMs));
  Serial.println();

  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onDroppedConnection(&droppedConnectionCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  mesh.update();
}

void sendMessage() {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast(msg);
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("tcpRetryConfig: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> Connected to node %u\n", nodeId);
}

void droppedConnectionCallback(uint32_t nodeId) {
  // With an aggressive profile you should expect to see this more often -
  // and to see the reconnect that follows it happen much sooner.
  Serial.printf("--> Dropped connection to node %u\n", nodeId);
}
