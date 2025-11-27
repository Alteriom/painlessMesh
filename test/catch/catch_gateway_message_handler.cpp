#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "painlessmesh/gateway.hpp"

// Define logger for test environment at global scope
painlessmesh::logger::LogClass Log;

using namespace painlessmesh;
using namespace painlessmesh::gateway;

SCENARIO("GatewayMessageHandler basic configuration works correctly") {
  GIVEN("A new GatewayMessageHandler") {
    GatewayMessageHandler handler;

    THEN("Should have default configuration") {
      auto metrics = handler.getMetrics();
      REQUIRE(metrics.duplicatesDetected == 0);
      REQUIRE(metrics.messagesProcessed == 0);
      REQUIRE(metrics.acknowledgmentsSent == 0);
      REQUIRE(metrics.duplicateAcksSkipped == 0);
      REQUIRE(handler.getTrackedMessageCount() == 0);
    }

    WHEN("Configuring with SharedGatewayConfig") {
      SharedGatewayConfig config;
      config.maxTrackedMessages = 200;
      config.duplicateTrackingTimeout = 30000;

      handler.configure(config);

      THEN("Configuration should be applied") {
        // We can't directly check internal values, but we can verify
        // behavior with a stress test later
        REQUIRE(handler.getTrackedMessageCount() == 0);
      }
    }
  }
}

SCENARIO("GatewayMessageHandler duplicate message detection works correctly") {
  GIVEN("A GatewayMessageHandler with default configuration") {
    GatewayMessageHandler handler;

    WHEN("Processing a new message") {
      GatewayDataPackage pkg;
      pkg.messageId = 12345;
      pkg.originNode = 67890;
      pkg.destination = "https://api.example.com";
      pkg.payload = "{\"test\": 1}";

      bool shouldProcess = handler.handleIncomingMessage(pkg);

      THEN("Message should be processed") {
        REQUIRE(shouldProcess);
        REQUIRE(handler.getTrackedMessageCount() == 1);

        auto metrics = handler.getMetrics();
        REQUIRE(metrics.messagesProcessed == 1);
        REQUIRE(metrics.duplicatesDetected == 0);
      }
    }

    WHEN("Processing the same message twice") {
      GatewayDataPackage pkg;
      pkg.messageId = 12345;
      pkg.originNode = 67890;
      pkg.destination = "https://api.example.com";
      pkg.payload = "{\"test\": 1}";

      bool firstResult = handler.handleIncomingMessage(pkg);
      bool secondResult = handler.handleIncomingMessage(pkg);

      THEN("First should process, second should be dropped") {
        REQUIRE(firstResult);
        REQUIRE(!secondResult);
        REQUIRE(handler.getTrackedMessageCount() == 1);

        auto metrics = handler.getMetrics();
        REQUIRE(metrics.messagesProcessed == 1);
        REQUIRE(metrics.duplicatesDetected == 1);
      }
    }

    WHEN("Processing messages with same ID from different origins") {
      GatewayDataPackage pkg1;
      pkg1.messageId = 12345;
      pkg1.originNode = 111;

      GatewayDataPackage pkg2;
      pkg2.messageId = 12345;
      pkg2.originNode = 222;

      bool result1 = handler.handleIncomingMessage(pkg1);
      bool result2 = handler.handleIncomingMessage(pkg2);

      THEN("Both should be processed (different origins)") {
        REQUIRE(result1);
        REQUIRE(result2);
        REQUIRE(handler.getTrackedMessageCount() == 2);

        auto metrics = handler.getMetrics();
        REQUIRE(metrics.messagesProcessed == 2);
        REQUIRE(metrics.duplicatesDetected == 0);
      }
    }

    WHEN("Processing messages with different IDs from same origin") {
      GatewayDataPackage pkg1;
      pkg1.messageId = 111;
      pkg1.originNode = 67890;

      GatewayDataPackage pkg2;
      pkg2.messageId = 222;
      pkg2.originNode = 67890;

      bool result1 = handler.handleIncomingMessage(pkg1);
      bool result2 = handler.handleIncomingMessage(pkg2);

      THEN("Both should be processed (different message IDs)") {
        REQUIRE(result1);
        REQUIRE(result2);
        REQUIRE(handler.getTrackedMessageCount() == 2);

        auto metrics = handler.getMetrics();
        REQUIRE(metrics.messagesProcessed == 2);
        REQUIRE(metrics.duplicatesDetected == 0);
      }
    }
  }
}

SCENARIO("GatewayMessageHandler acknowledgment tracking works correctly") {
  GIVEN("A GatewayMessageHandler with a processed message") {
    GatewayMessageHandler handler;

    GatewayDataPackage pkg;
    pkg.messageId = 12345;
    pkg.originNode = 67890;
    pkg.requiresAck = true;

    handler.handleIncomingMessage(pkg);

    WHEN("Checking if ack should be sent for first time") {
      bool shouldSendAck = handler.shouldSendAcknowledgment(pkg.messageId, pkg.originNode);

      THEN("Should return true (ack not yet sent)") {
        REQUIRE(shouldSendAck);
      }
    }

    WHEN("Marking acknowledgment as sent and checking again") {
      handler.markAcknowledgmentSent(pkg.messageId, pkg.originNode);
      bool shouldSendAck = handler.shouldSendAcknowledgment(pkg.messageId, pkg.originNode);

      THEN("Should return false (ack already sent)") {
        REQUIRE(!shouldSendAck);

        auto metrics = handler.getMetrics();
        REQUIRE(metrics.acknowledgmentsSent == 1);
        REQUIRE(metrics.duplicateAcksSkipped == 1);
      }
    }

    WHEN("Checking ack status for unprocessed message") {
      bool shouldSendAck = handler.shouldSendAcknowledgment(99999, 88888);

      THEN("Should return true (message not tracked, no ack sent)") {
        REQUIRE(shouldSendAck);
      }
    }
  }
}

SCENARIO("GatewayMessageHandler sends ack for message not yet processed") {
  GIVEN("A GatewayMessageHandler") {
    GatewayMessageHandler handler;

    WHEN("Marking ack sent before processing the message") {
      uint32_t msgId = 12345;
      uint32_t originNode = 67890;

      handler.markAcknowledgmentSent(msgId, originNode);

      THEN("Message should be tracked and acknowledged") {
        REQUIRE(handler.getTrackedMessageCount() == 1);

        auto metrics = handler.getMetrics();
        REQUIRE(metrics.acknowledgmentsSent == 1);

        // Now processing the same message should detect duplicate
        GatewayDataPackage pkg;
        pkg.messageId = msgId;
        pkg.originNode = originNode;

        bool shouldProcess = handler.handleIncomingMessage(pkg);
        REQUIRE(!shouldProcess);
        REQUIRE(handler.getMetrics().duplicatesDetected == 1);
      }
    }
  }
}

SCENARIO("GatewayMessageHandler cleanup works correctly") {
  GIVEN("A GatewayMessageHandler with short timeout") {
    GatewayMessageHandler handler;

    SharedGatewayConfig config;
    config.maxTrackedMessages = 100;
    config.duplicateTrackingTimeout = 10;  // 10ms timeout
    handler.configure(config);

    WHEN("Adding messages and waiting for expiry") {
      GatewayDataPackage pkg1;
      pkg1.messageId = 111;
      pkg1.originNode = 1;

      GatewayDataPackage pkg2;
      pkg2.messageId = 222;
      pkg2.originNode = 1;

      handler.handleIncomingMessage(pkg1);
      handler.handleIncomingMessage(pkg2);

      REQUIRE(handler.getTrackedMessageCount() == 2);

      // Wait for messages to expire
      delay(20000);  // 20ms in test env

      uint32_t removed = handler.cleanup();

      THEN("Expired messages should be removed") {
        REQUIRE(removed == 2);
        REQUIRE(handler.getTrackedMessageCount() == 0);
      }
    }
  }
}

SCENARIO("GatewayMessageHandler metrics work correctly") {
  GIVEN("A GatewayMessageHandler with various operations") {
    GatewayMessageHandler handler;

    // Process some messages
    GatewayDataPackage pkg1;
    pkg1.messageId = 111;
    pkg1.originNode = 1;
    handler.handleIncomingMessage(pkg1);

    GatewayDataPackage pkg2;
    pkg2.messageId = 222;
    pkg2.originNode = 1;
    handler.handleIncomingMessage(pkg2);

    // Duplicate
    handler.handleIncomingMessage(pkg1);

    // Send acks
    handler.markAcknowledgmentSent(pkg1.messageId, pkg1.originNode);

    // Duplicate ack check
    handler.shouldSendAcknowledgment(pkg1.messageId, pkg1.originNode);

    THEN("Metrics should reflect all operations") {
      auto metrics = handler.getMetrics();
      REQUIRE(metrics.messagesProcessed == 2);
      REQUIRE(metrics.duplicatesDetected == 1);
      REQUIRE(metrics.acknowledgmentsSent == 1);
      REQUIRE(metrics.duplicateAcksSkipped == 1);
    }

    WHEN("Resetting metrics") {
      handler.resetMetrics();

      THEN("All metrics should be zero") {
        auto metrics = handler.getMetrics();
        REQUIRE(metrics.duplicatesDetected == 0);
        REQUIRE(metrics.messagesProcessed == 0);
        REQUIRE(metrics.acknowledgmentsSent == 0);
        REQUIRE(metrics.duplicateAcksSkipped == 0);
      }

      AND_THEN("Tracked messages should still exist") {
        REQUIRE(handler.getTrackedMessageCount() == 2);
      }
    }
  }
}

SCENARIO("GatewayMetrics rate calculations work correctly") {
  GIVEN("GatewayMetrics with various values") {
    GatewayMetrics metrics;

    WHEN("No messages processed") {
      THEN("Duplicate rate should be 0") {
        REQUIRE(metrics.getDuplicateRate() == 0);
        REQUIRE(metrics.getDuplicateAckRate() == 0);
      }
    }

    WHEN("Some duplicates detected") {
      metrics.messagesProcessed = 80;
      metrics.duplicatesDetected = 20;

      THEN("Duplicate rate should be 20%") {
        REQUIRE(metrics.getDuplicateRate() == 20);
      }
    }

    WHEN("Some duplicate acks skipped") {
      metrics.acknowledgmentsSent = 90;
      metrics.duplicateAcksSkipped = 10;

      THEN("Duplicate ack rate should be 10%") {
        REQUIRE(metrics.getDuplicateAckRate() == 10);
      }
    }

    WHEN("Reset is called") {
      metrics.messagesProcessed = 100;
      metrics.duplicatesDetected = 50;
      metrics.acknowledgmentsSent = 75;
      metrics.duplicateAcksSkipped = 25;

      metrics.reset();

      THEN("All values should be zero") {
        REQUIRE(metrics.duplicatesDetected == 0);
        REQUIRE(metrics.messagesProcessed == 0);
        REQUIRE(metrics.acknowledgmentsSent == 0);
        REQUIRE(metrics.duplicateAcksSkipped == 0);
      }
    }
  }
}

SCENARIO("GatewayMessageHandler handles high message volume") {
  GIVEN("A GatewayMessageHandler with adequate capacity") {
    GatewayMessageHandler handler;

    SharedGatewayConfig config;
    config.maxTrackedMessages = 1000;
    config.duplicateTrackingTimeout = 60000;
    handler.configure(config);

    WHEN("Processing many unique messages") {
      for (uint32_t i = 0; i < 500; i++) {
        GatewayDataPackage pkg;
        pkg.messageId = i;
        pkg.originNode = 1;
        handler.handleIncomingMessage(pkg);
      }

      THEN("All messages should be tracked") {
        REQUIRE(handler.getTrackedMessageCount() == 500);

        auto metrics = handler.getMetrics();
        REQUIRE(metrics.messagesProcessed == 500);
        REQUIRE(metrics.duplicatesDetected == 0);
      }
    }

    WHEN("Processing many messages with duplicates") {
      // First batch - unique
      for (uint32_t i = 0; i < 100; i++) {
        GatewayDataPackage pkg;
        pkg.messageId = i;
        pkg.originNode = 1;
        handler.handleIncomingMessage(pkg);
      }

      // Second batch - same messages (duplicates)
      for (uint32_t i = 0; i < 100; i++) {
        GatewayDataPackage pkg;
        pkg.messageId = i;
        pkg.originNode = 1;
        handler.handleIncomingMessage(pkg);
      }

      THEN("Duplicates should be detected") {
        REQUIRE(handler.getTrackedMessageCount() == 100);

        auto metrics = handler.getMetrics();
        REQUIRE(metrics.messagesProcessed == 100);
        REQUIRE(metrics.duplicatesDetected == 100);
        REQUIRE(metrics.getDuplicateRate() == 50);  // 100/(100+100) = 50%
      }
    }
  }
}

SCENARIO("GatewayMessageHandler memory limit enforcement") {
  GIVEN("A GatewayMessageHandler with small capacity") {
    GatewayMessageHandler handler;

    SharedGatewayConfig config;
    config.maxTrackedMessages = 5;
    config.duplicateTrackingTimeout = 60000;
    handler.configure(config);

    WHEN("Adding more messages than capacity") {
      for (uint32_t i = 0; i < 10; i++) {
        GatewayDataPackage pkg;
        pkg.messageId = i;
        pkg.originNode = 1;
        delay(1000);  // Small delay to ensure different timestamps
        handler.handleIncomingMessage(pkg);
      }

      THEN("Only capacity limit messages should be tracked") {
        REQUIRE(handler.getTrackedMessageCount() == 5);

        auto metrics = handler.getMetrics();
        REQUIRE(metrics.messagesProcessed == 10);
        REQUIRE(metrics.duplicatesDetected == 0);  // All were unique
      }
    }
  }
}

SCENARIO("GatewayMessageHandler clearTrackedMessages works correctly") {
  GIVEN("A GatewayMessageHandler with tracked messages") {
    GatewayMessageHandler handler;

    GatewayDataPackage pkg1;
    pkg1.messageId = 111;
    pkg1.originNode = 1;
    handler.handleIncomingMessage(pkg1);

    GatewayDataPackage pkg2;
    pkg2.messageId = 222;
    pkg2.originNode = 2;
    handler.handleIncomingMessage(pkg2);

    REQUIRE(handler.getTrackedMessageCount() == 2);

    WHEN("Clearing tracked messages") {
      handler.clearTrackedMessages();

      THEN("Tracker should be empty") {
        REQUIRE(handler.getTrackedMessageCount() == 0);
      }

      AND_THEN("Metrics should be preserved") {
        auto metrics = handler.getMetrics();
        REQUIRE(metrics.messagesProcessed == 2);
      }

      AND_THEN("Previously processed messages should be processable again") {
        bool result = handler.handleIncomingMessage(pkg1);
        REQUIRE(result);
        REQUIRE(handler.getTrackedMessageCount() == 1);
      }
    }
  }
}

SCENARIO("GatewayMessageHandler network partition scenario") {
  GIVEN("A GatewayMessageHandler simulating network partition") {
    GatewayMessageHandler handler;

    WHEN("Message arrives, is processed, and ack sent") {
      GatewayDataPackage pkg;
      pkg.messageId = 12345;
      pkg.originNode = 67890;
      pkg.requiresAck = true;

      // First arrival and processing
      REQUIRE(handler.handleIncomingMessage(pkg));
      REQUIRE(handler.shouldSendAcknowledgment(pkg.messageId, pkg.originNode));
      handler.markAcknowledgmentSent(pkg.messageId, pkg.originNode);

      // Simulate network partition - message arrives again (retry)
      bool secondProcess = handler.handleIncomingMessage(pkg);
      bool secondAck = handler.shouldSendAcknowledgment(pkg.messageId, pkg.originNode);

      THEN("Duplicate message and ack should be blocked") {
        REQUIRE(!secondProcess);
        REQUIRE(!secondAck);

        auto metrics = handler.getMetrics();
        REQUIRE(metrics.messagesProcessed == 1);
        REQUIRE(metrics.duplicatesDetected == 1);
        REQUIRE(metrics.acknowledgmentsSent == 1);
        REQUIRE(metrics.duplicateAcksSkipped == 1);
      }
    }
  }
}
