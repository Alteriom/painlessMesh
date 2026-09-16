# Bridging painlessMesh to Internet via Router

You can bridge your mesh network to the Internet by creating a **gateway node** that connects to both the mesh network and your WiFi router simultaneously.

## Quick Start (Recommended: Auto Channel Detection)

The **bridge-centric approach** automatically detects your router's channel and configures the mesh accordingly. No manual channel configuration required!

### Resilient Initialization

**Power-up order no longer matters!** The bridge will initialize successfully even if:
- Router is not yet powered on
- Internet connection is unavailable
- Router is temporarily offline

The bridge will:
- Establish the mesh network immediately
- Accept connections from mesh nodes right away
- Retry router connection automatically in the background
- Update status when router/Internet becomes available

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

// Your router credentials
#define ROUTER_SSID     "YourRouterSSID"
#define ROUTER_PASSWORD "YourRouterPassword"

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Single call does everything:
  // 1. Attempts to connect to router and detect its channel
  // 2. Initializes mesh on detected channel (or default if router unavailable)
  // 3. Sets node as root/bridge
  // 4. Maintains/retries router connection automatically
  mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                    ROUTER_SSID, ROUTER_PASSWORD,
                    &userScheduler, MESH_PORT);
  
  mesh.onReceive(&receivedCallback);
}

void loop() {
  mesh.update();
}

void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("Received from mesh node %u: %s\n", from, msg.c_str());
  // Forward to Internet services (HTTP, MQTT, etc.)
}
```

**Expected Output (Router Available):**
```
=== Bridge Mode Initialization ===
Step 1: Attempting to connect to router YourRouterSSID...
✓ Router connected on channel 6
✓ Router IP: 192.168.1.100
Step 2: Initializing mesh on channel 6...
STARTUP: init(): Mesh channel set to 6
Step 3: Establishing bridge connection...
=== Bridge Mode Active ===
  Mesh SSID: MyMeshNetwork
  Mesh Channel: 6 (matches router)
  Router: YourRouterSSID (connected)
  Port: 5555
```

**Expected Output (Router Unavailable but Visible):**
```
=== Bridge Mode Initialization ===
Step 1: Attempting to connect to router YourRouterSSID...
⚠ Router connection unavailable during initialization
⚠ Scanning for router 'YourRouterSSID' to detect channel...
✓ Router found on channel 6 (not connected, will retry)
⚠ Proceeding with bridge setup on channel 6
⚠ Bridge will retry router connection in background
Step 2: Initializing mesh on channel 6...
STARTUP: init(): Mesh channel set to 6
Step 3: Establishing bridge connection...
=== Bridge Mode Active ===
  Mesh SSID: MyMeshNetwork
  Mesh Channel: 6 (default, router pending)
  Router: YourRouterSSID (will retry)
  Port: 5555

INFO: Bridge initialized without router connection
INFO: Mesh network is active and accepting node connections
INFO: Router connection will be established automatically when available
```

**Note:** If the router cannot connect but is visible in a WiFi scan, the bridge detects its channel and uses it for the mesh. This minimizes channel switching when the router becomes connectable. If the router is completely invisible (powered off), channel 1 is used as default.

### Regular Nodes with Auto-Detection

Regular mesh nodes can also auto-detect the mesh channel:

```cpp
void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // channel=0 means auto-detect
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, 
            WIFI_AP_STA, 0);
  
  mesh.onReceive(&receivedCallback);
}
```

**Expected Output:**
```
STARTUP: Auto-detecting mesh channel...
CONNECTION: Scanning all channels for mesh 'MyMeshNetwork'...
CONNECTION: Found mesh on channel 6 (RSSI: -45)
STARTUP: Mesh channel auto-detected: 6
```

### Automatic Channel Re-synchronization

A bridge lives on its router's channel, so the mesh has to be there too. The
library keeps it there without any configuration on the regular nodes:

- **A bridge announces its channel.** Every bridge status carries
  `routerChannel`, and an elected bridge's takeover message does too; peers
  that hear a takeover move their AP and station to that channel a second
  later. The channel a node last heard a bridge from is its *home*.
- **A node that finds nothing looks everywhere.** After two empty scans
  (about 30 s) a node re-detects the mesh channel. A node with nothing under
  its AP scans all channels at once; a node with stations attached scans one
  channel per pass so its children are not dropped.
- **A node follows the mesh, not a straggler.** A disconnected node joins
  the mesh wherever it is. A connected node leaves its partition only for a
  strictly bigger one, and only after seeing it on two consecutive scans;
  a node at home does not leave for a partition elsewhere, and away from
  home it returns as soon as it sees the mesh there.
- **An uplink lost at home is rescanned there.** The bridge's AP is on this
  channel; the node scans it again before looking anywhere else.

Set `mesh.setContainsRoot(true)` on every regular node of a mesh that has a
bridge (`initAsBridge()` sets it on the bridge): it is what lets a node that
is still connected to a partition the bridge has left notice that it has no
root and go looking for the bridge's channel.

## Manual Channel Configuration

If you prefer the traditional approach or need more control, you can still manually configure the channel:

```cpp
void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Initialize mesh with AP+STA mode on specific channel
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6);
  
  // Connect to your router
  mesh.stationManual(ROUTER_SSID, ROUTER_PASSWORD);
  mesh.setHostname("MESH_BRIDGE");
  
  // Configure as root/bridge node
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  
  mesh.onReceive(&receivedCallback);
}
```

## Important Requirements

### WiFi Channel Behavior

#### With initAsBridge() (Recommended)

The new `initAsBridge()` method automatically handles all channel detection and configuration:

1. Connects to your router first in STA mode
2. Detects the router's actual channel
3. Initializes the mesh AP on the detected channel
4. Maintains both connections on the same channel

**No manual channel configuration needed!** Just provide your router and mesh credentials.

#### With Manual Configuration

When using `stationManual()`, the library automatically handles channel switching. The ESP32/ESP8266 will:

1. Initially operate the mesh AP on your specified channel (e.g., channel 6)
2. Automatically switch to the router's channel when connecting via `stationManual()`
3. The mesh AP channel will adjust to match the router's channel

**Note:** While ESP32/ESP8266 hardware can only operate on one channel at a time in AP+STA mode, the WiFi stack automatically coordinates this. When connected to a router on a different channel, the mesh AP will operate on that channel instead.

**Best Practices:**

- Use `initAsBridge()` for new projects - it handles everything automatically
- Use channels 1, 6, or 11 (non-overlapping 2.4GHz channels) if not using a router
- Regular nodes should use `channel=0` to auto-detect the mesh
- For optimal performance, you may choose to configure your router to use your preferred mesh channel

### Other Requirements

1. **WIFI_AP_STA Mode**: This enables simultaneous AP (for mesh) and Station (for router) operation.

2. **Root Configuration**:
   - Call `mesh.setRoot(true)` on the bridge node
   - Call `mesh.setContainsRoot(true)` on all mesh nodes for optimal routing

3. **ESP32-C5 / ESP32-C6**: these need the ESP32 Arduino core 3.x and AsyncTCP v3.4.7 or later; `tcp_alloc` crashes on the C6 are the sign of an older AsyncTCP. See the [dependencies](README.md#dependencies) in the README.

## Complete Examples

We provide several working bridge examples in the repository:

- **Basic Bridge**: [`examples/bridge/bridge.ino`](examples/bridge/bridge.ino)
- **Bridge Failover**: [`examples/bridge_failover/`](examples/bridge_failover/) - Automatic election when the bridge goes
- **Shared Gateway**: [`examples/sharedGateway/`](examples/sharedGateway/) - Every node on the router
- **Send to Internet**: [`examples/sendToInternet/`](examples/sendToInternet/) - HTTP requests through the gateway, with a mock server for offline testing
- **MQTT Bridge**: [`examples/mqttBridge/mqttBridge.ino`](examples/mqttBridge/mqttBridge.ino) - Bridges mesh to MQTT broker
- **Web Server Bridge**: [`examples/webServer/webServer.ino`](examples/webServer/webServer.ino) - Provides web interface

## Forwarding Data to Internet

Once the bridge is established, you can forward mesh data to Internet services:

```cpp
void receivedCallback(uint32_t from, String& msg) {
  // Check if connected to router
  if (WiFi.status() == WL_CONNECTED) {
    // Forward to MQTT broker
    mqttClient.publish("mesh/data", msg.c_str());
    
    // Or send via HTTP
    HTTPClient http;
    http.begin("http://myserver.com/api/data");
    http.POST(msg);
    http.end();
  }
}
```

## Architecture Diagram

```text
Internet
   |
Router (WiFi)
   |
Bridge Node (AP+STA mode)
   |
Mesh Network
 / | \
Node1 Node2 Node3...
```

## Sending Data Through the Gateway

Regular nodes have no IP route to the Internet; `HTTPClient` on a regular
node fails with *connection refused*. They send through the gateway instead:

```cpp
// On every node, after mesh.init(): enables sending on regular nodes and
// relaying on the bridge
mesh.enableSendToInternet();

// Later, on a regular node
if (mesh.hasInternetConnection()) {          // a gateway with Internet is known
  mesh.sendToInternet(
      "https://api.example.com/data", jsonPayload,
      [](bool success, uint16_t httpStatus, String error) {
        Serial.printf("Delivery: %s (%u)\n", success ? "OK" : error.c_str(), httpStatus);
      });
}
```

The request travels to the gateway as a `GATEWAY_DATA` package, the gateway
makes the HTTP call and answers with `GATEWAY_ACK`. The gateway's HTTP work
is bounded by `NODE_TIMEOUT` (2 s per socket wait at the stock 10 s
watchdog), and its peers' watchdogs are postponed by exactly the time the
call took, so a slow endpoint cannot partition the mesh around the gateway.
An endpoint that needs longer needs a larger `NODE_TIMEOUT`; the
`static_assert` in `painlessmesh/gateway.hpp` says so at compile time.

## Bridge Failover

Any node given the router's credentials can take over when the bridge goes:

```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 0);
mesh.setContainsRoot(true);
mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
mesh.enableBridgeFailover(true);
mesh.onBridgeRoleChanged([](bool isBridge, const String& reason) {
  Serial.printf("%s: %s\n", isBridge ? "Promoted to bridge" : "Regular node", reason.c_str());
});
```

A bridge that stops cleanly (`mesh.stop()`) broadcasts that it is leaving
and the candidates elect within seconds; a bridge that loses power is
noticed when its last status ages out (60 s, plus up to one 30 s monitor
tick). The winner is the candidate with the best router RSSI, then the
longest uptime, then the most free memory, then the lowest node ID; it
promotes itself with `initAsBridge()` and announces its channel. The
protocol, its tuning (`setElectionStartupDelay()`,
`setElectionRandomDelay()`, `setBridgeTimeout()`, `setMinimumBridgeRSSI()`)
and its troubleshooting are in
[examples/bridge_failover/README.md](examples/bridge_failover/README.md).

## Multi-Bridge Coordination

Several bridges can serve one mesh. Each is started with a priority (10 is
primary, 1 is standby), and the bridges announce themselves to each other
every 30 s:

```cpp
mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD, ROUTER_SSID, ROUTER_PASSWORD,
                  &userScheduler, MESH_PORT, 10);       // priority 10
mesh.setBridgeSelectionStrategy(painlessMesh::PRIORITY_BASED);  // or ROUND_ROBIN, BEST_SIGNAL
mesh.onBridgeCoordination(
    [](const painlessmesh::plugin::BridgeCoordinationPackage& pkg, uint32_t from) {
      Serial.printf("Bridge %u: priority %d, load %d%%\n", from, pkg.priority, pkg.load);
    });
mesh.onBridgeCoordinationChanged(
    [](const painlessmesh::plugin::BridgeCoordinationPackage& pkg, uint32_t from, String change) {
      Serial.printf("Bridge %s: %u (%s)\n", change.c_str(), from, pkg.role.c_str());
    });
```

`getPrimaryBridge()` returns the bridge the strategy currently selects;
`getBridges()` lists every known bridge with its Internet state, RSSI and
last-seen time.

## Message Queue for Offline Periods

When no gateway has Internet, a node can hold messages and send them later:

```cpp
mesh.enableMessageQueue(true, 100);                     // after mesh.init()
uint32_t id = mesh.queueMessage(alarmJson, "https://api.example.com/alarm", PRIORITY_CRITICAL);

// Drain when connectivity returns: the queue never sends on its own
if (mesh.hasInternetConnection()) {
  for (auto& queued : mesh.flushMessageQueue()) {
    mesh.sendToInternet(queued.destination, queued.payload, onResult);
    mesh.removeQueuedMessage(queued.id);
  }
}
```

Priorities are `PRIORITY_CRITICAL`, `PRIORITY_HIGH`, `PRIORITY_NORMAL` and
`PRIORITY_LOW`; when the queue is full the lowest priority is evicted first
and a critical message is never evicted. `getQueueStats()` reports queued,
dropped and flushed counts.

## Shared Gateway Mode

When every node is within reach of the router, every node can be its own
gateway and the mesh is the fallback:

```cpp
mesh.initAsSharedGateway(MESH_PREFIX, MESH_PASSWORD, ROUTER_SSID, ROUTER_PASSWORD,
                         &userScheduler, MESH_PORT);
mesh.sendToInternet(url, payload, onResult);     // local uplink if healthy, else via the mesh
mesh.onGatewayChanged([](uint32_t oldGateway, uint32_t newGateway) {
  Serial.printf("Primary gateway %u -> %u\n", oldGateway, newGateway);
});
```

A node's own uplink is health-checked periodically (`hasLocalInternet()`),
and a request is served locally when it is healthy. See
[examples/sharedGateway](examples/sharedGateway/).

## Frequently Asked Questions

### Why are there three ways to connect a bridge?

1. `init()` followed by `stationManual()` — the original API, still the most
   flexible: you choose the channel and the mode.
2. `init()` with `stationSSID` and `stationPassword` — the same, in one call.
3. `initAsBridge()` — detects the router's channel, sets the root flags,
   retries the router in the background, and starts the bridge status
   broadcasts. Use this unless you have a reason not to.

### Does the bridge need to be up first?

No. Regular nodes started with `channel = 0` scan for the mesh, and a mesh
that forms before the bridge follows the bridge to its channel when it
appears. The ESP8266 should be a leaf in a mesh of more than a few nodes
(`init(..., maxconn = 0)`); see the README.

## Additional Resources

- [USER_GUIDE.md](USER_GUIDE.md) — the complete guide, including the API reference
- [examples/bridge_failover/README.md](examples/bridge_failover/README.md) — the election protocol in detail
- [SECURITY.md](SECURITY.md) — what gateway HTTPS does and does not protect
- [CHANGELOG.md](CHANGELOG.md) — what changed in 2.0, and what to know before upgrading
