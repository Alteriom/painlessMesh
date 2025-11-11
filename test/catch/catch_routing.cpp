#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <Arduino.h>

#include "catch_utils.hpp"
#include "painlessmesh/layout.hpp"
#include "painlessmesh/protocol.hpp"
#include "painlessmesh/logger.hpp"

using namespace painlessmesh;

painlessmesh::logger::LogClass Log;

/**
 * Test helper to create a simple node tree for testing routing algorithms
 * 
 * Creates a linear topology: A -- B -- C -- D
 * where A is the root node
 */
protocol::NodeTree createLinearTopology() {
  // Node A (root) with Node B as child
  protocol::NodeTree nodeA(1000, true);
  
  // Node B with Node C as child
  protocol::NodeTree nodeB(2000, false);
  
  // Node C with Node D as child
  protocol::NodeTree nodeC(3000, false);
  
  // Node D (leaf)
  protocol::NodeTree nodeD(4000, false);
  
  // Build tree: A -> B -> C -> D
  nodeC.subs.push_back(nodeD);
  nodeB.subs.push_back(nodeC);
  nodeA.subs.push_back(nodeB);
  
  return nodeA;
}

/**
 * Test helper to create a star topology for testing routing
 * 
 * Creates topology:    B
 *                      |
 *                  C - A - D
 *                      |
 *                      E
 * where A is the hub (root)
 */
protocol::NodeTree createStarTopology() {
  protocol::NodeTree nodeA(1000, true);  // Hub
  protocol::NodeTree nodeB(2000, false);
  protocol::NodeTree nodeC(3000, false);
  protocol::NodeTree nodeD(4000, false);
  protocol::NodeTree nodeE(5000, false);
  
  // Connect all to hub A
  nodeA.subs.push_back(nodeB);
  nodeA.subs.push_back(nodeC);
  nodeA.subs.push_back(nodeD);
  nodeA.subs.push_back(nodeE);
  
  return nodeA;
}

/**
 * Test helper to create a branched topology for testing
 * 
 * Creates topology:    B - C
 *                      |
 *                      A - D - E
 * where A is the root with two branches
 */
protocol::NodeTree createBranchedTopology() {
  protocol::NodeTree nodeA(1000, true);  // Root
  protocol::NodeTree nodeB(2000, false);
  protocol::NodeTree nodeC(3000, false);
  protocol::NodeTree nodeD(4000, false);
  protocol::NodeTree nodeE(5000, false);
  
  // A connects to B and D (two branches)
  nodeB.subs.push_back(nodeC);  // Branch 1: A -> B -> C
  nodeD.subs.push_back(nodeE);  // Branch 2: A -> D -> E
  
  nodeA.subs.push_back(nodeB);
  nodeA.subs.push_back(nodeD);
  
  return nodeA;
}

SCENARIO("layout::contains correctly identifies nodes in tree") {
  GIVEN("A linear topology A-B-C-D") {
    auto tree = createLinearTopology();
    
    THEN("All nodes should be found") {
      REQUIRE(layout::contains(tree, 1000) == true);  // A
      REQUIRE(layout::contains(tree, 2000) == true);  // B
      REQUIRE(layout::contains(tree, 3000) == true);  // C
      REQUIRE(layout::contains(tree, 4000) == true);  // D
    }
    
    THEN("Non-existent nodes should not be found") {
      REQUIRE(layout::contains(tree, 9999) == false);
    }
  }
  
  GIVEN("A star topology with hub A") {
    auto tree = createStarTopology();
    
    THEN("All nodes including spokes should be found") {
      REQUIRE(layout::contains(tree, 1000) == true);  // Hub A
      REQUIRE(layout::contains(tree, 2000) == true);  // Spoke B
      REQUIRE(layout::contains(tree, 3000) == true);  // Spoke C
      REQUIRE(layout::contains(tree, 4000) == true);  // Spoke D
      REQUIRE(layout::contains(tree, 5000) == true);  // Spoke E
    }
  }
}

SCENARIO("layout::asList returns all nodes in the tree") {
  GIVEN("A linear topology A-B-C-D") {
    auto tree = createLinearTopology();
    
    WHEN("Getting list with self") {
      auto list = layout::asList(tree, true);
      
      THEN("Should return all 4 nodes") {
        REQUIRE(list.size() == 4);
        REQUIRE(std::find(list.begin(), list.end(), 1000) != list.end());
        REQUIRE(std::find(list.begin(), list.end(), 2000) != list.end());
        REQUIRE(std::find(list.begin(), list.end(), 3000) != list.end());
        REQUIRE(std::find(list.begin(), list.end(), 4000) != list.end());
      }
    }
    
    WHEN("Getting list without self") {
      auto list = layout::asList(tree, false);
      
      THEN("Should return 3 nodes (excluding root)") {
        REQUIRE(list.size() == 3);
        REQUIRE(std::find(list.begin(), list.end(), 1000) == list.end());
        REQUIRE(std::find(list.begin(), list.end(), 2000) != list.end());
        REQUIRE(std::find(list.begin(), list.end(), 3000) != list.end());
        REQUIRE(std::find(list.begin(), list.end(), 4000) != list.end());
      }
    }
  }
}

SCENARIO("Node tree serialization and deserialization") {
  GIVEN("A linear topology") {
    auto tree = createLinearTopology();
    
    WHEN("Serializing to JSON") {
      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      obj = tree.addTo(std::move(obj));
      
      THEN("Should contain correct nodeId and subs") {
        REQUIRE(obj["nodeId"] == 1000);
        REQUIRE(obj["root"] == true);
        REQUIRE(obj["subs"].size() == 1);
        
        JsonObject sub1 = obj["subs"][0];
        REQUIRE(sub1["nodeId"] == 2000);
        REQUIRE(sub1["subs"].size() == 1);
      }
      
      AND_WHEN("Deserializing back") {
        auto tree2 = protocol::NodeTree(obj);
        
        THEN("Should reconstruct the same tree") {
          REQUIRE(tree2.nodeId == 1000);
          REQUIRE(tree2.root == true);
          REQUIRE(tree2.subs.size() == 1);
          REQUIRE(tree2.subs.front().nodeId == 2000);
        }
      }
    }
  }
}

SCENARIO("Multi-topology node counting") {
  GIVEN("Different topologies") {
    WHEN("Using linear topology") {
      auto tree = createLinearTopology();
      THEN("Size should be 4") {
        REQUIRE(layout::size(tree) == 4);
      }
    }
    
    WHEN("Using star topology") {
      auto tree = createStarTopology();
      THEN("Size should be 5") {
        REQUIRE(layout::size(tree) == 5);
      }
    }
    
    WHEN("Using branched topology") {
      auto tree = createBranchedTopology();
      THEN("Size should be 5") {
        REQUIRE(layout::size(tree) == 5);
      }
    }
  }
}
