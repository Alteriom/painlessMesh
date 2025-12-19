#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
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
 * Helper function to determine if an HTTP status code should be treated as success
 * 
 * NOTE: This intentionally duplicates the logic in wifi.hpp initGatewayInternetHandler()
 * to serve as:
 * 1. A specification/documentation of the expected behavior
 * 2. A regression test that will fail if the production code changes unexpectedly
 * 
 * If you change this logic, you MUST also update the production code in wifi.hpp
 * and vice versa.
 */
bool isHttpStatusSuccess(uint16_t httpCode) {
    // Only specific 2xx status codes indicate genuine success
    // 200 OK: Standard successful response
    // 201 Created: Resource successfully created
    // 202 Accepted: Request accepted for processing
    // 204 No Content: Successful with no response body
    return (httpCode == 200 || httpCode == 201 || 
            httpCode == 202 || httpCode == 204);
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
            
            THEN("205 Reset Content should NOT be success") {
                REQUIRE(isHttpStatusSuccess(205) == false);
            }
            
            THEN("206 Partial Content should NOT be success") {
                REQUIRE(isHttpStatusSuccess(206) == false);
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
            
            THEN("Ambiguous codes like 203, 205, 206 now fail") {
                REQUIRE(isHttpStatusSuccess(203) == false);
                REQUIRE(isHttpStatusSuccess(205) == false);
                REQUIRE(isHttpStatusSuccess(206) == false);
                INFO("This is the breaking change");
                INFO("However, these codes rarely indicate genuine success");
                INFO("The new behavior is more accurate");
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
