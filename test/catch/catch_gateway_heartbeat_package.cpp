#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "catch_utils.hpp"

#include "painlessmesh/gateway.hpp"

using namespace painlessmesh;
using namespace painlessmesh::gateway;

// Logger for test environment
painlessmesh::logger::LogClass Log;

SCENARIO("GatewayHeartbeatPackage has correct defaults") {
    GIVEN("A default GatewayHeartbeatPackage") {
        GatewayHeartbeatPackage pkg;

        THEN("type should be GATEWAY_HEARTBEAT (622)") {
            REQUIRE(pkg.type == protocol::GATEWAY_HEARTBEAT);
            REQUIRE(pkg.type == 622);
        }

        THEN("routing should be BROADCAST") {
            REQUIRE(pkg.routing == router::BROADCAST);
        }

        THEN("isPrimary should be false") {
            REQUIRE(pkg.isPrimary == false);
        }

        THEN("hasInternet should be false") {
            REQUIRE(pkg.hasInternet == false);
        }

        THEN("routerRSSI should be 0") {
            REQUIRE(pkg.routerRSSI == 0);
        }

        THEN("uptime should be 0") {
            REQUIRE(pkg.uptime == 0);
        }

        THEN("timestamp should be 0") {
            REQUIRE(pkg.timestamp == 0);
        }
    }
}

SCENARIO("GatewayHeartbeatPackage serialization works correctly") {
    GIVEN("A GatewayHeartbeatPackage with test data") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 12345;
        pkg.isPrimary = true;
        pkg.hasInternet = true;
        pkg.routerRSSI = -42;
        pkg.uptime = 86400;  // 1 day in seconds
        pkg.timestamp = 1609459200;

        REQUIRE(pkg.routing == router::BROADCAST);
        REQUIRE(pkg.type == protocol::GATEWAY_HEARTBEAT);

        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.isPrimary == pkg.isPrimary);
                REQUIRE(pkg2.hasInternet == pkg.hasInternet);
                REQUIRE(pkg2.routerRSSI == pkg.routerRSSI);
                REQUIRE(pkg2.uptime == pkg.uptime);
                REQUIRE(pkg2.timestamp == pkg.timestamp);
                REQUIRE(pkg2.routing == pkg.routing);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}

SCENARIO("GatewayHeartbeatPackage handles boolean states correctly") {
    GIVEN("A GatewayHeartbeatPackage with isPrimary = true, hasInternet = true") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 11111;
        pkg.isPrimary = true;
        pkg.hasInternet = true;

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("Both booleans should be true") {
                REQUIRE(pkg2.isPrimary == true);
                REQUIRE(pkg2.hasInternet == true);
            }
        }
    }

    GIVEN("A GatewayHeartbeatPackage with isPrimary = false, hasInternet = true") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 22222;
        pkg.isPrimary = false;
        pkg.hasInternet = true;

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("isPrimary should be false, hasInternet should be true") {
                REQUIRE(pkg2.isPrimary == false);
                REQUIRE(pkg2.hasInternet == true);
            }
        }
    }

    GIVEN("A GatewayHeartbeatPackage with isPrimary = true, hasInternet = false") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 33333;
        pkg.isPrimary = true;
        pkg.hasInternet = false;

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("isPrimary should be true, hasInternet should be false") {
                REQUIRE(pkg2.isPrimary == true);
                REQUIRE(pkg2.hasInternet == false);
            }
        }
    }

    GIVEN("A GatewayHeartbeatPackage with both booleans false") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 44444;
        pkg.isPrimary = false;
        pkg.hasInternet = false;

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("Both booleans should be false") {
                REQUIRE(pkg2.isPrimary == false);
                REQUIRE(pkg2.hasInternet == false);
            }
        }
    }
}

SCENARIO("GatewayHeartbeatPackage handles RSSI values correctly") {
    GIVEN("A GatewayHeartbeatPackage with strong signal") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 55555;
        pkg.routerRSSI = -35;  // Very strong signal

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("Strong RSSI should be preserved") {
                REQUIRE(pkg2.routerRSSI == -35);
            }
        }
    }

    GIVEN("A GatewayHeartbeatPackage with weak signal") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 66666;
        pkg.routerRSSI = -85;  // Weak signal

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("Weak RSSI should be preserved") {
                REQUIRE(pkg2.routerRSSI == -85);
            }
        }
    }

    GIVEN("A GatewayHeartbeatPackage with worst possible signal") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 77777;
        pkg.routerRSSI = -127;  // Worst possible RSSI for int8_t

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("Worst RSSI should be preserved") {
                REQUIRE(pkg2.routerRSSI == -127);
            }
        }
    }

    GIVEN("A GatewayHeartbeatPackage with zero RSSI (not connected)") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 88888;
        pkg.routerRSSI = 0;  // Not connected

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("Zero RSSI should be preserved") {
                REQUIRE(pkg2.routerRSSI == 0);
            }
        }
    }
}

SCENARIO("GatewayHeartbeatPackage handles edge case uptime values") {
    GIVEN("A GatewayHeartbeatPackage with large uptime") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 11111;
        // ~68 years in seconds - realistic maximum for embedded device
        // Note: ArduinoJson may not correctly handle values > INT32_MAX
        pkg.uptime = 0x7FFFFFFF;

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("Large uptime should be preserved") {
                REQUIRE(pkg2.uptime == 0x7FFFFFFF);
            }
        }
    }

    GIVEN("A GatewayHeartbeatPackage with zero uptime") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 22222;
        pkg.uptime = 0;

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("Zero uptime should be preserved") {
                REQUIRE(pkg2.uptime == 0);
            }
        }
    }

    GIVEN("A GatewayHeartbeatPackage with typical uptime (1 day)") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 33333;
        pkg.uptime = 86400;  // 1 day in seconds

        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<GatewayHeartbeatPackage>();

            THEN("1 day uptime should be preserved") {
                REQUIRE(pkg2.uptime == 86400);
            }
        }
    }
}

SCENARIO("GatewayHeartbeatPackage isHealthy() method works correctly") {
    GIVEN("A GatewayHeartbeatPackage that is primary with Internet") {
        GatewayHeartbeatPackage pkg;
        pkg.isPrimary = true;
        pkg.hasInternet = true;

        THEN("isHealthy() should return true") {
            REQUIRE(pkg.isHealthy() == true);
        }
    }

    GIVEN("A GatewayHeartbeatPackage that is primary without Internet") {
        GatewayHeartbeatPackage pkg;
        pkg.isPrimary = true;
        pkg.hasInternet = false;

        THEN("isHealthy() should return false") {
            REQUIRE(pkg.isHealthy() == false);
        }
    }

    GIVEN("A GatewayHeartbeatPackage that is not primary with Internet") {
        GatewayHeartbeatPackage pkg;
        pkg.isPrimary = false;
        pkg.hasInternet = true;

        THEN("isHealthy() should return false") {
            REQUIRE(pkg.isHealthy() == false);
        }
    }

    GIVEN("A GatewayHeartbeatPackage that is not primary without Internet") {
        GatewayHeartbeatPackage pkg;
        pkg.isPrimary = false;
        pkg.hasInternet = false;

        THEN("isHealthy() should return false") {
            REQUIRE(pkg.isHealthy() == false);
        }
    }
}

SCENARIO("GatewayHeartbeatPackage hasAcceptableSignal() method works correctly") {
    GIVEN("A GatewayHeartbeatPackage with excellent signal") {
        GatewayHeartbeatPackage pkg;
        pkg.routerRSSI = -35;

        THEN("hasAcceptableSignal() should return true") {
            REQUIRE(pkg.hasAcceptableSignal() == true);
        }
    }

    GIVEN("A GatewayHeartbeatPackage with good signal") {
        GatewayHeartbeatPackage pkg;
        pkg.routerRSSI = -50;

        THEN("hasAcceptableSignal() should return true") {
            REQUIRE(pkg.hasAcceptableSignal() == true);
        }
    }

    GIVEN("A GatewayHeartbeatPackage with acceptable signal at threshold") {
        GatewayHeartbeatPackage pkg;
        pkg.routerRSSI = -69;  // Just above threshold

        THEN("hasAcceptableSignal() should return true") {
            REQUIRE(pkg.hasAcceptableSignal() == true);
        }
    }

    GIVEN("A GatewayHeartbeatPackage with signal at threshold boundary") {
        GatewayHeartbeatPackage pkg;
        pkg.routerRSSI = -70;  // At threshold

        THEN("hasAcceptableSignal() should return false") {
            REQUIRE(pkg.hasAcceptableSignal() == false);
        }
    }

    GIVEN("A GatewayHeartbeatPackage with weak signal") {
        GatewayHeartbeatPackage pkg;
        pkg.routerRSSI = -85;

        THEN("hasAcceptableSignal() should return false") {
            REQUIRE(pkg.hasAcceptableSignal() == false);
        }
    }

    GIVEN("A GatewayHeartbeatPackage with zero RSSI (not connected)") {
        GatewayHeartbeatPackage pkg;
        pkg.routerRSSI = 0;

        THEN("hasAcceptableSignal() should return false") {
            REQUIRE(pkg.hasAcceptableSignal() == false);
        }
    }
}

SCENARIO("GatewayHeartbeatPackage estimatedMemoryFootprint works") {
    GIVEN("A default GatewayHeartbeatPackage") {
        GatewayHeartbeatPackage pkg;

        WHEN("Calculating memory footprint") {
            size_t footprint = pkg.estimatedMemoryFootprint();

            THEN("Footprint should equal sizeof(GatewayHeartbeatPackage)") {
                REQUIRE(footprint == sizeof(GatewayHeartbeatPackage));
            }
        }
    }

    GIVEN("A GatewayHeartbeatPackage with data") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 12345;
        pkg.isPrimary = true;
        pkg.hasInternet = true;
        pkg.routerRSSI = -42;
        pkg.uptime = 86400;
        pkg.timestamp = 1609459200;

        WHEN("Calculating memory footprint") {
            size_t footprint = pkg.estimatedMemoryFootprint();

            THEN("Footprint should still equal sizeof (no dynamic memory)") {
                REQUIRE(footprint == sizeof(GatewayHeartbeatPackage));
            }
        }
    }
}

SCENARIO("GatewayHeartbeatPackage type constant is correctly defined") {
    GIVEN("The protocol::GATEWAY_HEARTBEAT constant") {
        THEN("It should equal 622") {
            REQUIRE(protocol::GATEWAY_HEARTBEAT == 622);
        }
    }

    GIVEN("A GatewayHeartbeatPackage") {
        GatewayHeartbeatPackage pkg;

        THEN("type should match protocol::GATEWAY_HEARTBEAT") {
            REQUIRE(pkg.type == protocol::GATEWAY_HEARTBEAT);
        }
    }
}

SCENARIO("GatewayHeartbeatPackage comprehensive serialization test") {
    GIVEN("A GatewayHeartbeatPackage representing a healthy primary gateway") {
        GatewayHeartbeatPackage pkg;
        pkg.from = 98765;
        pkg.isPrimary = true;
        pkg.hasInternet = true;
        pkg.routerRSSI = -45;
        pkg.uptime = 172800;  // 2 days
        pkg.timestamp = 1699564800;

        WHEN("Converting it to and from Variant multiple times") {
            auto var1 = protocol::Variant(&pkg);
            auto pkg2 = var1.to<GatewayHeartbeatPackage>();
            
            auto var2 = protocol::Variant(&pkg2);
            auto pkg3 = var2.to<GatewayHeartbeatPackage>();

            THEN("Should maintain values through multiple conversions") {
                REQUIRE(pkg3.from == pkg.from);
                REQUIRE(pkg3.isPrimary == pkg.isPrimary);
                REQUIRE(pkg3.hasInternet == pkg.hasInternet);
                REQUIRE(pkg3.routerRSSI == pkg.routerRSSI);
                REQUIRE(pkg3.uptime == pkg.uptime);
                REQUIRE(pkg3.timestamp == pkg.timestamp);
                REQUIRE(pkg3.type == pkg.type);
                REQUIRE(pkg3.routing == pkg.routing);
            }

            THEN("Helper methods should work correctly") {
                REQUIRE(pkg3.isHealthy() == true);
                REQUIRE(pkg3.hasAcceptableSignal() == true);
            }
        }
    }
}

SCENARIO("GatewayHeartbeatPackage can be compared for election") {
    GIVEN("Two GatewayHeartbeatPackages from different gateways") {
        GatewayHeartbeatPackage pkg1;
        pkg1.from = 11111;
        pkg1.isPrimary = true;
        pkg1.hasInternet = true;
        pkg1.routerRSSI = -65;
        pkg1.uptime = 86400;  // 1 day

        GatewayHeartbeatPackage pkg2;
        pkg2.from = 22222;
        pkg2.isPrimary = true;
        pkg2.hasInternet = true;
        pkg2.routerRSSI = -42;  // Better signal
        pkg2.uptime = 43200;   // Less uptime

        THEN("pkg2 should have better RSSI") {
            REQUIRE(pkg2.routerRSSI > pkg1.routerRSSI);
        }

        THEN("pkg1 should have longer uptime") {
            REQUIRE(pkg1.uptime > pkg2.uptime);
        }

        THEN("Both should be healthy") {
            REQUIRE(pkg1.isHealthy() == true);
            REQUIRE(pkg2.isHealthy() == true);
        }

        THEN("pkg2 should have acceptable signal") {
            REQUIRE(pkg2.hasAcceptableSignal() == true);
        }

        THEN("pkg1 should have acceptable signal") {
            REQUIRE(pkg1.hasAcceptableSignal() == true);
        }
    }
}
