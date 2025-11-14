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

extern painlessmesh::logger::LogClass Log;

namespace painlessmesh {
namespace wifi {
class Mesh : public painlessmesh::Mesh<Connection> {
 public:
  // Multi-bridge selection strategy enum (must be declared early)
  enum BridgeSelectionStrategy {
    PRIORITY_BASED = 0,  // Use highest priority bridge (default)
    ROUND_ROBIN = 1,     // Distribute load evenly
    BEST_SIGNAL = 2      // Use bridge with best RSSI
  };

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
            uint8_t hidden = 0, uint8_t maxconn = MAX_CONN,
            TSTRING stationSSID = "", TSTRING stationPassword = "") {
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

    // Add periodic monitoring task to detect when no bridge exists
    // This handles the case where no node was initially configured as a bridge
    this->addTask(30000, TASK_FOREVER, [this]() {
      // Only check if failover is enabled and we have credentials
      if (!bridgeFailoverEnabled || !routerCredentialsConfigured) {
        return;
      }
      
      // Don't check if we're already a bridge
      if (this->isBridge()) {
        return;
      }
      
      // Skip check during startup period (60 seconds) to allow initial bridge discovery
      if (millis() < 60000) {
        return;
      }
      
      // Check if there are any healthy bridges
      bool hasHealthyBridge = false;
      for (const auto& bridge : this->getBridges()) {
        if (bridge.isHealthy(bridgeTimeoutMs) && bridge.internetConnected) {
          hasHealthyBridge = true;
          break;
        }
      }
      
      // If no healthy bridge exists, trigger an election
      if (!hasHealthyBridge) {
        Log(CONNECTION, "Bridge monitor: No healthy bridge detected, triggering election\n");
        // Small delay to randomize election start across nodes
        uint32_t randomDelay = random(1000, 3000);
        this->addTask(randomDelay, TASK_ONCE, [this]() {
          this->startBridgeElection();
        });
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
    
    // If station credentials provided, connect to router
    if (!stationSSID.isEmpty() && (connectMode & WIFI_STA)) {
      Log(STARTUP, "init(): Connecting to station %s\n", stationSSID.c_str());
      this->stationManual(stationSSID, stationPassword);
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
            uint8_t maxconn = MAX_CONN,
            TSTRING stationSSID = "", TSTRING stationPassword = "") {
    this->setScheduler(baseScheduler);
    init(ssid, password, port, connectMode, channel, hidden, maxconn, 
         stationSSID, stationPassword);
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
   * Initialize mesh as a bridge node with priority (for multi-bridge mode)
   * 
   * This overload adds bridge priority configuration for multi-bridge deployments.
   * Priority determines which bridge is preferred when multiple bridges are available.
   * 
   * @param meshSSID The name of your mesh network
   * @param meshPassword WiFi password for the mesh
   * @param routerSSID SSID of the router to connect to
   * @param routerPassword Password for the router
   * @param baseScheduler Task scheduler for mesh operations
   * @param port TCP port for mesh communication (default: 5555)
   * @param priority Bridge priority: 10=highest (primary), 5=medium (secondary), 1=lowest (default: 5)
   */
  void initAsBridge(TSTRING meshSSID, TSTRING meshPassword,
                    TSTRING routerSSID, TSTRING routerPassword,
                    Scheduler *baseScheduler, uint16_t port, uint8_t priority) {
    using namespace logger;
    
    // Validate and store priority
    if (priority < 1) priority = 1;
    if (priority > 10) priority = 10;
    bridgePriority = priority;
    
    // Store role based on priority
    if (priority >= 8) {
      bridgeRole = "primary";
    } else if (priority >= 5) {
      bridgeRole = "secondary";
    } else {
      bridgeRole = "standby";
    }
    
    Log(STARTUP, "=== Bridge Mode Initialization (Priority: %d, Role: %s) ===\n", 
        priority, bridgeRole.c_str());
    
    // Call the base initAsBridge method
    initAsBridge(meshSSID, meshPassword, routerSSID, routerPassword, baseScheduler, port);
    
    // Setup multi-bridge coordination if enabled
    if (multiBridgeEnabled) {
      initBridgeCoordination();
    }
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
            if (WiFi.status() == WL_CONNECTED) {
              WiFi.disconnect();
              // Schedule reconnection after disconnect completes
              // The WiFi event handler will signal when disconnect is complete
              _pendingStationReconnect = true;
            } else {
              // Already disconnected, reconnect immediately
              handleStationDisconnectComplete();
            }
          }
        });
  }

  /**
   * Handle station disconnect completion
   * Called after WiFi disconnect event is fully processed
   * This ensures proper sequencing: disconnect -> event -> reconnect
   */
  void handleStationDisconnectComplete() {
    if (_pendingStationReconnect) {
      _pendingStationReconnect = false;
      this->stationScan.yieldConnectToAP();
      // Re-enable scanning if it was disabled
      this->stationScan.task.enableIfNot();
    }
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

  /**
   * Establish TCP connection to mesh network
   * 
   * This method is called by WiFi event handlers when station gets IP address.
   * It creates a TCP client connection to the mesh network gateway.
   * 
   * Architecture Note: This is intentionally kept in the Mesh class rather than
   * extracted to a separate StationConnection class because:
   * - It's tightly coupled with WiFi event lifecycle
   * - Needs access to mesh state and callbacks
   * - Moving it would increase complexity without clear benefits
   * - The existing design keeps connection logic cohesive with WiFi management
   */
  void tcpConnect() {
    using namespace logger;
    Log(GENERAL, "tcpConnect():\n");
    if (stationScan.manual && stationScan.port == 0)
      return;  // We have been configured not to connect to the mesh

    if (WiFi.status() == WL_CONNECTED && WiFi.localIP()) {
      // Determine target IP and port for connection
      IPAddress targetIP = stationScan.manualIP ? stationScan.manualIP : WiFi.gatewayIP();
      uint16_t targetPort = stationScan.port;
      
      AsyncClient *pConn = new AsyncClient();
      painlessmesh::tcp::connect<Connection, painlessmesh::Mesh<Connection>>(
          (*pConn), targetIP, targetPort, (*this));
    } else {
      Log(ERROR, "tcpConnect(): err Something unexpected in tcpConnect()\n");
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
   * Set the minimum RSSI required for bridge election
   * 
   * Prevents nodes with poor router signal from becoming bridges in isolated
   * elections. When a node is the only candidate, it must meet this threshold.
   * When multiple candidates exist, the best RSSI wins regardless of threshold.
   * 
   * @param minRSSI Minimum RSSI in dBm (default: -80 dBm, range: -100 to -30)
   */
  void setMinimumBridgeRSSI(int8_t minRSSI) {
    if (minRSSI < -100) minRSSI = -100;
    if (minRSSI > -30) minRSSI = -30;
    minimumBridgeRSSI = minRSSI;
  }

  /**
   * Set callback for when this node's bridge role changes
   * 
   * @param callback Function to call when role changes
   */
  void onBridgeRoleChanged(std::function<void(bool isBridge, TSTRING reason)> callback) {
    bridgeRoleChangedCallback = callback;
  }

  /**
   * Enable or disable multi-bridge coordination mode
   * 
   * When enabled, multiple bridges can operate simultaneously for:
   * - Load balancing across multiple Internet connections
   * - Geographic distribution
   * - Hot standby redundancy without failover delays
   * 
   * @param enabled true to enable multi-bridge mode, false for single-bridge (default)
   */
  void enableMultiBridge(bool enabled) {
    multiBridgeEnabled = enabled;
    if (enabled) {
      Log(logger::GENERAL, "enableMultiBridge(): Multi-bridge coordination enabled\n");
    }
  }

  /**
   * Set bridge selection strategy for multi-bridge mode
   * 
   * @param strategy Selection strategy:
   *   - PRIORITY_BASED: Always use highest priority bridge (default)
   *   - ROUND_ROBIN: Distribute load evenly across bridges
   *   - BEST_SIGNAL: Always use bridge with best RSSI
   */
  void setBridgeSelectionStrategy(BridgeSelectionStrategy strategy) {
    bridgeSelectionStrategy = strategy;
    Log(logger::GENERAL, "setBridgeSelectionStrategy(): Strategy set to %d\n", (int)strategy);
  }

  /**
   * Set maximum number of concurrent bridges in multi-bridge mode
   * 
   * @param maxBridges Maximum bridges to track (default: 2, max: 5)
   */
  void setMaxBridges(uint8_t maxBridges) {
    if (maxBridges < 1) maxBridges = 1;
    if (maxBridges > 5) maxBridges = 5;
    maxConcurrentBridges = maxBridges;
    Log(logger::GENERAL, "setMaxBridges(): Max concurrent bridges set to %d\n", maxBridges);
  }

  /**
   * Get list of all active bridges (with Internet connection)
   * 
   * @return vector of node IDs for active bridges
   */
  std::vector<uint32_t> getActiveBridges() {
    std::vector<uint32_t> activeBridges;
    auto bridges = this->getBridges();
    
    for (const auto& bridge : bridges) {
      if (bridge.internetConnected && bridge.isHealthy()) {
        activeBridges.push_back(bridge.nodeId);
      }
    }
    
    return activeBridges;
  }

  /**
   * Get recommended bridge for message transmission
   * 
   * Uses the configured bridge selection strategy to pick the best bridge.
   * Returns 0 if no suitable bridge is available.
   * 
   * @return node ID of recommended bridge, or 0 if none available
   */
  uint32_t getRecommendedBridge() {
    auto activeBridges = getActiveBridges();
    
    if (activeBridges.empty()) {
      return 0;
    }
    
    // Single bridge - return it
    if (activeBridges.size() == 1) {
      return activeBridges[0];
    }
    
    // Multi-bridge mode: apply selection strategy
    switch (bridgeSelectionStrategy) {
      case ROUND_ROBIN: {
        // Simple round-robin: cycle through bridges
        lastSelectedBridgeIndex = (lastSelectedBridgeIndex + 1) % activeBridges.size();
        return activeBridges[lastSelectedBridgeIndex];
      }
      
      case BEST_SIGNAL: {
        // Find bridge with best RSSI
        uint32_t bestBridge = 0;
        int8_t bestRSSI = -127;
        
        for (const auto& bridge : this->getBridges()) {
          if (bridge.internetConnected && bridge.isHealthy() && bridge.routerRSSI > bestRSSI) {
            bestRSSI = bridge.routerRSSI;
            bestBridge = bridge.nodeId;
          }
        }
        return bestBridge;
      }
      
      case PRIORITY_BASED:
      default: {
        // Use highest priority bridge (stored in bridgePriorities map)
        uint32_t bestBridge = 0;
        uint8_t highestPriority = 0;
        
        for (uint32_t bridgeId : activeBridges) {
          uint8_t priority = bridgePriorities[bridgeId];
          if (priority > highestPriority) {
            highestPriority = priority;
            bestBridge = bridgeId;
          }
        }
        
        // If no priority info, use first active bridge
        return bestBridge ? bestBridge : activeBridges[0];
      }
    }
  }

  /**
   * Select a specific bridge for next transmission
   * 
   * This overrides the automatic bridge selection for one message.
   * 
   * @param bridgeNodeId Node ID of bridge to use
   */
  void selectBridge(uint32_t bridgeNodeId) {
    selectedBridgeOverride = bridgeNodeId;
  }

  /**
   * Check if multi-bridge mode is enabled
   * 
   * @return true if multi-bridge coordination is enabled
   */
  bool isMultiBridgeEnabled() const {
    return multiBridgeEnabled;
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
    
    // Register ourselves as a bridge in the knownBridges list
    // This ensures the bridge knows about itself and reports correct status
    this->addTask([this]() {
      // Check Internet connectivity: WiFi connected AND valid IP address
      bool hasInternet = (WiFi.status() == WL_CONNECTED) && 
                         (WiFi.localIP() != IPAddress(0, 0, 0, 0));
      
      this->updateBridgeStatus(
        this->nodeId,               // bridgeNodeId
        hasInternet,                // internetConnected
        WiFi.RSSI(),                // routerRSSI
        WiFi.channel(),             // routerChannel
        millis(),                   // uptime
        WiFi.gatewayIP().toString(),// gatewayIP
        this->getNodeTime()         // timestamp
      );
      
      Log(STARTUP, "initBridgeStatusBroadcast(): Registered self as bridge (nodeId: %u)\n", 
          this->nodeId);
    });
    
    // Create periodic task to broadcast bridge status
    bridgeStatusTask = this->addTask(
      this->bridgeStatusIntervalMs,
      TASK_FOREVER,
      [this]() {
        this->sendBridgeStatus();
      }
    );
    
    // Send immediate broadcast so nodes can discover this bridge right away
    // This ensures bridge is discoverable before the first periodic broadcast
    this->addTask([this]() {
      Log(STARTUP, "Sending initial bridge status broadcast\n");
      this->sendBridgeStatus();
    });
    
    // Also broadcast when new nodes connect so they can discover the bridge immediately
    // Add a small delay to ensure TCP connection is fully established and ready to receive data
    this->newConnectionCallbacks.push_back([this](uint32_t nodeId) {
      Log(CONNECTION, "New node %u connected, scheduling delayed bridge status\n", nodeId);
      this->addTask(1000, TASK_ONCE, [this, nodeId]() {
        Log(CONNECTION, "Sending bridge status to newly connected node %u\n", nodeId);
        this->sendBridgeStatus();
      });
    });
    
    Log(STARTUP, "Bridge status broadcast enabled (interval: %d ms)\n", 
        this->bridgeStatusIntervalMs);
  }

  /**
   * Initialize bridge coordination broadcasting
   * Sets up periodic coordination messages between bridges
   */
  void initBridgeCoordination() {
    using namespace logger;
    
    if (!this->isBridge() || !multiBridgeEnabled) {
      return;
    }
    
    Log(STARTUP, "initBridgeCoordination(): Setting up multi-bridge coordination\n");
    
    // Register our own priority in the bridgePriorities map
    // This ensures getRecommendedBridge() with PRIORITY_BASED strategy works correctly
    bridgePriorities[this->nodeId] = bridgePriority;
    Log(STARTUP, "initBridgeCoordination(): Registered self priority (nodeId: %u, priority: %d)\n",
        this->nodeId, bridgePriority);
    
    // Register handler for incoming coordination messages (Type 613)
    this->callbackList.onPackage(
        613,  // BRIDGE_COORDINATION type
        [this](protocol::Variant& variant, std::shared_ptr<Connection>, uint32_t) {
          JsonDocument doc;
          TSTRING str;
          variant.printTo(str);
          deserializeJson(doc, str);
          JsonObject obj = doc.as<JsonObject>();
          
          if (obj["priority"].is<unsigned int>()) {
            uint32_t fromNode = obj["from"];
            uint8_t priority = obj["priority"];
            TSTRING role = obj["role"].as<TSTRING>();
            uint8_t load = obj["load"] | 0;
            
            // Store bridge priority for selection decisions
            bridgePriorities[fromNode] = priority;
            
            // Update peer bridges list
            if (obj["peerBridges"].is<JsonArray>()) {
              JsonArray peers = obj["peerBridges"];
              for (JsonVariant peer : peers) {
                uint32_t peerId = peer.as<uint32_t>();
                if (peerId != this->nodeId && 
                    std::find(knownBridgePeers.begin(), knownBridgePeers.end(), peerId) == knownBridgePeers.end()) {
                  knownBridgePeers.push_back(peerId);
                }
              }
            }
            
            Log(CONNECTION, "Bridge coordination from %u: priority=%d, role=%s, load=%d%%\n",
                fromNode, priority, role.c_str(), load);
          }
          return false;  // Don't consume the package
        });
    
    // Create periodic task to send coordination messages
    bridgeCoordinationTask = this->addTask(
      30000,  // 30 seconds interval
      TASK_FOREVER,
      [this]() {
        this->sendBridgeCoordination();
      }
    );
    
    Log(STARTUP, "Bridge coordination enabled (priority: %d, role: %s)\n", 
        bridgePriority, bridgeRole.c_str());
  }

  /**
   * Send bridge coordination message to other bridges
   * Called periodically in multi-bridge mode
   */
  void sendBridgeCoordination() {
    using namespace logger;
    
    if (!this->isBridge() || !multiBridgeEnabled) {
      return;
    }
    
    // Calculate current load (simplified: based on node count)
    uint8_t currentLoad = 0;
    auto nodeCount = this->getNodeList(false).size();
    if (nodeCount > 0) {
      currentLoad = (nodeCount * 100) / MAX_CONN;
      if (currentLoad > 100) currentLoad = 100;
    }
    
    // Create coordination message
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    
    obj["type"] = 613;  // BRIDGE_COORDINATION
    obj["from"] = this->nodeId;
    obj["routing"] = 2;  // BROADCAST
    obj["priority"] = bridgePriority;
    obj["role"] = bridgeRole;
    obj["load"] = currentLoad;
    obj["timestamp"] = this->getNodeTime();
    obj["message_type"] = 613;
    
    // Add peer bridges list
    JsonArray peers = obj["peerBridges"].to<JsonArray>();
    for (uint32_t peerId : knownBridgePeers) {
      peers.add(peerId);
    }
    
    String msg;
    serializeJson(doc, msg);
    
    // Update our own priority in bridgePriorities map
    // This ensures priority-based selection always has current data
    bridgePriorities[this->nodeId] = bridgePriority;
    
    this->sendBroadcast(msg);
    
    Log(CONNECTION, "Bridge coordination sent: priority=%d, role=%s, load=%d%%\n",
        bridgePriority, bridgeRole.c_str(), currentLoad);
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
    JsonDocument doc;
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
    
    // Validate RSSI threshold for single-candidate elections
    // When only one candidate exists, it indicates the node is isolated from the mesh.
    // In this case, require minimum signal quality to prevent poor connections.
    // When multiple candidates exist, the mesh is connected and best RSSI wins.
    if (electionCandidates.size() == 1 && winner->routerRSSI < minimumBridgeRSSI) {
      Log(CONNECTION, "=== Election Failed: Insufficient Signal Quality ===\n");
      Log(CONNECTION, "  Single candidate with RSSI %d dBm (minimum required: %d dBm)\n",
          winner->routerRSSI, minimumBridgeRSSI);
      Log(CONNECTION, "  Node is isolated from mesh with poor router signal\n");
      Log(CONNECTION, "  Rejecting election to prevent unreliable bridge\n");
      Log(CONNECTION, "  Recommendation: Move closer to router or wait for mesh connection\n");
      
      electionState = ELECTION_IDLE;
      electionCandidates.clear();
      
      // Notify via callback that election failed
      if (bridgeRoleChangedCallback) {
        bridgeRoleChangedCallback(false, "Insufficient signal quality for isolated bridge");
      }
      return;
    }
    
    Log(CONNECTION, "=== Election Winner: Node %u ===\n", winner->nodeId);
    Log(CONNECTION, "  Router RSSI: %d dBm\n", winner->routerRSSI);
    Log(CONNECTION, "  Uptime: %u ms\n", winner->uptime);
    Log(CONNECTION, "  Free Memory: %u bytes\n", winner->freeMemory);
    
    // Record election in diagnostics history
    if (this->diagnosticsEnabled) {
      ElectionRecord record;
      record.timestamp = millis();
      record.winnerNodeId = winner->nodeId;
      record.winnerRSSI = winner->routerRSSI;
      record.candidateCount = electionCandidates.size();
      record.reason = "Bridge failure detected";
      
      this->electionHistory.push_back(record);
      
      // Keep history limited to MAX_ELECTION_HISTORY
      if (this->electionHistory.size() > this->MAX_ELECTION_HISTORY) {
        this->electionHistory.erase(this->electionHistory.begin());
      }
      
      Log(CONNECTION, "evaluateElection(): Election recorded in history\n");
    }
    
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
    JsonDocument doc;
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
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    
    obj["type"] = 610;  // BRIDGE_STATUS type
    obj["from"] = this->nodeId;
    obj["routing"] = 2;  // BROADCAST routing
    obj["timestamp"] = this->getNodeTime();
    
    // Check Internet connectivity: WiFi connected AND valid IP address
    // We check for valid local IP instead of gateway IP because:
    // 1. Gateway IP might not be immediately available after connection
    // 2. Some networks (mobile hotspots) may not provide gateway IP via DHCP
    // 3. Having a valid local IP + being connected is sufficient for internet access
    bool hasInternet = (WiFi.status() == WL_CONNECTED) && 
                       (WiFi.localIP() != IPAddress(0, 0, 0, 0));
    obj["internetConnected"] = hasInternet;
    
    int8_t rssi = WiFi.RSSI();
    uint8_t channel = WiFi.channel();
    uint32_t uptime = millis();
    TSTRING gatewayIP = WiFi.gatewayIP().toString();
    
    obj["routerRSSI"] = rssi;
    obj["routerChannel"] = channel;
    obj["uptime"] = uptime;
    obj["gatewayIP"] = gatewayIP;
    obj["message_type"] = 610;
    
    String msg;
    serializeJson(doc, msg);
    
    Log(GENERAL, "sendBridgeStatus(): Broadcasting status (Internet: %s)\n",
        hasInternet ? "Connected" : "Disconnected");
    Log(GENERAL, "sendBridgeStatus(): WiFi status=%d, localIP=%s, gatewayIP=%s\n",
        WiFi.status(), WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str());
    
    // Update our own bridge status in knownBridges list
    // This ensures the bridge reports itself correctly when queried
    this->updateBridgeStatus(this->nodeId, hasInternet, rssi, channel, 
                            uptime, gatewayIP, this->getNodeTime());
    
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
            // Handle station disconnect completion after callbacks
            this->handleStationDisconnectComplete();
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
          // Handle station disconnect completion after callbacks
          this->handleStationDisconnectComplete();
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
  
  // Station disconnect handling state
  bool _pendingStationReconnect = false;

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
  int8_t minimumBridgeRSSI = -80;  // Default -80 dBm minimum for isolated elections
  uint32_t lastRoleChangeTime = 0;
  ElectionState electionState = ELECTION_IDLE;
  uint32_t electionDeadline = 0;
  std::vector<BridgeCandidate> electionCandidates;
  std::function<void(bool isBridge, TSTRING reason)> bridgeRoleChangedCallback;

  // Multi-bridge coordination state and configuration
 protected:
  bool multiBridgeEnabled = false;
  BridgeSelectionStrategy bridgeSelectionStrategy = PRIORITY_BASED;
  uint8_t maxConcurrentBridges = 2;
  uint8_t bridgePriority = 5;  // Default medium priority
  TSTRING bridgeRole = "secondary";  // Default role
  std::shared_ptr<Task> bridgeCoordinationTask;
  std::map<uint32_t, uint8_t> bridgePriorities;  // nodeId -> priority mapping
  std::vector<uint32_t> knownBridgePeers;  // List of peer bridge node IDs
  uint32_t selectedBridgeOverride = 0;  // Manual bridge selection override
  size_t lastSelectedBridgeIndex = 0;  // For round-robin selection
};
}  // namespace wifi
};  // namespace painlessmesh

#endif

#endif
