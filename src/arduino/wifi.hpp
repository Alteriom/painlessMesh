#ifndef _PAINLESS_MESH_ARDUINO_WIFI_HPP_
#define _PAINLESS_MESH_ARDUINO_WIFI_HPP_

#include "painlessmesh/configuration.hpp"

#include "painlessmesh/logger.hpp"
#ifdef PAINLESSMESH_ENABLE_ARDUINO_WIFI
#include "painlessMeshSTA.h"

#include "painlessmesh/callback.hpp"
#include "painlessmesh/mesh.hpp"
#include "painlessmesh/router.hpp"
#include "painlessmesh/tcp.hpp"
#include "painlessmesh/message_queue.hpp"

extern painlessmesh::logger::LogClass Log;

namespace painlessmesh {
namespace wifi {
class Mesh : public painlessmesh::Mesh<Connection> {
 public:
  /** Initialize the mesh network
   *
   * Add this to your setup() function. This routine does the following things:
   *
   * - Starts a wifi network
   * - Begins searching for other wifi networks that are part of the mesh
   * - Logs on to the best mesh network node it finds… if it doesn’t find
   * anything, it starts a new search in 5 seconds.
   *
   * @param ssid The name of your mesh.  All nodes share same AP ssid. They are
   * distinguished by BSSID.
   * @param password Wifi password to your mesh.
   * @param port the TCP port that you want the mesh server to run on. Defaults
   * to 5555 if not specified.
   * @param connectMode Switch between WIFI_AP, WIFI_STA and WIFI_AP_STA
   * (default) mode
   */
  void init(TSTRING ssid, TSTRING password, uint16_t port = 5555,
            WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1,
            uint8_t hidden = 0, uint8_t maxconn = MAX_CONN) {
    using namespace logger;
    // Init random generator seed to generate delay variance
    randomSeed(millis());

    // Shut Wifi down and start with a blank slage
    if (WiFi.status() != WL_DISCONNECTED) WiFi.disconnect();

    Log(STARTUP, "init(): %d\n",
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        // Disable autoconnect
        WiFi.setAutoReconnect(false));
#else
        // Disable autoconnect
        WiFi.setAutoConnect(false));
#endif
    WiFi.persistent(false);

    // start configuration
    if (!WiFi.mode(connectMode)) {
      Log(GENERAL, "WiFi.mode() false");
    }

    _meshSSID = ssid;
    _meshPassword = password;
    _meshChannel = channel;
    Log(STARTUP, "init(): Mesh channel set to %d\n", _meshChannel);
    _meshHidden = hidden;
    _meshMaxConn = maxconn;
    _meshPort = port;

    uint8_t MAC[] = {0, 0, 0, 0, 0, 0};
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    esp_read_mac(MAC, ESP_MAC_WIFI_SOFTAP);
#else
    if (WiFi.softAPmacAddress(MAC) == 0) {
      Log(ERROR, "init(): WiFi.softAPmacAddress(MAC) failed.\n");
    }
#endif
    uint32_t nodeId = tcp::encodeNodeId(MAC);
    if (nodeId == 0) Log(ERROR, "NodeId set to 0\n");

    this->init(nodeId);

    // Add bridge election package handler (Type 611)
    this->callbackList.onPackage(
        611,  // BRIDGE_ELECTION type
        [this](protocol::Variant& variant, std::shared_ptr<Connection>, uint32_t) {
          JsonDocument doc;
          TSTRING str;
          variant.printTo(str);
          deserializeJson(doc, str);
          JsonObject obj = doc.as<JsonObject>();
          
          if (obj["routerRSSI"].is<int>()) {
            uint32_t fromNode = obj["from"];
            int8_t routerRSSI = obj["routerRSSI"];
            uint32_t uptime = obj["uptime"] | 0;
            uint32_t freeMemory = obj["freeMemory"] | 0;
            
            this->handleBridgeElection(fromNode, routerRSSI, uptime, freeMemory);
            
            Log(CONNECTION, "Bridge election candidate from %u: RSSI %d dBm\n",
                fromNode, routerRSSI);
          }
          return false;  // Don't consume the package
        });

    // Add bridge takeover package handler (Type 612)
    this->callbackList.onPackage(
        612,  // BRIDGE_TAKEOVER type
        [this](protocol::Variant& variant, std::shared_ptr<Connection>, uint32_t) {
          JsonDocument doc;
          TSTRING str;
          variant.printTo(str);
          deserializeJson(doc, str);
          JsonObject obj = doc.as<JsonObject>();
          
          if (obj["previousBridge"].is<unsigned int>()) {
            uint32_t newBridge = obj["from"];
            uint32_t previousBridge = obj["previousBridge"];
            TSTRING reason = obj["reason"].as<TSTRING>();
            
            Log(CONNECTION, "Bridge takeover: Node %u replaced %u (%s)\n",
                newBridge, previousBridge, reason.c_str());
                
            // Notify callback if this node was not the winner
            if (newBridge != this->nodeId && bridgeRoleChangedCallback) {
              bridgeRoleChangedCallback(false, "Another node won election");
            }
          }
          return false;  // Don't consume the package
        });

    // Add callback to detect bridge failures and trigger elections
    this->onBridgeStatusChanged([this](uint32_t bridgeNodeId, bool hasInternet) {
      if (!hasInternet && bridgeFailoverEnabled && routerCredentialsConfigured) {
        Log(CONNECTION, "Bridge %u lost Internet, considering election...\n", bridgeNodeId);
        
        // Check if we still have any healthy bridges
        if (!this->hasInternetConnection()) {
          Log(CONNECTION, "No healthy bridges, starting election\n");
          // Small delay to let all nodes detect the failure
          this->addTask(2000, TASK_ONCE, [this]() {
            this->startBridgeElection();
          });
        }
      }
    });

    tcpServerInit();
    eventHandleInit();

    _apIp = IPAddress(0, 0, 0, 0);

    if (connectMode & WIFI_AP) {
      apInit(nodeId);  // setup AP
    }
    if (connectMode & WIFI_STA) {
      this->initStation();
    }
  }

  /** Initialize the mesh network
   *
   * Add this to your setup() function. This routine does the following things:
   *
   * - Starts a wifi network
   * - Begins searching for other wifi networks that are part of the mesh
   * - Logs on to the best mesh network node it finds… if it doesn’t find
   * anything, it starts a new search in 5 seconds.
   *
   * @param ssid The name of your mesh.  All nodes share same AP ssid. They are
   * distinguished by BSSID.
   * @param password Wifi password to your mesh.
   * @param port the TCP port that you want the mesh server to run on. Defaults
   * to 5555 if not specified.
   * @param connectMode Switch between WIFI_AP, WIFI_STA and WIFI_AP_STA
   * (default) mode
   */
  void init(TSTRING ssid, TSTRING password, Scheduler *baseScheduler,
            uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA,
            uint8_t channel = 1, uint8_t hidden = 0,
            uint8_t maxconn = MAX_CONN) {
    this->setScheduler(baseScheduler);
    init(ssid, password, port, connectMode, channel, hidden, maxconn);
  }

  /**
   * Initialize mesh as a bridge node with automatic channel detection
   * 
   * This method connects to a router first, detects its channel, then
   * initializes the mesh on the same channel. This ensures the bridge
   * can maintain both router and mesh connections on the same channel.
   * 
   * The bridge node will automatically:
   * - Connect to the specified router in STA mode
   * - Detect the router's WiFi channel
   * - Initialize the mesh AP on the detected channel
   * - Set itself as root node
   * - Maintain the router connection
   * 
   * @param meshSSID The name of your mesh network
   * @param meshPassword WiFi password for the mesh
   * @param routerSSID SSID of the router to connect to
   * @param routerPassword Password for the router
   * @param baseScheduler Task scheduler for mesh operations
   * @param port TCP port for mesh communication (default: 5555)
   */
  void initAsBridge(TSTRING meshSSID, TSTRING meshPassword,
                    TSTRING routerSSID, TSTRING routerPassword,
                    Scheduler *baseScheduler, uint16_t port = 5555) {
    using namespace logger;
    
    Log(STARTUP, "=== Bridge Mode Initialization ===\n");
    Log(STARTUP, "Step 1: Connecting to router %s...\n", routerSSID.c_str());
    
    // Step 1: Connect to router first to detect its channel
    // Shut Wifi down and start with a blank slate
    if (WiFi.status() != WL_DISCONNECTED) WiFi.disconnect();
    
    Log(STARTUP, "initAsBridge(): %d\n",
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        WiFi.setAutoReconnect(false));
#else
        WiFi.setAutoConnect(false));
#endif
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    
    // Connect to router and wait for connection
    WiFi.begin(routerSSID.c_str(), routerPassword.c_str());
    
    // Wait for connection (with timeout)
    int timeout = 30;  // 30 seconds timeout
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
      delay(1000);
      timeout--;
      Log(STARTUP, ".");
    }
    
    uint8_t detectedChannel = 1;  // Default fallback
    
    if (WiFi.status() == WL_CONNECTED) {
      detectedChannel = WiFi.channel();
      // Validate channel is in valid range (1-13 for 2.4GHz)
      if (detectedChannel < 1 || detectedChannel > 13) {
        Log(ERROR, "\n✗ Invalid channel detected: %d, using default channel 1\n", detectedChannel);
        detectedChannel = 1;
      } else {
        Log(STARTUP, "\n✓ Router connected on channel %d\n", detectedChannel);
        Log(STARTUP, "✓ Router IP: %s\n", WiFi.localIP().toString().c_str());
      }
    } else {
      Log(ERROR, "\n✗ Failed to connect to router, using default channel 1\n");
    }
    
    Log(STARTUP, "Step 2: Initializing mesh on channel %d...\n", detectedChannel);
    
    // Step 2: Initialize mesh on detected channel
    init(meshSSID, meshPassword, baseScheduler, port, WIFI_AP_STA, 
         detectedChannel, 0, MAX_CONN);
    
    Log(STARTUP, "Step 3: Establishing bridge connection...\n");
    
    // Step 3: Re-establish router connection using stationManual
    stationManual(routerSSID, routerPassword, 0);
    
    // Step 4: Configure as root/bridge node
    this->setRoot(true);
    this->setContainsRoot(true);
    
    // Step 5: Setup bridge status broadcasting
    initBridgeStatusBroadcast();
    
    Log(STARTUP, "=== Bridge Mode Active ===\n");
    Log(STARTUP, "  Mesh SSID: %s\n", meshSSID.c_str());
    Log(STARTUP, "  Mesh Channel: %d (matches router)\n", detectedChannel);
    Log(STARTUP, "  Router: %s\n", routerSSID.c_str());
    Log(STARTUP, "  Port: %d\n", port);
  }

  /**
   * Connect (as a station) to a specified network and ip
   *
   * You can pass {0,0,0,0} as IP to have it connect to the gateway
   *
   * This stops the node from scanning for other (non specified) nodes
   * and you should probably also use this node as an anchor: `setAnchor(true)`
   */
  void stationManual(TSTRING ssid, TSTRING password, uint16_t port = 0,
                     IPAddress remote_ip = IPAddress(0, 0, 0, 0)) {
    using namespace logger;
    // Set station config
    stationScan.manualIP = remote_ip;

    Log(STARTUP, "stationManual(): Connecting to %s\n", ssid.c_str());

    // Start scan
    stationScan.init(this, ssid, password, port, _meshChannel,
                     static_cast<bool>(_meshHidden));
    stationScan.manual = true;

    // Directly initiate connection - ESP will auto-detect router's channel
    WiFi.begin(ssid.c_str(), password.c_str());

    Log(STARTUP, "stationManual(): Connection initiated\n");
  }

  void initStation() {
    stationScan.init(this, _meshSSID, _meshPassword, _meshPort, _meshChannel,
                     static_cast<bool>(_meshHidden));
    mScheduler->addTask(stationScan.task);
    stationScan.task.enable();

    this->droppedConnectionCallbacks.push_back(
        [this](uint32_t nodeId, bool station) {
          if (station) {
            if (WiFi.status() == WL_CONNECTED) WiFi.disconnect();
            // TODO: Can we do this when we get signalled that wifi disconnect
            // is complete
            this->stationScan.yieldConnectToAP();
            // Re-enable it if it was disabled
            this->stationScan.task.enableIfNot();
          }
        });
  }

  void tcpServerInit() {
    using namespace logger;
    Log(GENERAL, "tcpServerInit():\n");
    _tcpListener = new AsyncServer(_meshPort);
    painlessmesh::tcp::initServer<Connection, painlessmesh::Mesh<Connection>>(
        (*_tcpListener), (*this));
    Log(STARTUP, "AP tcp server established on port %d\n", _meshPort);
    return;
  }

  void tcpConnect() {
    using namespace logger;
    // TODO: move to Connection or StationConnection?
    Log(GENERAL, "tcpConnect():\n");
    if (stationScan.manual && stationScan.port == 0)
      return;  // We have been configured not to connect to the mesh

    // TODO: We could pass this to tcpConnect instead of loading it here
    if (WiFi.status() == WL_CONNECTED && WiFi.localIP()) {
      AsyncClient *pConn = new AsyncClient();

      IPAddress ip = WiFi.gatewayIP();
      if (stationScan.manualIP) {
        ip = stationScan.manualIP;
      }

      painlessmesh::tcp::connect<Connection, painlessmesh::Mesh<Connection>>(
          (*pConn), ip, stationScan.port, (*this));
    } else {
      Log(ERROR, "tcpConnect(): err Something un expected in tcpConnect()\n");
    }
  }

  bool setHostname(const char *hostname) {
#ifdef ESP8266
    return WiFi.hostname(hostname);
#elif defined(ESP32)
    if (strlen(hostname) > 32) {
      return false;
    }
    return WiFi.setHostname(hostname);
#endif  // ESP8266
  }

  IPAddress getStationIP() { return WiFi.localIP(); }
  IPAddress getAPIP() { return _apIp; }

  /**
   * Enable or disable automatic bridge failover
   * 
   * When enabled, nodes will participate in bridge elections if the primary
   * bridge goes offline and they have router credentials configured.
   * 
   * @param enabled true to enable automatic failover (default), false to disable
   */
  void enableBridgeFailover(bool enabled) {
    bridgeFailoverEnabled = enabled;
  }

  /**
   * Set router credentials for bridge election participation
   * 
   * Nodes must have router credentials configured to participate in bridge
   * elections. When a bridge fails, only nodes with credentials can become
   * the new bridge.
   * 
   * @param ssid Router SSID
   * @param password Router password
   */
  void setRouterCredentials(TSTRING ssid, TSTRING password) {
    routerSSID = ssid;
    routerPassword = password;
    routerCredentialsConfigured = true;
  }

  /**
   * Set the election timeout (how long to collect candidates)
   * 
   * @param timeoutMs Timeout in milliseconds (default: 5000 = 5 seconds)
   */
  void setElectionTimeout(uint32_t timeoutMs) {
    electionTimeoutMs = timeoutMs;
  }

  /**
   * Set callback for when this node's bridge role changes
   * 
   * @param callback Function to call when role changes
   */
  void onBridgeRoleChanged(std::function<void(bool isBridge, TSTRING reason)> callback) {
    bridgeRoleChangedCallback = callback;
  }

  // ========== Message Queue API ==========
  
  /**
   * Enable message queuing for offline mode
   * 
   * When enabled, messages can be queued during Internet outages and automatically
   * sent when connectivity is restored.
   * 
   * @param maxSize Maximum number of messages to queue (default: 500)
   * @param enablePersistence Enable SPIFFS/LittleFS persistence (default: false)
   * @param storagePath Path to queue storage file (default: "/painlessmesh/queue.dat")
   */
  void enableMessageQueue(uint32_t maxSize = 500, bool enablePersistence = false,
                         const TSTRING& storagePath = "/painlessmesh/queue.dat") {
    messageQueue.init(maxSize, enablePersistence, storagePath);
    messageQueueEnabled = true;
  }
  
  /**
   * Queue a message for later delivery
   * 
   * @param payload Message content
   * @param destination Where to send the message (e.g., MQTT topic, HTTP endpoint)
   * @param priority Message priority level (CRITICAL, HIGH, NORMAL, LOW)
   * @return Message ID if queued successfully, 0 if failed
   */
  uint32_t queueMessage(const TSTRING& payload, const TSTRING& destination,
                       painlessmesh::queue::MessagePriority priority = 
                           painlessmesh::queue::PRIORITY_NORMAL) {
    if (!messageQueueEnabled) {
      Log(logger::ERROR, "queueMessage(): Message queue not enabled\n");
      return 0;
    }
    return messageQueue.queueMessage(payload, destination, priority);
  }
  
  /**
   * Manually flush the message queue
   * 
   * Attempts to send all queued messages using the provided send callback.
   * This is automatically called when Internet connectivity is restored.
   * 
   * @param sendCallback Function to actually send the message
   * @return Number of messages successfully sent
   */
  uint32_t flushMessageQueue(painlessmesh::queue::MessageQueue::sendCallback_t sendCallback) {
    if (!messageQueueEnabled) {
      return 0;
    }
    return messageQueue.flushQueue(sendCallback);
  }
  
  /**
   * Get number of queued messages
   * 
   * @param priority If specified, count only messages of this priority
   * @return Number of messages
   */
  uint32_t getQueuedMessageCount(painlessmesh::queue::MessagePriority priority) const {
    if (!messageQueueEnabled) {
      return 0;
    }
    return messageQueue.getQueuedMessageCount(priority);
  }
  
  uint32_t getQueuedMessageCount() const {
    if (!messageQueueEnabled) {
      return 0;
    }
    return messageQueue.getQueuedMessageCount();
  }
  
  /**
   * Remove messages older than specified age
   * 
   * @param maxAgeHours Maximum age in hours
   * @return Number of messages removed
   */
  uint32_t pruneMessageQueue(uint32_t maxAgeHours) {
    if (!messageQueueEnabled) {
      return 0;
    }
    return messageQueue.pruneQueue(maxAgeHours);
  }
  
  /**
   * Clear all messages from queue
   */
  void clearMessageQueue() {
    if (messageQueueEnabled) {
      messageQueue.clear();
    }
  }
  
  /**
   * Get message queue statistics
   */
  painlessmesh::queue::QueueStats getQueueStats() const {
    return messageQueue.getStats();
  }
  
  /**
   * Set callback for queue state changes
   */
  void onQueueStateChanged(painlessmesh::queue::MessageQueue::queueStateCallback_t callback) {
    messageQueue.onQueueStateChanged(callback);
  }
  
  /**
   * Set maximum retry attempts for queued messages
   */
  void setMaxQueueRetryAttempts(uint32_t attempts) {
    messageQueue.setMaxRetryAttempts(attempts);
  }
  
  /**
   * Save queue to persistent storage
   */
  bool saveQueueToStorage() {
    if (!messageQueueEnabled) {
      return false;
    }
    return messageQueue.saveToStorage();
  }
  
  /**
   * Load queue from persistent storage
   * 
   * @return Number of messages loaded
   */
  uint32_t loadQueueFromStorage() {
    if (!messageQueueEnabled) {
      return 0;
    }
    return messageQueue.loadFromStorage();
  }

  void stop() {
    // remove all WiFi events
#ifdef ESP32
    WiFi.removeEvent(eventScanDoneHandler);
    WiFi.removeEvent(eventSTAStartHandler);
    WiFi.removeEvent(eventSTADisconnectedHandler);
    WiFi.removeEvent(eventSTAGotIPHandler);
#elif defined(ESP8266)
    eventSTAConnectedHandler = WiFiEventHandler();
    eventSTADisconnectedHandler = WiFiEventHandler();
    eventSTAGotIPHandler = WiFiEventHandler();

    stationScan.asyncTask.setCallback(NULL);
    mScheduler->deleteTask(stationScan.asyncTask);
#endif  // ESP32
    // Stop scanning task
    stationScan.task.setCallback(NULL);
    mScheduler->deleteTask(stationScan.task);
    painlessmesh::Mesh<Connection>::stop();

    // Shutdown wifi hardware
    if (WiFi.status() != WL_DISCONNECTED) WiFi.disconnect();

    // Delete the tcp server
    delete _tcpListener;
  }

 protected:
  friend class ::StationScan;
  TSTRING _meshSSID;
  TSTRING _meshPassword;
  uint8_t _meshChannel;
  uint8_t _meshHidden;
  uint8_t _meshMaxConn;
  uint16_t _meshPort;

  IPAddress _apIp;
  StationScan stationScan;

  void init(Scheduler *scheduler, uint32_t id) {
    painlessmesh::Mesh<Connection>::init(scheduler, id);
  }

  void init(uint32_t id) { painlessmesh::Mesh<Connection>::init(id); }

  void apInit(uint32_t nodeId) {
    _apIp = IPAddress(10, (nodeId & 0xFF00) >> 8, (nodeId & 0xFF), 1);
    IPAddress netmask(255, 255, 255, 0);

    WiFi.softAPConfig(_apIp, _apIp, netmask);
    WiFi.softAP(_meshSSID.c_str(), _meshPassword.c_str(), _meshChannel,
                _meshHidden, _meshMaxConn);
  }

  /**
   * Initialize bridge status broadcasting
   * Sets up a periodic task to broadcast bridge status to the mesh
   */
  void initBridgeStatusBroadcast() {
    using namespace logger;
    
    if (!this->isBridge() || !this->bridgeStatusBroadcastEnabled) {
      return;
    }
    
    Log(STARTUP, "initBridgeStatusBroadcast(): Setting up bridge status broadcast\n");
    
    // Create periodic task to broadcast bridge status
    bridgeStatusTask = this->addTask(
      this->bridgeStatusIntervalMs,
      TASK_FOREVER,
      [this]() {
        this->sendBridgeStatus();
      }
    );
    
    Log(STARTUP, "Bridge status broadcast enabled (interval: %d ms)\n", 
        this->bridgeStatusIntervalMs);
  }

  /**
   * Scan for router and return its signal strength
   * 
   * @param routerSSID SSID of router to scan for
   * @return RSSI in dBm (negative number, -127 to 0), or 0 if not found
   */
  int8_t scanRouterSignalStrength(TSTRING routerSSID) {
    using namespace logger;
    Log(CONNECTION, "scanRouterSignalStrength(): Scanning for %s...\n", routerSSID.c_str());
    
    int n = WiFi.scanNetworks(false, false);
    Log(CONNECTION, "scanRouterSignalStrength(): Found %d networks\n", n);
    
    for (int i = 0; i < n; i++) {
      if (WiFi.SSID(i) == routerSSID) {
        int8_t rssi = WiFi.RSSI(i);
        Log(CONNECTION, "scanRouterSignalStrength(): Found %s with RSSI %d dBm\n", 
            routerSSID.c_str(), rssi);
        return rssi;
      }
    }
    
    Log(CONNECTION, "scanRouterSignalStrength(): Router %s not found\n", routerSSID.c_str());
    return 0;  // Router not found
  }

  /**
   * Start bridge election process
   * Called when primary bridge failure is detected
   */
  void startBridgeElection() {
    using namespace logger;
    
    if (!bridgeFailoverEnabled) {
      Log(CONNECTION, "startBridgeElection(): Failover disabled\n");
      return;
    }
    
    if (!routerCredentialsConfigured) {
      Log(CONNECTION, "startBridgeElection(): No router credentials, cannot participate\n");
      return;
    }
    
    if (electionState != ELECTION_IDLE) {
      Log(CONNECTION, "startBridgeElection(): Election already in progress\n");
      return;
    }
    
    // Prevent rapid role changes
    if (millis() - lastRoleChangeTime < 60000) {
      Log(CONNECTION, "startBridgeElection(): Too soon after last role change\n");
      return;
    }
    
    Log(CONNECTION, "=== Bridge Election Started ===\n");
    electionState = ELECTION_SCANNING;
    
    // Scan for router to get RSSI
    int8_t routerRSSI = scanRouterSignalStrength(routerSSID);
    
    if (routerRSSI == 0) {
      Log(CONNECTION, "startBridgeElection(): Router not visible, cannot participate\n");
      electionState = ELECTION_IDLE;
      return;
    }
    
    Log(CONNECTION, "startBridgeElection(): My router RSSI: %d dBm\n", routerRSSI);
    
    // Clear previous candidates
    electionCandidates.clear();
    
    // Add self as candidate
    BridgeCandidate selfCandidate;
    selfCandidate.nodeId = this->nodeId;
    selfCandidate.routerRSSI = routerRSSI;
    selfCandidate.uptime = millis();
    selfCandidate.freeMemory = ESP.getFreeHeap();
    electionCandidates.push_back(selfCandidate);
    
    // Broadcast candidacy using JSON directly (avoiding dependency on alteriom package)
    DynamicJsonDocument doc(256);
    JsonObject obj = doc.to<JsonObject>();
    obj["type"] = 611;  // BRIDGE_ELECTION
    obj["from"] = this->nodeId;
    obj["routing"] = 2;  // BROADCAST
    obj["routerRSSI"] = routerRSSI;
    obj["uptime"] = millis();
    obj["freeMemory"] = ESP.getFreeHeap();
    obj["timestamp"] = this->getNodeTime();
    obj["routerSSID"] = routerSSID;
    obj["message_type"] = 611;
    
    String msg;
    serializeJson(doc, msg);
    this->sendBroadcast(msg);
    
    Log(CONNECTION, "startBridgeElection(): Candidacy broadcast sent\n");
    
    // Set election timeout
    electionDeadline = millis() + electionTimeoutMs;
    electionState = ELECTION_COLLECTING;
    
    // Schedule election evaluation
    this->addTask(electionTimeoutMs + 100, TASK_ONCE, [this]() {
      this->evaluateElection();
    });
  }

  /**
   * Evaluate election and determine winner
   * Called after election timeout expires
   */
  void evaluateElection() {
    using namespace logger;
    
    if (electionState != ELECTION_COLLECTING) {
      Log(CONNECTION, "evaluateElection(): Not in collecting state\n");
      return;
    }
    
    Log(CONNECTION, "=== Evaluating Election ===\n");
    Log(CONNECTION, "evaluateElection(): %d candidates\n", electionCandidates.size());
    
    // Find best candidate
    BridgeCandidate* winner = nullptr;
    int8_t bestRSSI = -127;  // Worst possible RSSI
    
    for (auto& candidate : electionCandidates) {
      Log(CONNECTION, "evaluateElection(): Candidate %u: RSSI=%d, uptime=%u, mem=%u\n",
          candidate.nodeId, candidate.routerRSSI, candidate.uptime, candidate.freeMemory);
      
      if (candidate.routerRSSI > bestRSSI) {
        bestRSSI = candidate.routerRSSI;
        winner = &candidate;
      } else if (candidate.routerRSSI == bestRSSI && winner != nullptr) {
        // Tiebreaker 1: Higher uptime
        if (candidate.uptime > winner->uptime) {
          winner = &candidate;
        } else if (candidate.uptime == winner->uptime) {
          // Tiebreaker 2: More free memory
          if (candidate.freeMemory > winner->freeMemory) {
            winner = &candidate;
          } else if (candidate.freeMemory == winner->freeMemory) {
            // Tiebreaker 3: Lower node ID (deterministic)
            if (candidate.nodeId < winner->nodeId) {
              winner = &candidate;
            }
          }
        }
      }
    }
    
    if (winner == nullptr) {
      Log(ERROR, "evaluateElection(): No winner found!\n");
      electionState = ELECTION_IDLE;
      return;
    }
    
    Log(CONNECTION, "=== Election Winner: Node %u ===\n", winner->nodeId);
    Log(CONNECTION, "  Router RSSI: %d dBm\n", winner->routerRSSI);
    Log(CONNECTION, "  Uptime: %u ms\n", winner->uptime);
    Log(CONNECTION, "  Free Memory: %u bytes\n", winner->freeMemory);
    
    if (winner->nodeId == this->nodeId) {
      Log(CONNECTION, "🎯 I WON! Promoting to bridge...\n");
      promoteToBridge();
    } else {
      Log(CONNECTION, "Winner is node %u, remaining as regular node\n", winner->nodeId);
    }
    
    electionState = ELECTION_IDLE;
    electionCandidates.clear();
  }

  /**
   * Promote this node to bridge role
   * Called when node wins election
   */
  void promoteToBridge() {
    using namespace logger;
    
    Log(STARTUP, "=== Becoming Bridge Node ===\n");
    
    // Store previous bridge (if any)
    auto primaryBridge = this->getPrimaryBridge();
    uint32_t previousBridgeId = primaryBridge ? primaryBridge->nodeId : 0;
    
    // Reconfigure as bridge
    this->stop();
    delay(1000);
    
    this->initAsBridge(_meshSSID, _meshPassword, routerSSID, routerPassword,
                       mScheduler, _meshPort);
    
    lastRoleChangeTime = millis();
    
    Log(STARTUP, "✓ Bridge promotion complete\n");
    
    // Notify via callback
    if (bridgeRoleChangedCallback) {
      bridgeRoleChangedCallback(true, "Election winner - best router signal");
    }
    
    // Broadcast takeover announcement
    DynamicJsonDocument doc(256);
    JsonObject obj = doc.to<JsonObject>();
    obj["type"] = 612;  // BRIDGE_TAKEOVER
    obj["from"] = this->nodeId;
    obj["routing"] = 2;  // BROADCAST
    obj["previousBridge"] = previousBridgeId;
    obj["reason"] = "Election winner - best router signal";
    obj["routerRSSI"] = WiFi.RSSI();
    obj["timestamp"] = this->getNodeTime();
    obj["message_type"] = 612;
    
    String msg;
    serializeJson(doc, msg);
    
    // Small delay to ensure mesh is ready
    delay(2000);
    this->sendBroadcast(msg);
    
    Log(STARTUP, "✓ Takeover announcement sent\n");
  }

  /**
   * Handle received bridge election package
   * Called by package handler when election message arrives
   */
  void handleBridgeElection(uint32_t fromNode, int8_t routerRSSI, uint32_t uptime, 
                            uint32_t freeMemory) {
    using namespace logger;
    
    if (electionState != ELECTION_COLLECTING) {
      Log(CONNECTION, "handleBridgeElection(): Not collecting candidates, ignoring\n");
      return;
    }
    
    // Check if candidate already exists
    for (auto& candidate : electionCandidates) {
      if (candidate.nodeId == fromNode) {
        Log(CONNECTION, "handleBridgeElection(): Duplicate candidate from %u, ignoring\n", fromNode);
        return;
      }
    }
    
    BridgeCandidate candidate;
    candidate.nodeId = fromNode;
    candidate.routerRSSI = routerRSSI;
    candidate.uptime = uptime;
    candidate.freeMemory = freeMemory;
    
    electionCandidates.push_back(candidate);
    
    Log(CONNECTION, "handleBridgeElection(): Added candidate %u (RSSI: %d dBm)\n",
        fromNode, routerRSSI);
  }

  /**
   * Send bridge status broadcast
   * Called periodically by bridge nodes to report connectivity status
   */
  void sendBridgeStatus() {
    using namespace logger;
    
    if (!this->bridgeStatusBroadcastEnabled) {
      return;
    }
    
    // Create bridge status package
    // We need to include the package header here since we're in wifi namespace
    // The package will be sent as a JSON string
    DynamicJsonDocument doc(256);
    JsonObject obj = doc.to<JsonObject>();
    
    obj["type"] = 610;  // BRIDGE_STATUS type
    obj["from"] = this->nodeId;
    obj["routing"] = 2;  // BROADCAST routing
    obj["timestamp"] = this->getNodeTime();
    obj["internetConnected"] = (WiFi.status() == WL_CONNECTED);
    obj["routerRSSI"] = WiFi.RSSI();
    obj["routerChannel"] = WiFi.channel();
    obj["uptime"] = millis();
    obj["gatewayIP"] = WiFi.gatewayIP().toString();
    obj["message_type"] = 610;
    
    String msg;
    serializeJson(doc, msg);
    
    Log(GENERAL, "sendBridgeStatus(): Broadcasting status (Internet: %s)\n",
        (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected");
    
    this->sendBroadcast(msg);
  }
  void eventHandleInit() {
    using namespace logger;
#ifdef ESP32
    eventScanDoneHandler = WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info) {
          if (this->semaphoreTake()) {
            Log(CONNECTION,
                "eventScanDoneHandler: ARDUINO_EVENT_WIFI_SCAN_DONE\n");
            this->stationScan.scanComplete();
            this->semaphoreGive();
          }
        },
#if ESP_ARDUINO_VERSION_MAJOR >= 2
        WiFiEvent_t::ARDUINO_EVENT_WIFI_SCAN_DONE);
#else
        WiFiEvent_t::SYSTEM_EVENT_SCAN_DONE);
#endif

    eventSTAStartHandler = WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info) {
          if (this->semaphoreTake()) {
            Log(CONNECTION,
                "eventSTAStartHandler: ARDUINO_EVENT_WIFI_STA_START\n");
            this->semaphoreGive();
          }
        },
#if ESP_ARDUINO_VERSION_MAJOR >= 2
        WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_START);
#else
        WiFiEvent_t::SYSTEM_EVENT_STA_START);
#endif

    eventSTADisconnectedHandler = WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info) {
          if (this->semaphoreTake()) {
            Log(CONNECTION,
                "eventSTADisconnectedHandler: "
                "ARDUINO_EVENT_WIFI_STA_DISCONNECTED\n");
            this->droppedConnectionCallbacks.execute(0, true);
            this->semaphoreGive();
          }
        },
#if ESP_ARDUINO_VERSION_MAJOR >= 2
        WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#else
        WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
#endif

    eventSTAGotIPHandler = WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info) {
          if (this->semaphoreTake()) {
            Log(CONNECTION,
                "eventSTAGotIPHandler: ARDUINO_EVENT_WIFI_STA_GOT_IP\n");
            this->tcpConnect();  // Connect to TCP port
            this->semaphoreGive();
          }
        },
#if ESP_ARDUINO_VERSION_MAJOR >= 2
        WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
#else
        WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
#endif

#elif defined(ESP8266)
    eventSTAConnectedHandler = WiFi.onStationModeConnected(
        [&](const WiFiEventStationModeConnected &event) {
          // Log(CONNECTION, "Event: Station Mode Connected to \"%s\"\n",
          // event.ssid.c_str());
          Log(CONNECTION, "Event: Station Mode Connected\n");
        });

    eventSTADisconnectedHandler = WiFi.onStationModeDisconnected(
        [&](const WiFiEventStationModeDisconnected &event) {
          Log(CONNECTION, "Event: Station Mode Disconnected\n");
          this->droppedConnectionCallbacks.execute(0, true);
        });

    eventSTAGotIPHandler =
        WiFi.onStationModeGotIP([&](const WiFiEventStationModeGotIP &event) {
          Log(CONNECTION,
              "Event: Station Mode Got IP (IP: %s  Mask: %s  Gateway: %s)\n",
              event.ip.toString().c_str(), event.mask.toString().c_str(),
              event.gw.toString().c_str());
          this->tcpConnect();  // Connect to TCP port
        });
#endif  // ESP32
    return;
  }

#ifdef ESP32
  WiFiEventId_t eventScanDoneHandler;
  WiFiEventId_t eventSTAStartHandler;
  WiFiEventId_t eventSTADisconnectedHandler;
  WiFiEventId_t eventSTAGotIPHandler;
#elif defined(ESP8266)
  WiFiEventHandler eventSTAConnectedHandler;
  WiFiEventHandler eventSTADisconnectedHandler;
  WiFiEventHandler eventSTAGotIPHandler;
#endif  // ESP8266
  AsyncServer *_tcpListener;
  std::shared_ptr<Task> bridgeStatusTask;

  // Bridge failover state and configuration
  enum ElectionState {
    ELECTION_IDLE,
    ELECTION_SCANNING,
    ELECTION_COLLECTING
  };

  struct BridgeCandidate {
    uint32_t nodeId;
    int8_t routerRSSI;
    uint32_t uptime;
    uint32_t freeMemory;
  };

  bool bridgeFailoverEnabled = true;
  bool routerCredentialsConfigured = false;
  TSTRING routerSSID = "";
  TSTRING routerPassword = "";
  uint32_t electionTimeoutMs = 5000;  // Default 5 seconds
  uint32_t lastRoleChangeTime = 0;
  ElectionState electionState = ELECTION_IDLE;
  uint32_t electionDeadline = 0;
  std::vector<BridgeCandidate> electionCandidates;
  std::function<void(bool isBridge, TSTRING reason)> bridgeRoleChangedCallback;
  
  // Message queue for offline mode
  bool messageQueueEnabled = false;
  painlessmesh::queue::MessageQueue messageQueue;
};
}  // namespace wifi
};  // namespace painlessmesh

#endif

#endif
