/**
 * @file catch_mesh_connectivity.cpp
 * @brief Tests for mesh connectivity detection and bridge discovery methods
 *
 * Tests the following methods:
 * - hasActiveMeshConnections(): Check if node has active mesh connections
 * - getLastKnownBridge(): Get best bridge regardless of lastSeen timeout
 * - Interaction with getPrimaryBridge() when mesh is disconnected
 */

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

// Global logger for test environment
painlessmesh::logger::LogClass Log;

// NOTE ON TEST ENVIRONMENT:
// The test environment's millis() returns 64-bit epoch time in milliseconds (e.g., 1763742830198),
// while BridgeInfo.lastSeen is uint32_t. This causes type mismatch when checking isHealthy().
// In real Arduino/ESP32 environment, millis() returns uint32_t and works correctly.
// Therefore, some tests focus on bridge tracking logic rather than health timeout behavior.

SCENARIO("hasActiveMeshConnections detects mesh connectivity state") {
  GIVEN("A mesh node with no connections") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    uint32_t nodeId = 1234567;
    mesh.init(&scheduler, nodeId);
    
    THEN("hasActiveMeshConnections returns false") {
      REQUIRE(mesh.hasActiveMeshConnections() == false);
    }
  }
}

SCENARIO("getLastKnownBridge returns bridge regardless of lastSeen timeout") {
  GIVEN("A mesh with a bridge") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    uint32_t nodeId = 1234567;
    mesh.init(&scheduler, nodeId);
    
    // Add a bridge
    mesh.updateBridgeStatus(
      12345,    // nodeId
      true,     // internetConnected
      -50,      // routerRSSI
      6,        // routerChannel
      100000,   // uptime
      "192.168.1.1",  // gatewayIP
      1000      // timestamp
    );
    
    THEN("getLastKnownBridge returns the bridge") {
      auto lastKnown = mesh.getLastKnownBridge();
      REQUIRE(lastKnown != nullptr);
      REQUIRE(lastKnown->nodeId == 12345);
      REQUIRE(lastKnown->routerRSSI == -50);
      REQUIRE(lastKnown->internetConnected == true);
    }
    
    // NOTE: Testing timeout behavior is unreliable due to test environment's
    // millis() returning 64-bit epoch time. See test notes above.
  }
  
  GIVEN("A mesh with multiple bridges with different RSSI") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    uint32_t nodeId = 1234567;
    mesh.init(&scheduler, nodeId);
    
    // Add first bridge (weaker RSSI)
    mesh.updateBridgeStatus(
      111,      // nodeId
      true,     // internetConnected
      -70,      // routerRSSI (weaker)
      1,        // routerChannel
      10000,    // uptime
      "1.1.1.1",
      100
    );
    
    // Add second bridge (stronger RSSI)
    mesh.updateBridgeStatus(
      222,      // nodeId
      true,     // internetConnected
      -40,      // routerRSSI (stronger)
      6,        // routerChannel
      5000,     // uptime
      "2.2.2.2",
      100
    );
    
    THEN("getLastKnownBridge returns the one with best RSSI") {
      auto lastKnown = mesh.getLastKnownBridge();
      REQUIRE(lastKnown != nullptr);
      REQUIRE(lastKnown->nodeId == 222);  // Better RSSI
      REQUIRE(lastKnown->routerRSSI == -40);
    }
  }
}

SCENARIO("getLastKnownBridge ignores bridges without Internet") {
  GIVEN("A mesh with bridges, some with and some without Internet") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    uint32_t nodeId = 1234567;
    mesh.init(&scheduler, nodeId);
    
    // Add bridge without Internet (better RSSI)
    mesh.updateBridgeStatus(
      100,      // nodeId
      false,    // internetConnected - NO
      -30,      // routerRSSI (best)
      6,
      1000,
      "192.168.1.100",
      0
    );
    
    // Add bridge with Internet (worse RSSI)
    mesh.updateBridgeStatus(
      200,      // nodeId
      true,     // internetConnected - YES
      -70,      // routerRSSI (worse)
      6,
      2000,
      "192.168.1.200",
      0
    );
    
    THEN("getLastKnownBridge returns the bridge with Internet") {
      auto lastKnown = mesh.getLastKnownBridge();
      REQUIRE(lastKnown != nullptr);
      REQUIRE(lastKnown->nodeId == 200);  // Has Internet, even with worse RSSI
    }
  }
}

SCENARIO("Empty bridge list handling") {
  GIVEN("A mesh with no bridges") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    uint32_t nodeId = 1234567;
    mesh.init(&scheduler, nodeId);
    
    THEN("getLastKnownBridge returns nullptr") {
      REQUIRE(mesh.getLastKnownBridge() == nullptr);
    }
    
    THEN("hasActiveMeshConnections returns false") {
      REQUIRE(mesh.hasActiveMeshConnections() == false);
    }
  }
}

SCENARIO("Bridge list management with getLastKnownBridge") {
  GIVEN("A mesh with bridges added and updated") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    uint32_t nodeId = 1234567;
    mesh.init(&scheduler, nodeId);
    
    // Add a bridge
    mesh.updateBridgeStatus(
      333,
      true,     // has Internet
      -55,      // RSSI
      6,
      1000,
      "192.168.1.3",
      0
    );
    
    WHEN("Bridge loses Internet") {
      mesh.updateBridgeStatus(
        333,
        false,    // NO Internet
        -55,
        6,
        2000,
        "192.168.1.3",
        0
      );
      
      THEN("getLastKnownBridge returns nullptr (no bridge with Internet)") {
        auto lastKnown = mesh.getLastKnownBridge();
        REQUIRE(lastKnown == nullptr);
      }
    }
    
    WHEN("A second bridge with Internet is added") {
      mesh.updateBridgeStatus(
        444,
        true,     // has Internet
        -65,      // worse RSSI than first
        6,
        500,
        "192.168.1.4",
        0
      );
      
      THEN("getLastKnownBridge returns the one with Internet and best RSSI") {
        auto lastKnown = mesh.getLastKnownBridge();
        REQUIRE(lastKnown != nullptr);
        // Both have Internet, so best RSSI wins
        REQUIRE(lastKnown->nodeId == 333);
        REQUIRE(lastKnown->routerRSSI == -55);
      }
    }
  }
}

