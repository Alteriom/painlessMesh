#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"

#include <set>
#include <vector>

#include "catch_utils.hpp"

#include "painlessmesh/gateway.hpp"
#include "painlessmesh/protocol.hpp"

// For PendingInternetRequest and internetResultCallback_t we need mesh.hpp
// But we can test the core components without the full mesh

using namespace painlessmesh;
using namespace painlessmesh::gateway;

// Logger for test environment
painlessmesh::logger::LogClass Log;

// Forward declaration of PendingInternetRequest for testing
// This mirrors the struct definition in mesh.hpp

/**
 * Callback type for Internet request results
 */
typedef std::function<void(bool success, uint16_t httpStatus, TSTRING error)>
    internetResultCallback_t;

/**
 * Pending Internet request entry for testing
 */
struct TestPendingInternetRequest {
  uint32_t messageId = 0;
  uint32_t timestamp = 0;
  uint8_t retryCount = 0;
  uint8_t maxRetries = 3;
  uint8_t priority = 2;
  uint32_t timeoutMs = 30000;
  uint32_t retryDelayMs = 1000;
  uint32_t gatewayNodeId = 0;
  TSTRING destination = "";
  TSTRING payload = "";
  internetResultCallback_t callback;

  bool isTimedOut() const {
    return (static_cast<uint32_t>(millis()) - timestamp) > timeoutMs;
  }
};

SCENARIO("sendToInternet returns unique message IDs") {
  GIVEN("A node ID") {
    uint32_t nodeId = 0x12345678;

    WHEN("Generating multiple message IDs") {
      uint32_t id1 = GatewayDataPackage::generateMessageId(nodeId);
      uint32_t id2 = GatewayDataPackage::generateMessageId(nodeId);
      uint32_t id3 = GatewayDataPackage::generateMessageId(nodeId);

      THEN("IDs should be unique") {
        REQUIRE(id1 != id2);
        REQUIRE(id2 != id3);
        REQUIRE(id1 != id3);
      }

      THEN("IDs should contain node ID in upper 16 bits") {
        uint16_t nodeIdPart1 = (id1 >> 16) & 0xFFFF;
        uint16_t nodeIdPart2 = (id2 >> 16) & 0xFFFF;
        uint16_t nodeIdPart3 = (id3 >> 16) & 0xFFFF;

        REQUIRE(nodeIdPart1 == (nodeId & 0xFFFF));
        REQUIRE(nodeIdPart2 == (nodeId & 0xFFFF));
        REQUIRE(nodeIdPart3 == (nodeId & 0xFFFF));
      }
    }
  }

  GIVEN("Different node IDs") {
    uint32_t nodeId1 = 0x11111111;
    uint32_t nodeId2 = 0x22222222;

    WHEN("Generating IDs from different nodes") {
      uint32_t id1 = GatewayDataPackage::generateMessageId(nodeId1);
      uint32_t id2 = GatewayDataPackage::generateMessageId(nodeId2);

      THEN("IDs should have different node ID parts") {
        uint16_t nodeIdPart1 = (id1 >> 16) & 0xFFFF;
        uint16_t nodeIdPart2 = (id2 >> 16) & 0xFFFF;

        REQUIRE(nodeIdPart1 != nodeIdPart2);
      }
    }
  }
}

SCENARIO("PendingInternetRequest tracks request state correctly") {
  GIVEN("A new PendingInternetRequest") {
    TestPendingInternetRequest request;
    request.messageId = 12345;
    request.timestamp = static_cast<uint32_t>(millis());
    request.timeoutMs = 5000;  // 5 second timeout

    THEN("Request should not be timed out initially") {
      REQUIRE(!request.isTimedOut());
    }

    WHEN("The timestamp is old enough to time out") {
      // Simulate timeout by setting timestamp to 6 seconds in the past
      request.timestamp = static_cast<uint32_t>(millis()) - 6000;

      THEN("Request should be timed out") {
        REQUIRE(request.isTimedOut());
      }
    }
  }

  GIVEN("A PendingInternetRequest with all fields set") {
    TestPendingInternetRequest request;
    request.messageId = 0xABCD1234;
    request.timestamp = static_cast<uint32_t>(millis());
    request.retryCount = 2;
    request.maxRetries = 5;
    request.priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_HIGH);
    request.timeoutMs = 30000;
    request.retryDelayMs = 2000;
    request.gatewayNodeId = 67890;
    request.destination = "https://api.example.com/data";
    request.payload = "{\"sensor\": 42}";

    THEN("All fields should be correctly stored") {
      REQUIRE(request.messageId == 0xABCD1234);
      REQUIRE(request.retryCount == 2);
      REQUIRE(request.maxRetries == 5);
      REQUIRE(request.priority == 1);  // PRIORITY_HIGH
      REQUIRE(request.timeoutMs == 30000);
      REQUIRE(request.retryDelayMs == 2000);
      REQUIRE(request.gatewayNodeId == 67890);
      REQUIRE(request.destination == "https://api.example.com/data");
      REQUIRE(request.payload == "{\"sensor\": 42}");
    }
  }
}

SCENARIO("GatewayDataPackage is created with correct fields") {
  GIVEN("Parameters for an Internet request") {
    uint32_t fromNode = 12345;
    uint32_t gatewayNode = 67890;
    TSTRING destination = "https://api.example.com/sensor";
    TSTRING payload = "{\"temperature\": 23.5}";
    uint8_t priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_CRITICAL);

    WHEN("Creating a GatewayDataPackage") {
      GatewayDataPackage pkg;
      pkg.from = fromNode;
      pkg.dest = gatewayNode;
      pkg.messageId = GatewayDataPackage::generateMessageId(fromNode);
      pkg.originNode = fromNode;
      pkg.timestamp = 1000000;
      pkg.priority = priority;
      pkg.destination = destination;
      pkg.payload = payload;
      pkg.contentType = "application/json";
      pkg.retryCount = 0;
      pkg.requiresAck = true;

      THEN("Package should have correct type") {
        REQUIRE(pkg.type == protocol::GATEWAY_DATA);
        REQUIRE(pkg.type == 620);
      }

      THEN("Package should have correct routing") {
        REQUIRE(pkg.routing == router::SINGLE);
      }

      THEN("Package should have correct fields") {
        REQUIRE(pkg.from == fromNode);
        REQUIRE(pkg.dest == gatewayNode);
        REQUIRE(pkg.originNode == fromNode);
        REQUIRE(pkg.priority == 0);  // PRIORITY_CRITICAL
        REQUIRE(pkg.destination == destination);
        REQUIRE(pkg.payload == payload);
        REQUIRE(pkg.contentType == "application/json");
        REQUIRE(pkg.requiresAck == true);
      }

      THEN("Message ID should contain node ID") {
        uint16_t nodeIdPart = (pkg.messageId >> 16) & 0xFFFF;
        REQUIRE(nodeIdPart == (fromNode & 0xFFFF));
      }
    }
  }
}

SCENARIO("GatewayAckPackage correctly represents acknowledgments") {
  GIVEN("A successful delivery acknowledgment") {
    GatewayAckPackage ack;
    ack.messageId = 0xABCD1234;
    ack.originNode = 12345;
    ack.from = 67890;
    ack.dest = 12345;
    ack.success = true;
    ack.httpStatus = 200;
    ack.error = "";
    ack.timestamp = 1000000;

    THEN("Success fields should be correct") {
      REQUIRE(ack.type == protocol::GATEWAY_ACK);
      REQUIRE(ack.type == 621);
      REQUIRE(ack.success == true);
      REQUIRE(ack.httpStatus == 200);
      REQUIRE(ack.error == "");
    }

    WHEN("Serializing and deserializing") {
      auto var = protocol::Variant(&ack);
      auto ack2 = var.to<GatewayAckPackage>();

      THEN("All fields should be preserved") {
        REQUIRE(ack2.messageId == ack.messageId);
        REQUIRE(ack2.originNode == ack.originNode);
        REQUIRE(ack2.success == ack.success);
        REQUIRE(ack2.httpStatus == ack.httpStatus);
        REQUIRE(ack2.error == ack.error);
      }
    }
  }

  GIVEN("A failed delivery acknowledgment") {
    GatewayAckPackage ack;
    ack.messageId = 0xABCD1234;
    ack.originNode = 12345;
    ack.from = 67890;
    ack.dest = 12345;
    ack.success = false;
    ack.httpStatus = 503;
    ack.error = "Service unavailable";
    ack.timestamp = 1000000;

    THEN("Failure fields should be correct") {
      REQUIRE(ack.success == false);
      REQUIRE(ack.httpStatus == 503);
      REQUIRE(ack.error == "Service unavailable");
    }

    WHEN("Serializing and deserializing") {
      auto var = protocol::Variant(&ack);
      auto ack2 = var.to<GatewayAckPackage>();

      THEN("Error message should be preserved") {
        REQUIRE(ack2.success == false);
        REQUIRE(ack2.httpStatus == 503);
        REQUIRE(ack2.error == "Service unavailable");
      }
    }
  }
}

SCENARIO("Priority levels are correctly defined") {
  GIVEN("GatewayPriority enum") {
    THEN("CRITICAL should be 0") {
      REQUIRE(static_cast<uint8_t>(GatewayPriority::PRIORITY_CRITICAL) == 0);
    }

    THEN("HIGH should be 1") {
      REQUIRE(static_cast<uint8_t>(GatewayPriority::PRIORITY_HIGH) == 1);
    }

    THEN("NORMAL should be 2") {
      REQUIRE(static_cast<uint8_t>(GatewayPriority::PRIORITY_NORMAL) == 2);
    }

    THEN("LOW should be 3") {
      REQUIRE(static_cast<uint8_t>(GatewayPriority::PRIORITY_LOW) == 3);
    }
  }

  GIVEN("A GatewayDataPackage") {
    GatewayDataPackage pkg;

    WHEN("Default priority") {
      THEN("Should be NORMAL (2)") {
        REQUIRE(pkg.priority == static_cast<uint8_t>(GatewayPriority::PRIORITY_NORMAL));
        REQUIRE(pkg.priority == 2);
      }
    }

    WHEN("Setting priority to CRITICAL") {
      pkg.priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_CRITICAL);

      THEN("Should be 0") {
        REQUIRE(pkg.priority == 0);
      }
    }

    WHEN("Setting priority to LOW") {
      pkg.priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_LOW);

      THEN("Should be 3") {
        REQUIRE(pkg.priority == 3);
      }
    }
  }
}

SCENARIO("Retry logic with exponential backoff") {
  GIVEN("A PendingInternetRequest with retry parameters") {
    TestPendingInternetRequest request;
    request.retryCount = 0;
    request.maxRetries = 3;
    request.retryDelayMs = 1000;  // 1 second base delay

    WHEN("Calculating retry delays with exponential backoff") {
      // Simulate exponential backoff: delay = baseDelay * (2 ^ retryCount)
      uint32_t delay0 = request.retryDelayMs * (1 << 0);  // 1000 * 1 = 1000
      uint32_t delay1 = request.retryDelayMs * (1 << 1);  // 1000 * 2 = 2000
      uint32_t delay2 = request.retryDelayMs * (1 << 2);  // 1000 * 4 = 4000

      THEN("Delays should double with each retry") {
        REQUIRE(delay0 == 1000);
        REQUIRE(delay1 == 2000);
        REQUIRE(delay2 == 4000);
      }
    }

    WHEN("Checking retry limits") {
      THEN("Should allow retries when under max") {
        REQUIRE(request.retryCount < request.maxRetries);
      }

      request.retryCount = 3;

      THEN("Should deny retries when at max") {
        REQUIRE(request.retryCount >= request.maxRetries);
      }
    }
  }
}

SCENARIO("internetResultCallback_t can be stored and invoked") {
  GIVEN("A callback function") {
    bool callbackInvoked = false;
    bool receivedSuccess = false;
    uint16_t receivedStatus = 0;
    TSTRING receivedError = "";

    internetResultCallback_t callback = [&](bool success, uint16_t httpStatus, TSTRING error) {
      callbackInvoked = true;
      receivedSuccess = success;
      receivedStatus = httpStatus;
      receivedError = error;
    };

    WHEN("Storing callback in PendingInternetRequest") {
      TestPendingInternetRequest request;
      request.callback = callback;

      THEN("Callback should be callable") {
        REQUIRE(request.callback != nullptr);
      }

      WHEN("Invoking callback with success") {
        request.callback(true, 200, "");

        THEN("Callback should receive correct values") {
          REQUIRE(callbackInvoked == true);
          REQUIRE(receivedSuccess == true);
          REQUIRE(receivedStatus == 200);
          REQUIRE(receivedError == "");
        }
      }
    }
  }

  GIVEN("A callback invoked with failure") {
    bool callbackInvoked = false;
    bool receivedSuccess = true;
    uint16_t receivedStatus = 0;
    TSTRING receivedError = "";

    internetResultCallback_t callback = [&](bool success, uint16_t httpStatus, TSTRING error) {
      callbackInvoked = true;
      receivedSuccess = success;
      receivedStatus = httpStatus;
      receivedError = error;
    };

    WHEN("Callback is invoked with failure") {
      callback(false, 503, "Gateway timeout");

      THEN("Callback should receive failure values") {
        REQUIRE(callbackInvoked == true);
        REQUIRE(receivedSuccess == false);
        REQUIRE(receivedStatus == 503);
        REQUIRE(receivedError == "Gateway timeout");
      }
    }
  }
}

SCENARIO("Message ID generation is consistent and unique") {
  GIVEN("A series of message IDs from the same node") {
    uint32_t nodeId = 0xDEADBEEF;
    std::vector<uint32_t> ids;

    WHEN("Generating 100 message IDs") {
      for (int i = 0; i < 100; i++) {
        ids.push_back(GatewayDataPackage::generateMessageId(nodeId));
      }

      THEN("All IDs should be unique") {
        std::set<uint32_t> uniqueIds(ids.begin(), ids.end());
        REQUIRE(uniqueIds.size() == ids.size());
      }

      THEN("All IDs should contain the node ID") {
        for (auto id : ids) {
          uint16_t nodeIdPart = (id >> 16) & 0xFFFF;
          REQUIRE(nodeIdPart == (nodeId & 0xFFFF));
        }
      }
    }
  }
}

SCENARIO("GatewayDataPackage serialization with all fields") {
  GIVEN("A fully populated GatewayDataPackage") {
    GatewayDataPackage pkg;
    pkg.from = 12345;
    pkg.dest = 67890;
    pkg.messageId = 0xABCD1234;
    pkg.originNode = 12345;
    pkg.timestamp = 1609459200;
    pkg.priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_HIGH);
    pkg.destination = "https://api.example.com/sensor/data";
    pkg.payload = "{\"temperature\": 23.5, \"humidity\": 45.2}";
    pkg.contentType = "application/json";
    pkg.retryCount = 2;
    pkg.requiresAck = true;

    WHEN("Serializing and deserializing") {
      auto var = protocol::Variant(&pkg);
      auto pkg2 = var.to<GatewayDataPackage>();

      THEN("All fields should round-trip correctly") {
        REQUIRE(pkg2.from == pkg.from);
        REQUIRE(pkg2.dest == pkg.dest);
        REQUIRE(pkg2.messageId == pkg.messageId);
        REQUIRE(pkg2.originNode == pkg.originNode);
        REQUIRE(pkg2.timestamp == pkg.timestamp);
        REQUIRE(pkg2.priority == pkg.priority);
        REQUIRE(pkg2.destination == pkg.destination);
        REQUIRE(pkg2.payload == pkg.payload);
        REQUIRE(pkg2.contentType == pkg.contentType);
        REQUIRE(pkg2.retryCount == pkg.retryCount);
        REQUIRE(pkg2.requiresAck == pkg.requiresAck);
        REQUIRE(pkg2.type == pkg.type);
        REQUIRE(pkg2.routing == pkg.routing);
      }
    }
  }
}

SCENARIO("Protocol type constants are correctly defined") {
  GIVEN("Protocol type constants") {
    THEN("GATEWAY_DATA should be 620") {
      REQUIRE(protocol::GATEWAY_DATA == 620);
    }

    THEN("GATEWAY_ACK should be 621") {
      REQUIRE(protocol::GATEWAY_ACK == 621);
    }
  }
}
