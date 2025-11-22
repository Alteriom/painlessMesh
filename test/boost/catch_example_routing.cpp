#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "Arduino.h"
#include "mesh_simulator.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * @brief Test mesh routing functionality
 * Uses the existing simulator (MeshTest, Nodes classes)
 */

SCENARIO("Routing - broadcast reaches all nodes in mesh") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    Nodes n(&scheduler, 6, io_service);

    for (auto i = 0; i < 10000; ++i) {
        n.update();
        delay(10);
    }
    
    // Setup receivers
    size_t received[6] = {0};
    for (size_t i = 1; i < 6; ++i) {
        n.nodes[i]->onReceive([&, i](uint32_t from, std::string msg) {
            received[i]++;
        });
    }
    
    // Node 0 broadcasts
    n.nodes[0]->sendBroadcast("Broadcast message");
    for (auto i = 0; i < 1000; ++i) {
        n.update();
        delay(10);
    }

    // All other nodes should receive it
    size_t totalReceived = 0;
    for (size_t i = 1; i < 6; ++i) {
        totalReceived += received[i];
    }
    
    REQUIRE(totalReceived >= 5);
    n.stop();
}

SCENARIO("Routing - single message reaches specific destination") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    Nodes n(&scheduler, 5, io_service);

    for (auto i = 0; i < 10000; ++i) {
        n.update();
        delay(10);
    }
    
    size_t received = 0;
    std::string lastMsg;
    n.nodes[4]->onReceive([&](uint32_t from, std::string msg) {
        received++;
        lastMsg = msg;
    });
    
    n.nodes[0]->sendSingle(n.nodes[4]->getNodeId(), "Direct message");
    for (auto i = 0; i < 1000; ++i) {
        n.update();
        delay(10);
    }

    REQUIRE(received >= 1);
    REQUIRE(lastMsg == "Direct message");
    n.stop();
}
