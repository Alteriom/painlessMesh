#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "Arduino.h"
#include "catch_utils.hpp"
#include "../../examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;
using namespace painlessmesh;

SCENARIO("MeshNodeListPackage serialization works correctly") {
  GIVEN("A MeshNodeListPackage with test data") {
    auto pkg = MeshNodeListPackage();
    pkg.from = 12345;
    pkg.meshId = "test_mesh_001";
    pkg.nodeCount = 3;
    
    pkg.nodes[0].nodeId = 11111;
    pkg.nodes[0].status = 1;  // online
    pkg.nodes[0].lastSeen = 1234567890;
    pkg.nodes[0].signalStrength = -65;
    
    pkg.nodes[1].nodeId = 22222;
    pkg.nodes[1].status = 0;  // offline
    pkg.nodes[1].lastSeen = 1234567800;
    pkg.nodes[1].signalStrength = -85;
    
    pkg.nodes[2].nodeId = 33333;
    pkg.nodes[2].status = 2;  // unreachable
    pkg.nodes[2].lastSeen = 1234567700;
    pkg.nodes[2].signalStrength = -95;

    REQUIRE(pkg.type == 600);  // MESH_NODE_LIST per mqtt-schema v0.7.2
    REQUIRE(pkg.messageType == 600);

    WHEN("Converting it to and from Variant") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<MeshNodeListPackage>();

      THEN("Should result in the same values") {
        REQUIRE(pkg2.from == pkg.from);
        REQUIRE(pkg2.meshId == pkg.meshId);
        REQUIRE(pkg2.nodeCount == pkg.nodeCount);
        REQUIRE(pkg2.type == pkg.type);
        REQUIRE(pkg2.messageType == pkg.messageType);
        
        REQUIRE(pkg2.nodes[0].nodeId == 11111);
        REQUIRE(pkg2.nodes[0].status == 1);
        REQUIRE(pkg2.nodes[1].nodeId == 22222);
        REQUIRE(pkg2.nodes[2].nodeId == 33333);
      }
    }
  }
}

SCENARIO("MeshTopologyPackage serialization works correctly") {
  GIVEN("A MeshTopologyPackage with test data") {
    auto pkg = MeshTopologyPackage();
    pkg.from = 54321;
    pkg.rootNode = 10000;
    pkg.connectionCount = 2;
    
    pkg.connections[0].fromNode = 10000;
    pkg.connections[0].toNode = 11111;
    pkg.connections[0].linkQuality = 0.95;
    pkg.connections[0].latencyMs = 25;
    pkg.connections[0].hopCount = 1;
    
    pkg.connections[1].fromNode = 11111;
    pkg.connections[1].toNode = 22222;
    pkg.connections[1].linkQuality = 0.78;
    pkg.connections[1].latencyMs = 45;
    pkg.connections[1].hopCount = 2;

    REQUIRE(pkg.type == 601);  // MESH_TOPOLOGY per mqtt-schema v0.7.2
    REQUIRE(pkg.messageType == 601);

    WHEN("Converting it to and from Variant") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<MeshTopologyPackage>();

      THEN("Should result in the same values") {
        REQUIRE(pkg2.from == pkg.from);
        REQUIRE(pkg2.rootNode == pkg.rootNode);
        REQUIRE(pkg2.connectionCount == pkg.connectionCount);
        REQUIRE(pkg2.type == pkg.type);
        REQUIRE(pkg2.messageType == pkg.messageType);
        
        REQUIRE(pkg2.connections[0].fromNode == 10000);
        REQUIRE(pkg2.connections[0].toNode == 11111);
        REQUIRE(pkg2.connections[0].linkQuality == Approx(0.95).epsilon(0.01));
        REQUIRE(pkg2.connections[1].latencyMs == 45);
      }
    }
  }
}

SCENARIO("MeshAlertPackage serialization works correctly") {
  GIVEN("A MeshAlertPackage with test data") {
    auto pkg = MeshAlertPackage();
    pkg.from = 99999;
    pkg.alertCount = 2;
    
    pkg.alerts[0].alertType = 1;  // node_offline
    pkg.alerts[0].severity = 2;   // critical
    pkg.alerts[0].message = "Node 11111 offline";
    pkg.alerts[0].nodeId = 11111;
    pkg.alerts[0].metricValue = 0.0;
    pkg.alerts[0].threshold = 0.0;
    pkg.alerts[0].alertId = 1001;
    
    pkg.alerts[1].alertType = 4;  // packet_loss
    pkg.alerts[1].severity = 1;   // warning
    pkg.alerts[1].message = "High packet loss";
    pkg.alerts[1].nodeId = 22222;
    pkg.alerts[1].metricValue = 15.5;
    pkg.alerts[1].threshold = 10.0;
    pkg.alerts[1].alertId = 1002;

    REQUIRE(pkg.type == 602);  // MESH_ALERT per mqtt-schema v0.7.2
    REQUIRE(pkg.messageType == 602);

    WHEN("Converting it to and from Variant") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<MeshAlertPackage>();

      THEN("Should result in the same values") {
        REQUIRE(pkg2.from == pkg.from);
        REQUIRE(pkg2.alertCount == pkg.alertCount);
        REQUIRE(pkg2.type == pkg.type);
        REQUIRE(pkg2.messageType == pkg.messageType);
        
        REQUIRE(pkg2.alerts[0].alertType == 1);
        REQUIRE(pkg2.alerts[0].severity == 2);
        REQUIRE(pkg2.alerts[0].message == "Node 11111 offline");
        REQUIRE(pkg2.alerts[0].nodeId == 11111);
        REQUIRE(pkg2.alerts[0].alertId == 1001);
        
        REQUIRE(pkg2.alerts[1].metricValue == Approx(15.5).epsilon(0.01));
        REQUIRE(pkg2.alerts[1].threshold == Approx(10.0).epsilon(0.01));
      }
    }
  }
}

SCENARIO("MeshBridgePackage serialization works correctly") {
  GIVEN("A MeshBridgePackage with test data") {
    auto pkg = MeshBridgePackage();
    pkg.from = 88888;
    pkg.meshProtocol = 0;  // painlessMesh
    pkg.fromNodeId = 11111;
    pkg.toNodeId = 22222;
    pkg.meshType = 8;  // SINGLE message type in painlessMesh
    pkg.rawPayload = "7b2274656d70223a32332e357d";  // hex encoded JSON
    pkg.rssi = -72;
    pkg.hopCount = 2;
    pkg.meshTimestamp = 1234567890;
    pkg.gatewayNodeId = 99999;
    pkg.meshNetworkId = "mesh_net_001";

    REQUIRE(pkg.type == 603);  // MESH_BRIDGE per mqtt-schema v0.7.2
    REQUIRE(pkg.messageType == 603);

    WHEN("Converting it to and from Variant") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<MeshBridgePackage>();

      THEN("Should result in the same values") {
        REQUIRE(pkg2.from == pkg.from);
        REQUIRE(pkg2.meshProtocol == pkg.meshProtocol);
        REQUIRE(pkg2.fromNodeId == pkg.fromNodeId);
        REQUIRE(pkg2.toNodeId == pkg.toNodeId);
        REQUIRE(pkg2.meshType == pkg.meshType);
        REQUIRE(pkg2.rawPayload == pkg.rawPayload);
        REQUIRE(pkg2.rssi == pkg.rssi);
        REQUIRE(pkg2.hopCount == pkg.hopCount);
        REQUIRE(pkg2.meshTimestamp == pkg.meshTimestamp);
        REQUIRE(pkg2.gatewayNodeId == pkg.gatewayNodeId);
        REQUIRE(pkg2.meshNetworkId == pkg.meshNetworkId);
        REQUIRE(pkg2.type == pkg.type);
        REQUIRE(pkg2.messageType == pkg.messageType);
      }
    }
  }
}

SCENARIO("All mesh packages have correct type codes") {
  GIVEN("Mesh packages instantiated") {
    auto nodeList = MeshNodeListPackage();
    auto topology = MeshTopologyPackage();
    auto alert = MeshAlertPackage();
    auto bridge = MeshBridgePackage();

    THEN("Type codes match mqtt-schema v0.7.2 specification") {
      REQUIRE(nodeList.type == 600);
      REQUIRE(topology.type == 601);
      REQUIRE(alert.type == 602);
      REQUIRE(bridge.type == 603);
      
      REQUIRE(nodeList.messageType == 600);
      REQUIRE(topology.messageType == 601);
      REQUIRE(alert.messageType == 602);
      REQUIRE(bridge.messageType == 603);
    }
  }
}
