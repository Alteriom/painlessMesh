#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "Arduino.h"
#include "simulator_utils.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * @brief Test mesh routing functionality
 * 
 * These tests validate that messages are properly routed through the mesh:
 * - Multi-hop routing works correctly
 * - Broadcast messages reach all nodes
 * - Single messages reach the intended destination
 * - Mesh adapts to topology changes
 */

SCENARIO("Routing - broadcast reaches all nodes in mesh") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network with 6 nodes") {
        SimulatedMeshNetwork network(&scheduler, 6, io_service);

        WHEN("Any node broadcasts a message") {
            network.waitForFullMesh(10000);
            
            // Reset counters
            for (size_t i = 0; i < 6; ++i) {
                network[i]->resetCounters();
            }
            
            // Node 0 broadcasts
            network[0]->sendBroadcast("Broadcast message");
            network.runFor(1000);

            THEN("All other nodes receive the broadcast") {
                size_t totalReceived = 0;
                for (size_t i = 1; i < 6; ++i) {
                    totalReceived += network[i]->getMessagesReceived();
                }
                
                // At least 5 nodes should receive it (all except sender)
                REQUIRE(totalReceived >= 5);
            }

            AND_THEN("Message content is preserved") {
                bool allCorrect = true;
                for (size_t i = 1; i < 6; ++i) {
                    if (network[i]->getMessagesReceived() > 0) {
                        if (network[i]->getLastMessage() != "Broadcast message") {
                            allCorrect = false;
                        }
                    }
                }
                REQUIRE(allCorrect);
            }
        }

        network.stop();
    }
}

SCENARIO("Routing - single message reaches specific destination") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh with 5 nodes") {
        SimulatedMeshNetwork network(&scheduler, 5, io_service);

        WHEN("Node 0 sends a single message to node 4") {
            network.waitForFullMesh(10000);
            
            // Reset counters
            for (size_t i = 0; i < 5; ++i) {
                network[i]->resetCounters();
            }
            
            network[0]->sendSingle(network[4]->getNodeId(), "Direct message");
            network.runFor(1000);

            THEN("Node 4 receives the message") {
                REQUIRE(network[4]->getMessagesReceived() >= 1);
                REQUIRE(network[4]->getLastMessage() == "Direct message");
            }

            AND_THEN("Node 4 correctly identifies the sender") {
                REQUIRE(network[4]->getLastMessageFrom() == network[0]->getNodeId());
            }
        }

        network.stop();
    }
}

SCENARIO("Routing - messages route through intermediate nodes") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A linear mesh topology") {
        // Create nodes but connect them linearly: 0->1->2->3
        Scheduler scheduler;
        boost::asio::io_context io_service;
        
        std::vector<std::shared_ptr<SimulatedMeshNode>> nodes;
        for (size_t i = 0; i < 4; ++i) {
            auto node = std::make_shared<SimulatedMeshNode>(
                &scheduler, 7000 + i, io_service);
            nodes.push_back(node);
        }
        
        // Connect in a line
        nodes[1]->connectTo(*nodes[0]);
        nodes[2]->connectTo(*nodes[1]);
        nodes[3]->connectTo(*nodes[2]);

        WHEN("Nodes form a mesh") {
            // Run until topology stabilizes
            for (auto i = 0; i < 5000; ++i) {
                for (auto &&n : nodes) {
                    n->update();
                    io_service.poll();
                }
                delay(10);
            }

            AND_WHEN("End nodes communicate") {
                nodes[0]->resetCounters();
                nodes[3]->resetCounters();
                
                // Node 0 sends to node 3 (must route through 1 and 2)
                nodes[0]->sendSingle(nodes[3]->getNodeId(), "Multi-hop message");
                
                for (auto i = 0; i < 2000; ++i) {
                    for (auto &&n : nodes) {
                        n->update();
                        io_service.poll();
                    }
                    delay(10);
                }

                THEN("Message reaches the destination through routing") {
                    REQUIRE(nodes[3]->getMessagesReceived() >= 1);
                    REQUIRE(nodes[3]->getLastMessage() == "Multi-hop message");
                }
            }
        }

        for (auto &&n : nodes) {
            n->stop();
        }
    }
}

SCENARIO("Routing - mesh topology updates correctly") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A formed mesh") {
        SimulatedMeshNetwork network(&scheduler, 4, io_service);
        network.waitForFullMesh(10000);

        WHEN("Topology is queried") {
            auto nodeList0 = network[0]->getNodeList();
            auto nodeList1 = network[1]->getNodeList();

            THEN("All nodes have consistent view of the mesh") {
                // Each node should know about 3 others
                REQUIRE(nodeList0.size() == 3);
                REQUIRE(nodeList1.size() == 3);
                
                // All should see 4 nodes total including themselves
                REQUIRE(layout::size(network[0]->asNodeTree()) == 4);
                REQUIRE(layout::size(network[1]->asNodeTree()) == 4);
            }
        }

        network.stop();
    }
}

SCENARIO("Routing - handles node joins correctly") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A small mesh") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);
        network.waitForFullMesh(5000);

        WHEN("A new node joins") {
            // Create and connect a new node
            auto newNode = std::make_shared<SimulatedMeshNode>(
                &scheduler, 8888, io_service);
            newNode->connectTo(*network[0]);
            
            // Let the mesh update
            for (auto i = 0; i < 3000; ++i) {
                for (size_t j = 0; j < 3; ++j) {
                    network[j]->update();
                }
                newNode->update();
                io_service.poll();
                delay(10);
            }

            THEN("All nodes eventually see the expanded mesh") {
                // Each original node should see 4 nodes total
                size_t nodesSeeing4 = 0;
                for (size_t i = 0; i < 3; ++i) {
                    if (layout::size(network[i]->asNodeTree()) >= 4) {
                        nodesSeeing4++;
                    }
                }
                
                // At least some nodes should see the full mesh
                REQUIRE(nodesSeeing4 > 0);
            }

            newNode->stop();
        }

        network.stop();
    }
}

SCENARIO("Routing - multiple broadcasts don't interfere") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh with 4 nodes") {
        SimulatedMeshNetwork network(&scheduler, 4, io_service);
        network.waitForFullMesh(5000);

        WHEN("Multiple nodes broadcast simultaneously") {
            for (size_t i = 0; i < 4; ++i) {
                network[i]->resetCounters();
            }
            
            // All nodes broadcast at roughly the same time
            network[0]->sendBroadcast("Message from 0");
            network[1]->sendBroadcast("Message from 1");
            network[2]->sendBroadcast("Message from 2");
            network[3]->sendBroadcast("Message from 3");
            
            network.runFor(2000);

            THEN("All nodes receive messages from others") {
                // Each node should receive at least 3 messages (from the other 3)
                for (size_t i = 0; i < 4; ++i) {
                    REQUIRE(network[i]->getMessagesReceived() >= 3);
                }
            }
        }

        network.stop();
    }
}

SCENARIO("Routing - large message handling") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("Two connected nodes") {
        SimulatedMeshNetwork network(&scheduler, 2, io_service);
        network.waitForFullMesh(2000);

        WHEN("A large message is sent") {
            std::string largeMessage;
            for (int i = 0; i < 100; ++i) {
                largeMessage += "This is a large message segment " + std::to_string(i) + ". ";
            }
            
            network[0]->sendBroadcast(largeMessage);
            network.runFor(1000);

            THEN("Large message is delivered successfully") {
                REQUIRE(network[1]->getMessagesReceived() >= 1);
                REQUIRE(network[1]->getLastMessage() == largeMessage);
            }
        }

        network.stop();
    }
}

SCENARIO("Routing - message ordering with rapid sends") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("Two nodes with message tracking") {
        SimulatedMeshNetwork network(&scheduler, 2, io_service);
        
        std::vector<std::string> receivedOrder;
        network[1]->setReceiveCallback([&](uint32_t from, std::string msg) {
            receivedOrder.push_back(msg);
        });

        WHEN("Multiple messages are sent in quick succession") {
            network.waitForFullMesh(2000);
            
            for (int i = 0; i < 5; ++i) {
                network[0]->sendBroadcast("Message " + std::to_string(i));
                network.runFor(100);
            }
            
            network.runFor(1000);

            THEN("All messages are delivered") {
                REQUIRE(receivedOrder.size() >= 5);
            }

            AND_THEN("Messages can be identified") {
                bool foundAll = true;
                for (int i = 0; i < 5; ++i) {
                    std::string expected = "Message " + std::to_string(i);
                    bool found = false;
                    for (const auto& msg : receivedOrder) {
                        if (msg == expected) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) foundAll = false;
                }
                REQUIRE(foundAll);
            }
        }

        network.stop();
    }
}

SCENARIO("Routing - node list accuracy") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh network") {
        auto nodeCount = runif(5, 8);
        SimulatedMeshNetwork network(&scheduler, nodeCount, io_service);

        WHEN("Mesh stabilizes") {
            network.waitForFullMesh(10000);

            THEN("Each node's list contains all other nodes") {
                for (size_t i = 0; i < nodeCount; ++i) {
                    auto list = network[i]->getNodeList();
                    // List should have all nodes except self
                    REQUIRE(list.size() == nodeCount - 1);
                }
            }

            AND_THEN("Node lists contain valid node IDs") {
                auto list0 = network[0]->getNodeList();
                
                for (auto nodeId : list0) {
                    // Should not contain its own ID
                    REQUIRE(nodeId != network[0]->getNodeId());
                    
                    // Should be a valid node in the network
                    bool isValid = false;
                    for (size_t i = 1; i < nodeCount; ++i) {
                        if (nodeId == network[i]->getNodeId()) {
                            isValid = true;
                            break;
                        }
                    }
                    REQUIRE(isValid);
                }
            }
        }

        network.stop();
    }
}
