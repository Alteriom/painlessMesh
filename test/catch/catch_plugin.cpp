#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "catch_utils.hpp"

#include "painlessmesh/plugin.hpp"
// Included to check they compile
#include "plugin/performance.hpp"
#include "plugin/remote.hpp"

using namespace painlessmesh;

logger::LogClass Log;

class CustomPackage : public plugin::SinglePackage {
 public:
  double sensor = 1.0;

  CustomPackage() : SinglePackage(20) {}

  CustomPackage(JsonObject jsonObj) : SinglePackage(jsonObj) {
    sensor = jsonObj["sensor"];
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = SinglePackage::addTo(std::move(jsonObj));
    jsonObj["sensor"] = sensor;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const { return JSON_OBJECT_SIZE(noJsonFields + 1); }
#endif
};

class BCustomPackage : public plugin::BroadcastPackage {
 public:
  double sensor = 1.0;

  BCustomPackage() : BroadcastPackage(21) {}

  BCustomPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    sensor = jsonObj["sensor"];
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["sensor"] = sensor;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const { return JSON_OBJECT_SIZE(noJsonFields + 1); }
#endif
};

class MockConnection : public layout::Neighbour {
 public:
  bool addMessage(TSTRING msg) { return true; }
};

SCENARIO("We can send a custom package") {
  GIVEN("A package") {
    auto pkg = CustomPackage();
    pkg.from = 1;
    pkg.dest = 2;
    pkg.sensor = 0.5;
    REQUIRE(pkg.routing == router::SINGLE);
    REQUIRE(pkg.type == 20);
    WHEN("Converting it to and from Variant") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<CustomPackage>();
      THEN("Should result in the same values") {
        REQUIRE(pkg2.sensor == pkg.sensor);
        REQUIRE(pkg2.from == pkg.from);
        REQUIRE(pkg2.dest == pkg.dest);
        REQUIRE(pkg2.routing == pkg.routing);
        REQUIRE(pkg2.type == pkg.type);
      }
    }
  }

  GIVEN("A broadcast package") {
    auto pkg = BCustomPackage();
    pkg.from = 1;
    pkg.sensor = 0.5;
    REQUIRE(pkg.routing == router::BROADCAST);
    REQUIRE(pkg.type == 21);
    WHEN("Converting it to and from Variant") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<CustomPackage>();
      THEN("Should result in the same values") {
        REQUIRE(pkg2.sensor == pkg.sensor);
        REQUIRE(pkg2.from == pkg.from);
        REQUIRE(pkg2.routing == pkg.routing);
        REQUIRE(pkg2.type == pkg.type);
      }
    }
  }

  GIVEN("A package handler function") {
    auto handler = plugin::PackageHandler<MockConnection>();
    auto func = [](protocol::Variant& variant) {
      auto pkg = variant.to<CustomPackage>();
      REQUIRE(pkg.routing == router::BROADCAST);
      return false;
    };
    THEN("We can pass it to handler") { handler.onPackage(20, func); }
  }

  GIVEN("A package") {
    auto handler = plugin::PackageHandler<MockConnection>();
    auto pkg = CustomPackage();
    THEN("We can call sendPackage") {
      auto res = handler.sendPackage(&pkg);
      REQUIRE(!res);
    }
  }
}

SCENARIO("We can add tasks to the taskscheduler") {
  GIVEN("A couple of tasks added") {
    Scheduler mScheduler;
    auto handler = plugin::PackageHandler<MockConnection>();
    int i = 0;
    int j = 0;
    int k = 0;
    auto task1 = handler.addTask(mScheduler, 0, 1, [&i]() { ++i; });
    auto task2 = handler.addTask(mScheduler, 0, 3, [&j]() { ++j; });
    auto task3 = handler.addTask(mScheduler, 0, 3, [&k]() { ++k; });
    auto task4 = handler.addTask(mScheduler, 0, 3, []() {});

    WHEN("Executing the tasks") {
      THEN("They should be called and automatically removed") {
        REQUIRE(i == 0);
        REQUIRE(j == 0);
        mScheduler.execute();
        REQUIRE(i == 1);
        mScheduler.execute();
        REQUIRE(i == 1);
        REQUIRE(j == 2);
        // Still kept in handler, because hasn't been executed 3 times yet
        task3->disable();
        handler.stop();
      }
    }
  }
}

SCENARIO("We can add anonymous tasks to the taskscheduler") {
  GIVEN("A couple of tasks added") {
    Scheduler mScheduler;
    auto handler = plugin::PackageHandler<MockConnection>();
    int i = 0;
    int j = 0;
    int k = 0;
    handler.addTask(mScheduler, 0, 1, [&i]() { ++i; });
    handler.addTask(mScheduler, 0, 3, [&j]() { ++j; });
    handler.addTask(mScheduler, 0, 3, [&k]() { ++k; });

    WHEN("Executing the tasks") {
      THEN("They should be called and automatically removed") {
        REQUIRE(i == 0);
        REQUIRE(j == 0);
        mScheduler.execute();
        REQUIRE(i == 1);
        mScheduler.execute();
        REQUIRE(i == 1);
        REQUIRE(j == 2);
        handler.addTask(mScheduler, 0, 1, [&i]() { ++i; });
        mScheduler.execute();
        REQUIRE(i == 2);
        handler.stop();
      }
    }
  }
}

SCENARIO("BridgeCoordinationPackage serialization works correctly") {
  GIVEN("A BridgeCoordinationPackage with test data") {
    auto pkg = plugin::BridgeCoordinationPackage();
    pkg.from = 123456;
    pkg.priority = 10;
    pkg.role = "primary";
    pkg.load = 45;
    pkg.timestamp = 987654321;
    pkg.peerBridges.push_back(111111);
    pkg.peerBridges.push_back(222222);
    pkg.peerBridges.push_back(333333);
    
    REQUIRE(pkg.type == 613);
    REQUIRE(pkg.routing == router::BROADCAST);
    REQUIRE(pkg.priority == 10);
    REQUIRE(pkg.role == "primary");
    REQUIRE(pkg.load == 45);
    REQUIRE(pkg.peerBridges.size() == 3);
    
    WHEN("Converting it to and from Variant") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<plugin::BridgeCoordinationPackage>();
      
      THEN("Should result in the same values") {
        REQUIRE(pkg2.from == pkg.from);
        REQUIRE(pkg2.priority == pkg.priority);
        REQUIRE(pkg2.role == pkg.role);
        REQUIRE(pkg2.load == pkg.load);
        REQUIRE(pkg2.timestamp == pkg.timestamp);
        REQUIRE(pkg2.type == pkg.type);
        REQUIRE(pkg2.routing == pkg.routing);
        REQUIRE(pkg2.peerBridges.size() == pkg.peerBridges.size());
        
        // Check peer bridge values
        for (size_t i = 0; i < pkg.peerBridges.size(); i++) {
          REQUIRE(pkg2.peerBridges[i] == pkg.peerBridges[i]);
        }
      }
    }
    
    WHEN("Serializing to JSON") {
      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      obj = pkg.addTo(std::move(obj));
      
      THEN("Should contain all expected fields") {
        REQUIRE(obj["type"] == 613);
        REQUIRE(obj["from"] == 123456);
        REQUIRE(obj["priority"] == 10);
        REQUIRE(obj["role"] == "primary");
        REQUIRE(obj["load"] == 45);
        REQUIRE(obj["timestamp"] == 987654321);
        REQUIRE(obj["routing"] == static_cast<int>(router::BROADCAST));
        
        AND_THEN("Should have peerBridges array") {
          REQUIRE(obj["peerBridges"].is<JsonArray>());
          JsonArray peers = obj["peerBridges"];
          REQUIRE(peers.size() == 3);
          REQUIRE(peers[0] == 111111);
          REQUIRE(peers[1] == 222222);
          REQUIRE(peers[2] == 333333);
        }
      }
    }
  }
  
  GIVEN("A BridgeCoordinationPackage with empty peer list") {
    auto pkg = plugin::BridgeCoordinationPackage();
    pkg.from = 999999;
    pkg.priority = 5;
    pkg.role = "secondary";
    pkg.load = 0;
    
    WHEN("Converting to JSON and back") {
      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      obj = pkg.addTo(std::move(obj));
      
      auto pkg2 = plugin::BridgeCoordinationPackage(obj);
      
      THEN("Empty peer list should be preserved") {
        REQUIRE(pkg2.peerBridges.size() == 0);
        REQUIRE(pkg2.priority == 5);
        REQUIRE(pkg2.role == "secondary");
      }
    }
  }
  
  GIVEN("A BridgeCoordinationPackage with maximum values") {
    auto pkg = plugin::BridgeCoordinationPackage();
    pkg.from = 4294967295;  // Max uint32_t
    pkg.priority = 10;
    pkg.load = 100;
    
    // Add maximum expected peer bridges (5)
    for (uint32_t i = 1; i <= 5; i++) {
      pkg.peerBridges.push_back(i * 1000000);
    }
    
    WHEN("Serializing and deserializing") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<plugin::BridgeCoordinationPackage>();
      
      THEN("All values should be preserved correctly") {
        REQUIRE(pkg2.from == pkg.from);
        REQUIRE(pkg2.priority == pkg.priority);
        REQUIRE(pkg2.load == pkg.load);
        REQUIRE(pkg2.peerBridges.size() == 5);
      }
    }
  }
}
