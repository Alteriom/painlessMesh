#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "Arduino.h"
#include "mesh_simulator.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * @brief Test the startHere.ino example functionality
 * Uses the existing simulator (MeshTest, Nodes classes)
 */

SCENARIO("startHere example - node list tracking") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    Nodes n(&scheduler, 4, io_service);

    for (auto i = 0; i < 10000; ++i) {
        n.update();
        delay(10);
    }

    // Each node should see 4 nodes including itself
    for (size_t i = 0; i < 4; ++i) {
        auto nodeList = n.nodes[i]->getNodeList();
        REQUIRE(nodeList.size() == 3); // getNodeList returns other nodes
        REQUIRE(layout::size(n.nodes[i]->asNodeTree()) == 4);
    }

    n.stop();
}

SCENARIO("startHere example - message sending with node ID") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    Nodes n(&scheduler, 3, io_service);

    for (auto i = 0; i < 5000; ++i) {
        n.update();
        delay(10);
    }
    
    size_t received = 0;
    std::string lastMsg;
    n.nodes[1]->onReceive([&](uint32_t from, std::string msg) {
        received++;
        lastMsg = msg;
    });
    
    // Simulate the startHere message format
    std::string msg = "Hello from node " + std::to_string(n.nodes[0]->getNodeId());
    n.nodes[0]->sendBroadcast(msg);
    
    for (auto i = 0; i < 1000; ++i) {
        n.update();
        delay(10);
    }

    REQUIRE(received > 0);
    REQUIRE(lastMsg.find("Hello from node") != std::string::npos);
    
    n.stop();
}
