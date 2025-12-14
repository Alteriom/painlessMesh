//************************************************************
// Bridge Node Example - Automatic Channel Detection & Resilient Initialization
//
// This example demonstrates the bridge-centric architecture that
// automatically detects the router's WiFi channel and configures the
// mesh network accordingly.
//
// Features:
// - Resilient initialization: Works even if router is unavailable at boot
// - Automatically detects router channel when available
// - Creates mesh network immediately (default channel if needed)
// - No manual channel configuration required
// - Automatically sets itself as root node
// - Retries router connection in background if initially unavailable
// - Broadcasts bridge status to mesh (Type 610 - BRIDGE_STATUS)
//   - Reports Internet connectivity status
//   - Updates every 30 seconds by default
//   - Enables nodes to implement failover and queueing logic
//
// POWER-UP ORDER INDEPENDENCE (v1.9.7+):
// The bridge now initializes successfully regardless of power-up order:
// - Bridge can boot before router is ready
// - Mesh nodes can connect immediately to bridge
// - Router connection is established automatically when available
// - No need to restart bridge when router becomes ready
//
// For more details, see BRIDGE_TO_INTERNET.md
//
// EXTERNAL DEVICE CONNECTIONS:
// ----------------------------
// External devices (phones, computers) can connect to the bridge's WiFi
// AP for debugging and testing purposes. The AP will broadcast the mesh
// SSID (e.g., "whateverYouLike") with the configured password.
//
// Connection details:
// - SSID: Your MESH_PREFIX value
// - Password: Your MESH_PASSWORD value
// - IP Range: 10.x.x.x (automatically assigned via DHCP)
// - Gateway: 10.x.x.1 (the bridge node itself)
//
// Note: ESP32 AP mode supports up to 10 concurrent connections by default,
// ESP8266 supports up to 4. If mesh nodes are already connected, fewer
// slots will be available for external devices.
//
// Troubleshooting external connections:
// 1. Ensure the bridge has successfully initialized (check serial output)
// 2. Wait a few seconds after boot for the AP to fully start
// 3. Check for channel conflicts with other WiFi networks
// 4. Try forgetting the network on your device and reconnecting
// 5. Monitor serial output with CONNECTION debug level enabled
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
  // 1. Attempt to connect to router and detect its channel
  // 2. Initialize mesh on the detected channel (or default if router unavailable)
  // 3. Set this node as root/bridge
  // 4. Maintain/retry router connection automatically
  // 5. Start broadcasting bridge status (Type 610) every 30 seconds
  //
  // RESILIENT INITIALIZATION (v1.9.7+):
  // The bridge will successfully initialize even if the router is unavailable
  // at boot time. It will:
  // - Establish the mesh network immediately on a default channel
  // - Accept connections from mesh nodes
  // - Retry router connection automatically in the background
  // - Update bridge status when router becomes available
  //
  // This solves the power-up order issue where bridge initialization would
  // fail if the router wasn't ready yet.
  mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                    ROUTER_SSID, ROUTER_PASSWORD,
                    &userScheduler, MESH_PORT);

  // Optional: Configure bridge status broadcasting
  // mesh.setBridgeStatusInterval(60000);  // Change to 60 seconds
  // mesh.enableBridgeStatusBroadcast(false);  // Disable if not needed

  // Setup over the air update support
  mesh.initOTAReceive("bridge");

  // Set up message callback
  mesh.onReceive(&receivedCallback);
  
  Serial.println("✓ Bridge node initialized and ready!");
  Serial.println("Mesh network active - accepting node connections");
  Serial.println("Router connection will be established automatically when available");
  Serial.println("Broadcasting bridge status to mesh every 30 seconds");
}

void loop() { mesh.update(); }

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
}
