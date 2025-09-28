#ifndef _PAINLESS_MESH_STA_H_
#define _PAINLESS_MESH_STA_H_

#include "painlessmesh/configuration.hpp"

#include "painlessmesh/mesh.hpp"

#include <list>

typedef struct {
  uint8_t bssid[6];
  TSTRING ssid;
  int8_t rssi;
} WiFi_AP_Record_t;

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

  friend painlessMesh;
};

#endif
