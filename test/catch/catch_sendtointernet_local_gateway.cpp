#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"

#include "catch_utils.hpp"

#include "painlessmesh/gateway.hpp"
#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

// Logger for test environment
painlessmesh::logger::LogClass Log;

/**
 * Issue #445: a bridge could not reach the Internet through its own uplink.
 *
 * sendToInternet() short-circuits on hasLocalInternet(), executing the gateway
 * package locally instead of routing it to a peer. That flag is driven solely
 * by the Internet health checker, and initAsBridge() never started it -- only
 * initAsSharedGateway() did. So on a bridge the flag stayed false forever, the
 * short-circuit never fired, and a bridge with no peer yet fell through to the
 * mesh-routing path and failed with "No active mesh connections" despite
 * having a working router link.
 *
 * initAsBridge() lives in src/arduino/wifi.hpp, which is ESP-only and is not
 * compiled by this suite. What these scenarios pin down is the contract that
 * made the missing call fatal: with local Internet the send must proceed
 * without any mesh peer, and without it the very same node fails the way the
 * bug report showed.
 */
SCENARIO("A node with local Internet sends without any mesh peer",
         "[gateway][internet][issue445]") {
  GIVEN("A gateway node whose health checker reports local Internet") {
    using Connection = painlessmesh::Connection;
    Scheduler scheduler;
    Mesh<Connection> node;

    node.init(&scheduler, 0x11111111);
    node.enableSendToInternet();

    node.setMockInternetConnected(true);
    node.checkInternetNow();

    REQUIRE(node.hasLocalInternet() == true);

    WHEN("It is the only node in the mesh") {
      REQUIRE(node.hasActiveMeshConnections() == false);

      THEN("sendToInternet() accepts the request instead of failing") {
        bool callbackCalled = false;
        TSTRING callbackError;

        uint32_t msgId = node.sendToInternet(
            "https://api.example.com/data", "{\"test\": \"data\"}",
            [&](bool success, uint16_t httpStatus, TSTRING error) {
              callbackCalled = true;
              callbackError = error;
            });

        REQUIRE(msgId != 0);

        // The request is tracked locally: the package was dispatched to this
        // node's own GATEWAY_DATA handler. That handler is registered by
        // initGatewayInternetHandler() in wifi.hpp and so does not exist in
        // this build, which is why no acknowledgment comes back here.
        REQUIRE(node.getPendingInternetRequestCount() == 1);

        scheduler.execute();

        // Crucially, it did not fail with the issue #445 error.
        REQUIRE(callbackCalled == false);
        REQUIRE(callbackError.find("mesh connections") == TSTRING::npos);
      }
    }
  }
}

SCENARIO("Without a running health check the same node reproduces issue #445",
         "[gateway][internet][issue445]") {
  GIVEN("A gateway node whose health checker has never run") {
    using Connection = painlessmesh::Connection;
    Scheduler scheduler;
    Mesh<Connection> node;

    node.init(&scheduler, 0x11111111);
    node.enableSendToInternet();

    // This is the state initAsBridge() left every bridge in: the uplink may be
    // perfectly healthy, but nothing ever sets the flag sendToInternet() reads.
    REQUIRE(node.hasLocalInternet() == false);
    REQUIRE(node.hasActiveMeshConnections() == false);

    WHEN("It sends to the Internet") {
      bool callbackCalled = false;
      TSTRING callbackError;

      uint32_t msgId = node.sendToInternet(
          "https://api.example.com/data", "{\"test\": \"data\"}",
          [&](bool success, uint16_t httpStatus, TSTRING error) {
            callbackCalled = true;
            callbackError = error;
          });

      scheduler.execute();

      THEN("It fails exactly as reported in the issue") {
        REQUIRE(msgId == 0);
        REQUIRE(callbackCalled == true);
        REQUIRE(callbackError.find("mesh connections") != TSTRING::npos);

        INFO("This is the log line from issue #445:");
        INFO("  ERROR: sendToInternet(): No active mesh connections");
        INFO("The fix is to start the health checker in initAsBridge(),");
        INFO("so a bridge lands in the first scenario, not this one.");
      }
    }
  }
}

/**
 * Bridge promotion runs stop() and then initAsBridge() again, so the health
 * check has to survive a re-init. stop() disables every task and drops it from
 * the reusable pool, but Mesh keeps its shared_ptr, so the enable path used to
 * see a non-null member and refuse -- leaving the re-initialised bridge with no
 * health check and hasLocalInternet() stuck false, i.e. issue #445 again.
 */
SCENARIO("The Internet health check can be re-armed after a stop",
         "[gateway][internet][issue445]") {
  GIVEN("A node with the health check running") {
    using Connection = painlessmesh::Connection;
    Scheduler scheduler;
    Mesh<Connection> node;

    node.init(&scheduler, 0x11111111);
    node.enableInternetHealthCheck();

    REQUIRE(node.isInternetHealthCheckEnabled() == true);

    WHEN("The node is stopped, as bridge promotion does before re-init") {
      node.stop();

      // stop() left the member pointing at a disabled, callback-less task.
      REQUIRE(node.isInternetHealthCheckEnabled() == false);

      node.enableInternetHealthCheck();

      THEN("The health check is running again") {
        REQUIRE(node.isInternetHealthCheckEnabled() == true);
      }

      AND_THEN("It still drives hasLocalInternet()") {
        node.setMockInternetConnected(true);
        node.checkInternetNow();
        REQUIRE(node.hasLocalInternet() == true);
      }
    }
  }
}

SCENARIO("Enabling the health check twice is still a no-op",
         "[gateway][internet]") {
  GIVEN("A node with the health check running") {
    using Connection = painlessmesh::Connection;
    Scheduler scheduler;
    Mesh<Connection> node;

    node.init(&scheduler, 0x11111111);
    node.enableInternetHealthCheck();

    WHEN("Enabling it a second time") {
      node.enableInternetHealthCheck();

      THEN("It remains enabled") {
        REQUIRE(node.isInternetHealthCheckEnabled() == true);
      }
    }
  }
}

// Mesh::callbackList is protected (inherited from plugin::PackageHandler).
// Expose it the way catch_public_callback_api.cpp does, so the scenario below
// can stand in for the GATEWAY_DATA handler that lives in wifi.hpp and is
// never compiled by this suite.
template <typename T>
class LocalGatewayMesh : public Mesh<T> {
 public:
  using plugin::PackageHandler<T>::callbackList;
};

/**
 * A gateway serving its own uplink has no mesh hop to repeat, so a retry has
 * to go back to the local handler. The routing path needs an active peer and
 * a discovered bridge; a lone gateway satisfies neither, so it used to burn
 * every retry without issuing one HTTP request and then report "Max retries
 * exceeded" in place of the transport error that actually happened -- the
 * same dishonest failure as issue #446, one layer up.
 */
SCENARIO("A local gateway retries against its own uplink, not a peer",
         "[gateway][internet][issue446]") {
  GIVEN("A lone gateway whose uplink keeps failing at the transport layer") {
    using Connection = painlessmesh::Connection;
    Scheduler scheduler;
    LocalGatewayMesh<Connection> node;

    node.init(&scheduler, 0x11111111);
    node.enableSendToInternet();
    node.setMockInternetConnected(true);
    node.checkInternetNow();
    node.setInternetRetryCount(2);
    node.setInternetRetryDelay(1);

    REQUIRE(node.hasLocalInternet() == true);
    REQUIRE(node.hasActiveMeshConnections() == false);

    // Stands in for wifi.hpp's GATEWAY_DATA handler, answering the way the
    // real one does when HTTPClient reports a transport error: status 0, a
    // real message, no success.
    int dispatches = 0;
    node.callbackList.onPackage(
        protocol::GATEWAY_DATA,
        [&node, &dispatches](protocol::Variant& variant,
                             std::shared_ptr<Connection>, uint32_t) {
          ++dispatches;
          auto pkg = variant.to<gateway::GatewayDataPackage>();

          gateway::GatewayAckPackage ack;
          ack.from = node.getNodeId();
          ack.dest = pkg.originNode;
          ack.messageId = pkg.messageId;
          ack.originNode = pkg.originNode;
          ack.success = false;
          ack.httpStatus = 0;
          ack.error = "connection refused";
          protocol::Variant ackVariant(&ack);
          node.callbackList.execute(protocol::GATEWAY_ACK, ackVariant,
                                    std::shared_ptr<Connection>(), 0);
          return true;
        });

    WHEN("The request is sent and the scheduler runs long enough to retry") {
      bool callbackCalled = false;
      TSTRING callbackError;

      uint32_t msgId = node.sendToInternet(
          "https://api.example.com/data", "{}",
          [&](bool success, uint16_t httpStatus, TSTRING error) {
            callbackCalled = true;
            callbackError = error;
          });

      REQUIRE(msgId != 0);
      REQUIRE(dispatches == 1);

      auto deadline = millis() + 2000;
      while (!callbackCalled && millis() < deadline) scheduler.execute();

      THEN("Every retry reached the local uplink") {
        REQUIRE(dispatches == 3);  // the first attempt plus two retries
      }

      AND_THEN("The reported error is the real one, not a retry-count fiction") {
        REQUIRE(callbackCalled == true);
        REQUIRE(callbackError.find("connection refused") != TSTRING::npos);
        REQUIRE(callbackError.find("Max retries exceeded") == TSTRING::npos);
      }
    }
  }
}
