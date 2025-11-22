#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "Arduino.h"
#include "simulator_utils.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * @brief Test time synchronization across the mesh
 * 
 * Time sync is a critical feature mentioned in many examples.
 * This validates that:
 * - Nodes synchronize their time
 * - Time stays synchronized
 * - Time sync works with varying network sizes
 */

SCENARIO("Time sync - nodes synchronize time after forming mesh") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh with nodes having random time offsets") {
        auto dim = runif(8, 12);
        SimulatedMeshNetwork network(&scheduler, dim, io_service);

        WHEN("The mesh first forms") {
            // Measure initial time differences
            int initialDiff = 0;
            for (size_t i = 0; i < network.size() - 1; ++i) {
                initialDiff += std::abs((int)network[0]->getNodeTime() -
                                       (int)network[i + 1]->getNodeTime());
            }
            initialDiff /= network.size();

            THEN("Initial time differences should be large") {
                REQUIRE(initialDiff > 10000);
            }
        }

        WHEN("Time sync protocol runs") {
            // Allow time for mesh formation and time sync
            network.runFor(10000, 10);
            
            // Measure final time differences
            int finalDiff = 0;
            for (size_t i = 0; i < network.size() - 1; ++i) {
                finalDiff += std::abs((int)network[0]->getNodeTime() -
                                     (int)network[i + 1]->getNodeTime());
            }
            finalDiff /= network.size();

            THEN("Final time differences should be small") {
                REQUIRE(finalDiff < 10000);
            }
        }

        network.stop();
    }
}

SCENARIO("Time sync - synchronized nodes can coordinate actions") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A synchronized mesh network") {
        SimulatedMeshNetwork network(&scheduler, 5, io_service);
        
        // Let mesh form and sync
        network.waitForFullMesh(5000);
        network.runFor(10000, 10);

        WHEN("Nodes check their synchronized time") {
            auto time0 = network[0]->getNodeTime();
            auto time1 = network[1]->getNodeTime();
            auto time2 = network[2]->getNodeTime();
            auto time3 = network[3]->getNodeTime();
            auto time4 = network[4]->getNodeTime();

            THEN("All times should be within a small window") {
                int maxDiff = 0;
                maxDiff = std::max(maxDiff, std::abs((int)time0 - (int)time1));
                maxDiff = std::max(maxDiff, std::abs((int)time0 - (int)time2));
                maxDiff = std::max(maxDiff, std::abs((int)time0 - (int)time3));
                maxDiff = std::max(maxDiff, std::abs((int)time0 - (int)time4));
                
                REQUIRE(maxDiff < 50000); // Within 50ms
            }
        }

        network.stop();
    }
}

SCENARIO("Time sync - time adjusted callback fires") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh node with time adjustment tracking") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);
        
        int adjustmentCount = 0;
        int32_t lastOffset = 0;
        
        network[0]->onNodeTimeAdjusted([&](int32_t offset) {
            adjustmentCount++;
            lastOffset = offset;
        });

        WHEN("Time synchronization occurs") {
            network.runFor(10000, 10);

            THEN("Time adjustment callback should fire") {
                REQUIRE(adjustmentCount > 0);
                // Offset should be non-zero (time was adjusted)
                REQUIRE(lastOffset != 0);
            }
        }

        network.stop();
    }
}

SCENARIO("Time sync - maintains synchronization over time") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A synchronized mesh") {
        SimulatedMeshNetwork network(&scheduler, 4, io_service);
        
        // Initial sync
        network.waitForFullMesh(5000);
        network.runFor(10000, 10);

        WHEN("Time passes and mesh continues running") {
            // Run for extended period
            network.runFor(20000, 10);
            
            // Check final synchronization
            int finalDiff = 0;
            for (size_t i = 0; i < network.size() - 1; ++i) {
                finalDiff += std::abs((int)network[0]->getNodeTime() -
                                     (int)network[i + 1]->getNodeTime());
            }
            finalDiff /= network.size();

            THEN("Time should remain synchronized") {
                REQUIRE(finalDiff < 15000); // Still within 15ms
            }
        }

        network.stop();
    }
}

SCENARIO("Time sync - coordinated blink timing example") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A synchronized mesh for coordinated actions") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);
        
        network.waitForFullMesh(5000);
        network.runFor(10000, 10);

        WHEN("Nodes calculate synchronized blink timing") {
            const uint32_t BLINK_PERIOD = 3000000; // 3 seconds in microseconds
            
            // All nodes calculate when next blink should occur
            auto now0 = network[0]->getNodeTime();
            auto now1 = network[1]->getNodeTime();
            auto now2 = network[2]->getNodeTime();
            
            auto nextBlink0 = now0 + (BLINK_PERIOD - (now0 % BLINK_PERIOD));
            auto nextBlink1 = now1 + (BLINK_PERIOD - (now1 % BLINK_PERIOD));
            auto nextBlink2 = now2 + (BLINK_PERIOD - (now2 % BLINK_PERIOD));

            THEN("Next blink times should be closely aligned") {
                int diff01 = std::abs((int)nextBlink0 - (int)nextBlink1);
                int diff02 = std::abs((int)nextBlink0 - (int)nextBlink2);
                
                REQUIRE(diff01 < 100000); // Within 100ms
                REQUIRE(diff02 < 100000);
            }
        }

        network.stop();
    }
}

SCENARIO("Time sync - getNodeTime() increases monotonically") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A running mesh node") {
        SimulatedMeshNode node(&scheduler, 12345, io_service);

        WHEN("Time passes") {
            auto time1 = node.getNodeTime();
            delay(100);
            auto time2 = node.getNodeTime();
            delay(100);
            auto time3 = node.getNodeTime();

            THEN("Time should always increase") {
                REQUIRE(time2 > time1);
                REQUIRE(time3 > time2);
            }
        }
    }
}

SCENARIO("Time sync - works with large meshes") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A large mesh network") {
        auto dim = runif(10, 15);
        SimulatedMeshNetwork network(&scheduler, dim, io_service);

        WHEN("Large mesh synchronizes") {
            network.runFor(15000, 10);
            
            int avgDiff = 0;
            for (size_t i = 0; i < network.size() - 1; ++i) {
                avgDiff += std::abs((int)network[0]->getNodeTime() -
                                   (int)network[i + 1]->getNodeTime());
            }
            avgDiff /= network.size();

            THEN("Even large meshes should achieve synchronization") {
                REQUIRE(avgDiff < 20000); // Within 20ms average for large mesh
            }
        }

        network.stop();
    }
}
