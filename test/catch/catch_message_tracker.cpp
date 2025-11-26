#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "painlessmesh/message_tracker.hpp"

// Define logger for test environment at global scope
painlessmesh::logger::LogClass Log;

using namespace painlessmesh;

SCENARIO("MessageTracker basic operations work correctly") {
  GIVEN("An empty message tracker with capacity 100") {
    MessageTracker tracker(100, 60000);
    
    REQUIRE(tracker.empty());
    REQUIRE(tracker.size() == 0);
    REQUIRE(tracker.getMaxMessages() == 100);
    REQUIRE(tracker.getTimeoutMs() == 60000);
    
    WHEN("Checking if an unprocessed message is processed") {
      bool processed = tracker.isProcessed(12345, 67890);
      
      THEN("Should return false") {
        REQUIRE(!processed);
      }
    }
    
    WHEN("Marking a message as processed") {
      tracker.markProcessed(12345, 67890);
      
      THEN("Message should be tracked") {
        REQUIRE(tracker.size() == 1);
        REQUIRE(!tracker.empty());
        REQUIRE(tracker.isProcessed(12345, 67890));
      }
    }
    
    WHEN("Marking multiple messages as processed") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(101, 1);
      tracker.markProcessed(102, 2);
      tracker.markProcessed(103, 2);
      
      THEN("All messages should be tracked") {
        REQUIRE(tracker.size() == 4);
        REQUIRE(tracker.isProcessed(100, 1));
        REQUIRE(tracker.isProcessed(101, 1));
        REQUIRE(tracker.isProcessed(102, 2));
        REQUIRE(tracker.isProcessed(103, 2));
      }
    }
    
    WHEN("Marking the same message twice") {
      tracker.markProcessed(12345, 67890);
      tracker.markProcessed(12345, 67890);
      
      THEN("Should only have one entry") {
        REQUIRE(tracker.size() == 1);
        REQUIRE(tracker.isProcessed(12345, 67890));
      }
    }
    
    WHEN("Clearing the tracker") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(101, 2);
      tracker.markProcessed(102, 3);
      
      tracker.clear();
      
      THEN("Tracker should be empty") {
        REQUIRE(tracker.empty());
        REQUIRE(tracker.size() == 0);
        REQUIRE(!tracker.isProcessed(100, 1));
        REQUIRE(!tracker.isProcessed(101, 2));
        REQUIRE(!tracker.isProcessed(102, 3));
      }
    }
  }
}

SCENARIO("MessageTracker acknowledgment tracking works correctly") {
  GIVEN("A message tracker with some processed messages") {
    MessageTracker tracker(100, 60000);
    
    tracker.markProcessed(100, 1);
    tracker.markProcessed(101, 1);
    tracker.markProcessed(102, 2);
    
    WHEN("Checking acknowledgment status of unacknowledged message") {
      bool acked = tracker.isAcknowledged(100, 1);
      
      THEN("Should return false") {
        REQUIRE(!acked);
      }
    }
    
    WHEN("Marking a message as acknowledged") {
      bool result = tracker.markAcknowledged(100, 1);
      
      THEN("Should succeed and update status") {
        REQUIRE(result);
        REQUIRE(tracker.isAcknowledged(100, 1));
        REQUIRE(!tracker.isAcknowledged(101, 1));
        REQUIRE(!tracker.isAcknowledged(102, 2));
      }
    }
    
    WHEN("Marking a non-existent message as acknowledged") {
      bool result = tracker.markAcknowledged(999, 999);
      
      THEN("Should return false") {
        REQUIRE(!result);
      }
    }
    
    WHEN("Checking acknowledgment of non-existent message") {
      bool acked = tracker.isAcknowledged(999, 999);
      
      THEN("Should return false") {
        REQUIRE(!acked);
      }
    }
    
    WHEN("Marking multiple messages as acknowledged") {
      tracker.markAcknowledged(100, 1);
      tracker.markAcknowledged(101, 1);
      
      THEN("Both should be acknowledged") {
        REQUIRE(tracker.isAcknowledged(100, 1));
        REQUIRE(tracker.isAcknowledged(101, 1));
        REQUIRE(!tracker.isAcknowledged(102, 2));
      }
    }
  }
}

SCENARIO("MessageTracker cleanup of expired entries works correctly") {
  GIVEN("A message tracker with short timeout") {
    MessageTracker tracker(100, 10);  // 10ms timeout
    
    WHEN("Adding messages and waiting for expiry") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(101, 1);
      
      // Wait for messages to expire
      delay(20000);  // 20ms (delay uses usleep with microseconds)
      
      uint32_t removed = tracker.cleanup();
      
      THEN("Expired messages should be removed") {
        REQUIRE(removed == 2);
        REQUIRE(tracker.empty());
        REQUIRE(!tracker.isProcessed(100, 1));
        REQUIRE(!tracker.isProcessed(101, 1));
      }
    }
    
    WHEN("Cleanup with no expired messages") {
      tracker.markProcessed(100, 1);
      
      // Cleanup immediately (no time passed)
      uint32_t removed = tracker.cleanup();
      
      THEN("No messages should be removed") {
        REQUIRE(removed == 0);
        REQUIRE(tracker.size() == 1);
        REQUIRE(tracker.isProcessed(100, 1));
      }
    }
    
    WHEN("Partial expiry of messages") {
      tracker.markProcessed(100, 1);
      
      // Wait for first message to expire
      delay(15000);  // 15ms
      
      // Add another message (this one is fresh)
      tracker.markProcessed(101, 1);
      
      uint32_t removed = tracker.cleanup();
      
      THEN("Only expired message should be removed") {
        REQUIRE(removed == 1);
        REQUIRE(tracker.size() == 1);
        REQUIRE(!tracker.isProcessed(100, 1));
        REQUIRE(tracker.isProcessed(101, 1));
      }
    }
  }
  
  GIVEN("A message tracker with long timeout") {
    MessageTracker tracker(100, 3600000);  // 1 hour timeout
    
    WHEN("Cleanup is called on fresh messages") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(101, 2);
      tracker.markProcessed(102, 3);
      
      uint32_t removed = tracker.cleanup();
      
      THEN("No messages should be removed") {
        REQUIRE(removed == 0);
        REQUIRE(tracker.size() == 3);
      }
    }
  }
}

SCENARIO("MessageTracker memory limit enforcement works correctly") {
  GIVEN("A message tracker with capacity 5") {
    MessageTracker tracker(5, 60000);
    
    WHEN("Adding messages up to capacity") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(101, 1);
      tracker.markProcessed(102, 1);
      tracker.markProcessed(103, 1);
      tracker.markProcessed(104, 1);
      
      THEN("All messages should be tracked") {
        REQUIRE(tracker.size() == 5);
        REQUIRE(tracker.isProcessed(100, 1));
        REQUIRE(tracker.isProcessed(101, 1));
        REQUIRE(tracker.isProcessed(102, 1));
        REQUIRE(tracker.isProcessed(103, 1));
        REQUIRE(tracker.isProcessed(104, 1));
      }
    }
    
    WHEN("Adding messages beyond capacity") {
      // Add 5 messages
      tracker.markProcessed(100, 1);
      delay(1000);  // Small delay to ensure different timestamps
      tracker.markProcessed(101, 1);
      delay(1000);
      tracker.markProcessed(102, 1);
      delay(1000);
      tracker.markProcessed(103, 1);
      delay(1000);
      tracker.markProcessed(104, 1);
      delay(1000);
      
      // Add 6th message (should evict oldest)
      tracker.markProcessed(105, 1);
      
      THEN("Oldest message should be evicted") {
        REQUIRE(tracker.size() == 5);
        REQUIRE(!tracker.isProcessed(100, 1));  // Evicted
        REQUIRE(tracker.isProcessed(101, 1));
        REQUIRE(tracker.isProcessed(102, 1));
        REQUIRE(tracker.isProcessed(103, 1));
        REQUIRE(tracker.isProcessed(104, 1));
        REQUIRE(tracker.isProcessed(105, 1));   // New entry
      }
    }
    
    WHEN("Reducing max messages below current size") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(101, 1);
      tracker.markProcessed(102, 1);
      tracker.markProcessed(103, 1);
      tracker.markProcessed(104, 1);
      
      REQUIRE(tracker.size() == 5);
      
      tracker.setMaxMessages(3);
      
      THEN("Oldest entries should be removed to fit new limit") {
        REQUIRE(tracker.size() == 3);
        REQUIRE(tracker.getMaxMessages() == 3);
      }
    }
  }
  
  GIVEN("A message tracker with capacity 1") {
    MessageTracker tracker(1, 60000);
    
    WHEN("Adding two messages") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(101, 1);
      
      THEN("Only the newest should remain") {
        REQUIRE(tracker.size() == 1);
        REQUIRE(!tracker.isProcessed(100, 1));
        REQUIRE(tracker.isProcessed(101, 1));
      }
    }
  }
}

SCENARIO("MessageTracker handles different message ID and origin combinations") {
  GIVEN("A message tracker") {
    MessageTracker tracker(100, 60000);
    
    WHEN("Same message ID from different origin nodes") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(100, 2);
      tracker.markProcessed(100, 3);
      
      THEN("Each should be tracked separately") {
        REQUIRE(tracker.size() == 3);
        REQUIRE(tracker.isProcessed(100, 1));
        REQUIRE(tracker.isProcessed(100, 2));
        REQUIRE(tracker.isProcessed(100, 3));
        REQUIRE(!tracker.isProcessed(100, 4));  // Not tracked
      }
    }
    
    WHEN("Different message IDs from same origin node") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(101, 1);
      tracker.markProcessed(102, 1);
      
      THEN("Each should be tracked separately") {
        REQUIRE(tracker.size() == 3);
        REQUIRE(tracker.isProcessed(100, 1));
        REQUIRE(tracker.isProcessed(101, 1));
        REQUIRE(tracker.isProcessed(102, 1));
        REQUIRE(!tracker.isProcessed(103, 1));  // Not tracked
      }
    }
    
    WHEN("Acknowledging specific message from specific node") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(100, 2);
      
      tracker.markAcknowledged(100, 1);
      
      THEN("Only that specific combination should be acknowledged") {
        REQUIRE(tracker.isAcknowledged(100, 1));
        REQUIRE(!tracker.isAcknowledged(100, 2));
      }
    }
  }
}

SCENARIO("MessageTracker configuration changes work correctly") {
  GIVEN("A message tracker with default configuration") {
    MessageTracker tracker(100, 60000);
    
    WHEN("Changing timeout value") {
      tracker.setTimeoutMs(30000);
      
      THEN("New timeout should be set") {
        REQUIRE(tracker.getTimeoutMs() == 30000);
      }
    }
    
    WHEN("Changing max messages to larger value") {
      tracker.markProcessed(100, 1);
      tracker.markProcessed(101, 1);
      
      tracker.setMaxMessages(200);
      
      THEN("Max should increase and existing entries preserved") {
        REQUIRE(tracker.getMaxMessages() == 200);
        REQUIRE(tracker.size() == 2);
        REQUIRE(tracker.isProcessed(100, 1));
        REQUIRE(tracker.isProcessed(101, 1));
      }
    }
  }
}

SCENARIO("MessageTracker handles edge cases correctly") {
  GIVEN("An empty message tracker") {
    MessageTracker tracker(100, 60000);
    
    WHEN("Cleanup is called on empty tracker") {
      uint32_t removed = tracker.cleanup();
      
      THEN("Should return 0 and not crash") {
        REQUIRE(removed == 0);
        REQUIRE(tracker.empty());
      }
    }
    
    WHEN("Acknowledging non-existent message") {
      bool result = tracker.markAcknowledged(999, 999);
      
      THEN("Should return false") {
        REQUIRE(!result);
      }
    }
    
    WHEN("Clearing empty tracker") {
      tracker.clear();
      
      THEN("Should not crash and remain empty") {
        REQUIRE(tracker.empty());
      }
    }
  }
  
  GIVEN("A message tracker with capacity 0") {
    MessageTracker tracker(0, 60000);
    
    WHEN("Trying to add a message") {
      tracker.markProcessed(100, 1);
      
      THEN("Message should be evicted immediately due to 0 capacity") {
        // With capacity 0, the message is added then immediately evicted
        // to enforce the limit
        REQUIRE(tracker.size() == 0);
      }
    }
  }
  
  GIVEN("A message tracker") {
    MessageTracker tracker(100, 60000);
    
    WHEN("Using maximum uint32_t values for message ID and origin") {
      tracker.markProcessed(0xFFFFFFFF, 0xFFFFFFFF);
      
      THEN("Should handle correctly") {
        REQUIRE(tracker.size() == 1);
        REQUIRE(tracker.isProcessed(0xFFFFFFFF, 0xFFFFFFFF));
      }
    }
    
    WHEN("Using zero values for message ID and origin") {
      tracker.markProcessed(0, 0);
      
      THEN("Should handle correctly") {
        REQUIRE(tracker.size() == 1);
        REQUIRE(tracker.isProcessed(0, 0));
      }
    }
  }
}

SCENARIO("MessageTracker preserves acknowledgment on re-processing") {
  GIVEN("A tracker with an acknowledged message") {
    MessageTracker tracker(100, 60000);
    
    tracker.markProcessed(100, 1);
    tracker.markAcknowledged(100, 1);
    
    REQUIRE(tracker.isAcknowledged(100, 1));
    
    WHEN("Re-marking the same message as processed") {
      tracker.markProcessed(100, 1);
      
      THEN("Acknowledgment status should be preserved") {
        REQUIRE(tracker.isProcessed(100, 1));
        REQUIRE(tracker.isAcknowledged(100, 1));
      }
    }
  }
}

SCENARIO("MessageTracker performance with high message rates") {
  GIVEN("A message tracker with large capacity") {
    MessageTracker tracker(1000, 60000);
    
    WHEN("Adding many messages rapidly") {
      for (uint32_t i = 0; i < 500; i++) {
        tracker.markProcessed(i, 1);
      }
      
      THEN("All messages should be tracked") {
        REQUIRE(tracker.size() == 500);
        
        // Verify some random samples
        REQUIRE(tracker.isProcessed(0, 1));
        REQUIRE(tracker.isProcessed(250, 1));
        REQUIRE(tracker.isProcessed(499, 1));
        REQUIRE(!tracker.isProcessed(500, 1));
      }
    }
    
    WHEN("Checking many messages for processing status") {
      // Pre-populate with some messages
      for (uint32_t i = 0; i < 100; i++) {
        tracker.markProcessed(i, 1);
      }
      
      // Check many messages (both existing and non-existing)
      int foundCount = 0;
      for (uint32_t i = 0; i < 200; i++) {
        if (tracker.isProcessed(i, 1)) {
          foundCount++;
        }
      }
      
      THEN("Should correctly identify processed messages") {
        REQUIRE(foundCount == 100);
      }
    }
  }
}
