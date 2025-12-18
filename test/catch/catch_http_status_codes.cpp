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
 * This mirrors the logic in wifi.hpp initGatewayInternetHandler()
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
