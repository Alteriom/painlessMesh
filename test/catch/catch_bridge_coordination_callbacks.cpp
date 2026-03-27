#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "painlessmesh/plugin.hpp"

using namespace painlessmesh;

struct BridgeCoordinationState {
  uint8_t priority;
  TSTRING role;
  uint8_t load;
  uint32_t lastSeen;
};

static TSTRING detectChange(
    const plugin::BridgeCoordinationPackage& pkg,
    uint32_t fromNode,
    std::map<uint32_t, BridgeCoordinationState>& stateMap) {
  auto it = stateMap.find(fromNode);
  if (it == stateMap.end()) {
    stateMap[fromNode] = {pkg.priority, pkg.role, pkg.load, (uint32_t)millis()};
    return "new";
  }
  auto& prev = it->second;
  bool changed = (prev.priority != pkg.priority ||
                  prev.role != pkg.role ||
                  prev.load != pkg.load);
  prev.priority = pkg.priority;
  prev.role = pkg.role;
  prev.load = pkg.load;
  prev.lastSeen = (uint32_t)millis();
  return changed ? "updated" : "";
}

static std::vector<uint32_t> detectLost(
    std::map<uint32_t, BridgeCoordinationState>& stateMap,
    uint32_t now,
    uint32_t timeoutMs) {
  std::vector<uint32_t> lost;
  for (auto it = stateMap.begin(); it != stateMap.end(); ) {
    if ((now - it->second.lastSeen) > timeoutMs) {
      lost.push_back(it->first);
      it = stateMap.erase(it);
    } else {
      ++it;
    }
  }
  return lost;
}

SCENARIO("Change detection reports 'new' for first message from a bridge") {
  GIVEN("An empty state map") {
    std::map<uint32_t, BridgeCoordinationState> stateMap;
    plugin::BridgeCoordinationPackage pkg;
    pkg.from = 100;
    pkg.priority = 8;
    pkg.role = "primary";
    pkg.load = 20;

    WHEN("First coordination message arrives") {
      TSTRING result = detectChange(pkg, 100, stateMap);

      THEN("Change type is 'new'") {
        REQUIRE(result == "new");
        REQUIRE(stateMap.size() == 1);
        REQUIRE(stateMap[100].priority == 8);
        REQUIRE(stateMap[100].role == "primary");
        REQUIRE(stateMap[100].load == 20);
      }
    }
  }
}

SCENARIO("Change detection reports 'updated' when values change") {
  GIVEN("A state map with one known bridge") {
    std::map<uint32_t, BridgeCoordinationState> stateMap;
    stateMap[100] = {8, "primary", 20, (uint32_t)millis()};

    WHEN("Priority changes") {
      plugin::BridgeCoordinationPackage pkg;
      pkg.from = 100;
      pkg.priority = 5;
      pkg.role = "primary";
      pkg.load = 20;
      TSTRING result = detectChange(pkg, 100, stateMap);

      THEN("Change type is 'updated'") {
        REQUIRE(result == "updated");
        REQUIRE(stateMap[100].priority == 5);
      }
    }

    WHEN("Role changes") {
      plugin::BridgeCoordinationPackage pkg;
      pkg.from = 100;
      pkg.priority = 8;
      pkg.role = "secondary";
      pkg.load = 20;
      TSTRING result = detectChange(pkg, 100, stateMap);

      THEN("Change type is 'updated'") {
        REQUIRE(result == "updated");
        REQUIRE(stateMap[100].role == "secondary");
      }
    }

    WHEN("Load changes") {
      plugin::BridgeCoordinationPackage pkg;
      pkg.from = 100;
      pkg.priority = 8;
      pkg.role = "primary";
      pkg.load = 75;
      TSTRING result = detectChange(pkg, 100, stateMap);

      THEN("Change type is 'updated'") {
        REQUIRE(result == "updated");
        REQUIRE(stateMap[100].load == 75);
      }
    }
  }
}

SCENARIO("Change detection reports nothing when values are identical") {
  GIVEN("A state map with one known bridge") {
    std::map<uint32_t, BridgeCoordinationState> stateMap;
    stateMap[100] = {8, "primary", 20, (uint32_t)millis()};

    WHEN("Same values arrive again") {
      plugin::BridgeCoordinationPackage pkg;
      pkg.from = 100;
      pkg.priority = 8;
      pkg.role = "primary";
      pkg.load = 20;
      TSTRING result = detectChange(pkg, 100, stateMap);

      THEN("Change type is empty (no change)") {
        REQUIRE(result == "");
      }
    }
  }
}

SCENARIO("Lost detection fires when bridge exceeds timeout") {
  GIVEN("A state map with two bridges, one stale") {
    std::map<uint32_t, BridgeCoordinationState> stateMap;
    uint32_t now = 100000;
    stateMap[100] = {8, "primary", 20, now};
    stateMap[200] = {5, "secondary", 10, now - 70000};

    WHEN("Lost detection runs with 60s timeout") {
      auto lost = detectLost(stateMap, now, 60000);

      THEN("Only the stale bridge is reported lost") {
        REQUIRE(lost.size() == 1);
        REQUIRE(lost[0] == 200);
        REQUIRE(stateMap.size() == 1);
        REQUIRE(stateMap.count(100) == 1);
      }
    }
  }
}

SCENARIO("Lost detection reports nothing when all bridges are fresh") {
  GIVEN("A state map with two fresh bridges") {
    std::map<uint32_t, BridgeCoordinationState> stateMap;
    uint32_t now = 100000;
    stateMap[100] = {8, "primary", 20, now};
    stateMap[200] = {5, "secondary", 10, now - 30000};

    WHEN("Lost detection runs") {
      auto lost = detectLost(stateMap, now, 60000);

      THEN("No bridges are reported lost") {
        REQUIRE(lost.empty());
        REQUIRE(stateMap.size() == 2);
      }
    }
  }
}
