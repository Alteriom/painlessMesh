//************************************************************
// Bridge Node Example - Automatic Channel Detection
//
// This example demonstrates the new bridge-centric architecture that
// automatically detects the router's WiFi channel and configures the
// mesh network accordingly.
//
// Features:
// - Connects to router FIRST and auto-detects its channel
// - Creates mesh network on the same channel as router
// - No manual channel configuration required
// - Automatically sets itself as root node
//
// For more details, see BRIDGE_TO_INTERNET.md
//************************************************************
#include "painlessMesh.h"

// Mesh network configuration
#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

// Router credentials
#define ROUTER_SSID "MSYSSID"
#define ROUTER_PASSWORD "SSIDPASSWORD"

// prototypes
void receivedCallback(uint32_t from, String &msg);

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  
  // Set debug message types before init() to see startup messages
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

  // NEW: Single call to initialize as bridge with auto channel detection
  // This will:
  // 1. Connect to router and detect its channel
  // 2. Initialize mesh on the detected channel
  // 3. Set this node as root
  // 4. Maintain router connection
  mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                    ROUTER_SSID, ROUTER_PASSWORD,
                    &userScheduler, MESH_PORT);

  // Setup over the air update support
  mesh.initOTAReceive("bridge");

  // Set up message callback
  mesh.onReceive(&receivedCallback);
  
  Serial.println("Bridge node initialized and ready!");
}

void loop() { mesh.update(); }

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
}
