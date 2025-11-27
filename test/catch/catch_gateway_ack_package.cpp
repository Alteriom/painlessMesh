#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "catch_utils.hpp"

#include "painlessmesh/gateway.hpp"

using namespace painlessmesh;
using namespace painlessmesh::gateway;

// Logger for test environment
painlessmesh::logger::LogClass Log;

SCENARIO("GatewayAckPackage has correct defaults") {
    GIVEN("A default GatewayAckPackage") {
        GatewayAckPackage pkg;

        THEN("type should be GATEWAY_ACK (621)") {
            REQUIRE(pkg.type == protocol::GATEWAY_ACK);
            REQUIRE(pkg.type == 621);
        }

        THEN("routing should be SINGLE") {
            REQUIRE(pkg.routing == router::SINGLE);
        }

        THEN("messageId should be 0") {
            REQUIRE(pkg.messageId == 0);
        }

        THEN("originNode should be 0") {
            REQUIRE(pkg.originNode == 0);
        }

        THEN("success should be false") {
            REQUIRE(pkg.success == false);
        }

        THEN("httpStatus should be 0") {
            REQUIRE(pkg.httpStatus == 0);
        }

        THEN("error should be empty") {
            REQUIRE(pkg.error == "");
        }

        THEN("timestamp should be 0") {
            REQUIRE(pkg.timestamp == 0);
        }
    }
}

SCENARIO("GatewayAckPackage serialization works correctly") {
    GIVEN("A GatewayAckPackage with success test data") {
        GatewayAckPackage pkg;
        pkg.from = 12345;
        pkg.dest = 67890;
        pkg.messageId = 0xABCD1234;
        pkg.originNode = 12345;
        pkg.success = true;
        pkg.httpStatus = 200;
        pkg.error = "";
        pkg.timestamp = 1609459200;

        REQUIRE(pkg.routing == router::SINGLE);
        REQUIRE(pkg.type == protocol::GATEWAY_ACK);

        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayAckPackage>();

            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.dest == pkg.dest);
                REQUIRE(pkg2.messageId == pkg.messageId);
                REQUIRE(pkg2.originNode == pkg.originNode);
                REQUIRE(pkg2.success == pkg.success);
                REQUIRE(pkg2.httpStatus == pkg.httpStatus);
                REQUIRE(pkg2.error == pkg.error);
                REQUIRE(pkg2.timestamp == pkg.timestamp);
                REQUIRE(pkg2.routing == pkg.routing);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}

SCENARIO("GatewayAckPackage failure scenario serialization") {
    GIVEN("A GatewayAckPackage with failure data") {
        GatewayAckPackage pkg;
        pkg.from = 12345;
        pkg.dest = 67890;
        pkg.messageId = 0xDEADBEEF;
        pkg.originNode = 12345;
        pkg.success = false;
        pkg.httpStatus = 500;
        pkg.error = "Connection timeout";
        pkg.timestamp = 1609459300;

        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayAckPackage>();

            THEN("Should result in the same values including error message") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.dest == pkg.dest);
                REQUIRE(pkg2.messageId == pkg.messageId);
                REQUIRE(pkg2.originNode == pkg.originNode);
                REQUIRE(pkg2.success == pkg.success);
                REQUIRE(pkg2.httpStatus == pkg.httpStatus);
                REQUIRE(pkg2.error == pkg.error);
                REQUIRE(pkg2.timestamp == pkg.timestamp);
                REQUIRE(pkg2.routing == pkg.routing);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}

SCENARIO("GatewayAckPackage handles empty error string correctly") {
    GIVEN("A GatewayAckPackage with success=true and empty error") {
        GatewayAckPackage pkg;
        pkg.from = 12345;
        pkg.dest = 67890;
        pkg.messageId = 1;
        pkg.originNode = 12345;
        pkg.success = true;
        pkg.error = "";

        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayAckPackage>();

            THEN("Empty string should be preserved") {
                REQUIRE(pkg2.error == "");
            }
        }
    }
}

SCENARIO("GatewayAckPackage handles long error messages") {
    GIVEN("A GatewayAckPackage with a long error message") {
        GatewayAckPackage pkg;
        pkg.from = 12345;
        pkg.dest = 67890;
        pkg.messageId = 1;
        pkg.originNode = 12345;
        pkg.success = false;
        pkg.httpStatus = 503;
        
        // Create an error message of ~200 characters
        TSTRING longError = "Service temporarily unavailable: The server is currently unable to handle the request due to temporary overloading or maintenance. This is a temporary condition which will be resolved soon.";
        
        pkg.error = longError;

        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayAckPackage>();

            THEN("Long error message should be preserved") {
                REQUIRE(pkg2.error == pkg.error);
                REQUIRE(pkg2.error.length() == pkg.error.length());
            }
        }
    }
}

SCENARIO("GatewayAckPackage boolean success serialization") {
    GIVEN("A GatewayAckPackage with success = true") {
        GatewayAckPackage pkg;
        pkg.from = 1;
        pkg.dest = 2;
        pkg.success = true;

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayAckPackage>();

            THEN("success should be true") {
                REQUIRE(pkg2.success == true);
            }
        }
    }

    GIVEN("A GatewayAckPackage with success = false") {
        GatewayAckPackage pkg;
        pkg.from = 1;
        pkg.dest = 2;
        pkg.success = false;

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayAckPackage>();

            THEN("success should be false") {
                REQUIRE(pkg2.success == false);
            }
        }
    }
}

SCENARIO("GatewayAckPackage type constant is correctly defined") {
    GIVEN("The protocol::GATEWAY_ACK constant") {
        THEN("It should equal 621") {
            REQUIRE(protocol::GATEWAY_ACK == 621);
        }
    }

    GIVEN("A GatewayAckPackage") {
        GatewayAckPackage pkg;

        THEN("type should match protocol::GATEWAY_ACK") {
            REQUIRE(pkg.type == protocol::GATEWAY_ACK);
        }
    }
}

SCENARIO("GatewayAckPackage estimatedMemoryFootprint works") {
    GIVEN("A default GatewayAckPackage") {
        GatewayAckPackage pkg;

        WHEN("Calculating memory footprint") {
            size_t footprint = pkg.estimatedMemoryFootprint();

            THEN("Footprint should be at least sizeof(GatewayAckPackage)") {
                REQUIRE(footprint >= sizeof(GatewayAckPackage));
            }
        }
    }

    GIVEN("A GatewayAckPackage with error message content") {
        GatewayAckPackage pkg;
        pkg.error = "Connection refused: Host unreachable";

        WHEN("Calculating memory footprint") {
            size_t footprint = pkg.estimatedMemoryFootprint();

            THEN("Footprint should increase with string content") {
                size_t expectedMin = sizeof(GatewayAckPackage) +
                                    pkg.error.length();
                REQUIRE(footprint >= expectedMin);
            }
        }
    }
}
