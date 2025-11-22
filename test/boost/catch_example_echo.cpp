#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "Arduino.h"
#include "mesh_simulator.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * @brief Test the echoNode.ino example functionality
 * Uses the existing MeshTest and Nodes simulator classes
 */

SCENARIO("echoNode example - echoes messages back to sender") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    Nodes n(&scheduler, 2, io_service);

    // Setup echo behavior on node 0
    n.nodes[0]->onReceive([&](uint32_t from, std::string msg) {
        n.nodes[0]->sendSingle(from, msg);
    });
    
    // Track replies on node 1
    size_t replyCount = 0;
    std::string lastReply;
    n.nodes[1]->onReceive([&](uint32_t from, std::string msg) {
        replyCount++;
        lastReply = msg;
    });

    // Let mesh form
    for (auto i = 0; i < 2000; ++i) {
        n.update();
        delay(10);
    }
    
    std::string testMessage = "Test echo message";
    n.nodes[1]->sendSingle(n.nodes[0]->getNodeId(), testMessage);
    
    // Allow time for echo
    for (auto i = 0; i < 1000; ++i) {
        n.update();
        delay(10);
    }

    REQUIRE(replyCount >= 1);
    REQUIRE(lastReply == testMessage);
    
    n.stop();
}

SCENARIO("echoNode example - handles multiple messages") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    Nodes n(&scheduler, 2, io_service);
    
    // Setup echo on node 0
    n.nodes[0]->onReceive([&](uint32_t from, std::string msg) {
        n.nodes[0]->sendSingle(from, msg);
    });
    
    // Track on node 1
    size_t repliesReceived = 0;
    std::vector<std::string> replies;
    n.nodes[1]->onReceive([&](uint32_t from, std::string msg) {
        repliesReceived++;
        replies.push_back(msg);
    });

    for (auto i = 0; i < 2000; ++i) {
        n.update();
        delay(10);
    }
    
    n.nodes[1]->sendSingle(n.nodes[0]->getNodeId(), "Message 1");
    for (auto i = 0; i < 300; ++i) n.update();
    
    n.nodes[1]->sendSingle(n.nodes[0]->getNodeId(), "Message 2");
    for (auto i = 0; i < 300; ++i) n.update();
    
    n.nodes[1]->sendSingle(n.nodes[0]->getNodeId(), "Message 3");
    for (auto i = 0; i < 300; ++i) n.update();

    REQUIRE(repliesReceived >= 3);
    n.stop();
}
