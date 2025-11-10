#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "painlessmesh/message_queue.hpp"

// Define logger for test environment in correct namespace
namespace painlessmesh {
  logger::LogClass Log;
}

using namespace painlessmesh;

SCENARIO("MessageQueue basic operations work correctly") {
  GIVEN("An empty message queue with capacity 10") {
    MessageQueue queue(10);
    
    REQUIRE(queue.empty());
    REQUIRE(queue.size() == 0);
    
    WHEN("Enqueueing a NORMAL priority message") {
      uint32_t msgId = queue.enqueue(PRIORITY_NORMAL, "test_payload", "mqtt://test");
      
      THEN("Message should be queued successfully") {
        REQUIRE(msgId != 0);
        REQUIRE(queue.size() == 1);
        REQUIRE(!queue.empty());
        
        auto messages = queue.getMessages();
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].id == msgId);
        REQUIRE(messages[0].priority == PRIORITY_NORMAL);
        REQUIRE(messages[0].payload == "test_payload");
        REQUIRE(messages[0].destination == "mqtt://test");
        REQUIRE(messages[0].attempts == 0);
      }
    }
    
    WHEN("Enqueueing multiple messages") {
      uint32_t id1 = queue.enqueue(PRIORITY_CRITICAL, "critical");
      uint32_t id2 = queue.enqueue(PRIORITY_HIGH, "high");
      uint32_t id3 = queue.enqueue(PRIORITY_NORMAL, "normal");
      uint32_t id4 = queue.enqueue(PRIORITY_LOW, "low");
      
      THEN("All messages should be queued") {
        REQUIRE(queue.size() == 4);
        REQUIRE(queue.size(PRIORITY_CRITICAL) == 1);
        REQUIRE(queue.size(PRIORITY_HIGH) == 1);
        REQUIRE(queue.size(PRIORITY_NORMAL) == 1);
        REQUIRE(queue.size(PRIORITY_LOW) == 1);
      }
    }
    
    WHEN("Removing a message") {
      uint32_t msgId = queue.enqueue(PRIORITY_NORMAL, "test");
      bool removed = queue.remove(msgId);
      
      THEN("Message should be removed") {
        REQUIRE(removed);
        REQUIRE(queue.empty());
        REQUIRE(queue.size() == 0);
      }
    }
    
    WHEN("Clearing the queue") {
      queue.enqueue(PRIORITY_NORMAL, "msg1");
      queue.enqueue(PRIORITY_NORMAL, "msg2");
      queue.enqueue(PRIORITY_NORMAL, "msg3");
      
      queue.clear();
      
      THEN("Queue should be empty") {
        REQUIRE(queue.empty());
        REQUIRE(queue.size() == 0);
      }
    }
  }
}

SCENARIO("MessageQueue priority-based eviction works correctly") {
  GIVEN("A message queue with capacity 5") {
    MessageQueue queue(5);
    
    WHEN("Queue is full with LOW priority messages") {
      for (int i = 0; i < 5; i++) {
        queue.enqueue(PRIORITY_LOW, "low_msg");
      }
      
      REQUIRE(queue.size() == 5);
      
      AND_WHEN("Attempting to queue a CRITICAL message") {
        uint32_t criticalId = queue.enqueue(PRIORITY_CRITICAL, "critical_msg");
        
        THEN("CRITICAL message should be queued by evicting LOW") {
          REQUIRE(criticalId != 0);
          REQUIRE(queue.size() == 5);
          REQUIRE(queue.size(PRIORITY_CRITICAL) == 1);
          REQUIRE(queue.size(PRIORITY_LOW) == 4);
        }
      }
      
      AND_WHEN("Attempting to queue a HIGH message") {
        uint32_t highId = queue.enqueue(PRIORITY_HIGH, "high_msg");
        
        THEN("HIGH message should be queued by evicting LOW") {
          REQUIRE(highId != 0);
          REQUIRE(queue.size() == 5);
          REQUIRE(queue.size(PRIORITY_HIGH) == 1);
          REQUIRE(queue.size(PRIORITY_LOW) == 4);
        }
      }
      
      AND_WHEN("Attempting to queue another LOW message") {
        uint32_t lowId = queue.enqueue(PRIORITY_LOW, "another_low");
        
        THEN("LOW message should be dropped") {
          REQUIRE(lowId == 0);
          REQUIRE(queue.size() == 5);
        }
      }
    }
    
    WHEN("Queue is full with NORMAL priority messages") {
      for (int i = 0; i < 5; i++) {
        queue.enqueue(PRIORITY_NORMAL, "normal_msg");
      }
      
      REQUIRE(queue.size() == 5);
      
      AND_WHEN("Attempting to queue a CRITICAL message") {
        uint32_t criticalId = queue.enqueue(PRIORITY_CRITICAL, "critical_msg");
        
        THEN("CRITICAL message should be queued by evicting NORMAL") {
          REQUIRE(criticalId != 0);
          REQUIRE(queue.size() == 5);
          REQUIRE(queue.size(PRIORITY_CRITICAL) == 1);
          REQUIRE(queue.size(PRIORITY_NORMAL) == 4);
        }
      }
      
      AND_WHEN("Attempting to queue a LOW message") {
        uint32_t lowId = queue.enqueue(PRIORITY_LOW, "low_msg");
        
        THEN("LOW message should be dropped") {
          REQUIRE(lowId == 0);
          REQUIRE(queue.size() == 5);
        }
      }
    }
    
    WHEN("Queue is full with CRITICAL messages") {
      for (int i = 0; i < 5; i++) {
        queue.enqueue(PRIORITY_CRITICAL, "critical_msg");
      }
      
      REQUIRE(queue.size() == 5);
      
      AND_WHEN("Attempting to queue any priority message") {
        uint32_t lowId = queue.enqueue(PRIORITY_LOW, "low");
        uint32_t normalId = queue.enqueue(PRIORITY_NORMAL, "normal");
        uint32_t highId = queue.enqueue(PRIORITY_HIGH, "high");
        
        THEN("All messages should be dropped (can't evict CRITICAL)") {
          REQUIRE(lowId == 0);
          REQUIRE(normalId == 0);
          REQUIRE(highId == 0);
          REQUIRE(queue.size() == 5);
          REQUIRE(queue.size(PRIORITY_CRITICAL) == 5);
        }
      }
    }
  }
}

SCENARIO("MessageQueue statistics tracking works correctly") {
  GIVEN("A message queue with capacity 10") {
    MessageQueue queue(10);
    
    WHEN("Enqueueing and removing messages") {
      queue.enqueue(PRIORITY_CRITICAL, "msg1");
      queue.enqueue(PRIORITY_HIGH, "msg2");
      queue.enqueue(PRIORITY_NORMAL, "msg3");
      
      auto stats = queue.getStats();
      
      THEN("Statistics should reflect operations") {
        REQUIRE(stats.totalQueued == 3);
        REQUIRE(stats.currentSize == 3);
        REQUIRE(stats.criticalQueued == 1);
        REQUIRE(stats.highQueued == 1);
        REQUIRE(stats.normalQueued == 1);
        REQUIRE(stats.lowQueued == 0);
        REQUIRE(stats.totalSent == 0);
        REQUIRE(stats.totalDropped == 0);
      }
      
      AND_WHEN("Removing a message") {
        auto messages = queue.getMessages();
        queue.remove(messages[0].id);
        
        stats = queue.getStats();
        
        THEN("Statistics should update") {
          REQUIRE(stats.totalSent == 1);
          REQUIRE(stats.currentSize == 2);
        }
      }
    }
    
    WHEN("Messages are dropped due to queue full") {
      // Fill queue with CRITICAL messages
      for (int i = 0; i < 10; i++) {
        queue.enqueue(PRIORITY_CRITICAL, "critical");
      }
      
      // Try to add LOW priority (will be dropped)
      queue.enqueue(PRIORITY_LOW, "low");
      
      auto stats = queue.getStats();
      
      THEN("Dropped count should increment") {
        REQUIRE(stats.totalDropped == 1);
        REQUIRE(stats.totalQueued == 10);
      }
    }
  }
}

SCENARIO("MessageQueue attempt counter works correctly") {
  GIVEN("A queue with one message") {
    MessageQueue queue(10);
    uint32_t msgId = queue.enqueue(PRIORITY_NORMAL, "test");
    
    WHEN("Incrementing attempts") {
      uint32_t attempts1 = queue.incrementAttempts(msgId);
      uint32_t attempts2 = queue.incrementAttempts(msgId);
      uint32_t attempts3 = queue.incrementAttempts(msgId);
      
      THEN("Attempt counter should increment") {
        REQUIRE(attempts1 == 1);
        REQUIRE(attempts2 == 2);
        REQUIRE(attempts3 == 3);
        
        auto messages = queue.getMessages();
        REQUIRE(messages[0].attempts == 3);
      }
    }
    
    WHEN("Incrementing attempts for non-existent message") {
      uint32_t attempts = queue.incrementAttempts(99999);
      
      THEN("Should return 0") {
        REQUIRE(attempts == 0);
      }
    }
  }
}

SCENARIO("MessageQueue state change callback works correctly") {
  GIVEN("A queue with state change callback") {
    MessageQueue queue(10);
    
    QueueState lastState = QUEUE_EMPTY;
    uint32_t lastCount = 0;
    
    queue.onStateChanged([&lastState, &lastCount](QueueState state, uint32_t count) {
      lastState = state;
      lastCount = count;
    });
    
    WHEN("Queue transitions from EMPTY to NORMAL") {
      queue.enqueue(PRIORITY_NORMAL, "msg1");
      
      THEN("Callback should be fired with NORMAL state") {
        REQUIRE(lastState == QUEUE_NORMAL);
        REQUIRE(lastCount == 1);
      }
    }
    
    WHEN("Queue reaches 75% capacity") {
      for (int i = 0; i < 8; i++) {  // 8/10 = 80% > 75%
        queue.enqueue(PRIORITY_NORMAL, "msg");
      }
      
      THEN("Callback should be fired with 75_PERCENT state") {
        REQUIRE(lastState == QUEUE_75_PERCENT);
        // With maxQueueSize=10, threshold is 10*3/4=7 (integer division)
        // Callback fires when 7th message is added (first to reach threshold)
        REQUIRE(lastCount == 7);
      }
    }
    
    WHEN("Queue becomes full") {
      for (int i = 0; i < 10; i++) {
        queue.enqueue(PRIORITY_NORMAL, "msg");
      }
      
      THEN("Callback should be fired with FULL state") {
        REQUIRE(lastState == QUEUE_FULL);
        REQUIRE(lastCount == 10);
      }
    }
    
    WHEN("Queue transitions from FULL back to NORMAL") {
      // Fill queue
      for (int i = 0; i < 10; i++) {
        queue.enqueue(PRIORITY_NORMAL, "msg");
      }
      
      // Remove enough to go below 75%
      auto messages = queue.getMessages();
      for (int i = 0; i < 5; i++) {
        queue.remove(messages[i].id);
      }
      
      THEN("Callback should be fired with NORMAL state") {
        REQUIRE(lastState == QUEUE_NORMAL);
        // After removing 4 messages: 6 left, 6 < 7, so NORMAL state triggers
        // Callback is called with count=6 (the size when it transitioned)
        REQUIRE(lastCount == 6);
      }
    }
    
    WHEN("Queue is cleared") {
      queue.enqueue(PRIORITY_NORMAL, "msg1");
      queue.enqueue(PRIORITY_NORMAL, "msg2");
      queue.clear();
      
      THEN("Callback should be fired with EMPTY state") {
        REQUIRE(lastState == QUEUE_EMPTY);
        REQUIRE(lastCount == 0);
      }
    }
  }
}

SCENARIO("MessageQueue pruning old messages works correctly") {
  GIVEN("A queue with messages of different ages") {
    MessageQueue queue(10);
    
    // We can't easily test actual time delays in unit tests,
    // but we can verify the pruning logic works
    WHEN("Pruning with a very large maxAge") {
      queue.enqueue(PRIORITY_NORMAL, "msg1");
      queue.enqueue(PRIORITY_NORMAL, "msg2");
      queue.enqueue(PRIORITY_NORMAL, "msg3");
      
      // Prune messages older than 1 hour (none should be removed immediately)
      uint32_t removed = queue.pruneOldMessages(3600000);
      
      THEN("No messages should be removed") {
        REQUIRE(removed == 0);
        REQUIRE(queue.size() == 3);
      }
    }
    
    WHEN("Pruning with a very small maxAge") {
      queue.enqueue(PRIORITY_NORMAL, "msg1");
      
      // Wait enough time to ensure message is definitely older than threshold
      // delay() in test env uses usleep, so delay(2000) = 2ms
      delay(2000);
      
      // Prune messages older than 1ms
      uint32_t removed = queue.pruneOldMessages(1);
      
      THEN("All messages should be removed") {
        REQUIRE(removed == 1);
        REQUIRE(queue.empty());
      }
    }
  }
}

SCENARIO("MessageQueue handles edge cases correctly") {
  GIVEN("A queue with capacity 1") {
    MessageQueue queue(1);
    
    WHEN("Attempting to remove non-existent message") {
      bool removed = queue.remove(99999);
      
      THEN("Should return false") {
        REQUIRE(!removed);
      }
    }
    
    WHEN("Getting messages from empty queue") {
      auto messages = queue.getMessages();
      
      THEN("Should return empty vector") {
        REQUIRE(messages.empty());
      }
    }
    
    WHEN("Getting size by priority from empty queue") {
      uint32_t count = queue.size(PRIORITY_CRITICAL);
      
      THEN("Should return 0") {
        REQUIRE(count == 0);
      }
    }
  }
}
