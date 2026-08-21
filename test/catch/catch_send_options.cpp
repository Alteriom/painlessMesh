#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"
#undef ARDUINOJSON_ENABLE_ARDUINO_STRING
#undef PAINLESSMESH_ENABLE_ARDUINO_STRING
#define PAINLESSMESH_ENABLE_STD_STRING
typedef std::string TSTRING;

#include "catch_utils.hpp"

#include "painlessmesh/logger.hpp"

painlessmesh::logger::LogClass Log;

#include "painlessmesh/layout.hpp"
#include "painlessmesh/protocol.hpp"
#include "painlessmesh/router.hpp"

using namespace painlessmesh;

SCENARIO("Single and Broadcast carry priority on the wire (issue #384)") {
  GIVEN("A Single package at the default priority") {
    TSTRING msg = "hello";
    auto single = protocol::Single(1, 2, msg);

    THEN("the serialized form omits the prio key entirely") {
      protocol::Variant variant(single);
      TSTRING str;
      variant.printTo(str);
      REQUIRE(str.find("prio") == TSTRING::npos);
      REQUIRE(variant.priority() == protocol::PRIORITY_NORMAL);
    }
  }

  GIVEN("A Single package at CRITICAL priority") {
    TSTRING msg = "alarm";
    auto single = protocol::Single(1, 2, msg);
    single.priority = protocol::PRIORITY_CRITICAL;

    THEN("the serialized form carries prio and it round-trips") {
      protocol::Variant variant(single);
      TSTRING str;
      variant.printTo(str);
      REQUIRE(str.find("\"prio\":0") != TSTRING::npos);

      protocol::Variant parsed(str);
      REQUIRE(!parsed.error);
      REQUIRE(parsed.priority() == protocol::PRIORITY_CRITICAL);
      auto pkg = parsed.to<protocol::Single>();
      REQUIRE(pkg.priority == protocol::PRIORITY_CRITICAL);
      REQUIRE(pkg.msg == "alarm");
    }
  }

  GIVEN("A Broadcast package at LOW priority with an ack msgId") {
    TSTRING msg = "telemetry";
    auto pkg = protocol::Broadcast(1, 0, msg);
    pkg.priority = protocol::PRIORITY_LOW;
    pkg.msgId = 42;

    THEN("priority and msgId compose on the wire") {
      protocol::Variant variant(pkg);
      TSTRING str;
      variant.printTo(str);
      REQUIRE(str.find("\"prio\":3") != TSTRING::npos);
      REQUIRE(str.find("\"msgId\":42") != TSTRING::npos);

      protocol::Variant parsed(str);
      REQUIRE(parsed.priority() == protocol::PRIORITY_LOW);
      REQUIRE(parsed.msgId() == 42);
      auto back = parsed.to<protocol::Broadcast>();
      REQUIRE(back.priority == protocol::PRIORITY_LOW);
      REQUIRE(back.msgId == 42);
    }
  }

  GIVEN("A pre-2.0 package without a prio key") {
    TSTRING str =
        "{\"type\":9,\"dest\":2,\"from\":1,\"msg\":\"legacy\"}";

    THEN("it parses at PRIORITY_NORMAL") {
      protocol::Variant parsed(str);
      REQUIRE(!parsed.error);
      REQUIRE(parsed.priority() == protocol::PRIORITY_NORMAL);
      REQUIRE(parsed.to<protocol::Single>().priority ==
              protocol::PRIORITY_NORMAL);
    }
  }
}

namespace {

class MockConnection : public layout::Neighbour {
 public:
  // (message, priority) pairs in enqueue order
  std::vector<std::pair<std::string, uint8_t>> queued;

  bool addMessage(TSTRING msg, bool priority = false) {
    queued.push_back(
        {msg, priority ? protocol::PRIORITY_HIGH : protocol::PRIORITY_NORMAL});
    return true;
  }

  bool addMessageWithPriority(TSTRING msg, uint8_t priorityLevel) {
    queued.push_back({msg, priorityLevel});
    return true;
  }
};

class MockMesh : public layout::Layout<MockConnection> {
 public:
  void setNodeId(uint32_t id) { nodeId = id; }
};

std::shared_ptr<MockConnection> makeNeighbour(uint32_t id) {
  auto conn = std::make_shared<MockConnection>();
  conn->nodeId = id;
  return conn;
}

}  // namespace

SCENARIO("Forwarding hops preserve the sender's priority (issue #384)") {
  GIVEN("A node routing between two neighbours") {
    MockMesh mesh;
    mesh.setNodeId(1);
    auto from = makeNeighbour(2);
    auto towards = makeNeighbour(3);
    mesh.subs.push_back(from);
    mesh.subs.push_back(towards);
    callback::MeshPackageCallbackList<MockConnection> cbl;

    WHEN("a CRITICAL Single for another node arrives") {
      TSTRING msg = "alarm";
      auto pkg = protocol::Single(5, 3, msg);
      pkg.priority = protocol::PRIORITY_CRITICAL;
      protocol::Variant variant(pkg);
      TSTRING wire;
      variant.printTo(wire);

      router::routePackage<MockConnection>(mesh, from, wire, cbl, 0);

      THEN("it is re-enqueued towards the destination at CRITICAL") {
        REQUIRE(towards->queued.size() == 1);
        REQUIRE(towards->queued[0].second == protocol::PRIORITY_CRITICAL);
        // The forwarded copy keeps the prio field for the next hop too
        REQUIRE(towards->queued[0].first.find("\"prio\":0") !=
                std::string::npos);
        REQUIRE(from->queued.empty());
      }
    }

    WHEN("a default-priority Single for another node arrives") {
      TSTRING msg = "hello";
      auto pkg = protocol::Single(5, 3, msg);
      protocol::Variant variant(pkg);
      TSTRING wire;
      variant.printTo(wire);

      router::routePackage<MockConnection>(mesh, from, wire, cbl, 0);

      THEN("it is forwarded at NORMAL with no prio key") {
        REQUIRE(towards->queued.size() == 1);
        REQUIRE(towards->queued[0].second == protocol::PRIORITY_NORMAL);
        REQUIRE(towards->queued[0].first.find("prio") == std::string::npos);
      }
    }

    WHEN("a HIGH-priority Broadcast arrives from one neighbour") {
      TSTRING msg = "urgent";
      auto pkg = protocol::Broadcast(2, 0, msg);
      pkg.priority = protocol::PRIORITY_HIGH;
      protocol::Variant variant(pkg);
      TSTRING wire;
      variant.printTo(wire);

      router::routePackage<MockConnection>(mesh, from, wire, cbl, 0);

      THEN("it is re-broadcast to the other neighbour at HIGH") {
        REQUIRE(towards->queued.size() == 1);
        REQUIRE(towards->queued[0].second == protocol::PRIORITY_HIGH);
        AND_THEN("the originating neighbour is excluded") {
          REQUIRE(from->queued.empty());
        }
      }
    }
  }
}

SCENARIO("router::send and broadcast enqueue at the package priority") {
  GIVEN("A layout with one neighbour") {
    MockMesh mesh;
    mesh.setNodeId(1);
    auto peer = makeNeighbour(7);
    mesh.subs.push_back(peer);

    WHEN("a typed package with LOW priority is sent") {
      TSTRING msg = "bulk";
      auto pkg = protocol::Single(1, 7, msg);
      pkg.priority = protocol::PRIORITY_LOW;
      REQUIRE(router::send<protocol::Single, MockConnection>(pkg, mesh));

      THEN("the connection sees LOW") {
        REQUIRE(peer->queued.size() == 1);
        REQUIRE(peer->queued[0].second == protocol::PRIORITY_LOW);
      }
    }

    WHEN("a typed broadcast with CRITICAL priority is sent") {
      TSTRING msg = "alert";
      auto pkg = protocol::Broadcast(1, 0, msg);
      pkg.priority = protocol::PRIORITY_CRITICAL;
      auto count =
          router::broadcast<protocol::Broadcast, MockConnection>(pkg, mesh, 0);

      THEN("the connection sees CRITICAL") {
        REQUIRE(count == 1);
        REQUIRE(peer->queued.size() == 1);
        REQUIRE(peer->queued[0].second == protocol::PRIORITY_CRITICAL);
      }
    }
  }
}
