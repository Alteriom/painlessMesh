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
  // Move both Wi-Fi interfaces to a bridge-announced channel immediately.
  bool followBridgeChannel(uint8_t targetChannel);
  // This one will call the connectToAP next in the task and should be used
  // instead of connectToAP
  void yieldConnectToAP() {
    task.yield([this]() { connectToAP(); });
  }
  
  // Scan every channel for the mesh SSID. With avoidChannel set, prefer a
  // channel other than it: re-detection runs when the node's own partition
  // has gone quiet, so the mesh on its current channel is the partition it
  // is stranded in, not the one it is looking for.
  static uint8_t scanForMeshChannel(TSTRING meshSSID, bool meshHidden,
                                    uint8_t avoidChannel = 0);
  
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
  // millis() of the last requestIP(), to tell a station that is still
  // obtaining an address from one that associated and never got one.
  uint32_t connectAttemptStarted = 0;
  // Empty scans before re-detecting the mesh channel. A disconnected or
  // orphaned node scans every 0.5 * SCAN_INTERVAL = 15 s, so 2 is ~30 s —
  // what the old comment promised while the value of 6 delivered 90 s, long
  // enough for a bridge's followers to miss a 120 s gateway contract.
  static const uint16_t EMPTY_SCAN_THRESHOLD = 2;
  
  // TCP failure blocklist to prevent infinite retry loops
  // Maps nodeId -> blockUntil timestamp (millis())
  std::map<uint32_t, uint32_t> tcpFailureBlocklist;
  
  // Threshold for detecting millis() rollover in time comparisons
  // Using 2^30 (~12 days) as reasonable limit - any time difference larger is likely rollover
  static constexpr int32_t MILLIS_ROLLOVER_THRESHOLD = (int32_t)(1U << 30);

  friend painlessMesh;
};

#endif
