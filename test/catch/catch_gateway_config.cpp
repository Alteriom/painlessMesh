#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "painlessmesh/gateway.hpp"

using namespace painlessmesh::gateway;

// Logger for test environment
painlessmesh::logger::LogClass Log;

SCENARIO("SharedGatewayConfig has sensible defaults") {
    GIVEN("A default SharedGatewayConfig") {
        SharedGatewayConfig config;
        
        THEN("enabled should be false by default") {
            REQUIRE(config.enabled == false);
        }
        
        THEN("router credentials should be empty") {
            REQUIRE(config.routerSSID == "");
            REQUIRE(config.routerPassword == "");
        }
        
        THEN("internet check defaults are reasonable") {
            REQUIRE(config.internetCheckInterval == 30000);
            REQUIRE(config.internetCheckHost == "8.8.8.8");
            REQUIRE(config.internetCheckPort == 53);
            REQUIRE(config.internetCheckTimeout == 5000);
        }
        
        THEN("message handling defaults are reasonable") {
            REQUIRE(config.messageRetryCount == 3);
            REQUIRE(config.retryInterval == 1000);
            REQUIRE(config.duplicateTrackingTimeout == 60000);
            REQUIRE(config.maxTrackedMessages == 500);
        }
        
        THEN("gateway coordination defaults are reasonable") {
            REQUIRE(config.gatewayHeartbeatInterval == 15000);
            REQUIRE(config.gatewayFailureTimeout == 45000);
            REQUIRE(config.participateInElection == true);
        }
        
        THEN("advanced configuration defaults are reasonable") {
            REQUIRE(config.relayedMessagePriority == 0);
            REQUIRE(config.maintainPermanentConnection == true);
        }
    }
}

SCENARIO("SharedGatewayConfig validation works correctly when disabled") {
    GIVEN("A disabled SharedGatewayConfig") {
        SharedGatewayConfig config;
        config.enabled = false;
        
        WHEN("validating with empty router credentials") {
            auto result = config.validate();
            
            THEN("validation should pass (nothing to validate when disabled)") {
                REQUIRE(result.valid == true);
                REQUIRE(result.errorMessage == "");
            }
        }
        
        WHEN("validating with invalid settings but disabled") {
            config.internetCheckInterval = 0;  // Invalid
            config.maxTrackedMessages = 0;     // Invalid
            
            auto result = config.validate();
            
            THEN("validation should still pass because feature is disabled") {
                REQUIRE(result.valid == true);
            }
        }
    }
}

SCENARIO("SharedGatewayConfig validation catches missing SSID when enabled") {
    GIVEN("An enabled SharedGatewayConfig without SSID") {
        SharedGatewayConfig config;
        config.enabled = true;
        config.routerSSID = "";
        
        WHEN("validating") {
            auto result = config.validate();
            
            THEN("validation should fail with appropriate message") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage == "routerSSID is required when enabled");
            }
        }
    }
}

SCENARIO("SharedGatewayConfig validation enforces SSID length limits") {
    GIVEN("An enabled SharedGatewayConfig with too long SSID") {
        SharedGatewayConfig config;
        config.enabled = true;
        config.routerSSID = "ThisSSIDIsWayTooLongForWiFiSpec123456";  // > 32 chars
        
        WHEN("validating") {
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("routerSSID exceeds maximum length") != TSTRING::npos);
            }
        }
    }
    
    GIVEN("An enabled SharedGatewayConfig with maximum valid SSID length") {
        SharedGatewayConfig config;
        config.enabled = true;
        config.routerSSID = "12345678901234567890123456789012";  // Exactly 32 chars
        
        WHEN("validating") {
            auto result = config.validate();
            
            THEN("validation should pass") {
                REQUIRE(result.valid == true);
            }
        }
    }
}

SCENARIO("SharedGatewayConfig validation enforces password length limits") {
    GIVEN("An enabled SharedGatewayConfig with too long password") {
        SharedGatewayConfig config;
        config.enabled = true;
        config.routerSSID = "ValidSSID";
        // Create a password longer than 63 characters
        config.routerPassword = "ThisPasswordIsWayTooLongForWPA2AndShouldFailValidation1234567890ABCDEF";
        
        WHEN("validating") {
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("routerPassword exceeds maximum length") != TSTRING::npos);
            }
        }
    }
    
    GIVEN("An enabled SharedGatewayConfig with empty password (open network)") {
        SharedGatewayConfig config;
        config.enabled = true;
        config.routerSSID = "OpenNetwork";
        config.routerPassword = "";
        
        WHEN("validating") {
            auto result = config.validate();
            
            THEN("validation should pass (empty password is allowed)") {
                REQUIRE(result.valid == true);
            }
        }
    }
}

SCENARIO("SharedGatewayConfig validation enforces interval constraints") {
    GIVEN("An enabled SharedGatewayConfig with valid router credentials") {
        SharedGatewayConfig config;
        config.enabled = true;
        config.routerSSID = "TestNetwork";
        config.routerPassword = "TestPassword";
        
        WHEN("internetCheckInterval is too low") {
            config.internetCheckInterval = 500;  // < 1000ms
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("internetCheckInterval") != TSTRING::npos);
            }
        }
        
        WHEN("internetCheckTimeout is too low") {
            config.internetCheckTimeout = 50;  // < 100ms
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("internetCheckTimeout") != TSTRING::npos);
            }
        }
        
        WHEN("internetCheckTimeout >= internetCheckInterval") {
            config.internetCheckInterval = 5000;
            config.internetCheckTimeout = 5000;  // Should be less than interval
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("internetCheckTimeout must be less than") != TSTRING::npos);
            }
        }
        
        WHEN("gatewayHeartbeatInterval is too low") {
            config.gatewayHeartbeatInterval = 500;  // < 1000ms
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("gatewayHeartbeatInterval") != TSTRING::npos);
            }
        }
        
        WHEN("gatewayFailureTimeout is less than 2x heartbeat") {
            config.gatewayHeartbeatInterval = 15000;
            config.gatewayFailureTimeout = 20000;  // Should be >= 30000
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("gatewayFailureTimeout should be at least 2x") != TSTRING::npos);
            }
        }
        
        WHEN("duplicateTrackingTimeout is too low") {
            config.duplicateTrackingTimeout = 500;  // < 1000ms
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("duplicateTrackingTimeout") != TSTRING::npos);
            }
        }
        
        WHEN("retryInterval is too low") {
            config.retryInterval = 50;  // < 100ms
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("retryInterval") != TSTRING::npos);
            }
        }
    }
}

SCENARIO("SharedGatewayConfig validation enforces count constraints") {
    GIVEN("An enabled SharedGatewayConfig with valid router credentials") {
        SharedGatewayConfig config;
        config.enabled = true;
        config.routerSSID = "TestNetwork";
        config.routerPassword = "TestPassword";
        
        WHEN("maxTrackedMessages is too low") {
            config.maxTrackedMessages = 5;  // < 10
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("maxTrackedMessages") != TSTRING::npos);
            }
        }
        
        WHEN("maxTrackedMessages is at minimum") {
            config.maxTrackedMessages = 10;
            auto result = config.validate();
            
            THEN("validation should pass") {
                REQUIRE(result.valid == true);
            }
        }
    }
}

SCENARIO("SharedGatewayConfig validation enforces non-empty internet check host") {
    GIVEN("An enabled SharedGatewayConfig with empty internet check host") {
        SharedGatewayConfig config;
        config.enabled = true;
        config.routerSSID = "TestNetwork";
        config.internetCheckHost = "";
        
        WHEN("validating") {
            auto result = config.validate();
            
            THEN("validation should fail") {
                REQUIRE(result.valid == false);
                REQUIRE(result.errorMessage.find("internetCheckHost cannot be empty") != TSTRING::npos);
            }
        }
    }
}

SCENARIO("SharedGatewayConfig passes validation with all valid settings") {
    GIVEN("A fully configured valid SharedGatewayConfig") {
        SharedGatewayConfig config;
        config.enabled = true;
        config.routerSSID = "MyHomeNetwork";
        config.routerPassword = "SecurePassword123";
        config.internetCheckInterval = 30000;
        config.internetCheckHost = "1.1.1.1";  // Cloudflare DNS
        config.internetCheckPort = 53;
        config.internetCheckTimeout = 5000;
        config.messageRetryCount = 5;
        config.retryInterval = 2000;
        config.duplicateTrackingTimeout = 120000;
        config.maxTrackedMessages = 1000;
        config.gatewayHeartbeatInterval = 10000;
        config.gatewayFailureTimeout = 30000;  // >= 2x heartbeat
        config.participateInElection = true;
        config.relayedMessagePriority = 1;
        config.maintainPermanentConnection = false;
        
        WHEN("validating") {
            auto result = config.validate();
            
            THEN("validation should pass") {
                REQUIRE(result.valid == true);
                REQUIRE(result.errorMessage == "");
            }
        }
    }
}

SCENARIO("SharedGatewayConfig helper methods work correctly") {
    GIVEN("A SharedGatewayConfig with router credentials") {
        SharedGatewayConfig config;
        config.routerSSID = "TestNetwork";
        config.routerPassword = "TestPassword";
        
        WHEN("checking hasRouterCredentials") {
            THEN("should return true") {
                REQUIRE(config.hasRouterCredentials() == true);
            }
        }
        
        WHEN("participateInElection is true") {
            config.participateInElection = true;
            
            THEN("canParticipateInElection should return true") {
                REQUIRE(config.canParticipateInElection() == true);
            }
        }
        
        WHEN("participateInElection is false") {
            config.participateInElection = false;
            
            THEN("canParticipateInElection should return false") {
                REQUIRE(config.canParticipateInElection() == false);
            }
        }
    }
    
    GIVEN("A SharedGatewayConfig without router credentials") {
        SharedGatewayConfig config;
        config.routerSSID = "";
        
        WHEN("checking hasRouterCredentials") {
            THEN("should return false") {
                REQUIRE(config.hasRouterCredentials() == false);
            }
        }
        
        WHEN("checking canParticipateInElection") {
            config.participateInElection = true;
            
            THEN("should return false (no credentials)") {
                REQUIRE(config.canParticipateInElection() == false);
            }
        }
    }
}

SCENARIO("SharedGatewayConfig memory footprint estimation") {
    GIVEN("A default SharedGatewayConfig") {
        SharedGatewayConfig config;
        
        WHEN("estimating memory footprint") {
            size_t footprint = config.estimatedMemoryFootprint();
            
            THEN("footprint should be reasonable") {
                // Base struct size + default string content
                REQUIRE(footprint >= sizeof(SharedGatewayConfig));
                REQUIRE(footprint < 1000);  // Should be well under 1KB
            }
        }
    }
    
    GIVEN("A SharedGatewayConfig with long strings") {
        SharedGatewayConfig config;
        config.routerSSID = "12345678901234567890123456789012";  // 32 chars
        config.routerPassword = "123456789012345678901234567890123456789012345678901234567890123";  // 63 chars
        config.internetCheckHost = "my.custom.ntp.server.example.com";
        
        WHEN("estimating memory footprint") {
            size_t footprint = config.estimatedMemoryFootprint();
            
            THEN("footprint should increase with string content") {
                REQUIRE(footprint >= sizeof(SharedGatewayConfig) + 32 + 63 + 32);
            }
        }
    }
}

SCENARIO("ValidationResult works correctly") {
    GIVEN("A valid ValidationResult") {
        ValidationResult result(true, "");
        
        THEN("bool conversion should return true") {
            REQUIRE(static_cast<bool>(result) == true);
            REQUIRE(result.valid == true);
            REQUIRE(result.errorMessage == "");
        }
    }
    
    GIVEN("An invalid ValidationResult with error message") {
        ValidationResult result(false, "Something went wrong");
        
        THEN("bool conversion should return false") {
            REQUIRE(static_cast<bool>(result) == false);
            REQUIRE(result.valid == false);
            REQUIRE(result.errorMessage == "Something went wrong");
        }
    }
    
    GIVEN("A default ValidationResult") {
        ValidationResult result;
        
        THEN("should be valid by default") {
            REQUIRE(result.valid == true);
            REQUIRE(result.errorMessage == "");
        }
    }
}

SCENARIO("SharedGatewayConfig edge case validation") {
    GIVEN("An enabled SharedGatewayConfig") {
        SharedGatewayConfig config;
        config.enabled = true;
        config.routerSSID = "TestNetwork";
        
        WHEN("all intervals are at minimum valid values") {
            config.internetCheckInterval = 1000;
            config.internetCheckTimeout = 100;
            config.gatewayHeartbeatInterval = 1000;
            config.gatewayFailureTimeout = 2000;
            config.duplicateTrackingTimeout = 1000;
            config.retryInterval = 100;
            config.maxTrackedMessages = 10;
            
            auto result = config.validate();
            
            THEN("validation should pass") {
                REQUIRE(result.valid == true);
            }
        }
        
        WHEN("failure timeout is exactly 2x heartbeat interval") {
            config.gatewayHeartbeatInterval = 15000;
            config.gatewayFailureTimeout = 30000;
            
            auto result = config.validate();
            
            THEN("validation should pass") {
                REQUIRE(result.valid == true);
            }
        }
    }
}
