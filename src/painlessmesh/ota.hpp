#ifndef _PAINLESS_MESH_PLUGIN_OTA_HPP_
#define _PAINLESS_MESH_PLUGIN_OTA_HPP_

#include "painlessmesh/configuration.hpp"

#include "painlessmesh/base64.hpp"
#include "painlessmesh/logger.hpp"
#include "painlessmesh/plugin.hpp"

#if !defined(USE_FS_SPIFFS) && !defined(USE_FS_LITTLEFS)
#if defined(ESP32) || defined(ESP8266)
#ifdef ESP32
#define USE_FS_SPIFFS
#else
#define USE_FS_LITTLEFS
#endif
#endif
#endif

#ifdef ESP32
#include <Update.h>
#endif

#if defined(USE_FS_SPIFFS)
#include <SPIFFS.h>
#elif defined(USE_FS_LITTLEFS)
#include <LittleFS.h>
#endif

namespace painlessmesh {
namespace plugin {

/** OTA over the mesh
 *
 * OTA is implemented as a painlessmesh::plugin.
 *
 * The protocol consists of three message types: ota::Announce, ota::DataRequest
 * and ota::Data. The first message is generally send by the node that
 * distributes the firmware and announces the current version of firmware
 * available for each hardware and role. Firmware version is determined by
 * its MD5 signature. See
 * [painlessMeshBoost](http://gitlab.com/painlessMesh/painlessMeshBoost) for a
 * possible implementation of a distribution node.
 *
 * Once a node receives an announce message it will check it against its own
 * role and hardware to discover if it is suitable this node. If that checks out
 * and the MD5 is different than its own MD5 it will send a data request back to
 * the firmware distribution node. This request also includes a partNo, to
 * determine which part of the data it needs (starting from zero).
 *
 * When the distribution node receives a data request, it sends the data back to
 * the node (with a data message). The node will then write this data and
 * request the next part of the data. This exchange continuous until the node
 * has all the data, written it and reboots into the new firmware.
 */
namespace ota {

/**
 * Operation codes for various firmware states
 */
enum class OTA_OP_CODES {
  ANNOUNCE = 10,      // Announce a new update
  DATA_REQUEST = 11,  // Request data from host
  DATA = 12,          // Inbound data to nodes
};

/** Package used by the firmware distribution node to announce new version
 * available
 *
 * This is based on the general BroadcastPackage to ensure it is being
 * broadcasted. It is possible to define a Announce::role, which defines the
 * node role the firmware is meant for.
 *
 * The package type/identifier is set to 10.
 */
class Announce : public BroadcastPackage {
 public:
  TSTRING md5;
  TSTRING hardware;

  /**
   * \brief The type of node the firmware is meant for
   *
   * Nodes can fulfill different roles, which require specific firmware. E.g a
   * node can be a sensor and therefore needs the firmware meant for sensor
   * nodes. This allows one to set the type of node (role) this firmware is
   * aimed at.
   *
   * Note that the role should not contain underscores or dots.
   */
  TSTRING role;

  /** Force an update even if the node already has this firmware version
   *
   * Mainly usefull when testing updates etc.
   */
  bool forced = false;

  /** Receive broadcast chunks. The default behavior is to have each node
   * request and receive each of its firwmare payload chunks, however this can
   * in some cases create a lot of additional network traffic. If broadcasted is
   * true, the root node will act on behalf of all other nodes in leading the
   * request for packets.
   */
  bool broadcasted = false;

  size_t noPart;

  Announce() : BroadcastPackage(10) {}

  Announce(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    md5 = jsonObj["md5"].as<TSTRING>();
    hardware = jsonObj["hardware"].as<TSTRING>();
    role = jsonObj["role"].as<TSTRING>();
#if ARDUINOJSON_VERSION_MAJOR < 7
    if (jsonObj.containsKey("forced")) forced = jsonObj["forced"];
    if (jsonObj.containsKey("broadcasted"))
      broadcasted = jsonObj["broadcasted"];
#else
    if (jsonObj["forced"].is<bool>()) forced = jsonObj["forced"];
    if (jsonObj["broadcasted"].is<bool>()) broadcasted = jsonObj["broadcasted"];
#endif
    noPart = jsonObj["noPart"];
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["md5"] = md5;
    jsonObj["hardware"] = hardware;
    jsonObj["role"] = role;
    if (forced) jsonObj["forced"] = forced;
    jsonObj["noPart"] = noPart;
    jsonObj["broadcasted"] = broadcasted;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 5) +
           round(1.1 * (md5.length() + hardware.length() + role.length()));
  }
#endif

 protected:
  Announce(int type, router::Type routing) : BroadcastPackage(type) {
    this->routing = routing;
  }
};

class Data;

/** Request (part of) the firmware update
 *
 * This is send by the node needing the new firmware, to the firmware
 * distribution node to request a part (DataRequest::partNo) of the data.
 *
 * The package type/identifier is set to 11.
 */
class DataRequest : public Announce {
 public:
  size_t partNo = 0;
  uint32_t dest = 0;

  DataRequest() : Announce(11, router::SINGLE) {}

  DataRequest(JsonObject jsonObj) : Announce(jsonObj) {
    dest = jsonObj["dest"];
    partNo = jsonObj["partNo"];
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = Announce::addTo(std::move(jsonObj));
    jsonObj["dest"] = dest;
    jsonObj["partNo"] = partNo;
    return jsonObj;
  }

  static DataRequest replyTo(const Announce& ann, uint32_t from,
                             size_t partNo) {
    DataRequest req;
    req.dest = ann.from;
    req.md5 = ann.md5;
    req.hardware = ann.hardware;
    req.role = ann.role;
    req.forced = ann.forced;
    req.noPart = ann.noPart;
    req.partNo = partNo;
    req.from = from;
    req.broadcasted = ann.broadcasted;
    return req;
  }

  static DataRequest replyTo(const Data& d, size_t partNo);

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 5 + 2) +
           round(1.1 * (md5.length() + hardware.length() + role.length()));
  }
#endif

 protected:
  DataRequest(int type) : Announce(type, router::SINGLE) {}
};

/** Package containing part of the firmware
 *
 * The package type/identifier is set to 12.
 */
class Data : public DataRequest {
 public:
  TSTRING data;

  Data() : DataRequest(12) {}

  Data(JsonObject jsonObj) : DataRequest(jsonObj) {
    data = jsonObj["data"].as<TSTRING>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = DataRequest::addTo(std::move(jsonObj));
    jsonObj["data"] = data;
    return jsonObj;
  }

  static Data replyTo(const DataRequest& req, TSTRING data, size_t partNo) {
    Data d;
    d.from = req.dest;
    d.dest = req.from;
    d.md5 = req.md5;
    d.hardware = req.hardware;
    d.role = req.role;
    d.forced = req.forced;
    d.noPart = req.noPart;
    d.partNo = partNo;
    d.data = data;
    d.broadcasted = req.broadcasted;
    return d;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 5 + 2 + 1) +
           round(1.1 * (md5.length() + hardware.length() + role.length() +
                        data.length()));
  }
#endif
};

inline DataRequest DataRequest::replyTo(const Data& d, size_t partNo) {
  DataRequest req;
  req.from = d.dest;
  req.dest = d.from;
  req.md5 = d.md5;
  req.hardware = d.hardware;
  req.role = d.role;
  req.forced = d.forced;
  req.noPart = d.noPart;
  req.partNo = partNo;
  req.broadcasted = d.broadcasted;
  return req;
}

/** Data related to the current state of the node update
 *
 * This class is used by the OTA algorithm to keep track of both the current
 * version of the software and the ongoing update.
 *
 * The firmware md5 uniquely identifies each firmware version
 */
class State : public protocol::PackageInterface {
 public:
  TSTRING md5;
#ifdef ESP32
  TSTRING hardware = "ESP32";
#else
  TSTRING hardware = "ESP8266";
#endif
  TSTRING role;
  size_t noPart = 0;
  size_t partNo = 0;
  bool broadcasted = false;
  TSTRING ota_fn = "/ota_fw.json";

  State() {}

  State(JsonObject jsonObj) {
    md5 = jsonObj["md5"].as<TSTRING>();
    hardware = jsonObj["hardware"].as<TSTRING>();
    role = jsonObj["role"].as<TSTRING>();
    broadcasted = jsonObj["broadcasted"].as<bool>();
  }

  State(const Announce& ann) {
    md5 = ann.md5;
    hardware = ann.hardware;
    role = ann.role;
    noPart = ann.noPart;
    broadcasted = ann.broadcasted;
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["role"] = role;
    jsonObj["md5"] = md5;
    jsonObj["hardware"] = hardware;
    jsonObj["broadcasted"] = broadcasted;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(3) +
           round(1.1 * (md5.length() + hardware.length() + role.length()));
  }
#endif

  std::shared_ptr<Task> task;
};

typedef std::function<size_t(painlessmesh::plugin::ota::DataRequest,
                             char* buffer)>
    otaDataPacketCallbackType_t;

template <class T>
void addSendPackageCallback(Scheduler& scheduler,
                            plugin::PackageHandler<T>& mesh,
                            otaDataPacketCallbackType_t callback,
                            size_t otaPartSize) {
  using namespace logger;
#if defined(ESP32) || defined(ESP8266)
  mesh.onPackage(
      (int)OTA_OP_CODES::DATA_REQUEST,
      [&mesh, callback, otaPartSize](painlessmesh::protocol::Variant& variant) {
        auto pkg = variant.to<painlessmesh::plugin::ota::DataRequest>();
        char buffer[otaPartSize + 1];
        memset(buffer, 0, otaPartSize + 1);
        auto size = callback(pkg, buffer);
        // Handle zero size
        if (!size) {
          // No data is available by the user app.

          // todo - doubtful, shall we return true or false. What is the purpose
          // of this return value.
          return true;
        }
        // Encode data as base64 so there are no null characters and can be
        // shown in plaintext
        auto b64Data =
            painlessmesh::base64::encode((unsigned char*)buffer, size);
        auto reply =
            painlessmesh::plugin::ota::Data::replyTo(pkg, b64Data, pkg.partNo);
        mesh.sendPackage(&reply);
        return true;
      });

#endif
}

template <class T>
void addReceivePackageCallback(
    Scheduler& scheduler, plugin::PackageHandler<T>& mesh, TSTRING role = "",
    std::function<void(int, int)> progress_cb = NULL) {
  using namespace logger;
#if defined(ESP32) || defined(ESP8266)
  auto currentFW = std::make_shared<State>();
  currentFW->role = role;
  auto updateFW = std::make_shared<State>();
  updateFW->role = role;
#ifdef USE_FS_SPIFFS
  SPIFFS.begin(true);  // Start the SPI Flash Files System
  if (SPIFFS.exists(currentFW->ota_fn)) {
    auto file = SPIFFS.open(currentFW->ota_fn, "r");
#else
  LittleFS.begin();  // Start the SPI Flash Files System
  if (LittleFS.exists(currentFW->ota_fn)) {
    auto file = LittleFS.open(currentFW->ota_fn, "r");
#endif
    TSTRING msg = "";
    while (file.available()) {
      msg += (char)file.read();
    }
    protocol::Variant var(msg);
    auto fw = var.to<State>();
    if (fw.role == role && fw.hardware == currentFW->hardware) {
      Log(DEBUG, "MD5 found %s\n", fw.md5.c_str());
      currentFW->md5 = fw.md5;
    }
  }

  mesh.onPackage((int)OTA_OP_CODES::ANNOUNCE, [currentFW, updateFW, &mesh,
                                               &scheduler](
                                                  protocol::Variant& variant) {
    // convert variant to Announce
    auto pkg = variant.to<Announce>();
    // Check if we want the update
    if (currentFW->role == pkg.role && currentFW->hardware == pkg.hardware) {
      if ((currentFW->md5 == pkg.md5 && !pkg.forced) ||
          updateFW->md5 == pkg.md5) {
        // Either already have it, or already updating to it
        return false;
      } else {
        updateFW->md5 = pkg.md5;
        updateFW->broadcasted = pkg.broadcasted;
        // If we are not in broadcasted mode, or we are the root node, begin
        // requesting data
        if (!pkg.broadcasted || mesh.isRoot()) {
          auto request =
              DataRequest::replyTo(pkg, mesh.getNodeId(), updateFW->partNo);
          // enable the request task
          updateFW->task =
              mesh.addTask(scheduler, 30 * TASK_SECOND, 10,
                           [request, &mesh]() { mesh.sendPackage(&request); });
          updateFW->task->setOnDisable([updateFW]() {
            Log(ERROR, "OTA: Did not receive the requested data.\n");
            updateFW->md5 = "";
          });
        }
      }
    }
    return false;
  });

  // mesh.onPackage(11, [currentFW](protocol::Variant &variant) {
  //   Log(ERROR, "Data request should not be send to this node\n");
  //   return false;
  // });

  mesh.onPackage((int)OTA_OP_CODES::DATA, [currentFW, updateFW, progress_cb,
                                           &mesh, &scheduler](
                                              protocol::Variant& variant) {
    auto pkg = variant.to<Data>();
    // Check whether it is a new part, of correct md5 role etc etc
    if (updateFW->md5 == pkg.md5 && updateFW->role == pkg.role &&
        updateFW->hardware == pkg.hardware) {
      // Have we have received the next chunk of firmware in sequence?
      if (updateFW->partNo == pkg.partNo) {
        if (progress_cb != NULL) {
          progress_cb(pkg.partNo, pkg.noPart);
        }
        if (pkg.partNo == 0) {
#ifdef ESP32
          uint32_t maxSketchSpace = UPDATE_SIZE_UNKNOWN;
#else
          uint32_t maxSketchSpace =
                  (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
#endif
          Log(DEBUG, "Sketch size %d\n", maxSketchSpace);
          if (Update.isRunning()) {
            Update.end(false);
          }
          if (!Update.begin(maxSketchSpace)) {  // start with max available size
            Log(DEBUG, "handleOTA(): OTA start failed!");
            Update.printError(Serial);
            Update.end();
          } else {
            Update.setMD5(pkg.md5.c_str());
          }
        }

        //    write data
        auto b64Data = base64::decode(pkg.data);
        if (Update.write((uint8_t*)b64Data.c_str(), b64Data.length()) !=
            b64Data.length()) {
          Log(ERROR, "handleOTA(): OTA write failed!");
          Update.printError(Serial);
          Update.end();
          updateFW->md5 = "";
          updateFW->partNo = 0;
          return false;
        }

        // If last part then write ota_fn and reboot
        if (pkg.partNo == pkg.noPart - 1) {
          // check md5, reboot
          if (Update.end(true)) {  // true to set the size to the
                                   // current progress
#ifdef USE_FS_SPIFFS
            auto file = SPIFFS.open(updateFW->ota_fn, "w");
            if (!file) {
              Log(ERROR,
                  "handleOTA(): Unable to write md5 of new update to the "
                  "SPIFFS "
                  "file. This will result in endless update loops for OTA\n");
            }
#else
            auto file = LittleFS.open(updateFW->ota_fn, "w");
            if (!file) {
              Log(ERROR, "handleOTA(): Unable to write md5 of new binary to the LittleFS file. This will result in endless update loops for OTA\n");
            }
#endif

            String msg;
            protocol::Variant var(updateFW.get());
            var.printTo(msg);
            file.print(msg);
            file.close();

            Log(DEBUG, "handleOTA(): OTA Success! %s, %s\n", msg.c_str(),
                updateFW->role.c_str());
            // Delay restart by 2 seconds to allow mesh activity to finish
            mesh.addTask(scheduler, 2 * TASK_SECOND, TASK_ONCE,
                         []() { ESP.restart(); })
                ->enableDelayed();
          } else {
            Log(DEBUG, "handleOTA(): OTA failed!\n");
            Update.printError(Serial);
            updateFW->md5 = "";
            updateFW->partNo = 0;
          }
          if (updateFW->task != NULL) {
            updateFW->task->setOnDisable(NULL);
            updateFW->task->disable();
          }
        } else {
          // else request more
          ++updateFW->partNo;
          if (updateFW->task != NULL) {
            auto request = DataRequest::replyTo(pkg, updateFW->partNo);
            updateFW->task->setCallback(
                [request, &mesh]() { mesh.sendPackage(&request); });
            // updateFW->task->disable();
            updateFW->task->restart();
          }
        }
      } else {
        // We are out-of-sequence, resume with non-broadcasted update
        if (updateFW->broadcasted && pkg.broadcasted) {
          Log(DEBUG, "Out of sequence packet! We may have missed a packet?");
          // Drop of out broadcasted mode
          updateFW->broadcasted = false;
          auto request = DataRequest::replyTo(pkg, updateFW->partNo);
          request.broadcasted = false;
          updateFW->task =
              mesh.addTask(scheduler, 30 * TASK_SECOND, 10,
                           [request, &mesh]() { mesh.sendPackage(&request); });
          updateFW->task->setOnDisable([updateFW]() {
            Log(ERROR, "OTA: Did not receive the requested data.\n");
            updateFW->md5 = "";
          });
        }
      }
    }
    return false;
  });

#endif
}

}  // namespace ota
}  // namespace plugin
}  // namespace painlessmesh

#endif
