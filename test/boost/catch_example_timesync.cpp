#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "Arduino.h"
#include "mesh_simulator.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * @brief Test time synchronization across the mesh
 * Uses the existing simulator (MeshTest, Nodes classes)
 */

SCENARIO("Time sync - nodes synchronize time after forming mesh") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    auto nodeCount = runif(8, 12);
    Nodes n(&scheduler, nodeCount, io_service);

    // Measure initial time differences
    int initialDiff = 0;
    for (size_t i = 0; i < n.size() - 1; ++i) {
        initialDiff += std::abs((int)n.nodes[0]->getNodeTime() -
                               (int)n.nodes[i + 1]->getNodeTime());
    }
    initialDiff /= n.size();

    REQUIRE(initialDiff > 10000);

    // Allow time for mesh formation and time sync
    for (auto i = 0; i < 10000; ++i) {
        n.update();
        delay(10);
    }
    
    // Measure final time differences
    int finalDiff = 0;
    for (size_t i = 0; i < n.size() - 1; ++i) {
        finalDiff += std::abs((int)n.nodes[0]->getNodeTime() -
                             (int)n.nodes[i + 1]->getNodeTime());
    }
    finalDiff /= n.size();

    REQUIRE(finalDiff < 10000);
    n.stop();
}

SCENARIO("Time sync - time adjusted callback fires") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    Nodes n(&scheduler, 3, io_service);
    
    int adjustmentCount = 0;
    n.nodes[0]->onNodeTimeAdjusted([&](int32_t offset) {
        adjustmentCount++;
    });

    for (auto i = 0; i < 10000; ++i) {
        n.update();
        delay(10);
    }

    REQUIRE(adjustmentCount > 0);
    n.stop();
}
