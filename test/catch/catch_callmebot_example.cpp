#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"

#include "painlessmesh/gateway.hpp"
#include "sendToInternet/callmebot.h"

// Logger for test environment
painlessmesh::logger::LogClass Log;

/**
 * The sendToInternet example's reading of a CallMeBot reply.
 *
 * The library says what HTTP says; examples/sendToInternet/callmebot.h says
 * what CallMeBot means. These are the replies the service has actually given
 * in painlessMesh issues #450, #452 and #463, passed through the same
 * summarizeResponseBody() the gateway applies before the body reaches the
 * sketch -- so a phrase that only matches the raw HTML fails here.
 * catch_issue450_testpoint_semantics.cpp checks the same reading against the
 * test point's delivery ledger.
 */

using callmebot::Verdict;
using painlessmesh::gateway::summarizeResponseBody;

namespace {

const std::string QUEUED =
    "<p><b>Message queued.</b> You will receive it within a few seconds.</p>";
const std::string TOO_MANY =
    "<h1>Oops! Too many requests...</h1>"
    "<p>You have called to the API to often. Please review your script/code/app.</p>";

callmebot::Judgement judgeRaw(uint16_t status, const std::string& body) {
  return callmebot::judge(status, summarizeResponseBody(body));
}

}  // namespace

SCENARIO("The example reads CallMeBot's replies, not its status codes",
         "[example][callmebot]") {
  THEN("\"Message queued\" under a success status is accepted") {
    auto j = judgeRaw(200, QUEUED);
    REQUIRE(j.verdict == Verdict::Queued);
    REQUIRE(j.accepted);
    REQUIRE(j.holdOffMs == 0);
  }

  THEN("\"Too many requests\" is a refusal under 201 and 203 alike, and holds off") {
    for (uint16_t status : {201, 203, 200, 429}) {
      INFO("HTTP " << status);
      auto j = judgeRaw(status, TOO_MANY);
      REQUIRE(j.verdict == Verdict::RateLimited);
      REQUIRE_FALSE(j.accepted);
      REQUIRE(j.holdOffMs == callmebot::RATE_LIMIT_HOLD_OFF_MS);
    }
  }

  THEN("208 is not a delivery, even with the queued text (#452)") {
    REQUIRE(judgeRaw(208, QUEUED).verdict == Verdict::NotDelivered);
    REQUIRE(judgeRaw(208, "<p>HTTP 208 Already Reported</p>").verdict ==
            Verdict::NotDelivered);
    REQUIRE_FALSE(judgeRaw(208, QUEUED).accepted);
  }

  THEN("A paused account is recognised after a long echo of the request (#463)") {
    std::string reply = "371 Message to: +10000000000 Text to send: ";
    for (int i = 0; i < 20; ++i) reply += "ALARM: O2 level critical at 5.4 mg/L! ";
    reply += "<b>Your Account is Paused</b> due to technical issues. Please send the "
             "word 'resume' to the bot to re-enable the service.";
    REQUIRE(reply.size() > painlessmesh::gateway::GATEWAY_RESPONSE_HEAD_BYTES +
                               painlessmesh::gateway::GATEWAY_RESPONSE_TAIL_BYTES);
    // Through what the gateway keeps of a body, not the whole string: its
    // first bytes are all echo, and the verdict is only in the tail.
    painlessmesh::gateway::ResponseExcerpt excerpt;
    excerpt.add(reply);
    auto j = judgeRaw(200, excerpt.text());
    REQUIRE(j.verdict == Verdict::AccountPaused);
    REQUIRE_FALSE(j.accepted);
    REQUIRE(std::string(j.meaning).find("resume") != std::string::npos);
  }

  THEN("No reply and unknown replies are never counted as sent") {
    REQUIRE(callmebot::judge(0, "").verdict == Verdict::NoReply);
    REQUIRE(judgeRaw(200, "<p>Something new</p>").verdict == Verdict::Unrecognized);
    REQUIRE(judgeRaw(500, QUEUED).verdict == Verdict::Unrecognized);
    REQUIRE_FALSE(judgeRaw(200, "").accepted);
  }
}

SCENARIO("The example never prints the API key", "[example][callmebot]") {
  THEN("The key is replaced wherever it sits in the query") {
    REQUIRE(callmebot::redactApiKey(
                "https://api.callmebot.com/whatsapp.php?phone=%2B1&apikey=123456&text=hi") ==
            "https://api.callmebot.com/whatsapp.php?phone=%2B1&apikey=***&text=hi");
    REQUIRE(callmebot::redactApiKey("https://x/whatsapp.php?phone=1&apikey=123456") ==
            "https://x/whatsapp.php?phone=1&apikey=***");
    REQUIRE(callmebot::redactApiKey("https://x/status/200") == "https://x/status/200");
  }
}
