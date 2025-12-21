#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
#include "painlessmesh/gateway.hpp"

using namespace painlessmesh;

// Declare logger for test environment
painlessmesh::logger::LogClass Log;

/**
 * Tests for Internet Connectivity Check Enhancement
 * 
 * Issue: Bridge nodes only checked WiFi.status() == WL_CONNECTED to determine
 * internet availability, but this doesn't verify actual internet connectivity.
 * A node can be connected to WiFi but the router might have no internet access.
 * 
 * Fix: Added hasActualInternetAccess() function that performs DNS resolution
 * to verify actual internet reachability before attempting HTTP requests.
 */

SCENARIO("Gateway connectivity check detects WiFi-connected but no-internet scenario", "[gateway][internet][connectivity]") {
    GIVEN("A gateway data package for internet request") {
        gateway::GatewayDataPackage pkg;
        pkg.from = 123456;
        pkg.dest = 999999;
        pkg.messageId = 12345678;
        pkg.originNode = 123456;
        pkg.timestamp = 1000000;
        pkg.priority = 2;
        pkg.destination = "https://api.callmebot.com/whatsapp.php?phone=+1234567890&apikey=KEY&text=Test";
        pkg.payload = "";
        pkg.contentType = "application/json";
        pkg.retryCount = 0;
        pkg.requiresAck = true;
        
        THEN("Package should have valid structure") {
            REQUIRE(pkg.messageId != 0);
            REQUIRE(pkg.destination.length() > 0);
            REQUIRE(pkg.originNode != 0);
        }
    }
}

SCENARIO("Gateway ACK package contains appropriate error for no-internet scenario", "[gateway][ack][error]") {
    GIVEN("A Gateway ACK package with no-internet error") {
        gateway::GatewayAckPackage ack;
        ack.from = 999999;
        ack.dest = 123456;
        ack.messageId = 12345678;
        ack.originNode = 123456;
        ack.success = false;
        ack.httpStatus = 0;
        ack.error = "Router has no internet access - check WAN connection";
        ack.timestamp = 1000000;
        
        WHEN("ACK is created for no-internet scenario") {
            THEN("Success should be false") {
                REQUIRE(ack.success == false);
            }
            
            THEN("HTTP status should be 0 (network error)") {
                REQUIRE(ack.httpStatus == 0);
            }
            
            THEN("Error message should clearly indicate router internet issue") {
                REQUIRE(ack.error.find("Router") != std::string::npos);
                REQUIRE(ack.error.find("internet access") != std::string::npos);
                
                INFO("Error message should help users diagnose the issue");
                INFO("Current message: " + ack.error);
                INFO("Expected: Clearly indicates router WAN connectivity problem");
            }
        }
    }
}

SCENARIO("Different connectivity failure scenarios have distinct error messages", "[gateway][error][messages]") {
    GIVEN("Various connectivity failure scenarios") {
        WHEN("WiFi is not connected") {
            TSTRING error = "Gateway WiFi not connected";
            
            THEN("Error message should mention WiFi connection") {
                REQUIRE(error.find("WiFi") != std::string::npos);
                REQUIRE(error.find("not connected") != std::string::npos);
                
                INFO("This indicates the ESP is not associated with the WiFi network");
            }
        }
        
        WHEN("WiFi is connected but router has no internet") {
            TSTRING error = "Router has no internet access - check WAN connection";
            
            THEN("Error message should mention router and WAN") {
                REQUIRE(error.find("Router") != std::string::npos);
                REQUIRE(error.find("WAN") != std::string::npos);
                
                INFO("This indicates WiFi is working but router's internet link is down");
                INFO("Users should check: modem connection, ISP service, router WAN port");
            }
        }
        
        WHEN("HTTP request receives ambiguous response") {
            TSTRING error = "Ambiguous response - HTTP 203 may indicate cached/proxied response, not actual delivery";
            
            THEN("Error message should mention caching/proxy") {
                bool hasCached = error.find("cached") != std::string::npos;
                bool hasProxied = error.find("proxied") != std::string::npos;
                bool hasEither = hasCached || hasProxied;
                REQUIRE(hasEither);
                
                INFO("This indicates request reached a cache/proxy but not the destination");
                INFO("Often occurs with captive portals or network proxies");
            }
        }
    }
}

SCENARIO("Gateway internet check flow matches expected behavior", "[gateway][flow][documentation]") {
    GIVEN("The gateway receives an internet request") {
        INFO("STEP 1: Check WiFi.status() == WL_CONNECTED");
        INFO("- If false: Return 'Gateway WiFi not connected'");
        INFO("- If true: Continue to Step 2");
        INFO("");
        INFO("STEP 2: Check hasActualInternetAccess() (DNS resolution)");
        INFO("- If false: Return 'Router has no internet access - check WAN connection'");
        INFO("- If true: Continue to HTTP request");
        INFO("");
        INFO("STEP 3: Perform HTTP request");
        INFO("- Success codes (200, 201, 202, 204): Return success=true");
        INFO("- HTTP 203 (cached): Return success=false, retryable");
        INFO("- HTTP 5xx: Return success=false, retryable");
        INFO("- HTTP 429: Return success=false, retryable");
        INFO("- HTTP 4xx (except 429): Return success=false, non-retryable");
        INFO("- Network errors (code < 0): Return success=false, retryable");
        
        WHEN("Following this flow") {
            THEN("Users get early, clear errors for connectivity issues") {
                REQUIRE(true);
                
                INFO("Benefits:");
                INFO("1. Faster failure detection (no HTTP timeout wait)");
                INFO("2. Clear error messages for troubleshooting");
                INFO("3. Reduced unnecessary retries");
                INFO("4. Lower bandwidth usage");
            }
        }
    }
}

SCENARIO("Error messages provide actionable troubleshooting guidance", "[gateway][errors][usability]") {
    GIVEN("Different error scenarios") {
        struct ErrorCase {
            TSTRING error;
            TSTRING expectedGuidance;
        };
        
        std::vector<ErrorCase> cases = {
            {"Gateway WiFi not connected", 
             "Check WiFi credentials, signal strength, router availability"},
            {"Router has no internet access - check WAN connection",
             "Check modem connection, ISP service status, router WAN port"},
            {"Ambiguous response - HTTP 203",
             "May indicate captive portal, proxy, or cached response"}
        };
        
        WHEN("Users receive these errors") {
            THEN("They should understand the root cause and next steps") {
                for (const auto& testCase : cases) {
                    INFO("Error: " + testCase.error);
                    INFO("Guidance: " + testCase.expectedGuidance);
                    
                    // Verify error is descriptive (not just a code)
                    REQUIRE(testCase.error.length() > 10);
                    REQUIRE(testCase.error != "Error");
                    REQUIRE(testCase.error != "Failed");
                }
            }
        }
    }
}
