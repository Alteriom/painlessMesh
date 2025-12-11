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
  if (channel == 0 && mesh->_meshChannel == 0) {
    Log(STARTUP, "stationScan(): Auto-detecting mesh channel...\n");
    uint8_t detectedChannel = scanForMeshChannel(ssid, hidden);
    if (detectedChannel > 0) {
      mesh->_meshChannel = detectedChannel;
      channel = detectedChannel;
      Log(STARTUP, "stationScan(): Mesh channel auto-detected: %d\n", detectedChannel);
    } else {
      // Mesh not found, fall back to channel 1
      mesh->_meshChannel = 1;
      channel = 1;
      Log(CONNECTION, "stationScan(): Mesh not found, falling back to channel 1\n");
    }
  }

#ifdef ESP32
  WiFi.scanNetworks(true, hidden, false, 300U, channel);
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
  WiFi.scanNetworks(true, hidden, channel);
#endif

  task.delay(10 * SCAN_INTERVAL);  // Scan should be completed by then and next
                                   // step called. If not then we restart here.
  return;
}

void ICACHE_FLASH_ATTR StationScan::scanComplete() {
  using namespace painlessmesh::logger;
  Log(CONNECTION, "scanComplete(): Scan finished\n");

  aps.clear();
  Log(CONNECTION, "scanComplete():-- > Cleared old APs.\n");

  auto num = WiFi.scanComplete();
  if (num == WIFI_SCAN_FAILED) {
    Log(ERROR, "wifi scan failed. Retrying....\n");
    task.forceNextIteration();
    return;
  } else if (num == WIFI_SCAN_RUNNING) {
    Log(ERROR,
        "scanComplete should never be called when scan is still running.\n");
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

void ICACHE_FLASH_ATTR StationScan::filterAPs() {
  auto ap = aps.begin();
  while (ap != aps.end()) {
    auto apNodeId = painlessmesh::tcp::encodeNodeId(ap->bssid);
    if (painlessmesh::router::findRoute<painlessmesh::Connection>(
            (*mesh), apNodeId) != NULL) {
      ap = aps.erase(ap);
    } else {
      ap++;
    }
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
  bool isRooted = layout::isRooted(mesh->asNodeTree());
  if (aps.empty()) {
    // No unknown nodes found
    consecutiveEmptyScans++;
    
    // If we've had multiple consecutive empty scans and we're not connected,
    // trigger a full channel re-scan to find the mesh
    if (consecutiveEmptyScans >= EMPTY_SCAN_THRESHOLD && 
        WiFi.status() != WL_CONNECTED &&
        channel > 0) {
      Log(CONNECTION,
          "connectToAP(): No mesh nodes found for %d scans, triggering channel re-detection\n",
          consecutiveEmptyScans);
      
      // Perform full channel scan to find the mesh
      uint8_t detectedChannel = scanForMeshChannel(ssid, hidden);
      if (detectedChannel > 0 && detectedChannel != mesh->_meshChannel) {
        Log(CONNECTION,
            "connectToAP(): Mesh found on different channel %d (was %d), updating...\n",
            detectedChannel, mesh->_meshChannel);
        
        // Update mesh channel
        uint8_t oldChannel = mesh->_meshChannel;
        mesh->_meshChannel = detectedChannel;
        channel = detectedChannel;
        
        // Restart AP on new channel to match the mesh
        // This ensures this node's AP is also discoverable on the correct channel
        if (WiFi.getMode() & WIFI_AP) {
          Log(CONNECTION,
              "connectToAP(): Restarting AP from channel %d to channel %d\n",
              oldChannel, detectedChannel);
          
          // Disconnect AP and allow WiFi stack to fully reset
          // Using true parameter ensures DHCP server is properly stopped
          WiFi.softAPdisconnect(true);
          delay(200);  // Increased delay to ensure complete WiFi stack reset
          
          // Call apInit via friend class access (StationScan is friend of wifi::Mesh)
          mesh->apInit(mesh->getNodeId());
          
          // Additional stabilization delay after AP restart
          // This ensures DHCP server is fully initialized before clients connect
          delay(100);
          
          Log(CONNECTION, "connectToAP(): AP restarted on channel %d\n", detectedChannel);
        }
        // Reset counter only when mesh is found on a new channel
        // This allows isolated bridge retry to continue when mesh is truly absent
        consecutiveEmptyScans = 0;
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
      // Trying to connect, if that fails we will reconnect later
      Log(CONNECTION,
          "connectToAP(): Trying to connect, scan rate set to "
          "4*normal\n");
      task.delay(4 * SCAN_INTERVAL);
    }
  }
}

// Helper function to scan all channels for a specific mesh SSID
// Returns the channel number if found, or 0 if not found
uint8_t ICACHE_FLASH_ATTR StationScan::scanForMeshChannel(TSTRING meshSSID, bool meshHidden) {
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
  
  // Search for the mesh SSID in scan results
  for (int16_t i = 0; i < numNetworks; ++i) {
    TSTRING foundSSID = WiFi.SSID(i);
    uint8_t foundChannel = WiFi.channel(i);
    int32_t rssi = WiFi.RSSI(i);
    
    // Check if this is our mesh network
    if (foundSSID == meshSSID || (foundSSID == "" && meshHidden)) {
      // Validate channel is in valid range (1-13 for 2.4GHz)
      if (foundChannel >= 1 && foundChannel <= 13) {
        Log(CONNECTION, "scanForMeshChannel(): Found mesh on channel %d (RSSI: %d)\n", 
            foundChannel, rssi);
        WiFi.scanDelete();
        return foundChannel;
      } else {
        Log(ERROR, "scanForMeshChannel(): Found mesh on invalid channel %d, ignoring\n", 
            foundChannel);
      }
    }
  }
  
  Log(CONNECTION, "scanForMeshChannel(): Mesh '%s' not found on any channel\n", meshSSID.c_str());
  WiFi.scanDelete();
  return 0;  // Not found
}

#endif
