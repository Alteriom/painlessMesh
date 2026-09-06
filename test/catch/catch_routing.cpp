#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <Arduino.h>

#include "catch_utils.hpp"
#include "painlessmesh/layout.hpp"
#include "painlessmesh/protocol.hpp"
#include "painlessmesh/router.hpp"
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

// A connection that has closed stays in Layout::subs until
// eraseClosedConnections() next runs, so it still answers findRoute().
class ClosableConnection : public protocol::NodeTree {
 public:
  bool up = true;
  bool connected() const { return up; }
};

static std::shared_ptr<ClosableConnection> conn_holding(uint32_t nodeId,
                                                        bool up = true) {
  auto conn = std::make_shared<ClosableConnection>();
  conn->nodeId = nodeId;
  conn->up = up;
  return conn;
}

SCENARIO("A route through a closed connection cannot refuse a live one") {
  GIVEN("a node whose only route to 2098834584 is a connection that dropped") {
    layout::Layout<ClosableConnection> tree;
    auto dead = conn_holding(2098834584, /*up=*/false);
    auto alive = conn_holding(3711130777);
    tree.subs.push_back(dead);
    tree.subs.push_back(alive);

    THEN("findRoute does not answer with the dead one either") {
      // Routing a packet to a closed connection queues it where nothing
      // will drain it and tells the sender it succeeded.
      REQUIRE(router::findRoute<ClosableConnection>(tree, 2098834584) ==
              nullptr);
    }
    THEN("findLiveRoute does not, so a direct connection is accepted") {
      REQUIRE(router::findLiveRoute<ClosableConnection>(tree, 2098834584) ==
              nullptr);
    }
    THEN("a route through a live connection is still found and still refuses") {
      REQUIRE(router::findLiveRoute<ClosableConnection>(tree, 3711130777) ==
              alive);
    }
  }

  GIVEN("the connection being judged is itself in subs") {
    layout::Layout<ClosableConnection> tree;
    auto self = conn_holding(1297448309);
    tree.subs.push_back(self);
    THEN("it is not treated as a duplicate of itself") {
      REQUIRE(router::findLiveRoute<ClosableConnection>(tree, 1297448309,
                                                        self) == nullptr);
    }
    THEN("without excluding it, it would be") {
      REQUIRE(router::findLiveRoute<ClosableConnection>(tree, 1297448309) ==
              self);
    }
  }
}

SCENARIO("A node that comes back is not refused for where it used to be") {
  // A station has one uplink. When a node arrives on a fresh direct
  // connection, any other route to it is its old place: a neighbour's tree
  // that still lists it, or the dead link it had before it rebooted. The
  // loop check is the tree the node presents, not the routes we remember.
  GIVEN("a neighbour whose tree still lists 2098834584 under 3711130777") {
    auto neighbour = conn_holding(2101688781);
    protocol::NodeTree via(3711130777, false);
    via.subs.push_back(protocol::NodeTree(2098834584, false));
    protocol::NodeTree leaf(381621429, false);
    neighbour->subs.push_back(via);
    neighbour->subs.push_back(leaf);

    WHEN("the node is forgotten from that tree") {
      REQUIRE(layout::forget(*neighbour, 2098834584));
      THEN("only it is gone; the nodes on the way to it stay") {
        REQUIRE(!layout::contains(*neighbour, 2098834584));
        REQUIRE(layout::contains(*neighbour, 3711130777));
        REQUIRE(layout::contains(*neighbour, 381621429));
        REQUIRE(neighbour->subs.size() == 2);
      }
      THEN("forgetting it again finds nothing") {
        REQUIRE(!layout::forget(*neighbour, 2098834584));
      }
    }
    WHEN("a node that was never there is forgotten") {
      REQUIRE(!layout::forget(*neighbour, 139984357));
      THEN("the tree is untouched") {
        REQUIRE(layout::contains(*neighbour, 2098834584));
        REQUIRE(neighbour->subs.size() == 2);
      }
    }
  }

  GIVEN("the tree a returning node presents") {
    protocol::NodeTree fresh(2098834584, false);
    fresh.subs.push_back(protocol::NodeTree(381621429, false));
    THEN("it is a loop only if this node is in it") {
      REQUIRE(!layout::contains(fresh, 2101688781));
      REQUIRE(layout::contains(fresh, 381621429));
    }
  }
}

SCENARIO("A node presented under one neighbour is forgotten under the others") {
  // A node is in one place. On the rig every board carried a node twice —
  // under the neighbour it had moved to and under the one it had left —
  // and a message routed by the older copy never arrived.
  GIVEN("two neighbours whose cached trees both list 3711130777") {
    auto stale = conn_holding(3198819345);
    protocol::NodeTree via(1297448309, false);
    via.subs.push_back(protocol::NodeTree(3711130777, false));
    stale->subs.push_back(via);
    stale->subs.push_back(protocol::NodeTree(139984357, false));

    protocol::NodeTree presented(2098834584, false);
    presented.subs.push_back(protocol::NodeTree(3711130777, false));
    presented.subs.push_back(protocol::NodeTree(381621429, false));

    WHEN("the fresher neighbour's tree is applied against the other") {
      size_t removed = layout::forgetAll(*stale, presented);
      THEN("only the nodes the fresh tree carries are gone from the stale one") {
        REQUIRE(removed == 1);
        REQUIRE(!layout::contains(*stale, 3711130777));
        REQUIRE(layout::contains(*stale, 1297448309));
        REQUIRE(layout::contains(*stale, 139984357));
      }
      THEN("a second application finds nothing") {
        REQUIRE(layout::forgetAll(*stale, presented) == 0);
      }
    }
  }
}

SCENARIO("A tree's fingerprint tells a restated sync from a changed one") {
  GIVEN("the tree a neighbour presents") {
    auto tree = createBranchedTopology();
    auto fp = layout::fingerprint(tree);
    THEN("it is never 0, which means nothing presented yet") {
      REQUIRE(fp != 0);
      REQUIRE(layout::fingerprint(protocol::NodeTree(0, false)) != 0);
    }
    THEN("the same tree presented again has the same fingerprint") {
      REQUIRE(layout::fingerprint(createBranchedTopology()) == fp);
    }
    THEN("a node gone from it changes the fingerprint") {
      auto changed = createBranchedTopology();
      REQUIRE(layout::forget(changed, 5000));
      REQUIRE(layout::fingerprint(changed) != fp);
    }
    THEN("a node moved to another branch changes the fingerprint") {
      auto moved = createBranchedTopology();
      REQUIRE(layout::forget(moved, 3000));
      moved.subs.back().subs.push_back(protocol::NodeTree(3000, false));
      REQUIRE(layout::size(moved) == layout::size(tree));
      REQUIRE(layout::fingerprint(moved) != fp);
    }
    THEN("a node becoming root changes the fingerprint") {
      auto rooted = createBranchedTopology();
      rooted.subs.front().root = true;
      REQUIRE(layout::fingerprint(rooted) != fp);
    }
  }
}
