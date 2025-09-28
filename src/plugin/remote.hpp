#pragma once

#include "painlessmesh/configuration.hpp"

#include "painlessmesh/logger.hpp"
#include "painlessmesh/plugin.hpp"

extern painlessmesh::logger::LogClass Log;

namespace painlessmesh {
namespace plugin {
/** Add remote logger to the mesh
 *
 * Call remote::begin(nodeId, millis) to start it
 */
namespace remote {
class RemotePackage : public plugin::SinglePackage {
 public:
  std::list<std::pair<uint, TSTRING>> log;
  RemotePackage() : plugin::SinglePackage(14) {}
  RemotePackage(JsonObject jsonObj) : plugin::SinglePackage(jsonObj) {
    for (JsonObject doc : jsonObj["log"].as<JsonArray>()) {
      std::pair<uint, TSTRING> pr(doc["first"], doc["second"]);
      log.push_back(pr);
    }
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = SinglePackage::addTo(std::move(jsonObj));
    JsonArray arr = jsonObj["log"].to<JsonArray>();
    for (auto&& pr : log) {
      JsonDocument obj;
      obj["first"] = pr.first;
      obj["second"] = pr.second;
      arr.add(obj);
    }
    return jsonObj;
  }
};

template <class T>
void begin(T& mesh, uint32_t destination, double frequency = 120,
           std::function<bool(protocol::Variant&)> callback = nullptr) {
  auto sendPkg = std::make_shared<RemotePackage>();
  sendPkg->from = mesh.getNodeId();
  sendPkg->dest = destination;
  mesh.addTask(frequency * TASK_SECOND, TASK_FOREVER, [sendPkg, &mesh]() {
    Log.remote("Memory %u and stability %u\n", ESP.getFreeHeap(),
               mesh.stability);
    // Note that we could clear the queue afterwards, but we
    // actually want to keep resending it in case something just
    // went wrong and we lost connection
    sendPkg->log = Log.get_remote_queue();
    if (!sendPkg->log.empty()) mesh.sendPackage(sendPkg.get());
  });

  if (callback) {
    mesh.onPackage(sendPkg->type, callback);
  }
}

}  // namespace remote
}  // namespace plugin
}  // namespace painlessmesh
