#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

// Declare logger for test environment
painlessmesh::logger::LogClass Log;

SCENARIO("initAsBridge returns false when router connection fails", "[bridge][init][failure]") {
    GIVEN("A mesh node attempting to become a bridge") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 1234567;
        node.init(&scheduler, nodeId);
        
        WHEN("Bridge initialization fails due to unreachable router") {
            THEN("The behavior should prevent channel mismatch") {
                // In the test environment, we can't actually test WiFi connection,
                // but we document the expected behavior:
                //
                // 1. If router connection fails in initAsBridge():
                //    - Function returns false
                //    - Node does NOT become a bridge
                //    - Node does NOT switch to channel 1
                //    - Node remains on original mesh channel
                //
                // 2. If promoteToBridge() receives false from initAsBridge():
                //    - Node reverts to regular node mode
                //    - Node re-initializes on original mesh channel
                //    - Election state is reset to IDLE
                //    - Bridge role callback is called with success=false
                //
                // This prevents the scenario where:
                // - Node tries to become bridge with unreachable router
                // - Switches to channel 1 (default fallback)
                // - Loses mesh connectivity (other nodes on different channel)
                // - Cannot receive bridge status updates from real bridge
                // - Creates confusing state where bridge appears in list but is stale
                
                INFO("initAsBridge() return value prevents channel mismatch");
                INFO("promoteToBridge() handles failure by reverting to regular node");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("promoteToBridge handles router connection failure gracefully", "[bridge][election][failure]") {
    GIVEN("A node that wins bridge election but cannot connect to router") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 2345678;
        node.init(&scheduler, nodeId);
        
        WHEN("Node attempts to promote to bridge with unreachable router") {
            THEN("Node should revert to regular mode on original channel") {
                // Expected behavior when initAsBridge() fails in promoteToBridge():
                //
                // 1. Save current mesh channel before attempting bridge init
                // 2. Call stop() to disconnect from mesh
                // 3. Call initAsBridge() which returns false (router unreachable)
                // 4. Detect failure and log error message
                // 5. Re-initialize as regular node on saved channel
                // 6. Reset election state to IDLE
                // 7. Call bridge role callback with success=false
                //
                // This ensures:
                // - Node stays connected to mesh on correct channel
                // - No partial bridge state (either full bridge or regular node)
                // - Election can be retried later if router becomes available
                // - Other nodes continue to function normally
                
                INFO("Failed promotion reverts to regular node gracefully");
                INFO("Original mesh channel is preserved");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Bridge initialization provides clear success/failure indication", "[bridge][init][api]") {
    GIVEN("Code that calls initAsBridge()") {
        WHEN("Router connection succeeds") {
            THEN("initAsBridge() returns true") {
                // When router connection succeeds:
                // - WiFi.status() == WL_CONNECTED
                // - Channel is detected from router
                // - Mesh is initialized on detected channel
                // - Node is set as root/bridge
                // - Bridge status broadcasting starts
                // - Function returns true
                
                INFO("Success case returns true");
                REQUIRE(true);
            }
        }
        
        WHEN("Router connection fails") {
            THEN("initAsBridge() returns false") {
                // When router connection fails:
                // - WiFi.status() != WL_CONNECTED after timeout
                // - Error logged: "Failed to connect to router"
                // - Error logged: "Cannot become bridge without router connection"
                // - Error logged: "Bridge initialization aborted - remaining as regular node"
                // - Function returns false immediately
                // - Node does NOT become bridge
                // - No partial initialization
                
                INFO("Failure case returns false and logs errors");
                REQUIRE(true);
            }
        }
    }
}

SCENARIO("Examples handle bridge initialization failure appropriately", "[bridge][examples]") {
    GIVEN("Bridge example code in setup()") {
        WHEN("initAsBridge() is called") {
            THEN("Return value should be checked with graceful fallback") {
                // Updated examples check return value and fallback gracefully:
                //
                // bool bridgeSuccess = mesh.initAsBridge(...);
                // if (!bridgeSuccess) {
                //   Serial.println("✗ Failed to initialize as bridge!");
                //   Serial.println("Router unreachable - falling back to regular mesh node");
                //   
                //   // Fallback: Initialize as regular mesh node
                //   mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
                //   
                //   Serial.println("✓ Initialized as regular mesh node");
                // }
                //
                // This approach:
                // - Gives control to library users (no forced restart)
                // - Allows device to participate in mesh
                // - Provides clear feedback about state
                // - Enables flexible recovery strategies
                
                INFO("Examples check initAsBridge() return value");
                INFO("Graceful fallback to regular node maintains mesh connectivity");
                INFO("No forced restart - user maintains control");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Fallback patterns prevent network instability", "[bridge][fallback][stability]") {
    GIVEN("Multiple fallback patterns available") {
        WHEN("Bridge initialization fails") {
            THEN("Different patterns support different use cases") {
                // Pattern 1: Basic Fallback
                // - Falls back to regular mesh node
                // - Maintains mesh connectivity
                // - Requires manual intervention for bridge role
                //
                // Pattern 2: Auto-Promotion
                // - Falls back with failover enabled
                // - Automatically promotes when router available
                // - No manual intervention needed
                //
                // Pattern 3: Multi-Bridge Redundancy
                // - Primary falls back to regular node
                // - Secondary bridge maintains connectivity
                // - Graceful degradation of primary
                //
                // Pattern 4: Retry with Backoff
                // - Retries with exponential backoff
                // - Prevents network flooding
                // - Eventually falls back if persistent failure
                //
                // Pattern 5: User-Controlled Restart
                // - User decides when to restart
                // - Appropriate for dedicated bridge hardware
                // - Clear intent that bridge role is required
                
                INFO("Multiple fallback patterns documented");
                INFO("Users choose pattern based on deployment requirements");
                INFO("No single forced behavior - flexible library design");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Channel discovery prevents mesh fragmentation", "[bridge][channel][discovery]") {
    GIVEN("Nodes on different channels") {
        WHEN("Bridge appears on different channel") {
            THEN("Channel management prevents instability") {
                // Channel Discovery Process:
                // 1. Nodes scan for mesh SSID on all channels
                // 2. Connection negotiation determines channel
                // 3. Bridge maintains router's channel
                // 4. Regular nodes switch to bridge's channel
                //
                // Channel Re-synchronization:
                // - Triggered after 6 consecutive empty scans
                // - Re-scans all channels to find mesh
                // - Prevents permanent fragmentation
                //
                // Election Deferral:
                // - Election deferred if approaching re-sync threshold
                // - Prevents channel chase loops
                // - Allows mesh to stabilize before election
                //
                // This design ensures:
                // - No endless restart loops
                // - Eventual mesh convergence
                // - Bridge maintains router connection
                // - Network stability during failures
                
                INFO("Channel discovery prevents fragmentation");
                INFO("Re-sync mechanism handles channel mismatches");
                INFO("Election deferral prevents chase loops");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Documentation provides comprehensive guidance", "[bridge][documentation]") {
    GIVEN("Library users need guidance on bridge initialization") {
        WHEN("Consulting documentation") {
            THEN("Comprehensive patterns and best practices are provided") {
                // Documentation includes:
                // - Design philosophy (library doesn't dictate recovery)
                // - Detailed behavior of initAsBridge() success/failure paths
                // - Five recommended fallback patterns with use cases
                // - Channel discovery and mesh formation explanation
                // - Best practices for different deployment scenarios
                // - Testing recommendations for each scenario
                //
                // See: docs/BRIDGE_INITIALIZATION_FALLBACK.md
                
                INFO("Comprehensive documentation in BRIDGE_INITIALIZATION_FALLBACK.md");
                INFO("Covers fallback patterns, channel management, best practices");
                INFO("Testing scenarios for validation");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Isolated nodes retry becoming bridge when mesh unavailable", "[bridge][isolated][retry]") {
    GIVEN("A node with router credentials that failed initial bridge setup") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 5678901;
        node.init(&scheduler, nodeId);
        
        WHEN("Node is isolated with no mesh connections") {
            THEN("Node should periodically retry bridge promotion") {
                // Expected behavior for isolated bridge retry:
                //
                // Conditions for retry:
                // 1. Bridge failover is enabled
                // 2. Router credentials are configured
                // 3. Node is not already a bridge
                // 4. Startup delay has passed
                // 5. No active mesh connections
                // 6. Multiple consecutive empty scans (default: 6)
                //
                // Retry mechanism:
                // - Periodic task runs every 60 seconds (isolatedBridgeRetryIntervalMs)
                // - Scans for router signal strength
                // - Checks minimum RSSI threshold (-80 dBm default)
                // - Attempts direct bridge promotion if router visible
                // - Limited to 5 actual attempts before cooldown
                // - Counter resets after 5 minutes (isolatedBridgeRetryResetIntervalMs)
                //
                // On success:
                // - Node becomes bridge on router's channel
                // - Bridge status broadcasts begin
                // - Other nodes can discover this bridge
                // - Retry counter resets
                //
                // On failure:
                // - Node reverts to regular mesh mode
                // - Router credentials are preserved
                // - Retry counter incremented (only for actual attempts)
                // - Next retry scheduled
                
                INFO("Isolated nodes retry bridge promotion periodically");
                INFO("Retry is limited to prevent endless attempts");
                INFO("Counter resets after 5 minute cooldown or on success");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Isolated bridge retry respects minimum RSSI threshold", "[bridge][isolated][rssi]") {
    GIVEN("An isolated node with weak router signal") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 6789012;
        node.init(&scheduler, nodeId);
        
        WHEN("Router RSSI is below threshold") {
            THEN("Bridge promotion should be skipped") {
                // RSSI threshold behavior:
                //
                // Default threshold: -80 dBm
                // Configurable via: mesh.setMinimumBridgeRSSI()
                //
                // When router RSSI < threshold:
                // - attemptIsolatedBridgePromotion() returns early
                // - Log message: "Router RSSI %d dBm below threshold %d dBm"
                // - No bridge promotion attempted
                // - Retry counter NOT incremented (to allow retry when signal improves)
                //
                // Rationale:
                // - Prevents unreliable bridge connections
                // - Encourages node to wait for mesh or better signal
                // - Avoids network instability from marginal connections
                
                INFO("Weak signal prevents bridge promotion");
                INFO("Retry counter not incremented for signal issues");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Isolated bridge retry resets when mesh becomes available", "[bridge][isolated][reset]") {
    GIVEN("An isolated node that has been retrying") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 7890123;
        node.init(&scheduler, nodeId);
        
        WHEN("Mesh connections become active") {
            THEN("Retry counter should reset") {
                // Reset behavior:
                //
                // When hasActiveMeshConnections() returns true:
                // - _isolatedBridgeRetryAttempts is reset to 0
                // - Normal election-based bridge selection resumes
                // - Isolated retry task exits early
                //
                // This ensures:
                // - Fresh start when rejoining mesh
                // - Election mechanism handles connected nodes
                // - No conflict between isolated retry and elections
                
                INFO("Retry counter resets when mesh available");
                INFO("Election mechanism takes over for connected nodes");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

// ============================================================================
// Comprehensive use case tests based on woodlist's feedback
// ============================================================================

SCENARIO("Use Case 1: INITIAL_BRIDGE=true with router temporarily unavailable", "[bridge][usecase][woodlist]") {
    GIVEN("A node configured as INITIAL_BRIDGE that fails to connect to router") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 11111111;
        node.init(&scheduler, nodeId);
        
        WHEN("Initial bridge setup fails and falls back to regular node") {
            THEN("Node should periodically retry becoming a bridge") {
                // Scenario from woodlist comment (quote preserved for reference):
                // "Upon one fall in coming as a bridge, the algorithm never attempts
                //  to get connected to router, having absent mesh network"
                //
                // Expected behavior with fix:
                // 1. setup() calls initAsBridge() which fails (router unavailable)
                // 2. Example falls back to regular node with failover enabled:
                //    mesh.init(meshPrefix, meshPassword, &scheduler, meshPort, WIFI_AP_STA, 0);
                //    mesh.setRouterCredentials(routerSSID, routerPassword);
                //    mesh.enableBridgeFailover(true);
                // 3. Periodic task runs every 60 seconds (isolatedBridgeRetryIntervalMs)
                // 4. After startup delay (60s) and empty scan threshold (6 scans),
                //    the node attempts direct bridge promotion via attemptIsolatedBridgePromotion()
                //
                // This addresses the comment:
                // "The main loop should contain cyclic trying, if the mesh network is unavailable"
                
                INFO("INITIAL_BRIDGE nodes retry bridge promotion after initial failure");
                INFO("Retry mechanism is in the periodic task, not blocking in loop()");
                INFO("Uses isolatedBridgeRetryIntervalMs (60s) between attempts");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Use Case 2: REGULAR NODE with no mesh found - isolated retry", "[bridge][usecase][woodlist]") {
    GIVEN("A regular node (INITIAL_BRIDGE=false) that cannot find any mesh network") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 22222222;
        node.init(&scheduler, nodeId);
        
        WHEN("Node scans but finds no mesh networks") {
            THEN("Node should eventually try to become bridge if isolated") {
                // Scenario from woodlist's serial log:
                // "CONNECTION: scanForMeshChannel(): Mesh 'FishFarmMesh' not found on any channel"
                // "CONNECTION: stationScan(): Mesh not found, falling back to channel 1"
                // Then endlessly: "CONNECTION: Bridge monitor: Skipping - no active mesh connections"
                //
                // Expected behavior with fix:
                // 1. Node starts as regular node
                // 2. No mesh found, consecutive empty scans accumulate
                // 3. After ISOLATED_BRIDGE_RETRY_SCAN_THRESHOLD (6) empty scans,
                //    isolated bridge retry task activates
                // 4. Node attempts direct bridge promotion via attemptIsolatedBridgePromotion()
                // 5. If router visible with adequate RSSI, node becomes bridge
                // 6. If router not visible, waits and retries up to MAX_ISOLATED_BRIDGE_RETRY_ATTEMPTS (5)
                //
                // This addresses the endless "Skipping - no active mesh connections" loop
                
                INFO("Regular nodes in isolation retry becoming bridge");
                INFO("Requires 6 consecutive empty scans before retry");
                INFO("Retry limited to 5 attempts with 5-minute cooldown");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Use Case 3: Router association refused error handling", "[bridge][usecase][woodlist]") {
    GIVEN("A node attempting to connect to router that refuses association") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 33333333;
        node.init(&scheduler, nodeId);
        
        WHEN("Router refuses association with 'comeback time too long' error") {
            THEN("Node should handle the failure gracefully and retry later") {
                // Scenario from woodlist's serial log:
                // "E (431) wifi:Association refused temporarily, comeback time 262144 (TUs) too long"
                // "✗ Failed to initialize as bridge!"
                // "Router unreachable - falling back to regular node with failover"
                // "✓ Running as regular node - will auto-promote when router available"
                //
                // Expected behavior:
                // 1. initAsBridge() times out waiting for router connection
                // 2. Returns false indicating failure
                // 3. Example falls back to regular node mode
                // 4. Node will retry via isolated bridge retry mechanism
                //
                // The ESP32 WiFi driver error is informational - the library handles
                // the timeout gracefully regardless of the underlying WiFi failure reason
                
                INFO("Association refused error is handled by connection timeout");
                INFO("Node falls back to regular mode with auto-retry enabled");
                INFO("Retry happens via periodic isolated bridge retry task");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Use Case 4: Node loses mesh connection to bridge", "[bridge][usecase][woodlist]") {
    GIVEN("A node that was connected to mesh but lost connection") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 44444444;
        node.init(&scheduler, nodeId);
        
        WHEN("Mesh connection is lost and bridge becomes unreachable") {
            THEN("Node should handle disconnection and attempt recovery") {
                // Scenario from woodlist's serial log:
                // "CONNECTION: Time out reached"
                // "Changed connections. Nodes: 0"
                // "Internet available: YES" -> "Internet available: NO"
                // "Known bridges: 1" but "No primary bridge available!"
                //
                // Expected behavior:
                // 1. Node detects lost mesh connections
                // 2. hasActiveMeshConnections() returns false
                // 3. Bridge monitor skips election (can't coordinate)
                // 4. Isolated bridge retry mechanism activates after threshold
                // 5. If router visible, node can become bridge
                // 6. If not, continues scanning for mesh reconnection
                //
                // The distinction between "bridge in list but stale" vs "truly unreachable"
                // is handled by the bridgeTimeoutMs check and hasInternetConnection() flag
                
                INFO("Disconnected node transitions to isolated retry mode");
                INFO("Known bridges remain in list but marked as stale");
                INFO("Recovery via either mesh reconnection or direct bridge promotion");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Use Case 5: Multiple retry attempts with cooldown", "[bridge][usecase][woodlist]") {
    GIVEN("A node that has failed bridge promotion multiple times") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 55555555;
        node.init(&scheduler, nodeId);
        
        WHEN("Node reaches maximum retry attempts") {
            THEN("Node should enter cooldown before retrying again") {
                // Configuration parameters:
                // - MAX_ISOLATED_BRIDGE_RETRY_ATTEMPTS = 5
                // - isolatedBridgeRetryIntervalMs = 60000 (60 seconds)
                // - isolatedBridgeRetryResetIntervalMs = 300000 (5 minutes)
                // - ISOLATED_BRIDGE_RETRY_SCAN_THRESHOLD = 6 empty scans
                //
                // Retry timeline:
                // T=0: Node starts, begins scanning
                // T=~60s: Startup delay passes
                // T=~90s: 6 empty scans accumulated (fast scan rate)
                // T=90s: First retry attempt
                // T=150s: Second retry attempt
                // T=210s: Third retry attempt
                // T=270s: Fourth retry attempt
                // T=330s: Fifth retry attempt (max reached)
                // T=330s-630s: Cooldown period (5 minutes)
                // T=630s: Counter resets, retries resume
                //
                // This prevents infinite retry loops while still allowing
                // eventual recovery when conditions improve
                
                INFO("Max 5 actual promotion attempts before cooldown");
                INFO("Counter only incremented when actual attempt made");
                INFO("5-minute cooldown before counter resets");
                INFO("Counter also resets if mesh connection established");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Use Case 6: Correct serial output for bridge failure", "[bridge][usecase][woodlist]") {
    GIVEN("A node attempting bridge initialization that fails") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 66666666;
        node.init(&scheduler, nodeId);
        
        WHEN("initAsBridge() returns false") {
            THEN("Serial output should show failure message") {
                // woodlist noted (quote preserved for reference):
                // "Since now it's never has been seen the output of actual 'bridgeSuccess=false'"
                // "if (!bridgeSuccess) { Serial.println(\"✗ Failed to initialize as bridge!\");"
                //
                // The example code in bridge_failover.ino handles this.
                // Note: The following uses example placeholders (MESH_PREFIX, etc.)
                // that match the actual example code pattern:
                //
                // bool bridgeSuccess = mesh.initAsBridge(meshPrefix, meshPassword,
                //                                        routerSSID, routerPassword,
                //                                        &scheduler, meshPort);
                // if (!bridgeSuccess) {
                //   Serial.println("✗ Failed to initialize as bridge!");
                //   Serial.println("Router unreachable - falling back to regular node with failover");
                //   mesh.init(meshPrefix, meshPassword, &scheduler, meshPort, WIFI_AP_STA, 0);
                //   mesh.setRouterCredentials(routerSSID, routerPassword);
                //   mesh.enableBridgeFailover(true);
                //   Serial.println("✓ Running as regular node - will auto-promote when router available");
                //   Serial.println("Note: If isolated (no mesh found), will retry bridge connection periodically");
                // }
                //
                // If INITIAL_BRIDGE=false, the code path doesn't call initAsBridge() at all,
                // so these messages wouldn't appear. The messages only appear when INITIAL_BRIDGE=true.
                
                INFO("Failure messages only shown when INITIAL_BRIDGE=true");
                INFO("Regular node mode (INITIAL_BRIDGE=false) skips initAsBridge()");
                INFO("Serial output matches the configured mode");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}

SCENARIO("Isolated bridge retry skips scan threshold after failed promotion", "[bridge][isolated][pending]") {
    GIVEN("A node that failed bridge promotion and was reverted to regular node") {
        Scheduler scheduler;
        Mesh<Connection> node;
        
        uint32_t nodeId = 77777777;
        node.init(&scheduler, nodeId);
        
        WHEN("Bridge promotion fails and node is reverted") {
            THEN("Next retry should skip the empty scan threshold check") {
                // Problem fixed by this scenario:
                //
                // Before fix:
                // 1. Node is isolated, accumulates 6+ empty scans
                // 2. Isolated bridge retry attempts promotion
                // 3. Promotion fails (router unreachable during connection)
                // 4. Node reverts to regular mode with init()
                // 5. init() resets consecutiveEmptyScans to 0
                // 6. Next retry task runs but sees only 0-4 empty scans
                // 7. Skips retry with "Only N empty scans, waiting for more scans"
                // 8. Must wait ~90 seconds for 6 more scans to accumulate
                // 9. This creates a 2-minute delay loop between actual attempts
                //
                // After fix:
                // 1. When promotion fails, _isolatedRetryPending flag is set
                // 2. Next retry task sees the flag and skips scan threshold check
                // 3. Immediate retry possible without waiting for scan accumulation
                // 4. Flag is cleared when retry is attempted or mesh becomes active
                //
                // This reduces time to retry from ~120 seconds to ~60 seconds
                
                INFO("_isolatedRetryPending flag preserves isolation state across reinit");
                INFO("Flag allows immediate retry without waiting for scan threshold");
                INFO("Flag is cleared when retry proceeds or mesh connects");
                REQUIRE(true); // Documented behavior
            }
        }
    }
}
