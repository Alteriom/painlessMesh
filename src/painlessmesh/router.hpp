#ifndef _PAINLESS_MESH_ROUTER_HPP_
#define _PAINLESS_MESH_ROUTER_HPP_

#include <algorithm>
#include <memory>

#include "painlessmesh/callback.hpp"
#include "painlessmesh/layout.hpp"
#include "painlessmesh/logger.hpp"
#include "painlessmesh/protocol.hpp"

extern painlessmesh::logger::LogClass Log;

namespace painlessmesh {

/**
 * Helper functions to route messages
 */
namespace router {
// Layouts are taken by const reference throughout this header: Layout<T>::subs
// is a std::list of shared_ptrs, so passing by value used to copy the whole
// connection list (one heap allocation per connection) on every packet sent,
// broadcast or forwarded (issue #387).
template <class T>
std::shared_ptr<T> findRoute(const layout::Layout<T>& tree,
                             std::function<bool(std::shared_ptr<T>)> func) {
  auto route = std::find_if(tree.subs.begin(), tree.subs.end(), func);
  if (route == tree.subs.end()) return NULL;
  return (*route);
}

template <class T>
std::shared_ptr<T> findRoute(const layout::Layout<T>& tree, uint32_t nodeId) {
  return findRoute<T>(tree, [nodeId](std::shared_ptr<T> s) {
    return layout::contains((*s), nodeId);
  });
}

/** A route to nodeId that can actually carry something.
 *
 * findRoute() searches every sub, and a closed connection stays in subs
 * until eraseClosedConnections() next runs, so it will happily answer with
 * a link that is already gone. Callers deciding whether a node is
 * *reachable* — rather than which sub to hand a packet to — need the
 * distinction: acting on a dead route to refuse a live connection leaves
 * the node with neither.
 *
 * `exclude` drops the connection being judged, which cannot duplicate
 * itself. Mirrors the liveness test layout::syncLayout already applies.
 */
template <class T>
std::shared_ptr<T> findLiveRoute(const layout::Layout<T>& tree, uint32_t nodeId,
                                 std::shared_ptr<T> exclude = nullptr) {
  return findRoute<T>(tree, [nodeId, exclude](std::shared_ptr<T> s) {
    return s != exclude && s->connected() && layout::contains((*s), nodeId);
  });
}

template <class T, class U>
bool send(T& package, std::shared_ptr<U> conn, bool priority = false) {
  painlessmesh::protocol::Variant variant(package);
  TSTRING msg;
  variant.printTo(msg);
  return conn->addMessage(msg, priority);
}

template <class T, class U>
bool send(T&& package, std::shared_ptr<U> conn, bool priority = false) {
  painlessmesh::protocol::Variant variant(package);
  TSTRING msg;
  variant.printTo(msg);
  return conn->addMessage(msg, priority);
}

template <class U>
bool send(protocol::Variant& variant, std::shared_ptr<U> conn,
          bool priority = false) {
  TSTRING msg;
  variant.printTo(msg);
  return conn->addMessage(msg, priority);
}

template <class U>
bool send(protocol::Variant&& variant, std::shared_ptr<U> conn,
          bool priority = false) {
  TSTRING msg;
  variant.printTo(msg);
  return conn->addMessage(msg, priority);
}

// New priority-level send functions (0-3 priority levels)
template <class T, class U>
bool sendWithPriority(T& package, std::shared_ptr<U> conn, uint8_t priorityLevel) {
  painlessmesh::protocol::Variant variant(&package);
  TSTRING msg;
  variant.printTo(msg);
  return conn->addMessageWithPriority(msg, priorityLevel);
}

template <class T, class U>
bool sendWithPriority(T&& package, std::shared_ptr<U> conn, uint8_t priorityLevel) {
  painlessmesh::protocol::Variant variant(&package);
  TSTRING msg;
  variant.printTo(msg);
  return conn->addMessageWithPriority(msg, priorityLevel);
}

template <class U>
bool sendWithPriority(protocol::Variant& variant, std::shared_ptr<U> conn, uint8_t priorityLevel) {
  TSTRING msg;
  variant.printTo(msg);
  return conn->addMessageWithPriority(msg, priorityLevel);
}

template <class U>
bool sendWithPriority(protocol::Variant&& variant, std::shared_ptr<U> conn, uint8_t priorityLevel) {
  TSTRING msg;
  variant.printTo(msg);
  return conn->addMessageWithPriority(msg, priorityLevel);
}

// The layout-level send and broadcast functions below all funnel into the
// protocol::Variant& core overloads, which enqueue at the priority carried in
// the package's "prio" field (PRIORITY_NORMAL when absent). This is what keeps
// a sender's priority attached to a package across intermediate hops instead
// of silently dropping it to NORMAL after the first hop (issue #384): the
// forwarding path in routePackage() re-reads the field from the wire.

template <class U>
bool send(protocol::Variant& variant, const layout::Layout<U>& layout) {
  TSTRING msg;
  variant.printTo(msg);
  auto conn = findRoute<U>(layout, variant.dest());
  if (conn) return conn->addMessageWithPriority(msg, variant.priority());
  return false;
}

template <class T, class U>
bool send(T& package, const layout::Layout<U>& layout) {
  painlessmesh::protocol::Variant variant(package);
  return send<U>(variant, layout);
}

template <class T, class U>
bool send(T&& package, const layout::Layout<U>& layout) {
  painlessmesh::protocol::Variant variant(package);
  return send<U>(variant, layout);
}

template <class U>
bool send(protocol::Variant&& variant, const layout::Layout<U>& layout) {
  return send<U>(variant, layout);
}

template <class T>
size_t broadcast(protocol::Variant& variant, const layout::Layout<T>& layout,
                 uint32_t exclude) {
  TSTRING msg;
  variant.printTo(msg);
  const auto priority = variant.priority();
  size_t i = 0;
  for (auto&& conn : layout.subs) {
    if (conn->nodeId != 0 && conn->nodeId != exclude) {
      auto sent = conn->addMessageWithPriority(msg, priority);
      if (sent) ++i;
    }
  }
  return i;
}

template <class T, class U>
size_t broadcast(T& package, const layout::Layout<U>& layout,
                 uint32_t exclude) {
  painlessmesh::protocol::Variant variant(package);
  return broadcast<U>(variant, layout, exclude);
}

template <class T, class U>
size_t broadcast(T&& package, const layout::Layout<U>& layout,
                 uint32_t exclude) {
  painlessmesh::protocol::Variant variant(package);
  return broadcast<U>(variant, layout, exclude);
}

template <class T>
size_t broadcast(protocol::Variant&& variant, const layout::Layout<T>& layout,
                 uint32_t exclude) {
  return broadcast<T>(variant, layout, exclude);
}

template <class T>
void routePackage(const layout::Layout<T>& layout,
                  std::shared_ptr<T> connection, const TSTRING& pkg,
                  callback::MeshPackageCallbackList<T>& cbl,
                  uint32_t receivedAt) {
  using namespace logger;
  Log(COMMUNICATION, "routePackage(): Recvd from %u: %s\n", connection->nodeId,
      pkg.c_str());
#if ARDUINOJSON_VERSION_MAJOR == 7
  protocol::Variant variant(pkg);
  if (variant.error) {
    Log(ERROR,
        "routePackage(): parsing failed. err=%u, total_length=%d, data=%s<--\n",
        variant.error, pkg.length(), pkg.c_str());
    return;
  }

  if (variant.routing() == SINGLE && variant.dest() != layout.getNodeId()) {
    // Send on without further processing
    send<T>(variant, layout);
    return;
  } else if (variant.routing() == BROADCAST) {
    broadcast<T>(variant, layout, connection->nodeId);
  }
  auto calls = cbl.execute(variant.type(), variant, connection, receivedAt);
  if (calls == 0)
    Log(DEBUG, "routePackage(): No callbacks executed; %u, %s\n",
        variant.type(), pkg.c_str());
#else
  // Calculate required capacity based on message size and nesting depth
  // Fixed capacity approach to avoid segmentation fault issues with
  // dynamic reallocation (see issue #521 and CODE_REFACTORING_RECOMMENDATIONS.md)
  size_t nestingDepth = std::count(pkg.begin(), pkg.end(), '{') + 
                        std::count(pkg.begin(), pkg.end(), '[');
  
#if ARDUINOJSON_VERSION_MAJOR >= 7
  // ArduinoJson v7: automatic capacity management, use generous buffer
  size_t calculatedCapacity = pkg.length() + 1024;
#else
  // ArduinoJson v6: manual capacity calculation required
  // Base capacity: message length + overhead for JSON structure
  // Each nesting level adds overhead for pointers and metadata
  size_t calculatedCapacity = pkg.length() + 
                              JSON_OBJECT_SIZE(10) * (std::max)(nestingDepth, size_t(1)) + 
                              512;  // Additional buffer for strings and padding
#endif
  
  // Cap at 8KB for safety on ESP8266 (which has ~80KB total heap)
  // Messages larger than this should be rejected
  constexpr size_t MAX_MESSAGE_CAPACITY = 8192;
  size_t capacity = (std::min)(calculatedCapacity, MAX_MESSAGE_CAPACITY);
  
  auto variant = std::make_shared<protocol::Variant>(pkg, capacity);
  
  if (variant->error) {
    if (variant->error == DeserializationError::NoMemory) {
      Log(ERROR,
          "routePackage(): Message too large. length=%d, calculated_capacity=%u, "
          "nesting_depth=%u. Consider increasing MAX_MESSAGE_CAPACITY if needed.\n",
          pkg.length(), calculatedCapacity, nestingDepth);
    } else {
      Log(ERROR,
          "routePackage(): parsing failed. err=%u, length=%d, data=%s<--\n",
          variant->error, pkg.length(), pkg.c_str());
    }
    return;
  }

  if (variant->routing() == SINGLE && variant->dest() != layout.getNodeId()) {
    // Send on without further processing
    send<T>((*variant), layout);
    return;
  } else if (variant->routing() == BROADCAST) {
    broadcast<T>((*variant), layout, connection->nodeId);
  }
  auto calls = cbl.execute(variant->type(), (*variant), connection, receivedAt);
  if (calls == 0)
    Log(DEBUG, "routePackage(): No callbacks executed; %u, %s\n",
        variant->type(), pkg.c_str());
#endif
}

template <class T, class U>
void handleNodeSync(T& mesh, protocol::NodeTree newTree,
                    std::shared_ptr<U> conn) {
  Log(logger::SYNC, "handleNodeSync(): with %u\n", conn->nodeId);

  if (!conn->validSubs(newTree)) {
    Log(logger::SYNC, "handleNodeSync(): invalid new connection\n");
    Log.remote("Invalid connection to %u\n", conn->nodeId);
    conn->close();
    return;
  }

  if (conn->newConnection) {
    // Only a *live* route may refuse this one. eraseClosedConnections()
    // runs later, so a link that has already dropped is still in subs and
    // still answers findRoute() — and refusing a working direct connection
    // on its authority leaves the node with no route at all once the dead
    // one is finally erased. Measured on hardware: a bridge turned away a
    // node twice as "already connected", then finished the run with that
    // node missing from its tree entirely.
    auto oldConnection = router::findLiveRoute<U>(mesh, newTree.nodeId, conn);
    if (oldConnection) {
      Log(logger::SYNC,
          "handleNodeSync(): already connected to %u. Closing the new "
          "connection \n",
          newTree.nodeId);
      Log.remote("Already connected to %u\n", newTree.nodeId);
      conn->close();
      return;
    }
    auto remoteNodeId = newTree.nodeId;
    mesh.addTask([&mesh, remoteNodeId]() {
      Log(logger::CONNECTION, "newConnectionTask():\n");
      Log(logger::CONNECTION, "newConnectionTask(): adding %u now= %u\n",
          remoteNodeId, mesh.getNodeTime());
      mesh.newConnectionCallbacks.execute(remoteNodeId);
    });

    // Initially interval is every 10 seconds,
    // this will slow down to TIME_SYNC_INTERVAL
    // after first succesfull sync
    // TODO move it to a new connection callback and use initTimeSync from
    // ntp.hpp
    conn->timeSyncTask.set(10 * TASK_SECOND, TASK_FOREVER, [conn, &mesh]() {
      Log(logger::S_TIME, "timeSyncTask(): %u\n", conn->nodeId);
      mesh.startTimeSync(conn);
    });
    mesh.mScheduler->addTask(conn->timeSyncTask);
    if (conn->station)
      // We are STA, request time immediately
      conn->timeSyncTask.enable();
    else
      // We are the AP, give STA the change to initiate time sync
      conn->timeSyncTask.enableDelayed();
    conn->newConnection = false;
  }

  if (conn->updateSubs(newTree)) {
    auto nodeId = newTree.nodeId;
    mesh.addTask(
        [&mesh, nodeId]() { mesh.changedConnectionCallbacks.execute(nodeId); });
  } else {
    conn->nodeSyncTask.delay();
    mesh.stability += (std::min)(1000 - mesh.stability, (size_t)25);
  }
}

template <class T, typename U>
void addPackageCallback(callback::MeshPackageCallbackList<U>& callbackList,
                        T& mesh) {
  // REQUEST type,
  callbackList.onPackage(
      protocol::NODE_SYNC_REQUEST,
      [&mesh](protocol::Variant& variant, std::shared_ptr<U> connection,
              uint32_t receivedAt) {
        auto newTree = variant.to<protocol::NodeSyncRequest>();
        handleNodeSync<T, U>(mesh, newTree, connection);
        send<protocol::NodeSyncReply>(
            connection->reply(std::move(mesh.asNodeTree())), connection, true);
        return false;
      });

  // Reply type just handle it
  callbackList.onPackage(
      protocol::NODE_SYNC_REPLY,
      [&mesh](protocol::Variant& variant, std::shared_ptr<U> connection,
              uint32_t receivedAt) {
        auto newTree = variant.to<protocol::NodeSyncReply>();
        handleNodeSync<T, U>(mesh, newTree, connection);
        connection->timeOutTask.disable();
        return false;
      });
}

}  // namespace router
}  // namespace painlessmesh

#endif
