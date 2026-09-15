#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"

#include <boost/asio.hpp>

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <string>

#include "ArduinoJson.h"
#include "painlessmesh/gateway.hpp"
#include "sendToInternet/callmebot.h"

// Logger for test environment
painlessmesh::logger::LogClass Log;

/**
 * Issue #450: the gateway's verdict disagrees with the service.
 *
 * (Since 2.0.4 the split is explicit: the library reports what HTTP says and
 * carries the body; examples/sendToInternet/callmebot.h reads what CallMeBot
 * means. The ledger is compared with the example's reading, and the library's
 * part is checked on its own terms: the status, no retry, the words intact.)
 *
 * The bridge in the report reached CallMeBot, got HTTP 208, and told the
 * sketch "Ambiguous response ... not actual delivery". Nobody can say whether
 * the message arrived, because the gateway classifies on the status code alone
 * and throws the body away. Probing CallMeBot while triaging showed why that
 * cannot work: it answers a rate-limit refusal with HTTP 203 *and* with HTTP
 * 201, the same HTML error page under both, and 201 is on the library's
 * success list.
 *
 * The status-code tests this suite already has enumerate codes and check them
 * against a table the tests themselves wrote. This file does something
 * different: it makes a real HTTP request to the test point
 * (test/mock-http-server/server.py), feeds the real response into the real
 * classifier, and then asks the test point's delivery ledger what actually
 * happened. The classifier and the ledger have to agree. The same test point
 * serves the hardware farm, so the verdict here and the verdict there are
 * measured against one source of truth.
 *
 * The test point's address comes from PAINLESSMESH_TESTPOINT
 * (e.g. http://127.0.0.1:8080). CI starts one before running the suite. When
 * the variable is unset the scenarios warn and pass, so a bench build without
 * a server still runs the rest of the suite.
 */

namespace {

struct TestPoint {
  std::string host;
  std::string port;
  bool configured = false;
};

TestPoint testPointFromEnvironment() {
  TestPoint tp;
  const char* raw = std::getenv("PAINLESSMESH_TESTPOINT");
  if (raw == nullptr || *raw == '\0') return tp;
  std::string url(raw);
  const std::string scheme = "http://";
  if (url.compare(0, scheme.size(), scheme) == 0) url.erase(0, scheme.size());
  auto slash = url.find('/');
  if (slash != std::string::npos) url.erase(slash);
  auto colon = url.rfind(':');
  if (colon == std::string::npos) {
    tp.host = url;
    tp.port = "80";
  } else {
    tp.host = url.substr(0, colon);
    tp.port = url.substr(colon + 1);
  }
  tp.configured = !tp.host.empty();
  return tp;
}

struct HttpReply {
  int status = 0;
  std::string body;
  bool chunked = false;  // Transfer-Encoding: chunked; body is the raw framing
};

// A deliberately small HTTP/1.0 client: one request, one reply, connection
// closed by the server. It is the desktop stand-in for HTTPClient on the ESP.
HttpReply httpGet(const TestPoint& tp, const std::string& target) {
  boost::asio::ip::tcp::iostream stream;
  stream.expires_after(std::chrono::seconds(10));
  stream.connect(tp.host, tp.port);
  if (!stream) {
    FAIL("cannot connect to the test point at " << tp.host << ":" << tp.port
                                                 << ": " << stream.error().message());
  }
  stream << "GET " << target << " HTTP/1.0\r\n"
         << "Host: " << tp.host << "\r\n"
         << "Connection: close\r\n\r\n"
         << std::flush;

  HttpReply reply;
  std::string httpVersion;
  stream >> httpVersion >> reply.status;
  std::string line;
  std::getline(stream, line);  // rest of the status line
  while (std::getline(stream, line) && line != "\r") {
    std::string lower = line;
    for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lower.rfind("transfer-encoding:", 0) == 0 &&
        lower.find("chunked") != std::string::npos) {
      reply.chunked = true;
    }
  }
  std::ostringstream body;
  body << stream.rdbuf();
  reply.body = body.str();
  return reply;
}

// What the test point's ledger says happened to the request carrying `tag`.
bool ledgerSaysDelivered(const TestPoint& tp, const std::string& tag) {
  auto reply = httpGet(tp, "/requests/" + tag);
  REQUIRE(reply.status == 200);
  JsonDocument doc;
  auto err = deserializeJson(doc, reply.body);
  REQUIRE(err == DeserializationError::Ok);
  REQUIRE(doc["tag"].as<std::string>() == tag);
  return doc["delivered"].as<bool>();
}

std::string uniqueTag(const std::string& prefix) {
  return prefix + "-" + std::to_string(static_cast<unsigned long>(millis())) +
         "-" + std::to_string(rand() % 100000);
}

}  // namespace

SCENARIO("The gateway's success verdict agrees with the service's delivery ledger",
         "[gateway][internet][testpoint][issue450]") {
  auto tp = testPointFromEnvironment();
  if (!tp.configured) {
    WARN("PAINLESSMESH_TESTPOINT is not set; start test/mock-http-server/server.py "
         "and export PAINLESSMESH_TESTPOINT=http://127.0.0.1:8080 to run this scenario");
    return;
  }

  GIVEN("A CallMeBot-shaped service that does not encode delivery in the status") {
    // Profiles are documented in test/mock-http-server/server.py. `phrase`
    // is what the application must be able to read in the body the library
    // hands it, chosen to be unique to that body: "Already Reported" is also
    // the HTTP reason phrase for 208, which could be echoed without reading a
    // byte of the body. The two 208 profiles are the field finding of issue
    // #452: CallMeBot answered 208 to a message that never arrived, so no
    // body -- not even the delivered profile's own -- makes it a delivery.
    struct Profile {
      const char* name;
      const char* phrase;
      callmebot::Verdict verdict;
    } profiles[] = {{"queued", "Message queued", callmebot::Verdict::Queued},
                    {"ratelimit-203", "Too many requests", callmebot::Verdict::RateLimited},
                    {"ratelimit-201", "Too many requests", callmebot::Verdict::RateLimited},
                    {"unverified-208", "never arrived", callmebot::Verdict::NotDelivered},
                    {"queued-208", "Message queued", callmebot::Verdict::NotDelivered},
                    // How the real service sends a success: chunked. The
                    // application must get the words, not the framing.
                    {"queued-chunked", "Message queued", callmebot::Verdict::Queued},
                    // #463: a success status, a long echo, and the verdict
                    // only in the part of the body past what a gateway keeps
                    // from the start.
                    {"paused-after-echo", "Account is Paused",
                     callmebot::Verdict::AccountPaused}};

    for (const auto& p : profiles) {
      const char* profile = p.name;
      DYNAMIC_SECTION("profile " << profile) {
        auto tag = uniqueTag(profile);
        auto reply = httpGet(
            tp, std::string("/callmebot/whatsapp.php?phone=%2B10000000000&apikey=") +
                    profile + "&text=" + tag);
        REQUIRE(reply.status > 0);

        // The real classifier and summary, on the real status and on the
        // part of the body a gateway keeps -- exactly what the gateway
        // handler hands them.
        painlessmesh::gateway::ResponseExcerpt excerpt;
        if (reply.chunked) {
          painlessmesh::gateway::ChunkedBodyDecoder decoder(excerpt);
          decoder.add(reply.body);
          REQUIRE(decoder.complete());
        } else {
          excerpt.add(reply.body);
        }
        auto outcome =
            painlessmesh::gateway::classifyHttpResult(reply.status, excerpt.text());
        auto response = painlessmesh::gateway::summarizeResponseBody(excerpt.text());
        auto judgement = callmebot::judge(outcome.ackStatus, response);
        bool delivered = ledgerSaysDelivered(tp, tag);

        INFO("service answered HTTP " << reply.status << " with body: " << reply.body);
        INFO("ledger says delivered=" << delivered << ", example says accepted="
                                     << judgement.accepted);

        // The library: the status as HTTP reads it, never a retry after the
        // server answered, and the service's words intact for the sketch.
        REQUIRE(outcome.transportError == false);
        REQUIRE(outcome.retryable == false);
        REQUIRE(outcome.ackStatus == reply.status);
        REQUIRE(response.find(p.phrase) != std::string::npos);

        // The example: its reading agrees with what the service did.
        REQUIRE(judgement.verdict == p.verdict);
        REQUIRE(judgement.accepted == delivered);
      }
    }
  }
}

SCENARIO("Plain status routes still agree with the ledger",
         "[gateway][internet][testpoint]") {
  auto tp = testPointFromEnvironment();
  if (!tp.configured) {
    WARN("PAINLESSMESH_TESTPOINT is not set; skipping");
    return;
  }

  GIVEN("The routes the hardware farm relays through today") {
    struct Route {
      const char* path;
      bool delivered;
    } routes[] = {{"/status/200", true}, {"/status/204", true},
                  {"/status/400", false}, {"/status/503", false}};

    for (const auto& route : routes) {
      DYNAMIC_SECTION("route " << route.path) {
        auto tag = uniqueTag("status");
        auto reply = httpGet(tp, std::string(route.path) + "?tag=" + tag);
        auto outcome =
            painlessmesh::gateway::classifyHttpResult(reply.status, reply.body);
        REQUIRE(ledgerSaysDelivered(tp, tag) == route.delivered);
        REQUIRE(outcome.success == route.delivered);
      }
    }
  }
}
