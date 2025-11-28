#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

// Declare logger for test environment
painlessmesh::logger::LogClass Log;

// Test data conventions (consistent with other test files in repo):
// - Test node IDs: 12345 (self), 99999, 11111, 22222, 33333 (other nodes)
// - RSSI values: -45 (strong), -50 (good), -55 (medium), -60/-70 (weak), -127 (min)
// - Router channel: 6 (standard)
// - Uptime: 10000 ms (typical)
// - Gateway IP: "192.168.1.1" (standard)
// - Timestamp: 1000 (arbitrary)

// ==================== isPrimaryGateway Tests ====================

SCENARIO("isPrimaryGateway returns false when no bridges exist", "[gateway][isPrimaryGateway]") {
    GIVEN("A mesh instance with no bridges") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Checking if this node is primary gateway") {
            bool isPrimary = mesh.isPrimaryGateway();
            
            THEN("Should return false") {
                REQUIRE(isPrimary == false);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("isPrimaryGateway returns false when bridges exist but this node is not primary", "[gateway][isPrimaryGateway]") {
    GIVEN("A mesh instance with a bridge that is not this node") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add a bridge with different node ID
        mesh.updateBridgeStatus(99999, true, -50, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Checking if this node is primary gateway") {
            bool isPrimary = mesh.isPrimaryGateway();
            
            THEN("Should return false since this node is not the bridge") {
                REQUIRE(isPrimary == false);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("isPrimaryGateway returns true when this node is the primary gateway", "[gateway][isPrimaryGateway]") {
    GIVEN("A mesh instance where this node is registered as a bridge") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        uint32_t myNodeId = 12345;
        mesh.init(&scheduler, myNodeId);
        
        // Register this node as a bridge with Internet
        mesh.updateBridgeStatus(myNodeId, true, -45, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Checking if this node is primary gateway") {
            bool isPrimary = mesh.isPrimaryGateway();
            
            THEN("Should return true") {
                REQUIRE(isPrimary == true);
            }
        }
        
        mesh.stop();
    }
}

// ==================== getPrimaryGateway Tests ====================

SCENARIO("getPrimaryGateway returns 0 when no bridges exist", "[gateway][getPrimaryGateway]") {
    GIVEN("A mesh instance with no bridges") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Getting the primary gateway") {
            uint32_t gatewayId = mesh.getPrimaryGateway();
            
            THEN("Should return 0") {
                REQUIRE(gatewayId == 0);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getPrimaryGateway returns the correct node ID when a primary bridge exists", "[gateway][getPrimaryGateway]") {
    GIVEN("A mesh instance with a single bridge") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add a bridge with Internet
        uint32_t bridgeId = 99999;
        mesh.updateBridgeStatus(bridgeId, true, -50, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Getting the primary gateway") {
            uint32_t gatewayId = mesh.getPrimaryGateway();
            
            THEN("Should return the bridge node ID") {
                REQUIRE(gatewayId == bridgeId);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getPrimaryGateway returns the best (highest RSSI) bridge when multiple exist", "[gateway][getPrimaryGateway]") {
    GIVEN("A mesh instance with multiple bridges") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add multiple bridges with different RSSI values
        // Bridge with weaker signal
        mesh.updateBridgeStatus(11111, true, -70, 6, 10000, "192.168.1.1", 1000);
        // Bridge with strongest signal (should be selected as primary)
        mesh.updateBridgeStatus(22222, true, -45, 6, 10000, "192.168.1.1", 1000);
        // Bridge with medium signal
        mesh.updateBridgeStatus(33333, true, -55, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Getting the primary gateway") {
            uint32_t gatewayId = mesh.getPrimaryGateway();
            
            THEN("Should return the bridge with best RSSI (22222)") {
                REQUIRE(gatewayId == 22222);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getPrimaryGateway returns 0 when bridges exist but none have Internet", "[gateway][getPrimaryGateway]") {
    GIVEN("A mesh instance with bridges that have no Internet") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add bridges without Internet connectivity
        mesh.updateBridgeStatus(11111, false, -50, 6, 10000, "192.168.1.1", 1000);
        mesh.updateBridgeStatus(22222, false, -45, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Getting the primary gateway") {
            uint32_t gatewayId = mesh.getPrimaryGateway();
            
            THEN("Should return 0 since no bridge has Internet") {
                REQUIRE(gatewayId == 0);
            }
        }
        
        mesh.stop();
    }
}

// ==================== getGateways Tests ====================

SCENARIO("getGateways returns empty list when no bridges exist", "[gateway][getGateways]") {
    GIVEN("A mesh instance with no bridges") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Getting the list of gateways") {
            auto gateways = mesh.getGateways();
            
            THEN("Should return empty list") {
                REQUIRE(gateways.size() == 0);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getGateways returns list of all bridges with Internet access", "[gateway][getGateways]") {
    GIVEN("A mesh instance with multiple bridges with Internet") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add multiple bridges with Internet
        mesh.updateBridgeStatus(11111, true, -50, 6, 10000, "192.168.1.1", 1000);
        mesh.updateBridgeStatus(22222, true, -45, 6, 10000, "192.168.1.1", 1000);
        mesh.updateBridgeStatus(33333, true, -55, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Getting the list of gateways") {
            auto gateways = mesh.getGateways();
            
            THEN("Should return list with all 3 bridges") {
                REQUIRE(gateways.size() == 3);
                
                // Check that all bridge IDs are in the list
                bool found11111 = false;
                bool found22222 = false;
                bool found33333 = false;
                
                for (auto id : gateways) {
                    if (id == 11111) found11111 = true;
                    if (id == 22222) found22222 = true;
                    if (id == 33333) found33333 = true;
                }
                
                REQUIRE(found11111 == true);
                REQUIRE(found22222 == true);
                REQUIRE(found33333 == true);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getGateways excludes bridges without Internet connection", "[gateway][getGateways]") {
    GIVEN("A mesh instance with some bridges having Internet and some not") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add bridges - some with Internet, some without
        mesh.updateBridgeStatus(11111, true, -50, 6, 10000, "192.168.1.1", 1000);  // Has Internet
        mesh.updateBridgeStatus(22222, false, -45, 6, 10000, "192.168.1.1", 1000); // No Internet
        mesh.updateBridgeStatus(33333, true, -55, 6, 10000, "192.168.1.1", 1000);  // Has Internet
        
        WHEN("Getting the list of gateways") {
            auto gateways = mesh.getGateways();
            
            THEN("Should only return bridges with Internet (2 bridges)") {
                REQUIRE(gateways.size() == 2);
                
                // Check that only bridges with Internet are in the list
                bool found11111 = false;
                bool found22222 = false;
                bool found33333 = false;
                
                for (auto id : gateways) {
                    if (id == 11111) found11111 = true;
                    if (id == 22222) found22222 = true;  // Should NOT be found
                    if (id == 33333) found33333 = true;
                }
                
                REQUIRE(found11111 == true);
                REQUIRE(found22222 == false);
                REQUIRE(found33333 == true);
            }
        }
        
        mesh.stop();
    }
}

// ==================== getGatewayCount Tests ====================

SCENARIO("getGatewayCount returns 0 when no gateways exist", "[gateway][getGatewayCount]") {
    GIVEN("A mesh instance with no bridges") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Getting the gateway count") {
            size_t count = mesh.getGatewayCount();
            
            THEN("Should return 0") {
                REQUIRE(count == 0);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getGatewayCount returns correct count with multiple gateways", "[gateway][getGatewayCount]") {
    GIVEN("A mesh instance with multiple bridges with Internet") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add multiple bridges with Internet
        mesh.updateBridgeStatus(11111, true, -50, 6, 10000, "192.168.1.1", 1000);
        mesh.updateBridgeStatus(22222, true, -45, 6, 10000, "192.168.1.1", 1000);
        mesh.updateBridgeStatus(33333, true, -55, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Getting the gateway count") {
            size_t count = mesh.getGatewayCount();
            
            THEN("Should return 3") {
                REQUIRE(count == 3);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getGatewayCount only counts healthy bridges with Internet", "[gateway][getGatewayCount]") {
    GIVEN("A mesh instance with some bridges having Internet and some not") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add bridges - some with Internet, some without
        mesh.updateBridgeStatus(11111, true, -50, 6, 10000, "192.168.1.1", 1000);  // Has Internet
        mesh.updateBridgeStatus(22222, false, -45, 6, 10000, "192.168.1.1", 1000); // No Internet
        mesh.updateBridgeStatus(33333, true, -55, 6, 10000, "192.168.1.1", 1000);  // Has Internet
        mesh.updateBridgeStatus(44444, false, -60, 6, 10000, "192.168.1.1", 1000); // No Internet
        
        WHEN("Getting the gateway count") {
            size_t count = mesh.getGatewayCount();
            
            THEN("Should return 2 (only bridges with Internet)") {
                REQUIRE(count == 2);
            }
        }
        
        mesh.stop();
    }
}

// ==================== onGatewayChanged Callback Tests ====================

SCENARIO("onGatewayChanged callback is triggered when primary gateway changes", "[gateway][onGatewayChanged]") {
    GIVEN("A mesh instance with a gateway changed callback registered") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        uint32_t capturedOldPrimary = 0;
        uint32_t capturedNewPrimary = 0;
        bool callbackCalled = false;
        
        mesh.onGatewayChanged([&](uint32_t oldPrimary, uint32_t newPrimary) {
            capturedOldPrimary = oldPrimary;
            capturedNewPrimary = newPrimary;
            callbackCalled = true;
        });
        
        WHEN("A new primary gateway becomes available") {
            // Add first bridge - should trigger callback
            mesh.updateBridgeStatus(11111, true, -50, 6, 10000, "192.168.1.1", 1000);
            
            THEN("Callback should be triggered with correct values") {
                REQUIRE(callbackCalled == true);
                REQUIRE(capturedOldPrimary == 0);
                REQUIRE(capturedNewPrimary == 11111);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("onGatewayChanged callback receives correct old and new gateway IDs", "[gateway][onGatewayChanged]") {
    GIVEN("A mesh instance with an existing primary gateway") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // First add an initial gateway
        mesh.updateBridgeStatus(11111, true, -70, 6, 10000, "192.168.1.1", 1000);
        
        uint32_t capturedOldPrimary = 0;
        uint32_t capturedNewPrimary = 0;
        bool callbackCalled = false;
        
        mesh.onGatewayChanged([&](uint32_t oldPrimary, uint32_t newPrimary) {
            capturedOldPrimary = oldPrimary;
            capturedNewPrimary = newPrimary;
            callbackCalled = true;
        });
        
        WHEN("A better gateway (higher RSSI) becomes available") {
            // Add a better bridge - should trigger callback
            mesh.updateBridgeStatus(22222, true, -45, 6, 10000, "192.168.1.1", 1000);
            
            THEN("Callback should receive correct old and new gateway IDs") {
                REQUIRE(callbackCalled == true);
                REQUIRE(capturedOldPrimary == 11111);
                REQUIRE(capturedNewPrimary == 22222);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("onGatewayChanged callback not triggered when status updates don't change primary", "[gateway][onGatewayChanged]") {
    GIVEN("A mesh instance with an existing primary gateway") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add initial gateway with best RSSI
        mesh.updateBridgeStatus(11111, true, -45, 6, 10000, "192.168.1.1", 1000);
        
        int callbackCount = 0;
        
        mesh.onGatewayChanged([&](uint32_t oldPrimary, uint32_t newPrimary) {
            callbackCount++;
        });
        
        WHEN("Adding a gateway with worse RSSI (doesn't change primary)") {
            // Add a bridge with worse RSSI - should NOT trigger callback
            mesh.updateBridgeStatus(22222, true, -70, 6, 10000, "192.168.1.1", 1000);
            
            THEN("Callback should NOT be triggered") {
                REQUIRE(callbackCount == 0);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("onGatewayChanged callback triggered when primary gateway loses Internet", "[gateway][onGatewayChanged]") {
    GIVEN("A mesh instance with a primary gateway") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add initial gateway
        mesh.updateBridgeStatus(11111, true, -45, 6, 10000, "192.168.1.1", 1000);
        
        uint32_t capturedOldPrimary = 0;
        uint32_t capturedNewPrimary = 0;
        bool callbackCalled = false;
        
        mesh.onGatewayChanged([&](uint32_t oldPrimary, uint32_t newPrimary) {
            capturedOldPrimary = oldPrimary;
            capturedNewPrimary = newPrimary;
            callbackCalled = true;
        });
        
        WHEN("The primary gateway loses Internet connectivity") {
            // Update the same bridge to have no Internet
            mesh.updateBridgeStatus(11111, false, -45, 6, 10000, "192.168.1.1", 1000);
            
            THEN("Callback should be triggered with new primary = 0") {
                REQUIRE(callbackCalled == true);
                REQUIRE(capturedOldPrimary == 11111);
                REQUIRE(capturedNewPrimary == 0);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("onGatewayChanged callback triggered when gateway regains Internet", "[gateway][onGatewayChanged]") {
    GIVEN("A mesh instance with a bridge that has no Internet") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add bridge without Internet
        mesh.updateBridgeStatus(11111, false, -45, 6, 10000, "192.168.1.1", 1000);
        
        uint32_t capturedOldPrimary = 0;
        uint32_t capturedNewPrimary = 0;
        bool callbackCalled = false;
        
        mesh.onGatewayChanged([&](uint32_t oldPrimary, uint32_t newPrimary) {
            capturedOldPrimary = oldPrimary;
            capturedNewPrimary = newPrimary;
            callbackCalled = true;
        });
        
        WHEN("The bridge regains Internet connectivity") {
            // Update the bridge to have Internet
            mesh.updateBridgeStatus(11111, true, -45, 6, 10000, "192.168.1.1", 1000);
            
            THEN("Callback should be triggered with new primary = bridge ID") {
                REQUIRE(callbackCalled == true);
                REQUIRE(capturedOldPrimary == 0);
                REQUIRE(capturedNewPrimary == 11111);
            }
        }
        
        mesh.stop();
    }
}

// ==================== Edge Case Tests ====================

SCENARIO("Gateway APIs handle RSSI tie-breaking correctly", "[gateway][edgecase]") {
    GIVEN("A mesh instance with multiple bridges having same RSSI") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add bridges with same RSSI
        mesh.updateBridgeStatus(11111, true, -50, 6, 10000, "192.168.1.1", 1000);
        mesh.updateBridgeStatus(22222, true, -50, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Getting the primary gateway") {
            uint32_t gatewayId = mesh.getPrimaryGateway();
            
            THEN("Should return one of the bridges (first one found with best RSSI)") {
                // Either bridge is valid since they have the same RSSI
                REQUIRE((gatewayId == 11111 || gatewayId == 22222));
            }
        }
        
        WHEN("Getting gateway count") {
            size_t count = mesh.getGatewayCount();
            
            THEN("Should return 2") {
                REQUIRE(count == 2);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("Gateway APIs handle extreme RSSI values", "[gateway][edgecase]") {
    GIVEN("A mesh instance with bridges having extreme RSSI values") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        // Add bridges with extreme RSSI values
        mesh.updateBridgeStatus(11111, true, -127, 6, 10000, "192.168.1.1", 1000);  // Worst possible
        mesh.updateBridgeStatus(22222, true, 0, 6, 10000, "192.168.1.1", 1000);     // Best possible
        
        WHEN("Getting the primary gateway") {
            uint32_t gatewayId = mesh.getPrimaryGateway();
            
            THEN("Should return the bridge with best RSSI (0 dBm)") {
                REQUIRE(gatewayId == 22222);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("Gateway APIs work with single bridge", "[gateway][edgecase]") {
    GIVEN("A mesh instance with a single bridge") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        mesh.updateBridgeStatus(11111, true, -50, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Using all gateway APIs") {
            bool isPrimary = mesh.isPrimaryGateway();
            uint32_t gatewayId = mesh.getPrimaryGateway();
            auto gateways = mesh.getGateways();
            size_t count = mesh.getGatewayCount();
            
            THEN("Should return consistent values") {
                REQUIRE(isPrimary == false);  // This node (12345) is not the bridge
                REQUIRE(gatewayId == 11111);
                REQUIRE(gateways.size() == 1);
                REQUIRE(count == 1);
            }
        }
        
        mesh.stop();
    }
}
