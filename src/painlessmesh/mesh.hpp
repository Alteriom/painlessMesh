#ifndef _PAINLESS_MESH_MESH_HPP_
#define _PAINLESS_MESH_MESH_HPP_

#include <algorithm>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <queue>

#include "painlessmesh/configuration.hpp"

#include "painlessmesh/connection.hpp"
#include "painlessmesh/gateway.hpp"
#include "painlessmesh/logger.hpp"
#include "painlessmesh/message_queue.hpp"
#include "painlessmesh/message_tracker.hpp"
#include "painlessmesh/ntp.hpp"
#include "painlessmesh/plugin.hpp"
#include "painlessmesh/protocol.hpp"
#include "painlessmesh/rtc.hpp"
#include "painlessmesh/tcp.hpp"

#ifdef PAINLESSMESH_ENABLE_OTA
#include "painlessmesh/ota.hpp"
#endif

namespace painlessmesh {
typedef std::function<void(uint32_t nodeId)> newConnectionCallback_t;
typedef std::function<void(uint32_t nodeId)> droppedConnectionCallback_t;
typedef std::function<void(uint32_t from, TSTRING &msg)> receivedCallback_t;
typedef std::function<void()> changedConnectionsCallback_t;
typedef std::function<void(int32_t offset)> nodeTimeAdjustedCallback_t;
typedef std::function<void(uint32_t nodeId, int32_t delay)> nodeDelayCallback_t;
typedef std::function<void(uint32_t bridgeNodeId, bool internetAvailable)> bridgeStatusChangedCallback_t;
typedef std::function<void(uint32_t timestamp)> rtcSyncCompleteCallback_t;
typedef std::function<void(bool available)> localInternetChangedCallback_t;
typedef std::function<void(uint32_t oldPrimary, uint32_t newPrimary)> gatewayChangedCallback_t;

/**
 * Callback type for Internet request results
 *
 * @param success Whether the request was delivered successfully
 * @param httpStatus HTTP status code from the destination (0 if not applicable)
 * @param error Human-readable error message (empty if success)
 */
typedef std::function<void(bool success, uint16_t httpStatus, TSTRING error)>
    internetResultCallback_t;

/**
 * Pending Internet request entry
 *
 * Tracks a pending sendToInternet() request for acknowledgment handling
 */
struct PendingInternetRequest {
  uint32_t messageId = 0;          ///< Unique message ID
  uint32_t timestamp = 0;          ///< When request was sent (millis)
  uint8_t retryCount = 0;          ///< Number of retries attempted
  uint8_t maxRetries = 3;          ///< Maximum retry attempts
  uint8_t priority = 2;            ///< Priority level (0=CRITICAL to 3=LOW)
  uint32_t timeoutMs = 30000;      ///< Timeout in milliseconds
  uint32_t retryDelayMs = 1000;    ///< Current retry delay (for exponential backoff)
  uint32_t gatewayNodeId = 0;      ///< Target gateway node ID
  TSTRING destination = "";        ///< Internet destination URL
  TSTRING payload = "";            ///< Request payload
  internetResultCallback_t callback;  ///< User callback for result

  /**
   * Check if this request has timed out
   */
  bool isTimedOut() const {
    return (static_cast<uint32_t>(millis()) - timestamp) > timeoutMs;
  }
};

/**
 * Bridge information structure
 * 
 * Tracks the status and health of bridge nodes in the mesh network
 */
class BridgeInfo {
public:
  uint32_t nodeId = 0;              // Bridge node ID
  bool internetConnected = false;   // Is bridge connected to Internet?
  int8_t routerRSSI = 0;           // Router WiFi signal strength in dBm
  uint8_t routerChannel = 0;       // Router WiFi channel
  uint32_t lastSeen = 0;           // Timestamp when last status received (millis)
  uint32_t uptime = 0;             // Bridge uptime in milliseconds
  TSTRING gatewayIP = "";          // Router gateway IP address
  uint32_t timestamp = 0;          // Timestamp from bridge status message
  
  /**
   * Check if this bridge is considered healthy
   * A bridge is healthy if we've received a status update within the timeout period
   */
  bool isHealthy(uint32_t timeoutMs = 60000) const {
    // Cast millis() to uint32_t to handle overflow correctly
    // This ensures consistent behavior on both embedded and test environments
    return (static_cast<uint32_t>(millis()) - lastSeen) < timeoutMs;
  }
};

/**
 * Bridge Health Metrics structure
 * 
 * Comprehensive metrics for monitoring bridge health, connectivity quality,
 * and performance. Useful for troubleshooting and capacity planning.
 */
struct BridgeHealthMetrics {
  // Connectivity
  uint32_t uptimeSeconds = 0;
  uint32_t internetUptimeSeconds = 0;
  uint32_t totalDisconnects = 0;
  uint32_t currentUptime = 0;
  
  // Signal Quality
  int8_t currentRSSI = 0;
  int8_t avgRSSI = 0;
  int8_t minRSSI = 0;
  int8_t maxRSSI = -127;
  
  // Traffic
  uint64_t bytesRx = 0;
  uint64_t bytesTx = 0;
  uint32_t messagesRx = 0;
  uint32_t messagesTx = 0;
  uint32_t messagesQueued = 0;
  uint32_t messagesDropped = 0;
  
  // Performance
  uint32_t avgLatencyMs = 0;
  uint8_t packetLossPercent = 0;
  uint32_t meshNodeCount = 0;
};

/**
 * Bridge Status structure
 * 
 * Current status of this node's bridge role and connectivity
 */
struct BridgeStatus {
  bool isBridge = false;              // Is this node acting as a bridge?
  bool internetConnected = false;     // Is Internet connection available?
  TSTRING role = "regular";           // Role: "regular", "bridge", "root"
  uint32_t bridgeNodeId = 0;          // Current bridge node ID (0 if none)
  int8_t bridgeRSSI = 0;             // Signal strength to bridge/router (dBm)
  uint32_t timeSinceBridgeChange = 0; // Time since last bridge change (ms)
};

/**
 * Election Record structure
 * 
 * Records information about bridge election events
 */
struct ElectionRecord {
  uint32_t timestamp = 0;          // When election occurred (millis)
  uint32_t winnerNodeId = 0;       // Node that won election
  int8_t winnerRSSI = 0;          // Winner's router RSSI
  uint32_t candidateCount = 0;    // Number of candidates
  TSTRING reason = "";             // Why election was triggered
};

/**
 * Bridge Change Event structure
 * 
 * Records the last bridge change event
 */
struct BridgeChangeEvent {
  uint32_t timestamp = 0;            // When change occurred (millis)
  uint32_t oldBridgeId = 0;         // Previous bridge node ID
  uint32_t newBridgeId = 0;         // New bridge node ID
  TSTRING reason = "";               // Reason for change
  bool internetAvailable = false;    // Internet available after change
};

/**
 * Bridge Connectivity Test Result
 * 
 * Results from bridge connectivity testing
 */
struct BridgeTestResult {
  bool success = false;              // Overall test success
  bool bridgeReachable = false;      // Can reach bridge node
  bool internetReachable = false;    // Can reach Internet (if bridge available)
  uint32_t latencyMs = 0;           // Round-trip latency to bridge
  TSTRING message = "";              // Detailed test message
};

/**
 * Main api class for the mesh
 *
 * Brings all the functions together except for the WiFi functions
 */
template <class T>
class Mesh : public ntp::MeshTime, public plugin::PackageHandler<T> {
 public:
  void init(uint32_t id) {
    using namespace logger;
    if (!isExternalScheduler) {
      mScheduler = new Scheduler();
    }
    this->nodeId = id;

#ifdef ESP32
    xSemaphore = xSemaphoreCreateMutex();
#endif

    // Add package handlers
    this->callbackList = painlessmesh::ntp::addPackageCallback(
        std::move(this->callbackList), (*this));
    this->callbackList = painlessmesh::router::addPackageCallback(
        std::move(this->callbackList), (*this));

    // Add bridge status package handler (Type BRIDGE_STATUS)
    // This will be called when any node receives a bridge status broadcast
    this->callbackList.onPackage(
        protocol::BRIDGE_STATUS,
        [this](protocol::Variant& variant, std::shared_ptr<T>, uint32_t) {
          // We need to manually parse the JSON since BridgeStatusPackage is in alteriom namespace
          // and may not be available in all contexts. We'll parse the critical fields directly.
          JsonDocument doc;
          TSTRING str;
          variant.printTo(str);
          deserializeJson(doc, str);
          JsonObject obj = doc.as<JsonObject>();
          
          if (obj["internetConnected"].is<bool>()) {
            uint32_t bridgeNodeId = obj["from"];
            bool internetConnected = obj["internetConnected"];
            int8_t routerRSSI = obj["routerRSSI"] | 0;
            uint8_t routerChannel = obj["routerChannel"] | 0;
            uint32_t uptime = obj["uptime"] | 0;
            TSTRING gatewayIP = obj["gatewayIP"].as<TSTRING>();
            uint32_t timestamp = obj["timestamp"] | 0;
            
            // Update bridge status
            this->updateBridgeStatus(bridgeNodeId, internetConnected, routerRSSI, 
                                    routerChannel, uptime, gatewayIP, timestamp);
            
            Log(GENERAL, "Bridge status received from %u: Internet %s\n",
                bridgeNodeId, internetConnected ? "Connected" : "Disconnected");
          }
          return false;  // Don't consume the package, allow other handlers
        });

    this->changedConnectionCallbacks.push_back([this](uint32_t nodeId) {
      Log(MESH_STATUS, "Changed connections in neighbour %u\n", nodeId);
      if (nodeId != 0) layout::syncLayout<T>((*this), nodeId);
    });
    this->droppedConnectionCallbacks.push_back([this](uint32_t nodeId,
                                                      bool station) {
      Log(MESH_STATUS, "Dropped connection %u, station %d\n", nodeId, station);
      this->metricsDisconnectCount++;
      this->eraseClosedConnections();
    });
    this->newConnectionCallbacks.push_back([](uint32_t nodeId) {
      Log(MESH_STATUS, "New connection %u\n", nodeId);
    });
  }

  void init(Scheduler *scheduler, uint32_t id) {
    this->setScheduler(scheduler);
    this->init(id);
  }

#ifdef PAINLESSMESH_ENABLE_OTA
  std::shared_ptr<Task> offerOTA(painlessmesh::plugin::ota::Announce announce) {
    auto announceTask = this->addTask(TASK_SECOND * 60, 60, [this, announce]() {
      this->sendPackage(&announce);
    });
    return announceTask;
  }

  std::shared_ptr<Task> offerOTA(TSTRING role, TSTRING hardware, TSTRING md5,
                                 size_t noPart, bool forced = false,
                                 bool broadcasted = false, bool compressed = false) {
    painlessmesh::plugin::ota::Announce announce;
    announce.md5 = md5;
    announce.role = role;
    announce.hardware = hardware;
    announce.from = this->nodeId;
    announce.noPart = noPart;
    announce.forced = forced;
    announce.broadcasted = broadcasted;
    announce.compressed = compressed;
    return offerOTA(announce);
  }

  void initOTASend(
      painlessmesh::plugin::ota::otaDataPacketCallbackType_t callback,
      size_t otaPartSize) {
    painlessmesh::plugin::ota::addSendPackageCallback(
        *this->mScheduler, (*this), callback, otaPartSize);
  }
  void initOTAReceive(TSTRING role = "",
                      std::function<void(int, int)> progress_cb = NULL) {
    painlessmesh::plugin::ota::addReceivePackageCallback(
        *this->mScheduler, (*this), role, progress_cb);
  }
#endif

  /**
   * Set the node as an root/master node for the mesh
   *
   * This is an optional setting that can speed up mesh formation.
   * At most one node in the mesh should be a root, or you could
   * end up with multiple subMeshes.
   *
   * We recommend any AP_ONLY nodes (e.g. a bridgeNode) to be set
   * as a root node.
   *
   * If one node is root, then it is also recommended to call
   * painlessMesh::setContainsRoot() on all the nodes in the mesh.
   */
  void setRoot(bool on = true) { this->root = on; };

  /**
   * The mesh should contains a root node
   *
   * This will cause the mesh to restructure more quickly around the root node.
   * Note that this could have adverse effects if set, while there is no root
   * node present. Also see painlessMesh::setRoot().
   */
  void setContainsRoot(bool on = true) { shouldContainRoot = on; };

  /**
   * Check whether this node is a root node.
   */
  bool isRoot() { return this->root; };

  /**
   * Change the internal log level
   */
  void setDebugMsgTypes(uint16_t types) { Log.setLogLevel(types); }

  /**
   * Disconnect and stop this node
   */
  void stop() {
    using namespace logger;
    // Close all connections
    while (this->subs.size() > 0) {
      auto conn = this->subs.begin();
      (*conn)->close();
      this->eraseClosedConnections();
    }
    plugin::PackageHandler<T>::stop();

    newConnectionCallbacks.clear();
    droppedConnectionCallbacks.clear();
    changedConnectionCallbacks.clear();

    if (!isExternalScheduler) {
      delete mScheduler;
    }
  }

  /** Perform crucial maintenance task
   *
   * Add this to your loop() function. This routine runs various maintenance
   * tasks.
   */
  void update(void) {
    if (semaphoreTake()) {
      // Check if something is executed (returns false)
      if (!mScheduler->execute())
        Log(logger::GENERAL, "update(): Scheduler executed a task\n");
      semaphoreGive();
    }
    return;
  }

  /** Send message to a specific node
   *
   * @param destId The nodeId of the node to send it to.
   * @param msg The message to send
   *
   * @return true if everything works, false if not.
   */
  bool sendSingle(uint32_t destId, TSTRING msg) {
    Log(logger::COMMUNICATION, "sendSingle(): dest=%u msg=%s\n", destId,
        msg.c_str());
    auto single = painlessmesh::protocol::Single(this->nodeId, destId, msg);
    return painlessmesh::router::send<T>(single, (*this));
  }
  
  /** Send message to a specific node with priority
   *
   * @param destId The nodeId of the node to send it to.
   * @param msg The message to send
   * @param priorityLevel Priority level: 0=CRITICAL, 1=HIGH, 2=NORMAL, 3=LOW
   *
   * @return true if everything works, false if not.
   */
  bool sendSingle(uint32_t destId, TSTRING msg, uint8_t priorityLevel) {
    Log(logger::COMMUNICATION, "sendSingle(): dest=%u msg=%s priority=%u\n", destId,
        msg.c_str(), priorityLevel);
    auto single = painlessmesh::protocol::Single(this->nodeId, destId, msg);
    auto conn = painlessmesh::router::findRoute<T>((*this), destId);
    if (!conn) return false;
    return painlessmesh::router::sendWithPriority<painlessmesh::protocol::Single, T>(single, conn, priorityLevel);
  }

  /** Broadcast a message to every node on the mesh network.
   *
   * @param includeSelf Send message to myself as well. Default is false.
   *
   * @return true if everything works, false if not
   */
  bool sendBroadcast(TSTRING msg, bool includeSelf = false) {
    using namespace logger;
    Log(COMMUNICATION, "sendBroadcast(): msg=%s\n", msg.c_str());
    painlessmesh::protocol::Broadcast pkg(this->nodeId, 0, msg);
    auto success = router::broadcast<protocol::Broadcast, T>(pkg, (*this), 0);
    if (includeSelf) {
      protocol::Variant var(pkg);
      this->callbackList.execute(var.type(), var, NULL, 0);
    }
    if (success > 0) return true;
    return false;
  }
  
  /** Broadcast a message with priority to every node on the mesh network.
   *
   * @param msg The message to broadcast
   * @param priorityLevel Priority level: 0=CRITICAL, 1=HIGH, 2=NORMAL, 3=LOW
   * @param includeSelf Send message to myself as well. Default is false.
   *
   * @return true if everything works, false if not
   */
  bool sendBroadcast(TSTRING msg, uint8_t priorityLevel, bool includeSelf = false) {
    using namespace logger;
    Log(COMMUNICATION, "sendBroadcast(): msg=%s priority=%u\n", msg.c_str(), priorityLevel);
    painlessmesh::protocol::Broadcast pkg(this->nodeId, 0, msg);
    
    // Broadcast to all connections with priority
    size_t success = 0;
    for (auto&& conn : this->subs) {
      if (conn->nodeId != 0) {
        painlessmesh::protocol::Variant variant(pkg);
        TSTRING msgStr;
        variant.printTo(msgStr);
        auto sent = conn->addMessageWithPriority(msgStr, priorityLevel);
        if (sent) ++success;
      }
    }
    
    if (includeSelf) {
      protocol::Variant var(pkg);
      this->callbackList.execute(var.type(), var, NULL, 0);
    }
    if (success > 0) return true;
    return false;
  }

  /** Sends a node a packet to measure network trip delay to that node.
   *
   * After calling this function, user program have to wait to the response in
   * the form of a callback specified by onNodeDelayReceived().
   *
   * @return true if nodeId is connected to the mesh, false otherwise
   */
  bool startDelayMeas(uint32_t id) {
    using namespace logger;
    Log(S_TIME, "startDelayMeas(): NodeId %u\n", id);
    auto conn = painlessmesh::router::findRoute<T>((*this), id);
    if (!conn) return false;
    return router::send<protocol::TimeDelay, T>(
        protocol::TimeDelay(this->nodeId, id, this->getNodeTime()), conn);
  }

  /** Set a callback routine for any messages that are addressed to this node.
   *
   * Every time this node receives a message, this callback routine will the
   * called.  â€œfromâ€ is the id of the original sender of the message, and â€œmsgâ€
   * is a string that contains the message.  The message can be anything.  A
   * JSON, some other text string, or binary data.
   *
   * \code
   * mesh.onReceive([](auto nodeId, auto msg) {
   *    // Do something with the message
   *    Serial.println(msg);
   * });
   * \endcode
   */
  void onReceive(receivedCallback_t onReceive) {
    using namespace painlessmesh;
    this->callbackList.onPackage(
        protocol::SINGLE,
        [onReceive](protocol::Variant &variant, std::shared_ptr<T>, uint32_t) {
          auto pkg = variant.to<protocol::Single>();
          onReceive(pkg.from, pkg.msg);
          return false;
        });
    this->callbackList.onPackage(
        protocol::BROADCAST,
        [onReceive](protocol::Variant &variant, std::shared_ptr<T>, uint32_t) {
          auto pkg = variant.to<protocol::Broadcast>();
          onReceive(pkg.from, pkg.msg);
          return false;
        });
  }

  /** Callback that gets called every time the local node makes a new
   * connection.
   *
   * \code
   * mesh.onNewConnection([](auto nodeId) {
   *    // Do something with the event
   *    Serial.println(String(nodeId));
   * });
   * \endcode
   */
  void onNewConnection(newConnectionCallback_t onNewConnection) {
    Log(logger::GENERAL, "onNewConnection():\n");
    newConnectionCallbacks.push_back([onNewConnection](uint32_t nodeId) {
      if (nodeId != 0) onNewConnection(nodeId);
    });
  }

  /** Callback that gets called every time the local node drops a connection.
   *
   * \code
   * mesh.onDroppedConnection([](auto nodeId) {
   *    // Do something with the event
   *    Serial.println(String(nodeId));
   * });
   * \endcode
   */
  void onDroppedConnection(droppedConnectionCallback_t onDroppedConnection) {
    droppedConnectionCallbacks.push_back(
        [onDroppedConnection](uint32_t nodeId, bool station) {
          if (nodeId != 0) onDroppedConnection(nodeId);
        });
  }

  /** Callback that gets called every time the layout of the mesh changes
   *
   * \code
   * mesh.onChangedConnections([]() {
   *    // Do something with the event
   * });
   * \endcode
   */
  void onChangedConnections(changedConnectionsCallback_t onChangedConnections) {
    Log(logger::GENERAL, "onChangedConnections():\n");
    changedConnectionCallbacks.push_back(
        [onChangedConnections](uint32_t nodeId) {
          if (nodeId != 0) onChangedConnections();
        });
  }

  /** Callback that gets called every time node time gets adjusted
   *
   * Node time is automatically kept in sync in the mesh. This gets called
   * whenever the time is to far out of sync with the rest of the mesh and gets
   * adjusted.
   *
   * \code
   * mesh.onNodeTimeAdjusted([](auto offset) {
   *    // Do something with the event
   *    Serial.println(String(offset));
   * });
   * \endcode
   */
  void onNodeTimeAdjusted(nodeTimeAdjustedCallback_t onTimeAdjusted) {
    Log(logger::GENERAL, "onNodeTimeAdjusted():\n");
    nodeTimeAdjustedCallback = onTimeAdjusted;
  }

  /** Callback that gets called when a delay measurement is received.
   *
   * This fires when a time delay masurement response is received, after a
   * request was sent.
   *
   * \code
   * mesh.onNodeDelayReceived([](auto nodeId, auto delay) {
   *    // Do something with the event
   *    Serial.println(String(delay));
   * });
   * \endcode
   */
  void onNodeDelayReceived(nodeDelayCallback_t onDelayReceived) {
    Log(logger::GENERAL, "onNodeDelayReceived():\n");
    nodeDelayReceivedCallback = onDelayReceived;
  }

  /** Callback that gets called when bridge status changes.
   *
   * This fires when a bridge node reports a change in Internet connectivity status.
   * Useful for implementing failover logic or message queueing.
   *
   * \code
   * mesh.onBridgeStatusChanged([](auto bridgeNodeId, auto hasInternet) {
   *    if (hasInternet) {
   *      Serial.println("Internet available - sending queued data");
   *    } else {
   *      Serial.println("Internet offline - queueing messages");
   *    }
   * });
   * \endcode
   */
  void onBridgeStatusChanged(bridgeStatusChangedCallback_t onBridgeStatusChanged) {
    Log(logger::GENERAL, "onBridgeStatusChanged():\n");
    bridgeStatusChangedCallback = onBridgeStatusChanged;
  }

  /**
   * Check if any bridge/gateway in the mesh has Internet connectivity
   * 
   * IMPORTANT: This method checks if a GATEWAY node (bridge) in the mesh has
   * Internet access, NOT whether THIS node can directly make HTTP/HTTPS requests.
   * 
   * Regular mesh nodes do NOT have direct IP routing to the Internet - they only
   * communicate over the painlessMesh protocol. To send data to the Internet from
   * a regular node, you must either:
   * 
   * 1. Use sendToInternet() to route data through a gateway node
   * 2. Use initAsSharedGateway(meshSSID, meshPwd, ROUTER_SSID, ROUTER_PWD, scheduler, port)
   *    to give all nodes direct router access (requires router credentials)
   * 3. Send mesh messages to a bridge node that handles Internet communication
   * 
   * Use hasLocalInternet() to check if THIS specific node has direct Internet access.
   * 
   * When connected to mesh, requires recent bridge status (within timeout).
   * When disconnected from mesh, returns true if any bridge previously
   * reported Internet connectivity - allowing operations to be queued for
   * when mesh connectivity is restored.
   * 
   * \code
   * if (mesh.hasInternetConnection()) {
   *   // A gateway exists with Internet - use sendToInternet() to reach Internet
   *   mesh.sendToInternet("https://api.example.com/data", jsonPayload, callback);
   * }
   * 
   * // DON'T DO THIS - regular mesh nodes cannot make direct HTTP requests:
   * // HTTPClient http;
   * // http.begin("https://api.example.com");  // Will fail with "connection refused"
   * \endcode
   * 
   * @return true if at least one gateway/bridge reports Internet connection
   * @see hasLocalInternet() to check if THIS node has direct Internet access
   * @see sendToInternet() to send data to Internet via gateway
   * @see initAsSharedGateway() to give all nodes direct Internet access (requires router credentials)
   */
  bool hasInternetConnection() {
    bool hasConnections = hasActiveMeshConnections();
    
    for (const auto& bridge : knownBridges) {
      // When connected: require fresh status
      // When disconnected: use last known state
      bool isUsable = hasConnections 
          ? (bridge.isHealthy(bridgeTimeoutMs) && bridge.internetConnected)
          : bridge.internetConnected;
      
      if (isUsable) {
        return true;
      }
    }
    return false;
  }

  /**
   * Get list of nodes that currently have Internet access
   * 
   * Returns node IDs of all healthy bridges with active Internet connectivity.
   * Only bridges that have reported status within the timeout period are included.
   * 
   * \code
   * auto internetNodes = mesh.getNodesWithInternet();
   * for (auto nodeId : internetNodes) {
   *   Serial.printf("Node %u has Internet access\n", nodeId);
   * }
   * \endcode
   * 
   * @return Vector of node IDs with Internet connectivity
   */
  std::vector<uint32_t> getNodesWithInternet() {
    std::vector<uint32_t> nodesWithInternet;
    for (const auto& bridge : knownBridges) {
      if (bridge.isHealthy(bridgeTimeoutMs) && bridge.internetConnected) {
        nodesWithInternet.push_back(bridge.nodeId);
      }
    }
    return nodesWithInternet;
  }

  /**
   * Get list of all known bridges in the mesh
   * 
   * @return vector of BridgeInfo objects for all tracked bridges
   */
  std::vector<BridgeInfo> getBridges() {
    return knownBridges;
  }

  /**
   * Get the primary (best) bridge node
   * 
   * Primary bridge is selected based on:
   * 1. Must be healthy (seen within timeout) - unless node is disconnected
   * 2. Must have Internet connection
   * 3. Best WiFi RSSI to router
   * 
   * When this node is disconnected from the mesh (no active connections),
   * returns the last known bridge even if lastSeen is stale. This allows
   * nodes to attempt routing through the last known bridge once mesh
   * connectivity is restored.
   * 
   * @return pointer to BridgeInfo of primary bridge, or nullptr if no suitable bridge
   */
  BridgeInfo* getPrimaryBridge() {
    BridgeInfo* primary = nullptr;
    int8_t bestRSSI = -127;  // Worst possible RSSI
    
    // Check if we have active mesh connections
    bool hasConnections = hasActiveMeshConnections();
    
    for (auto& bridge : knownBridges) {
      // When connected to mesh: require healthy (recent) bridge status
      // When disconnected: use any bridge that reported Internet (stale info is better than none)
      bool isUsable = hasConnections 
          ? (bridge.isHealthy(bridgeTimeoutMs) && bridge.internetConnected)
          : bridge.internetConnected;
      
      if (isUsable) {
        if (bridge.routerRSSI > bestRSSI) {
          bestRSSI = bridge.routerRSSI;
          primary = &bridge;
        }
      }
    }
    
    return primary;
  }

  /**
   * Get the last known bridge with Internet connection (regardless of lastSeen timeout)
   * 
   * Unlike getPrimaryBridge(), this method returns the best known bridge even if
   * the lastSeen timestamp is stale. This is useful when the local node is temporarily
   * disconnected from the mesh and cannot receive bridge status updates.
   * 
   * The returned bridge may not be currently reachable, but represents the last
   * known good configuration.
   * 
   * @return pointer to BridgeInfo of last known bridge, or nullptr if no bridge ever seen
   */
  BridgeInfo* getLastKnownBridge() {
    BridgeInfo* best = nullptr;
    int8_t bestRSSI = -127;  // Worst possible RSSI
    
    for (auto& bridge : knownBridges) {
      // Include any bridge that reported Internet at some point
      if (bridge.internetConnected) {
        if (bridge.routerRSSI > bestRSSI) {
          bestRSSI = bridge.routerRSSI;
          best = &bridge;
        }
      }
    }
    
    return best;
  }

  /**
   * Check if this node has active mesh connections
   * 
   * Returns true if this node has at least one active connection to other
   * mesh nodes. When false, the node is isolated and cannot receive any
   * mesh messages including bridge status updates.
   * 
   * This is useful for distinguishing between:
   * - Bridge genuinely unavailable (mesh connected, but bridge not responding)
   * - Local node temporarily disconnected (cannot receive any mesh messages)
   * 
   * @return true if at least one mesh connection is active
   */
  bool hasActiveMeshConnections() {
    // Check if we have any active connections in our subs list
    for (auto& conn : this->subs) {
      if (conn && conn->connected()) {
        return true;
      }
    }
    return false;
  }

  /**
   * Check if this node is acting as a bridge
   * 
   * @return true if this node is configured as a root node (typically bridges)
   */
  bool isBridge() {
    return this->root;
  }

  // ==================== Gateway Status API ====================

  /**
   * Check if this node is the primary gateway
   * 
   * A node is the primary gateway if it's the bridge with the best RSSI
   * to the router and has Internet connectivity.
   * 
   * \code
   * if (mesh.isPrimaryGateway()) {
   *   Serial.println("I am the primary gateway!");
   * }
   * \endcode
   * 
   * @return true if this node is the primary gateway
   */
  bool isPrimaryGateway() {
    BridgeInfo* primary = getPrimaryBridge();
    if (primary == nullptr) {
      return false;
    }
    return primary->nodeId == this->nodeId;
  }

  /**
   * Get the node ID of the current primary gateway
   * 
   * Returns the node ID of the bridge with the best RSSI to the router
   * that has Internet connectivity, or 0 if no gateway is available.
   * 
   * \code
   * uint32_t gatewayId = mesh.getPrimaryGateway();
   * if (gatewayId != 0) {
   *   Serial.printf("Primary gateway is node %u\n", gatewayId);
   * } else {
   *   Serial.println("No gateway available");
   * }
   * \endcode
   * 
   * @return Node ID of the primary gateway, or 0 if none available
   */
  uint32_t getPrimaryGateway() {
    BridgeInfo* primary = getPrimaryBridge();
    if (primary == nullptr) {
      return 0;
    }
    return primary->nodeId;
  }

  /**
   * Get list of all nodes with Internet access
   * 
   * Returns node IDs of all healthy bridges with active Internet connectivity.
   * Only bridges that have reported status within the timeout period are included.
   * 
   * This method provides the same data as getNodesWithInternet() but returns
   * a std::list for consistency with other node list methods in painlessMesh.
   * 
   * \code
   * auto gateways = mesh.getGateways();
   * Serial.printf("Available gateways: %d\n", gateways.size());
   * for (auto gatewayId : gateways) {
   *   Serial.printf("  Gateway: %u\n", gatewayId);
   * }
   * \endcode
   * 
   * @return List of node IDs with Internet connectivity
   */
  std::list<uint32_t> getGateways() {
    // Convert vector to list for API consistency with getNodeList()
    // The copy overhead is acceptable since gateway lists are typically small
    auto nodes = getNodesWithInternet();
    return std::list<uint32_t>(nodes.begin(), nodes.end());
  }

  /**
   * Get count of available gateways
   * 
   * Returns the number of healthy bridges with active Internet connectivity.
   * 
   * \code
   * size_t count = mesh.getGatewayCount();
   * if (count == 0) {
   *   Serial.println("Warning: No gateways available!");
   * } else {
   *   Serial.printf("%d gateway(s) available\n", count);
   * }
   * \endcode
   * 
   * @return Number of available gateways
   */
  size_t getGatewayCount() {
    return getNodesWithInternet().size();
  }

  /**
   * Register callback for primary gateway changes
   * 
   * This callback fires when the primary gateway changes, either because
   * a new gateway becomes available, the current gateway becomes unavailable,
   * or a better gateway is discovered.
   * 
   * \code
   * mesh.onGatewayChanged([](uint32_t oldPrimary, uint32_t newPrimary) {
   *   if (newPrimary == 0) {
   *     Serial.println("Warning: Lost all gateways!");
   *   } else if (oldPrimary == 0) {
   *     Serial.printf("Gateway available: %u\n", newPrimary);
   *   } else {
   *     Serial.printf("Gateway changed: %u -> %u\n", oldPrimary, newPrimary);
   *   }
   * });
   * \endcode
   * 
   * @param callback Function to call when primary gateway changes
   */
  void onGatewayChanged(gatewayChangedCallback_t callback) {
    Log(logger::GENERAL, "onGatewayChanged():\n");
    gatewayChangedCallback = callback;
  }

  /**
   * Set the interval for bridge status broadcasts (bridge nodes only)
   * 
   * @param intervalMs Broadcast interval in milliseconds (default: 30000 = 30 seconds)
   */
  void setBridgeStatusInterval(uint32_t intervalMs) {
    bridgeStatusIntervalMs = intervalMs;
  }

  /**
   * Set the timeout for considering a bridge offline
   * 
   * @param timeoutMs Timeout in milliseconds (default: 60000 = 60 seconds)
   */
  void setBridgeTimeout(uint32_t timeoutMs) {
    bridgeTimeoutMs = timeoutMs;
  }

  /**
   * Enable or disable bridge status broadcasting (bridge nodes only)
   * 
   * @param enabled true to enable broadcasting (default), false to disable
   */
  void enableBridgeStatusBroadcast(bool enabled) {
    bridgeStatusBroadcastEnabled = enabled;
  }

  /**
   * Clean up expired bridge entries that haven't reported recently
   * 
   * Removes bridges that haven't sent a status update within the timeout period.
   * This helps maintain an accurate list of available gateways and prevents
   * the bridge list from growing unbounded.
   * 
   * Called periodically when bridge cleanup is enabled, or can be called manually.
   * 
   * \code
   * // Manual cleanup
   * mesh.cleanupExpiredBridges();
   * 
   * // Or enable automatic cleanup (recommended)
   * mesh.enableBridgeCleanup();
   * \endcode
   */
  void cleanupExpiredBridges() {
    using namespace logger;
    size_t sizeBefore = knownBridges.size();
    
    // Remove bridges that haven't reported within the timeout period
    knownBridges.erase(
        std::remove_if(knownBridges.begin(), knownBridges.end(),
                       [this](const BridgeInfo& bridge) {
                         return !bridge.isHealthy(bridgeTimeoutMs);
                       }),
        knownBridges.end());
    
    size_t removed = sizeBefore - knownBridges.size();
    if (removed > 0) {
      Log(GENERAL, "cleanupExpiredBridges(): Removed %u expired bridges\n", removed);
    }
  }

  /**
   * Enable periodic cleanup of expired bridge entries
   * 
   * Starts a background task that periodically removes stale bridge entries.
   * The cleanup runs at the same interval as the bridge timeout (default 60 seconds).
   * 
   * This is recommended for all nodes to prevent memory growth from
   * accumulating stale bridge entries.
   * 
   * \code
   * mesh.enableBridgeCleanup();  // Start automatic cleanup
   * \endcode
   */
  void enableBridgeCleanup() {
    using namespace logger;
    if (bridgeCleanupTask != nullptr) {
      Log(GENERAL, "enableBridgeCleanup(): Already enabled\n");
      return;
    }
    
    Log(GENERAL, "enableBridgeCleanup(): Starting cleanup task (interval: %u ms)\n",
        bridgeTimeoutMs);
    
    bridgeCleanupTask = this->addTask(
        bridgeTimeoutMs,
        TASK_FOREVER,
        [this]() {
          cleanupExpiredBridges();
        });
  }

  /**
   * Disable periodic cleanup of expired bridge entries
   */
  void disableBridgeCleanup() {
    using namespace logger;
    if (bridgeCleanupTask != nullptr) {
      bridgeCleanupTask->disable();
      bridgeCleanupTask = nullptr;
      Log(GENERAL, "disableBridgeCleanup(): Cleanup task disabled\n");
    }
  }

  /**
   * Check if bridge cleanup is enabled
   * 
   * @return true if periodic cleanup is running
   */
  bool isBridgeCleanupEnabled() const {
    return bridgeCleanupTask != nullptr && bridgeCleanupTask->isEnabled();
  }

  // ==================== Local Internet Health Check API ====================

  /**
   * Check if THIS node has direct local Internet access
   * 
   * This method checks whether this specific node can directly reach the Internet,
   * as opposed to hasInternetConnection() which checks if any bridge in the mesh
   * has Internet access.
   * 
   * For gateway/bridge nodes, this returns the result of the local health check.
   * For regular nodes, this typically returns false unless they have their own
   * Internet connection.
   * 
   * \code
   * if (mesh.hasLocalInternet()) {
   *   Serial.println("This node has direct Internet access");
   * }
   * \endcode
   * 
   * @return true if this node has verified local Internet connectivity
   */
  bool hasLocalInternet() {
    return internetHealthChecker.hasLocalInternet();
  }

  /**
   * Register callback for local Internet connectivity changes
   * 
   * This callback fires when THIS node's direct Internet connectivity changes,
   * as detected by the periodic health check.
   * 
   * \code
   * mesh.onLocalInternetChanged([](bool available) {
   *   if (available) {
   *     Serial.println("Local Internet connected");
   *   } else {
   *     Serial.println("Local Internet disconnected");
   *   }
   * });
   * \endcode
   * 
   * @param callback Function to call when local Internet status changes
   */
  void onLocalInternetChanged(localInternetChangedCallback_t callback) {
    Log(logger::GENERAL, "onLocalInternetChanged():\n");
    localInternetChangedCallback = callback;
    internetHealthChecker.onConnectivityChanged(callback);
  }

  /**
   * Get detailed status of local Internet connectivity
   * 
   * Returns comprehensive information about Internet connectivity checks
   * including success/failure counts, timing, and error messages.
   * 
   * \code
   * auto status = mesh.getInternetStatus();
   * Serial.printf("Internet %s, uptime: %d%%\n", 
   *               status.available ? "UP" : "DOWN",
   *               status.getUptimePercent());
   * \endcode
   * 
   * @return InternetStatus structure with detailed connectivity info
   */
  gateway::InternetStatus getInternetStatus() {
    return internetHealthChecker.getStatus();
  }

  /**
   * Configure Internet health checker with gateway settings
   * 
   * Sets up the health checker with parameters from SharedGatewayConfig.
   * This should be called before enabling gateway mode.
   * 
   * @param config Gateway configuration with check parameters
   */
  void configureInternetHealthCheck(const gateway::SharedGatewayConfig& config) {
    internetHealthChecker.setConfig(config);
  }

  /**
   * Set custom Internet check target
   * 
   * Override the default check host/port (8.8.8.8:53).
   * 
   * @param host Host to check (IP or hostname)
   * @param port Port to connect to (default 53 for DNS)
   */
  void setInternetCheckTarget(const TSTRING& host, uint16_t port = 53) {
    internetHealthChecker.setCheckTarget(host, port);
  }

  /**
   * Set Internet check interval
   * 
   * @param intervalMs Interval between checks in milliseconds
   */
  void setInternetCheckInterval(uint32_t intervalMs) {
    internetHealthChecker.setCheckInterval(intervalMs);
  }

  /**
   * Set Internet check timeout
   * 
   * @param timeoutMs Timeout for each check in milliseconds
   */
  void setInternetCheckTimeout(uint32_t timeoutMs) {
    internetHealthChecker.setCheckTimeout(timeoutMs);
  }

  /**
   * Perform an immediate Internet connectivity check
   * 
   * Bypasses the periodic check schedule and performs an immediate check.
   * Fires the onLocalInternetChanged callback if status changes.
   * 
   * @return true if Internet is reachable
   */
  bool checkInternetNow() {
    return internetHealthChecker.checkNow();
  }

  /**
   * Enable periodic Internet health checking
   * 
   * Starts a TaskScheduler task that periodically checks Internet connectivity.
   * The check interval is configured via setInternetCheckInterval() or
   * configureInternetHealthCheck().
   * 
   * \code
   * mesh.enableInternetHealthCheck();  // Use default 30 second interval
   * // or
   * mesh.setInternetCheckInterval(15000);  // 15 seconds
   * mesh.enableInternetHealthCheck();
   * \endcode
   */
  void enableInternetHealthCheck() {
    using namespace logger;
    if (internetHealthCheckTask != nullptr) {
      Log(GENERAL, "enableInternetHealthCheck(): Already enabled\n");
      return;
    }
    
    Log(GENERAL, "enableInternetHealthCheck(): Starting health check task (interval: %u ms)\n",
        internetHealthChecker.getCheckInterval());
    
    internetHealthCheckTask = this->addTask(
        internetHealthChecker.getCheckInterval(),
        TASK_FOREVER,
        [this]() {
          this->internetHealthChecker.checkNow();
        });
  }

  /**
   * Disable periodic Internet health checking
   */
  void disableInternetHealthCheck() {
    using namespace logger;
    if (internetHealthCheckTask != nullptr) {
      internetHealthCheckTask->disable();
      internetHealthCheckTask = nullptr;
      Log(GENERAL, "disableInternetHealthCheck(): Health check task disabled\n");
    }
  }

  /**
   * Check if Internet health checking is enabled
   * 
   * @return true if periodic health checks are running
   */
  bool isInternetHealthCheckEnabled() const {
    return internetHealthCheckTask != nullptr && internetHealthCheckTask->isEnabled();
  }

  /**
   * Reset Internet health check statistics
   */
  void resetInternetHealthStats() {
    internetHealthChecker.resetStats();
  }

#ifdef PAINLESSMESH_BOOST
  /**
   * Set mock Internet connectivity (test environment only)
   * 
   * @param connected Whether to simulate connected state
   */
  void setMockInternetConnected(bool connected) {
    internetHealthChecker.setMockConnected(connected);
  }
#endif

  // ==================== Send to Internet API ====================

  /**
   * Send data to an Internet destination through the mesh gateway
   *
   * This method provides a high-level API for sending data to the Internet
   * through the mesh network. It automatically handles:
   * - Local Internet: If this node has direct Internet access, sends directly
   * - Gateway routing: Otherwise routes through the best available gateway
   * - Acknowledgment tracking: Tracks message delivery with callbacks
   * - Retry logic: Implements exponential backoff for failed deliveries
   * - Priority handling: Respects message priority levels
   *
   * PRIORITY LEVELS:
   * - 0 (CRITICAL): Immediate processing, no delays
   * - 1 (HIGH): High priority, processed before normal
   * - 2 (NORMAL): Standard processing (default)
   * - 3 (LOW): Background processing when idle
   *
   * CALLBACK BEHAVIOR:
   * The callback is invoked when:
   * - Success: Gateway confirms delivery to Internet destination
   * - Failure: Timeout, no gateway available, or delivery error
   *
   * \code
   * // Send sensor data to cloud API
   * uint32_t msgId = mesh.sendToInternet(
   *   "https://api.example.com/sensor",
   *   "{\"temperature\": 23.5}",
   *   [](bool success, uint16_t httpStatus, String error) {
   *     if (success) {
   *       Serial.printf("Delivered! HTTP %u\n", httpStatus);
   *     } else {
   *       Serial.printf("Failed: %s\n", error.c_str());
   *     }
   *   },
   *   PRIORITY_NORMAL  // Optional priority
   * );
   * Serial.printf("Message ID: %u\n", msgId);
   * \endcode
   *
   * @param destination The Internet destination URL (e.g., "https://api.example.com/data")
   * @param payload The data payload to send (typically JSON)
   * @param callback Function called with result (success, httpStatus, error)
   * @param priority Message priority (0=CRITICAL to 3=LOW, default: 2=NORMAL)
   * @return Unique message ID for tracking, or 0 if immediate failure
   */
  uint32_t sendToInternet(
      TSTRING destination,
      TSTRING payload,
      internetResultCallback_t callback,
      uint8_t priority = static_cast<uint8_t>(gateway::GatewayPriority::PRIORITY_NORMAL)) {
    using namespace logger;

    // Generate unique message ID
    uint32_t messageId = gateway::GatewayDataPackage::generateMessageId(this->nodeId);

    Log(COMMUNICATION, "sendToInternet(): msgId=%u dest=%s priority=%u\n",
        messageId, destination.c_str(), priority);

    // Check if we have local Internet access
    // Note: Even with local Internet, we still use the gateway protocol for consistency.
    // A future optimization could bypass the mesh for nodes with direct Internet access.
    if (hasLocalInternet()) {
      Log(COMMUNICATION, "sendToInternet(): Local Internet available, using gateway protocol for consistency\n");
    }

    // Find the best gateway to route through
    BridgeInfo* gateway = getPrimaryBridge();
    if (gateway == nullptr) {
      Log(ERROR, "sendToInternet(): No gateway available\n");
      if (callback) {
        // Schedule callback to avoid blocking
        this->addTask([callback]() {
          callback(false, 0, "No gateway available");
        });
      }
      return 0;
    }

    // Create and store the pending request
    PendingInternetRequest request;
    request.messageId = messageId;
    request.timestamp = millis();
    request.retryCount = 0;
    request.maxRetries = internetRetryCount;
    request.priority = priority;
    request.timeoutMs = internetRequestTimeout;
    request.retryDelayMs = internetRetryDelay;
    request.gatewayNodeId = gateway->nodeId;
    request.destination = destination;
    request.payload = payload;
    request.callback = callback;

    // Store pending request
    pendingInternetRequests[messageId] = request;

    // Create gateway data package
    gateway::GatewayDataPackage pkg;
    pkg.from = this->nodeId;
    pkg.dest = gateway->nodeId;
    pkg.messageId = messageId;
    pkg.originNode = this->nodeId;
    pkg.timestamp = this->getNodeTime();
    pkg.priority = priority;
    pkg.destination = destination;
    pkg.payload = payload;
    pkg.contentType = "application/json";
    pkg.retryCount = 0;
    pkg.requiresAck = true;

    // Send the package with priority
    bool sent = false;
    auto conn = painlessmesh::router::findRoute<T>((*this), gateway->nodeId);
    if (conn) {
      sent = painlessmesh::router::sendWithPriority(pkg, conn, priority);
      if (!sent) {
        Log(ERROR, "sendToInternet(): sendWithPriority failed to gateway %u (send buffer full?)\n", gateway->nodeId);
      }
    } else {
      Log(ERROR, "sendToInternet(): No route to gateway %u\n", gateway->nodeId);
    }

    if (!sent) {
      Log(ERROR, "sendToInternet(): Failed to send to gateway %u, scheduling retry\n", gateway->nodeId);
      // Schedule retry or failure callback
      scheduleInternetRetry(messageId);
    } else {
      Log(COMMUNICATION, "sendToInternet(): Sent to gateway %u\n", gateway->nodeId);
      // Schedule timeout check
      scheduleInternetTimeout(messageId);
    }

    return messageId;
  }

  /**
   * Configure Internet request timeout
   *
   * @param timeoutMs Timeout in milliseconds (default: 30000)
   */
  void setInternetRequestTimeout(uint32_t timeoutMs) {
    internetRequestTimeout = timeoutMs;
  }

  /**
   * Configure Internet request retry count
   *
   * @param retryCount Number of retry attempts (default: 3)
   */
  void setInternetRetryCount(uint8_t retryCount) {
    internetRetryCount = retryCount;
  }

  /**
   * Configure base retry delay for exponential backoff
   *
   * @param delayMs Base delay in milliseconds (default: 1000)
   */
  void setInternetRetryDelay(uint32_t delayMs) {
    internetRetryDelay = delayMs;
  }

  /**
   * Get number of pending Internet requests
   *
   * @return Number of requests waiting for acknowledgment
   */
  size_t getPendingInternetRequestCount() const {
    return pendingInternetRequests.size();
  }

  /**
   * Cancel a pending Internet request
   *
   * @param messageId Message ID to cancel
   * @return true if request was found and cancelled
   */
  bool cancelInternetRequest(uint32_t messageId) {
    auto it = pendingInternetRequests.find(messageId);
    if (it != pendingInternetRequests.end()) {
      if (it->second.callback) {
        it->second.callback(false, 0, "Request cancelled");
      }
      pendingInternetRequests.erase(it);
      Log(logger::GENERAL, "cancelInternetRequest(): Cancelled msgId=%u\n", messageId);
      return true;
    }
    return false;
  }

  /**
   * Enable the sendToInternet() API
   *
   * This registers the handler for GatewayAckPackage (Type 621) and
   * starts the periodic cleanup of timed-out requests.
   *
   * Call this after mesh.init() to enable the sendToInternet() functionality.
   *
   * \code
   * mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
   * mesh.enableSendToInternet();  // Enable the API
   * \endcode
   */
  void enableSendToInternet() {
    using namespace logger;
    Log(GENERAL, "enableSendToInternet(): Enabling Internet routing API\n");

    // Register handler for gateway acknowledgment packages
    this->callbackList.onPackage(
        protocol::GATEWAY_ACK,
        [this](protocol::Variant& variant, std::shared_ptr<T>, uint32_t) {
          auto ack = variant.to<gateway::GatewayAckPackage>();
          this->handleGatewayAck(ack);
          return false;  // Don't consume, allow other handlers
        });

    // Start cleanup task for timed-out requests
    if (internetCleanupTask == nullptr) {
      internetCleanupTask = this->addTask(
          5000,  // Check every 5 seconds
          TASK_FOREVER,
          [this]() {
            this->cleanupTimedOutRequests();
          });
    }

    sendToInternetEnabled = true;
  }

  /**
   * Disable the sendToInternet() API
   */
  void disableSendToInternet() {
    using namespace logger;
    Log(GENERAL, "disableSendToInternet(): Disabling Internet routing API\n");

    if (internetCleanupTask != nullptr) {
      internetCleanupTask->disable();
      internetCleanupTask = nullptr;
    }

    // Cancel all pending requests
    for (auto& pair : pendingInternetRequests) {
      if (pair.second.callback) {
        pair.second.callback(false, 0, "API disabled");
      }
    }
    pendingInternetRequests.clear();

    sendToInternetEnabled = false;
  }

  /**
   * Check if sendToInternet() API is enabled
   *
   * @return true if the API is enabled
   */
  bool isSendToInternetEnabled() const {
    return sendToInternetEnabled;
  }

 private:
  /**
   * Handle incoming gateway acknowledgment package
   */
  void handleGatewayAck(const gateway::GatewayAckPackage& ack) {
    using namespace logger;
    Log(COMMUNICATION, "handleGatewayAck(): msgId=%u success=%d http=%u\n",
        ack.messageId, ack.success, ack.httpStatus);

    auto it = pendingInternetRequests.find(ack.messageId);
    if (it == pendingInternetRequests.end()) {
      Log(GENERAL, "handleGatewayAck(): Unknown message ID %u\n", ack.messageId);
      return;
    }

    PendingInternetRequest& request = it->second;

    // Call user callback
    if (request.callback) {
      request.callback(ack.success, ack.httpStatus, ack.error);
    }

    // Remove from pending
    pendingInternetRequests.erase(it);
  }

  /**
   * Schedule retry for a failed Internet request
   */
  void scheduleInternetRetry(uint32_t messageId) {
    auto it = pendingInternetRequests.find(messageId);
    if (it == pendingInternetRequests.end()) {
      return;
    }

    PendingInternetRequest& request = it->second;

    if (request.retryCount >= request.maxRetries) {
      // Max retries reached - fail the request
      Log(logger::ERROR, "scheduleInternetRetry(): Max retries reached for msgId=%u\n",
          messageId);
      if (request.callback) {
        request.callback(false, 0, "Max retries exceeded");
      }
      pendingInternetRequests.erase(it);
      return;
    }

    // Calculate exponential backoff delay
    uint32_t delay = request.retryDelayMs * (1 << request.retryCount);
    request.retryCount++;

    Log(logger::COMMUNICATION, "scheduleInternetRetry(): Retry %u for msgId=%u in %u ms\n",
        request.retryCount, messageId, delay);

    // Schedule retry
    this->addTask(delay, TASK_ONCE, [this, messageId]() {
      this->retryInternetRequest(messageId);
    });
  }

  /**
   * Retry sending an Internet request
   */
  void retryInternetRequest(uint32_t messageId) {
    auto it = pendingInternetRequests.find(messageId);
    if (it == pendingInternetRequests.end()) {
      return;
    }

    PendingInternetRequest& request = it->second;

    // Find gateway (may have changed)
    BridgeInfo* gateway = getPrimaryBridge();
    if (gateway == nullptr) {
      Log(logger::ERROR, "retryInternetRequest(): No gateway for retry msgId=%u\n",
          messageId);
      scheduleInternetRetry(messageId);
      return;
    }

    // Update gateway node ID (may have changed)
    request.gatewayNodeId = gateway->nodeId;

    // Create gateway data package
    gateway::GatewayDataPackage pkg;
    pkg.from = this->nodeId;
    pkg.dest = gateway->nodeId;
    pkg.messageId = messageId;
    pkg.originNode = this->nodeId;
    pkg.timestamp = this->getNodeTime();
    pkg.priority = request.priority;
    pkg.destination = request.destination;
    pkg.payload = request.payload;
    pkg.contentType = "application/json";
    pkg.retryCount = request.retryCount;
    pkg.requiresAck = true;

    // Send the package
    auto conn = painlessmesh::router::findRoute<T>((*this), gateway->nodeId);
    bool sent = false;
    if (conn) {
      sent = painlessmesh::router::sendWithPriority(pkg, conn, request.priority);
    }

    if (!sent) {
      Log(logger::ERROR, "retryInternetRequest(): Retry send failed msgId=%u\n", messageId);
      scheduleInternetRetry(messageId);
    } else {
      Log(logger::COMMUNICATION, "retryInternetRequest(): Retry sent msgId=%u to gateway %u\n",
          messageId, gateway->nodeId);
    }
  }

  /**
   * Schedule timeout check for an Internet request
   */
  void scheduleInternetTimeout(uint32_t messageId) {
    auto it = pendingInternetRequests.find(messageId);
    if (it == pendingInternetRequests.end()) {
      return;
    }

    uint32_t timeout = it->second.timeoutMs;

    this->addTask(timeout, TASK_ONCE, [this, messageId]() {
      this->checkInternetRequestTimeout(messageId);
    });
  }

  /**
   * Check if a specific request has timed out
   */
  void checkInternetRequestTimeout(uint32_t messageId) {
    auto it = pendingInternetRequests.find(messageId);
    if (it == pendingInternetRequests.end()) {
      return;  // Already handled
    }

    PendingInternetRequest& request = it->second;

    if (request.isTimedOut()) {
      Log(logger::ERROR, "checkInternetRequestTimeout(): Request timed out msgId=%u\n",
          messageId);
      if (request.callback) {
        request.callback(false, 0, "Request timed out");
      }
      pendingInternetRequests.erase(it);
    }
  }

  /**
   * Cleanup all timed-out Internet requests
   */
  void cleanupTimedOutRequests() {
    auto it = pendingInternetRequests.begin();
    while (it != pendingInternetRequests.end()) {
      if (it->second.isTimedOut()) {
        Log(logger::GENERAL, "cleanupTimedOutRequests(): Cleaning up msgId=%u\n",
            it->first);
        if (it->second.callback) {
          it->second.callback(false, 0, "Request timed out");
        }
        it = pendingInternetRequests.erase(it);
      } else {
        ++it;
      }
    }
  }

 public:
  /**
   * Update bridge information from received status package
   * Internal method called when bridge status is received
   * 
   * @param bridgeNodeId ID of the bridge node
   * @param internetConnected Internet connectivity status
   * @param routerRSSI Router signal strength
   * @param routerChannel Router WiFi channel
   * @param uptime Bridge uptime
   * @param gatewayIP Router gateway IP
   * @param timestamp Status timestamp
   */
  void updateBridgeStatus(uint32_t bridgeNodeId, bool internetConnected, 
                         int8_t routerRSSI, uint8_t routerChannel,
                         uint32_t uptime, TSTRING gatewayIP, uint32_t timestamp) {
    // Find existing bridge or add new one
    BridgeInfo* bridge = nullptr;
    uint32_t oldPrimaryBridgeId = 0;
    auto oldPrimary = this->getPrimaryBridge();
    if (oldPrimary != nullptr) {
      oldPrimaryBridgeId = oldPrimary->nodeId;
    }
    
    for (auto& b : knownBridges) {
      if (b.nodeId == bridgeNodeId) {
        bridge = &b;
        break;
      }
    }
    
    bool wasConnected = false;
    bool isNewBridge = false;
    if (bridge == nullptr) {
      // New bridge - enforce MAX_KNOWN_BRIDGES limit before adding
      if (knownBridges.size() >= MAX_KNOWN_BRIDGES) {
        // First try to remove expired bridges
        cleanupExpiredBridges();
        
        // If still at capacity, remove the bridge with worst RSSI (lowest signal)
        if (knownBridges.size() >= MAX_KNOWN_BRIDGES && !knownBridges.empty()) {
          auto worstBridge = knownBridges.begin();
          int8_t worstRSSI = worstBridge->routerRSSI;
          
          for (auto it = knownBridges.begin(); it != knownBridges.end(); ++it) {
            if (it->routerRSSI < worstRSSI) {
              worstRSSI = it->routerRSSI;
              worstBridge = it;
            }
          }
          
          Log(logger::GENERAL, "updateBridgeStatus(): Removing bridge %u (RSSI: %d) to make room\n",
              worstBridge->nodeId, worstBridge->routerRSSI);
          knownBridges.erase(worstBridge);
        }
      }
      
      // Add new bridge
      BridgeInfo newBridge;
      newBridge.nodeId = bridgeNodeId;
      knownBridges.push_back(newBridge);
      bridge = &knownBridges.back();
      isNewBridge = true;
    } else {
      wasConnected = bridge->internetConnected;
    }
    
    // Update bridge info
    bridge->internetConnected = internetConnected;
    bridge->routerRSSI = routerRSSI;
    bridge->routerChannel = routerChannel;
    bridge->lastSeen = millis();
    bridge->uptime = uptime;
    bridge->gatewayIP = gatewayIP;
    bridge->timestamp = timestamp;
    
    // Check if primary bridge changed
    auto newPrimary = this->getPrimaryBridge();
    uint32_t newPrimaryBridgeId = (newPrimary != nullptr) ? newPrimary->nodeId : 0;
    
    // Trigger gateway changed callback if primary gateway changed
    if (oldPrimaryBridgeId != newPrimaryBridgeId && gatewayChangedCallback) {
      gatewayChangedCallback(oldPrimaryBridgeId, newPrimaryBridgeId);
    }
    
    if (diagnosticsEnabled && oldPrimaryBridgeId != newPrimaryBridgeId) {
      // Record bridge change
      lastBridgeChange.timestamp = millis();
      lastBridgeChange.oldBridgeId = oldPrimaryBridgeId;
      lastBridgeChange.newBridgeId = newPrimaryBridgeId;
      lastBridgeChange.internetAvailable = internetConnected;
      if (isNewBridge) {
        lastBridgeChange.reason = "New bridge discovered";
      } else if (internetConnected && !wasConnected) {
        lastBridgeChange.reason = "Bridge Internet restored";
      } else if (!internetConnected && wasConnected) {
        lastBridgeChange.reason = "Bridge Internet lost";
      } else {
        lastBridgeChange.reason = "Primary bridge changed";
      }
      
      lastBridgeChangeTime = millis();
      Log(logger::GENERAL, "updateBridgeStatus(): Bridge change recorded\n");
    }
    
    // Trigger callback if status changed
    if (wasConnected != internetConnected && bridgeStatusChangedCallback) {
      bridgeStatusChangedCallback(bridgeNodeId, internetConnected);
    }
  }

  /**
   * Enable RTC integration for offline timekeeping
   * 
   * Allows nodes to maintain accurate timestamps even when Internet/bridge
   * is unavailable. User must provide an implementation of RTCInterface
   * for their specific RTC hardware.
   * 
   * \code
   * // Example with DS3231 RTC
   * class MyRTC : public painlessmesh::rtc::RTCInterface {
   *   // ... implement interface methods ...
   * };
   * MyRTC myRTC;
   * mesh.enableRTC(&myRTC);
   * \endcode
   * 
   * @param rtcInterface Pointer to user's RTC implementation
   * @return true if RTC enabled successfully, false otherwise
   */
  bool enableRTC(rtc::RTCInterface* rtcInterface) {
    using namespace logger;
    Log(GENERAL, "enableRTC(): Initializing RTC\n");
    bool success = rtcManager.enable(rtcInterface);
    if (success) {
      // RTC enabled successfully - mark node as having time authority
      setTimeAuthority(true);
    }
    return success;
  }

  /**
   * Disable RTC integration
   */
  void disableRTC() {
    using namespace logger;
    Log(GENERAL, "disableRTC(): Disabling RTC\n");
    rtcManager.disable();
    // RTC disabled - remove time authority if no other source
    // Note: Bridge nodes may still have time authority from Internet
    setTimeAuthority(false);
  }

  /**
   * Sync RTC from NTP/Internet time source
   * 
   * Should be called when Internet connection is available to update
   * the RTC with accurate time. Typically called in onBridgeStatusChanged
   * callback when Internet becomes available.
   * 
   * \code
   * mesh.onBridgeStatusChanged([](auto bridgeNodeId, auto hasInternet) {
   *   if (hasInternet) {
   *     // Get NTP time and sync RTC
   *     uint32_t ntpTime = getNTPTime();  // User implements this
   *     if (mesh.syncRTCFromNTP(ntpTime)) {
   *       Serial.println("RTC synced successfully");
   *     }
   *   }
   * });
   * \endcode
   * 
   * @param ntpTimestamp Unix timestamp from NTP source
   * @return true if sync successful, false otherwise
   */
  bool syncRTCFromNTP(uint32_t ntpTimestamp) {
    using namespace logger;
    Log(GENERAL, "syncRTCFromNTP(): Syncing RTC to timestamp %u\n", ntpTimestamp);
    
    if (!rtcManager.isEnabled()) {
      Log(ERROR, "syncRTCFromNTP(): RTC not enabled\n");
      return false;
    }
    
    bool success = rtcManager.syncFromNTP(ntpTimestamp);
    
    if (success && rtcSyncCompleteCallback) {
      rtcSyncCompleteCallback(ntpTimestamp);
    }
    
    return success;
  }

  /**
   * Get accurate time with RTC fallback
   * 
   * Returns time from RTC if available, otherwise falls back to mesh time.
   * This provides the most accurate timestamp available to the node.
   * 
   * @return Unix timestamp in seconds, or mesh time in microseconds if RTC unavailable
   */
  uint32_t getAccurateTime() {
    if (rtcManager.isEnabled()) {
      uint32_t rtcTime = rtcManager.getTime();
      if (rtcTime > 0) {
        return rtcTime;
      }
    }
    // Fallback to mesh time (microseconds)
    return this->getNodeTime();
  }

  /**
   * Check if RTC is enabled and available
   * 
   * @return true if RTC can be used, false otherwise
   */
  bool hasRTC() const {
    return rtcManager.isEnabled();
  }

  /**
   * Get RTC type
   * 
   * @return RTCType enum value, or RTC_NONE if no RTC enabled
   */
  rtc::RTCType getRTCType() const {
    return rtcManager.getType();
  }

  /**
   * Get time since last RTC sync
   * 
   * @return Milliseconds since last RTC sync, or 0 if never synced
   */
  uint32_t getTimeSinceRTCSync() const {
    return rtcManager.getTimeSinceLastSync();
  }

  /**
   * Callback when RTC sync completes
   * 
   * This fires when syncRTCFromNTP() successfully updates the RTC.
   * 
   * \code
   * mesh.onRTCSyncComplete([](auto timestamp) {
   *   Serial.printf("RTC synced to: %u\n", timestamp);
   * });
   * \endcode
   */
  void onRTCSyncComplete(rtcSyncCompleteCallback_t onRTCSyncComplete) {
    Log(logger::GENERAL, "onRTCSyncComplete():\n");
    rtcSyncCompleteCallback = onRTCSyncComplete;
  }

  /**
   * Set time authority status for this node
   * 
   * Nodes with time authority (RTC or Internet) are preferred as time sources
   * during mesh time synchronization. This prevents nodes from adopting time
   * from nodes without accurate time sources.
   * 
   * This is automatically set to true when:
   * - RTC is enabled via enableRTC()
   * - Bridge has Internet connectivity
   * 
   * \code
   * // Manual control (advanced usage)
   * mesh.setTimeAuthority(true);  // Mark as authoritative time source
   * mesh.setTimeAuthority(false); // Mark as non-authoritative
   * \endcode
   * 
   * @param hasAuthority True if node has accurate time source (RTC/Internet)
   */
  void setTimeAuthority(bool hasAuthority) {
    using namespace logger;
    if (this->hasTimeAuthority != hasAuthority) {
      this->hasTimeAuthority = hasAuthority;
      Log(GENERAL, "setTimeAuthority(): Time authority %s\n", 
          hasAuthority ? "enabled" : "disabled");
      
      // Trigger time sync with all connections to propagate authority status
      for (auto&& connection : this->subs) {
        if (connection->nodeId != 0) {
          connection->timeSyncTask.forceNextIteration();
        }
      }
    }
  }

  /**
   * Check if this node has time authority
   * 
   * @return True if node has RTC or Internet time source
   */
  bool getTimeAuthority() const {
    return this->hasTimeAuthority;
  }

  //
  // Message Queue API
  //
  
  /**
   * Enable or disable message queueing for offline mode
   * 
   * When enabled, messages can be queued when Internet is unavailable
   * and automatically flushed when connection is restored.
   * 
   * @param enabled True to enable queueing, false to disable
   * @param maxSize Maximum number of messages in queue (default 1000)
   * 
   * \code
   * mesh.enableMessageQueue(true, 500);  // Enable with 500 message capacity
   * \endcode
   */
  void enableMessageQueue(bool enabled, uint32_t maxSize = 1000) {
    if (enabled && !messageQueue) {
      messageQueue = new MessageQueue(maxSize);
      Log(logger::GENERAL, "enableMessageQueue(): Queue enabled with capacity %u\n", maxSize);
    } else if (!enabled && messageQueue) {
      delete messageQueue;
      messageQueue = nullptr;
      Log(logger::GENERAL, "enableMessageQueue(): Queue disabled\n");
    }
  }
  
  /**
   * Queue a message with priority for later delivery
   * 
   * Use this to queue critical messages when Internet is unavailable.
   * Messages are automatically delivered when connection is restored.
   * 
   * @param payload Message content to queue
   * @param destination Optional destination metadata (e.g., MQTT topic, HTTP endpoint)
   * @param priority Message priority (default: PRIORITY_NORMAL)
   * @return Message ID if successfully queued, 0 if failed
   * 
   * \code
   * // Queue critical alarm
   * uint32_t msgId = mesh.queueMessage(
   *   alarmData.toJSON(),
   *   "mqtt://cloud.example.com/alarms",
   *   PRIORITY_CRITICAL
   * );
   * \endcode
   */
  uint32_t queueMessage(const TSTRING& payload, 
                       const TSTRING& destination = "",
                       MessagePriority priority = PRIORITY_NORMAL) {
    if (!messageQueue) {
      Log(logger::ERROR, "queueMessage(): Message queue not enabled\n");
      return 0;
    }
    
    return messageQueue->enqueue(priority, payload, destination);
  }
  
  /**
   * Flush all queued messages
   * 
   * Attempts to send all queued messages. This is typically called
   * automatically when Internet connection is restored, but can be
   * called manually.
   * 
   * Note: This returns the messages for the application to send.
   * The application is responsible for actually transmitting them
   * and calling removeQueuedMessage() when successful.
   * 
   * @return Vector of queued messages to send
   * 
   * \code
   * auto messages = mesh.flushMessageQueue();
   * for (auto& msg : messages) {
   *   if (sendToCloud(msg.payload, msg.destination)) {
   *     mesh.removeQueuedMessage(msg.id);
   *   }
   * }
   * \endcode
   */
  std::vector<QueuedMessage> flushMessageQueue() {
    if (!messageQueue) {
      return std::vector<QueuedMessage>();
    }
    
    return messageQueue->getMessages();
  }
  
  /**
   * Remove a successfully sent message from the queue
   * 
   * @param messageId ID of message to remove
   * @return true if message was found and removed
   */
  bool removeQueuedMessage(uint32_t messageId) {
    if (!messageQueue) {
      return false;
    }
    
    return messageQueue->remove(messageId);
  }
  
  /**
   * Increment send attempt counter for a message
   * 
   * @param messageId ID of message
   * @return New attempt count, or 0 if message not found
   */
  uint32_t incrementQueuedMessageAttempts(uint32_t messageId) {
    if (!messageQueue) {
      return 0;
    }
    
    return messageQueue->incrementAttempts(messageId);
  }
  
  /**
   * Get number of queued messages
   * 
   * @param priority Optional priority level to count (counts all if not specified)
   * @return Number of queued messages
   */
  uint32_t getQueuedMessageCount(MessagePriority priority) {
    if (!messageQueue) {
      return 0;
    }
    
    return messageQueue->size(priority);
  }
  
  uint32_t getQueuedMessageCount() {
    if (!messageQueue) {
      return 0;
    }
    
    return messageQueue->size();
  }
  
  /**
   * Get queue statistics
   * 
   * @return QueueStats structure with detailed statistics
   */
  QueueStats getQueueStats() {
    if (!messageQueue) {
      return QueueStats();
    }
    
    return messageQueue->getStats();
  }
  
  /**
   * Set callback for queue state changes
   * 
   * Fires when queue state changes (EMPTY, NORMAL, 75%, FULL)
   * 
   * \code
   * mesh.onQueueStateChanged([](QueueState state, uint32_t count) {
   *   if (state == QUEUE_75_PERCENT) {
   *     Serial.printf("Warning: Queue 75%% full (%u messages)\n", count);
   *   }
   * });
   * \endcode
   */
  void onQueueStateChanged(queueStateChangedCallback_t callback) {
    if (messageQueue) {
      messageQueue->onStateChanged(callback);
    }
  }
  
  /**
   * Prune old messages from the queue
   * 
   * @param maxAgeMs Maximum age in milliseconds
   * @return Number of messages removed
   */
  uint32_t pruneQueue(uint32_t maxAgeMs) {
    if (!messageQueue) {
      return 0;
    }
    
    return messageQueue->pruneOldMessages(maxAgeMs);
  }
  
  /**
   * Clear all messages from the queue
   */
  void clearQueue() {
    if (messageQueue) {
      messageQueue->clear();
    }
  }

  /**
   * Are we connected/know a route to the given node?
   *
   * @param nodeId The nodeId we are looking for
   */
  bool isConnected(uint32_t nodeId) {
    return painlessmesh::router::findRoute<T>((*this), nodeId) != NULL;
  }

  /** Get a list of all known nodes.
   *
   * This includes nodes that are both directly and indirectly connected to the
   * current node.
   */
  std::list<uint32_t> getNodeList(bool includeSelf = false) {
    return painlessmesh::layout::asList(this->asNodeTree(), includeSelf);
  }

  /**
   * Return a json representation of the current mesh layout
   */
  inline TSTRING subConnectionJson(bool pretty = false) {
    return this->asNodeTree().toString(pretty);
  }

  /**
   * Structure to hold detailed connection information
   */
  struct ConnectionInfo {
    uint32_t nodeId;           // Connected node ID
    uint32_t lastSeen;         // Timestamp of last message (ms)
    int rssi;                  // Signal strength (dBm)
    int avgLatency;            // Average round-trip time (ms)
    int hopCount;              // Hops from current node
    int quality;               // Connection quality (0-100)
    uint32_t messagesRx;       // Messages received
    uint32_t messagesTx;       // Messages sent
    uint32_t messagesDropped;  // Failed transmissions
  };

  /**
   * Get detailed connection information for all direct neighbors
   */
  std::vector<ConnectionInfo> getConnectionDetails() {
    std::vector<ConnectionInfo> connections;
    
    for (auto conn : this->subs) {
      if (conn->connected()) {
        ConnectionInfo info;
        info.nodeId = conn->nodeId;
        info.lastSeen = conn->timeLastReceived;
        info.rssi = conn->getRSSI();
        info.avgLatency = conn->getLatency();
        info.hopCount = 1;  // Direct connection
        info.quality = conn->getQuality();
        info.messagesRx = conn->messagesRx;
        info.messagesTx = conn->messagesTx;
        info.messagesDropped = conn->messagesDropped;
        
        connections.push_back(info);
      }
    }
    
    return connections;
  }

  /**
   * Get hop count to specific node
   * Returns -1 if node is unreachable
   */
  int getHopCount(uint32_t nodeId) {
    // Check if requesting hop count to self
    if (nodeId == this->getNodeId()) {
      return 0;
    }
    
    // Check if it's a direct connection (1 hop)
    for (auto conn : this->subs) {
      if (conn->nodeId == nodeId && conn->connected()) {
        return 1;
      }
    }
    
    // For indirect connections, use BFS to find shortest path
    auto tree = this->asNodeTree();
    
    // BFS to find hop count
    std::queue<std::pair<uint32_t, uint8_t>> queue;  // (nodeId, hops)
    std::set<uint32_t> visited;
    
    // Start from this node
    queue.push({this->getNodeId(), 0});
    visited.insert(this->getNodeId());
    
    while (!queue.empty()) {
      auto current = queue.front();
      queue.pop();
      
      uint32_t currentNode = current.first;
      uint8_t hops = current.second;
      
      // Found the target
      if (currentNode == nodeId) {
        return hops;
      }
      
      // Get neighbors of current node
      std::list<uint32_t> neighbors;
      if (getNodeNeighbors(tree, currentNode, neighbors)) {
        // Add unvisited neighbors to queue
        for (auto neighbor : neighbors) {
          if (visited.find(neighbor) == visited.end()) {
            visited.insert(neighbor);
            queue.push({neighbor, static_cast<uint8_t>(hops + 1)});
          }
        }
      }
    }
    
    return -1;  // Node not found or unreachable
  }

  /**
   * Get routing table as map (destination -> next hop)
   * 
   * Uses BFS to build a complete routing table with next-hop information
   * for all reachable nodes in the mesh. For direct connections, the next
   * hop is the node itself. For multi-hop paths, the next hop is the first
   * node on the shortest path to the destination.
   */
  std::map<uint32_t, uint32_t> getRoutingTable() {
    std::map<uint32_t, uint32_t> table;
    auto tree = this->asNodeTree();
    
    // BFS from this node to build routing table
    std::queue<uint32_t> queue;
    std::set<uint32_t> visited;
    std::map<uint32_t, uint32_t> nextHop;  // dest -> next hop from source
    
    // Start from this node
    queue.push(this->getNodeId());
    visited.insert(this->getNodeId());
    
    while (!queue.empty()) {
      uint32_t current = queue.front();
      queue.pop();
      
      // Get neighbors of current node
      std::list<uint32_t> neighbors;
      if (getNodeNeighbors(tree, current, neighbors)) {
        for (auto neighbor : neighbors) {
          if (visited.find(neighbor) == visited.end()) {
            visited.insert(neighbor);
            queue.push(neighbor);
            
            // Determine next hop for this neighbor
            if (current == this->getNodeId()) {
              // Direct connection - next hop is the neighbor itself
              nextHop[neighbor] = neighbor;
            } else {
              // Multi-hop - inherit parent's next hop
              nextHop[neighbor] = nextHop[current];
            }
            
            // Add to routing table
            table[neighbor] = nextHop[neighbor];
          }
        }
      }
    }
    
    return table;
  }

  /**
   * Get complete path from this node to target node
   * 
   * Returns a vector containing the complete path of node IDs from this node
   * to the target node, including both endpoints. Uses BFS to find the shortest
   * path. Returns an empty vector if the target is unreachable.
   * 
   * \param nodeId The target node ID
   * \return Vector of node IDs representing the path (empty if unreachable)
   * 
   * \code
   * auto path = mesh.getPathToNode(targetNodeId);
   * if (!path.empty()) {
   *   Serial.printf("Path length: %d hops\n", path.size() - 1);
   *   for (auto node : path) {
   *     Serial.printf("%u -> ", node);
   *   }
   * }
   * \endcode
   */
  std::vector<uint32_t> getPathToNode(uint32_t nodeId) {
    std::vector<uint32_t> path;
    
    // Check if requesting path to self
    if (nodeId == this->getNodeId()) {
      path.push_back(this->getNodeId());
      return path;
    }
    
    auto tree = this->asNodeTree();
    
    // BFS to find path
    std::queue<uint32_t> queue;
    std::set<uint32_t> visited;
    std::map<uint32_t, uint32_t> parent;  // child -> parent mapping
    
    // Start from this node
    queue.push(this->getNodeId());
    visited.insert(this->getNodeId());
    parent[this->getNodeId()] = 0;  // Root has no parent
    
    bool found = false;
    while (!queue.empty() && !found) {
      uint32_t current = queue.front();
      queue.pop();
      
      // Check if we reached the target
      if (current == nodeId) {
        found = true;
        break;
      }
      
      // Get neighbors of current node
      std::list<uint32_t> neighbors;
      if (getNodeNeighbors(tree, current, neighbors)) {
        for (auto neighbor : neighbors) {
          if (visited.find(neighbor) == visited.end()) {
            visited.insert(neighbor);
            parent[neighbor] = current;
            queue.push(neighbor);
          }
        }
      }
    }
    
    // Reconstruct path from parent map
    if (found) {
      uint32_t current = nodeId;
      while (current != 0) {
        path.insert(path.begin(), current);
        current = parent[current];
      }
    }
    
    return path;
  }

  /**
   * Get bridge health metrics
   * 
   * Collects comprehensive health metrics including connectivity, signal quality,
   * traffic, and performance data. Useful for monitoring and troubleshooting.
   * 
   * \code
   * auto metrics = mesh.getBridgeHealthMetrics();
   * Serial.printf("Uptime: %u s, Messages RX: %u, Avg Latency: %u ms\n",
   *               metrics.uptimeSeconds, metrics.messagesRx, metrics.avgLatencyMs);
   * \endcode
   * 
   * @return BridgeHealthMetrics structure with current metrics
   */
  BridgeHealthMetrics getBridgeHealthMetrics() {
    BridgeHealthMetrics metrics;
    
    // Connectivity metrics
    metrics.uptimeSeconds = millis() / 1000;
    metrics.currentUptime = millis();
    
    // Check for primary bridge with Internet connection
    auto primaryBridge = getPrimaryBridge();
    if (primaryBridge != nullptr) {
      metrics.internetUptimeSeconds = (millis() - primaryBridge->lastSeen) / 1000;
      metrics.currentRSSI = primaryBridge->routerRSSI;
    }
    
    // Aggregate traffic and performance from all connections
    uint64_t totalBytesRx = 0;
    uint64_t totalBytesTx = 0;
    uint32_t totalMessagesRx = 0;
    uint32_t totalMessagesTx = 0;
    uint32_t totalMessagesDropped = 0;
    uint32_t totalLatency = 0;
    uint32_t latencySampleCount = 0;
    int8_t sumRSSI = 0;
    int8_t minRSSI = 0;
    int8_t maxRSSI = -127;
    uint32_t rssiCount = 0;
    
    for (auto conn : this->subs) {
      if (conn->connected()) {
        totalBytesRx += conn->bytesRx;
        totalBytesTx += conn->bytesTx;
        totalMessagesRx += conn->messagesRx;
        totalMessagesTx += conn->messagesTx;
        totalMessagesDropped += conn->messagesDropped;
        
        // Latency
        int latency = conn->getLatency();
        if (latency >= 0) {
          totalLatency += latency;
          latencySampleCount++;
        }
        
        // RSSI
        int rssi = conn->getRSSI();
        if (rssi != 0) {
          sumRSSI += rssi;
          rssiCount++;
          if (rssi < minRSSI || minRSSI == 0) minRSSI = rssi;
          if (rssi > maxRSSI) maxRSSI = rssi;
        }
      }
    }
    
    // Set traffic metrics
    metrics.bytesRx = totalBytesRx;
    metrics.bytesTx = totalBytesTx;
    metrics.messagesRx = totalMessagesRx;
    metrics.messagesTx = totalMessagesTx;
    metrics.messagesDropped = totalMessagesDropped;
    
    // Calculate averages
    if (latencySampleCount > 0) {
      metrics.avgLatencyMs = totalLatency / latencySampleCount;
    }
    
    if (rssiCount > 0) {
      metrics.avgRSSI = sumRSSI / rssiCount;
      metrics.minRSSI = minRSSI;
      metrics.maxRSSI = maxRSSI;
    }
    
    // Calculate packet loss percentage
    uint32_t totalAttempted = totalMessagesTx + totalMessagesDropped;
    if (totalAttempted > 0) {
      metrics.packetLossPercent = (totalMessagesDropped * 100) / totalAttempted;
    }
    
    // Mesh node count
    metrics.meshNodeCount = this->getNodeList(true).size();
    
    // Track disconnects (stored in protected member)
    metrics.totalDisconnects = metricsDisconnectCount;
    
    return metrics;
  }

  /**
   * Reset health metrics counters
   * 
   * Resets all counters to zero but keeps current state metrics (RSSI, node count, etc.)
   * Useful for periodic monitoring windows.
   * 
   * \code
   * // Reset metrics at the start of each monitoring period
   * mesh.resetHealthMetrics();
   * \endcode
   */
  void resetHealthMetrics() {
    using namespace logger;
    Log(GENERAL, "resetHealthMetrics(): Resetting all metric counters\n");
    
    // Reset connection metrics
    for (auto conn : this->subs) {
      if (conn->connected()) {
        conn->messagesRx = 0;
        conn->messagesTx = 0;
        conn->messagesDropped = 0;
        conn->bytesRx = 0;
        conn->bytesTx = 0;
        conn->latencySamples.clear();
      }
    }
    
    // Reset disconnect counter
    metricsDisconnectCount = 0;
  }

  /**
   * Export health metrics as JSON string
   * 
   * Generates a JSON representation of current health metrics for easy integration
   * with monitoring tools, MQTT publishing, or logging.
   * 
   * \code
   * String json = mesh.getHealthMetricsJSON();
   * mqttClient.publish("bridge/metrics", json.c_str());
   * \endcode
   * 
   * @return JSON string containing all health metrics
   */
  TSTRING getHealthMetricsJSON() {
    auto metrics = getBridgeHealthMetrics();
    
    TSTRING json = "{";
    json += "\"connectivity\":{";
    json += "\"uptimeSeconds\":";
    json += std::to_string(metrics.uptimeSeconds);
    json += ",\"internetUptimeSeconds\":";
    json += std::to_string(metrics.internetUptimeSeconds);
    json += ",\"totalDisconnects\":";
    json += std::to_string(metrics.totalDisconnects);
    json += ",\"currentUptime\":";
    json += std::to_string(metrics.currentUptime);
    json += "},";
    
    json += "\"signalQuality\":{";
    json += "\"currentRSSI\":";
    json += std::to_string(metrics.currentRSSI);
    json += ",\"avgRSSI\":";
    json += std::to_string(metrics.avgRSSI);
    json += ",\"minRSSI\":";
    json += std::to_string(metrics.minRSSI);
    json += ",\"maxRSSI\":";
    json += std::to_string(metrics.maxRSSI);
    json += "},";
    
    json += "\"traffic\":{";
    json += "\"bytesRx\":";
    json += std::to_string(metrics.bytesRx);
    json += ",\"bytesTx\":";
    json += std::to_string(metrics.bytesTx);
    json += ",\"messagesRx\":";
    json += std::to_string(metrics.messagesRx);
    json += ",\"messagesTx\":";
    json += std::to_string(metrics.messagesTx);
    json += ",\"messagesQueued\":";
    json += std::to_string(metrics.messagesQueued);
    json += ",\"messagesDropped\":";
    json += std::to_string(metrics.messagesDropped);
    json += "},";
    
    json += "\"performance\":{";
    json += "\"avgLatencyMs\":";
    json += std::to_string(metrics.avgLatencyMs);
    json += ",\"packetLossPercent\":";
    json += std::to_string(metrics.packetLossPercent);
    json += ",\"meshNodeCount\":";
    json += std::to_string(metrics.meshNodeCount);
    json += "}";
    
    json += "}";
    return json;
  }

  /**
   * Set periodic health metrics callback
   * 
   * Registers a callback that will be invoked periodically with current health metrics.
   * Useful for automated monitoring, MQTT publishing, or Prometheus export.
   * 
   * \code
   * mesh.onHealthMetricsUpdate([](BridgeHealthMetrics metrics) {
   *   String json = mesh.getHealthMetricsJSON();
   *   mqttClient.publish("bridge/metrics", json.c_str());
   * }, 60000);  // Every 60 seconds
   * \endcode
   * 
   * @param callback Function to call with metrics
   * @param intervalMs Callback interval in milliseconds (default: 60000 = 60 seconds)
   */
  void onHealthMetricsUpdate(std::function<void(BridgeHealthMetrics)> callback, 
                             uint32_t intervalMs = 60000) {
    using namespace logger;
    Log(GENERAL, "onHealthMetricsUpdate(): Setting up periodic callback every %u ms\n", intervalMs);
    
    // Create a task that periodically collects and reports metrics
    auto metricsTask = this->addTask(intervalMs, TASK_FOREVER, [this, callback]() {
      auto metrics = this->getBridgeHealthMetrics();
      callback(metrics);
    });
    
    metricsTask->enable();
  }

  // ==================== Enhanced Diagnostics API ====================

  /**
   * Enable or disable diagnostics collection
   * 
   * When enabled, the mesh will track election history, bridge changes,
   * and other diagnostic information. This has minimal overhead.
   * 
   * @param enabled true to enable diagnostics (default), false to disable
   */
  void enableDiagnostics(bool enabled = true) {
    diagnosticsEnabled = enabled;
    if (enabled) {
      Log(logger::GENERAL, "enableDiagnostics(): Diagnostics enabled\n");
    } else {
      Log(logger::GENERAL, "enableDiagnostics(): Diagnostics disabled\n");
    }
  }

  /**
   * Get current bridge status
   * 
   * Returns information about this node's bridge role and connectivity status
   * 
   * \code
   * auto status = mesh.getBridgeStatus();
   * if (status.isBridge && status.internetConnected) {
   *   Serial.println("I am bridge with Internet");
   * }
   * \endcode
   * 
   * @return BridgeStatus structure with current status
   */
  BridgeStatus getBridgeStatus() {
    BridgeStatus status;
    
    status.isBridge = this->isBridge();
    status.internetConnected = this->hasInternetConnection();
    
    if (status.isBridge) {
      status.role = "bridge";
    } else if (this->root) {
      status.role = "root";
    } else {
      status.role = "regular";
    }
    
    // Get primary bridge info
    auto primaryBridge = this->getPrimaryBridge();
    if (primaryBridge != nullptr) {
      status.bridgeNodeId = primaryBridge->nodeId;
      status.bridgeRSSI = primaryBridge->routerRSSI;
    }
    
    // Calculate time since last bridge change
    if (lastBridgeChangeTime > 0) {
      status.timeSinceBridgeChange = millis() - lastBridgeChangeTime;
    }
    
    return status;
  }

  /**
   * Get election history
   * 
   * Returns list of recent bridge elections if diagnostics are enabled.
   * History is limited to last 10 elections.
   * 
   * \code
   * auto history = mesh.getElectionHistory();
   * for (const auto& record : history) {
   *   Serial.printf("Election: winner=%u, RSSI=%d, candidates=%u\n",
   *                 record.winnerNodeId, record.winnerRSSI, record.candidateCount);
   * }
   * \endcode
   * 
   * @return Vector of ElectionRecord structures
   */
  std::vector<ElectionRecord> getElectionHistory() {
    if (!diagnosticsEnabled) {
      Log(logger::GENERAL, "getElectionHistory(): Diagnostics not enabled\n");
      return std::vector<ElectionRecord>();
    }
    return electionHistory;
  }

  /**
   * Get last bridge change event
   * 
   * Returns information about the most recent bridge change
   * 
   * \code
   * auto event = mesh.getLastBridgeChange();
   * if (event.timestamp > 0) {
   *   Serial.printf("Bridge changed from %u to %u: %s\n",
   *                 event.oldBridgeId, event.newBridgeId, event.reason.c_str());
   * }
   * \endcode
   * 
   * @return BridgeChangeEvent structure
   */
  BridgeChangeEvent getLastBridgeChange() {
    return lastBridgeChange;
  }

  /**
   * Get Internet path to specific node
   * 
   * Returns the routing path from the specified node to the Internet bridge.
   * Path includes all intermediate nodes from source to bridge.
   * 
   * \code
   * auto path = mesh.getInternetPath(targetNodeId);
   * Serial.print("Path to Internet: ");
   * for (auto nodeId : path) {
   *   Serial.printf("%u -> ", nodeId);
   * }
   * Serial.println("Internet");
   * \endcode
   * 
   * @param nodeId Node to find path from
   * @return Vector of node IDs representing the path (empty if no path)
   */
  std::vector<uint32_t> getInternetPath(uint32_t nodeId) {
    std::vector<uint32_t> path;
    
    // Get primary bridge
    auto primaryBridge = this->getPrimaryBridge();
    if (primaryBridge == nullptr) {
      Log(logger::GENERAL, "getInternetPath(): No bridge available\n");
      return path;
    }
    
    // If requesting path for the bridge itself
    if (nodeId == primaryBridge->nodeId) {
      path.push_back(nodeId);
      return path;
    }
    
    // Start with the target node
    path.push_back(nodeId);
    
    // For now, simplified routing: if direct connection, add bridge
    // TODO: Implement proper multi-hop path discovery
    if (this->isConnected(primaryBridge->nodeId)) {
      path.push_back(primaryBridge->nodeId);
    }
    
    return path;
  }

  /**
   * Get bridge node ID for specific node
   * 
   * Returns the bridge node ID that the specified node should use to reach Internet.
   * For most cases, this is the primary bridge.
   * 
   * \code
   * uint32_t bridgeId = mesh.getBridgeForNodeId(targetNodeId);
   * if (bridgeId != 0) {
   *   Serial.printf("Node %u uses bridge %u\n", targetNodeId, bridgeId);
   * }
   * \endcode
   * 
   * @param nodeId Node to find bridge for
   * @return Bridge node ID, or 0 if no bridge available
   */
  uint32_t getBridgeForNodeId(uint32_t nodeId) {
    auto primaryBridge = this->getPrimaryBridge();
    if (primaryBridge != nullptr) {
      return primaryBridge->nodeId;
    }
    return 0;
  }

  /**
   * Export topology as DOT format (Graphviz)
   * 
   * Generates a GraphViz DOT format representation of the mesh topology.
   * Can be visualized using Graphviz tools.
   * 
   * \code
   * String dot = mesh.exportTopologyDOT();
   * Serial.println(dot);
   * // Save to file or send to visualization tool
   * \endcode
   * 
   * @return String containing DOT format graph
   */
  TSTRING exportTopologyDOT() {
    TSTRING dot = "digraph mesh {\n";
    dot += "  rankdir=TB;\n";
    dot += "  node [shape=box];\n\n";
    
    // Add this node
    dot += "  \"" + TSTRING(std::to_string(this->nodeId).c_str()) + "\" ";
    if (this->isBridge()) {
      dot += "[style=filled,fillcolor=lightblue,label=\"" + TSTRING(std::to_string(this->nodeId).c_str()) + "\\nBridge\"];\n";
    } else {
      dot += "[label=\"" + TSTRING(std::to_string(this->nodeId).c_str()) + "\"];\n";
    }
    
    // Add Internet node if bridge exists
    auto primaryBridge = this->getPrimaryBridge();
    if (primaryBridge != nullptr && primaryBridge->internetConnected) {
      dot += "  \"Internet\" [shape=cloud,style=filled,fillcolor=lightgreen];\n";
      dot += "  \"" + TSTRING(std::to_string(primaryBridge->nodeId).c_str()) + "\" -> \"Internet\" [style=dashed,color=green];\n";
    }
    
    // Add all known nodes and connections
    auto nodeList = this->getNodeList(false);
    for (auto node : nodeList) {
      dot += "  \"" + TSTRING(std::to_string(node).c_str()) + "\";\n";
    }
    
    // Add edges for direct connections
    for (auto conn : this->subs) {
      if (conn->connected()) {
        dot += "  \"" + TSTRING(std::to_string(this->nodeId).c_str()) + "\" -> \"" + TSTRING(std::to_string(conn->nodeId).c_str()) + "\"";
        
        // Add edge labels with latency
        int latency = conn->getLatency();
        if (latency >= 0) {
          dot += " [label=\"" + TSTRING(std::to_string(latency).c_str()) + "ms\"]";
        }
        dot += ";\n";
      }
    }
    
    dot += "}\n";
    return dot;
  }

  /**
   * Test bridge connectivity
   * 
   * Runs a connectivity test to the primary bridge and optionally to Internet.
   * Measures latency and reachability.
   * 
   * \code
   * auto result = mesh.testBridgeConnectivity();
   * if (result.success) {
   *   Serial.printf("Bridge test passed: %s (latency: %u ms)\n", 
   *                 result.message.c_str(), result.latencyMs);
   * } else {
   *   Serial.printf("Bridge test failed: %s\n", result.message.c_str());
   * }
   * \endcode
   * 
   * @return BridgeTestResult with test results
   */
  BridgeTestResult testBridgeConnectivity() {
    BridgeTestResult result;
    
    // Check if we have a bridge
    auto primaryBridge = this->getPrimaryBridge();
    if (primaryBridge == nullptr) {
      result.success = false;
      result.message = "No bridge available";
      return result;
    }
    
    // Check if bridge is reachable
    result.bridgeReachable = this->isConnected(primaryBridge->nodeId);
    if (!result.bridgeReachable) {
      result.success = false;
      result.message = "Bridge not reachable";
      return result;
    }
    
    // Estimate latency from connection info
    for (auto conn : this->subs) {
      if (conn->nodeId == primaryBridge->nodeId && conn->connected()) {
        int latency = conn->getLatency();
        if (latency >= 0) {
          result.latencyMs = latency;
        }
        break;
      }
    }
    
    // Check Internet connectivity through bridge
    result.internetReachable = primaryBridge->internetConnected;
    
    result.success = result.bridgeReachable;
    if (result.internetReachable) {
      result.message = "Bridge reachable with Internet";
    } else {
      result.message = "Bridge reachable, no Internet";
    }
    
    return result;
  }

  /**
   * Check if specific bridge is reachable
   * 
   * Tests if the specified bridge node can be reached from this node.
   * 
   * \code
   * if (mesh.isBridgeReachable(bridgeNodeId)) {
   *   Serial.println("Bridge is reachable");
   * }
   * \endcode
   * 
   * @param bridgeNodeId Bridge node ID to test
   * @return true if bridge is reachable, false otherwise
   */
  bool isBridgeReachable(uint32_t bridgeNodeId) {
    return this->isConnected(bridgeNodeId);
  }

  /**
   * Get comprehensive diagnostic report
   * 
   * Generates a human-readable diagnostic report with mesh status, bridge info,
   * connectivity, and performance metrics. Useful for debugging and monitoring.
   * 
   * \code
   * Serial.println(mesh.getDiagnosticReport());
   * // Example output:
   * // === painlessMesh Diagnostics ===
   * // Mode: Regular Node
   * // Mesh: ProductionMesh (25 nodes)
   * // Bridge: 123456789 (RSSI: -42 dBm, Internet: âœ“)
   * // Queue: 3 messages (2 CRITICAL, 1 NORMAL)
   * // Uptime: 02:15:33
   * // Last Election: 00:45:12 ago (Winner: 123456789)
   * \endcode
   * 
   * @return String containing formatted diagnostic report
   */
  TSTRING getDiagnosticReport() {
    TSTRING report = "=== painlessMesh Diagnostics ===\n";
    
    // Node info
    auto status = this->getBridgeStatus();
    report += "Node ID: " + TSTRING(std::to_string(this->nodeId).c_str()) + "\n";
    report += "Mode: " + status.role + "\n";
    
    // Mesh info
    auto nodeList = this->getNodeList(true);
    report += "Mesh Nodes: " + TSTRING(std::to_string(nodeList.size()).c_str()) + "\n";
    
    // Bridge info
    if (status.isBridge) {
      report += "Bridge: " + TSTRING(std::to_string(this->nodeId).c_str()) + " (this node)\n";
    } else {
      auto primaryBridge = this->getPrimaryBridge();
      if (primaryBridge != nullptr) {
        report += "Bridge: " + TSTRING(std::to_string(primaryBridge->nodeId).c_str());
        report += " (RSSI: " + TSTRING(std::to_string(primaryBridge->routerRSSI).c_str()) + " dBm";
        report += ", Internet: " + TSTRING(primaryBridge->internetConnected ? "âœ“" : "âœ—") + ")\n";
      } else {
        report += "Bridge: None available\n";
      }
    }
    
    // Connection info
    report += "Direct Connections: " + TSTRING(std::to_string(this->subs.size()).c_str()) + "\n";
    
    // Health metrics
    auto metrics = this->getBridgeHealthMetrics();
    report += "Messages RX: " + TSTRING(std::to_string(metrics.messagesRx).c_str()) + "\n";
    report += "Messages TX: " + TSTRING(std::to_string(metrics.messagesTx).c_str()) + "\n";
    report += "Messages Dropped: " + TSTRING(std::to_string(metrics.messagesDropped).c_str()) + "\n";
    
    if (metrics.avgLatencyMs > 0) {
      report += "Avg Latency: " + TSTRING(std::to_string(metrics.avgLatencyMs).c_str()) + " ms\n";
    }
    
    // Uptime
    uint32_t uptimeSeconds = millis() / 1000;
    uint32_t hours = uptimeSeconds / 3600;
    uint32_t minutes = (uptimeSeconds % 3600) / 60;
    uint32_t seconds = uptimeSeconds % 60;
    
    char uptimeStr[32];
    snprintf(uptimeStr, sizeof(uptimeStr), "%02u:%02u:%02u", hours, minutes, seconds);
    report += "Uptime: " + TSTRING(uptimeStr) + "\n";
    
    // Election info
    if (diagnosticsEnabled && !electionHistory.empty()) {
      auto& lastElection = electionHistory.back();
      uint32_t timeSinceElection = millis() - lastElection.timestamp;
      uint32_t electionMinutes = (timeSinceElection / 1000) / 60;
      uint32_t electionSeconds = (timeSinceElection / 1000) % 60;
      
      char electionTimeStr[32];
      snprintf(electionTimeStr, sizeof(electionTimeStr), "%02u:%02u", electionMinutes, electionSeconds);
      
      report += "Last Election: " + TSTRING(electionTimeStr) + " ago";
      report += " (Winner: " + TSTRING(std::to_string(lastElection.winnerNodeId).c_str());
      report += ", " + TSTRING(std::to_string(lastElection.candidateCount).c_str()) + " candidates)\n";
    }
    
    report += "================================\n";
    return report;
  }

  inline std::shared_ptr<Task> addTask(unsigned long aInterval,
                                       long aIterations,
                                       std::function<void()> aCallback) {
    return plugin::PackageHandler<T>::addTask((*this->mScheduler), aInterval,
                                              aIterations, aCallback);
  }

  inline std::shared_ptr<Task> addTask(std::function<void()> aCallback) {
    return plugin::PackageHandler<T>::addTask((*this->mScheduler), aCallback);
  }

  /**
   * Add a one-shot delayed task
   *
   * @param aCallback Function to call after the delay
   * @param delayMs Delay in milliseconds before executing the callback
   * @return Shared pointer to the task
   */
  inline std::shared_ptr<Task> addTask(std::function<void()> aCallback,
                                       unsigned long delayMs) {
    return plugin::PackageHandler<T>::addTask((*this->mScheduler), delayMs,
                                              TASK_ONCE, aCallback);
  }

  ~Mesh() {
    this->stop();
    if (!isExternalScheduler) delete mScheduler;
    if (messageQueue) delete messageQueue;
  }

 protected:
  void setScheduler(Scheduler *baseScheduler) {
    this->mScheduler = baseScheduler;
    isExternalScheduler = true;
  }

 public:  // Windows MSVC: router functions need access
  void startTimeSync(std::shared_ptr<T> conn) {
    using namespace logger;
    Log(S_TIME, "startTimeSync(): from %u with %u\n", this->nodeId,
        conn->nodeId);
    painlessmesh::protocol::TimeSync timeSync;
    if (ntp::adopt(this->asNodeTree(), (*conn))) {
      timeSync = painlessmesh::protocol::TimeSync(this->nodeId, conn->nodeId,
                                                  this->getNodeTime());
      Log(S_TIME, "startTimeSync(): Requesting time from %u\n", conn->nodeId);
    } else {
      timeSync = painlessmesh::protocol::TimeSync(this->nodeId, conn->nodeId);
      Log(S_TIME, "startTimeSync(): Requesting %u to adopt our time\n",
          conn->nodeId);
    }
    router::send<protocol::TimeSync, T>(timeSync, conn, true);
  }

  /**
   * Helper function to get direct neighbors of a node from the mesh topology tree
   * 
   * \param tree The node tree to search
   * \param nodeId The node whose neighbors we want to find
   * \param neighbors Output list of neighbor node IDs
   * \param parent The parent node ID (used during recursion, 0 means no parent)
   * \return true if the node was found in the tree
   */
  bool getNodeNeighbors(const protocol::NodeTree& tree, uint32_t nodeId,
                       std::list<uint32_t>& neighbors, uint32_t parent = 0) const {
    if (tree.nodeId == nodeId) {
      // Found the target node - collect its neighbors
      // Add parent if it exists
      if (parent != 0) {
        neighbors.push_back(parent);
      }
      // Add all direct children
      for (auto& sub : tree.subs) {
        if (sub.nodeId != 0) {
          neighbors.push_back(sub.nodeId);
        }
      }
      return true;
    }
    
    // Recursively search in children
    for (auto& sub : tree.subs) {
      if (getNodeNeighbors(sub, nodeId, neighbors, tree.nodeId)) {
        return true;
      }
    }
    
    return false;
  }

  bool closeConnectionSTA() {
    auto connection = this->subs.begin();
    while (connection != this->subs.end()) {
      if ((*connection)->station) {
        // We found the STA connection, close it
        (*connection)->close();
        return true;
      }
      ++connection;
    }
    return false;
  }

  void eraseClosedConnections() {
    using namespace logger;
    Log(CONNECTION, "eraseClosedConnections():\n");
    this->subs.remove_if(
        [](const std::shared_ptr<T> &conn) { return !conn->connected(); });
  }

 public:  // Windows MSVC: TCP lambdas need access to droppedConnectionCallbacks
  // Callback functions
  callback::List<uint32_t> newConnectionCallbacks;
  callback::List<uint32_t, bool> droppedConnectionCallbacks;
  callback::List<uint32_t> changedConnectionCallbacks;
  nodeTimeAdjustedCallback_t nodeTimeAdjustedCallback;
  nodeDelayCallback_t nodeDelayReceivedCallback;
  bridgeStatusChangedCallback_t bridgeStatusChangedCallback;
  rtcSyncCompleteCallback_t rtcSyncCompleteCallback;
  gatewayChangedCallback_t gatewayChangedCallback;
  
  // Message queue for offline mode
  MessageQueue* messageQueue = nullptr;
  
#ifdef ESP32
  SemaphoreHandle_t xSemaphore = NULL;
#endif

  bool isExternalScheduler = false;

  /// Is the node a root node
  bool shouldContainRoot;

  Scheduler *mScheduler;

 public:  // Windows MSVC: lambdas in friend functions need public access

  /**
   * Wrapper function for ESP32 semaphore function
   *
   * Waits for the semaphore to be available and then returns true
   *
   * Always return true on ESP8266
   */
  bool semaphoreTake() {
#ifdef ESP32
    return xSemaphoreTake(xSemaphore, (TickType_t)100) == pdTRUE;  // Was 10
#else
    return true;
#endif
  }

  /**
   * Wrapper function for ESP32 semaphore give function
   *
   * Does nothing on ESP8266 hardware
   */
  void semaphoreGive() {
#ifdef ESP32
    xSemaphoreGive(xSemaphore);
#endif
  }

  // Bridge status tracking
  std::vector<BridgeInfo> knownBridges;
  uint32_t bridgeStatusIntervalMs = 30000;  // Default 30 seconds
  uint32_t bridgeTimeoutMs = 60000;         // Default 60 seconds
  bool bridgeStatusBroadcastEnabled = true;
  
  // Bridge cleanup configuration
  static const size_t MAX_KNOWN_BRIDGES = 20;  // Memory efficient limit for ESP8266
  std::shared_ptr<Task> bridgeCleanupTask = nullptr;

  // Health metrics tracking
  uint32_t metricsDisconnectCount = 0;

  // RTC management
  rtc::RTCManager rtcManager;

  // Diagnostics tracking
  bool diagnosticsEnabled = false;
  std::vector<ElectionRecord> electionHistory;
  static const size_t MAX_ELECTION_HISTORY = 10;
  BridgeChangeEvent lastBridgeChange;
  uint32_t lastBridgeChangeTime = 0;

  // Local Internet health check
  gateway::InternetHealthChecker internetHealthChecker;
  std::shared_ptr<Task> internetHealthCheckTask = nullptr;
  localInternetChangedCallback_t localInternetChangedCallback;

  // sendToInternet API
  std::map<uint32_t, PendingInternetRequest> pendingInternetRequests;
  std::shared_ptr<Task> internetCleanupTask = nullptr;
  uint32_t internetRequestTimeout = 30000;   // Default 30 seconds
  uint8_t internetRetryCount = 3;            // Default 3 retries
  uint32_t internetRetryDelay = 1000;        // Default 1 second base delay
  bool sendToInternetEnabled = false;

  friend T;
  friend void onDataCb(void *, AsyncClient *, void *, size_t);
  friend void tcpSentCb(void *, AsyncClient *, size_t, uint32_t);
  friend void meshRecvCb(void *, AsyncClient *, void *, size_t);
  friend void painlessmesh::ntp::handleTimeSync<Mesh, T>(
      Mesh &, painlessmesh::protocol::TimeSync, std::shared_ptr<T>, uint32_t);
  friend void painlessmesh::ntp::handleTimeDelay<Mesh, T>(
      Mesh &, painlessmesh::protocol::TimeDelay, std::shared_ptr<T>, uint32_t);
  friend void painlessmesh::router::handleNodeSync<Mesh, T>(
      Mesh &, protocol::NodeTree, std::shared_ptr<T> conn);
  friend void painlessmesh::tcp::initServer<T, Mesh>(AsyncServer &, Mesh &);
  friend void painlessmesh::tcp::connect<T, Mesh>(AsyncClient &, IPAddress,
                                                  uint16_t, Mesh &);
};

class Connection : public painlessmesh::layout::Neighbour,
                   public painlessmesh::tcp::BufferedConnection {
 public:
  Mesh<Connection> *mesh;
  bool station = true;
  bool newConnection = true;

  Task timeSyncTask;
  Task nodeSyncTask;
  Task timeOutTask;

  // Connection metrics tracking
  uint32_t messagesRx = 0;
  uint32_t messagesTx = 0;
  uint32_t messagesDropped = 0;
  uint32_t timeLastReceived = 0;
  uint64_t bytesRx = 0;
  uint64_t bytesTx = 0;
  
  // Latency tracking (rolling window)
  std::vector<uint32_t> latencySamples;
  static const size_t MAX_LATENCY_SAMPLES = 10;

  Connection(AsyncClient *client, Mesh<painlessmesh::Connection> *mesh,
             bool station)
      : painlessmesh::tcp::BufferedConnection(client),
        mesh(mesh),
        station(station) {}

  void initTasks() {
    auto self = this->shared_from_this();
    auto mesh = this->mesh;
    this->onReceive([self](const TSTRING &str) {
      auto variant = painlessmesh::protocol::Variant(str);

      router::routePackage<painlessmesh::Connection>(
          (*self->mesh), self->shared_from_this(), str,
          self->mesh->callbackList, self->mesh->getNodeTime());
    });

    this->onDisconnect([mesh, self]() {
      Log.remote("id:%u AsyncClient disconnect\n", self->nodeId);
      self->timeSyncTask.setCallback(NULL);
      self->timeSyncTask.disable();
      self->nodeSyncTask.setCallback(NULL);
      self->nodeSyncTask.disable();
      self->timeOutTask.setCallback(NULL);
      self->timeOutTask.disable();
      auto nodeId = self->nodeId;
      auto station = self->station;
      mesh->addTask([mesh, nodeId, station]() {
        mesh->changedConnectionCallbacks.execute(nodeId);
        mesh->droppedConnectionCallbacks.execute(nodeId, station);
      });
      self->clear();
    });

    using namespace logger;

    timeOutTask.set(NODE_TIMEOUT, TASK_ONCE, [self]() {
      Log(CONNECTION, "Time out reached\n");
      Log.remote("id:%u TimeOut\n", self->nodeId);
      self->close();
    });
    mesh->mScheduler->addTask(timeOutTask);

    this->nodeSyncTask.set(TASK_MINUTE, TASK_FOREVER, [self]() {
      Log(SYNC, "nodeSyncTask(): request with %u\n", self->nodeId);
      router::send<protocol::NodeSyncRequest, Connection>(
          self->request(self->mesh->asNodeTree()), self);
      self->timeOutTask.disable();
      self->timeOutTask.restartDelayed();
    });

    mesh->mScheduler->addTask(this->nodeSyncTask);
    if (station)
      this->nodeSyncTask.enable();
    else
      this->nodeSyncTask.enableDelayed(10 * TASK_SECOND);

    Log(CONNECTION, "painlessmesh::Connection: New connection established.\n");
    this->initialize(mesh->mScheduler);
  }

  bool addMessage(const TSTRING &msg, bool priority = false) {
    return this->write(msg, priority);
  }
  
  /**
   * Add message with explicit priority level (0-3)
   * 
   * \param msg The message to send
   * \param priorityLevel Priority level: 0=CRITICAL, 1=HIGH, 2=NORMAL, 3=LOW
   */
  bool addMessageWithPriority(const TSTRING &msg, uint8_t priorityLevel) {
    return this->writeWithPriority(msg, priorityLevel);
  }

  /**
   * Record message received timestamp and bytes
   */
  void onMessageReceived(size_t bytes = 0) {
    messagesRx++;
    bytesRx += bytes;
    timeLastReceived = millis();
  }

  /**
   * Record message sent and bytes
   */
  void onMessageSent(bool success, size_t bytes = 0) {
    if (success) {
      messagesTx++;
      bytesTx += bytes;
    } else {
      messagesDropped++;
    }
  }

  /**
   * Record round-trip time sample
   */
  void recordLatency(uint32_t latencyMs) {
    latencySamples.push_back(latencyMs);
    if (latencySamples.size() > MAX_LATENCY_SAMPLES) {
      latencySamples.erase(latencySamples.begin());
    }
  }

  /**
   * Get average latency from recent samples
   * Returns -1 if no samples available
   */
  int getLatency() {
    if (latencySamples.empty()) return -1;

    uint32_t sum = 0;
    for (auto sample : latencySamples) {
      sum += sample;
    }
    return sum / latencySamples.size();
  }

  /**
   * Calculate connection quality (0-100)
   * Based on: latency, packet loss, RSSI
   */
  int getQuality() {
    // Start with perfect quality
    int quality = 100;

    // Penalize high latency (>100ms)
    int latency = getLatency();
    if (latency > 100) {
      quality -= (latency - 100) / 5;
    }

    // Penalize packet loss
    if (messagesTx > 0) {
      int lossRate = (messagesDropped * 100) / messagesTx;
      quality -= lossRate;
    }

    // Penalize weak RSSI (if available)
    int rssi = getRSSI();
    if (rssi < -80 && rssi != 0) {
      quality -= (80 + rssi);  // e.g., -90 dBm = penalty of 10
    }

    return (std::max)(0, (std::min)(100, quality));
  }

  /**
   * Get WiFi RSSI if available
   * Returns RSSI in dBm (typically -30 to -90) or 0 if not available
   */
  int getRSSI() {
#if defined(ESP32)
    // On ESP32, get WiFi RSSI
    if (WiFi.status() == WL_CONNECTED) {
      return WiFi.RSSI();
    }
    return 0;
#elif defined(ESP8266)
    // On ESP8266, get WiFi RSSI
    if (WiFi.status() == WL_CONNECTED) {
      return WiFi.RSSI();
    }
    return 0;
#else
    // Not available on non-WiFi platforms
    return 0;
#endif
  }

 protected:
  std::shared_ptr<Connection> shared_from_this() { return shared_from(this); }
};
};  // namespace painlessmesh
#endif
