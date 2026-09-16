#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"

#include <algorithm>
#include <vector>

#include "catch_utils.hpp"

#include "painlessmesh/gateway.hpp"
#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

// Logger for test environment
painlessmesh::logger::LogClass Log;

/**
 * What the origin node does with a gateway's answer.
 *
 * On the rig one sendToInternet() produced four HTTP requests: the reply was
 * slow, HTTPClient reported a read timeout, the ack carried status 0, and the
 * origin retried "network errors" three times. The server had the request
 * every time. For a message service that is four messages -- or one message
 * and three "already reported" refusals.
 *
 * These scenarios drive the real path on one node: sendToInternet() on a node
 * with local Internet dispatches to its own GATEWAY_DATA handler, and the
 * handler below stands in for wifi.hpp's, answering through the real
 * sendGatewayAck() with scripted verdicts. Every call of the handler is one
 * HTTP request the gateway would have issued.
 */

namespace {

struct Answer {
  bool success;
  uint16_t httpStatus;
  TSTRING error;
  TSTRING response;
  int8_t retryable;  // -1: a gateway from before the field existed
  uint32_t retryAfterMs;
};

struct Harness {
  Scheduler scheduler;
  Mesh<Connection> node;
  std::vector<Answer> script;
  std::vector<uint32_t> requestTimesMs;
  std::vector<TSTRING> requestIds;
  bool inHandler = false;

  Harness() {
    node.init(&scheduler, 0x11111111);
    node.enableSendToInternet();
    node.setInternetRetryDelay(20);
    node.setMockInternetConnected(true);
    node.checkInternetNow();
    REQUIRE(node.hasLocalInternet());

    node.onPackage(protocol::GATEWAY_DATA, [this](protocol::Variant& variant) {
      auto pkg = variant.to<gateway::GatewayDataPackage>();
      requestTimesMs.push_back(millis());
      requestIds.push_back(
          gateway::requestIdFor(pkg.originNode, pkg.messageId, pkg.requestNonce));
      REQUIRE(!script.empty());
      const size_t index = requestTimesMs.size() - 1;
      const Answer& a = script[index < script.size() ? index : script.size() - 1];
      inHandler = true;
      node.sendGatewayAck(pkg, a.success, a.httpStatus, a.error, nullptr, a.response,
                          a.retryable, a.retryAfterMs);
      inHandler = false;
      return true;
    });
  }

  size_t requests() const { return requestTimesMs.size(); }

  // Runs the scheduler until `done` or `ms` of wall clock have passed.
  template <typename Done>
  void runFor(uint32_t ms, Done done) {
    const auto start = millis();
    while (!done() && millis() - start < ms) {
      scheduler.execute();
      delay(1);
    }
  }
};

}  // namespace

SCENARIO("A request that may have reached the server is issued once",
         "[gateway][internet][retry][duplicates]") {
  Harness h;
  GIVEN("A gateway whose request timed out waiting for the reply") {
    h.script = {{false, 0,
                 "read Timeout (the request may have reached the server; not retried)",
                 "", 0, 0}};
    bool done = false;
    InternetResult result;
    h.node.sendToInternet("http://example.test/slow", "",
                          [&](const InternetResult& r) {
                            done = true;
                            result = r;
                          });
    h.runFor(1000, [&] { return done; });
    h.runFor(200, [] { return false; });  // room for a retry that must not come

    THEN("One HTTP request, and the application is told it was not retried") {
      REQUIRE(done);
      REQUIRE(h.requests() == 1);
      REQUIRE(result.success == false);
      REQUIRE(result.retryable == false);
      REQUIRE(result.attempts == 1);
      REQUIRE(result.error.find("may have reached the server") != TSTRING::npos);
      REQUIRE(h.node.getPendingInternetRequestCount() == 0);
    }
  }

  GIVEN("A gateway that got a 2xx it cannot vouch for") {
    h.script = {{false, 208, "HTTP 208: not a delivery the gateway can confirm",
                 "HTTP 208 Already Reported", 0, 0}};
    bool done = false;
    InternetResult result;
    h.node.sendToInternet("http://example.test/208", "",
                          [&](const InternetResult& r) {
                            done = true;
                            result = r;
                          });
    h.runFor(1000, [&] { return done; });
    h.runFor(200, [] { return false; });

    THEN("It is never resent, and the body reaches the application") {
      REQUIRE(h.requests() == 1);
      REQUIRE(result.httpStatus == 208);
      REQUIRE(result.response == "HTTP 208 Already Reported");
    }
  }
}

SCENARIO("A request that cannot have arrived is retried, with the same id",
         "[gateway][internet][retry]") {
  Harness h;
  GIVEN("A connection refused, then a 200") {
    h.script = {{false, 0, "connection refused", "", 1, 0},
                {true, 200, "", "Message queued.", 1, 0}};
    bool done = false;
    InternetResult result;
    const uint32_t id = h.node.sendToInternet("http://example.test/", "",
                                              [&](const InternetResult& r) {
                                                done = true;
                                                result = r;
                                              });
    h.runFor(2000, [&] { return done; });

    THEN("Two requests, one id, and the result says so") {
      REQUIRE(done);
      REQUIRE(h.requests() == 2);
      REQUIRE(h.requestIds[0] == h.requestIds[1]);
      // Three parts: origin, message id, and the call's nonce.
      REQUIRE(std::count(h.requestIds[0].begin(), h.requestIds[0].end(), '-') == 3);
      REQUIRE(result.success == true);
      REQUIRE(result.messageId == id);
      REQUIRE(result.httpStatus == 200);
      REQUIRE(result.response == "Message queued.");
      REQUIRE(result.attempts == 2);
    }
  }

  GIVEN("A server that keeps answering 503") {
    h.script = {{false, 503, "HTTP 503", "busy", 1, 0}};
    bool done = false;
    InternetResult result;
    h.node.sendToInternet("http://example.test/", "", [&](const InternetResult& r) {
      done = true;
      result = r;
    });
    h.runFor(3000, [&] { return done; });

    THEN("The retries run out, and the last answer is what the application gets") {
      REQUIRE(done);
      REQUIRE(h.requests() == 4);  // the send and three retries
      REQUIRE(result.success == false);
      REQUIRE(result.attempts == 4);
      REQUIRE(result.error.find("503") != TSTRING::npos);
    }
  }
}

SCENARIO("A server's Retry-After sets the earliest retry",
         "[gateway][internet][retry][retry-after]") {
  Harness h;
  GIVEN("A 429 asking for 300 ms while the backoff would wait 20 ms") {
    h.script = {{false, 429, "HTTP 429", "slow down", 1, 300},
                {true, 200, "", "ok", 1, 0}};
    bool done = false;
    h.node.sendToInternet("http://example.test/", "",
                          [&](const InternetResult&) { done = true; });
    h.runFor(3000, [&] { return done; });

    THEN("The retry waits for the server") {
      REQUIRE(done);
      REQUIRE(h.requests() == 2);
      REQUIRE(h.requestTimesMs[1] - h.requestTimesMs[0] >= 300);
    }
  }

  GIVEN("A Retry-After longer than what is left of the request timeout") {
    // Request timeout 1 s; the periodic timeout sweep runs every 5 s, so a
    // 5.5 s Retry-After crosses it with the request still pending.
    h.node.setInternetRequestTimeout(1000);
    h.script = {{false, 503, "HTTP 503", "maintenance", 1, 5500},
                {true, 200, "", "ok", 1, 0}};
    bool done = false;
    InternetResult result;
    h.node.sendToInternet("http://example.test/", "", [&](const InternetResult& r) {
      done = true;
      result = r;
    });
    h.runFor(8000, [&] { return done; });

    THEN("The deadline moves with the wait, and the invited retry runs") {
      REQUIRE(done);
      INFO(result.error);
      REQUIRE(result.success == true);
      REQUIRE(h.requests() == 2);
    }
  }

  GIVEN("A 429 asking for longer than a retry will wait") {
    h.script = {{false, 429, "HTTP 429", "come back tomorrow", 1,
                 gateway::GATEWAY_RETRY_AFTER_MAX_MS + 1000}};
    bool done = false;
    InternetResult result;
    h.node.sendToInternet("http://example.test/", "", [&](const InternetResult& r) {
      done = true;
      result = r;
    });
    h.runFor(1000, [&] { return done; });

    THEN("It fails now and says when the server wants it back") {
      REQUIRE(done);
      REQUIRE(h.requests() == 1);
      REQUIRE(result.retryable == false);
      REQUIRE(result.error.find("retry after 61 s") != TSTRING::npos);
    }
  }
}

SCENARIO("An ack from a gateway without the retry field keeps its old reading, minus 2xx",
         "[gateway][internet][retry][compat]") {
  Harness h;
  GIVEN("A legacy gateway reporting a network error with status 0") {
    h.script = {{false, 0, "connection refused", "", -1, 0},
                {true, 200, "", "", -1, 0}};
    bool done = false;
    bool ok = false;
    h.node.sendToInternet("http://example.test/",
                          "", [&](bool success, uint16_t, TSTRING) {
                            done = true;
                            ok = success;
                          });
    h.runFor(2000, [&] { return done; });
    THEN("It is retried, as before") {
      REQUIRE(h.requests() == 2);
      REQUIRE(ok);
    }
  }

  GIVEN("A legacy gateway reporting a router without Internet") {
    h.script = {{false, 0, "Router has no internet access - check WAN connection", "",
                 -1, 0}};
    bool done = false;
    h.node.sendToInternet("http://example.test/", "",
                          [&](bool, uint16_t, TSTRING) { done = true; });
    h.runFor(1000, [&] { return done; });
    h.runFor(200, [] { return false; });
    THEN("It is final, as before") { REQUIRE(h.requests() == 1); }
  }

  GIVEN("A legacy gateway reporting a 2xx failure") {
    h.script = {{false, 203, "HTTP 203", "", -1, 0}};
    bool done = false;
    h.node.sendToInternet("http://example.test/", "",
                          [&](bool, uint16_t, TSTRING) { done = true; });
    h.runFor(1000, [&] { return done; });
    h.runFor(200, [] { return false; });
    THEN("It is not resent: the server answered") { REQUIRE(h.requests() == 1); }
  }
}

SCENARIO("A gateway's own request completes after its handler has returned",
         "[gateway][internet][local]") {
  // On the HIL rig an ESP8266 shared gateway (~11 KB free) served its own
  // request and delivered the result from inside the HTTP handler, where the
  // HTTPClient, its WiFiClient and the response buffers were still allocated;
  // the application's callback could not allocate the event it built.
  Harness h;
  GIVEN("A gateway answering its own request") {
    h.script = {{true, 200, "", "ok", 0, 0}};
    bool done = false;
    bool calledInsideHandler = true;
    h.node.sendToInternet("http://example.test/", "", [&](const InternetResult& r) {
      done = true;
      calledInsideHandler = h.inHandler;
      REQUIRE(r.success);
    });
    THEN("The callback runs from the scheduler, once the handler is gone") {
      REQUIRE_FALSE(done);
      h.runFor(1000, [&] { return done; });
      REQUIRE(done);
      REQUIRE_FALSE(calledInsideHandler);
      REQUIRE(h.requests() == 1);
    }
  }
}

SCENARIO("A request that never left the node is safe to resend",
         "[gateway][internet][retry]") {
  // No local Internet and no mesh peer: refused before anything is sent.
  Scheduler scheduler;
  Mesh<Connection> node;
  node.init(&scheduler, 0x22222222);
  node.enableSendToInternet();
  node.updateBridgeStatus(0x11111111, true, -42, 6, 10000, "192.168.1.1",
                          static_cast<uint32_t>(millis()));
  REQUIRE_FALSE(node.hasActiveMeshConnections());

  bool done = false;
  InternetResult result;
  const uint32_t id = node.sendToInternet("http://example.test/", "",
                                          [&](const InternetResult& r) {
                                            done = true;
                                            result = r;
                                          });
  const auto start = millis();
  while (!done && millis() - start < 1000) {
    scheduler.execute();
    delay(1);
  }

  THEN("The application is told nothing was issued, and it may resend") {
    REQUIRE(id == 0);
    REQUIRE(done);
    REQUIRE(result.success == false);
    REQUIRE(result.attempts == 0);
    REQUIRE(result.retryable == true);
  }
}

SCENARIO("Two calls never share a request id, even when their message ids do",
         "[gateway][internet][request-id]") {
  // A 16-bit message-id counter wraps after 65,535 requests in a boot; the
  // call's nonce keeps the Idempotency-Key a service sees unique anyway.
  Harness h;
  h.script = {{true, 200, "", "ok", 0, 0}};
  int done = 0;
  for (int i = 0; i < 2; ++i) {
    h.node.sendToInternet("http://example.test/", "",
                          [&](const InternetResult&) { ++done; });
  }
  h.runFor(1000, [&] { return done == 2; });
  REQUIRE(h.requests() == 2);
  REQUIRE(h.requestIds[0] != h.requestIds[1]);

  THEN("The nonce is what differs for the same origin and message id") {
    REQUIRE(gateway::requestIdFor(1, 2, 3) != gateway::requestIdFor(1, 2, 4));
    REQUIRE(gateway::requestIdFor(1, 2, 0) == "pm-00000001-00000002");
    REQUIRE(gateway::newRequestNonce() != 0);
  }
}

SCENARIO("The new ack fields survive the mesh, and old acks parse as before",
         "[gateway][internet][ack]") {
  GIVEN("An ack carrying a response, a retry verdict and a Retry-After") {
    gateway::GatewayAckPackage ack;
    ack.from = 1;
    ack.dest = 2;
    ack.messageId = 3;
    ack.originNode = 2;
    ack.success = false;
    ack.httpStatus = 429;
    ack.error = "HTTP 429";
    ack.response = "Too many requests";
    ack.retryable = 1;
    ack.retryAfterMs = 5000;

    gateway::GatewayDataPackage data;
    data.messageId = 3;
    data.originNode = 2;
    data.requestNonce = 0xDEADBEEF;
    auto dataCopy = protocol::Variant(&data).to<gateway::GatewayDataPackage>();
    gateway::GatewayDataPackage legacy;
    legacy.messageId = 3;
    TSTRING legacyJson;
    protocol::Variant(&legacy).printTo(legacyJson);
    auto legacyCopy = protocol::Variant(&legacy).to<gateway::GatewayDataPackage>();
    THEN("A request's nonce survives, and is absent and 0 when unset") {
      REQUIRE(dataCopy.requestNonce == 0xDEADBEEF);
      REQUIRE(legacyJson.find("nonce") == TSTRING::npos);
      REQUIRE(legacyCopy.requestNonce == 0);
    }

    auto copy = protocol::Variant(&ack).to<gateway::GatewayAckPackage>();
    THEN("They arrive intact") {
      REQUIRE(copy.response == "Too many requests");
      REQUIRE(copy.retryable == 1);
      REQUIRE(copy.retryAfterMs == 5000);
    }
  }

  GIVEN("An ack with none of them set") {
    gateway::GatewayAckPackage ack;
    ack.from = 1;
    ack.dest = 2;
    ack.messageId = 3;
    ack.originNode = 2;
    ack.httpStatus = 200;
    ack.success = true;

    protocol::Variant variant(&ack);
    TSTRING json;
    variant.printTo(json);
    auto copy = variant.to<gateway::GatewayAckPackage>();
    THEN("Nothing new goes on the wire, and the reader sees the legacy default") {
      REQUIRE(json.find("resp") == TSTRING::npos);
      REQUIRE(json.find("retry") == TSTRING::npos);
      REQUIRE(copy.retryable == -1);
      REQUIRE(copy.retryAfterMs == 0);
      REQUIRE(copy.response.empty());
    }
  }
}
