#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "Arduino.h"
#include "mesh_simulator.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * @brief Test the basic.ino example functionality
 * 
 * This test validates that the basic example works correctly using
 * the existing MeshTest and Nodes simulator classes.
 */

SCENARIO("Basic example - message broadcasting works") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    Nodes n(&scheduler, 3, io_service);

    // Let mesh form
    for (auto i = 0; i < 5000; ++i) {
        n.update();
        delay(10);
    }
    
    REQUIRE(layout::size(n.nodes[0]->asNodeTree()) == 3);
    
    // Test broadcast
    size_t msg1 = 0, msg2 = 0;
    std::string last1, last2;
    
    n.nodes[1]->onReceive([&](uint32_t from, std::string msg) {
        msg1++;
        last1 = msg;
    });
    n.nodes[2]->onReceive([&](uint32_t from, std::string msg) {
        msg2++;
        last2 = msg;
    });
    
    n.nodes[0]->sendBroadcast("Hello from node 0");
    
    for (auto i = 0; i < 1000; ++i) {
        n.update();
        delay(10);
    }

    REQUIRE(msg1 >= 1);
    REQUIRE(msg2 >= 1);
    REQUIRE(last1 == "Hello from node 0");
    REQUIRE(last2 == "Hello from node 0");
    
    n.stop();
}

SCENARIO("Basic example - callbacks work") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    Nodes n(&scheduler, 2, io_service);
    
    size_t received0 = 0, received1 = 0;
    n.nodes[0]->onReceive([&](uint32_t from, std::string msg) { received0++; });
    n.nodes[1]->onReceive([&](uint32_t from, std::string msg) { received1++; });

    for (auto i = 0; i < 1000; ++i) {
        n.update();
        delay(10);
    }
    
    n.nodes[0]->sendBroadcast("Test");
    for (auto i = 0; i < 500; ++i) {
        n.update();
        delay(10);
    }

    REQUIRE(received1 > 0);
    n.stop();
}
