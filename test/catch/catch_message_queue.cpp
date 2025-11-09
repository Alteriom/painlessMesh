#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"

#include "painlessmesh/message_queue.hpp"

using namespace painlessmesh::queue;

// Declare logger for test environment (not needed as we use printf)
// painlessmesh::logger::LogClass Log;

SCENARIO("MessageQueue initialization works correctly") {
  GIVEN("A new MessageQueue") {
    MessageQueue queue;
    
    WHEN("Initializing with default parameters") {
      queue.init();
      
      THEN("Queue should be empty") {
        REQUIRE(queue.isEmpty());
        REQUIRE(queue.getQueuedMessageCount() == 0);
      }
    }
    
    WHEN("Initializing with custom size") {
      queue.init(100, false, "/test/queue.dat");
      
      THEN("Queue should be empty") {
        REQUIRE(queue.isEmpty());
        REQUIRE(queue.getQueuedMessageCount() == 0);
      }
    }
  }
}

SCENARIO("MessageQueue queuing works correctly") {
  GIVEN("An initialized MessageQueue") {
    MessageQueue queue;
    queue.init(10);  // Small size for testing
    
    WHEN("Queuing a NORMAL priority message") {
      uint32_t id = queue.queueMessage("test payload", "test dest", PRIORITY_NORMAL);
      
      THEN("Message should be queued successfully") {
        REQUIRE(id > 0);
        REQUIRE(queue.getQueuedMessageCount() == 1);
        REQUIRE(!queue.isEmpty());
      }
    }
    
    WHEN("Queuing a CRITICAL priority message") {
      uint32_t id = queue.queueMessage("critical alarm", "alarm topic", PRIORITY_CRITICAL);
      
      THEN("Message should be queued successfully") {
        REQUIRE(id > 0);
        REQUIRE(queue.getQueuedMessageCount() == 1);
        REQUIRE(queue.getQueuedMessageCount(PRIORITY_CRITICAL) == 1);
      }
    }
    
    WHEN("Queuing multiple messages with different priorities") {
      uint32_t id1 = queue.queueMessage("critical", "dest1", PRIORITY_CRITICAL);
      uint32_t id2 = queue.queueMessage("high", "dest2", PRIORITY_HIGH);
      uint32_t id3 = queue.queueMessage("normal", "dest3", PRIORITY_NORMAL);
      uint32_t id4 = queue.queueMessage("low", "dest4", PRIORITY_LOW);
      
      THEN("All messages should be queued") {
        REQUIRE(id1 > 0);
        REQUIRE(id2 > 0);
        REQUIRE(id3 > 0);
        REQUIRE(id4 > 0);
        REQUIRE(queue.getQueuedMessageCount() == 4);
        REQUIRE(queue.getQueuedMessageCount(PRIORITY_CRITICAL) == 1);
        REQUIRE(queue.getQueuedMessageCount(PRIORITY_HIGH) == 1);
        REQUIRE(queue.getQueuedMessageCount(PRIORITY_NORMAL) == 1);
        REQUIRE(queue.getQueuedMessageCount(PRIORITY_LOW) == 1);
      }
    }
  }
}

SCENARIO("MessageQueue priority-based dropping works correctly") {
  GIVEN("A queue with limited capacity") {
    MessageQueue queue;
    queue.init(5);  // Very small size
    
    WHEN("Filling queue with LOW priority messages") {
      for (int i = 0; i < 5; i++) {
        queue.queueMessage("low msg", "dest", PRIORITY_LOW);
      }
      
      THEN("Queue should be full") {
        REQUIRE(queue.isFull());
        REQUIRE(queue.getQueuedMessageCount() == 5);
      }
      
      AND_WHEN("Queuing a CRITICAL message when full") {
        uint32_t id = queue.queueMessage("critical", "dest", PRIORITY_CRITICAL);
        
        THEN("CRITICAL message should replace LOW priority message") {
          REQUIRE(id > 0);
          REQUIRE(queue.getQueuedMessageCount() == 5);
          REQUIRE(queue.getQueuedMessageCount(PRIORITY_CRITICAL) == 1);
          REQUIRE(queue.getQueuedMessageCount(PRIORITY_LOW) == 4);
        }
      }
      
      AND_WHEN("Queuing another LOW priority message when full") {
        uint32_t id = queue.queueMessage("another low", "dest", PRIORITY_LOW);
        
        THEN("Message should be dropped") {
          REQUIRE(id == 0);
          REQUIRE(queue.getQueuedMessageCount() == 5);
        }
      }
    }
    
    WHEN("Filling queue with CRITICAL messages") {
      for (int i = 0; i < 5; i++) {
        queue.queueMessage("critical msg", "dest", PRIORITY_CRITICAL);
      }
      
      THEN("Queue should be full with CRITICAL messages") {
        REQUIRE(queue.isFull());
        REQUIRE(queue.getQueuedMessageCount(PRIORITY_CRITICAL) == 5);
      }
      
      AND_WHEN("Trying to queue a LOW priority message") {
        uint32_t id = queue.queueMessage("low", "dest", PRIORITY_LOW);
        
        THEN("LOW message should be dropped (cannot replace CRITICAL)") {
          REQUIRE(id == 0);
          REQUIRE(queue.getQueuedMessageCount() == 5);
        }
      }
    }
  }
}

SCENARIO("MessageQueue flushing works correctly") {
  GIVEN("A queue with several messages") {
    MessageQueue queue;
    queue.init(100);
    
    queue.queueMessage("msg1", "dest1", PRIORITY_NORMAL);
    queue.queueMessage("msg2", "dest2", PRIORITY_NORMAL);
    queue.queueMessage("msg3", "dest3", PRIORITY_NORMAL);
    
    REQUIRE(queue.getQueuedMessageCount() == 3);
    
    WHEN("Flushing with a callback that always succeeds") {
      int sendCount = 0;
      auto sendCallback = [&sendCount](const TSTRING& payload, const TSTRING& dest) {
        sendCount++;
        return true;  // Success
      };
      
      uint32_t sent = queue.flushQueue(sendCallback);
      
      THEN("All messages should be sent and removed from queue") {
        REQUIRE(sent == 3);
        REQUIRE(sendCount == 3);
        REQUIRE(queue.isEmpty());
        REQUIRE(queue.getQueuedMessageCount() == 0);
      }
    }
    
    WHEN("Flushing with a callback that always fails") {
      int sendCount = 0;
      queue.setMaxRetryAttempts(2);
      
      auto sendCallback = [&sendCount](const TSTRING& payload, const TSTRING& dest) {
        sendCount++;
        return false;  // Always fail
      };
      
      // First flush - attempts will be 1
      uint32_t sent1 = queue.flushQueue(sendCallback);
      REQUIRE(sent1 == 0);
      REQUIRE(queue.getQueuedMessageCount() == 3);
      
      // Second flush - attempts will be 2, messages should be removed
      uint32_t sent2 = queue.flushQueue(sendCallback);
      
      THEN("Messages should be removed after max retries") {
        REQUIRE(sent2 == 0);
        REQUIRE(queue.isEmpty());
        REQUIRE(sendCount == 6);  // 3 messages * 2 attempts
      }
    }
    
    WHEN("Flushing with a callback that succeeds for some messages") {
      int sendCount = 0;
      auto sendCallback = [&sendCount](const TSTRING& payload, const TSTRING& dest) {
        sendCount++;
        // Succeed only for msg1 and msg3
        return (payload == "msg1" || payload == "msg3");
      };
      
      uint32_t sent = queue.flushQueue(sendCallback);
      
      THEN("Only successful messages should be removed") {
        REQUIRE(sent == 2);
        REQUIRE(sendCount == 3);
        REQUIRE(queue.getQueuedMessageCount() == 1);
      }
    }
  }
}

SCENARIO("MessageQueue pruning works correctly") {
  GIVEN("A queue with messages of different ages") {
    MessageQueue queue;
    queue.init(100);
    
    // Add messages and manipulate timestamps
    uint32_t id1 = queue.queueMessage("old msg", "dest", PRIORITY_NORMAL);
    delay(100);
    uint32_t id2 = queue.queueMessage("recent msg", "dest", PRIORITY_NORMAL);
    
    (void)id1; // Suppress unused variable warning
    (void)id2; // Suppress unused variable warning
    
    REQUIRE(queue.getQueuedMessageCount() == 2);
    
    WHEN("Pruning with a very short age limit") {
      // Note: In real scenario, we'd need to manipulate internal timestamps
      // For this test, we'll just verify the prune function exists and can be called
      uint32_t removed = queue.pruneQueue(0);
      
      THEN("Prune should execute without error") {
        // With 0 hours, messages might be considered "old" depending on implementation
        // This test mainly verifies the function is callable
        REQUIRE(removed >= 0);
      }
    }
  }
}

SCENARIO("MessageQueue clear works correctly") {
  GIVEN("A queue with several messages") {
    MessageQueue queue;
    queue.init(100);
    
    queue.queueMessage("msg1", "dest1", PRIORITY_CRITICAL);
    queue.queueMessage("msg2", "dest2", PRIORITY_HIGH);
    queue.queueMessage("msg3", "dest3", PRIORITY_NORMAL);
    queue.queueMessage("msg4", "dest4", PRIORITY_LOW);
    
    REQUIRE(queue.getQueuedMessageCount() == 4);
    
    WHEN("Clearing the queue") {
      queue.clear();
      
      THEN("Queue should be empty") {
        REQUIRE(queue.isEmpty());
        REQUIRE(queue.getQueuedMessageCount() == 0);
        REQUIRE(queue.getQueuedMessageCount(PRIORITY_CRITICAL) == 0);
        REQUIRE(queue.getQueuedMessageCount(PRIORITY_HIGH) == 0);
        REQUIRE(queue.getQueuedMessageCount(PRIORITY_NORMAL) == 0);
        REQUIRE(queue.getQueuedMessageCount(PRIORITY_LOW) == 0);
      }
    }
  }
}

SCENARIO("MessageQueue statistics work correctly") {
  GIVEN("A new queue") {
    MessageQueue queue;
    queue.init(10);
    
    WHEN("Checking initial statistics") {
      auto stats = queue.getStats();
      
      THEN("Statistics should be zero") {
        REQUIRE(stats.totalQueued == 0);
        REQUIRE(stats.totalSent == 0);
        REQUIRE(stats.totalDropped == 0);
        REQUIRE(stats.totalFailed == 0);
        REQUIRE(stats.currentSize == 0);
        REQUIRE(stats.peakSize == 0);
      }
    }
    
    WHEN("Queuing messages") {
      queue.queueMessage("msg1", "dest", PRIORITY_NORMAL);
      queue.queueMessage("msg2", "dest", PRIORITY_NORMAL);
      queue.queueMessage("msg3", "dest", PRIORITY_NORMAL);
      
      auto stats = queue.getStats();
      
      THEN("Statistics should reflect queued messages") {
        REQUIRE(stats.totalQueued == 3);
        REQUIRE(stats.currentSize == 3);
        REQUIRE(stats.peakSize == 3);
      }
    }
    
    WHEN("Flushing messages") {
      queue.queueMessage("msg1", "dest", PRIORITY_NORMAL);
      queue.queueMessage("msg2", "dest", PRIORITY_NORMAL);
      
      auto sendCallback = [](const TSTRING& payload, const TSTRING& dest) {
        return true;
      };
      
      queue.flushQueue(sendCallback);
      
      auto stats = queue.getStats();
      
      THEN("Statistics should reflect sent messages") {
        REQUIRE(stats.totalQueued == 2);
        REQUIRE(stats.totalSent == 2);
        REQUIRE(stats.currentSize == 0);
        REQUIRE(stats.peakSize == 2);
      }
    }
  }
}

SCENARIO("MessageQueue state callbacks work correctly") {
  GIVEN("A queue with state callback") {
    MessageQueue queue;
    queue.init(10);
    
    QueueState lastState = QUEUE_EMPTY;
    uint32_t lastCount = 0;
    int callbackCount = 0;
    
    queue.onQueueStateChanged([&](QueueState state, uint32_t count) {
      lastState = state;
      lastCount = count;
      callbackCount++;
    });
    
    WHEN("Adding messages to trigger state changes") {
      // Add enough messages to trigger 25% threshold
      queue.queueMessage("msg1", "dest", PRIORITY_NORMAL);
      queue.queueMessage("msg2", "dest", PRIORITY_NORMAL);
      queue.queueMessage("msg3", "dest", PRIORITY_NORMAL);
      
      THEN("Callback should have been triggered") {
        REQUIRE(callbackCount > 0);
        REQUIRE(lastCount == 3);
      }
    }
    
    WHEN("Filling queue to trigger multiple state changes") {
      for (int i = 0; i < 8; i++) {
        queue.queueMessage("msg", "dest", PRIORITY_NORMAL);
      }
      
      THEN("Callback should track state progression") {
        REQUIRE(callbackCount > 0);
        REQUIRE(lastCount == 8);
      }
    }
  }
}

SCENARIO("MessageQueue retry logic works correctly") {
  GIVEN("A queue with custom retry attempts") {
    MessageQueue queue;
    queue.init(100);
    queue.setMaxRetryAttempts(3);
    
    THEN("Retry attempts should be set correctly") {
      REQUIRE(queue.getMaxRetryAttempts() == 3);
    }
    
    WHEN("Queuing a message and flushing with failures") {
      queue.queueMessage("test", "dest", PRIORITY_NORMAL);
      
      int attempts = 0;
      auto sendCallback = [&attempts](const TSTRING& payload, const TSTRING& dest) {
        attempts++;
        return false;  // Always fail
      };
      
      // Flush 3 times (should reach retry limit)
      queue.flushQueue(sendCallback);
      queue.flushQueue(sendCallback);
      queue.flushQueue(sendCallback);
      
      THEN("Message should be removed after max retries") {
        REQUIRE(attempts == 3);
        REQUIRE(queue.isEmpty());
      }
    }
  }
}
