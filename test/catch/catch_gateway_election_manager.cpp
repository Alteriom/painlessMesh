#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "painlessmesh/gateway.hpp"

using namespace painlessmesh;
using namespace painlessmesh::gateway;

// Logger for test environment
painlessmesh::logger::LogClass Log;

SCENARIO("GatewayElectionManager has correct defaults") {
    GIVEN("A default GatewayElectionManager") {
        GatewayElectionManager election;

        THEN("state should be IDLE") {
            REQUIRE(election.getState() == GatewayElectionManager::ElectionState::IDLE);
        }

        THEN("isElectedPrimary should be false") {
            REQUIRE(election.isElectedPrimary() == false);
        }

        THEN("getPrimaryGatewayId should return 0") {
            REQUIRE(election.getPrimaryGatewayId() == 0);
        }

        THEN("getCandidateCount should return 0") {
            REQUIRE(election.getCandidateCount() == 0);
        }
    }
}

SCENARIO("GatewayElectionManager configuration works correctly") {
    GIVEN("A GatewayElectionManager") {
        GatewayElectionManager election;

        WHEN("Setting node ID") {
            election.setNodeId(12345);

            THEN("Node should be ready for election") {
                // Node ID is set but cannot participate without Internet
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::IDLE);
            }
        }

        WHEN("Configuring with SharedGatewayConfig") {
            SharedGatewayConfig config;
            config.gatewayFailureTimeout = 30000;
            election.configure(config);

            THEN("Configuration should be accepted") {
                // Just verifying no crash - internal config is private
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::IDLE);
            }
        }

        WHEN("Setting local candidate") {
            election.setNodeId(12345);
            election.setLocalCandidate(true, -45);

            THEN("State should still be IDLE") {
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::IDLE);
            }
        }
    }
}

SCENARIO("GatewayElectionManager starts election correctly") {
    GIVEN("A configured GatewayElectionManager with Internet capability") {
        GatewayElectionManager election;
        election.setNodeId(12345);
        election.setLocalCandidate(true, -45);

        WHEN("Starting an election") {
            election.startElection();

            THEN("state should be ELECTION_RUNNING") {
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::ELECTION_RUNNING);
            }

            THEN("getCandidateCount should be 1 (self)") {
                REQUIRE(election.getCandidateCount() == 1);
            }

            THEN("isElectedPrimary should be false until election completes") {
                REQUIRE(election.isElectedPrimary() == false);
            }
        }
    }

    GIVEN("A GatewayElectionManager without Internet capability") {
        GatewayElectionManager election;
        election.setNodeId(12345);
        election.setLocalCandidate(false, -45);

        WHEN("Starting an election") {
            election.startElection();

            THEN("state should be ELECTION_RUNNING (election runs but node is not a candidate)") {
                // startElection() transitions to ELECTION_RUNNING regardless of Internet capability
                // However, the node won't add itself as a candidate without Internet
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::ELECTION_RUNNING);
            }

            THEN("getCandidateCount should be 0 (no valid candidates)") {
                REQUIRE(election.getCandidateCount() == 0);
            }
        }
    }
}

SCENARIO("GatewayElectionManager processes heartbeats correctly") {
    GIVEN("A GatewayElectionManager in IDLE state") {
        GatewayElectionManager election;
        election.setNodeId(12345);
        election.setLocalCandidate(true, -50);
        uint32_t currentTime = 1000;

        WHEN("Receiving a heartbeat from a primary gateway") {
            GatewayHeartbeatPackage heartbeat;
            heartbeat.from = 99999;
            heartbeat.isPrimary = true;
            heartbeat.hasInternet = true;
            heartbeat.routerRSSI = -40;
            heartbeat.uptime = 86400;

            election.processHeartbeat(heartbeat, currentTime);

            THEN("Primary gateway ID should be updated") {
                REQUIRE(election.getPrimaryGatewayId() == 99999);
            }

            THEN("State should remain IDLE") {
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::IDLE);
            }

            THEN("This node should not be elected primary") {
                REQUIRE(election.isElectedPrimary() == false);
            }
        }

        WHEN("Receiving a heartbeat from a non-primary gateway with Internet") {
            GatewayHeartbeatPackage heartbeat;
            heartbeat.from = 88888;
            heartbeat.isPrimary = false;
            heartbeat.hasInternet = true;
            heartbeat.routerRSSI = -35;
            heartbeat.uptime = 3600;

            election.processHeartbeat(heartbeat, currentTime);

            THEN("Primary gateway ID should remain 0") {
                REQUIRE(election.getPrimaryGatewayId() == 0);
            }

            THEN("Candidate should be tracked") {
                // Cannot directly verify, but no crash means success
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::IDLE);
            }
        }
    }
}

SCENARIO("GatewayElectionManager election winner selection is deterministic") {
    GIVEN("A GatewayElectionManager with multiple candidates") {
        GatewayElectionManager election;
        election.setNodeId(10000);
        election.setLocalCandidate(true, -60);  // Weakest signal
        election.setElectionDuration(0);  // Immediate election completion
        uint32_t currentTime = 1000;

        // Start election
        election.startElection(currentTime);

        // Add other candidates via heartbeats
        GatewayHeartbeatPackage hb1;
        hb1.from = 20000;
        hb1.isPrimary = false;
        hb1.hasInternet = true;
        hb1.routerRSSI = -40;  // Best signal
        election.processHeartbeat(hb1, currentTime);

        GatewayHeartbeatPackage hb2;
        hb2.from = 30000;
        hb2.isPrimary = false;
        hb2.hasInternet = true;
        hb2.routerRSSI = -50;  // Medium signal
        election.processHeartbeat(hb2, currentTime);

        WHEN("Election completes") {
            bool shouldBroadcast = election.update(currentTime);

            THEN("Node with highest RSSI wins") {
                REQUIRE(election.getPrimaryGatewayId() == 20000);
            }

            THEN("Local node should not be primary") {
                REQUIRE(election.isElectedPrimary() == false);
            }

            THEN("Should not broadcast as primary") {
                REQUIRE(shouldBroadcast == false);
            }

            THEN("State should be COOLDOWN") {
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::COOLDOWN);
            }
        }
    }
}

SCENARIO("GatewayElectionManager handles RSSI tie with node ID tiebreaker") {
    GIVEN("Two candidates with same RSSI") {
        GatewayElectionManager election;
        election.setNodeId(10000);  // Lower node ID
        election.setLocalCandidate(true, -45);
        election.setElectionDuration(0);
        uint32_t currentTime = 1000;

        election.startElection(currentTime);

        GatewayHeartbeatPackage hb1;
        hb1.from = 20000;  // Higher node ID
        hb1.isPrimary = false;
        hb1.hasInternet = true;
        hb1.routerRSSI = -45;  // Same RSSI as local
        election.processHeartbeat(hb1, currentTime);

        WHEN("Election completes") {
            bool shouldBroadcast = election.update(currentTime);

            THEN("Higher node ID wins the tiebreaker") {
                REQUIRE(election.getPrimaryGatewayId() == 20000);
            }

            THEN("Local node should not be primary") {
                REQUIRE(election.isElectedPrimary() == false);
            }

            THEN("Should not broadcast as primary") {
                REQUIRE(shouldBroadcast == false);
            }
        }
    }

    GIVEN("Local node has higher node ID with same RSSI") {
        GatewayElectionManager election;
        election.setNodeId(30000);  // Higher node ID
        election.setLocalCandidate(true, -45);
        election.setElectionDuration(0);
        uint32_t currentTime = 1000;

        election.startElection(currentTime);

        GatewayHeartbeatPackage hb1;
        hb1.from = 20000;  // Lower node ID
        hb1.isPrimary = false;
        hb1.hasInternet = true;
        hb1.routerRSSI = -45;  // Same RSSI as local
        election.processHeartbeat(hb1, currentTime);

        WHEN("Election completes") {
            bool shouldBroadcast = election.update(currentTime);

            THEN("Local node (higher ID) wins the tiebreaker") {
                REQUIRE(election.getPrimaryGatewayId() == 30000);
            }

            THEN("Local node should be elected primary") {
                REQUIRE(election.isElectedPrimary() == true);
            }

            THEN("Should broadcast as primary") {
                REQUIRE(shouldBroadcast == true);
            }
        }
    }
}

SCENARIO("GatewayElectionManager cooldown prevents rapid re-elections") {
    GIVEN("A GatewayElectionManager that just completed an election") {
        GatewayElectionManager election;
        election.setNodeId(12345);
        election.setLocalCandidate(true, -45);
        election.setElectionDuration(0);
        election.setCooldownPeriod(60000);  // 60 second cooldown
        uint32_t currentTime = 1000;

        election.startElection(currentTime);
        election.update(currentTime);  // Complete the election

        REQUIRE(election.getState() == GatewayElectionManager::ElectionState::COOLDOWN);

        WHEN("Trying to start another election during cooldown") {
            election.startElection(currentTime);

            THEN("State should remain in COOLDOWN") {
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::COOLDOWN);
            }
        }

        WHEN("Cooldown period elapses") {
            // Simulate time passing beyond cooldown
            uint32_t afterCooldown = currentTime + 60001;  // Just past cooldown
            election.update(afterCooldown);

            THEN("State should return to IDLE") {
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::IDLE);
            }

            AND_WHEN("Starting a new election after cooldown") {
                election.startElection(afterCooldown);

                THEN("Election should start") {
                    REQUIRE(election.getState() == GatewayElectionManager::ElectionState::ELECTION_RUNNING);
                }
            }
        }
    }
}

SCENARIO("GatewayElectionManager split-brain prevention during election") {
    GIVEN("A GatewayElectionManager in ELECTION_RUNNING state") {
        GatewayElectionManager election;
        election.setNodeId(10000);
        election.setLocalCandidate(true, -50);  // Local RSSI
        election.setElectionDuration(10000);  // Long election to test split-brain
        uint32_t currentTime = 1000;

        election.startElection(currentTime);
        REQUIRE(election.getState() == GatewayElectionManager::ElectionState::ELECTION_RUNNING);

        WHEN("Receiving heartbeat from node claiming primary with higher RSSI") {
            GatewayHeartbeatPackage hb;
            hb.from = 99999;
            hb.isPrimary = true;
            hb.hasInternet = true;
            hb.routerRSSI = -40;  // Better than local -50
            hb.uptime = 3600;

            election.processHeartbeat(hb, currentTime);

            THEN("Should defer to the higher priority primary") {
                REQUIRE(election.getPrimaryGatewayId() == 99999);
            }

            THEN("Local node should not be primary") {
                REQUIRE(election.isElectedPrimary() == false);
            }

            THEN("Should transition to COOLDOWN") {
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::COOLDOWN);
            }
        }

        WHEN("Receiving heartbeat from node claiming primary with lower RSSI") {
            GatewayHeartbeatPackage hb;
            hb.from = 99999;
            hb.isPrimary = true;
            hb.hasInternet = true;
            hb.routerRSSI = -60;  // Worse than local -50
            hb.uptime = 3600;

            election.processHeartbeat(hb, currentTime);

            THEN("Should NOT defer to lower priority primary") {
                // Primary ID might be set but election continues
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::ELECTION_RUNNING);
            }
        }
    }
}

SCENARIO("GatewayElectionManager reset clears all state") {
    GIVEN("A GatewayElectionManager with state") {
        GatewayElectionManager election;
        election.setNodeId(12345);
        election.setLocalCandidate(true, -45);
        election.setElectionDuration(0);
        uint32_t currentTime = 1000;

        election.startElection(currentTime);
        election.update(currentTime);  // Complete election

        // Verify we have state
        REQUIRE(election.getState() == GatewayElectionManager::ElectionState::COOLDOWN);
        REQUIRE(election.getPrimaryGatewayId() == 12345);
        REQUIRE(election.isElectedPrimary() == true);

        WHEN("Calling reset()") {
            election.reset();

            THEN("State should be IDLE") {
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::IDLE);
            }

            THEN("Primary gateway ID should be 0") {
                REQUIRE(election.getPrimaryGatewayId() == 0);
            }

            THEN("isElectedPrimary should be false") {
                REQUIRE(election.isElectedPrimary() == false);
            }

            THEN("Candidate count should be 0") {
                REQUIRE(election.getCandidateCount() == 0);
            }
        }
    }
}

SCENARIO("GatewayElectionManager election result callback is invoked") {
    GIVEN("A GatewayElectionManager with callback registered") {
        GatewayElectionManager election;
        election.setNodeId(12345);
        election.setLocalCandidate(true, -45);
        election.setElectionDuration(0);
        uint32_t currentTime = 1000;

        uint32_t callbackWinner = 0;
        bool callbackIsLocal = false;
        bool callbackInvoked = false;

        election.onElectionResult([&](uint32_t winnerId, bool isLocal) {
            callbackWinner = winnerId;
            callbackIsLocal = isLocal;
            callbackInvoked = true;
        });

        WHEN("Election completes with local node as winner") {
            election.startElection(currentTime);
            election.update(currentTime);

            THEN("Callback should be invoked") {
                REQUIRE(callbackInvoked == true);
            }

            THEN("Callback should receive correct winner") {
                REQUIRE(callbackWinner == 12345);
            }

            THEN("Callback should indicate local node won") {
                REQUIRE(callbackIsLocal == true);
            }
        }
    }

    GIVEN("A GatewayElectionManager with remote winner") {
        GatewayElectionManager election;
        election.setNodeId(10000);
        election.setLocalCandidate(true, -60);  // Weak signal
        election.setElectionDuration(0);
        uint32_t currentTime = 1000;

        uint32_t callbackWinner = 0;
        bool callbackIsLocal = true;  // Default to true to verify it changes
        bool callbackInvoked = false;

        election.onElectionResult([&](uint32_t winnerId, bool isLocal) {
            callbackWinner = winnerId;
            callbackIsLocal = isLocal;
            callbackInvoked = true;
        });

        election.startElection(currentTime);

        // Add stronger candidate
        GatewayHeartbeatPackage hb;
        hb.from = 99999;
        hb.isPrimary = false;
        hb.hasInternet = true;
        hb.routerRSSI = -40;  // Much stronger
        election.processHeartbeat(hb, currentTime);

        WHEN("Election completes with remote node as winner") {
            election.update(currentTime);

            THEN("Callback should be invoked") {
                REQUIRE(callbackInvoked == true);
            }

            THEN("Callback should receive correct winner") {
                REQUIRE(callbackWinner == 99999);
            }

            THEN("Callback should indicate local node did NOT win") {
                REQUIRE(callbackIsLocal == false);
            }
        }
    }
}

SCENARIO("GatewayElectionManager handles no valid candidates") {
    GIVEN("A GatewayElectionManager where no candidates have Internet") {
        GatewayElectionManager election;
        election.setNodeId(12345);
        election.setLocalCandidate(false, -45);  // No Internet
        election.setElectionDuration(0);
        uint32_t currentTime = 1000;

        election.startElection(currentTime);

        // Add candidate without Internet via heartbeat
        GatewayHeartbeatPackage hb;
        hb.from = 99999;
        hb.isPrimary = false;
        hb.hasInternet = false;  // No Internet
        hb.routerRSSI = -40;
        election.processHeartbeat(hb, currentTime);

        WHEN("Election completes") {
            election.update(currentTime);

            THEN("Primary gateway ID should be 0") {
                REQUIRE(election.getPrimaryGatewayId() == 0);
            }

            THEN("Local node should not be primary") {
                REQUIRE(election.isElectedPrimary() == false);
            }

            THEN("State should be COOLDOWN") {
                REQUIRE(election.getState() == GatewayElectionManager::ElectionState::COOLDOWN);
            }
        }
    }
}

SCENARIO("GatewayElectionManager only Internet-capable nodes can be candidates") {
    GIVEN("An election with mixed Internet capability") {
        GatewayElectionManager election;
        election.setNodeId(10000);
        election.setLocalCandidate(true, -70);  // Internet but weak signal
        election.setElectionDuration(0);

        election.startElection();

        // Add candidate with better signal but no Internet
        GatewayHeartbeatPackage hb1;
        hb1.from = 20000;
        hb1.isPrimary = false;
        hb1.hasInternet = false;  // No Internet - should not be eligible
        hb1.routerRSSI = -30;     // Much stronger signal
        election.processHeartbeat(hb1);

        // Add candidate with Internet but weaker than above
        GatewayHeartbeatPackage hb2;
        hb2.from = 30000;
        hb2.isPrimary = false;
        hb2.hasInternet = true;
        hb2.routerRSSI = -50;  // Stronger than local but weaker than node 20000
        election.processHeartbeat(hb2);

        WHEN("Election completes") {
            election.update(1000);

            THEN("Winner should be node with Internet and best RSSI") {
                // Node 30000 has best RSSI among Internet-capable nodes
                REQUIRE(election.getPrimaryGatewayId() == 30000);
            }

            THEN("Node without Internet should not win despite better RSSI") {
                REQUIRE(election.getPrimaryGatewayId() != 20000);
            }
        }
    }
}
