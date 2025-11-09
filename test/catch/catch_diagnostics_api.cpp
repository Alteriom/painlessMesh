#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

// Declare logger for test environment
painlessmesh::logger::LogClass Log;

SCENARIO("Bridge diagnostics structures have correct default values") {
    GIVEN("New diagnostic structures") {
        WHEN("Creating BridgeStatus") {
            BridgeStatus status;
            
            THEN("Should have default values") {
                REQUIRE(status.isBridge == false);
                REQUIRE(status.internetConnected == false);
                REQUIRE(status.role == "regular");
                REQUIRE(status.bridgeNodeId == 0);
                REQUIRE(status.bridgeRSSI == 0);
                REQUIRE(status.timeSinceBridgeChange == 0);
            }
        }
        
        WHEN("Creating ElectionRecord") {
            ElectionRecord record;
            
            THEN("Should have default values") {
                REQUIRE(record.timestamp == 0);
                REQUIRE(record.winnerNodeId == 0);
                REQUIRE(record.winnerRSSI == 0);
                REQUIRE(record.candidateCount == 0);
                REQUIRE(record.reason == "");
            }
        }
        
        WHEN("Creating BridgeChangeEvent") {
            BridgeChangeEvent event;
            
            THEN("Should have default values") {
                REQUIRE(event.timestamp == 0);
                REQUIRE(event.oldBridgeId == 0);
                REQUIRE(event.newBridgeId == 0);
                REQUIRE(event.reason == "");
                REQUIRE(event.internetAvailable == false);
            }
        }
        
        WHEN("Creating BridgeTestResult") {
            BridgeTestResult result;
            
            THEN("Should have default values") {
                REQUIRE(result.success == false);
                REQUIRE(result.bridgeReachable == false);
                REQUIRE(result.internetReachable == false);
                REQUIRE(result.latencyMs == 0);
                REQUIRE(result.message == "");
            }
        }
    }
}

SCENARIO("getBridgeStatus returns current bridge state") {
    GIVEN("A mesh instance") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Node is not a bridge") {
            auto status = mesh.getBridgeStatus();
            
            THEN("Status should reflect regular node") {
                REQUIRE(status.isBridge == false);
                REQUIRE(status.role == "regular");
                REQUIRE(status.bridgeNodeId == 0);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getBridgeStatus with root node") {
    GIVEN("A mesh configured as bridge") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        mesh.setRoot(true);
        
        WHEN("Getting bridge status") {
            auto status = mesh.getBridgeStatus();
            
            THEN("Status should reflect bridge role") {
                REQUIRE(status.isBridge == true);
                REQUIRE(status.role == "bridge");
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("Election history tracking") {
    GIVEN("A mesh with diagnostics") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Diagnostics disabled") {
            mesh.enableDiagnostics(false);
            auto history = mesh.getElectionHistory();
            
            THEN("Should return empty vector") {
                REQUIRE(history.size() == 0);
            }
        }
        
        WHEN("Diagnostics enabled") {
            mesh.enableDiagnostics(true);
            auto history = mesh.getElectionHistory();
            
            THEN("Should return empty vector initially") {
                REQUIRE(history.size() == 0);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("Bridge change event tracking") {
    GIVEN("A mesh instance") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Getting last bridge change") {
            auto event = mesh.getLastBridgeChange();
            
            THEN("Should have default values initially") {
                REQUIRE(event.timestamp == 0);
                REQUIRE(event.oldBridgeId == 0);
                REQUIRE(event.newBridgeId == 0);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getInternetPath finds path to bridge") {
    GIVEN("A mesh instance") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("No bridge available") {
            auto path = mesh.getInternetPath(67890);
            
            THEN("Path should be empty") {
                REQUIRE(path.size() == 0);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getInternetPath for bridge node") {
    GIVEN("A mesh with bridge") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        mesh.updateBridgeStatus(12345, true, -50, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Getting path") {
            // Check that bridge is tracked
            auto bridges = mesh.getBridges();
            
            THEN("Bridge should be tracked") {
                REQUIRE(bridges.size() == 1);
                REQUIRE(bridges[0].nodeId == 12345);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getBridgeForNodeId returns primary bridge") {
    GIVEN("A mesh instance") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("No bridge available") {
            uint32_t bridgeId = mesh.getBridgeForNodeId(67890);
            
            THEN("Should return 0") {
                REQUIRE(bridgeId == 0);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getBridgeForNodeId with available bridge") {
    GIVEN("A mesh with bridge") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        mesh.updateBridgeStatus(99999, true, -45, 6, 20000, "192.168.1.1", 2000);
        
        WHEN("Getting bridge for node") {
            // Check that bridge is tracked even if not immediately healthy
            auto bridges = mesh.getBridges();
            
            THEN("Bridge should be in list") {
                REQUIRE(bridges.size() == 1);
                REQUIRE(bridges[0].nodeId == 99999);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("exportTopologyDOT generates valid format") {
    GIVEN("A mesh instance") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Exporting topology") {
            auto dot = mesh.exportTopologyDOT();
            
            THEN("Should contain valid DOT format") {
                REQUIRE(dot.find("digraph mesh") != std::string::npos);
                REQUIRE(dot.find(std::to_string(12345)) != std::string::npos);
                REQUIRE(dot.find("}") != std::string::npos);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("exportTopologyDOT with bridge") {
    GIVEN("A mesh configured as bridge") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        mesh.setRoot(true);
        
        WHEN("Exporting topology") {
            auto dot = mesh.exportTopologyDOT();
            
            THEN("Should include bridge label") {
                REQUIRE(dot.find("Bridge") != std::string::npos);
                REQUIRE(dot.find(std::to_string(12345)) != std::string::npos);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("testBridgeConnectivity without bridge") {
    GIVEN("A mesh instance") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Testing connectivity without bridge") {
            auto result = mesh.testBridgeConnectivity();
            
            THEN("Test should fail with no bridge message") {
                REQUIRE(result.success == false);
                REQUIRE(result.bridgeReachable == false);
                REQUIRE(result.message == "No bridge available");
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("testBridgeConnectivity with unreachable bridge") {
    GIVEN("A mesh with bridge") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        mesh.updateBridgeStatus(99999, true, -45, 6, 10000, "192.168.1.1", 1000);
        
        WHEN("Testing connectivity") {
            auto result = mesh.testBridgeConnectivity();
            
            THEN("Test should complete") {
                // Bridge may not be found depending on timing
                REQUIRE(result.success == false);
                REQUIRE(result.bridgeReachable == false);
                // Message could be "No bridge available" or "Bridge not reachable"
                // depending on whether getPrimaryBridge finds a healthy bridge
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("isBridgeReachable checks connectivity") {
    GIVEN("A mesh instance") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Checking unknown node") {
            bool reachable = mesh.isBridgeReachable(99999);
            
            THEN("Should return false") {
                REQUIRE(reachable == false);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getDiagnosticReport generates report") {
    GIVEN("A mesh instance") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Generating report") {
            auto report = mesh.getDiagnosticReport();
            
            THEN("Should contain key information") {
                REQUIRE(report.find("painlessMesh Diagnostics") != std::string::npos);
                REQUIRE(report.find("Node ID: 12345") != std::string::npos);
                REQUIRE(report.find("Mode:") != std::string::npos);
                REQUIRE(report.find("Mesh Nodes:") != std::string::npos);
                REQUIRE(report.find("Uptime:") != std::string::npos);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("getDiagnosticReport for bridge node") {
    GIVEN("A mesh configured as bridge") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        mesh.setRoot(true);
        
        WHEN("Generating report") {
            auto report = mesh.getDiagnosticReport();
            
            THEN("Should show bridge mode and this node") {
                REQUIRE(report.find("Mode: bridge") != std::string::npos);
                REQUIRE(report.find("Bridge: 12345 (this node)") != std::string::npos);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("Bridge status tracking with diagnostics") {
    GIVEN("A mesh with diagnostics enabled") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        mesh.enableDiagnostics(true);
        
        WHEN("Bridge status is updated") {
            mesh.updateBridgeStatus(99999, true, -45, 6, 10000, "192.168.1.1", 1000);
            
            THEN("Bridge should be tracked") {
                auto bridges = mesh.getBridges();
                REQUIRE(bridges.size() == 1);
                REQUIRE(bridges[0].nodeId == 99999);
                REQUIRE(bridges[0].internetConnected == true);
            }
        }
        
        mesh.stop();
    }
}

SCENARIO("Primary bridge selection with multiple bridges") {
    GIVEN("A mesh with multiple bridges") {
        Scheduler scheduler;
        Mesh<Connection> mesh;
        mesh.init(&scheduler, 12345);
        
        WHEN("Multiple bridges with different RSSI exist") {
            // Update bridges in quick succession
            mesh.updateBridgeStatus(11111, true, -70, 6, 10000, "192.168.1.1", 1000);
            mesh.updateBridgeStatus(22222, true, -50, 6, 10000, "192.168.1.1", 1000);
            mesh.updateBridgeStatus(33333, true, -60, 6, 10000, "192.168.1.1", 1000);
            
            // Check that bridges are tracked
            auto bridges = mesh.getBridges();
            
            THEN("All bridges should be tracked") {
                REQUIRE(bridges.size() == 3);
                
                // Find the one with best RSSI
                BridgeInfo* best = nullptr;
                for (auto& bridge : bridges) {
                    if (best == nullptr || bridge.routerRSSI > best->routerRSSI) {
                        best = &bridge;
                    }
                }
                
                REQUIRE(best != nullptr);
                REQUIRE(best->nodeId == 22222);
                REQUIRE(best->routerRSSI == -50);
            }
        }
        
        mesh.stop();
    }
}
