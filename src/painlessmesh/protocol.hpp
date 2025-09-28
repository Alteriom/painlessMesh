#ifndef _PAINLESS_MESH_PROTOCOL_HPP_
#define _PAINLESS_MESH_PROTOCOL_HPP_

#include <cmath>
#include <list>

#include "Arduino.h"

#if ARDUINOJSON_VERSION_MAJOR == 7
#include "ArduinoJson/Deserialization/DeserializationError.hpp"
#include "ArduinoJson/Document/JsonDocument.hpp"
#endif
#include "painlessmesh/configuration.hpp"

namespace painlessmesh {

namespace router {

/** Different ways to route packages
 *
 * NEIGHBOUR packages are send to the neighbour and will be immediately
 * handled there. The TIME_SYNC and NODE_SYNC packages are NEIGHBOUR. SINGLE
 * messages are meant for a specific node. When another node receives this
 * message, it will look in its routing information and send it on to the
 * correct node, without processing the message in any other way. Only the
 * targetted node will actually parse/handle this message (without sending it
 * on). Finally, BROADCAST message are send to every node and
 * processed/handled by every node.
 * */
enum Type { ROUTING_ERROR = -1, NEIGHBOUR, SINGLE, BROADCAST };
}  // namespace router

namespace protocol {

enum Type {
  TIME_DELAY = 3,
  TIME_SYNC = 4,
  NODE_SYNC_REQUEST = 5,
  NODE_SYNC_REPLY = 6,
  CONTROL = 7,    // deprecated
  BROADCAST = 8,  // application data for everyone
  SINGLE = 9      // application data for a single node
};

enum TimeType {
  TIME_SYNC_ERROR = -1,
  TIME_SYNC_REQUEST,
  TIME_REQUEST,
  TIME_REPLY
};

class PackageInterface {
 public:
  virtual JsonObject addTo(JsonObject&& jsonObj) const = 0;
#if ARDUINOJSON_VERSION_MAJOR < 7
  virtual size_t jsonObjectSize() const = 0;
#endif
};

/**
 * Single package
 *
 * Message send to a specific node
 */
class Single : public PackageInterface {
 public:
  int type = SINGLE;
  uint32_t from;
  uint32_t dest;
  TSTRING msg = "";

  Single() {}
  Single(uint32_t fromID, uint32_t destID, TSTRING& message) {
    from = fromID;
    dest = destID;
    msg = message;
  }

  Single(JsonObject jsonObj) {
    dest = jsonObj["dest"].as<uint32_t>();
    from = jsonObj["from"].as<uint32_t>();
    msg = jsonObj["msg"].as<TSTRING>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["type"] = type;
    jsonObj["dest"] = dest;
    jsonObj["from"] = from;
    jsonObj["msg"] = msg;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(4) + ceil(1.1 * msg.length());
  }
#endif
};

/**
 * Broadcast package
 */
class Broadcast : public Single {
 public:
  int type = BROADCAST;

  using Single::Single;

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = Single::addTo(std::move(jsonObj));
    jsonObj["type"] = type;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(4) + ceil(1.1 * msg.length());
  }
#endif
};

class NodeTree : public PackageInterface {
 public:
  uint32_t nodeId = 0;
  bool root = false;
  std::list<NodeTree> subs;

  NodeTree() {}

  NodeTree(uint32_t nodeID, bool iAmRoot) {
    nodeId = nodeID;
    root = iAmRoot;
  }

  NodeTree(JsonObject jsonObj) {
#if ARDUINOJSON_VERSION_MAJOR < 7
    if (jsonObj.containsKey("root")) root = jsonObj["root"].as<bool>();
    if (jsonObj.containsKey("nodeId"))
#else

    if (jsonObj["root"].is<bool>()) root = jsonObj["root"].as<bool>();
    if (jsonObj["nodeId"].is<uint32_t>())
#endif
      nodeId = jsonObj["nodeId"].as<uint32_t>();
    else
      nodeId = jsonObj["from"].as<uint32_t>();

#if ARDUINOJSON_VERSION_MAJOR < 7
    if (jsonObj.containsKey("subs")) {
#else
    if (jsonObj["subs"].is<JsonArray>()) {
#endif
      auto jsonArr = jsonObj["subs"].as<JsonArray>();
      for (size_t i = 0; i < jsonArr.size(); ++i) {
        subs.push_back(NodeTree(jsonArr[i].as<JsonObject>()));
      }
    }
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["nodeId"] = nodeId;
    if (root) jsonObj["root"] = root;
    if (subs.size() > 0) {
#if ARDUINOJSON_VERSION_MAJOR == 7
      JsonArray subsArr = jsonObj["subs"].to<JsonArray>();
#else
      JsonArray subsArr = jsonObj.createNestedArray("subs");
#endif
      for (auto&& s : subs) {
#if ARDUINOJSON_VERSION_MAJOR == 7
        JsonObject subObj = subsArr.add<JsonObject>();
#else
        JsonObject subObj = subsArr.createNestedObject();
#endif
        subObj = s.addTo(std::move(subObj));
      }
    }
    return jsonObj;
  }

  bool operator==(const NodeTree& b) const {
    if (!(this->nodeId == b.nodeId && this->root == b.root &&
          this->subs.size() == b.subs.size()))
      return false;
    auto itA = this->subs.begin();
    auto itB = b.subs.begin();
    for (size_t i = 0; i < this->subs.size(); ++i) {
      if ((*itA) != (*itB)) {
        return false;
      }
      ++itA;
      ++itB;
    }
    return true;
  }

  bool operator!=(const NodeTree& b) const { return !this->operator==(b); }

  TSTRING toString(bool pretty = false);

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    size_t base = 1;
    if (root) ++base;
    if (subs.size() > 0) ++base;
    size_t size = JSON_OBJECT_SIZE(base);
    if (subs.size() > 0) size += JSON_ARRAY_SIZE(subs.size());
    for (auto&& s : subs) size += s.jsonObjectSize();
    return size;
  }
#endif

  void clear() {
    nodeId = 0;
    subs.clear();
    root = false;
  }
};

/**
 * NodeSyncRequest package
 */
class NodeSyncRequest : public NodeTree {
 public:
  int type = NODE_SYNC_REQUEST;
  uint32_t from;
  uint32_t dest;

  NodeSyncRequest() {}
  NodeSyncRequest(uint32_t fromID, uint32_t destID, std::list<NodeTree> subTree,
                  bool iAmRoot = false) {
    from = fromID;
    dest = destID;
    subs = subTree;
    nodeId = fromID;
    root = iAmRoot;
  }

  NodeSyncRequest(JsonObject jsonObj) : NodeTree(jsonObj) {
    dest = jsonObj["dest"].as<uint32_t>();
    from = jsonObj["from"].as<uint32_t>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = NodeTree::addTo(std::move(jsonObj));
    jsonObj["type"] = type;
    jsonObj["dest"] = dest;
    jsonObj["from"] = from;
    return jsonObj;
  }

  bool operator==(const NodeSyncRequest& b) const {
    if (!(this->from == b.from && this->dest == b.dest)) return false;
    return NodeTree::operator==(b);
  }

  bool operator!=(const NodeSyncRequest& b) const {
    return !this->operator==(b);
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    size_t base = 4;
    if (root) ++base;
    if (subs.size() > 0) ++base;
    size_t size = JSON_OBJECT_SIZE(base);
    if (subs.size() > 0) size += JSON_ARRAY_SIZE(subs.size());
    for (auto&& s : subs) size += s.jsonObjectSize();
    return size;
  }
#endif
};

/**
 * NodeSyncReply package
 */
class NodeSyncReply : public NodeSyncRequest {
 public:
  int type = NODE_SYNC_REPLY;

  using NodeSyncRequest::NodeSyncRequest;

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = NodeSyncRequest::addTo(std::move(jsonObj));
    jsonObj["type"] = type;
    return jsonObj;
  }
};

struct time_sync_msg_t {
  int type = TIME_SYNC_ERROR;
  uint32_t t0 = 0;
  uint32_t t1 = 0;
  uint32_t t2 = 0;
};

/**
 * TimeSync package
 */
class TimeSync : public PackageInterface {
 public:
  int type = TIME_SYNC;
  uint32_t dest;
  uint32_t from;
  time_sync_msg_t msg;

  TimeSync() {}

  TimeSync(uint32_t fromID, uint32_t destID) {
    from = fromID;
    dest = destID;
    msg.type = TIME_SYNC_REQUEST;
  }

  TimeSync(uint32_t fromID, uint32_t destID, uint32_t t0) {
    from = fromID;
    dest = destID;
    msg.type = TIME_REQUEST;
    msg.t0 = t0;
  }

  TimeSync(uint32_t fromID, uint32_t destID, uint32_t t0, uint32_t t1) {
    from = fromID;
    dest = destID;
    msg.type = TIME_REPLY;
    msg.t0 = t0;
    msg.t1 = t1;
  }

  TimeSync(uint32_t fromID, uint32_t destID, uint32_t t0, uint32_t t1,
           uint32_t t2) {
    from = fromID;
    dest = destID;
    msg.type = TIME_REPLY;
    msg.t0 = t0;
    msg.t1 = t1;
    msg.t2 = t2;
  }

  TimeSync(JsonObject jsonObj) {
    dest = jsonObj["dest"].as<uint32_t>();
    from = jsonObj["from"].as<uint32_t>();
    msg.type = jsonObj["msg"]["type"].as<int>();
#if ARDUINOJSON_VERSION_MAJOR < 7
    if (jsonObj["msg"].containsKey("t0"))
      msg.t0 = jsonObj["msg"]["t0"].as<uint32_t>();
    if (jsonObj["msg"].containsKey("t1"))
      msg.t1 = jsonObj["msg"]["t1"].as<uint32_t>();
    if (jsonObj["msg"].containsKey("t2"))
      msg.t2 = jsonObj["msg"]["t2"].as<uint32_t>();
#else
    if (jsonObj["msg"]["t0"].is<uint32_t>())
      msg.t0 = jsonObj["msg"]["t0"].as<uint32_t>();
    if (jsonObj["msg"]["t1"].is<uint32_t>())
      msg.t1 = jsonObj["msg"]["t1"].as<uint32_t>();
    if (jsonObj["msg"]["t2"].is<uint32_t>())
      msg.t2 = jsonObj["msg"]["t2"].as<uint32_t>();
#endif
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["type"] = type;
    jsonObj["dest"] = dest;
    jsonObj["from"] = from;
#if ARDUINOJSON_VERSION_MAJOR == 7
    auto msgObj = jsonObj["msg"].to<JsonObject>();
#else
    auto msgObj = jsonObj.createNestedObject("msg");
#endif
    msgObj["type"] = msg.type;
    if (msg.type >= 1) msgObj["t0"] = msg.t0;
    if (msg.type >= 2) {
      msgObj["t1"] = msg.t1;
      msgObj["t2"] = msg.t2;
    }
    return jsonObj;
  }

  /**
   * Create a reply to the current message with the new time set
   */
  void reply(uint newT0) {
    msg.t0 = newT0;
    ++msg.type;
    std::swap(from, dest);
  }

  /**
   * Create a reply to the current message with the new time set
   */
  void reply(uint32_t newT1, uint32_t newT2) {
    msg.t1 = newT1;
    msg.t2 = newT2;
    ++msg.type;
    std::swap(from, dest);
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(4);
  }
#endif
};

/**
 * TimeDelay package
 */
class TimeDelay : public TimeSync {
 public:
  int type = TIME_DELAY;
  using TimeSync::TimeSync;

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = TimeSync::addTo(std::move(jsonObj));
    jsonObj["type"] = type;
    return jsonObj;
  }
};

/**
 * Can store any package variant
 *
 * Internally stores packages as a JsonObject. Main use case is to convert
 * different packages from and to Json (using ArduinoJson).
 */
class Variant {
  // TODO: Consider disabling copy constructor/assignment, since it seems to
  // result in problems with arduinojson (see issue #521)
 public:
  // TODO: Consider disabling copy and assignment constructor. It seems to
  // sometimes lead to crashes. See issue 521

  // Copy constructor
  Variant(const Variant& other)
      : error(other.error), jsonBuffer(other.jsonBuffer) {
    if (error == DeserializationError::Ok)
      jsonObj = jsonBuffer.as<JsonObject>();
  }

  Variant& operator=(const Variant& other) {
    if (this != &other) {
      error = other.error;
      jsonBuffer = other.jsonBuffer;
      if (error == DeserializationError::Ok)
        jsonObj = jsonBuffer.as<JsonObject>();
    }
    return *this;
  }
#ifdef ARDUINOJSON_ENABLE_STD_STRING
  /**
   * Create Variant object from a json string
   *
   * @param json The json string containing a package
   */
  Variant(const std::string& json)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(4) +
                   2 * json.length()) {
#endif
    error = deserializeJson(jsonBuffer, json,
                            DeserializationOption::NestingLimit(255));
    if (error == DeserializationError::Ok)
      jsonObj = jsonBuffer.as<JsonObject>();
  }

  /**
   * Create Variant object from a json string
   *
   * @param json The json string containing a package
   * @param capacity The capacity to reserve for parsing the string
   */
  Variant(const std::string& json, size_t capacity)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(capacity) {
#endif
    error = deserializeJson(jsonBuffer, json,
                            DeserializationOption::NestingLimit(255));
    if (error == DeserializationError::Ok)
      jsonObj = jsonBuffer.as<JsonObject>();
  }
#endif

#ifdef ARDUINOJSON_ENABLE_ARDUINO_STRING
  /**
   * Create Variant object from a json string
   *
   * @param json The json string containing a package
   */
  Variant(String json)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(4) +
                   2 * json.length()) {
#endif
    error = deserializeJson(jsonBuffer, json,
                            DeserializationOption::NestingLimit(255));

    if (error == DeserializationError::Ok)
      jsonObj = jsonBuffer.as<JsonObject>();
  }

  /**
   * Create Variant object from a json string
   *
   * @param json The json string containing a package
   * @param capacity The capacity to reserve for parsing the string
   */
  Variant(String json, size_t capacity)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(capacity) {
#endif
    error = deserializeJson(jsonBuffer, json,
                            DeserializationOption::NestingLimit(255));
    if (!error) jsonObj = jsonBuffer.as<JsonObject>();
  }
#endif
  /**
   * Create Variant object from any package implementing PackageInterface
   */
  Variant(const PackageInterface* pkg)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(pkg->jsonObjectSize()) {
#endif
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = pkg->addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a Single package
   *
   * @param single The single package
   */
  Variant(Single single)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(single.jsonObjectSize()) {
#endif
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = single.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a Broadcast package
   *
   * @param broadcast The broadcast package
   */
  Variant(Broadcast broadcast)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(broadcast.jsonObjectSize()) {
#endif
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = broadcast.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a NodeTree
   *
   * @param nodeTree The NodeTree
   */
  Variant(NodeTree nodeTree)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(nodeTree.jsonObjectSize()) {
#endif
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = nodeTree.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a NodeSyncReply package
   *
   * @param nodeSyncReply The nodeSyncReply package
   */
  Variant(NodeSyncReply nodeSyncReply)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(nodeSyncReply.jsonObjectSize()) {
#endif
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = nodeSyncReply.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a NodeSyncRequest package
   *
   * @param nodeSyncRequest The nodeSyncRequest package
   */
  Variant(NodeSyncRequest nodeSyncRequest)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(nodeSyncRequest.jsonObjectSize()) {
#endif
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = nodeSyncRequest.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a TimeSync package
   *
   * @param timeSync The timeSync package
   */
  Variant(TimeSync timeSync)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(timeSync.jsonObjectSize()) {
#endif
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = timeSync.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a TimeDelay package
   *
   * @param timeDelay The timeDelay package
   */
  Variant(TimeDelay timeDelay)
#if ARDUINOJSON_VERSION_MAJOR == 7
      : jsonBuffer() {
#else
      : jsonBuffer(timeDelay.jsonObjectSize()) {
#endif
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = timeDelay.addTo(std::move(jsonObj));
  }

  /**
   * Whether this package is of the given type
   */
  template <typename T>
  inline bool is() {
    return false;
  }

  /**
   * Convert Variant to the given type
   */
  template <typename T>
  inline T to() {
    return T(jsonObj);
  }

  /**
   * Return package type
   */
  int type() { return jsonObj["type"].as<int>(); }

  /**
   * Package routing method
   */
  router::Type routing() {
#if ARDUINOJSON_VERSION_MAJOR < 7
    if (jsonObj.containsKey("routing"))
#else
    if (jsonObj["routing"].is<int>())
#endif
      return (router::Type)jsonObj["routing"].as<int>();

    auto type = this->type();
    if (type == SINGLE || type == TIME_DELAY) return router::SINGLE;
    if (type == BROADCAST) return router::BROADCAST;
    if (type == NODE_SYNC_REQUEST || type == NODE_SYNC_REPLY ||
        type == TIME_SYNC)
      return router::NEIGHBOUR;
    return router::ROUTING_ERROR;
  }

  /**
   * Destination node of the package
   */
  uint32_t dest() {
#if ARDUINOJSON_VERSION_MAJOR < 7
    if (jsonObj.containsKey("dest")) return jsonObj["dest"].as<uint32_t>();
#else
    if (jsonObj["dest"].is<uint32_t>()) return jsonObj["dest"].as<uint32_t>();
#endif
    return 0;
  }

#ifdef ARDUINOJSON_ENABLE_STD_STRING
  /**
   * Print a variant to a string
   *
   * @return A json representation of the string
   */
  void printTo(std::string& str, bool pretty = false) {
    if (pretty)
      serializeJsonPretty(jsonObj, str);
    else
      serializeJson(jsonObj, str);
  }
#endif

#ifdef ARDUINOJSON_ENABLE_ARDUINO_STRING
  /**
   * Print a variant to a string
   *
   * @return A json representation of the string
   */
  void printTo(String& str, bool pretty = false) {
    if (pretty)
      serializeJsonPretty(jsonObj, str);
    else
      serializeJson(jsonObj, str);
  }
#endif

  DeserializationError error = DeserializationError::Ok;

 private:
#if ARDUINOJSON_VERSION_MAJOR == 7
  JsonDocument jsonBuffer;
#else
  DynamicJsonDocument jsonBuffer;
#endif
  JsonObject jsonObj;
};

template <>
inline bool Variant::is<Single>() {
  return jsonObj["type"].as<int>() == SINGLE;
}

template <>
inline bool Variant::is<Broadcast>() {
  return jsonObj["type"].as<int>() == BROADCAST;
}

template <>
inline bool Variant::is<NodeSyncReply>() {
  return jsonObj["type"].as<int>() == NODE_SYNC_REPLY;
}

template <>
inline bool Variant::is<NodeSyncRequest>() {
  return jsonObj["type"].as<int>() == NODE_SYNC_REQUEST;
}

template <>
inline bool Variant::is<TimeSync>() {
  return jsonObj["type"].as<int>() == TIME_SYNC;
}

template <>
inline bool Variant::is<TimeDelay>() {
  return jsonObj["type"].as<int>() == TIME_DELAY;
}

template <>
inline JsonObject Variant::to<JsonObject>() {
  return jsonObj;
}

inline TSTRING NodeTree::toString(bool pretty) {
  TSTRING str;
  auto variant = Variant(*this);
  variant.printTo(str, pretty);
  return str;
}

}  // namespace protocol
}  // namespace painlessmesh
#endif
