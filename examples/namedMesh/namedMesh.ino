//************************************************************
// this is a simple example that uses the painlessMesh library
//
// This example shows how to build a mesh with named nodes using the plugin
// system
//
//
//************************************************************
#include <cstdint>
#include <map>
#include "Arduino.h"
#include "painlessMesh.h"
#include "painlessmesh/plugin.hpp"

#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

painlessMesh mesh;

// NOTE: various ideas for improvement
// - Send new names regularly and only send all names sometimes
// - Keep track of the names the neighbour already knows (when we receive them)
//   and only send unknown names/nodes
// - Use some kind of bidirectoral map, so both get_name_from_id and
//   get_id_from_name are fast

// Send the names you know to your neighbours
class NamePackage : public painlessmesh::plugin::NeighbourPackage {
 public:
  std::map<TSTRING, uint32_t> names;

  // Set an id for the package. Id can be any unique int not already in use. The
  // mesh uses the numbers 1..12, so using a number larger than that
  NamePackage() : painlessmesh::plugin::NeighbourPackage(21) {}

  NamePackage(JsonObject jsonObj)
      : painlessmesh::plugin::NeighbourPackage(jsonObj) {
    for (JsonPair kv : jsonObj) {
      names.insert(std::pair<TSTRING, uint32_t>(kv.key().c_str(), kv.value()));
    }
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = painlessmesh::plugin::NeighbourPackage::addTo(std::move(jsonObj));
    for (auto&& kv : names) {
      jsonObj[kv.first] = kv.second;
    }
    return jsonObj;
  }
};

std::map<TSTRING, uint32_t> known_names;
TSTRING get_name_from_id(uint32_t id) {
  for (auto&& kv : known_names) {
    if (kv.second == id) {
      return kv.first;
    }
  }
  return String(id);
}

uint32_t get_id_from_name(TSTRING name) {
  auto it = known_names.find(name);
  return (it != known_names.end()) ? it->second : 0;
}

void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP);

  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);

  auto sendPkg = std::make_shared<NamePackage>();
  sendPkg->names = known_names;
  // Every minute we send the names we know to our neighbours
  mesh.addTask(TASK_MINUTE, TASK_FOREVER, [sendPkg, &mesh]() {
    sendPkg->names = known_names;
    mesh.sendPackage(sendPkg.get());
  });

  // When we receive a message of the correct type, then insert the names they
  // know into our map of names
  mesh.onPackage(sendPkg->type, [](painlessmesh::protocol::Variant& var) {
    const auto& pkg = var.to<NamePackage>();
    for (auto&& kv : pkg.names) {
      known_names.insert(kv);
    }

    return false;
  });
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}
