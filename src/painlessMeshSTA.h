#ifndef _PAINLESS_MESH_STA_H_
#define _PAINLESS_MESH_STA_H_

#include "painlessmesh/configuration.hpp"

#include "painlessmesh/mesh.hpp"

#include <list>
#include <map>

typedef struct {
  uint8_t bssid[6];
  TSTRING ssid;
  int8_t rssi;
} WiFi_AP_Record_t;

// Entry for tracking nodes where TCP connection failed
typedef struct {
  uint32_t nodeId;
  uint32_t blockUntil;  // millis() timestamp when this node can be retried
} TCPFailureBlocklistEntry;

class StationScan {
 public:
  Task task;  // Station scanning for connections

#ifdef ESP8266
  Task asyncTask;
#endif

  StationScan() {}
  void init(painlessmesh::wifi::Mesh *pMesh, TSTRING ssid, TSTRING password,
            uint16_t port, uint8_t channel, bool hidden);
  void stationScan();
  void scanComplete();
  void filterAPs();
  void connectToAP();
  // This one will call the connectToAP next in the task and should be used
  // instead of connectToAP
  void yieldConnectToAP() {
    task.yield([this]() { connectToAP(); });
  }
  
  // Helper to scan all channels for a specific mesh SSID
  static uint8_t scanForMeshChannel(TSTRING meshSSID, bool meshHidden);
  
  // Check if channel re-synchronization is needed or in progress
  bool isChannelResyncNeeded() const {
    return consecutiveEmptyScans >= EMPTY_SCAN_THRESHOLD;
  }
  
  // Get the number of consecutive empty scans
  uint16_t getConsecutiveEmptyScans() const {
    return consecutiveEmptyScans;
  }
  
  // Add a node to the TCP failure blocklist
  // This prevents repeated connection attempts to nodes where TCP server is unresponsive
  void blockNodeAfterTCPFailure(uint32_t nodeId, uint32_t blockDurationMs = 60000);
  
  // Check if a node is currently blocked due to TCP failures
  bool isNodeBlocked(uint32_t nodeId) const;
  
  // Clean up expired entries from the blocklist
  void cleanupBlocklist();

  /// Valid APs found during the last scan
  std::list<WiFi_AP_Record_t> lastAPs;

 protected:
  TSTRING ssid;
  TSTRING password;
  painlessMesh *mesh;
  uint16_t port;
  uint8_t channel;
  bool hidden;
  std::list<WiFi_AP_Record_t> aps;

  void requestIP(WiFi_AP_Record_t &ap);

  // Manually configure network and ip
  bool manual = false;
  IPAddress manualIP = IPAddress(0, 0, 0, 0);
  
  // Track consecutive scans with no mesh nodes found (for channel re-detection)
  uint16_t consecutiveEmptyScans = 0;
  static const uint16_t EMPTY_SCAN_THRESHOLD = 6; // ~30 seconds at default SCAN_INTERVAL
  
  // TCP failure blocklist to prevent infinite retry loops
  // Maps nodeId -> blockUntil timestamp (millis())
  std::map<uint32_t, uint32_t> tcpFailureBlocklist;

  friend painlessMesh;
};

#endif
