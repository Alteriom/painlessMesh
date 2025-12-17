#ifndef _PAINLESS_MESH_ARDUINO_WIFI_HPP_
#define _PAINLESS_MESH_ARDUINO_WIFI_HPP_

#include "painlessmesh/configuration.hpp"

#include "painlessmesh/logger.hpp"
#ifdef PAINLESSMESH_ENABLE_ARDUINO_WIFI
#include "painlessMeshSTA.h"

#include "painlessmesh/callback.hpp"
#include "painlessmesh/gateway.hpp"
#include "painlessmesh/mesh.hpp"
#include "painlessmesh/router.hpp"
#include "painlessmesh/tcp.hpp"

#ifdef ESP32
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#elif defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#endif

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

    // Add bridge election package handler (Type BRIDGE_ELECTION)
    this->callbackList.onPackage(
        protocol::BRIDGE_ELECTION,
        [this](protocol::Variant& variant, std::shared_ptr<Connection>,
               uint32_t) {
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

            this->handleBridgeElection(fromNode, routerRSSI, uptime,
                                       freeMemory);

            Log(CONNECTION, "Bridge election candidate from %u: RSSI %d dBm\n",
                fromNode, routerRSSI);
          }
          return false;  // Don't consume the package
        });

    // Add bridge takeover package handler (Type BRIDGE_TAKEOVER)
    this->callbackList.onPackage(
        protocol::BRIDGE_TAKEOVER,
        [this](protocol::Variant& variant, std::shared_ptr<Connection>,
               uint32_t) {
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
    this->onBridgeStatusChanged([this](uint32_t bridgeNodeId,
                                       bool hasInternet) {
      if (!hasInternet && bridgeFailoverEnabled &&
          routerCredentialsConfigured) {
        Log(CONNECTION, "Bridge %u lost Internet, considering election...\n",
            bridgeNodeId);

        // Check if we still have any healthy bridges
        if (!this->hasInternetConnection()) {
          Log(CONNECTION, "No healthy bridges, starting election\n");
          // Small delay to let all nodes detect the failure
          this->addTask(2000, TASK_ONCE,
                        [this]() { this->startBridgeElection(); });
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

      // Skip check during startup period to allow initial bridge discovery
      if (millis() < electionStartupDelayMs) {
        return;
      }

      // IMPORTANT: Don't trigger election if we're disconnected from the mesh
      // When isolated, we can't receive bridge status broadcasts, so lack of
      // healthy bridge could simply mean WE are disconnected, not that the
      // bridge is unavailable. Wait until mesh connectivity is restored before
      // considering an election.
      if (!this->hasActiveMeshConnections()) {
        Log(CONNECTION,
            "Bridge monitor: Skipping - no active mesh connections\n");
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
        Log(CONNECTION,
            "Bridge monitor: No healthy bridge detected, triggering "
            "election\n");
        // Random delay to prevent simultaneous elections when multiple nodes
        // start together
        uint32_t randomDelay =
            random(electionRandomDelayMinMs, electionRandomDelayMaxMs);
        Log(CONNECTION, "Bridge monitor: Scheduling election in %u ms\n",
            randomDelay);
        this->addTask(randomDelay, TASK_ONCE,
                      [this]() { this->startBridgeElection(); });
      }
    });

    // Add separate periodic task for isolated bridge retry
    // This handles the case where a node:
    // - Has router credentials configured
    // - Is isolated (no mesh connections)
    // - Should attempt to become a bridge directly
    // This is different from the election mechanism which requires mesh
    // connectivity
    this->addTask(isolatedBridgeRetryIntervalMs, TASK_FOREVER, [this]() {
      // Only retry if failover is enabled and we have credentials
      if (!bridgeFailoverEnabled || !routerCredentialsConfigured) {
        return;
      }

      // Don't retry if we're already a bridge
      if (this->isBridge()) {
        return;
      }

      // Skip during startup period
      if (millis() < electionStartupDelayMs) {
        return;
      }

      // Only retry when isolated (no mesh connections found)
      if (this->hasActiveMeshConnections()) {
        // Reset retry counter and pending flag when mesh is active (no longer
        // isolated)
        _isolatedBridgeRetryAttempts = 0;
        _isolatedRetryPending = false;
        return;
      }

      // Limit retry attempts with reset after timeout
      if (_isolatedBridgeRetryAttempts >= MAX_ISOLATED_BRIDGE_RETRY_ATTEMPTS) {
        // Check if enough time has passed to reset the counter
        if (millis() > _isolatedBridgeRetryResetTime) {
          Log(CONNECTION,
              "Isolated bridge retry: Reset timeout reached, resetting attempt "
              "counter\n");
          _isolatedBridgeRetryAttempts = 0;
        } else {
          Log(CONNECTION,
              "Isolated bridge retry: Max attempts (%d) reached, reset in %u "
              "seconds\n",
              MAX_ISOLATED_BRIDGE_RETRY_ATTEMPTS,
              (_isolatedBridgeRetryResetTime - millis()) / 1000);
          return;
        }
      }

      // Check if mesh network exists on any channel before trying to become
      // bridge If mesh exists but we can't connect, don't try to become bridge
      // Skip this check if we already confirmed isolation from a previous
      // failed attempt
      uint16_t emptyScans = stationScan.getConsecutiveEmptyScans();
      if (!_isolatedRetryPending &&
          emptyScans < ISOLATED_BRIDGE_RETRY_SCAN_THRESHOLD) {
        Log(CONNECTION,
            "Isolated bridge retry: Only %d empty scans, waiting for more "
            "scans\n",
            emptyScans);
        return;
      }

      // Clear the pending flag now that we're proceeding
      _isolatedRetryPending = false;

      Log(CONNECTION,
          "Isolated bridge retry: Node isolated with %d empty scans, "
          "attempting bridge promotion\n",
          emptyScans);

      // Attempt to become bridge directly (bypassing election since we're
      // isolated) Only increment retry counter if we actually attempted
      // promotion
      if (this->attemptIsolatedBridgePromotion()) {
        _isolatedBridgeRetryAttempts++;
        // Set reset time when reaching max attempts
        if (_isolatedBridgeRetryAttempts >=
            MAX_ISOLATED_BRIDGE_RETRY_ATTEMPTS) {
          _isolatedBridgeRetryResetTime =
              millis() + isolatedBridgeRetryResetIntervalMs;
        }
      }
    });

    eventHandleInit();

    _apIp = IPAddress(0, 0, 0, 0);

    if (connectMode & WIFI_AP) {
      apInit(nodeId);  // setup AP
    }

    // Initialize TCP server AFTER AP is configured
    // This ensures the network interfaces are ready when the server starts
    // Fixes TCP connection error -14 when nodes try to connect to bridge
    tcpServerInit();

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
  void init(TSTRING ssid, TSTRING password, Scheduler* baseScheduler,
            uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA,
            uint8_t channel = 1, uint8_t hidden = 0, uint8_t maxconn = MAX_CONN,
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
  bool initAsBridge(TSTRING meshSSID, TSTRING meshPassword, TSTRING routerSSID,
                    TSTRING routerPassword, Scheduler* baseScheduler,
                    uint16_t port = 5555) {
    using namespace logger;

    Log(STARTUP, "=== Bridge Mode Initialization ===\n");
    Log(STARTUP, "Step 1: Attempting to connect to router %s...\n", routerSSID.c_str());

    // Store router credentials for future connection attempts
    setRouterCredentials(routerSSID, routerPassword);

    // Step 1: Attempt to connect to router first to detect its channel
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
    bool routerConnected = false;

    if (WiFi.status() == WL_CONNECTED) {
      detectedChannel = WiFi.channel();
      // Validate channel is in valid range (1-13 for 2.4GHz)
      if (detectedChannel < 1 || detectedChannel > 13) {
        Log(ERROR,
            "\n✗ Invalid channel detected: %d, falling back to channel 1\n",
            detectedChannel);
        detectedChannel = 1;
      } else {
        Log(STARTUP, "\n✓ Router connected on channel %d\n", detectedChannel);
        Log(STARTUP, "✓ Router IP: %s\n", WiFi.localIP().toString().c_str());
        routerConnected = true;
      }
    } else {
      Log(STARTUP, "\n⚠ Router connection unavailable during initialization\n");
      
      // Scan for router to detect its channel even though we can't connect
      // This minimizes channel mismatch when router becomes available later
      Log(STARTUP, "⚠ Scanning for router '%s' to detect channel...\n", routerSSID.c_str());
      
      // ESP32 and ESP8266 have different scanNetworks signatures
#ifdef ESP32
      int16_t numNetworks = WiFi.scanNetworks(false, false, false, 300U, 0);
#elif defined(ESP8266)
      int16_t numNetworks = WiFi.scanNetworks(false, false, 0);
#endif
      
      if (numNetworks > 0) {
        for (int16_t i = 0; i < numNetworks; i++) {
          if (WiFi.SSID(i) == routerSSID) {
            uint8_t scannedChannel = WiFi.channel(i);
            if (scannedChannel >= 1 && scannedChannel <= 13) {
              detectedChannel = scannedChannel;
              Log(STARTUP, "✓ Router found on channel %d (not connected, will retry)\n", 
                  detectedChannel);
              break;
            }
          }
        }
        WiFi.scanDelete();
      }
      
      if (detectedChannel == 1) {
        Log(STARTUP, "⚠ Router not found in scan, using default channel %d\n", detectedChannel);
      }
      
      Log(STARTUP, "⚠ Proceeding with bridge setup on channel %d\n", detectedChannel);
      Log(STARTUP, "⚠ Bridge will retry router connection in background\n");
    }

    Log(STARTUP, "Step 2: Initializing mesh on channel %d...\n",
        detectedChannel);

    // Step 2: Initialize mesh on detected/default channel
    // This allows the bridge to establish the mesh network even without router
    init(meshSSID, meshPassword, baseScheduler, port, WIFI_AP_STA,
         detectedChannel, 0, MAX_CONN);

    // Allow network stack to stabilize after AP and TCP server initialization
    // This prevents TCP connection errors (-14) when nodes connect
    delay(100);

    Log(STARTUP, "Step 3: Establishing bridge connection...\n");

    // Step 3: Establish/re-establish router connection using stationManual
    // If router wasn't available initially, this will be retried automatically
    stationManual(routerSSID, routerPassword, 0);

    // Step 4: Configure as root/bridge node
    // Bridge role is established regardless of router connectivity
    // This ensures mesh nodes can connect and the bridge can provide mesh services
    this->setRoot(true);
    this->setContainsRoot(true);

    // Step 5: Setup bridge status broadcasting
    initBridgeStatusBroadcast();

    // Step 6: Setup gateway Internet handler
    initGatewayInternetHandler();

    Log(STARTUP, "=== Bridge Mode Active ===\n");
    Log(STARTUP, "  Mesh SSID: %s\n", meshSSID.c_str());
    Log(STARTUP, "  Mesh Channel: %d%s\n", detectedChannel, 
        routerConnected ? " (matches router)" : " (default, router pending)");
    Log(STARTUP, "  Router: %s%s\n", routerSSID.c_str(),
        routerConnected ? " (connected)" : " (will retry)");
    Log(STARTUP, "  Port: %d\n", port);
    
    if (!routerConnected) {
      Log(STARTUP, "\nINFO: Bridge initialized without router connection\n");
      Log(STARTUP, "INFO: Mesh network is active and accepting node connections\n");
      Log(STARTUP, "INFO: Router connection will be established automatically when available\n");
    }
    
    // Return true - bridge mesh functionality is active even without router
    // The mesh network is operational and nodes can connect
    // Router connection will be retried automatically via stationManual
    return true;
  }

  /**
   * Initialize mesh as a bridge node with priority (for multi-bridge mode)
   *
   * This overload adds bridge priority configuration for multi-bridge
   * deployments. Priority determines which bridge is preferred when multiple
   * bridges are available.
   *
   * @param meshSSID The name of your mesh network
   * @param meshPassword WiFi password for the mesh
   * @param routerSSID SSID of the router to connect to
   * @param routerPassword Password for the router
   * @param baseScheduler Task scheduler for mesh operations
   * @param port TCP port for mesh communication (default: 5555)
   * @param priority Bridge priority: 10=highest (primary), 5=medium
   * (secondary), 1=lowest (default: 5)
   */
  bool initAsBridge(TSTRING meshSSID, TSTRING meshPassword, TSTRING routerSSID,
                    TSTRING routerPassword, Scheduler* baseScheduler,
                    uint16_t port, uint8_t priority) {
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

    Log(STARTUP,
        "=== Bridge Mode Initialization (Priority: %d, Role: %s) ===\n",
        priority, bridgeRole.c_str());

    // Call the base initAsBridge method
    bool success = initAsBridge(meshSSID, meshPassword, routerSSID,
                                routerPassword, baseScheduler, port);

    // Setup multi-bridge coordination if enabled and bridge init succeeded
    if (success && multiBridgeEnabled) {
      initBridgeCoordination();
    }

    return success;
  }

  /**
   * Initialize mesh as a shared gateway node
   *
   * This method initializes all mesh nodes in AP+STA mode with router
   * connectivity. Unlike initAsBridge() which creates a single bridge node,
   * initAsSharedGateway() allows all nodes to connect to the router while
   * maintaining mesh communication.
   *
   * Key features:
   * - All nodes operate in AP+STA mode
   * - All nodes connect to the same router
   * - Mesh and router operate on the same channel for reliability
   * - Automatic router reconnection on disconnect
   * - Channel synchronization between mesh and router
   *
   * @param meshPrefix The name prefix for the mesh network
   * @param meshPassword WiFi password for the mesh network
   * @param routerSSID SSID of the router to connect to
   * @param routerPassword Password for the router
   * @param userScheduler Task scheduler for mesh operations
   * @param port TCP port for mesh communication (default: 5555)
   * @param config SharedGatewayConfig with advanced settings (optional)
   * @return true if initialization succeeded, false otherwise
   */
  bool initAsSharedGateway(
      TSTRING meshPrefix, TSTRING meshPassword, TSTRING routerSSID,
      TSTRING routerPassword, Scheduler* userScheduler, uint16_t port = 5555,
      gateway::SharedGatewayConfig config = gateway::SharedGatewayConfig()) {
    using namespace logger;

    Log(STARTUP, "=== Shared Gateway Mode Initialization ===\n");

    // Validate configuration if enabled
    if (config.enabled) {
      auto result = config.validate();
      if (!result.valid) {
        Log(ERROR, "initAsSharedGateway(): Config validation failed: %s\n",
            result.errorMessage.c_str());
        return false;
      }
    }

    // Store shared gateway configuration
    _sharedGatewayConfig = config;
    _sharedGatewayConfig.routerSSID = routerSSID;
    _sharedGatewayConfig.routerPassword = routerPassword;
    _sharedGatewayConfig.enabled = true;
    _sharedGatewayMode = true;

    Log(STARTUP, "Step 1: Scanning for router %s to detect channel...\n",
        routerSSID.c_str());

    // Step 1: Scan for router to detect its channel
    // We need to ensure mesh and router operate on the same channel
    if (WiFi.status() != WL_DISCONNECTED) WiFi.disconnect();

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    WiFi.setAutoReconnect(false);
    Log(STARTUP, "initAsSharedGateway(): AutoReconnect disabled\n");
#else
    WiFi.setAutoConnect(false);
    Log(STARTUP, "initAsSharedGateway(): AutoConnect disabled\n");
#endif
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    // Connect to router to detect channel
    WiFi.begin(routerSSID.c_str(), routerPassword.c_str());

    // Wait for connection with timeout (using constant for configurability)
    int timeout = ROUTER_CONNECTION_TIMEOUT_SECONDS;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
      delay(1000);
      timeout--;
      Log(STARTUP, ".");
    }

    uint8_t detectedChannel = 1;  // Default fallback

    if (WiFi.status() == WL_CONNECTED) {
      detectedChannel = WiFi.channel();
      // Validate channel is in valid range (1-14 for 2.4GHz, region-dependent)
      if (detectedChannel < MIN_WIFI_CHANNEL ||
          detectedChannel > MAX_WIFI_CHANNEL) {
        Log(ERROR,
            "\n✗ Invalid channel detected: %d, falling back to channel 1\n",
            detectedChannel);
        detectedChannel = 1;
      } else {
        Log(STARTUP, "\n✓ Router connected on channel %d\n", detectedChannel);
        Log(STARTUP, "✓ Router IP: %s\n", WiFi.localIP().toString().c_str());
      }
    } else {
      Log(ERROR, "\n✗ Failed to connect to router during channel detection\n");
      Log(ERROR,
          "Continuing with default channel 1, will retry router connection "
          "later\n");
    }

    // Disconnect from router, we'll reconnect after mesh init
    WiFi.disconnect();
    delay(100);

    Log(STARTUP, "Step 2: Initializing mesh on channel %d...\n",
        detectedChannel);

    // Step 2: Initialize mesh on detected channel with AP+STA mode
    // Set scheduler before init
    this->setScheduler(userScheduler);
    init(meshPrefix, meshPassword, port, WIFI_AP_STA, detectedChannel, 0,
         MAX_CONN);

    // Allow network stack to stabilize after AP and TCP server initialization
    // This prevents TCP connection errors (-14) when nodes connect
    delay(100);

    Log(STARTUP,
        "Step 3: Establishing router connection in shared gateway mode...\n");

    // Step 3: Establish router connection using stationManual
    // Port 0 means we don't expect TCP mesh connection to the router
    stationManual(routerSSID, routerPassword, 0);

    // Step 4: Setup router connection monitoring and reconnection logic
    initSharedGatewayMonitoring();

    // Step 5: Setup gateway Internet handler
    initGatewayInternetHandler();

    // Store router credentials for reconnection
    setRouterCredentials(routerSSID, routerPassword);

    Log(STARTUP, "=== Shared Gateway Mode Active ===\n");
    Log(STARTUP, "  Mesh Prefix: %s\n", meshPrefix.c_str());
    Log(STARTUP, "  Mesh Channel: %d (synced with router)\n", detectedChannel);
    Log(STARTUP, "  Router: %s\n", routerSSID.c_str());
    Log(STARTUP, "  Port: %d\n", port);
    Log(STARTUP, "  Mode: AP+STA (all nodes can connect to router)\n");

    return true;
  }

  /**
   * Check if shared gateway mode is enabled
   *
   * @return true if node is operating in shared gateway mode
   */
  bool isSharedGatewayMode() const { return _sharedGatewayMode; }

  /**
   * Get the shared gateway configuration
   *
   * @return const reference to the SharedGatewayConfig
   */
  const gateway::SharedGatewayConfig& getSharedGatewayConfig() const {
    return _sharedGatewayConfig;
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
   *
   * TCP Connection Retry:
   * The TCP connection now includes automatic retry with exponential backoff.
   * If the initial connection fails (error -14 ERR_CONN or other errors),
   * the system will retry up to TCP_CONNECT_MAX_RETRIES times before
   * triggering a full WiFi reconnection cycle. This helps handle:
   * - Timing issues where TCP server is not ready immediately
   * - Network stack stabilization after IP acquisition
   * - Transient network conditions
   */
  void tcpConnect() {
    using namespace logger;
    Log(GENERAL, "tcpConnect():\n");
    if (stationScan.manual && stationScan.port == 0)
      return;  // We have been configured not to connect to the mesh

    if (WiFi.status() == WL_CONNECTED && WiFi.localIP()) {
      // Determine target IP and port for connection
      IPAddress targetIP =
          stationScan.manualIP ? stationScan.manualIP : WiFi.gatewayIP();
      uint16_t targetPort = stationScan.port;

      Log(CONNECTION, "tcpConnect(): Connecting to %s:%d\n",
          targetIP.toString().c_str(), targetPort);

      // Add a small stabilization delay before attempting TCP connection
      // This helps prevent error -14 (ERR_CONN) by allowing the network stack
      // and TCP server to be fully ready. The delay is added via task scheduler
      // to avoid blocking the event loop.
      this->addTask(
          painlessmesh::tcp::TCP_CONNECT_STABILIZATION_DELAY_MS, TASK_ONCE,
          [this, targetIP, targetPort]() {
            // Verify WiFi is still connected after the delay
            if (WiFi.status() != WL_CONNECTED || !WiFi.localIP()) {
              Log(CONNECTION,
                  "tcpConnect(): WiFi disconnected during stabilization "
                  "delay\n");
              return;
            }

            Log(CONNECTION,
                "tcpConnect(): Starting TCP connection after stabilization\n");
            AsyncClient* pConn = new AsyncClient();
            painlessmesh::tcp::connect<Connection,
                                       painlessmesh::Mesh<Connection>>(
                (*pConn), targetIP, targetPort, (*this));
          });
    } else {
      Log(ERROR, "tcpConnect(): err Something unexpected in tcpConnect()\n");
    }
  }

  bool setHostname(const char* hostname) {
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
   * @param enabled true to enable automatic failover (default), false to
   * disable
   */
  void enableBridgeFailover(bool enabled) { bridgeFailoverEnabled = enabled; }

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
  void setElectionTimeout(uint32_t timeoutMs) { electionTimeoutMs = timeoutMs; }

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
   * Set the startup delay before first bridge election check
   *
   * Allows time for mesh network formation before starting bridge elections.
   * Longer delays reduce the risk of split-brain scenarios when multiple nodes
   * start simultaneously, ensuring nodes discover each other before elections.
   *
   * @param delayMs Startup delay in milliseconds (default: 60000 = 60 seconds,
   * min: 10000)
   */
  void setElectionStartupDelay(uint32_t delayMs) {
    if (delayMs < 10000) delayMs = 10000;  // Minimum 10 seconds
    electionStartupDelayMs = delayMs;
  }

  /**
   * Set the random delay range for bridge elections
   *
   * When multiple nodes detect missing bridge simultaneously, randomized delays
   * prevent all nodes from starting elections at the same instant. Longer
   * delays provide more time for mesh discovery and reduce split-brain risk.
   *
   * @param minMs Minimum random delay in milliseconds (default: 1000 = 1
   * second)
   * @param maxMs Maximum random delay in milliseconds (default: 3000 = 3
   * seconds)
   */
  void setElectionRandomDelay(uint32_t minMs, uint32_t maxMs) {
    if (minMs < 100) minMs = 100;             // Minimum 100ms
    if (maxMs < minMs) maxMs = minMs + 1000;  // Ensure max > min
    electionRandomDelayMinMs = minMs;
    electionRandomDelayMaxMs = maxMs;
  }

  /**
   * Set callback for when this node's bridge role changes
   *
   * @param callback Function to call when role changes
   */
  void onBridgeRoleChanged(
      std::function<void(bool isBridge, const TSTRING& reason)> callback) {
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
   * @param enabled true to enable multi-bridge mode, false for single-bridge
   * (default)
   */
  void enableMultiBridge(bool enabled) {
    multiBridgeEnabled = enabled;
    if (enabled) {
      Log(logger::GENERAL,
          "enableMultiBridge(): Multi-bridge coordination enabled\n");
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
    Log(logger::GENERAL, "setBridgeSelectionStrategy(): Strategy set to %d\n",
        (int)strategy);
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
    Log(logger::GENERAL, "setMaxBridges(): Max concurrent bridges set to %d\n",
        maxBridges);
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
   * Check if any bridge/gateway in the mesh has Internet connectivity
   *
   * IMPORTANT: This method checks if a GATEWAY node (bridge) in the mesh has
   * Internet access, NOT whether THIS node can directly make HTTP/HTTPS
   * requests.
   *
   * Regular mesh nodes do NOT have direct IP routing to the Internet. They only
   * communicate via the painlessMesh protocol. To send data to the Internet
   * from a regular node, you must use sendToInternet() which routes through a
   * gateway, or use initAsSharedGateway(meshSSID, meshPwd, ROUTER_SSID,
   * ROUTER_PWD, scheduler, port) to give all nodes direct router access
   * (requires router credentials).
   *
   * Override of base class method to also check if THIS node is a bridge
   * with Internet connectivity, not just other bridges in the mesh.
   *
   * \code
   * if (mesh.hasInternetConnection()) {
   *   // A gateway exists - use sendToInternet() to reach Internet
   *   mesh.sendToInternet("https://api.example.com", data, callback);
   * }
   *
   * // DON'T DO THIS on regular nodes - will fail with "connection refused":
   * // HTTPClient http;
   * // http.begin("https://api.example.com");
   * \endcode
   *
   * @return true if at least one bridge (including this node) has Internet
   * @see hasLocalInternet() to check if THIS node has direct Internet access
   * @see sendToInternet() to send data to Internet via gateway
   * @see initAsSharedGateway() requires router credentials (ROUTER_SSID,
   * ROUTER_PASSWORD)
   */
  bool hasInternetConnection() {
    // First check if THIS node is a bridge with Internet
    if (this->isBridge()) {
      // Check Internet connectivity: WiFi connected AND valid IP address
      bool hasInternet = (WiFi.status() == WL_CONNECTED) &&
                         (WiFi.localIP() != IPAddress(0, 0, 0, 0));
      if (hasInternet) {
        return true;
      }
    }

    // Then check other bridges in the mesh (call parent implementation)
    return painlessmesh::Mesh<Connection>::hasInternetConnection();
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
        lastSelectedBridgeIndex =
            (lastSelectedBridgeIndex + 1) % activeBridges.size();
        return activeBridges[lastSelectedBridgeIndex];
      }

      case BEST_SIGNAL: {
        // Find bridge with best RSSI
        uint32_t bestBridge = 0;
        int8_t bestRSSI = -127;

        for (const auto& bridge : this->getBridges()) {
          if (bridge.internetConnected && bridge.isHealthy() &&
              bridge.routerRSSI > bestRSSI) {
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
  bool isMultiBridgeEnabled() const { return multiBridgeEnabled; }

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

  void init(Scheduler* scheduler, uint32_t id) {
    painlessmesh::Mesh<Connection>::init(scheduler, id);
  }

  void init(uint32_t id) { painlessmesh::Mesh<Connection>::init(id); }

  void apInit(uint32_t nodeId) {
    using namespace logger;
    _apIp = IPAddress(10, (nodeId & 0xFF00) >> 8, (nodeId & 0xFF), 1);
    IPAddress netmask(255, 255, 255, 0);

    WiFi.softAPConfig(_apIp, _apIp, netmask);

#ifdef ESP32
    // ESP32: Explicitly enable AP mode to ensure DHCP server starts properly
    // This is particularly important after channel changes or AP restarts
    WiFi.enableAP(true);
#endif

    WiFi.softAP(_meshSSID.c_str(), _meshPassword.c_str(), _meshChannel,
                _meshHidden, _meshMaxConn);

    Log(STARTUP, "apInit(): AP configured - SSID: %s, Channel: %d, IP: %s\n",
        _meshSSID.c_str(), _meshChannel, _apIp.toString().c_str());
    Log(STARTUP, "apInit(): AP active - Max connections: %d\n", _meshMaxConn);
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

    Log(STARTUP,
        "initBridgeStatusBroadcast(): Setting up bridge status broadcast\n");

    // Register ourselves as a bridge in the knownBridges list
    // This ensures the bridge knows about itself and reports correct status
    this->addTask([this]() {
      // Check Internet connectivity: WiFi connected AND valid IP address
      bool hasInternet = (WiFi.status() == WL_CONNECTED) &&
                         (WiFi.localIP() != IPAddress(0, 0, 0, 0));

      this->updateBridgeStatus(this->nodeId,    // bridgeNodeId
                               hasInternet,     // internetConnected
                               WiFi.RSSI(),     // routerRSSI
                               WiFi.channel(),  // routerChannel
                               millis(),        // uptime
                               WiFi.gatewayIP().toString(),  // gatewayIP
                               this->getNodeTime()           // timestamp
      );

      Log(STARTUP,
          "initBridgeStatusBroadcast(): Registered self as bridge (nodeId: "
          "%u)\n",
          this->nodeId);
    });

    // Create periodic task to broadcast bridge status
    bridgeStatusTask = this->addTask(this->bridgeStatusIntervalMs, TASK_FOREVER,
                                     [this]() { this->sendBridgeStatus(); });

    // Send immediate broadcast so nodes can discover this bridge right away
    // This ensures bridge is discoverable before the first periodic broadcast
    this->addTask([this]() {
      Log(STARTUP, "Sending initial bridge status broadcast\n");
      this->sendBridgeStatus();
    });

    // Also send bridge status when new nodes connect so they can discover the
    // bridge immediately Send directly to the new node to ensure delivery,
    // independent of time sync Using changedConnectionCallbacks instead of
    // newConnectionCallbacks ensures routing is ready
    this->changedConnectionCallbacks.push_back([this](uint32_t nodeId) {
      Log(CONNECTION,
          "Node %u connection changed, sending bridge status directly\n",
          nodeId);

      // Small delay to ensure connection is fully stable, then send directly to
      // the new node This avoids issues with time sync blocking broadcast
      // messages
      this->addTask(500, TASK_ONCE, [this, nodeId]() {
        // Check if the connection is still valid - the node may have
        // disconnected during the 500ms delay (e.g., due to timeout or network
        // issues) This prevents attempting to send messages to dropped
        // connections findRoute returns nullptr if node is not in the routing
        // table
        auto conn = router::findRoute<Connection>((*this), nodeId);
        if (!conn || !conn->connected()) {
          Log(CONNECTION,
              "Bridge status send cancelled: Node %u no longer connected\n",
              nodeId);
          return;
        }

        // Create bridge status message
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();

        obj["type"] = protocol::BRIDGE_STATUS;
        obj["from"] = this->nodeId;
        obj["routing"] = 1;  // SINGLE routing (direct to node)
        obj["dest"] = nodeId;
        obj["timestamp"] = this->getNodeTime();

        bool hasInternet = (WiFi.status() == WL_CONNECTED) &&
                           (WiFi.localIP() != IPAddress(0, 0, 0, 0));
        obj["internetConnected"] = hasInternet;
        obj["routerRSSI"] = WiFi.RSSI();
        obj["routerChannel"] = WiFi.channel();
        obj["uptime"] = millis();
        obj["gatewayIP"] = WiFi.gatewayIP().toString();
        obj["message_type"] = protocol::BRIDGE_STATUS;

        String msg;
        serializeJson(doc, msg);

        Log(CONNECTION,
            "Sending bridge status directly to node %u (Internet: %s)\n",
            nodeId, hasInternet ? "YES" : "NO");

        // Send directly to the connection with high priority
        // This ensures the message is sent immediately rather than queued
        // The JSON message format is the same as what router::send() produces
        // (Variant serializes to the same JSON format we built manually)
        conn->addMessage(msg, true);
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

    Log(STARTUP,
        "initBridgeCoordination(): Setting up multi-bridge coordination\n");

    // Register our own priority in the bridgePriorities map
    // This ensures getRecommendedBridge() with PRIORITY_BASED strategy works
    // correctly
    bridgePriorities[this->nodeId] = bridgePriority;
    Log(STARTUP,
        "initBridgeCoordination(): Registered self priority (nodeId: %u, "
        "priority: %d)\n",
        this->nodeId, bridgePriority);

    // Register handler for incoming coordination messages (Type 613)
    this->callbackList.onPackage(
        613,  // BRIDGE_COORDINATION type
        [this](protocol::Variant& variant, std::shared_ptr<Connection>,
               uint32_t) {
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
                    std::find(knownBridgePeers.begin(), knownBridgePeers.end(),
                              peerId) == knownBridgePeers.end()) {
                  knownBridgePeers.push_back(peerId);
                }
              }
            }

            Log(CONNECTION,
                "Bridge coordination from %u: priority=%d, role=%s, "
                "load=%d%%\n",
                fromNode, priority, role.c_str(), load);
          }
          return false;  // Don't consume the package
        });

    // Create periodic task to send coordination messages
    bridgeCoordinationTask = this->addTask(
        30000,  // 30 seconds interval
        TASK_FOREVER, [this]() { this->sendBridgeCoordination(); });

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

    Log(CONNECTION,
        "Bridge coordination sent: priority=%d, role=%s, load=%d%%\n",
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
    Log(CONNECTION, "scanRouterSignalStrength(): Scanning for %s...\n",
        routerSSID.c_str());

    int n = WiFi.scanNetworks(false, false);
    Log(CONNECTION, "scanRouterSignalStrength(): Found %d networks\n", n);

    for (int i = 0; i < n; i++) {
      if (WiFi.SSID(i) == routerSSID) {
        int8_t rssi = WiFi.RSSI(i);
        Log(CONNECTION,
            "scanRouterSignalStrength(): Found %s with RSSI %d dBm\n",
            routerSSID.c_str(), rssi);
        return rssi;
      }
    }

    Log(CONNECTION, "scanRouterSignalStrength(): Router %s not found\n",
        routerSSID.c_str());
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
      Log(CONNECTION,
          "startBridgeElection(): No router credentials, cannot participate\n");
      return;
    }

    if (electionState != ELECTION_IDLE) {
      Log(CONNECTION, "startBridgeElection(): Election already in progress\n");
      return;
    }

    // Prevent rapid role changes
    if (millis() - lastRoleChangeTime < 60000) {
      Log(CONNECTION,
          "startBridgeElection(): Too soon after last role change\n");
      return;
    }

    // CRITICAL: Check if mesh channel re-synchronization is needed first
    // If we haven't found any mesh nodes and are approaching the re-sync
    // threshold, prioritize finding the mesh over becoming a bridge. This
    // prevents the scenario where a node tries to become a bridge when it
    // should be re-syncing to find the mesh on a different channel (e.g., after
    // another node became bridge and switched channels to match the router).
    uint16_t emptyScans = stationScan.getConsecutiveEmptyScans();
    if (emptyScans >= 3 && WiFi.status() != WL_CONNECTED) {
      Log(CONNECTION,
          "startBridgeElection(): Mesh connectivity lost (%d empty scans), "
          "deferring election to allow channel re-sync\n",
          emptyScans);

      // Schedule a retry after channel re-sync has had a chance to run
      // The channel re-sync threshold is StationScan::EMPTY_SCAN_THRESHOLD
      // scans (default 6) Fast scan interval is 0.5 * SCAN_INTERVAL = 15
      // seconds Wait for re-sync to complete plus a buffer
      uint32_t retryDelay =
          (StationScan::EMPTY_SCAN_THRESHOLD - emptyScans + 2) * 15000;
      Log(CONNECTION,
          "startBridgeElection(): Will retry election in %u seconds if still "
          "needed\n",
          retryDelay / 1000);
      return;
    }

    Log(CONNECTION, "=== Bridge Election Started ===\n");
    electionState = ELECTION_SCANNING;

    // Scan for router to get RSSI
    int8_t routerRSSI = scanRouterSignalStrength(routerSSID);

    if (routerRSSI == 0) {
      Log(CONNECTION,
          "startBridgeElection(): Router not visible, cannot participate\n");
      electionState = ELECTION_IDLE;
      return;
    }

    Log(CONNECTION, "startBridgeElection(): My router RSSI: %d dBm\n",
        routerRSSI);

    // Clear previous candidates
    electionCandidates.clear();

    // Add self as candidate
    BridgeCandidate selfCandidate;
    selfCandidate.nodeId = this->nodeId;
    selfCandidate.routerRSSI = routerRSSI;
    selfCandidate.uptime = millis();
    selfCandidate.freeMemory = ESP.getFreeHeap();
    electionCandidates.push_back(selfCandidate);

    // Broadcast candidacy using JSON directly (avoiding dependency on alteriom
    // package)
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["type"] = protocol::BRIDGE_ELECTION;
    obj["from"] = this->nodeId;
    obj["routing"] = 2;  // BROADCAST
    obj["routerRSSI"] = routerRSSI;
    obj["uptime"] = millis();
    obj["freeMemory"] = ESP.getFreeHeap();
    obj["timestamp"] = this->getNodeTime();
    obj["routerSSID"] = routerSSID;
    obj["message_type"] = protocol::BRIDGE_ELECTION;

    String msg;
    serializeJson(doc, msg);

    // Send election message using raw broadcast to preserve type
    // BRIDGE_ELECTION
    protocol::Variant variant(msg);
    router::broadcast<protocol::Variant, Connection>(variant, (*this), 0);

    Log(CONNECTION, "startBridgeElection(): Candidacy broadcast sent\n");

    // Set election timeout
    electionDeadline = millis() + electionTimeoutMs;
    electionState = ELECTION_COLLECTING;

    // Schedule election evaluation
    this->addTask(electionTimeoutMs + 100, TASK_ONCE,
                  [this]() { this->evaluateElection(); });
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
    Log(CONNECTION, "evaluateElection(): %d candidates\n",
        electionCandidates.size());

    // Find best candidate
    BridgeCandidate* winner = nullptr;
    int8_t bestRSSI = -127;  // Worst possible RSSI

    for (auto& candidate : electionCandidates) {
      Log(CONNECTION,
          "evaluateElection(): Candidate %u: RSSI=%d, uptime=%u, mem=%u\n",
          candidate.nodeId, candidate.routerRSSI, candidate.uptime,
          candidate.freeMemory);

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
    // When only one candidate exists, it indicates the node is isolated from
    // the mesh. In this case, require minimum signal quality to prevent poor
    // connections. When multiple candidates exist, the mesh is connected and
    // best RSSI wins.
    if (electionCandidates.size() == 1 &&
        winner->routerRSSI < minimumBridgeRSSI) {
      Log(CONNECTION, "=== Election Failed: Insufficient Signal Quality ===\n");
      Log(CONNECTION,
          "  Single candidate with RSSI %d dBm (minimum required: %d dBm)\n",
          winner->routerRSSI, minimumBridgeRSSI);
      Log(CONNECTION, "  Node is isolated from mesh with poor router signal\n");
      Log(CONNECTION, "  Rejecting election to prevent unreliable bridge\n");
      Log(CONNECTION,
          "  Recommendation: Move closer to router or wait for mesh "
          "connection\n");

      electionState = ELECTION_IDLE;
      electionCandidates.clear();

      // Notify via callback that election failed
      if (bridgeRoleChangedCallback) {
        bridgeRoleChangedCallback(
            false, "Insufficient signal quality for isolated bridge");
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
      Log(CONNECTION, "Winner is node %u, remaining as regular node\n",
          winner->nodeId);
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
    // SAFETY: Get the primary bridge nodeId safely to avoid crashes from
    // dangling pointers. The getPrimaryBridge() returns a pointer to a vector 
    // element which can become invalid if the vector is modified between the 
    // pointer retrieval and its use. Instead of holding the pointer, we immediately
    // copy the value we need or use a safer method that doesn't involve pointers.
    uint32_t previousBridgeId = 0;
    
    // Use getPrimaryGateway() which returns the nodeId value directly instead of a pointer
    // This avoids any potential issues with dangling pointers to vector elements
    previousBridgeId = this->getPrimaryGateway();

    // IMPORTANT: Send takeover announcement BEFORE switching channels
    // This ensures other nodes on the current channel receive the announcement
    Log(STARTUP,
        "Sending takeover announcement on current channel before "
        "switching...\n");
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["type"] = protocol::BRIDGE_TAKEOVER;
    obj["from"] = this->nodeId;
    obj["routing"] = 2;  // BROADCAST
    obj["previousBridge"] = previousBridgeId;
    obj["reason"] = "Election winner - best router signal";
    obj["routerRSSI"] = 0;  // Not yet connected to router
    obj["timestamp"] = this->getNodeTime();
    obj["message_type"] = protocol::BRIDGE_TAKEOVER;

    String msg;
    serializeJson(doc, msg);

    // Send takeover message using raw broadcast to preserve type
    // BRIDGE_TAKEOVER
    protocol::Variant variant(msg);
    router::broadcast<protocol::Variant, Connection>(variant, (*this), 0);

    // Give time for announcement to propagate before channel switch
    delay(1000);
    Log(STARTUP, "✓ Takeover announcement sent on channel %d\n", _meshChannel);

    // Save current mesh configuration to restore if bridge init fails
    uint8_t savedChannel = _meshChannel;

    // Now reconfigure as bridge (this will switch to router's channel)
    this->stop();
    delay(1000);

    bool bridgeInitSuccess =
        this->initAsBridge(_meshSSID, _meshPassword, routerSSID, routerPassword,
                           mScheduler, _meshPort);

    if (!bridgeInitSuccess) {
      Log(ERROR, "✗ Bridge promotion failed - router unreachable\n");
      Log(ERROR, "Reverting to regular node on channel %d\n", savedChannel);

      // Re-initialize as regular node on the original channel
      this->init(_meshSSID, _meshPassword, mScheduler, _meshPort, WIFI_AP_STA,
                 savedChannel, _meshHidden, MAX_CONN);

      // Reset election state and clear candidates (consistent with normal
      // election completion)
      electionState = ELECTION_IDLE;
      electionCandidates.clear();

      // Notify via callback
      if (bridgeRoleChangedCallback) {
        bridgeRoleChangedCallback(
            false, "Bridge promotion failed - router unreachable");
      }

      return;
    }

    lastRoleChangeTime = millis();

    Log(STARTUP, "✓ Bridge promotion complete on channel %d\n", _meshChannel);

    // Notify via callback
    if (bridgeRoleChangedCallback) {
      bridgeRoleChangedCallback(true, "Election winner - best router signal");
    }

    // Send a follow-up announcement on the new channel
    // This helps nodes that have already switched channels to discover the new
    // bridge Schedule it after a delay to ensure mesh is fully initialized
    this->addTask(3000, TASK_ONCE, [this, previousBridgeId]() {
      Log(STARTUP,
          "Sending follow-up takeover announcement on new channel %d\n",
          _meshChannel);
      JsonDocument doc2;
      JsonObject obj2 = doc2.to<JsonObject>();
      obj2["type"] = protocol::BRIDGE_TAKEOVER;
      obj2["from"] = this->nodeId;
      obj2["routing"] = 2;  // BROADCAST
      obj2["previousBridge"] = previousBridgeId;
      obj2["reason"] = "Election winner - best router signal";
      obj2["routerRSSI"] = WiFi.RSSI();
      obj2["timestamp"] = this->getNodeTime();
      obj2["message_type"] = protocol::BRIDGE_TAKEOVER;

      String msg2;
      serializeJson(doc2, msg2);

      // Send follow-up takeover using raw broadcast to preserve type
      // BRIDGE_TAKEOVER
      protocol::Variant variant2(msg2);
      router::broadcast<protocol::Variant, Connection>(variant2, (*this), 0);

      Log(STARTUP, "✓ Follow-up takeover announcement sent\n");
    });
  }

  /**
   * Attempt to promote an isolated node to bridge
   *
   * This method handles the case where a node is isolated (no mesh connections)
   * but has router credentials. Unlike the election-based promotion, this
   * directly attempts to connect to the router without requiring mesh
   * connectivity.
   *
   * This is useful for:
   * - Nodes that failed initial bridge setup and need to retry
   * - Nodes that are the first to start and no mesh exists yet
   * - Recovery scenarios where mesh network is unavailable
   *
   * @return true if promotion was attempted (regardless of success), false if
   * skipped
   */
  bool attemptIsolatedBridgePromotion() {
    using namespace logger;

    Log(CONNECTION, "=== Isolated Bridge Promotion Attempt ===\n");
    Log(CONNECTION, "Attempt %d of %d\n", _isolatedBridgeRetryAttempts + 1,
        MAX_ISOLATED_BRIDGE_RETRY_ATTEMPTS);

    // First, scan for router to check if it's visible
    int8_t routerRSSI = scanRouterSignalStrength(routerSSID);

    if (routerRSSI == 0) {
      Log(CONNECTION,
          "attemptIsolatedBridgePromotion(): Router %s not visible\n",
          routerSSID.c_str());
      return false;  // Don't count as an attempt - router not visible
    }

    // Check minimum RSSI threshold for isolated promotion
    if (routerRSSI < minimumBridgeRSSI) {
      Log(CONNECTION,
          "attemptIsolatedBridgePromotion(): Router RSSI %d dBm below "
          "threshold %d dBm\n",
          routerRSSI, minimumBridgeRSSI);
      return false;  // Don't count as an attempt - signal too weak
    }

    Log(CONNECTION,
        "attemptIsolatedBridgePromotion(): Router visible with RSSI %d dBm\n",
        routerRSSI);
    Log(CONNECTION,
        "Attempting direct bridge promotion (bypassing election)\n");

    // Save current mesh configuration
    uint8_t savedChannel = _meshChannel;

    // Stop current mesh operations
    this->stop();
    delay(1000);

    // Attempt to initialize as bridge
    bool bridgeInitSuccess =
        this->initAsBridge(_meshSSID, _meshPassword, routerSSID, routerPassword,
                           mScheduler, _meshPort);

    if (!bridgeInitSuccess) {
      Log(ERROR, "✗ Isolated bridge promotion failed - router unreachable\n");
      Log(ERROR, "Reverting to regular node on channel %d\n", savedChannel);

      // Re-initialize as regular node on the original channel
      this->init(_meshSSID, _meshPassword, mScheduler, _meshPort, WIFI_AP_STA,
                 savedChannel, _meshHidden, MAX_CONN);

      // Re-configure router credentials for future retry attempts
      this->setRouterCredentials(routerSSID, routerPassword);
      this->enableBridgeFailover(true);

      // Set flag to skip empty scan check on next retry attempt
      // since we already confirmed isolation before this failed attempt
      _isolatedRetryPending = true;

      // Notify via callback
      if (bridgeRoleChangedCallback) {
        bridgeRoleChangedCallback(
            false, "Isolated bridge promotion failed - router unreachable");
      }

      return true;  // Count as an attempt - we tried but failed
    }

    // Success! Reset retry counter
    _isolatedBridgeRetryAttempts = 0;
    lastRoleChangeTime = millis();

    Log(STARTUP, "✓ Isolated bridge promotion complete on channel %d\n",
        _meshChannel);

    // Notify via callback
    if (bridgeRoleChangedCallback) {
      bridgeRoleChangedCallback(true, "Isolated node promoted to bridge");
    }

    // Send bridge status announcement to attract other nodes
    this->addTask(3000, TASK_ONCE, [this]() {
      Log(STARTUP, "Sending bridge status announcement on channel %d\n",
          _meshChannel);
      this->sendBridgeStatus();
    });

    return true;  // Count as an attempt - we succeeded
  }

  /**
   * Handle received bridge election package
   * Called by package handler when election message arrives
   */
  void handleBridgeElection(uint32_t fromNode, int8_t routerRSSI,
                            uint32_t uptime, uint32_t freeMemory) {
    using namespace logger;

    if (electionState != ELECTION_COLLECTING) {
      Log(CONNECTION,
          "handleBridgeElection(): Not collecting candidates, ignoring\n");
      return;
    }

    // Check if candidate already exists
    for (auto& candidate : electionCandidates) {
      if (candidate.nodeId == fromNode) {
        Log(CONNECTION,
            "handleBridgeElection(): Duplicate candidate from %u, ignoring\n",
            fromNode);
        return;
      }
    }

    BridgeCandidate candidate;
    candidate.nodeId = fromNode;
    candidate.routerRSSI = routerRSSI;
    candidate.uptime = uptime;
    candidate.freeMemory = freeMemory;

    electionCandidates.push_back(candidate);

    Log(CONNECTION,
        "handleBridgeElection(): Added candidate %u (RSSI: %d dBm)\n", fromNode,
        routerRSSI);
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

    obj["type"] = protocol::BRIDGE_STATUS;
    obj["from"] = this->nodeId;
    obj["routing"] = 2;  // BROADCAST routing
    obj["timestamp"] = this->getNodeTime();

    // Check Internet connectivity: WiFi connected AND valid IP address
    // We check for valid local IP instead of gateway IP because:
    // 1. Gateway IP might not be immediately available after connection
    // 2. Some networks (mobile hotspots) may not provide gateway IP via DHCP
    // 3. Having a valid local IP + being connected is sufficient for internet
    // access
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
    obj["message_type"] = protocol::BRIDGE_STATUS;

    String msg;
    serializeJson(doc, msg);

    Log(GENERAL, "sendBridgeStatus(): Broadcasting status (Internet: %s)\n",
        hasInternet ? "Connected" : "Disconnected");
    Log(GENERAL,
        "sendBridgeStatus(): WiFi status=%d, localIP=%s, gatewayIP=%s\n",
        WiFi.status(), WiFi.localIP().toString().c_str(),
        WiFi.gatewayIP().toString().c_str());

    // Update our own bridge status in knownBridges list
    // This ensures the bridge reports itself correctly when queried
    this->updateBridgeStatus(this->nodeId, hasInternet, rssi, channel, uptime,
                             gatewayIP, this->getNodeTime());

    // Send bridge status using raw broadcast to preserve type BRIDGE_STATUS
    // Using sendBroadcast(msg) would wrap it in type 8 (BROADCAST) and hide
    // type BRIDGE_STATUS
    protocol::Variant variant(msg);
    router::broadcast<protocol::Variant, Connection>(variant, (*this), 0);
  }

  /**
   * Helper method to send gateway acknowledgment
   */
  void sendGatewayAck(const gateway::GatewayDataPackage& request, bool success,
                      uint16_t httpStatus, const TSTRING& error) {
    using namespace logger;

    gateway::GatewayAckPackage ack;
    ack.from = this->nodeId;
    ack.dest = request.originNode;
    ack.messageId = request.messageId;
    ack.originNode = request.originNode;
    ack.success = success;
    ack.httpStatus = httpStatus;
    ack.error = error;
    ack.timestamp = this->getNodeTime();

    auto conn = router::findRoute<Connection>((*this), request.originNode);
    if (conn) {
      protocol::Variant variant(&ack);
      router::send(std::move(variant), conn);
      Log(COMMUNICATION, "Sent GATEWAY_ACK to node %u (success=%d, http=%d)\n",
          request.originNode, success, httpStatus);
    } else {
      Log(ERROR, "Failed to send GATEWAY_ACK: no route to node %u\n",
          request.originNode);
    }
  }

  /**
   * Initialize gateway Internet handler
   * Registers GATEWAY_DATA package handler for bridge/gateway nodes
   *
   * This handler processes GATEWAY_DATA packages from mesh nodes requesting
   * HTTP/HTTPS requests to Internet destinations. It validates connectivity,
   * makes the request, and sends back a GATEWAY_ACK with the result.
   *
   * Security notes:
   * - HTTPS on ESP8266 uses setInsecure() which disables SSL certificate
   * validation to reduce memory overhead. This makes connections vulnerable to
   * MITM attacks.
   * - ESP32 uses default SSL settings with certificate validation.
   *
   * Limitations:
   * - HTTP redirects (3xx) are not automatically followed
   * - Only 2xx status codes are treated as success
   * - Request timeout is fixed at 30 seconds
   */
  void initGatewayInternetHandler() {
    using namespace logger;
    Log(STARTUP,
        "initGatewayInternetHandler(): Registering GATEWAY_DATA handler\n");

    this->callbackList.onPackage(
        protocol::GATEWAY_DATA, [this](protocol::Variant& variant,
                                       std::shared_ptr<Connection>, uint32_t) {
          auto pkg = variant.to<gateway::GatewayDataPackage>();

          Log(COMMUNICATION,
              "Gateway received Internet request: msgId=%u dest=%s\n",
              pkg.messageId, pkg.destination.c_str());

          // Check Internet connectivity
          if (WiFi.status() != WL_CONNECTED) {
            sendGatewayAck(pkg, false, 0, "Gateway not connected to Internet");
            return true;  // Consume package - we handled it (with error)
          }

#if defined(ESP32) || defined(ESP8266)
          // Make HTTP/HTTPS request
          HTTPClient http;
          http.setTimeout(GATEWAY_HTTP_TIMEOUT_MS);

          bool success = false;
          uint16_t httpCode = 0;
          TSTRING error = "";

#ifdef ESP8266
          // ESP8266: Declare clients at function scope to ensure
          // they survive until after the HTTP request completes
          WiFiClient client;
          WiFiClientSecure secureClient;
#endif

          if (pkg.destination.startsWith("https://")) {
#ifdef ESP32
            // ESP32: Use default SSL settings with certificate validation
            http.begin(pkg.destination.c_str());
#elif defined(ESP8266)
            // ESP8266: Use insecure mode to reduce memory overhead
            // WARNING: This disables SSL certificate validation
            secureClient.setInsecure();
            http.begin(secureClient, pkg.destination.c_str());
#endif
          } else {
#ifdef ESP32
            http.begin(pkg.destination.c_str());
#elif defined(ESP8266)
            // ESP8266: begin() requires a client parameter
            http.begin(client, pkg.destination.c_str());
#endif
          }

          // Make request (GET if no payload, POST if payload)
          if (pkg.payload.length() > 0) {
            http.addHeader("Content-Type", pkg.contentType.c_str());
            httpCode = http.POST(pkg.payload.c_str());
          } else {
            httpCode = http.GET();
          }

          if (httpCode > 0) {
            // Only 2xx status codes are treated as success
            // 3xx redirects are not automatically followed
            success = (httpCode >= 200 && httpCode < 300);
            Log(COMMUNICATION, "HTTP request completed: code=%d\n", httpCode);
          } else {
            error = http.errorToString(httpCode);
            Log(ERROR, "HTTP request failed: %s\n", error.c_str());
          }

          http.end();

          // Send acknowledgment back
          sendGatewayAck(pkg, success, httpCode, error);
#else
        // Non-ESP platform - send error
        sendGatewayAck(pkg, false, 0, "HTTP client not available on this platform");
#endif

          return true;  // Consume package - we have processed it and sent
                        // acknowledgment
        });
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
        [&](const WiFiEventStationModeConnected& event) {
          // Log(CONNECTION, "Event: Station Mode Connected to \"%s\"\n",
          // event.ssid.c_str());
          Log(CONNECTION, "Event: Station Mode Connected\n");
        });

    eventSTADisconnectedHandler = WiFi.onStationModeDisconnected(
        [&](const WiFiEventStationModeDisconnected& event) {
          Log(CONNECTION, "Event: Station Mode Disconnected\n");
          this->droppedConnectionCallbacks.execute(0, true);
          // Handle station disconnect completion after callbacks
          this->handleStationDisconnectComplete();
        });

    eventSTAGotIPHandler =
        WiFi.onStationModeGotIP([&](const WiFiEventStationModeGotIP& event) {
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
  AsyncServer* _tcpListener;
  std::shared_ptr<Task> bridgeStatusTask;

  // Station disconnect handling state
  bool _pendingStationReconnect = false;

  // Bridge failover state and configuration
  enum ElectionState { ELECTION_IDLE, ELECTION_SCANNING, ELECTION_COLLECTING };

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
  int8_t minimumBridgeRSSI =
      -80;  // Default -80 dBm minimum for isolated elections
  uint32_t electionStartupDelayMs =
      60000;  // Default 60 seconds before first election check
  uint32_t electionRandomDelayMinMs =
      1000;  // Default min 1 second random delay
  uint32_t electionRandomDelayMaxMs =
      3000;  // Default max 3 seconds random delay
  uint32_t lastRoleChangeTime = 0;
  ElectionState electionState = ELECTION_IDLE;
  uint32_t electionDeadline = 0;
  std::vector<BridgeCandidate> electionCandidates;
  std::function<void(bool isBridge, const TSTRING& reason)> bridgeRoleChangedCallback;

  // Isolated bridge retry state and configuration
  uint8_t _isolatedBridgeRetryAttempts = 0;
  uint32_t _isolatedBridgeRetryResetTime =
      0;  // Time when retry counter can be reset
  bool _isolatedRetryPending =
      false;  // Flag to skip empty scan check after failed promotion
  static const uint8_t MAX_ISOLATED_BRIDGE_RETRY_ATTEMPTS =
      5;  // Max retry attempts before waiting
  static const uint32_t isolatedBridgeRetryIntervalMs =
      60000;  // Retry every 60 seconds
  static const uint32_t isolatedBridgeRetryResetIntervalMs =
      300000;  // Reset counter after 5 minutes
  static const uint16_t ISOLATED_BRIDGE_RETRY_SCAN_THRESHOLD =
      6;  // Require 6 empty scans before retrying

  // Multi-bridge coordination state and configuration
 protected:
  bool multiBridgeEnabled = false;
  BridgeSelectionStrategy bridgeSelectionStrategy = PRIORITY_BASED;
  uint8_t maxConcurrentBridges = 2;
  uint8_t bridgePriority = 5;        // Default medium priority
  TSTRING bridgeRole = "secondary";  // Default role
  std::shared_ptr<Task> bridgeCoordinationTask;
  std::map<uint32_t, uint8_t> bridgePriorities;  // nodeId -> priority mapping
  std::vector<uint32_t> knownBridgePeers;        // List of peer bridge node IDs
  uint32_t selectedBridgeOverride = 0;  // Manual bridge selection override
  size_t lastSelectedBridgeIndex = 0;   // For round-robin selection

  // Shared gateway mode state and configuration
  bool _sharedGatewayMode = false;
  gateway::SharedGatewayConfig _sharedGatewayConfig;
  std::shared_ptr<Task> _sharedGatewayMonitorTask;
  uint32_t _lastRouterReconnectAttempt = 0;
  uint8_t _routerReconnectAttempts = 0;
  static const uint8_t MAX_ROUTER_RECONNECT_ATTEMPTS = 10;
  static const uint32_t ROUTER_RECONNECT_BASE_INTERVAL =
      5000;  // 5 seconds base interval
  static const uint32_t ROUTER_RECONNECT_MAX_INTERVAL =
      300000;  // 5 minutes max interval
  static const int ROUTER_CONNECTION_TIMEOUT_SECONDS =
      30;  // Router connection timeout
  static const uint8_t MIN_WIFI_CHANNEL = 1;
  static const uint8_t MAX_WIFI_CHANNEL =
      14;  // Support channels 1-14 for regions that allow it
  static const uint32_t GATEWAY_HTTP_TIMEOUT_MS =
      30000;  // 30 second timeout for gateway HTTP requests

  /**
   * Initialize shared gateway monitoring
   *
   * Sets up periodic monitoring of router connection and automatic
   * reconnection logic for shared gateway mode.
   */
  void initSharedGatewayMonitoring() {
    using namespace logger;

    if (!_sharedGatewayMode) {
      return;
    }

    Log(STARTUP,
        "initSharedGatewayMonitoring(): Setting up router connection "
        "monitoring\n");

    // Add callback for router disconnection in shared gateway mode
    this->droppedConnectionCallbacks.push_back(
        [this](uint32_t nodeId, bool station) {
          if (station && _sharedGatewayMode) {
            Log(CONNECTION,
                "Router disconnected in shared gateway mode, scheduling "
                "reconnection\n");
            scheduleRouterReconnect();
          }
        });

    // Create periodic monitoring task
    _sharedGatewayMonitorTask =
        this->addTask(_sharedGatewayConfig.internetCheckInterval, TASK_FOREVER,
                      [this]() { monitorRouterConnection(); });

    Log(STARTUP, "Router connection monitoring enabled (interval: %u ms)\n",
        _sharedGatewayConfig.internetCheckInterval);
  }

  /**
   * Monitor router connection in shared gateway mode
   *
   * Checks router connectivity and triggers reconnection if needed.
   */
  void monitorRouterConnection() {
    using namespace logger;

    if (!_sharedGatewayMode) {
      return;
    }

    bool isConnected = (WiFi.status() == WL_CONNECTED) &&
                       (WiFi.localIP() != IPAddress(0, 0, 0, 0));

    if (!isConnected) {
      Log(CONNECTION,
          "monitorRouterConnection(): Router connection lost, triggering "
          "reconnect\n");
      scheduleRouterReconnect();
    } else {
      // Connection is healthy, reset reconnect attempts
      _routerReconnectAttempts = 0;

      // Log periodic status
      Log(GENERAL,
          "monitorRouterConnection(): Router connected (RSSI: %d dBm, IP: "
          "%s)\n",
          WiFi.RSSI(), WiFi.localIP().toString().c_str());
    }
  }

  /**
   * Schedule router reconnection with exponential backoff
   */
  void scheduleRouterReconnect() {
    using namespace logger;

    if (!_sharedGatewayMode) {
      return;
    }

    // Don't schedule if already connected
    if (WiFi.status() == WL_CONNECTED) {
      return;
    }

    // Limit reconnection attempts
    if (_routerReconnectAttempts >= MAX_ROUTER_RECONNECT_ATTEMPTS) {
      Log(ERROR,
          "scheduleRouterReconnect(): Max reconnection attempts reached (%d)\n",
          MAX_ROUTER_RECONNECT_ATTEMPTS);
      Log(ERROR,
          "Router reconnection suspended. Manual intervention may be "
          "required.\n");
      return;
    }

    // Calculate delay with exponential backoff, preventing overflow
    // Limit shift amount to prevent overflow (5000 * 2^6 = 320000 is safe)
    uint8_t shiftAmount =
        (_routerReconnectAttempts > 6) ? 6 : _routerReconnectAttempts;
    uint32_t delay = ROUTER_RECONNECT_BASE_INTERVAL * (1UL << shiftAmount);
    if (delay > ROUTER_RECONNECT_MAX_INTERVAL)
      delay = ROUTER_RECONNECT_MAX_INTERVAL;

    // Don't reconnect too frequently
    uint32_t now = millis();
    if (now - _lastRouterReconnectAttempt < delay) {
      return;
    }

    _routerReconnectAttempts++;
    _lastRouterReconnectAttempt = now;

    Log(CONNECTION,
        "scheduleRouterReconnect(): Attempting reconnection (attempt %d/%d, "
        "delay %u ms)\n",
        _routerReconnectAttempts, MAX_ROUTER_RECONNECT_ATTEMPTS, delay);

    // Schedule reconnection
    this->addTask(delay, TASK_ONCE, [this]() { attemptRouterReconnect(); });
  }

  /**
   * Attempt to reconnect to the router
   */
  void attemptRouterReconnect() {
    using namespace logger;

    if (!_sharedGatewayMode) {
      return;
    }

    // Check if already connected
    if (WiFi.status() == WL_CONNECTED) {
      Log(CONNECTION,
          "attemptRouterReconnect(): Already connected to router\n");
      _routerReconnectAttempts = 0;
      return;
    }

    Log(CONNECTION, "attemptRouterReconnect(): Reconnecting to router %s...\n",
        _sharedGatewayConfig.routerSSID.c_str());

    // Use stationManual to reconnect (port 0 means no TCP mesh connection to
    // router)
    stationManual(_sharedGatewayConfig.routerSSID,
                  _sharedGatewayConfig.routerPassword, 0);
  }
};
}  // namespace wifi
};  // namespace painlessmesh

#endif

#endif
