#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "catch_utils.hpp"

#include "painlessmesh/ack.hpp"

using namespace painlessmesh;
using namespace painlessmesh::ack;

// Logger for test environment
painlessmesh::logger::LogClass Log;

SCENARIO("MessageAckPackage has correct defaults") {
  GIVEN("A default MessageAckPackage") {
    MessageAckPackage pkg;

    THEN("type should be MESSAGE_ACK (630)") {
      REQUIRE(pkg.type == protocol::MESSAGE_ACK);
      REQUIRE(pkg.type == 630);
    }

    THEN("routing should be SINGLE") { REQUIRE(pkg.routing == router::SINGLE); }

    THEN("messageId should be 0") { REQUIRE(pkg.messageId == 0); }
  }

  GIVEN("A MessageAckPackage built with the convenience constructor") {
    MessageAckPackage pkg(101, 202, 42);

    THEN("fields should be set") {
      REQUIRE(pkg.from == 101);
      REQUIRE(pkg.dest == 202);
      REQUIRE(pkg.messageId == 42);
    }
  }
}

SCENARIO("MessageAckPackage can be serialized and deserialized") {
  GIVEN("A MessageAckPackage with values") {
    MessageAckPackage pkg(12345, 67890, 999);

    WHEN("Converting it to and from a Variant") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<MessageAckPackage>();

      THEN("all fields survive the round trip") {
        REQUIRE(pkg2.type == protocol::MESSAGE_ACK);
        REQUIRE(pkg2.routing == router::SINGLE);
        REQUIRE(pkg2.from == pkg.from);
        REQUIRE(pkg2.dest == pkg.dest);
        REQUIRE(pkg2.messageId == pkg.messageId);
      }
    }
  }
}

SCENARIO("protocol::Single carries an optional msgId") {
  GIVEN("A Single package without a msgId") {
    TSTRING msg = "hello";
    auto pkg = protocol::Single(1, 2, msg);

    THEN("msgId defaults to 0") { REQUIRE(pkg.msgId == 0); }

    WHEN("Serializing it") {
      auto var = protocol::Variant(&pkg);
      TSTRING str;
      var.printTo(str);

      THEN("the wire format contains no msgId field (zero overhead)") {
        REQUIRE(str.find("msgId") == TSTRING::npos);
      }

      THEN("deserializing yields msgId == 0") {
        auto pkg2 = var.to<protocol::Single>();
        REQUIRE(pkg2.msgId == 0);
        REQUIRE(pkg2.msg == msg);
      }
    }
  }

  GIVEN("A Single package with a msgId") {
    TSTRING msg = "hello";
    auto pkg = protocol::Single(1, 2, msg);
    pkg.msgId = 77;

    WHEN("Round-tripping through a Variant") {
      auto var = protocol::Variant(&pkg);
      TSTRING str;
      var.printTo(str);
      auto pkg2 = var.to<protocol::Single>();

      THEN("the msgId is preserved on the wire") {
        REQUIRE(str.find("msgId") != TSTRING::npos);
        REQUIRE(pkg2.msgId == 77);
        REQUIRE(pkg2.from == 1);
        REQUIRE(pkg2.dest == 2);
        REQUIRE(pkg2.msg == msg);
      }
    }
  }

  GIVEN("A Broadcast package with a msgId") {
    TSTRING msg = "toAll";
    auto pkg = protocol::Broadcast(5, 0, msg);
    pkg.msgId = 88;

    WHEN("Round-tripping through a Variant") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<protocol::Broadcast>();

      THEN("the msgId is preserved") {
        REQUIRE(pkg2.msgId == 88);
        REQUIRE(pkg2.type == protocol::BROADCAST);
      }
    }
  }
}

SCENARIO("AckTracker generates unique non-zero message ids") {
  GIVEN("An AckTracker") {
    AckTracker tracker;

    THEN("successive ids are unique and non-zero") {
      auto id1 = tracker.nextMessageId();
      auto id2 = tracker.nextMessageId();
      REQUIRE(id1 != 0);
      REQUIRE(id2 != 0);
      REQUIRE(id1 != id2);
    }
  }
}

SCENARIO("AckTracker tracks a single destination") {
  GIVEN("An AckTracker tracking one message") {
    AckTracker tracker;
    size_t called = 0;
    uint32_t cbNode = 0;
    bool cbDelivered = false;
    uint32_t cbLatency = 0;

    auto id = tracker.nextMessageId();
    tracker.track(id, {2222}, [&](uint32_t nodeId, bool delivered,
                                  uint32_t latencyMs) {
      ++called;
      cbNode = nodeId;
      cbDelivered = delivered;
      cbLatency = latencyMs;
    },
                  5000, 1000);

    REQUIRE(tracker.pending() == 1);

    WHEN("the ACK arrives in time") {
      auto matched = tracker.handleAck(id, 2222, 1250);

      THEN("the callback fires with delivered=true and the latency") {
        REQUIRE(matched);
        REQUIRE(called == 1);
        REQUIRE(cbNode == 2222);
        REQUIRE(cbDelivered);
        REQUIRE(cbLatency == 250);
        REQUIRE(tracker.pending() == 0);
      }

      THEN("a duplicate ACK is ignored") {
        REQUIRE(!tracker.handleAck(id, 2222, 1300));
        REQUIRE(called == 1);
      }
    }

    WHEN("no ACK arrives before the timeout") {
      auto stillPending = tracker.expire(6100);

      THEN("the callback fires with delivered=false") {
        REQUIRE(called == 1);
        REQUIRE(cbNode == 2222);
        REQUIRE(!cbDelivered);
        REQUIRE(stillPending == 0);
        REQUIRE(tracker.pending() == 0);
      }
    }

    WHEN("expire runs before the timeout") {
      auto stillPending = tracker.expire(3000);

      THEN("nothing fires and the message stays pending") {
        REQUIRE(called == 0);
        REQUIRE(stillPending == 1);
        REQUIRE(tracker.pending() == 1);
      }
    }

    WHEN("an ACK for an unknown message arrives") {
      auto matched = tracker.handleAck(9999, 2222, 1250);

      THEN("it is ignored") {
        REQUIRE(!matched);
        REQUIRE(called == 0);
        REQUIRE(tracker.pending() == 1);
      }
    }

    WHEN("an ACK from an unexpected node arrives") {
      auto matched = tracker.handleAck(id, 3333, 1250);

      THEN("it is ignored") {
        REQUIRE(!matched);
        REQUIRE(called == 0);
        REQUIRE(tracker.pending() == 1);
      }
    }
  }
}

SCENARIO("AckTracker tracks broadcasts to multiple destinations") {
  GIVEN("An AckTracker tracking a broadcast to three nodes") {
    AckTracker tracker;
    std::map<uint32_t, bool> results;
    size_t called = 0;

    auto id = tracker.nextMessageId();
    tracker.track(id, {11, 22, 33},
                  [&](uint32_t nodeId, bool delivered, uint32_t) {
                    ++called;
                    results[nodeId] = delivered;
                  },
                  5000, 1000);

    REQUIRE(tracker.pending() == 1);

    WHEN("two nodes ACK and one times out") {
      REQUIRE(tracker.handleAck(id, 11, 1100));
      REQUIRE(tracker.handleAck(id, 33, 1200));
      REQUIRE(tracker.pending() == 1);
      tracker.expire(7000);

      THEN("the callback fires once per node with the correct status") {
        REQUIRE(called == 3);
        REQUIRE(results[11]);
        REQUIRE(!results[22]);
        REQUIRE(results[33]);
        REQUIRE(tracker.pending() == 0);
      }
    }

    WHEN("all nodes ACK") {
      tracker.handleAck(id, 11, 1100);
      tracker.handleAck(id, 22, 1150);
      tracker.handleAck(id, 33, 1200);

      THEN("the message is fully acknowledged") {
        REQUIRE(called == 3);
        REQUIRE(tracker.pending() == 0);
      }
    }
  }
}

SCENARIO("AckTracker handles millis() wraparound") {
  GIVEN("A message sent just before the uint32 clock wraps") {
    AckTracker tracker;
    size_t called = 0;
    bool cbDelivered = false;
    uint32_t cbLatency = 0;

    uint32_t sentAt = 0xFFFFFF00;  // 256 ms before wraparound
    auto id = tracker.nextMessageId();
    tracker.track(id, {42}, [&](uint32_t, bool delivered, uint32_t latencyMs) {
      ++called;
      cbDelivered = delivered;
      cbLatency = latencyMs;
    },
                  5000, sentAt);

    WHEN("the ACK arrives after the clock wrapped") {
      tracker.handleAck(id, 42, 100);  // 356 ms later, after wrap

      THEN("latency is computed correctly across the wrap") {
        REQUIRE(called == 1);
        REQUIRE(cbDelivered);
        REQUIRE(cbLatency == 356);
      }
    }

    WHEN("expire runs shortly after the wrap but before the timeout") {
      tracker.expire(1000);  // only ~1.3 s since send

      THEN("the message is not expired") {
        REQUIRE(called == 0);
        REQUIRE(tracker.pending() == 1);
      }
    }

    WHEN("expire runs after the timeout across the wrap") {
      tracker.expire(6000);

      THEN("the message expires") {
        REQUIRE(called == 1);
        REQUIRE(!cbDelivered);
      }
    }
  }
}

SCENARIO("AckTracker can track many messages independently") {
  GIVEN("Two tracked messages") {
    AckTracker tracker;
    size_t calledA = 0, calledB = 0;

    auto idA = tracker.nextMessageId();
    auto idB = tracker.nextMessageId();
    tracker.track(idA, {7}, [&](uint32_t, bool, uint32_t) { ++calledA; }, 1000,
                  100);
    tracker.track(idB, {7}, [&](uint32_t, bool, uint32_t) { ++calledB; }, 9000,
                  100);

    REQUIRE(tracker.pending() == 2);

    WHEN("only the first times out") {
      auto stillPending = tracker.expire(2000);

      THEN("only the first callback fires") {
        REQUIRE(calledA == 1);
        REQUIRE(calledB == 0);
        REQUIRE(stillPending == 1);
        REQUIRE(tracker.pending() == 1);
      }
    }
  }
}
