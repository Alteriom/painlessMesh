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
 * Issue #450: a bridge's own sendToInternet() fails for the first 30 s.
 *
 * The #445 fix made a bridge serve its own request as soon as
 * hasLocalInternet() is true, and had initAsBridge() start the health checker
 * that drives that flag. The reporter's bridge still logged "No active mesh
 * connections" on its first send, 8.5 s after boot, with a router link that
 * was demonstrably up (the same run later reached CallMeBot over TLS).
 *
 * The sequence in initAsBridge() (src/arduino/wifi.hpp) is:
 *
 *   init()                 -- drops the router association to start the mesh
 *   stationManual()        -- WiFi.begin() again: the station is re-associating
 *   enableInternetHealthCheck()
 *                          -- armed with enable(), so the first probe runs on
 *                             the very next scheduler pass, while the station
 *                             is still associating. It fails. The next probe
 *                             is one full interval (30 s) later.
 *
 * So for the whole first interval the flag is false although the uplink came
 * up a few hundred milliseconds after the probe, and every send in that window
 * falls through to the mesh path -- issue #445 again, with a 30 s window
 * instead of forever.
 *
 * wifi.hpp is ESP-only and not compiled here. What this file pins down is the
 * portable contract: a send made after the uplink has recovered, but before the
 * next periodic probe, must be served locally. The health checker's mock stands
 * in for the station: false while it associates, true once it has an IP.
 */
SCENARIO("A bridge whose first probe ran during station association",
         "[gateway][internet][issue450]") {
  GIVEN("A gateway whose health checker probed once, before the uplink was up") {
    using Connection = painlessmesh::Connection;
    Scheduler scheduler;
    Mesh<Connection> node;

    node.init(&scheduler, 0x11111111);
    node.enableSendToInternet();

    // The library default; spelled out because the scenario depends on it
    // being far longer than the test runs.
    node.setInternetCheckInterval(30000);

    node.setMockInternetConnected(false);
    node.enableInternetHealthCheck();

    // The first probe runs on the next scheduler pass, as it does one line
    // after stationManual() in initAsBridge().
    auto deadline = millis() + 1000;
    while (node.getInternetStatus().checkCount == 0 && millis() < deadline) {
      scheduler.execute();
    }
    REQUIRE(node.getInternetStatus().checkCount == 1);
    REQUIRE(node.hasLocalInternet() == false);
    REQUIRE(node.hasActiveMeshConnections() == false);

    WHEN("The station associates and gets an IP before the next periodic probe") {
      node.setMockInternetConnected(true);

      // Nothing has advanced the periodic task: this is the 30 s window.
      REQUIRE(node.getInternetStatus().checkCount == 1);

      AND_WHEN("The bridge sends its own request, as the reporter's sketch did") {
        bool callbackCalled = false;
        TSTRING callbackError;

        uint32_t msgId = node.sendToInternet(
            "https://api.callmebot.com/whatsapp.php?phone=x&apikey=y&text=hi",
            "",
            [&](bool success, uint16_t httpStatus, TSTRING error) {
              callbackCalled = true;
              callbackError = error;
            });

        scheduler.execute();

        THEN("The request is served on the local uplink, not refused") {
          INFO("This is the bridge's log line from issue #450:");
          INFO("  ERROR: sendToInternet(): No active mesh connections");
          INFO("  Cloud send failed: No mesh connections - cannot route to gateway");
          REQUIRE(msgId != 0);
          REQUIRE(callbackError.find("mesh connections") == TSTRING::npos);

          // Dispatched to this node's own GATEWAY_DATA handler, which lives in
          // wifi.hpp and so does not exist in this build: the request stays
          // pending instead of being acknowledged.
          REQUIRE(callbackCalled == false);
          REQUIRE(node.getPendingInternetRequestCount() == 1);
        }

        AND_THEN("The uplink was re-probed rather than trusted blindly") {
          // The flag must be earned by a probe, not assumed: a bridge whose
          // router really is down must still fall through to a mesh peer.
          REQUIRE(node.getInternetStatus().checkCount >= 2);
          REQUIRE(node.hasLocalInternet() == true);
        }
      }
    }
  }
}

/**
 * The re-probe that closes the window must not turn into a stall on every
 * call. On hardware the probe is a blocking TCP connect with a 5 s timeout, so
 * a node whose uplink is genuinely down -- a bridge whose router is off, a
 * regular node that never had one -- must not pay that on each send.
 */
SCENARIO("A node whose uplink is really down does not re-probe on every send",
         "[gateway][internet][issue450]") {
  GIVEN("A node whose health checker keeps failing") {
    using Connection = painlessmesh::Connection;
    Scheduler scheduler;
    Mesh<Connection> node;

    node.init(&scheduler, 0x11111111);
    node.enableSendToInternet();
    node.setInternetCheckInterval(30000);
    node.setMockInternetConnected(false);
    node.enableInternetHealthCheck();

    auto deadline = millis() + 1000;
    while (node.getInternetStatus().checkCount == 0 && millis() < deadline) {
      scheduler.execute();
    }
    REQUIRE(node.getInternetStatus().checkCount == 1);

    WHEN("It sends five requests back to back") {
      for (int i = 0; i < 5; ++i) {
        node.sendToInternet("https://api.example.com/data", "{}",
                            [](bool, uint16_t, TSTRING) {});
        scheduler.execute();
      }

      THEN("At most one extra probe was spent on them") {
        REQUIRE(node.getInternetStatus().checkCount <= 2);
        REQUIRE(node.hasLocalInternet() == false);
      }
    }
  }
}
