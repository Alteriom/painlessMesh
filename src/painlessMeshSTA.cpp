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

  task.set(SCAN_INTERVAL, TASK_FOREVER, [this]() { stationScan(); });
}

// Starts scan for APs whose name is Mesh SSID
void ICACHE_FLASH_ATTR StationScan::stationScan() {
  using namespace painlessmesh::logger;
  Log(CONNECTION, "stationScan(): %s\n", ssid.c_str());
  
  // If channel is 0, auto-detect the mesh channel first
  if (channel == 0) {
    Log(STARTUP, "stationScan(): Auto-detecting mesh channel...\n");
    uint8_t detectedChannel = scanForMeshChannel(ssid, hidden);
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

#ifdef ESP32
  int16_t started = WiFi.scanNetworks(true, hidden, false, 300U, channel);
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
  int16_t started = WiFi.scanNetworks(true, hidden, channel);
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

  for (auto i = 0; i < num; ++i) {
    WiFi_AP_Record_t record;
    record.ssid = WiFi.SSID(i);

    if (WiFi.channel(i) != mesh->_meshChannel) {
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
  if (WiFi.status() == WL_IDLE_STATUS &&
      millis() - connectAttemptStarted > (uint32_t)(0.5 * SCAN_INTERVAL)) {
    // The Arduino core reports WL_IDLE_STATUS from association until an
    // address arrives. Half a scan interval after the attempt began, that
    // means this station is associated with an AP that never gave it an
    // address — a peer whose DHCP server was restarting, or one that
    // rebooted under it. Nothing times that out: no disconnect event comes,
    // and the mesh never learns of the failure. Drop the half-open link;
    // the disconnect event schedules the rescan.
    Log(CONNECTION,
        "connectToAP(): Station associated without an address for %u ms, "
        "dropping it\n",
        millis() - connectAttemptStarted);
    WiFi.disconnect();
    task.delay(SCAN_INTERVAL);  // Only reached if the event never fires
    return;
  }
#endif
  bool isRooted = layout::isRooted(mesh->asNodeTree());
  if (aps.empty()) {
    // No unknown nodes found
    consecutiveEmptyScans++;
    
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
        (WiFi.status() != WL_CONNECTED || orphaned) &&
        channel > 0) {
      Log(CONNECTION,
          "connectToAP(): No mesh nodes found for %d scans%s, triggering channel re-detection\n",
          consecutiveEmptyScans, orphaned ? " (connected but unrooted)" : "");

      // Prefer a partition on another channel: the mesh visible on this one
      // is the partition we are stranded in.
      uint8_t detectedChannel = scanForMeshChannel(ssid, hidden, mesh->_meshChannel);
      if (detectedChannel > 0 && detectedChannel != mesh->_meshChannel) {
        Log(CONNECTION,
            "connectToAP(): Mesh found on different channel %d (was %d), following it\n",
            detectedChannel, mesh->_meshChannel);
        // followBridgeChannel() does the whole move: it closes the station
        // link, so an orphan actually leaves its old partition instead of
        // restarting its AP on the new channel while still attached to the
        // old one — which is what the inline copy this replaces did.
        followBridgeChannel(detectedChannel);
        return;
      } else if (detectedChannel == 0) {
        Log(CONNECTION,
            "connectToAP(): Mesh not found on any channel during re-scan\n");
        // Do NOT reset consecutiveEmptyScans here - mesh is still absent
        // This allows isolated bridge retry mechanism to trigger when
        // the counter exceeds ISOLATED_BRIDGE_RETRY_SCAN_THRESHOLD
      } else {
        // detectedChannel == mesh->_meshChannel
        // Mesh found on same channel we're already on - no channel change needed
        // Reset counter since mesh exists, nodes may appear in subsequent scans
        Log(CONNECTION,
            "connectToAP(): Mesh found on current channel %d, no channel change needed\n",
            detectedChannel);
        consecutiveEmptyScans = 0;
      }
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

      int prob = mesh->stability;
      if (!isRooted && random(0, 1000) < prob) {
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
                                                          uint8_t avoidChannel) {
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
    return 0;
  }
  
  Log(CONNECTION, "scanForMeshChannel(): Found %d networks\n", numNetworks);
  
  // Collect every channel the mesh is on, then choose. Returning the first
  // match made a stranded node's fate depend on scan order: seeing its own
  // partition first, it concluded nothing had changed and stayed put.
  std::vector<painlessmesh::gateway::MeshChannelCandidate> candidates;
  for (int16_t i = 0; i < numNetworks; ++i) {
    TSTRING foundSSID = WiFi.SSID(i);
    uint8_t foundChannel = WiFi.channel(i);
    int32_t rssi = WiFi.RSSI(i);

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

  uint8_t chosen = painlessmesh::gateway::pickMeshChannel(candidates, avoidChannel);
  if (chosen != 0) {
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
