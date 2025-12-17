#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"

#include <set>
#include <vector>

#include "catch_utils.hpp"

#include "painlessmesh/gateway.hpp"
#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

// Logger for test environment
painlessmesh::logger::LogClass Log;

SCENARIO("sendToInternet checks mesh connectivity before sending") {
  GIVEN("A mesh node with bridge status but no connections") {
    using Connection = painlessmesh::Connection;
    Mesh<Connection> mesh2;  // Regular node
    
    // Initialize scheduler
    Scheduler scheduler2;
    
    // Initialize mesh 2 as regular node
    mesh2.init(&scheduler2, 0x22222222);
    mesh2.enableSendToInternet();
    
    WHEN("Bridge status is known but no active connections") {
      // Simulate bridge status broadcast with Internet connectivity
      uint32_t bridgeNodeId = 0x11111111;
      bool internetConnected = true;
      int8_t routerRSSI = -42;
      uint8_t routerChannel = 6;
      uint32_t uptime = 10000;
      TSTRING gatewayIP = "192.168.1.1";
      uint32_t timestamp = static_cast<uint32_t>(millis());
      
      mesh2.updateBridgeStatus(bridgeNodeId, internetConnected, routerRSSI,
                              routerChannel, uptime, gatewayIP, timestamp);
      
      // Verify bridge is known
      auto bridges = mesh2.getBridges();
      REQUIRE(bridges.size() == 1);
      REQUIRE(bridges[0].nodeId == bridgeNodeId);
      REQUIRE(bridges[0].internetConnected == true);
      REQUIRE(mesh2.hasInternetConnection() == true);
      
      // Verify no active mesh connections (disconnected state)
      REQUIRE(mesh2.hasActiveMeshConnections() == false);
      
      THEN("sendToInternet should fail immediately with no connections error") {
        bool callbackCalled = false;
        bool callbackSuccess = false;
        TSTRING callbackError;
        
        uint32_t msgId = mesh2.sendToInternet(
            "https://api.example.com/data",
            "{\"test\": \"data\"}",
            [&](bool success, uint16_t httpStatus, TSTRING error) {
              callbackCalled = true;
              callbackSuccess = success;
              callbackError = error;
            });
        
        // sendToInternet should return 0 for immediate failure
        REQUIRE(msgId == 0);
        
        // No pending requests should be queued
        REQUIRE(mesh2.getPendingInternetRequestCount() == 0);
        
        // Run scheduler to process callbacks
        scheduler2.execute();
        
        // Callback should be invoked with error
        REQUIRE(callbackCalled == true);
        REQUIRE(callbackSuccess == false);
        REQUIRE(callbackError.find("mesh connections") != TSTRING::npos);
      }
      
      AND_THEN("Multiple sendToInternet calls should all fail gracefully") {
        int failureCount = 0;
        
        for (int i = 0; i < 5; i++) {
          uint32_t msgId = mesh2.sendToInternet(
              "https://api.example.com/data",
              "{\"test\": \"data" + std::to_string(i) + "\"}",
              [&](bool success, uint16_t httpStatus, TSTRING error) {
                if (!success) failureCount++;
              });
          
          REQUIRE(msgId == 0);
        }
        
        // Process all callbacks
        for (int i = 0; i < 10; i++) {
          scheduler2.execute();
        }
        
        // All 5 should have failed
        REQUIRE(failureCount == 5);
        
        // No requests should be pending
        REQUIRE(mesh2.getPendingInternetRequestCount() == 0);
      }
    }
  }
}

SCENARIO("sendToInternet behavior during bridge disconnection") {
  GIVEN("A mesh with known bridge but intermittent connectivity") {
    using Connection = painlessmesh::Connection;
    Mesh<Connection> mesh2;
    
    Scheduler scheduler2;
    mesh2.init(&scheduler2, 0x22222222);
    mesh2.enableSendToInternet();
    
    // Set up bridge status
    uint32_t bridgeNodeId = 0x11111111;
    mesh2.updateBridgeStatus(bridgeNodeId, true, -42, 6, 10000, "192.168.1.1",
                            static_cast<uint32_t>(millis()));
    
    WHEN("Node has no active connections") {
      REQUIRE(mesh2.hasActiveMeshConnections() == false);
      REQUIRE(mesh2.hasInternetConnection() == true);  // Bridge info exists
      
      THEN("getPrimaryBridge should still return bridge info") {
        auto* bridge = mesh2.getPrimaryBridge();
        REQUIRE(bridge != nullptr);
        REQUIRE(bridge->nodeId == bridgeNodeId);
      }
      
      AND_THEN("sendToInternet should detect no mesh connections") {
        bool callbackCalled = false;
        
        uint32_t msgId = mesh2.sendToInternet(
            "https://api.example.com/test",
            "{}",
            [&](bool success, uint16_t httpStatus, TSTRING error) {
              callbackCalled = true;
            });
        
        REQUIRE(msgId == 0);
        REQUIRE(mesh2.getPendingInternetRequestCount() == 0);
        
        // Process callback
        scheduler2.execute();
        REQUIRE(callbackCalled == true);
      }
    }
  }
}
