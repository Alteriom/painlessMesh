#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
#include "painlessmesh/gateway.hpp"
#include "painlessmesh/logger.hpp"

using namespace painlessmesh;

// Declare logger for test environment
painlessmesh::logger::LogClass Log;

/**
 * Tests for HTTP status code interpretation in gateway Internet handler
 * 
 * Issue: HTTP 203 was incorrectly treated as success, causing false positives
 * where messages appeared to be delivered but actually weren't.
 * 
 * Fix: Only specific 2xx codes (200, 201, 202, 204) are treated as success.
 * Other 2xx codes like 203 are now treated as failures with informative errors.
 */

/**
 * Helper calling the production classifier used by the gateway handler.
 *
 * This used to be a local re-implementation taking a uint16_t. That mirrored
 * the production bug rather than catching it: wifi.hpp also stored the
 * HTTPClient result in a uint16_t, so its negative transport errors wrapped
 * (-1 became 65535) and were reported as HTTP status codes. Both sides now go
 * through painlessmesh::gateway::classifyHttpResult(int), so these tests
 * exercise the real code path.
 */
bool isHttpStatusSuccess(int httpCode) {
    return gateway::classifyHttpResult(httpCode).success;
}

SCENARIO("HTTP status codes are correctly classified as success or failure", "[http][status][issue]") {
    GIVEN("Various HTTP status codes") {
        WHEN("Checking codes that should be treated as SUCCESS") {
            THEN("200 OK should be success") {
                REQUIRE(isHttpStatusSuccess(200) == true);
            }
            
            THEN("201 Created should be success") {
                REQUIRE(isHttpStatusSuccess(201) == true);
            }
            
            THEN("202 Accepted should be success") {
                REQUIRE(isHttpStatusSuccess(202) == true);
            }
            
            THEN("204 No Content should be success") {
                REQUIRE(isHttpStatusSuccess(204) == true);
            }
        }
        
        WHEN("Checking codes that should be treated as FAILURE") {
            THEN("203 Non-Authoritative Information should NOT be success") {
                // This is the bug fix - 203 was incorrectly treated as success
                REQUIRE(isHttpStatusSuccess(203) == false);
                
                INFO("HTTP 203 indicates a cached/proxied response");
                INFO("For APIs like WhatsApp/Callmebot, this does not guarantee delivery");
                INFO("The message may not have reached the actual destination service");
            }
            
            THEN("205, 206 and 208 are not deliveries the gateway can confirm") {
                // #451 briefly accepted every 2xx but 203 as the origin's own
                // verdict. The next report (#452) was a CallMeBot HTTP 208 for
                // a message that never arrived, printed as "sent". Only 200,
                // 201, 202 and 204 count on the status alone.
                REQUIRE(isHttpStatusSuccess(205) == false);
                REQUIRE(isHttpStatusSuccess(206) == false);
                REQUIRE(isHttpStatusSuccess(208) == false);
            }
        }
        
        WHEN("Checking other status code ranges") {
            THEN("1xx Informational should NOT be success") {
                REQUIRE(isHttpStatusSuccess(100) == false);
                REQUIRE(isHttpStatusSuccess(101) == false);
            }
            
            THEN("3xx Redirects should NOT be success") {
                REQUIRE(isHttpStatusSuccess(301) == false);
                REQUIRE(isHttpStatusSuccess(302) == false);
                REQUIRE(isHttpStatusSuccess(304) == false);
            }
            
            THEN("4xx Client Errors should NOT be success") {
                REQUIRE(isHttpStatusSuccess(400) == false);
                REQUIRE(isHttpStatusSuccess(401) == false);
                REQUIRE(isHttpStatusSuccess(404) == false);
            }
            
            THEN("5xx Server Errors should NOT be success") {
                REQUIRE(isHttpStatusSuccess(500) == false);
                REQUIRE(isHttpStatusSuccess(502) == false);
                REQUIRE(isHttpStatusSuccess(503) == false);
            }
        }
    }
}

SCENARIO("HTTP 203 specific behavior documentation", "[http][203][whatsapp]") {
    GIVEN("The issue scenario from the bug report") {
        INFO("ORIGINAL PROBLEM:");
        INFO("- Node reported: '✅ WhatsApp message sent! HTTP Status: 203'");
        INFO("- User expected: Message delivered to WhatsApp");
        INFO("- Actual result: Message was NOT delivered");
        INFO("");
        INFO("ROOT CAUSE:");
        INFO("- HTTP 203 means 'Non-Authoritative Information'");
        INFO("- Response came from cache/proxy, not from WhatsApp API server");
        INFO("- Old code: success = (httpCode >= 200 && httpCode < 300)");
        INFO("- This incorrectly treated 203 as success");
        
        WHEN("HTTP 203 is received") {
            uint16_t httpCode = 203;
            bool success = isHttpStatusSuccess(httpCode);
            
            THEN("It should be treated as FAILURE, not success") {
                REQUIRE(success == false);
                
                INFO("FIX:");
                INFO("- New code only accepts: 200, 201, 202, 204");
                INFO("- HTTP 203 now treated as failure");
                INFO("- Error message: 'Ambiguous response - may indicate cached response'");
                INFO("- User will see: '❌ Failed to send WhatsApp: Ambiguous response...'");
            }
        }
    }
}

SCENARIO("Success codes for different REST API operations", "[http][rest][api]") {
    GIVEN("Common REST API operations") {
        WHEN("Performing a GET request to retrieve data") {
            THEN("200 OK indicates success") {
                REQUIRE(isHttpStatusSuccess(200) == true);
                INFO("Standard success response for GET requests");
            }
        }
        
        WHEN("Performing a POST request to create a resource") {
            THEN("200 OK indicates success") {
                REQUIRE(isHttpStatusSuccess(200) == true);
            }
            
            THEN("201 Created indicates success") {
                REQUIRE(isHttpStatusSuccess(201) == true);
                INFO("Indicates new resource was successfully created");
            }
        }
        
        WHEN("Performing an async operation") {
            THEN("202 Accepted indicates success") {
                REQUIRE(isHttpStatusSuccess(202) == true);
                INFO("Request accepted for processing, may complete later");
            }
        }
        
        WHEN("Performing a DELETE request") {
            THEN("200 OK indicates success") {
                REQUIRE(isHttpStatusSuccess(200) == true);
            }
            
            THEN("204 No Content indicates success") {
                REQUIRE(isHttpStatusSuccess(204) == true);
                INFO("Successful deletion with no response body");
            }
        }
    }
}

SCENARIO("WhatsApp/Callmebot API specific behavior", "[whatsapp][callmebot][api]") {
    GIVEN("The Callmebot WhatsApp API") {
        INFO("API Endpoint: https://api.callmebot.com/whatsapp.php");
        INFO("Expected success code: 200 OK");
        INFO("Common failure: 203 from cache/proxy (message not actually sent)");
        
        WHEN("Message is successfully delivered") {
            uint16_t httpCode = 200;
            bool success = isHttpStatusSuccess(httpCode);
            
            THEN("200 should be recognized as success") {
                REQUIRE(success == true);
                INFO("User should see: '✅ WhatsApp message sent! HTTP Status: 200'");
            }
        }
        
        WHEN("Response comes from cache/proxy") {
            uint16_t httpCode = 203;
            bool success = isHttpStatusSuccess(httpCode);
            
            THEN("203 should be recognized as failure") {
                REQUIRE(success == false);
                INFO("User should see: '❌ Failed to send WhatsApp: Ambiguous response...'");
                INFO("This prevents false positives where user thinks message was sent");
            }
        }
    }
}

SCENARIO("Backward compatibility considerations", "[http][compatibility]") {
    GIVEN("The old behavior accepted all 2xx codes") {
        INFO("OLD BEHAVIOR: success = (httpCode >= 200 && httpCode < 300)");
        INFO("NEW BEHAVIOR: success = (200, 201, 202, 204 only)");
        
        WHEN("Considering impact on existing applications") {
            THEN("Most legitimate APIs only use 200 for success") {
                REQUIRE(isHttpStatusSuccess(200) == true);
                INFO("This is by far the most common success code");
            }
            
            THEN("REST APIs may use 201, 202, 204 appropriately") {
                REQUIRE(isHttpStatusSuccess(201) == true);
                REQUIRE(isHttpStatusSuccess(202) == true);
                REQUIRE(isHttpStatusSuccess(204) == true);
                INFO("These are still accepted as success");
            }
            
            THEN("2xx codes outside 200/201/202/204 are unverified, not success") {
                REQUIRE(isHttpStatusSuccess(203) == false);
                REQUIRE(isHttpStatusSuccess(205) == false);
                REQUIRE(isHttpStatusSuccess(206) == false);
                REQUIRE(isHttpStatusSuccess(208) == false);
                INFO("CallMeBot's 208 never delivered (issue #452); the body is reported instead");
            }
        }
    }
}

/**
 * Whether the gateway tells the origin node it may resend a request that
 * ended with this HTTPClient result.
 *
 * This used to be a test-local copy of the retry policy, kept "in sync" with
 * mesh.hpp by hand -- which is how a policy that retried a read timeout, and
 * so delivered one request four times on the rig, stayed green. It asks the
 * production classifier instead.
 */
bool retryableFor(int rawCode) {
    return gateway::classifyHttpResult(rawCode).retryable;
}

SCENARIO("A request is retried only when a retry cannot deliver it twice",
         "[http][retry][duplicates]") {
    GIVEN("Replies in which the server says it did not take the request") {
        THEN("429 Too Many Requests and 503 Service Unavailable are retried") {
            REQUIRE(retryableFor(429) == true);
            REQUIRE(retryableFor(503) == true);
        }
    }

    GIVEN("Transport errors that fail before the request is complete") {
        THEN("They are retried: nothing reached a server") {
            for (int code : {-1, -2, -3}) {
                INFO("HTTPClient error " << code);
                REQUIRE(retryableFor(code) == true);
                REQUIRE(gateway::transportErrorMayHaveReachedServer(code) == false);
            }
        }
    }

    GIVEN("Replies and errors after which the server may have processed it") {
        THEN("A read timeout is not retried: the rig saw one send issued four times") {
            REQUIRE(retryableFor(-11) == false);
            REQUIRE(gateway::transportErrorMayHaveReachedServer(-11) == true);
        }
        THEN("Every error raised after the request was written is not retried") {
            // handleHeaderResponse() runs once the whole request is out: a
            // connection closed before the reply (-4) or a reply that was not
            // HTTP (-7) left a request the server may have processed.
            for (int code : {-4, -5, -7, -9, -10}) {
                INFO("HTTPClient error " << code);
                REQUIRE(retryableFor(code) == false);
                REQUIRE(gateway::transportErrorMayHaveReachedServer(code) == true);
            }
        }
        THEN("Stream and memory errors, and unknown codes, are not retried") {
            for (int code : {-6, -8, -12, -99, 0}) {
                INFO("HTTPClient error " << code);
                REQUIRE(retryableFor(code) == false);
            }
        }
        THEN("No 2xx is retried: the server answered") {
            for (int code : {200, 201, 202, 203, 204, 205, 206, 208}) {
                INFO("HTTP " << code);
                REQUIRE(retryableFor(code) == false);
            }
        }
        THEN("500, 502 and 504 are not retried: the request may have been handled") {
            REQUIRE(retryableFor(500) == false);
            REQUIRE(retryableFor(502) == false);
            REQUIRE(retryableFor(504) == false);
        }
        THEN("Client errors and redirects are not retried") {
            for (int code : {301, 302, 400, 401, 403, 404, 422}) {
                INFO("HTTP " << code);
                REQUIRE(retryableFor(code) == false);
            }
        }
    }
}

SCENARIO("A server's Retry-After is read, and only its delay-seconds form",
         "[http][retry][retry-after]") {
    THEN("Seconds become milliseconds, surrounding spaces ignored") {
        REQUIRE(gateway::parseRetryAfterMs("5") == 5000);
        REQUIRE(gateway::parseRetryAfterMs(" 30 ") == 30000);
        REQUIRE(gateway::parseRetryAfterMs("0") == 0);
    }
    THEN("An HTTP-date, garbage or nothing is no instruction") {
        REQUIRE(gateway::parseRetryAfterMs("Wed, 21 Oct 2026 07:28:00 GMT") == 0);
        REQUIRE(gateway::parseRetryAfterMs("soon") == 0);
        REQUIRE(gateway::parseRetryAfterMs("") == 0);
        REQUIRE(gateway::parseRetryAfterMs("-5") == 0);
    }
    THEN("A delay past the ceiling is kept as it is, so the caller can refuse it") {
        REQUIRE(gateway::parseRetryAfterMs("3600") == 3600000UL);
        REQUIRE(gateway::parseRetryAfterMs("3600") > gateway::GATEWAY_RETRY_AFTER_MAX_MS);
        REQUIRE(gateway::parseRetryAfterMs("99999999999") == 0xFFFFFFFFUL);
    }
}

SCENARIO("Every attempt at one request carries the same request id",
         "[http][retry][request-id]") {
    THEN("It depends on the origin node and the message id, and nothing else") {
        const auto id = gateway::requestIdFor(0xCA4CFCF5, 0x0001000A);
        REQUIRE(id == "pm-ca4cfcf5-0001000a");
        REQUIRE(gateway::requestIdFor(0xCA4CFCF5, 0x0001000A) == id);
        REQUIRE(gateway::requestIdFor(0xCA4CFCF5, 0x0001000B) != id);
        REQUIRE(gateway::requestIdFor(0x00000001, 0x0001000A) != id);
    }
    THEN("With the call's nonce it has a third part, and the nonce alone separates two calls") {
        REQUIRE(gateway::requestIdFor(0xCA4CFCF5, 0x0001000A, 0x00C0FFEE) ==
                "pm-ca4cfcf5-0001000a-00c0ffee");
        REQUIRE(gateway::requestIdFor(0xCA4CFCF5, 0x0001000A, 1) !=
                gateway::requestIdFor(0xCA4CFCF5, 0x0001000A, 2));
    }
}

// ============================================================================
// Issue #446: HTTPClient transport errors are negative, not HTTP statuses
// ============================================================================

SCENARIO("HTTPClient transport errors are not mistaken for HTTP statuses",
         "[http][transport][issue446]") {
    GIVEN("The negative codes HTTPClient returns when no response arrives") {
        INFO("ORIGINAL PROBLEM:");
        INFO("- Node reported: 'Cloud send failed: HTTP 65535'");
        INFO("- 65535 is not an HTTP status; it is (uint16_t)-1");
        INFO("");
        INFO("ROOT CAUSE:");
        INFO("- wifi.hpp stored HTTPClient::POST()/GET() in a uint16_t");
        INFO("- HTTPC_ERROR_CONNECTION_REFUSED (-1) wrapped to 65535");
        INFO("- 65535 passes `httpCode > 0`, so the errorToString() branch");
        INFO("  was unreachable and the real cause never reached the node");

        WHEN("A connection is refused (HTTPC_ERROR_CONNECTION_REFUSED, -1)") {
            auto outcome = gateway::classifyHttpResult(-1);

            THEN("It is a transport error, not a status code") {
                REQUIRE(outcome.transportError == true);
                REQUIRE(outcome.success == false);
            }

            THEN("The acknowledgment carries status 0, never 65535") {
                REQUIRE(outcome.ackStatus == 0);
                REQUIRE(outcome.ackStatus != 65535);
            }

            THEN("The gateway says it may be retried: nothing reached a server") {
                REQUIRE(outcome.retryable == true);

                INFO("Before the fix the ack carried 65535, which falls into");
                INFO("the non-retryable bucket, so a transient network failure");
                INFO("was reported to the sketch as permanent.");
            }
        }

        WHEN("Any other HTTPC_ERROR_* code is returned") {
            THEN("Every negative code classifies as a transport error") {
                // -1 .. -11 are the HTTPC_ERROR_* range in the Arduino cores
                for (int code = -1; code >= -11; --code) {
                    auto outcome = gateway::classifyHttpResult(code);
                    INFO("HTTPClient error code: " << code);
                    REQUIRE(outcome.transportError == true);
                    REQUIRE(outcome.success == false);
                    REQUIRE(outcome.ackStatus == 0);
                }
            }
        }

        WHEN("Zero is returned") {
            auto outcome = gateway::classifyHttpResult(0);

            THEN("It is treated as a transport error, not a status") {
                REQUIRE(outcome.transportError == true);
                REQUIRE(outcome.success == false);
                REQUIRE(outcome.ackStatus == 0);
            }
        }
    }
}

SCENARIO("Real HTTP statuses survive classification unchanged",
         "[http][transport][issue446]") {
    GIVEN("Positive codes returned by HTTPClient") {
        WHEN("A success code is returned") {
            auto outcome = gateway::classifyHttpResult(200);

            THEN("It is forwarded as-is and is not a transport error") {
                REQUIRE(outcome.transportError == false);
                REQUIRE(outcome.success == true);
                REQUIRE(outcome.ackStatus == 200);
            }
        }

        WHEN("A failure code is returned") {
            auto outcome = gateway::classifyHttpResult(503);

            THEN("The status reaches the origin node, marked retryable") {
                REQUIRE(outcome.transportError == false);
                REQUIRE(outcome.success == false);
                REQUIRE(outcome.ackStatus == 503);
                REQUIRE(outcome.retryable == true);
            }
        }

        WHEN("A code larger than uint16_t is somehow returned") {
            auto outcome = gateway::classifyHttpResult(70000);

            THEN("It saturates rather than wrapping to a plausible status") {
                REQUIRE(outcome.ackStatus == 65535);
                REQUIRE(outcome.success == false);
            }
        }
    }
}

/**
 * Whether a reply means what the application wanted depends on the service,
 * so the library does not decide it from the body. It applies HTTP's meaning
 * of the status and carries the body, summarized, to the application. (2.0.3
 * matched CallMeBot's "Too many requests" page inside the library; that
 * knowledge now lives in the sendToInternet example, where it can be tested
 * against the service without making the library a CallMeBot client.)
 */
SCENARIO("The library reports the status and carries the body; it does not judge it",
         "[http][body]") {
    const std::string tooMany =
        "<h1>Oops! Too many requests...</h1>"
        "<p>You have called to the API to often. Please review your script/code/app.</p>";

    GIVEN("A 201 whose body a particular service uses to refuse") {
        auto outcome = gateway::classifyHttpResult(201, tooMany);
        THEN("It is an HTTP success; the words are for the application") {
            REQUIRE(outcome.success == true);
            REQUIRE(outcome.unverifiedStatus == false);
            REQUIRE(outcome.retryable == false);
            REQUIRE(outcome.reason.empty());
            REQUIRE(gateway::summarizeResponseBody(tooMany).find("Too many requests") !=
                    std::string::npos);
        }
    }

    GIVEN("HTTP 208") {
        auto outcome = gateway::classifyHttpResult(208, "<p>HTTP 208 Already Reported</p>");
        THEN("It is an unverified failure, never retried, that carries the body") {
            REQUIRE(outcome.success == false);
            REQUIRE(outcome.unverifiedStatus == true);
            REQUIRE(outcome.retryable == false);
            REQUIRE(outcome.ackStatus == 208);
            REQUIRE(outcome.reason.find("Already Reported") != std::string::npos);
        }
    }

    GIVEN("An ordinary failure with a JSON body") {
        auto outcome = gateway::classifyHttpResult(
            400, "{\"error\": \"missing phone\", \"ok\": false}");
        THEN("The reason is the body") {
            REQUIRE(outcome.success == false);
            REQUIRE(outcome.reason.find("missing phone") != std::string::npos);
        }
    }

    GIVEN("A long HTML page") {
        std::string page = "<html><body>";
        for (int i = 0; i < 50; ++i) page += "<p>line " + std::to_string(i) + "</p>\n";
        THEN("The summary is one line, untagged, bounded, and keeps both ends") {
            auto summary = gateway::summarizeResponseBody(page);
            REQUIRE(summary.find("<") == std::string::npos);
            REQUIRE(summary.find("\n") == std::string::npos);
            REQUIRE(summary.size() <= gateway::GATEWAY_RESPONSE_REASON_MAX);
            REQUIRE(summary.substr(0, 6) == "line 0");
            REQUIRE(summary.find(" ... ") != std::string::npos);
            REQUIRE(summary.find("line 49") != std::string::npos);
        }
    }

    GIVEN("A body longer than the gateway keeps") {
        std::string body = "BEGIN ";
        for (int i = 0; i < 400; ++i) body += "filler" + std::to_string(i) + " ";
        body += "the verdict is here END";
        gateway::ResponseExcerpt excerpt;
        excerpt.add(body);
        THEN("The excerpt keeps both ends, marks the gap, and is bounded") {
            auto text = excerpt.text();
            REQUIRE(text.substr(0, 6) == "BEGIN ");
            REQUIRE(text.find(" ... ") != std::string::npos);
            REQUIRE(text.size() == gateway::GATEWAY_RESPONSE_HEAD_BYTES + 5 +
                                       gateway::GATEWAY_RESPONSE_TAIL_BYTES);
            REQUIRE(text.substr(text.size() - 23) == "the verdict is here END");
            auto summary = gateway::summarizeResponseBody(text);
            REQUIRE(summary.find("the verdict is here END") != std::string::npos);
        }
    }

    GIVEN("A body short enough to keep whole") {
        gateway::ResponseExcerpt excerpt;
        excerpt.add(std::string("<p>Message queued.</p>"));
        THEN("It is kept as it was, with no gap") {
            REQUIRE(excerpt.text() == "<p>Message queued.</p>");
        }
    }

    GIVEN("A body past the scan limit") {
        gateway::ResponseExcerpt excerpt;
        size_t accepted = 0;
        while (excerpt.add('x')) ++accepted;
        THEN("Reading stops, and the excerpt stays bounded") {
            REQUIRE(accepted + 1 == gateway::GATEWAY_RESPONSE_SCAN_BYTES);
            REQUIRE(excerpt.text().size() == gateway::GATEWAY_RESPONSE_HEAD_BYTES + 5 +
                                                 gateway::GATEWAY_RESPONSE_TAIL_BYTES);
        }
    }

    GIVEN("A reply that echoes the request before its verdict, as in issue #463") {
        const std::string echoed =
            "371 Message to: +10000000000 Text to send: ALARM: O2 level critical at "
            "5.4 mg/L! Node: 3394043125 and a long tail of repeated request text "
            "that pads the reply well past any single line a log will show "
            "Your Account is Paused due to technical issues. Please send the word "
            "'resume' to the bot to re-enable the service for your number.";
        THEN("The summary still ends with the verdict") {
            auto summary = gateway::summarizeResponseBody(echoed);
            REQUIRE(summary.size() <= gateway::GATEWAY_RESPONSE_REASON_MAX);
            REQUIRE(summary.find("re-enable the service for your number.") != std::string::npos);
            REQUIRE(summary.find("371 Message to") == 0);
        }
    }
}

SCENARIO("A host that failed to resolve is refused without another lookup",
         "[gateway][dns][issue453]") {
    GIVEN("The host of a URL") {
        THEN("It is the authority without scheme, port, path or query") {
            REQUIRE(gateway::hostFromUrl("https://api.example.com/sensors") == "api.example.com");
            REQUIRE(gateway::hostFromUrl("http://10.42.0.1:8088/status/200?tag=x") == "10.42.0.1");
            REQUIRE(gateway::hostFromUrl("https://api.callmebot.com/whatsapp.php?phone=%2B1&text=a") == "api.callmebot.com");
            REQUIRE(gateway::hostFromUrl("not a url") == "not a url");
            REQUIRE(gateway::hostFromUrl("") == "");
        }
        THEN("An IPv4 literal is recognised, so it is never looked up") {
            REQUIRE(gateway::looksLikeIpLiteral("10.42.0.1") == true);
            REQUIRE(gateway::looksLikeIpLiteral("api.example.com") == false);
            REQUIRE(gateway::looksLikeIpLiteral("") == false);
        }
    }

    GIVEN("A cache that remembered a failure at t=1000") {
        gateway::NegativeDnsCache cache;
        cache.remember("api.example.com", 1000, 60000);

        THEN("The host is refused inside the TTL, with its age") {
            uint32_t age = 0;
            REQUIRE(cache.isFailing("api.example.com", 31000, &age) == true);
            REQUIRE(age == 30000);
        }
        THEN("Another host is not") {
            REQUIRE(cache.isFailing("api.callmebot.com", 31000) == false);
        }
        THEN("The TTL expires, and the entry is released") {
            REQUIRE(cache.isFailing("api.example.com", 61000) == false);
            REQUIRE(cache.size() == 0);
        }
        THEN("A later failure of the same host refreshes its one entry") {
            cache.remember("api.example.com", 5000, 60000);
            REQUIRE(cache.size() == 1);
            REQUIRE(cache.isFailing("api.example.com", 64000) == true);
        }
        THEN("The oldest entry is evicted when the slots are full") {
            cache.remember("b", 2000, 60000);
            cache.remember("c", 3000, 60000);
            cache.remember("d", 4000, 60000);
            cache.remember("e", 5000, 60000);
            REQUIRE(cache.size() == gateway::NegativeDnsCache::SLOTS);
            REQUIRE(cache.isFailing("api.example.com", 6000) == false);
            REQUIRE(cache.isFailing("e", 6000) == true);
        }
        THEN("forget() releases a host once it resolves again") {
            cache.forget("api.example.com");
            REQUIRE(cache.isFailing("api.example.com", 2000) == false);
        }
    }
}
