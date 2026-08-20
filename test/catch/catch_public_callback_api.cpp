/**
 * @file catch_public_callback_api.cpp
 * @brief Regression tests for the public callback API on painlessmesh::Mesh.
 *
 * Regression coverage for issue #389: calling `mesh.onReceive()` a second
 * time does not replace the first handler — both handlers fire on every
 * subsequent message. That is intentional at the primitive level
 * (`callback::PackageCallbackList`, covered in `catch_callback.cpp`) but
 * the public `Mesh::on*` surface previously had no tests at all, so the
 * accumulation semantics could regress silently.
 *
 * These tests exercise the public setters (`onReceive`, `onNewConnection`,
 * `onDroppedConnection`, `onChangedConnections`) and lock in the
 * "accumulates, does not replace" contract documented on `Mesh::onReceive`.
 */

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

// Global logger for test environment
painlessmesh::logger::LogClass Log;

namespace {

// Mesh::callbackList is protected (inherited from plugin::PackageHandler).
// Expose it via a thin subclass so tests can inject a Variant and observe
// which user-registered callbacks fire, without going through the async
// receive path.
template <typename T>
class TestMesh : public Mesh<T> {
 public:
  using plugin::PackageHandler<T>::callbackList;
  using plugin::PackageHandler<T>::taskList;
  using Mesh<T>::ackTracker;
  using Mesh<T>::pendingBroadcastAcks;
};

}  // namespace

SCENARIO("Mesh::onReceive accumulates handlers instead of replacing") {
  GIVEN("A mesh with two receive handlers registered back-to-back") {
    Scheduler scheduler;
    TestMesh<Connection> mesh;
    mesh.init(&scheduler, /*nodeId=*/1234567);

    int firstCount = 0;
    int secondCount = 0;
    uint32_t firstSeenFrom = 0;
    uint32_t secondSeenFrom = 0;

    mesh.onReceive([&](uint32_t from, TSTRING& msg) {
      ++firstCount;
      firstSeenFrom = from;
    });
    mesh.onReceive([&](uint32_t from, TSTRING& msg) {
      ++secondCount;
      secondSeenFrom = from;
    });

    WHEN("A SINGLE package is dispatched through the mesh callback list") {
      TSTRING body = "hello";
      protocol::Single pkg(/*fromID=*/999, /*destID=*/1234567, body);
      protocol::Variant var(pkg);

      mesh.callbackList.execute(protocol::SINGLE, var,
                                std::shared_ptr<Connection>(), 0);

      THEN("Both handlers fire — the second onReceive() did not replace the first") {
        REQUIRE(firstCount == 1);
        REQUIRE(secondCount == 1);
        REQUIRE(firstSeenFrom == 999);
        REQUIRE(secondSeenFrom == 999);
      }
    }

    WHEN("A BROADCAST package is dispatched through the mesh callback list") {
      TSTRING body = "shout";
      protocol::Broadcast pkg(/*fromID=*/42, /*destID=*/0, body);
      protocol::Variant var(pkg);

      mesh.callbackList.execute(protocol::BROADCAST, var,
                                std::shared_ptr<Connection>(), 0);

      THEN("Both handlers fire for broadcasts as well") {
        REQUIRE(firstCount == 1);
        REQUIRE(secondCount == 1);
        REQUIRE(firstSeenFrom == 42);
        REQUIRE(secondSeenFrom == 42);
      }
    }
  }
}

SCENARIO("Delivery callbacks run after package dispatch") {
  Scheduler scheduler;
  TestMesh<Connection> mesh;
  mesh.init(&scheduler, /*nodeId=*/1234567);
  bool callbackRan = false;
  const auto msgId = mesh.ackTracker.nextMessageId();
  REQUIRE(mesh.ackTracker.track(
      msgId, {999},
      [&](uint32_t, bool, uint32_t) {
        callbackRan = true;
        mesh.stop();
      },
      5000, 100));

  ack::MessageAckPackage pkg(/*fromNode=*/999, /*destNode=*/1234567, msgId);
  protocol::Variant var(&pkg);
  mesh.callbackList.execute(protocol::MESSAGE_ACK, var,
                            std::shared_ptr<Connection>(), 0);

  REQUIRE_FALSE(callbackRan);
  REQUIRE(mesh.ackTracker.pending() == 0);
  scheduler.execute();
  REQUIRE(callbackRan);
}

SCENARIO("Broadcast acknowledgment bursts use one bounded scheduler task") {
  Scheduler scheduler;
  TestMesh<Connection> mesh;
  mesh.init(&scheduler, /*nodeId=*/1234567);

  for (uint32_t i = 1; i <= PAINLESSMESH_MAX_QUEUED_BROADCAST_ACKS * 3; ++i) {
    TSTRING body = "burst";
    protocol::Broadcast pkg(/*fromID=*/i + 10, /*destID=*/0, body);
    pkg.msgId = i;
    protocol::Variant var(pkg);
    mesh.callbackList.execute(protocol::BROADCAST, var,
                              std::shared_ptr<Connection>(), 0);
  }

  REQUIRE(mesh.taskList.size() == 1);
  REQUIRE(mesh.pendingBroadcastAcks.size() ==
          PAINLESSMESH_MAX_QUEUED_BROADCAST_ACKS);
}

SCENARIO("Mesh::onNewConnection accumulates handlers instead of replacing") {
  GIVEN("A mesh with two new-connection handlers registered") {
    Scheduler scheduler;
    TestMesh<Connection> mesh;
    mesh.init(&scheduler, /*nodeId=*/1234567);

    int firstCount = 0;
    int secondCount = 0;

    mesh.onNewConnection([&](uint32_t nodeId) { ++firstCount; });
    mesh.onNewConnection([&](uint32_t nodeId) { ++secondCount; });

    WHEN("The new-connection callback list is executed") {
      mesh.newConnectionCallbacks.execute(/*nodeId=*/98765);

      THEN("Both handlers fire — the second onNewConnection() did not replace the first") {
        REQUIRE(firstCount == 1);
        REQUIRE(secondCount == 1);
      }
    }
  }
}

SCENARIO("Mesh::onDroppedConnection accumulates handlers instead of replacing") {
  GIVEN("A mesh with two dropped-connection handlers registered") {
    Scheduler scheduler;
    TestMesh<Connection> mesh;
    mesh.init(&scheduler, /*nodeId=*/1234567);

    int firstCount = 0;
    int secondCount = 0;

    mesh.onDroppedConnection([&](uint32_t nodeId) { ++firstCount; });
    mesh.onDroppedConnection([&](uint32_t nodeId) { ++secondCount; });

    WHEN("The dropped-connection callback list is executed") {
      mesh.droppedConnectionCallbacks.execute(/*nodeId=*/98765,
                                              /*station=*/true);

      THEN("Both handlers fire — the second onDroppedConnection() did not replace the first") {
        REQUIRE(firstCount == 1);
        REQUIRE(secondCount == 1);
      }
    }
  }
}
