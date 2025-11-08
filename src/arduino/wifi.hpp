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
};
}  // namespace wifi
};  // namespace painlessmesh

#endif

#endif
