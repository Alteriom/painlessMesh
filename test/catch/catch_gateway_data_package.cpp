#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "catch_utils.hpp"

#include "painlessmesh/gateway.hpp"

using namespace painlessmesh;
using namespace painlessmesh::gateway;

// Logger for test environment
painlessmesh::logger::LogClass Log;

SCENARIO("GatewayDataPackage has correct defaults") {
    GIVEN("A default GatewayDataPackage") {
        GatewayDataPackage pkg;

        THEN("type should be GATEWAY_DATA (620)") {
            REQUIRE(pkg.type == protocol::GATEWAY_DATA);
            REQUIRE(pkg.type == 620);
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

        THEN("timestamp should be 0") {
            REQUIRE(pkg.timestamp == 0);
        }

        THEN("priority should be NORMAL (2)") {
            REQUIRE(pkg.priority == static_cast<uint8_t>(GatewayPriority::NORMAL));
            REQUIRE(pkg.priority == 2);
        }

        THEN("destination should be empty") {
            REQUIRE(pkg.destination == "");
        }

        THEN("payload should be empty") {
            REQUIRE(pkg.payload == "");
        }

        THEN("contentType should be application/json") {
            REQUIRE(pkg.contentType == "application/json");
        }

        THEN("retryCount should be 0") {
            REQUIRE(pkg.retryCount == 0);
        }

        THEN("requiresAck should be false") {
            REQUIRE(pkg.requiresAck == false);
        }
    }
}

SCENARIO("GatewayDataPackage serialization works correctly") {
    GIVEN("A GatewayDataPackage with test data") {
        GatewayDataPackage pkg;
        pkg.from = 12345;
        pkg.dest = 67890;
        pkg.messageId = 0xABCD1234;
        pkg.originNode = 12345;
        pkg.timestamp = 1609459200;
        pkg.priority = static_cast<uint8_t>(GatewayPriority::HIGH);
        pkg.destination = "https://api.example.com/sensor";
        pkg.payload = "{\"temperature\": 23.5}";
        pkg.contentType = "application/json";
        pkg.retryCount = 2;
        pkg.requiresAck = true;

        REQUIRE(pkg.routing == router::SINGLE);
        REQUIRE(pkg.type == protocol::GATEWAY_DATA);

        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayDataPackage>();

            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.dest == pkg.dest);
                REQUIRE(pkg2.messageId == pkg.messageId);
                REQUIRE(pkg2.originNode == pkg.originNode);
                REQUIRE(pkg2.timestamp == pkg.timestamp);
                REQUIRE(pkg2.priority == pkg.priority);
                REQUIRE(pkg2.destination == pkg.destination);
                REQUIRE(pkg2.payload == pkg.payload);
                REQUIRE(pkg2.contentType == pkg.contentType);
                REQUIRE(pkg2.retryCount == pkg.retryCount);
                REQUIRE(pkg2.requiresAck == pkg.requiresAck);
                REQUIRE(pkg2.routing == pkg.routing);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}

SCENARIO("GatewayDataPackage handles empty strings correctly") {
    GIVEN("A GatewayDataPackage with empty optional fields") {
        GatewayDataPackage pkg;
        pkg.from = 12345;
        pkg.dest = 67890;
        pkg.messageId = 1;
        pkg.originNode = 12345;
        pkg.destination = "";
        pkg.payload = "";
        pkg.contentType = "";

        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayDataPackage>();

            THEN("Empty strings should be preserved") {
                REQUIRE(pkg2.destination == "");
                REQUIRE(pkg2.payload == "");
                REQUIRE(pkg2.contentType == "");
            }
        }
    }
}

SCENARIO("GatewayDataPackage handles large payload") {
    GIVEN("A GatewayDataPackage with a large payload") {
        GatewayDataPackage pkg;
        pkg.from = 12345;
        pkg.dest = 67890;
        pkg.messageId = 1;
        pkg.originNode = 12345;
        
        // Create a payload of ~500 characters
        TSTRING largePayload = "{\"data\": \"";
        for (int i = 0; i < 50; i++) {
            largePayload += "0123456789";
        }
        largePayload += "\"}";
        
        pkg.payload = largePayload;
        pkg.destination = "https://api.example.com/largedata";
        pkg.contentType = "application/json";

        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayDataPackage>();

            THEN("Large payload should be preserved") {
                REQUIRE(pkg2.payload == pkg.payload);
                REQUIRE(pkg2.payload.length() == pkg.payload.length());
            }
        }
    }
}

SCENARIO("GatewayDataPackage generateMessageId works correctly") {
    GIVEN("A node ID") {
        uint32_t nodeId = 0x12345678;

        WHEN("Generating multiple message IDs") {
            uint32_t id1 = GatewayDataPackage::generateMessageId(nodeId);
            uint32_t id2 = GatewayDataPackage::generateMessageId(nodeId);
            uint32_t id3 = GatewayDataPackage::generateMessageId(nodeId);

            THEN("IDs should be unique") {
                REQUIRE(id1 != id2);
                REQUIRE(id2 != id3);
                REQUIRE(id1 != id3);
            }

            THEN("IDs should contain node ID in upper 16 bits") {
                uint16_t nodeIdPart1 = (id1 >> 16) & 0xFFFF;
                uint16_t nodeIdPart2 = (id2 >> 16) & 0xFFFF;
                uint16_t nodeIdPart3 = (id3 >> 16) & 0xFFFF;
                
                REQUIRE(nodeIdPart1 == (nodeId & 0xFFFF));
                REQUIRE(nodeIdPart2 == (nodeId & 0xFFFF));
                REQUIRE(nodeIdPart3 == (nodeId & 0xFFFF));
            }

            THEN("Counter should increment in lower 16 bits") {
                uint16_t counter1 = id1 & 0xFFFF;
                uint16_t counter2 = id2 & 0xFFFF;
                uint16_t counter3 = id3 & 0xFFFF;
                
                // Counters should be sequential (allowing for wrap-around)
                REQUIRE((((counter2 - counter1) == 1) || ((counter2 == 0) && (counter1 == 0xFFFF))));
                REQUIRE((((counter3 - counter2) == 1) || ((counter3 == 0) && (counter2 == 0xFFFF))));
            }
        }
    }

    GIVEN("Different node IDs") {
        uint32_t nodeId1 = 0x11111111;
        uint32_t nodeId2 = 0x22222222;

        WHEN("Generating IDs from different nodes") {
            uint32_t id1 = GatewayDataPackage::generateMessageId(nodeId1);
            uint32_t id2 = GatewayDataPackage::generateMessageId(nodeId2);

            THEN("IDs should have different node ID parts") {
                uint16_t nodeIdPart1 = (id1 >> 16) & 0xFFFF;
                uint16_t nodeIdPart2 = (id2 >> 16) & 0xFFFF;
                
                REQUIRE(nodeIdPart1 != nodeIdPart2);
            }
        }
    }
}

SCENARIO("GatewayDataPackage priority levels are correct") {
    GIVEN("GatewayPriority enum values") {
        THEN("CRITICAL should be 0") {
            REQUIRE(static_cast<uint8_t>(GatewayPriority::CRITICAL) == 0);
        }

        THEN("HIGH should be 1") {
            REQUIRE(static_cast<uint8_t>(GatewayPriority::HIGH) == 1);
        }

        THEN("NORMAL should be 2") {
            REQUIRE(static_cast<uint8_t>(GatewayPriority::NORMAL) == 2);
        }

        THEN("LOW should be 3") {
            REQUIRE(static_cast<uint8_t>(GatewayPriority::LOW) == 3);
        }
    }

    GIVEN("A GatewayDataPackage") {
        GatewayDataPackage pkg;

        WHEN("Setting priority to CRITICAL") {
            pkg.priority = static_cast<uint8_t>(GatewayPriority::CRITICAL);

            THEN("Priority should be 0") {
                REQUIRE(pkg.priority == 0);
            }
        }

        WHEN("Setting priority to LOW") {
            pkg.priority = static_cast<uint8_t>(GatewayPriority::LOW);

            THEN("Priority should be 3") {
                REQUIRE(pkg.priority == 3);
            }
        }
    }
}

SCENARIO("GatewayDataPackage estimatedMemoryFootprint works") {
    GIVEN("A default GatewayDataPackage") {
        GatewayDataPackage pkg;

        WHEN("Calculating memory footprint") {
            size_t footprint = pkg.estimatedMemoryFootprint();

            THEN("Footprint should be at least sizeof(GatewayDataPackage)") {
                REQUIRE(footprint >= sizeof(GatewayDataPackage));
            }
        }
    }

    GIVEN("A GatewayDataPackage with content") {
        GatewayDataPackage pkg;
        pkg.destination = "https://api.example.com/endpoint";
        pkg.payload = "{\"data\": \"test\"}";
        pkg.contentType = "application/json";

        WHEN("Calculating memory footprint") {
            size_t footprint = pkg.estimatedMemoryFootprint();

            THEN("Footprint should increase with string content") {
                size_t expectedMin = sizeof(GatewayDataPackage) +
                                    pkg.destination.length() +
                                    pkg.payload.length() +
                                    pkg.contentType.length();
                REQUIRE(footprint >= expectedMin);
            }
        }
    }
}

SCENARIO("GatewayDataPackage type constant is correctly defined") {
    GIVEN("The protocol::GATEWAY_DATA constant") {
        THEN("It should equal 620") {
            REQUIRE(protocol::GATEWAY_DATA == 620);
        }
    }

    GIVEN("A GatewayDataPackage") {
        GatewayDataPackage pkg;

        THEN("type should match protocol::GATEWAY_DATA") {
            REQUIRE(pkg.type == protocol::GATEWAY_DATA);
        }
    }
}

SCENARIO("GatewayDataPackage requiresAck boolean serialization") {
    GIVEN("A GatewayDataPackage with requiresAck = true") {
        GatewayDataPackage pkg;
        pkg.from = 1;
        pkg.dest = 2;
        pkg.requiresAck = true;

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayDataPackage>();

            THEN("requiresAck should be true") {
                REQUIRE(pkg2.requiresAck == true);
            }
        }
    }

    GIVEN("A GatewayDataPackage with requiresAck = false") {
        GatewayDataPackage pkg;
        pkg.from = 1;
        pkg.dest = 2;
        pkg.requiresAck = false;

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayDataPackage>();

            THEN("requiresAck should be false") {
                REQUIRE(pkg2.requiresAck == false);
            }
        }
    }
}

SCENARIO("GatewayDataPackage with different content types") {
    GIVEN("A GatewayDataPackage with text/plain content") {
        GatewayDataPackage pkg;
        pkg.from = 12345;
        pkg.dest = 67890;
        pkg.contentType = "text/plain";
        pkg.payload = "Hello, World!";

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayDataPackage>();

            THEN("Content type and payload should be preserved") {
                REQUIRE(pkg2.contentType == "text/plain");
                REQUIRE(pkg2.payload == "Hello, World!");
            }
        }
    }

    GIVEN("A GatewayDataPackage with application/octet-stream content") {
        GatewayDataPackage pkg;
        pkg.from = 12345;
        pkg.dest = 67890;
        pkg.contentType = "application/octet-stream";
        pkg.payload = "YmluYXJ5ZGF0YQ==";  // base64 encoded "binarydata"

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayDataPackage>();

            THEN("Content type and payload should be preserved") {
                REQUIRE(pkg2.contentType == "application/octet-stream");
                REQUIRE(pkg2.payload == "YmluYXJ5ZGF0YQ==");
            }
        }
    }
}
