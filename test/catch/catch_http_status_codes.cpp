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
            
            THEN("205 and 206 are the origin's own acceptance, not a proxy's") {
                // Only 203 is defined as "someone in the middle changed this".
                // 205 Reset Content and 206 Partial Content come from the
                // origin, so on status alone they are accepted; what can
                // still overturn them is a body that refuses (issue #450).
                REQUIRE(isHttpStatusSuccess(205) == true);
                REQUIRE(isHttpStatusSuccess(206) == true);
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
            
            THEN("203 fails; other 2xx codes are the origin's verdict") {
                REQUIRE(isHttpStatusSuccess(203) == false);
                REQUIRE(isHttpStatusSuccess(205) == true);
                REQUIRE(isHttpStatusSuccess(206) == true);
                REQUIRE(isHttpStatusSuccess(208) == true);
                INFO("203 is the one 2xx that says a proxy transformed the reply");
                INFO("For the rest, the response body is what can refuse (issue #450)");
            }
        }
    }
}

/**
 * Helper function to determine if an HTTP status code should trigger automatic retry
 * 
 * NOTE: This intentionally duplicates the retry logic in mesh.hpp handleGatewayAck()
 * to serve as:
 * 1. A specification/documentation of the expected retry behavior
 * 2. A regression test that will fail if the production code changes unexpectedly
 * 
 * If you change this logic, you MUST also update the production code in mesh.hpp
 * and vice versa.
 */
bool isHttpStatusRetryable(uint16_t httpCode) {
    // HTTP 203 (Non-Authoritative Information) - cached/proxied response
    // Often temporary, retrying may succeed when cache expires
    if (httpCode == 203) {
        return true;
    }
    
    // HTTP 5xx server errors are typically transient
    if (httpCode >= 500 && httpCode < 600) {
        return true;
    }
    
    // HTTP 429 (Too Many Requests) should be retried with backoff
    if (httpCode == 429) {
        return true;
    }
    
    // Network errors (httpCode == 0) are retryable
    if (httpCode == 0) {
        return true;
    }
    
    // All other codes are NOT retryable:
    // - 1xx informational: not errors
    // - 2xx success (except 203): request succeeded
    // - 3xx redirects: should be followed by HTTP client, not retried
    // - 4xx client errors (except 429): user error, won't fix with retry
    return false;
}

SCENARIO("HTTP status codes trigger appropriate retry behavior", "[http][retry][issue]") {
    GIVEN("Various HTTP status codes that should trigger retries") {
        WHEN("HTTP 203 (Non-Authoritative Information) is received") {
            THEN("It should be retryable") {
                REQUIRE(isHttpStatusRetryable(203) == true);
                
                INFO("HTTP 203 indicates cached/proxied response");
                INFO("Cache may expire, so retrying can succeed");
                INFO("This fixes the 'permanent 203' issue");
            }
        }
        
        WHEN("HTTP 5xx server errors are received") {
            THEN("500 Internal Server Error should be retryable") {
                REQUIRE(isHttpStatusRetryable(500) == true);
            }
            
            THEN("502 Bad Gateway should be retryable") {
                REQUIRE(isHttpStatusRetryable(502) == true);
            }
            
            THEN("503 Service Unavailable should be retryable") {
                REQUIRE(isHttpStatusRetryable(503) == true);
            }
            
            THEN("504 Gateway Timeout should be retryable") {
                REQUIRE(isHttpStatusRetryable(504) == true);
            }
            
            INFO("Server errors are often transient");
            INFO("Retrying with backoff often succeeds");
        }
        
        WHEN("HTTP 429 (Too Many Requests) is received") {
            THEN("It should be retryable") {
                REQUIRE(isHttpStatusRetryable(429) == true);
                
                INFO("Rate limiting is temporary");
                INFO("Exponential backoff allows rate limit to reset");
            }
        }
        
        WHEN("Network error (HTTP 0) occurs") {
            THEN("It should be retryable") {
                REQUIRE(isHttpStatusRetryable(0) == true);
                
                INFO("Network errors are often transient");
                INFO("Connection may be restored on retry");
            }
        }
    }
    
    GIVEN("Various HTTP status codes that should NOT trigger retries") {
        WHEN("Successful 2xx codes are received") {
            THEN("200 OK should NOT be retryable") {
                REQUIRE(isHttpStatusRetryable(200) == false);
                INFO("Request succeeded, no retry needed");
            }
            
            THEN("201 Created should NOT be retryable") {
                REQUIRE(isHttpStatusRetryable(201) == false);
            }
            
            THEN("204 No Content should NOT be retryable") {
                REQUIRE(isHttpStatusRetryable(204) == false);
            }
        }
        
        WHEN("Client error 4xx codes are received") {
            THEN("400 Bad Request should NOT be retryable") {
                REQUIRE(isHttpStatusRetryable(400) == false);
                INFO("User error, retrying won't help");
            }
            
            THEN("401 Unauthorized should NOT be retryable") {
                REQUIRE(isHttpStatusRetryable(401) == false);
                INFO("Authentication error, needs user intervention");
            }
            
            THEN("404 Not Found should NOT be retryable") {
                REQUIRE(isHttpStatusRetryable(404) == false);
                INFO("Resource doesn't exist, retrying won't help");
            }
        }
        
        WHEN("Redirect 3xx codes are received") {
            THEN("301 Moved Permanently should NOT be retryable") {
                REQUIRE(isHttpStatusRetryable(301) == false);
                INFO("Should be followed by HTTP client, not retried");
            }
            
            THEN("302 Found should NOT be retryable") {
                REQUIRE(isHttpStatusRetryable(302) == false);
            }
        }
    }
}

SCENARIO("HTTP 203 retry behavior resolves the permanent response issue", "[http][203][retry][fix]") {
    GIVEN("The issue scenario: HTTP 203 appearing permanent") {
        INFO("ORIGINAL ISSUE:");
        INFO("- User sends WhatsApp message via sendToInternet()");
        INFO("- Bridge receives HTTP 203 from Callmebot API");
        INFO("- Request immediately fails with no retry");
        INFO("- User sees repeated HTTP 203 failures");
        INFO("- Problem described as '203 response is permanent'");
        INFO("");
        INFO("ROOT CAUSE:");
        INFO("- HTTP 203 treated as terminal failure (no retry)");
        INFO("- Even though cache may expire, request never retried");
        INFO("- User stuck in permanent failure state");
        
        WHEN("HTTP 203 is received") {
            uint16_t httpCode = 203;
            bool shouldRetry = isHttpStatusRetryable(httpCode);
            
            THEN("It should be marked as retryable") {
                REQUIRE(shouldRetry == true);
                
                INFO("FIX:");
                INFO("- HTTP 203 now triggers automatic retry");
                INFO("- Uses exponential backoff (increases delay each retry)");
                INFO("- Gives cache time to expire");
                INFO("- Eventually succeeds when fresh response available");
                INFO("- Or fails after max retries with clear error message");
                INFO("");
                INFO("BEHAVIOR:");
                INFO("- Attempt 1: Immediate send -> HTTP 203");
                INFO("- Attempt 2: Wait 2s -> HTTP 203");
                INFO("- Attempt 3: Wait 4s -> HTTP 203");  
                INFO("- Attempt 4: Wait 8s -> HTTP 200 SUCCESS");
                INFO("  (or max retries reached with clear failure)");
            }
        }
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

            THEN("handleGatewayAck() treats that status as retryable") {
                REQUIRE(isHttpStatusRetryable(outcome.ackStatus) == true);

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

            THEN("The status reaches the origin node so it can be retried") {
                REQUIRE(outcome.transportError == false);
                REQUIRE(outcome.success == false);
                REQUIRE(outcome.ackStatus == 503);
                REQUIRE(isHttpStatusRetryable(outcome.ackStatus) == true);
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
 * Issue #450: the status alone cannot say whether the service accepted the
 * request. CallMeBot answers a rate-limit refusal with HTTP 203 and with HTTP
 * 201, the same HTML page under both. The classifier now reads the body, and
 * a refusal carries the service's words to the origin node.
 */
SCENARIO("The response body can overturn a success-class status",
         "[http][body][issue450]") {
    const std::string tooMany =
        "<h1>Oops! Too many requests...</h1>"
        "<p>You have called to the API to often. Please review your script/code/app.</p>";
    const std::string queued =
        "<p><b>Message queued.</b> You will receive it within a few seconds.</p>";

    GIVEN("A refusal answered with HTTP 201") {
        auto outcome = gateway::classifyHttpResult(201, tooMany);
        THEN("It is a failure that names the service's reason") {
            REQUIRE(outcome.success == false);
            REQUIRE(outcome.refusedByBody == true);
            REQUIRE(outcome.transportError == false);
            REQUIRE(outcome.ackStatus == 201);
            REQUIRE(outcome.reason.find("Too many requests") != std::string::npos);
            REQUIRE(outcome.reason.find("<") == std::string::npos);
        }
    }

    GIVEN("A queued message answered with HTTP 208") {
        auto outcome = gateway::classifyHttpResult(208, queued);
        THEN("It is delivered") {
            REQUIRE(outcome.success == true);
            REQUIRE(outcome.refusedByBody == false);
            REQUIRE(outcome.reason.empty());
        }
    }

    GIVEN("A queued message answered with HTTP 203") {
        auto outcome = gateway::classifyHttpResult(203, queued);
        THEN("It stays ambiguous: a proxy may have answered, not the service") {
            REQUIRE(outcome.success == false);
            REQUIRE(outcome.refusedByBody == false);
        }
    }

    GIVEN("An ordinary failure with a JSON body") {
        auto outcome = gateway::classifyHttpResult(
            400, "{\"error\": \"missing phone\", \"ok\": false}");
        THEN("The reason is the body, and a JSON 'error' key is not a refusal") {
            REQUIRE(outcome.success == false);
            REQUIRE(outcome.refusedByBody == false);
            REQUIRE(outcome.reason.find("missing phone") != std::string::npos);
        }
    }

    GIVEN("A success with a JSON body that merely contains the word error") {
        auto outcome = gateway::classifyHttpResult(200, "{\"error\": null, \"ok\": true}");
        THEN("It is still a success") {
            REQUIRE(outcome.success == true);
        }
    }

    GIVEN("A long HTML page") {
        std::string page = "<html><body>";
        for (int i = 0; i < 50; ++i) page += "<p>line " + std::to_string(i) + "</p>\n";
        THEN("The summary is one line, untagged, and bounded") {
            auto summary = gateway::summarizeResponseBody(page);
            REQUIRE(summary.find("<") == std::string::npos);
            REQUIRE(summary.find("\n") == std::string::npos);
            REQUIRE(summary.size() <= gateway::GATEWAY_RESPONSE_REASON_MAX + 3);
            REQUIRE(summary.substr(0, 6) == "line 0");
        }
    }
}
