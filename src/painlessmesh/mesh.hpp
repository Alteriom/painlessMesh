#ifndef _PAINLESS_MESH_MESH_HPP_
#define _PAINLESS_MESH_MESH_HPP_

#include <vector>

#include "painlessmesh/configuration.hpp"

#include "painlessmesh/connection.hpp"
#include "painlessmesh/logger.hpp"
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
    return (millis() - lastSeen) < timeoutMs;
  }
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

    // Add bridge status package handler (Type 610)
    // This will be called when any node receives a bridge status broadcast
    this->callbackList.onPackage(
        610,  // BRIDGE_STATUS type
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
   * called.  “from” is the id of the original sender of the message, and “msg”
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
   * Check if any bridge in the mesh has Internet connectivity
   * 
   * @return true if at least one healthy bridge reports Internet connection
   */
  bool hasInternetConnection() {
    for (const auto& bridge : knownBridges) {
      if (bridge.isHealthy(bridgeTimeoutMs) && bridge.internetConnected) {
        return true;
      }
    }
    return false;
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
   * 1. Must be healthy (seen within timeout)
   * 2. Must have Internet connection
   * 3. Best WiFi RSSI to router
   * 
   * @return pointer to BridgeInfo of primary bridge, or nullptr if no suitable bridge
   */
  BridgeInfo* getPrimaryBridge() {
    BridgeInfo* primary = nullptr;
    int8_t bestRSSI = -127;  // Worst possible RSSI
    
    for (auto& bridge : knownBridges) {
      if (bridge.isHealthy(bridgeTimeoutMs) && bridge.internetConnected) {
        if (bridge.routerRSSI > bestRSSI) {
          bestRSSI = bridge.routerRSSI;
          primary = &bridge;
        }
      }
    }
    
    return primary;
  }

  /**
   * Check if this node is acting as a bridge
   * 
   * @return true if this node is configured as a root node (typically bridges)
   */
  bool isBridge() {
    return this->root;
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
    for (auto& b : knownBridges) {
      if (b.nodeId == bridgeNodeId) {
        bridge = &b;
        break;
      }
    }
    
    bool wasConnected = false;
    if (bridge == nullptr) {
      // New bridge - add to list
      BridgeInfo newBridge;
      newBridge.nodeId = bridgeNodeId;
      knownBridges.push_back(newBridge);
      bridge = &knownBridges.back();
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
    return rtcManager.enable(rtcInterface);
  }

  /**
   * Disable RTC integration
   */
  void disableRTC() {
    using namespace logger;
    Log(GENERAL, "disableRTC(): Disabling RTC\n");
    rtcManager.disable();
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
    // Check if it's a direct connection (1 hop)
    for (auto conn : this->subs) {
      if (conn->nodeId == nodeId && conn->connected()) {
        return 1;
      }
    }
    
    // For indirect connections, traverse the node tree
    auto tree = this->asNodeTree();
    // Simple BFS to find hop count
    // For now, return 2 for any node in the mesh but not directly connected
    auto nodeList = this->getNodeList(false);
    for (auto node : nodeList) {
      if (node == nodeId) {
        return 2;  // TODO: Implement proper hop count calculation
      }
    }
    
    return -1;  // Node not found
  }

  /**
   * Get routing table as map (destination -> next hop)
   */
  std::map<uint32_t, uint32_t> getRoutingTable() {
    std::map<uint32_t, uint32_t> table;
    auto nodeList = this->getNodeList(false);
    
    // For each destination, find the next hop
    for (auto destNode : nodeList) {
      // Check direct connections first
      for (auto conn : this->subs) {
        if (conn->nodeId == destNode && conn->connected()) {
          table[destNode] = destNode;  // Direct connection
          break;
        }
      }
      
      // TODO: Implement proper routing table lookup for multi-hop paths
      // For now, just mark direct connections
    }
    
    return table;
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

  ~Mesh() {
    this->stop();
    if (!isExternalScheduler) delete mScheduler;
  }

 protected:
  void setScheduler(Scheduler *baseScheduler) {
    this->mScheduler = baseScheduler;
    isExternalScheduler = true;
  }

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

  // Callback functions
  callback::List<uint32_t> newConnectionCallbacks;
  callback::List<uint32_t, bool> droppedConnectionCallbacks;
  callback::List<uint32_t> changedConnectionCallbacks;
  nodeTimeAdjustedCallback_t nodeTimeAdjustedCallback;
  nodeDelayCallback_t nodeDelayReceivedCallback;
  bridgeStatusChangedCallback_t bridgeStatusChangedCallback;
  rtcSyncCompleteCallback_t rtcSyncCompleteCallback;
#ifdef ESP32
  SemaphoreHandle_t xSemaphore = NULL;
#endif

  bool isExternalScheduler = false;

  /// Is the node a root node
  bool shouldContainRoot;

  Scheduler *mScheduler;

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

  // RTC management
  rtc::RTCManager rtcManager;

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
   * Record message received timestamp
   */
  void onMessageReceived() {
    messagesRx++;
    timeLastReceived = millis();
  }

  /**
   * Record message sent
   */
  void onMessageSent(bool success) {
    if (success) {
      messagesTx++;
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

    return std::max(0, std::min(100, quality));
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
