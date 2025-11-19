#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include <Arduino.h>

#include "catch_utils.hpp"

#include "painlessmesh/ntp.hpp"
#include "painlessmesh/protocol.hpp"

using namespace painlessmesh;

logger::LogClass Log;

SCENARIO("Time authority flag in NodeTree serialization") {
  GIVEN("A NodeTree with time authority") {
    protocol::NodeTree node(12345, false, true);
    node.hasTimeAuthority = true;
    
    WHEN("Serializing to JSON") {
      DynamicJsonDocument doc(512);
      JsonObject obj = doc.to<JsonObject>();
      obj = node.addTo(std::move(obj));
      
      THEN("hasTimeAuthority should be in JSON") {
        REQUIRE(obj.containsKey("hasTimeAuthority"));
        REQUIRE(obj["hasTimeAuthority"].as<bool>() == true);
      }
    }
    
    WHEN("Deserializing from JSON") {
      DynamicJsonDocument doc(512);
      JsonObject obj = doc.to<JsonObject>();
      obj = node.addTo(std::move(obj));
      
      protocol::NodeTree node2(obj);
      
      THEN("hasTimeAuthority should be preserved") {
        REQUIRE(node2.hasTimeAuthority == true);
        REQUIRE(node2.nodeId == 12345);
      }
    }
  }
  
  GIVEN("A NodeTree without time authority") {
    protocol::NodeTree node(54321, false, false);
    
    WHEN("Serializing to JSON") {
      DynamicJsonDocument doc(512);
      JsonObject obj = doc.to<JsonObject>();
      obj = node.addTo(std::move(obj));
      
      THEN("hasTimeAuthority should not be in JSON (defaults to false)") {
        REQUIRE_FALSE(obj.containsKey("hasTimeAuthority"));
      }
    }
  }
}

SCENARIO("Time authority in adopt decision") {
  GIVEN("Two nodes without time authority") {
    protocol::NodeTree node1(1000, false, false);
    protocol::NodeTree node2(2000, false, false);
    
    WHEN("Both have equal subnet counts") {
      THEN("Lower node ID adopts from higher node ID (existing behavior)") {
        REQUIRE(ntp::adopt(node1, node2) == true);  // 1000 < 2000
        REQUIRE(ntp::adopt(node2, node1) == false); // 2000 > 1000
      }
    }
  }
  
  GIVEN("Node without time authority and node with time authority") {
    protocol::NodeTree nodeWithoutAuth(1000, false, false);
    protocol::NodeTree nodeWithAuth(2000, false, true);
    
    WHEN("Node without authority considers adopting from node with authority") {
      THEN("Should adopt from authoritative node") {
        REQUIRE(ntp::adopt(nodeWithoutAuth, nodeWithAuth) == true);
      }
    }
    
    WHEN("Node with authority considers adopting from node without authority") {
      THEN("Should NOT adopt from non-authoritative node") {
        REQUIRE(ntp::adopt(nodeWithAuth, nodeWithoutAuth) == false);
      }
    }
  }
  
  GIVEN("Two nodes both with time authority") {
    protocol::NodeTree node1Auth(1000, false, true);
    protocol::NodeTree node2Auth(2000, false, true);
    
    WHEN("Both have time authority") {
      THEN("Should fall back to subnet count / node ID logic") {
        REQUIRE(ntp::adopt(node1Auth, node2Auth) == true);  // 1000 < 2000
        REQUIRE(ntp::adopt(node2Auth, node1Auth) == false); // 2000 > 1000
      }
    }
  }
  
  GIVEN("Complex scenario with authority and subnet differences") {
    protocol::NodeTree largeNoAuth(1000, false, false);
    protocol::NodeTree smallWithAuth(2000, false, true);
    
    // Add subnets to make largeNoAuth have more connections
    largeNoAuth.subs.push_back(protocol::NodeTree(3000, false));
    largeNoAuth.subs.push_back(protocol::NodeTree(4000, false));
    
    WHEN("Large network without authority vs small network with authority") {
      THEN("Authority takes precedence over subnet count") {
        REQUIRE(ntp::adopt(largeNoAuth, smallWithAuth) == true);
        REQUIRE(ntp::adopt(smallWithAuth, largeNoAuth) == false);
      }
    }
  }
}

SCENARIO("NodeTree equality includes time authority") {
  GIVEN("Two identical NodeTrees") {
    protocol::NodeTree node1(12345, true, true);
    protocol::NodeTree node2(12345, true, true);
    
    WHEN("Comparing them") {
      THEN("They should be equal") {
        REQUIRE(node1 == node2);
        REQUIRE_FALSE(node1 != node2);
      }
    }
  }
  
  GIVEN("Two NodeTrees differing only in time authority") {
    protocol::NodeTree node1(12345, true, true);
    protocol::NodeTree node2(12345, true, false);
    
    WHEN("Comparing them") {
      THEN("They should NOT be equal") {
        REQUIRE(node1 != node2);
        REQUIRE_FALSE(node1 == node2);
      }
    }
  }
}

SCENARIO("NodeTree clear resets time authority") {
  GIVEN("A NodeTree with time authority") {
    protocol::NodeTree node(12345, true, true);
    node.hasTimeAuthority = true;
    
    WHEN("Clearing the node") {
      node.clear();
      
      THEN("Time authority should be reset to false") {
        REQUIRE(node.hasTimeAuthority == false);
        REQUIRE(node.nodeId == 0);
        REQUIRE(node.root == false);
        REQUIRE(node.subs.empty());
      }
    }
  }
}

SCENARIO("NodeSyncRequest preserves time authority") {
  GIVEN("A NodeSyncRequest with time authority") {
    std::list<protocol::NodeTree> subs;
    subs.push_back(protocol::NodeTree(3000, false, false));
    
    protocol::NodeSyncRequest req(12345, 54321, subs, true);
    req.hasTimeAuthority = true;
    
    WHEN("Serializing and deserializing") {
      DynamicJsonDocument doc(1024);
      JsonObject obj = doc.to<JsonObject>();
      obj = req.addTo(std::move(obj));
      
      protocol::NodeSyncRequest req2(obj);
      
      THEN("Time authority should be preserved") {
        REQUIRE(req2.hasTimeAuthority == true);
        REQUIRE(req2.nodeId == 12345);
        REQUIRE(req2.from == 12345);
        REQUIRE(req2.dest == 54321);
      }
    }
  }
}

SCENARIO("Time authority prioritization in realistic scenarios") {
  GIVEN("Three-node network: RTC node, Internet bridge, and regular node") {
    protocol::NodeTree rtcNode(1000, false, true);     // Has RTC
    protocol::NodeTree bridgeNode(2000, false, true);  // Has Internet
    protocol::NodeTree regularNode(3000, false, false); // No time source
    
    WHEN("Regular node connects to RTC node") {
      THEN("Regular node should adopt from RTC node") {
        REQUIRE(ntp::adopt(regularNode, rtcNode) == true);
      }
    }
    
    WHEN("Regular node connects to bridge node") {
      THEN("Regular node should adopt from bridge node") {
        REQUIRE(ntp::adopt(regularNode, bridgeNode) == true);
      }
    }
    
    WHEN("RTC node connects to regular node") {
      THEN("RTC node should NOT adopt from regular node") {
        REQUIRE(ntp::adopt(rtcNode, regularNode) == false);
      }
    }
    
    WHEN("RTC node connects to bridge node") {
      THEN("Should use node ID tiebreaker (both have authority)") {
        REQUIRE(ntp::adopt(rtcNode, bridgeNode) == true);  // 1000 < 2000
        REQUIRE(ntp::adopt(bridgeNode, rtcNode) == false); // 2000 > 1000
      }
    }
  }
  
  GIVEN("Network split scenario") {
    // Simulate network split where one part has authority, other doesn't
    protocol::NodeTree authSubnet(1000, false, true);
    authSubnet.subs.push_back(protocol::NodeTree(1001, false, true));
    authSubnet.subs.push_back(protocol::NodeTree(1002, false, true));
    
    protocol::NodeTree noAuthSubnet(2000, false, false);
    noAuthSubnet.subs.push_back(protocol::NodeTree(2001, false, false));
    noAuthSubnet.subs.push_back(protocol::NodeTree(2002, false, false));
    noAuthSubnet.subs.push_back(protocol::NodeTree(2003, false, false));
    
    WHEN("Networks reconnect") {
      THEN("Non-auth subnet should adopt from auth subnet despite being larger") {
        REQUIRE(ntp::adopt(noAuthSubnet, authSubnet) == true);
        REQUIRE(ntp::adopt(authSubnet, noAuthSubnet) == false);
      }
    }
  }
}
