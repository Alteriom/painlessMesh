#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "Arduino.h"
#include "simulator_utils.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * @brief Test the echoNode.ino example functionality
 * 
 * This test validates the echo node behavior:
 * - Receives messages and echoes them back to sender
 * - Uses sendSingle() for targeted replies
 * - Demonstrates point-to-point communication
 */

SCENARIO("echoNode example - echoes messages back to sender") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh with an echo node and a sender") {
        SimulatedMeshNetwork network(&scheduler, 2, io_service);
        
        // Node 0 is the echo node
        // Node 1 is the sender
        
        // Setup echo behavior on node 0
        size_t echoCount = 0;
        network[0]->setReceiveCallback([&](uint32_t from, std::string msg) {
            echoCount++;
            // Echo the message back
            network[0]->sendSingle(from, msg);
        });
        
        // Track replies on node 1
        size_t replyCount = 0;
        std::string lastReply;
        network[1]->setReceiveCallback([&](uint32_t from, std::string msg) {
            replyCount++;
            lastReply = msg;
        });

        WHEN("Node 1 sends a message to the echo node") {
            network.waitForFullMesh(2000);
            
            std::string testMessage = "Test echo message";
            network[1]->sendSingle(network[0]->getNodeId(), testMessage);
            
            // Allow time for echo
            network.runFor(1000);

            THEN("Echo node receives the message") {
                REQUIRE(echoCount >= 1);
            }

            AND_THEN("Echo node sends reply back to sender") {
                REQUIRE(replyCount >= 1);
                REQUIRE(lastReply == testMessage);
            }
        }

        network.stop();
    }
}

SCENARIO("echoNode example - handles multiple messages") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("An echo node and a sender") {
        SimulatedMeshNetwork network(&scheduler, 2, io_service);
        
        // Setup echo on node 0
        network[0]->setReceiveCallback([&](uint32_t from, std::string msg) {
            network[0]->sendSingle(from, msg);
        });
        
        // Track on node 1
        size_t repliesReceived = 0;
        std::vector<std::string> replies;
        network[1]->setReceiveCallback([&](uint32_t from, std::string msg) {
            repliesReceived++;
            replies.push_back(msg);
        });

        WHEN("Multiple different messages are sent") {
            network.waitForFullMesh(2000);
            
            network[1]->sendSingle(network[0]->getNodeId(), "Message 1");
            network.runFor(300);
            
            network[1]->sendSingle(network[0]->getNodeId(), "Message 2");
            network.runFor(300);
            
            network[1]->sendSingle(network[0]->getNodeId(), "Message 3");
            network.runFor(300);

            THEN("All messages are echoed back") {
                REQUIRE(repliesReceived >= 3);
                REQUIRE(replies.size() >= 3);
            }

            AND_THEN("Messages are echoed in correct content") {
                bool foundMsg1 = false;
                bool foundMsg2 = false;
                bool foundMsg3 = false;
                
                for (const auto& reply : replies) {
                    if (reply == "Message 1") foundMsg1 = true;
                    if (reply == "Message 2") foundMsg2 = true;
                    if (reply == "Message 3") foundMsg3 = true;
                }
                
                REQUIRE(foundMsg1);
                REQUIRE(foundMsg2);
                REQUIRE(foundMsg3);
            }
        }

        network.stop();
    }
}

SCENARIO("echoNode example - works in multi-node mesh") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("A mesh with 4 nodes where one is an echo node") {
        SimulatedMeshNetwork network(&scheduler, 4, io_service);
        
        // Node 0 is echo node
        network[0]->setReceiveCallback([&](uint32_t from, std::string msg) {
            network[0]->sendSingle(from, msg);
        });
        
        // Node 1 sends and receives
        size_t node1Replies = 0;
        network[1]->setReceiveCallback([&](uint32_t from, std::string msg) {
            if (from == network[0]->getNodeId()) {
                node1Replies++;
            }
        });
        
        // Node 2 sends and receives
        size_t node2Replies = 0;
        network[2]->setReceiveCallback([&](uint32_t from, std::string msg) {
            if (from == network[0]->getNodeId()) {
                node2Replies++;
            }
        });

        WHEN("Multiple nodes send to echo node") {
            network.waitForFullMesh(5000);
            
            // Node 1 sends
            network[1]->sendSingle(network[0]->getNodeId(), "From node 1");
            network.runFor(500);
            
            // Node 2 sends
            network[2]->sendSingle(network[0]->getNodeId(), "From node 2");
            network.runFor(500);

            THEN("Each sender receives their echo") {
                REQUIRE(node1Replies >= 1);
                REQUIRE(node2Replies >= 1);
            }
        }

        network.stop();
    }
}

SCENARIO("echoNode example - sendSingle vs sendBroadcast") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("An echo node and multiple receivers") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);
        
        // Node 0 is echo node
        network[0]->setReceiveCallback([&](uint32_t from, std::string msg) {
            network[0]->sendSingle(from, msg);
        });
        
        // Track messages on nodes 1 and 2
        size_t node1Count = 0;
        size_t node2Count = 0;
        
        network[1]->setReceiveCallback([&](uint32_t from, std::string msg) {
            node1Count++;
        });
        
        network[2]->setReceiveCallback([&](uint32_t from, std::string msg) {
            node2Count++;
        });

        WHEN("Node 1 sends to echo node") {
            network.waitForFullMesh(5000);
            
            size_t initial1 = node1Count;
            size_t initial2 = node2Count;
            
            network[1]->sendSingle(network[0]->getNodeId(), "Test");
            network.runFor(1000);

            THEN("Only node 1 receives the echo (not broadcast)") {
                REQUIRE(node1Count > initial1);
                // Node 2 should not receive the echo (sendSingle is targeted)
                REQUIRE(node2Count == initial2);
            }
        }

        network.stop();
    }
}

SCENARIO("echoNode example - preserves message content") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("An echo node and sender with various message types") {
        SimulatedMeshNetwork network(&scheduler, 2, io_service);
        
        network[0]->setReceiveCallback([&](uint32_t from, std::string msg) {
            network[0]->sendSingle(from, msg);
        });
        
        std::vector<std::string> receivedMessages;
        network[1]->setReceiveCallback([&](uint32_t from, std::string msg) {
            receivedMessages.push_back(msg);
        });

        WHEN("Different message types are sent") {
            network.waitForFullMesh(2000);
            
            // Empty message
            network[1]->sendSingle(network[0]->getNodeId(), "");
            network.runFor(200);
            
            // Short message
            network[1]->sendSingle(network[0]->getNodeId(), "Hi");
            network.runFor(200);
            
            // Long message
            std::string longMsg = "This is a much longer message with spaces and punctuation! Testing 123.";
            network[1]->sendSingle(network[0]->getNodeId(), longMsg);
            network.runFor(200);
            
            // Message with special characters
            network[1]->sendSingle(network[0]->getNodeId(), "Special: @#$%^&*()");
            network.runFor(200);

            THEN("All messages are echoed exactly") {
                REQUIRE(receivedMessages.size() >= 4);
                
                // Check that messages match (order might vary slightly)
                bool foundEmpty = false;
                bool foundShort = false;
                bool foundLong = false;
                bool foundSpecial = false;
                
                for (const auto& msg : receivedMessages) {
                    if (msg == "") foundEmpty = true;
                    if (msg == "Hi") foundShort = true;
                    if (msg == longMsg) foundLong = true;
                    if (msg == "Special: @#$%^&*()") foundSpecial = true;
                }
                
                REQUIRE(foundEmpty);
                REQUIRE(foundShort);
                REQUIRE(foundLong);
                REQUIRE(foundSpecial);
            }
        }

        network.stop();
    }
}

SCENARIO("echoNode example - identifies sender correctly") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;

    GIVEN("An echo node that tracks sender IDs") {
        SimulatedMeshNetwork network(&scheduler, 3, io_service);
        
        uint32_t lastSenderReceived = 0;
        network[0]->setReceiveCallback([&](uint32_t from, std::string msg) {
            lastSenderReceived = from;
            network[0]->sendSingle(from, msg);
        });

        WHEN("Different nodes send messages") {
            network.waitForFullMesh(5000);
            
            // Node 1 sends
            network[1]->sendSingle(network[0]->getNodeId(), "From 1");
            network.runFor(300);
            uint32_t senderAfterNode1 = lastSenderReceived;
            
            // Node 2 sends
            network[2]->sendSingle(network[0]->getNodeId(), "From 2");
            network.runFor(300);
            uint32_t senderAfterNode2 = lastSenderReceived;

            THEN("Echo node correctly identifies each sender") {
                REQUIRE(senderAfterNode1 == network[1]->getNodeId());
                REQUIRE(senderAfterNode2 == network[2]->getNodeId());
                REQUIRE(senderAfterNode1 != senderAfterNode2);
            }
        }

        network.stop();
    }
}
