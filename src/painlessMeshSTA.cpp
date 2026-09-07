//
//  painlessMeshSTA.cpp
//
//
//  Created by Bill Gray on 7/26/16.
//
//
#include "painlessmesh/configuration.hpp"

#ifdef PAINLESSMESH_ENABLE_ARDUINO_WIFI

#include <Arduino.h>
#include <algorithm>
#include <memory>

#include "arduino/wifi.hpp"

#include "painlessmesh/layout.hpp"
#include "painlessmesh/tcp.hpp"

extern painlessmesh::logger::LogClass Log;

//***********************************************************************
// Calculate NodeID from a hardware MAC address
void ICACHE_FLASH_ATTR StationScan::init(painlessmesh::wifi::Mesh *pMesh,
                                         TSTRING pssid, TSTRING ppassword,
                                         uint16_t pport, uint8_t pchannel,
                                         bool phidden) {
  ssid = pssid;
  password = ppassword;
  mesh = pMesh;
  port = pport;
  channel = pchannel;
  hidden = phidden;

  // A node re-initialised in place — promoted to bridge, or back to a
  // regular node — keeps this object, and everything it had learned in
  // its previous life came with it: a newly promoted bridge ran an
  // all-channel re-detection because the request was still set from
  // when it was a rootless regular node, and its empty-scan count,
  // back-offs and "has ever seen a root" carried over the same way. A
  // new life starts with none of that. The manual flag is set by
  // stationManual() after this call, for the link that needs it.
  manual = false;
  consecutiveEmptyScans = 0;
  scanRequested = false;
  redetectRequested = false;
  scanAllChannels = false;
  scanSlice = 0;
  huntChannel = 0;
  huntPending = false;
  huntCounts.clear();
  huntRssi.clear();
  orphanScanBackoff = 0;
  orphanRedetects = 0;
  everRooted = false;
  rootedChannel = 0;
  homeStays = 0;
  pendingElsewhere = 0;
  partitionScans = 0;
  halfOpenDropped = false;
  connectAttemptStarted = 0;
  aps.clear();

  task.set(SCAN_INTERVAL, TASK_FOREVER, [this]() { stationScan(); });
}

// Starts scan for APs whose name is Mesh SSID
void ICACHE_FLASH_ATTR StationScan::stationScan() {
  using namespace painlessmesh::logger;
  Log(CONNECTION, "stationScan(): %s\n", ssid.c_str());
  
  // If channel is 0, auto-detect the mesh channel first
  if (channel == 0) {
    Log(STARTUP, "stationScan(): Auto-detecting mesh channel...\n");
    uint8_t detectedChannel = scanForMeshChannel(
        ssid, hidden, 0,
        mesh->routerCredentialsConfigured ? mesh->routerSSID : TSTRING(""));
    if (detectedChannel > 0) {
      uint8_t oldChannel = mesh->_meshChannel;
      mesh->_meshChannel = detectedChannel;
      channel = detectedChannel;
      Log(STARTUP, "stationScan(): Mesh channel auto-detected: %d\n", detectedChannel);
      // init() has already created the AP.  When channel 0 was requested and
      // no peer was visible during that first instant, the ESP Wi-Fi stack
      // created it on channel 1.  Recreate it on the detected channel before
      // connecting the station, otherwise a temporary station disconnect can
      // snap the AP back to channel 1 and isolate a failover candidate.
      if (oldChannel != detectedChannel && (WiFi.getMode() & WIFI_AP)) {
        WiFi.softAPdisconnect(true);
        delay(200);
        mesh->apInit(mesh->getNodeId());
        delay(100);
      }
    } else {
      // Keep channel == 0 so the next station scan retries all-channel
      // detection.  Permanently replacing it with channel 1 after one miss
      // made a node unable to follow a bridge that was still starting or had
      // just moved the mesh to its router channel.
      if (mesh->_meshChannel == 0) mesh->_meshChannel = 1;
      Log(CONNECTION,
          "stationScan(): Mesh not found, using channel 1 temporarily and "
          "retrying auto-detection\n");
    }
  }

  // Channel 0 scans them all. A re-detection dwells a shorter time per
  // channel than the single-channel scan: thirteen channels at 300 ms is
  // four seconds off the mesh channel, and the point of doing it
  // asynchronously is lost if the radio is away that long.
  uint8_t scanChannel = channel;
  bool allChannels = false;
  uint8_t slice = 0;
  if (redetectRequested) {
    redetectRequested = false;
    size_t stationsUnderAp = 0;
    for (auto&& sub : mesh->subs) {
      if (sub->connected() && !sub->station) ++stationsUnderAp;
    }
    if (stationsUnderAp == 0) {
      Log(CONNECTION,
          "stationScan(): re-detecting the mesh channel, scanning all channels\n");
      allChannels = true;
    } else {
      Log(CONNECTION,
          "stationScan(): re-detecting the mesh channel a channel at a time: "
          "%u station(s) under this AP would drop during an all-channel scan\n",
          (unsigned)stationsUnderAp);
      huntChannel = 1;
      huntCounts.clear();
      huntRssi.clear();
    }
  }
  if (huntChannel != 0) {
    if (huntChannel == mesh->_meshChannel) ++huntChannel;
    if (huntChannel > 13) {
      // Every other channel has been looked at; this scan is the node's own
      // channel, and scanComplete() decides with the tally.
      huntChannel = 0;
      huntPending = true;
    } else {
      slice = huntChannel;
      scanChannel = slice;
    }
  }
  if (allChannels) scanChannel = 0;
#ifdef ESP32
  int16_t started = WiFi.scanNetworks(true, hidden, false,
                                      (allChannels || slice) ? 120U : 300U,
                                      scanChannel);
#elif defined(ESP8266)
  // WiFi.scanNetworksAsync([&](int networks) { this->scanComplete(); }, true);
  // Try 600 times (60 seconds). If not completed after that, give up
  asyncTask.set(100 * TASK_MILLISECOND, 600, [this]() {
    auto num = WiFi.scanComplete();
    if (num == WIFI_SCAN_FAILED || num > 0) {
      this->asyncTask.disable();
      this->scanComplete();
    }
  });
  mesh->mScheduler->addTask(asyncTask);
  asyncTask.enableDelayed();
  int16_t started = WiFi.scanNetworks(true, hidden, scanChannel);
#endif

  if (started == WIFI_SCAN_FAILED) {
    // The radio refused to start a scan — on ESP32 that is what it does
    // while the station is mid-association. No scan-done event will ever
    // come, so the ten-interval safety net below would be a five-minute
    // silence; a node that has just lost its uplink cannot afford it.
    Log(ERROR, "stationScan(): scan could not start, retrying in %d s\n",
        (int)(0.5 * SCAN_INTERVAL / TASK_SECOND));
#ifdef ESP8266
    asyncTask.disable();
#endif
    task.delay(0.5 * SCAN_INTERVAL);
    return;
  }
  scanRequested = true;
  scanAllChannels = allChannels;
  scanSlice = slice;

  task.delay(10 * SCAN_INTERVAL);  // Scan should be completed by then and next
                                   // step called. If not then we restart here.
  return;
}

void ICACHE_FLASH_ATTR StationScan::scanComplete() {
  using namespace painlessmesh::logger;
  if (!scanRequested) {
    // The scan-done event of a synchronous scan — channel re-detection or
    // a bridge takeover — whose results are consumed and deleted by the
    // code that ran it. Treating it as ours found nothing, logged a scan
    // failure, and rescanned immediately: on a rootless mesh every node
    // did that back to back, and an OTA transfer through them stalled.
    Log(CONNECTION, "scanComplete(): not this task's scan, ignoring\n");
    return;
  }

  auto num = WiFi.scanComplete();
  if (num == WIFI_SCAN_RUNNING) {
    // A stale scan-done event — a synchronous scan's, delivered after this
    // task started its own — while ours is still in flight. Ours is still
    // owed a result, so the flag stays; clearing it here left the real
    // completion ignored and the node never joined.
    Log(CONNECTION,
        "scanComplete(): a scan is still running, waiting for it\n");
    return;
  }
  scanRequested = false;
  Log(CONNECTION, "scanComplete(): Scan finished\n");

  aps.clear();
  Log(CONNECTION, "scanComplete():-- > Cleared old APs.\n");

  if (num == WIFI_SCAN_FAILED) {
    Log(ERROR, "wifi scan failed. Retrying....\n");
    task.forceNextIteration();
    return;
  }

  Log(CONNECTION, "scanComplete(): num = %d\n", num);

  // One slice of a sliced hunt: tally the mesh APs it saw on channels other
  // than this node's, then rest at home before the next slice. Nothing
  // else is decided until the hunt has covered every channel.
  uint8_t slice = scanSlice;
  scanSlice = 0;
  if (slice != 0) {
    size_t seen = 0;
    for (auto i = 0; i < num; ++i) {
      TSTRING found = WiFi.SSID(i);
      bool isMesh = found == ssid || (found.equals("") && mesh->_meshHidden);
      uint8_t ch = WiFi.channel(i);
      if (!isMesh || ch == mesh->_meshChannel ||
          !painlessmesh::gateway::isValidMeshChannel(ch))
        continue;
      ++huntCounts[ch];
      ++seen;
      int8_t rssi = WiFi.RSSI(i);
      if (!huntRssi.count(ch) || rssi > huntRssi[ch]) huntRssi[ch] = rssi;
    }
    Log(CONNECTION, "scanComplete(): hunt slice channel %u: %u mesh AP(s)\n",
        slice, (unsigned)seen);
    ++huntChannel;
    task.delay(1500 * TASK_MILLISECOND);
    return;
  }

  // A re-detection scan covered every channel. The mesh seen on a channel
  // other than this node's may be the partition it is looking for — or a
  // straggler: on the rig, a node still in gateway mode on the router's
  // channel while the others are already back on the mesh channel, seen
  // by a node that had three peers here and left them for it. What tells
  // the two apart is size. The mesh APs on each other channel are
  // counted; the channel with the most is the candidate, the strongest
  // signal breaks a tie, and whether to go is decided below. A sliced hunt
  // arrives here with its tally already made.
  bool redetecting = scanAllChannels || huntPending;
  scanAllChannels = false;
  uint8_t elsewhere = 0;
  int8_t elsewhereRssi = -128;
  size_t elsewhereCount = 0;
  std::map<uint8_t, size_t> meshApsOnChannel;
  if (huntPending) {
    huntPending = false;
    for (auto&& entry : huntCounts) {
      meshApsOnChannel[entry.first] = entry.second;
      int8_t rssi = huntRssi.count(entry.first) ? huntRssi[entry.first] : -128;
      if (entry.second > elsewhereCount ||
          (entry.second == elsewhereCount && rssi > elsewhereRssi)) {
        elsewhere = entry.first;
        elsewhereCount = entry.second;
        elsewhereRssi = rssi;
      }
    }
    Log(CONNECTION,
        "scanComplete(): sliced hunt done: mesh on %u other channel(s)%s\n",
        (unsigned)huntCounts.size(), elsewhere ? "" : ", none elsewhere");
    huntCounts.clear();
    huntRssi.clear();
  }

  for (auto i = 0; i < num; ++i) {
    WiFi_AP_Record_t record;
    record.ssid = WiFi.SSID(i);
    bool isMesh = record.ssid == ssid ||
                  (record.ssid.equals("") && mesh->_meshHidden);

    if (WiFi.channel(i) != mesh->_meshChannel) {
      if (redetecting && isMesh &&
          painlessmesh::gateway::isValidMeshChannel(WiFi.channel(i))) {
        uint8_t ch = WiFi.channel(i);
        size_t count = ++meshApsOnChannel[ch];
        if (count > elsewhereCount ||
            (count == elsewhereCount && WiFi.RSSI(i) > elsewhereRssi)) {
          elsewhere = ch;
          elsewhereCount = count;
          elsewhereRssi = WiFi.RSSI(i);
        }
      }
      continue;
    }

    if (record.ssid != ssid) {
      if (record.ssid.equals("") && mesh->_meshHidden) {
        // Hidden mesh
        record.ssid = ssid;
      } else {
        continue;
      }
    }

    record.rssi = WiFi.RSSI(i);
    if (record.rssi == 0) continue;

    memcpy((void *)&record.bssid, (void *)WiFi.BSSID(i), sizeof(record.bssid));
    aps.push_back(record);
    Log(CONNECTION, "\tfound : %s, %ddBm\n", record.ssid.c_str(),
        (int16_t)record.rssi);
  }

  Log(CONNECTION, "\tFound %d nodes\n", aps.size());

  if (redetecting) {
    // A disconnected node follows the mesh wherever it is. A connected
    // node is already in a partition, and leaves it only for a bigger
    // one: the APs it can see on its own channel against those on the
    // other. Strictly bigger — at the start of a test, with one AP up on
    // each channel, a tie is exactly the straggler case, and following it
    // took a node and its subtree out of the mesh for a minute at a time.
    // A stranded partition still finds a bridge that has moved: its top
    // node lost its station link and follows unconditionally, and each
    // node it takes along drops its own children the same way.
    // Size cannot tell where the root is: a bridge that has just moved to
    // the router's channel is one AP against the rest of the mesh, and it
    // is the one to follow. Time can: a node still in gateway mode during
    // the sequential teardown — the straggler the rig saw a node follow and
    // sit alone with for a minute — is gone by the next scan; a bridge, or
    // the partition that has formed around it, is not. So a bigger
    // partition elsewhere is followed at once, and a smaller one only when
    // the same channel shows the mesh on two consecutive re-detections. A
    // disconnected node makes no connection while it looks again: joined
    // to this channel it would be "connected", and the rootless partition
    // it joined would take a re-detection or two longer to leave.
    // Not even a bigger one at once: during the teardown two nodes still
    // in gateway mode outnumbered the one AP a connected node could see
    // on its own channel, and it left the soak for them.
    bool connected = WiFi.status() == WL_CONNECTED;
    bool follow = false;
    // Home first. A node that was rooted knows the bridge's channel, and a
    // bridge — the old one back, or the backup promoted in its place — is
    // pinned to it by its router. Away from home with the mesh visible
    // there, home is the channel to follow whatever the sizes; at home
    // with a partition here, nothing elsewhere is worth leaving for. A
    // node alone at home with nothing here follows the mesh as before,
    // and comes back with it when a bridge appears.
    bool atHome = rootedChannel != 0 && mesh->_meshChannel == rootedChannel;
    if (rootedChannel != 0 && !atHome && meshApsOnChannel[rootedChannel] > 0 &&
        elsewhere != rootedChannel) {
      Log(CONNECTION,
          "scanComplete(): Mesh on channel %d, where it was rooted; going "
          "there rather than channel %d\n",
          rootedChannel, elsewhere);
      elsewhere = rootedChannel;
      elsewhereCount = meshApsOnChannel[rootedChannel];
    }
    if (atHome && elsewhere > 0 && (connected || !aps.empty())) {
      if (++homeStays >= 4) {
        Log(CONNECTION,
            "scanComplete(): The mesh has been elsewhere for %u re-detections "
            "with no root here; forgetting this as home\n",
            (unsigned)homeStays);
        rootedChannel = 0;
        homeStays = 0;
      } else {
        Log(CONNECTION,
            "scanComplete(): Mesh also on channel %d with %u nodes; this is "
            "the channel the mesh was rooted on, staying (%u of 4)\n",
            elsewhere, (unsigned)elsewhereCount, (unsigned)homeStays);
        elsewhere = 0;
      }
    } else if (atHome) {
      homeStays = 0;
    }
    if (elsewhere > 0) {
      if (!connected && aps.empty()) {
        follow = true;  // nothing here to lose, nothing to look again for
      } else if (pendingElsewhere == elsewhere) {
        follow = true;  // still there a scan later: not a straggler
      } else {
        Log(CONNECTION,
            "scanComplete(): Mesh also on channel %d with %u nodes, %u "
            "here; looking again before following\n",
            elsewhere, (unsigned)elsewhereCount, (unsigned)aps.size());
        pendingElsewhere = elsewhere;
        redetectRequested = true;
        // Look again soon, connected or not. A connected node used to fall
        // through to connectToAP(), whose "no root in sight" back-off put
        // the second look up to a minute away: on the rig the node the
        // bridge-discovery fixture sends from saw the new bridge's channel
        // at 103 s and looked again at 172 s, and the fixture's window had
        // closed. Skipping one round of connectToAP() costs nothing the
        // second look does not give back.
        aps.clear();
        task.delay(0.5 * SCAN_INTERVAL);
        return;
      }
    } else {
      pendingElsewhere = 0;
    }
    if (follow) pendingElsewhere = 0;
    if (follow) {
      Log(CONNECTION,
          "scanComplete(): Mesh found on different channel %d (was %d): %u "
          "nodes there, %u here; following it\n",
          elsewhere, mesh->_meshChannel, (unsigned)elsewhereCount,
          (unsigned)aps.size());
      // followBridgeChannel() does the whole move: it closes the station
      // link, so an orphan actually leaves its old partition instead of
      // restarting its AP on the new channel while still attached to the
      // old one.
      followBridgeChannel(elsewhere);
      return;
    }
    if (elsewhere > 0) {
      Log(CONNECTION,
          "scanComplete(): Mesh also on channel %d with %u nodes; this "
          "channel has %u, staying\n",
          elsewhere, (unsigned)elsewhereCount, (unsigned)aps.size());
      consecutiveEmptyScans = 0;
    } else if (aps.empty()) {
      // The mesh is on no channel at all. The empty-scan count stands: it
      // is what lets an isolated bridge retry once it passes
      // ISOLATED_BRIDGE_RETRY_SCAN_THRESHOLD.
      Log(CONNECTION,
          "scanComplete(): Mesh not found on any channel during re-scan\n");
    } else {
      Log(CONNECTION,
          "scanComplete(): Mesh found on current channel %d, no channel "
          "change needed\n",
          mesh->_meshChannel);
      consecutiveEmptyScans = 0;
    }
  }

  task.yield([this]() {
    // Task filter all unknown
    filterAPs();

    lastAPs = aps;

    // Next task is to sort by strength
    task.yield([this] {
      aps.sort([](WiFi_AP_Record_t a, WiFi_AP_Record_t b) {
        return a.rssi > b.rssi;
      });
      // Next task is to connect to the top ap
      task.yield([this]() { connectToAP(); });
    });
  });
}

void ICACHE_FLASH_ATTR StationScan::blockNodeAfterTCPFailure(uint32_t nodeId, uint32_t blockDurationMs) {
  using namespace painlessmesh::logger;
  uint32_t blockUntil = millis() + blockDurationMs;
  tcpFailureBlocklist[nodeId] = blockUntil;
  Log(CONNECTION, "blockNodeAfterTCPFailure(): Node %u blocked until %u (duration: %u ms)\n",
      nodeId, blockUntil, blockDurationMs);
}

bool ICACHE_FLASH_ATTR StationScan::isNodeBlocked(uint32_t nodeId) const {
  auto it = tcpFailureBlocklist.find(nodeId);
  if (it == tcpFailureBlocklist.end()) {
    return false;  // Not in blocklist
  }
  
  uint32_t now = millis();
  uint32_t blockUntil = it->second;
  
  // Handle millis() rollover using signed arithmetic
  // If blockUntil - now is positive and < MILLIS_ROLLOVER_THRESHOLD, the block is still active
  int32_t timeRemaining = (int32_t)(blockUntil - now);
  return (timeRemaining > 0 && timeRemaining < MILLIS_ROLLOVER_THRESHOLD);
}

void ICACHE_FLASH_ATTR StationScan::cleanupBlocklist() {
  using namespace painlessmesh::logger;
  uint32_t now = millis();
  
  auto it = tcpFailureBlocklist.begin();
  while (it != tcpFailureBlocklist.end()) {
    uint32_t blockUntil = it->second;
    int32_t timeRemaining = (int32_t)(blockUntil - now);
    
    // Remove expired entries (timeRemaining <= 0 or in far future due to rollover)
    if (timeRemaining <= 0 || timeRemaining >= MILLIS_ROLLOVER_THRESHOLD) {
      Log(CONNECTION, "cleanupBlocklist(): Removing expired entry for node %u\n", it->first);
      it = tcpFailureBlocklist.erase(it);
    } else {
      ++it;
    }
  }
}

void ICACHE_FLASH_ATTR StationScan::filterAPs() {
  // First, clean up expired blocklist entries
  cleanupBlocklist();
  
  auto ap = aps.begin();
  while (ap != aps.end()) {
    auto apNodeId = painlessmesh::tcp::encodeNodeId(ap->bssid);
    
    // Filter out nodes we're already connected to
    if (painlessmesh::router::findRoute<painlessmesh::Connection>(
            (*mesh), apNodeId) != NULL) {
      ap = aps.erase(ap);
      continue;
    }
    
    // Filter out nodes that are temporarily blocked due to TCP failures
    if (isNodeBlocked(apNodeId)) {
      using namespace painlessmesh::logger;
      Log(CONNECTION, "filterAPs(): Skipping blocked node %u (TCP server unresponsive)\n", apNodeId);
      ap = aps.erase(ap);
      continue;
    }
    
    ap++;
  }
}

void ICACHE_FLASH_ATTR StationScan::requestIP(WiFi_AP_Record_t &ap) {
  using namespace painlessmesh::logger;
  Log(CONNECTION, "connectToAP(): Best AP is %u<---\n",
      painlessmesh::tcp::encodeNodeId(ap.bssid));
  Log(CONNECTION, "requestIP(): Connecting to %s (channel: %d, BSSID: %02X:%02X:%02X:%02X:%02X:%02X)\n", 
      ap.ssid.c_str(), 
      mesh->_meshChannel,
      ap.bssid[0], ap.bssid[1], ap.bssid[2], 
      ap.bssid[3], ap.bssid[4], ap.bssid[5]);
  connectAttemptStarted = millis();
  halfOpenDropped = false;
  WiFi.begin(ap.ssid.c_str(), password.c_str(), mesh->_meshChannel, ap.bssid);
  return;
}

void ICACHE_FLASH_ATTR StationScan::connectToAP() {
  using namespace painlessmesh;
  using namespace painlessmesh::logger;
  // Next task will be to rescan
  task.setCallback([this]() { stationScan(); });

  if (manual) {
    if ((WiFi.SSID() == ssid) && WiFi.status() == WL_CONNECTED) {
      Log(CONNECTION,
          "connectToAP(): Already connected using manual connection. "
          "Disabling scanning.\n");
      task.disable();
      return;
    } else {
      if (WiFi.status() == WL_CONNECTED) {
        Log.remote("Close Sta because trying to connect manually\n");
        mesh->closeConnectionSTA();
        task.enableDelayed(10 * SCAN_INTERVAL);
        return;
      } else {
        // For manual router connections, reconnect directly using WiFi.begin()
        // Don't rely on scan results since router may be on different channel
        Log(CONNECTION, 
            "connectToAP(): Manual connection - attempting to reconnect to %s\n",
            ssid.c_str());
        WiFi.begin(ssid.c_str(), password.c_str());
        task.enableDelayed(SCAN_INTERVAL);
        return;
      }
    }
  }

#ifdef ESP32
  if (WiFi.status() == WL_IDLE_STATUS && !halfOpenDropped &&
      millis() - connectAttemptStarted > (uint32_t)(0.5 * SCAN_INTERVAL)) {
    // The Arduino core reports WL_IDLE_STATUS from association until an
    // address arrives. Half a scan interval after the attempt began, that
    // means this station is associated with an AP that never gave it an
    // address — a peer whose DHCP server was restarting, or one that
    // rebooted under it. Nothing times that out: no disconnect event comes,
    // and the mesh never learns of the failure. Drop the half-open link;
    // the disconnect event schedules the rescan.
    //
    // Once per attempt. When the status is WL_IDLE_STATUS with nothing
    // to disconnect — the core's own retry left it there — the disconnect
    // changes nothing, and this guard, firing on every pass, returned
    // before the scan results were ever looked at: the failover test's
    // sender logged "dropping it" every thirty seconds with the count
    // growing past three minutes and never connected to anything again.
    // The second pass falls through to the scan, whose requestIP() starts
    // a fresh attempt.
    Log(CONNECTION,
        "connectToAP(): Station associated without an address for %u ms, "
        "dropping it\n",
        millis() - connectAttemptStarted);
    halfOpenDropped = true;
    WiFi.disconnect();
    task.delay(SCAN_INTERVAL);  // Only reached if the event never fires
    return;
  }
#endif
  bool isRooted = layout::isRooted(mesh->asNodeTree());
  if (isRooted) {
    everRooted = true;
    rootedChannel = mesh->_meshChannel;
    homeStays = 0;
  }
  if (aps.empty()) {
    // No unknown nodes found
    consecutiveEmptyScans++;
    partitionScans = 0;  // nothing unrouted in sight: not partitioned
    
    // Re-detect the mesh channel once the empty scans pile up. Two cases
    // need it, and the second used to be excluded:
    //   - the station is disconnected, so the mesh has left this channel;
    //   - the station is connected but to a partition with no root while the
    //     mesh is meant to have one — the node is orphaned. That is what a
    //     bridge start does to everyone it does not directly serve: it moves
    //     to the router's channel, the nodes it served drop and re-scan, and
    //     the nodes behind *them* stay connected to each other on the old
    //     channel, filter their peers as known, count empty scans, and were
    //     gated out of re-detection by the WL_CONNECTED check for good.
    bool orphaned = mesh->shouldContainRoot && !isRooted;
    if (consecutiveEmptyScans >= EMPTY_SCAN_THRESHOLD &&
        (WiFi.status() != WL_CONNECTED || orphaned) && channel > 0) {
      Log(CONNECTION,
          "connectToAP(): No mesh nodes found for %d scans%s, re-detecting "
          "the mesh channel on the next scan\n",
          consecutiveEmptyScans, orphaned ? " (connected but unrooted)" : "");
      // The next scan of this task covers every channel and scanComplete()
      // follows the mesh if it is elsewhere. It used to run a synchronous
      // all-channel scan right here: four to seven seconds with the main
      // loop held and the radio off the mesh channel, on every node of a
      // rootless mesh in turn, and an ACK owed through the scanning node
      // arrived after the sender's budget — the soak's recurring loss.
      // The re-detection is one scan interval later than it was; the
      // interval below is the disconnected node's fast one or the orphan's
      // backed-off one, so a stranded follower still catches up within
      // the gateway contract, and a mesh that is simply rootless is not
      // deaf for seconds at a time.
      redetectRequested = true;
    }
    
    if (WiFi.status() == WL_CONNECTED &&
        !(mesh->shouldContainRoot && !isRooted)) {
      // if already connected -> scan slow
      Log(CONNECTION,
          "connectToAP(): Already connected, and no unknown nodes found: "
          "scan rate set to slow\n");
      task.delay(4 * SCAN_INTERVAL);
    } else if (orphaned && WiFi.status() == WL_CONNECTED) {
      // Connected, told the mesh has a root, and not seeing one. The first
      // re-detections come quickly — that is how a follower stranded by a
      // bridge's channel move catches up — but a mesh that is simply
      // rootless must not keep every node scanning all channels every
      // half interval for as long as it stays so. Back off to two
      // intervals; anything new on the air resets it.
      // A leaf that has re-detected twice while connected and rootless,
      // and found the mesh only on its own channel, is in a rootless
      // partition that its scans cannot get it out of: every AP it can
      // see is "known" — in its tree — including a bridge that was
      // promoted a minute ago and is listed where it used to be, before
      // its restart. On the rig the failover test's sender sat like that
      // through the whole promotion window. Dropping the station link
      // empties the tree, so the next scan sees every AP as new and the
      // bridge's among them. Only a leaf: an interior node would take its
      // subtree with it. The count resets when anything new is heard.
      // And only if this mesh ever had a root: one that never did is
      // rootless by design, and its leaves must not keep leaving.
      size_t apChildren = 0;
      for (auto&& sub : mesh->subs) {
        if (sub->connected() && !sub->station) ++apChildren;
      }
      // Not from home, and not a failover candidate. At home the root will
      // reappear here — the old bridge back, or a backup promoted in this
      // partition — and a candidate must stay connected to hold the
      // election at all: the bridge monitor skips a node with no mesh
      // connections. On the rig the backup left its partition at 90 s,
      // which skipped the election, rejoined, sent its candidacy at 129 s
      // and left again at 133 s, before the votes were counted.
      bool atHome = rootedChannel != 0 && mesh->_meshChannel == rootedChannel;
      bool candidate =
          mesh->bridgeFailoverEnabled && mesh->routerCredentialsConfigured;
      bool stranded = ++orphanRedetects >= 2 && apChildren == 0 && everRooted;
      if (stranded && (atHome || candidate)) {
        Log(CONNECTION,
            "connectToAP(): Still no root after %u re-detections; staying: "
            "%s\n",
            (unsigned)orphanRedetects,
            candidate ? "this node is a failover candidate"
                      : "this is the channel the mesh was rooted on");
      }
      if (stranded && !atHome && !candidate) {
        Log(CONNECTION,
            "connectToAP(): Still no root after %u re-detections and nothing "
            "new in sight; leaving this partition to look for it\n",
            (unsigned)orphanRedetects);
        orphanRedetects = 0;
        orphanScanBackoff = 0;
        mesh->closeConnectionSTA();
        mesh->stability = 0;
        task.delay(0.5 * SCAN_INTERVAL);
        return;
      }
      uint32_t interval = (0.5 * SCAN_INTERVAL) * (1u << orphanScanBackoff);
      Log(CONNECTION,
          "connectToAP(): No root in sight, next scan in %u s\n",
          (unsigned)(interval / TASK_SECOND));
      task.delay(interval);
      if (orphanScanBackoff < 2) orphanScanBackoff++;
    } else {
      // else scan fast (SCAN_INTERVAL)
      Log(CONNECTION,
          "connectToAP(): No unknown nodes found scan rate set to "
          "fast\n");
      task.setInterval(0.5 * SCAN_INTERVAL);
    }
    mesh->stability += min(1000 - mesh->stability, (size_t)25);
  } else {
    // Reset counter when APs are found
    consecutiveEmptyScans = 0;
    orphanScanBackoff = 0;
    if (WiFi.status() == WL_CONNECTED) {
      // TODO: Use %u instead of String() here and below
      // Also prob is always equal to stability, so we should use that directly
      Log(CONNECTION,
          "connectToAP(): Unknown nodes found. Current stability: %s\n",
          String(mesh->stability).c_str());

      // A node that is connected, told the mesh has a root, cannot see one,
      // and can see nodes it has no route to is in a partition — and the
      // root is in the other one. The probabilistic reconfigure below is
      // gated by `stability`, which only grows on scans that find nothing
      // unknown, so a partitioned node's probability is near zero after
      // its first attempt and the partition stands. On the rig one such
      // partition held for the whole of a five-minute OTA transfer while
      // the receiver's ten requests went to a sender it had no route to.
      // Two consecutive scans showing the other partition is enough grace
      // for a transient; then it reconnects, deterministically.
      // Only a leaf may jump. An interior node that drops its station link
      // takes its whole subtree with it and creates the fragmentation it
      // was meant to heal — measured: fourteen such jumps in one suite and
      // every delivery test failed. Leaves jumping one at a time still
      // converge: each leaf that leaves makes its parent a leaf.
      size_t apChildren = 0;
      for (auto&& sub : mesh->subs) {
        if (sub->connected() && !sub->station) ++apChildren;
      }
      if (!isRooted && mesh->shouldContainRoot && apChildren == 0) {
        ++partitionScans;
      } else {
        partitionScans = 0;
      }
      int prob = mesh->stability;
      if (!isRooted && (partitionScans >= 2 || random(0, 1000) < prob)) {
        if (partitionScans >= 2) {
          Log(CONNECTION,
              "connectToAP(): Nodes without a route seen on %u scans while "
              "unrooted; joining that partition\n",
              partitionScans);
          partitionScans = 0;
        }
        Log(CONNECTION, "connectToAP(): Reconfigure network: %s\n",
            String(prob).c_str());
        // close STA connection, this will trigger station disconnect which
        // will trigger connectToAP()
        mesh->closeConnectionSTA();
        mesh->stability = 0;  // Discourage switching again
        Log.remote("Close Sta to reconfigure network\n");
        // wifiEventCB should be triggered before this delay runs out
        // and reset the connecting
        task.delay(4 * SCAN_INTERVAL);
      } else {
        if (!isRooted && mesh->shouldContainRoot)
          // Increase scanning rate, because we want to find root
          task.delay(0.5 * SCAN_INTERVAL);
        else
          task.delay(4 * SCAN_INTERVAL);
      }
    } else {
      // Else try to connect to first
      auto ap = aps.front();
      aps.pop_front();  // drop bestAP from mesh list, so if doesn't work out,
                        // we can try the next one
      requestIP(ap);
      // A rejected attempt raises a disconnect event, which rescans at
      // once; this delay only bounds the silent failures — an association
      // that never gets an address — and two minutes was too long for a
      // node whose bridge has just moved.
      Log(CONNECTION,
          "connectToAP(): Trying to connect, next scan in one interval\n");
      task.delay(SCAN_INTERVAL);
    }
  }
}

bool ICACHE_FLASH_ATTR StationScan::followBridgeChannel(
    uint8_t targetChannel) {
  using namespace painlessmesh::logger;

  if (!painlessmesh::gateway::isValidMeshChannel(targetChannel) ||
      mesh == nullptr) {
    Log(ERROR,
        "followBridgeChannel(): Ignoring invalid bridge channel %u\n",
        targetChannel);
    return false;
  }

  if (mesh->_meshChannel == targetChannel) return false;

  uint8_t previousChannel = mesh->_meshChannel;
  Log(CONNECTION,
      "followBridgeChannel(): Moving mesh from channel %u to bridge channel "
      "%u\n",
      previousChannel, targetChannel);

  // Discard any asynchronous result from the old channel before changing the
  // radio, otherwise its callback can move the node back after the takeover.
  WiFi.scanDelete();
  task.disable();
  mesh->closeConnectionSTA();
  WiFi.disconnect();
  delay(100);

  mesh->_meshChannel = targetChannel;
  channel = targetChannel;
  consecutiveEmptyScans = 0;

  if (WiFi.getMode() & WIFI_AP) {
    WiFi.softAPdisconnect(true);
    delay(100);
    mesh->apInit(mesh->getNodeId());
    delay(100);
  }

  // Resume discovery immediately. The old recovery path required repeated
  // empty scans and exceeded the gateway failover contract.
  orphanScanBackoff = 0;
  task.enable();
  task.forceNextIteration();
  return true;
}

// Helper function to scan all channels for a specific mesh SSID
// Returns the channel number if found, or 0 if not found
uint8_t ICACHE_FLASH_ATTR StationScan::scanForMeshChannel(TSTRING meshSSID, bool meshHidden,
                                                          uint8_t avoidChannel,
                                                          TSTRING routerSSID) {
  using namespace painlessmesh::logger;
  Log(CONNECTION, "scanForMeshChannel(): Scanning all channels for mesh '%s'...\n", meshSSID.c_str());
  
  // Scan all channels (0 means scan all)
#ifdef ESP32
  int16_t numNetworks = WiFi.scanNetworks(false, meshHidden, false, 300U, 0);
#elif defined(ESP8266)
  int16_t numNetworks = WiFi.scanNetworks(false, meshHidden, 0);
#endif
  
  if (numNetworks == WIFI_SCAN_FAILED) {
    Log(ERROR, "scanForMeshChannel(): WiFi scan failed\n");
    // The only exit that used to leave the results allocated, and the one
    // taken when the radio is already busy — which is exactly when a node
    // is retrying this every re-detection interval.
    WiFi.scanDelete();
    return 0;
  }
  
  Log(CONNECTION, "scanForMeshChannel(): Found %d networks\n", numNetworks);
  
  // Collect every channel the mesh is on, then choose. Returning the first
  // match made a stranded node's fate depend on scan order: seeing its own
  // partition first, it concluded nothing had changed and stayed put.
  std::vector<painlessmesh::gateway::MeshChannelCandidate> candidates;
  uint8_t routerChannel = 0;
  for (int16_t i = 0; i < numNetworks; ++i) {
    TSTRING foundSSID = WiFi.SSID(i);
    uint8_t foundChannel = WiFi.channel(i);
    int32_t rssi = WiFi.RSSI(i);

    if (routerSSID.length() > 0 && foundSSID == routerSSID &&
        foundChannel >= 1 && foundChannel <= 13) {
      Log(CONNECTION,
          "scanForMeshChannel(): Router %s on channel %d (RSSI: %d)\n",
          routerSSID.c_str(), foundChannel, rssi);
      routerChannel = foundChannel;
    }
    if (foundSSID == meshSSID || (foundSSID == "" && meshHidden)) {
      if (foundChannel >= 1 && foundChannel <= 13) {
        Log(CONNECTION, "scanForMeshChannel(): Found mesh on channel %d (RSSI: %d)\n",
            foundChannel, rssi);
        candidates.push_back({foundChannel, rssi});
      } else {
        Log(ERROR, "scanForMeshChannel(): Found mesh on invalid channel %d, ignoring\n",
            foundChannel);
      }
    }
  }

  uint8_t chosen = painlessmesh::gateway::pickMeshChannel(candidates, avoidChannel,
                                                          routerChannel);
  if (chosen != 0) {
    if (routerChannel != 0 && chosen == routerChannel && candidates.size() > 1) {
      Log(CONNECTION,
          "scanForMeshChannel(): Mesh on %u channels; taking the router's, "
          "channel %d, where a bridge would be\n",
          (unsigned)candidates.size(), chosen);
    }
    if (avoidChannel != 0 && chosen != avoidChannel) {
      Log(CONNECTION,
          "scanForMeshChannel(): Mesh also on channel %d; preferring it over "
          "current channel %d\n",
          chosen, avoidChannel);
    }
    WiFi.scanDelete();
    return chosen;
  }

  Log(CONNECTION, "scanForMeshChannel(): Mesh '%s' not found on any channel\n", meshSSID.c_str());
  WiFi.scanDelete();
  return 0;  // Not found
}

#endif
