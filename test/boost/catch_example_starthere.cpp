#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "Arduino.h"
#include "simulator_utils.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * @brief Test the startHere.ino example functionality
 * 
 * This test validates the more advanced features in startHere:
 * - Node list tracking
 * - Time synchronization
 * - Delay measurements
 * - Connection change tracking
 */

SCENARIO("startHere example - node list tracking") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network with 4 nodes") {
        SimulatedMeshNetwork network(&scheduler, 4, io_service);

        WHEN("The mesh forms") {
            bool meshFormed = network.waitForFullMesh(10000);

            THEN("Each node should see 4 nodes including itself") {
                REQUIRE(meshFormed);
                
                for (size_t i = 0; i < 4; ++i) {
                    auto nodeList = network[i]->getNodeList();
                    REQUIRE(nodeList.size() == 3); // getNodeList returns other nodes
                    REQUIRE(layout::size(network[i]->asNodeTree()) == 4);
                }
            }

            AND_THEN("Node list should be accurate") {
                auto node0List = network[0]->getNodeList();
                
                // Check that all other nodes are in the list
                bool hasNode1 = false;
                bool hasNode2 = false;
                bool hasNode3 = false;
                
                for (auto nodeId : node0List) {
                    if (nodeId == network[1]->getNodeId()) hasNode1 = true;
                    if (nodeId == network[2]->getNodeId()) hasNode2 = true;
                    if (nodeId == network[3]->getNodeId()) hasNode3 = true;
                }
                
                REQUIRE(hasNode1);
                REQUIRE(hasNode2);
                REQUIRE(hasNode3);
            }
        }

        network.stop();
    }
}

SCENARIO("startHere example - time synchronization") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network with random time offsets") {
        SimulatedMeshNetwork network(&scheduler, 4, io_service);

        WHEN("The mesh runs for time sync to occur") {
            // Let mesh form
            network.waitForFullMesh(5000);
            
            // Get initial time differences
            int initialDiff = 0;
            for (size_t i = 0; i < network.size() - 1; ++i) {
                initialDiff += std::abs((int)network[0]->getNodeTime() -
                                       (int)network[i + 1]->getNodeTime());
            }
            initialDiff /= network.size();

            // Run for time sync
            network.runFor(10000, 10);
            
            // Get final time differences
            int finalDiff = 0;
            for (size_t i = 0; i < network.size() - 1; ++i) {
                finalDiff += std::abs((int)network[0]->getNodeTime() -
                                     (int)network[i + 1]->getNodeTime());
            }
            finalDiff /= network.size();

            THEN("Time differences should decrease") {
                REQUIRE(finalDiff < initialDiff);
                REQUIRE(finalDiff < 10000); // Should be within 10ms average
            }
        }

        network.stop();
    }
}

SCENARIO("startHere example - new connection callback") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network being formed") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);
        
        size_t connectionCallbacks = 0;
        network[0]->setConnectionCallback([&]() {
            connectionCallbacks++;
        });

        WHEN("Connections are established") {
            network.runFor(2000);

            THEN("Connection callbacks should fire") {
                REQUIRE(connectionCallbacks > 0);
                REQUIRE(network[0]->getConnectionsChanged() > 0);
            }
        }

        network.stop();
    }
}

SCENARIO("startHere example - message sending with node ID") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network with identified nodes") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);

        WHEN("Messages include node IDs") {
            network.waitForFullMesh(5000);
            
            // Simulate the startHere message format
            std::string msg = "Hello from node " + 
                            std::to_string(network[0]->getNodeId());
            network[0]->sendBroadcast(msg);
            
            network.runFor(1000);

            THEN("Receivers can identify the sender") {
                auto lastMsg = network[1]->getLastMessage();
                REQUIRE(lastMsg.find("Hello from node") != std::string::npos);
                REQUIRE(lastMsg.find(std::to_string(network[0]->getNodeId())) != 
                       std::string::npos);
            }
        }

        network.stop();
    }
}

SCENARIO("startHere example - delay measurement") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("Two connected nodes") {
        SimulatedMeshNetwork network(&scheduler, 2, io_service);

        WHEN("Delay measurement is requested") {
            network.waitForFullMesh(2000);
            
            bool delayReceived = false;
            int32_t measuredDelay = 0;
            
            network[0]->onNodeDelayReceived([&](uint32_t nodeId, int32_t delay) {
                if (nodeId == network[1]->getNodeId()) {
                    delayReceived = true;
                    measuredDelay = delay;
                }
            });
            
            network[0]->startDelayMeas(network[1]->getNodeId());
            
            // Wait for delay measurement response
            network.runFor(2000);

            THEN("Delay should be measured") {
                REQUIRE(delayReceived);
                REQUIRE(measuredDelay >= 0);
                REQUIRE(measuredDelay < 1000000); // Less than 1 second
            }
        }

        network.stop();
    }
}

SCENARIO("startHere example - handling disconnections") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A formed mesh network") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);
        network.waitForFullMesh(5000);
        
        size_t initialConnections = network[0]->getConnectionsChanged();

        WHEN("A node disconnects") {
            // Close one connection
            if (network[0]->subs.size() > 0) {
                (*network[0]->subs.begin())->close();
                network.runFor(1000);

                THEN("Connection changed callback fires") {
                    REQUIRE(network[0]->getConnectionsChanged() > initialConnections);
                }

                AND_THEN("Mesh topology updates") {
                    size_t newSize = layout::size(network[0]->asNodeTree());
                    REQUIRE(newSize < 3);
                }
            }
        }

        network.stop();
    }
}

SCENARIO("startHere example - subConnectionJson returns valid JSON") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);

        WHEN("The mesh forms") {
            network.waitForFullMesh(5000);

            THEN("subConnectionJson should return valid JSON") {
                std::string json = network[0]->subConnectionJson(true);
                
                // Basic JSON validation
                REQUIRE(json.find("{") != std::string::npos);
                REQUIRE(json.find("}") != std::string::npos);
                REQUIRE(json.find("subs") != std::string::npos);
            }
        }

        network.stop();
    }
}

SCENARIO("startHere example - random broadcast intervals") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network with broadcasting nodes") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);
        network.waitForFullMesh(5000);

        WHEN("Multiple broadcasts occur over time") {
            size_t initialMsgs1 = network[1]->getMessagesReceived();
            size_t initialMsgs2 = network[2]->getMessagesReceived();
            
            // Simulate multiple broadcast cycles
            for (int i = 0; i < 5; ++i) {
                network[0]->sendBroadcast("Message " + std::to_string(i));
                network.runFor(200);
            }

            THEN("All nodes receive multiple messages") {
                REQUIRE(network[1]->getMessagesReceived() > initialMsgs1);
                REQUIRE(network[2]->getMessagesReceived() > initialMsgs2);
            }
        }

        network.stop();
    }
}
