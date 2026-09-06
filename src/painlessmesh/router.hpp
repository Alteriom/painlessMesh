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

/** The live connection through which nodeId is reachable, or NULL.
 *
 * A closed connection stays in subs until eraseClosedConnections() next
 * runs. Routing a packet to it is a silent loss: the write is queued into
 * a buffer nothing will ever drain, and the sender is told it succeeded.
 * On the Alteriom HIL rig, correlating every unacknowledged delivery with
 * the receiver's log showed the message had usually never arrived at all —
 * 27 of 33 across three suites — which is this. A dead link is not a
 * route, for any purpose; the liveness test is the one
 * layout::syncLayout() already applies.
 */
template <class T>
std::shared_ptr<T> findRoute(const layout::Layout<T>& tree, uint32_t nodeId) {
  return findRoute<T>(tree, [nodeId](std::shared_ptr<T> s) {
    return s->connected() && layout::contains((*s), nodeId);
  });
}

/** findRoute() for the duplicate-connection check in handleNodeSync().
 *
 * `exclude` drops the connection being judged, which cannot duplicate
 * itself. Refusing a live direct connection on the authority of a dead
 * route left a node with neither once the dead one was erased.
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
    // The loop check is the tree the new node presents: if this node is
    // anywhere in it, the new connection would close a cycle. That is the
    // only thing a second route to the same node can legitimately mean —
    // a station has exactly one uplink, so a node that arrives on a fresh
    // direct connection with a tree that does not contain us has left
    // wherever else we remember it. Refusing it as "already connected" on
    // the authority of that memory used to hold a rebooted node out of
    // the mesh until the neighbour whose tree still carried it timed the
    // old link out: every AP in turn dropped it a second after the
    // association, for 30 to 100 s per reboot, measured on the Alteriom
    // HIL rig on every restart a suite performs.
    if (layout::contains(newTree, mesh.getNodeId())) {
      // This node in the presented tree is a cycle only if the presenter is
      // also reachable from here through some other live connection — the
      // two ends of the loop. Without that route it is the presenter's
      // memory of where this node used to be, held in a branch its owner
      // has not timed out yet: a newly promoted bridge listed the node that
      // came to join it at the place it held before the promotion, and was
      // refused as a loop on every attempt for the whole promotion window.
      // Stale, the mention is dropped before the tree is taken.
      auto otherRoute = router::findLiveRoute<U>(mesh, newTree.nodeId, conn);
      if (otherRoute) {
        Log(logger::SYNC,
            "handleNodeSync(): %u's tree contains this node and %u is already "
            "reachable through %u: a loop. Closing the new connection\n",
            newTree.nodeId, newTree.nodeId, otherRoute->nodeId);
        Log.remote("Loop through %u\n", newTree.nodeId);
        conn->close();
        return;
      }
      Log(logger::SYNC,
          "handleNodeSync(): %u's tree lists this node where it used to be; "
          "stale, not a loop\n",
          newTree.nodeId);
      layout::forget(newTree, mesh.getNodeId());
    }
    // Whatever else still routes to this node is stale. A direct link to
    // it is the one it had before it went away — TCP has not noticed yet —
    // and closes now instead of at its timeout. A route through a
    // neighbour is that neighbour's memory of the node's old place; the
    // node is taken out of it here so packets go down the live link, and
    // the neighbour's next sync brings its own tree up to date.
    auto oldConnection = router::findLiveRoute<U>(mesh, newTree.nodeId, conn);
    if (oldConnection) {
      if (oldConnection->nodeId == newTree.nodeId) {
        Log(logger::SYNC,
            "handleNodeSync(): %u connected again while its old link is "
            "still open; closing the old one\n",
            newTree.nodeId);
        oldConnection->close();
      } else {
        Log(logger::SYNC,
            "handleNodeSync(): %u was reachable through %u; that place is "
            "stale, the direct connection wins\n",
            newTree.nodeId, oldConnection->nodeId);
        layout::forget(*oldConnection, newTree.nodeId);
      }
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

  // What a neighbour presents is news only when it differs from what it
  // presented last time. The cached tree can differ from a restated claim
  // because a fresher neighbour has since taken a node out of it (below),
  // and taking the restatement as news put the node back, marked the
  // connection changed, forced the fresher neighbour's sync, which took it
  // out again: a sync every 30 to 80 ms between the two for the 10 s it
  // took the restating neighbour to time out the dead link behind its
  // claim, on every board that heard both.
  auto fingerprint = layout::fingerprint(newTree);
  bool restated = conn->presented == fingerprint;
  conn->presented = fingerprint;

  // A station has one uplink, so a node on a live direct link of ours is
  // not below any neighbour: a neighbour that lists it there holds the
  // link it had before it came here, and the direct link wins for as long
  // as it lives. (A new direct connection took the stale places out
  // above; this keeps the claimant from putting them back.)
  for (auto&& other : mesh.subs) {
    if (other == conn || other->nodeId == 0 || !other->connected()) continue;
    if (layout::forget(newTree, other->nodeId)) {
      Log(logger::SYNC,
          "handleNodeSync(): %u lists %u, which is directly connected; the "
          "direct link wins\n",
          conn->nodeId, other->nodeId);
    }
  }

  if (restated) {
    conn->nodeSyncTask.delay();
    mesh.stability += (std::min)(1000 - mesh.stability, (size_t)25);
    return;
  }

  // A node is in one place, and a changed sync is the freshest word on
  // every node below conn. Any other neighbour whose cached tree still
  // lists one of them lists it where it used to be: on the rig every board
  // carried a node twice — under the neighbour it had moved to and under
  // the one it had left — and a message routed by the older copy never
  // arrived, nor did the Internet request that went the same way. The
  // older copies go now; their owners' next changed syncs agree.
  for (auto&& other : mesh.subs) {
    if (other == conn || other->nodeId == 0) continue;
    size_t removed = layout::forgetAll(*other, newTree);
    if (removed) {
      Log(logger::SYNC,
          "handleNodeSync(): %u nodes now under %u were still listed under "
          "%u; forgotten there\n",
          (unsigned)removed, conn->nodeId, other->nodeId);
    }
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
