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
  // The scan-done event has arrived (ESP32: on the core's network-event
  // task). The result is read by scanComplete() from the station task,
  // in the loop, where the radio may be reconfigured.
  void scanDone();
  void filterAPs();
  void connectToAP();
  // Move both Wi-Fi interfaces to a bridge-announced channel immediately.
  bool followBridgeChannel(uint8_t targetChannel);
  // A bridge has spoken, from this channel: its status message carries the
  // channel it is on, which is its router's, and a message just received
  // is live — unlike a cached tree, which can carry a root that has gone.
  // That channel is home.
  void noteRooted(uint8_t channel) {
    everRooted = true;
    rootedChannel = channel;
    homeStays = 0;
  }
  // The tree contains a root. Enough for the rules that ask whether this
  // mesh ever had one; not enough to say which channel is home.
  void noteEverRooted() { everRooted = true; }
  // The station attempt requestIP() started is over: it got an address, or
  // it was disconnected. The half-open guard judges only an attempt still
  // in progress. Without this it judged a fresh association by the clock of
  // an attempt made 109 s earlier — the node had followed the bridge to its
  // channel in between — and dropped it, costing a scan interval.
  void stationAttemptOver() { connectAttemptStarted = 0; }
  // The station got an address: its link is up. Cleared once the drop
  // callbacks have judged a disconnect, so they can tell a link that was
  // up and went away from an attempt that never got that far.
  void stationUp() { stationLinkUp = true; }
  void stationDown() { stationLinkUp = false; }
  bool stationLinkUp = false;
  // The station link was closed by this node's own channel move a moment
  // ago (followBridgeChannel() closes it and scans next). Not a loss.
  bool droppedByMove() const {
    return channelMovedAt != 0 && millis() - channelMovedAt < 5000;
  }
  // The next scan covers every channel. For a node whose uplink just went
  // away in a mesh that should have a root: the AP it was on left for the
  // bridge's channel, and so will whatever is still here.
  void redetectOnNextScan() { redetectRequested = true; }
  // On the channel the bridge's status named: the bridge's AP is here.
  bool atHome() const { return rootedChannel != 0 && channel == rootedChannel; }
  // This one will call the connectToAP next in the task and should be used
  // instead of connectToAP
  void yieldConnectToAP() {
    task.yield([this]() { connectToAP(); });
  }
  
  // Scan every channel for the mesh SSID. With avoidChannel set, prefer a
  // channel other than it: re-detection runs when the node's own partition
  // has gone quiet, so the mesh on its current channel is the partition it
  // is stranded in, not the one it is looking for.
  // With routerSSID set, the mesh on the router's channel is preferred over
  // every other: that is where a bridge, and so the mesh, lives.
  static uint8_t scanForMeshChannel(TSTRING meshSSID, bool meshHidden,
                                    uint8_t avoidChannel = 0,
                                    TSTRING routerSSID = "");
  
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
  // millis() of the last followBridgeChannel(), for droppedByMove().
  uint32_t channelMovedAt = 0;
  // Whether the current attempt's half-open association has already been
  // dropped. WiFi.disconnect() on a station that has nothing to disconnect
  // leaves the status where it was, and a guard that fired on every pass
  // held a node out of the mesh for the rest of a test.
  bool halfOpenDropped = false;
  // Set when this task starts an async scan, cleared when its result is
  // consumed. The scan-done event also fires for the synchronous scans
  // channel re-detection runs, and consuming those results here found them
  // already deleted, reported "wifi scan failed", and rescanned at once.
  bool scanRequested = false;
  // Set when the next station scan must cover every channel: the empty
  // scans have piled up and the node is looking for the channel the mesh
  // moved to. The re-detection is this task's ordinary asynchronous scan
  // with the channel left open, not a synchronous all-channel scan run
  // inline. The synchronous one held the main loop and the radio for four
  // to seven seconds; measured on the rig, an ACK owed through the node
  // during that window came back after the sender's eight-second budget
  // and counted as a loss — one in every two or three soak runs.
  bool redetectRequested = false;
  // The scan in flight covers every channel (the fast re-detection of a
  // node with nothing under its AP), or is one slice of a sliced hunt.
  bool scanAllChannels = false;
  uint8_t scanSlice = 0;
  // A re-detection with stations under this node's AP is done a channel at
  // a time: an all-channel scan takes the AP off its channel for two to
  // three seconds, and an ESP8266 station does not survive that — on the
  // rig the soak's sender ran one with the ESP8266 as its child and lost
  // it for the rest of the test. A slice is one channel for 120 ms, then
  // 1.5 s at home; the mesh APs seen on each channel are tallied, and the
  // decision is taken on the own-channel scan that follows the last slice,
  // by the same rules as the all-channel scan.
  uint8_t huntChannel = 0;
  bool huntPending = false;
  std::map<uint8_t, size_t> huntCounts;
  std::map<uint8_t, int8_t> huntRssi;
  // Doubles the scan interval of a connected node that keeps finding the
  // mesh only on its own channel while told the mesh has a root: a mesh
  // that is simply rootless would otherwise cost every node a full
  // all-channel scan every half interval, for as long as it stays so.
  uint8_t orphanScanBackoff = 0;
  // Re-detections in a row, while connected and rootless, that found the
  // mesh only on this channel and nothing new on it. Two, and a leaf
  // drops its station link to look for the root with an empty tree.
  uint8_t orphanRedetects = 0;
  // Whether this node's tree has ever contained a root since the mesh
  // started. A leaf leaves a rootless partition to look for the root only
  // if there was one: a mesh that never had a bridge is rootless by
  // design, and its leaves dropping their links every other minute to look
  // for what does not exist cost the soak its deliveries.
  bool everRooted = false;
  // The channel the mesh was on the last time this node's tree had a root:
  // the bridge's channel, which is its router's. A rootless node that has
  // one treats it as home. It does not leave home for a partition on
  // another channel, and away from home it goes back as soon as it sees
  // the mesh there, whatever the sizes: a bridge that has just been
  // promoted is one AP on the router's channel, and it is the one to join.
  // On the rig, the node the failover test sends from followed a two-node
  // partition off the router's channel while the backup was being elected
  // beside it, then stayed away because its new partition was bigger than
  // the lone bridge, and never heard the bridge's status.
  uint8_t rootedChannel = 0;
  // Re-detections in a row, at home, that declined to follow a partition
  // seen elsewhere. A bridge that moves for good (its router changed
  // channel) would otherwise keep the old channel rootless forever, so
  // after enough of them home is forgotten and the ordinary rules apply.
  uint8_t homeStays = 0;
  // The other channel a disconnected node saw a smaller partition on, at
  // its last re-detection: it follows only if the same channel shows the
  // mesh again a scan later — a straggler is gone by then, a bridge is
  // not.
  uint8_t pendingElsewhere = 0;
  // Consecutive scans, while connected and unrooted in a mesh that should
  // have a root, that found nodes this node has no route to: a partition,
  // with the root on the other side. Two of them and the node joins it.
  uint8_t partitionScans = 0;
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
