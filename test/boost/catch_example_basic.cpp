#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "Arduino.h"
#include "simulator_utils.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * @brief Test the basic.ino example functionality
 * 
 * This test validates that the basic example works correctly:
 * - Nodes can form a mesh
 * - Messages can be broadcast
 * - Messages are received by all nodes
 * - Callbacks are triggered correctly
 */
SCENARIO("Basic example - message broadcasting works") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network with 3 nodes") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);

        WHEN("The network is allowed to stabilize") {
            bool meshFormed = network.waitForFullMesh(5000);
            
            THEN("All nodes should see the full mesh topology") {
                REQUIRE(meshFormed);
                REQUIRE(layout::size(network[0]->asNodeTree()) == 3);
                REQUIRE(layout::size(network[1]->asNodeTree()) == 3);
                REQUIRE(layout::size(network[2]->asNodeTree()) == 3);
            }

            AND_WHEN("Node 0 broadcasts a message") {
                network[0]->sendBroadcast("Hello from node 0");
                
                // Allow time for message propagation
                network.runFor(1000);

                THEN("All other nodes should receive the message") {
                    REQUIRE(network[1]->getMessagesReceived() >= 1);
                    REQUIRE(network[2]->getMessagesReceived() >= 1);
                    REQUIRE(network[1]->getLastMessage() == "Hello from node 0");
                    REQUIRE(network[2]->getLastMessage() == "Hello from node 0");
                }
            }

            AND_WHEN("Multiple messages are sent") {
                network[0]->resetCounters();
                network[1]->resetCounters();
                network[2]->resetCounters();

                network[0]->sendBroadcast("Message 1");
                network.runFor(500);
                network[1]->sendBroadcast("Message 2");
                network.runFor(500);
                network[2]->sendBroadcast("Message 3");
                network.runFor(500);

                THEN("All nodes receive messages from all others") {
                    // Each node should receive 2 messages (from the other 2 nodes)
                    REQUIRE(network[0]->getMessagesReceived() >= 2);
                    REQUIRE(network[1]->getMessagesReceived() >= 2);
                    REQUIRE(network[2]->getMessagesReceived() >= 2);
                }
            }
        }

        network.stop();
    }
}

SCENARIO("Basic example - callback functionality") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network with 2 nodes") {
        SimulatedMeshNetwork network(&scheduler, 2, io_service);

        WHEN("The network forms") {
            network.runFor(1000);

            THEN("Connection changed callbacks should have fired") {
                REQUIRE(network[0]->getConnectionsChanged() > 0);
                REQUIRE(network[1]->getConnectionsChanged() > 0);
            }
        }

        network.stop();
    }
}

SCENARIO("Basic example - received callback is triggered") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("Two connected nodes with custom receive callbacks") {
        SimulatedMeshNetwork network(&scheduler, 2, io_service);
        
        size_t node0Received = 0;
        size_t node1Received = 0;
        std::string node0LastMsg;
        std::string node1LastMsg;
        
        network[0]->setReceiveCallback([&](uint32_t from, std::string msg) {
            node0Received++;
            node0LastMsg = msg;
        });
        
        network[1]->setReceiveCallback([&](uint32_t from, std::string msg) {
            node1Received++;
            node1LastMsg = msg;
        });

        WHEN("Messages are exchanged") {
            network.runFor(1000);
            
            network[0]->sendBroadcast("Test message from 0");
            network.runFor(500);
            
            network[1]->sendBroadcast("Test message from 1");
            network.runFor(500);

            THEN("Both nodes should receive messages via callback") {
                REQUIRE(node0Received > 0);
                REQUIRE(node1Received > 0);
                REQUIRE(node0LastMsg == "Test message from 1");
                REQUIRE(node1LastMsg == "Test message from 0");
            }
        }

        network.stop();
    }
}

SCENARIO("Basic example - node ID retrieval") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh node") {
        SimulatedMeshNode node(&scheduler, 12345, io_service);

        THEN("getNodeId() should return the correct ID") {
            REQUIRE(node.getNodeId() == 12345);
        }
    }
}

SCENARIO("Basic example - mesh forms correctly with varying sizes") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network with 5 nodes") {
        SimulatedMeshNetwork network(&scheduler, 5, io_service);

        WHEN("The network stabilizes") {
            bool meshFormed = network.waitForFullMesh(10000);

            THEN("All nodes should see all 5 nodes") {
                REQUIRE(meshFormed);
                for (size_t i = 0; i < 5; ++i) {
                    REQUIRE(layout::size(network[i]->asNodeTree()) == 5);
                }
            }

            AND_WHEN("Any node broadcasts a message") {
                network[2]->sendBroadcast("Broadcast from middle node");
                network.runFor(1000);

                THEN("All other 4 nodes should receive it") {
                    size_t totalReceived = 0;
                    for (size_t i = 0; i < 5; ++i) {
                        if (i != 2) { // Skip sender
                            totalReceived += network[i]->getMessagesReceived();
                        }
                    }
                    REQUIRE(totalReceived >= 4);
                }
            }
        }

        network.stop();
    }
}
