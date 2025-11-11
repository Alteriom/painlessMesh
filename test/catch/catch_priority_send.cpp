#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"
#undef ARDUINOJSON_ENABLE_ARDUINO_STRING
#undef PAINLESSMESH_ENABLE_ARDUINO_STRING
#define PAINLESSMESH_ENABLE_STD_STRING
typedef std::string TSTRING;

#include "catch_utils.hpp"

#include "painlessmesh/buffer.hpp"
#include "painlessmesh/message_queue.hpp"

using namespace painlessmesh::buffer;
using namespace painlessmesh;

SCENARIO("SentBuffer handles priority-based message queuing") {
  GIVEN("A SentBuffer with mixed priority messages") {
    SentBuffer<std::string> buffer;
    
    WHEN("Messages are added with different priorities") {
      buffer.pushWithPriority("low_priority_msg", 3);      // LOW
      buffer.pushWithPriority("normal_priority_msg", 2);   // NORMAL
      buffer.pushWithPriority("high_priority_msg", 1);     // HIGH
      buffer.pushWithPriority("critical_msg", 0);          // CRITICAL
      
      THEN("Buffer contains all messages") {
        REQUIRE(buffer.size() == 4);
        REQUIRE(!buffer.empty());
      }
      
      THEN("Messages are retrieved in priority order") {
        temp_buffer_t tmp_buffer;
        
        // First message should be CRITICAL (priority 0)
        auto len1 = buffer.requestLength(tmp_buffer.length);
        REQUIRE(len1 > 0);
        auto ptr1 = buffer.readPtr(len1);
        REQUIRE(std::string(ptr1) == "critical_msg");
        REQUIRE(buffer.getLastReadPriority() == 0);
        buffer.freeRead();
        
        // Second message should be HIGH (priority 1)
        auto len2 = buffer.requestLength(tmp_buffer.length);
        auto ptr2 = buffer.readPtr(len2);
        REQUIRE(std::string(ptr2) == "high_priority_msg");
        REQUIRE(buffer.getLastReadPriority() == 1);
        buffer.freeRead();
        
        // Third message should be NORMAL (priority 2)
        auto len3 = buffer.requestLength(tmp_buffer.length);
        auto ptr3 = buffer.readPtr(len3);
        REQUIRE(std::string(ptr3) == "normal_priority_msg");
        REQUIRE(buffer.getLastReadPriority() == 2);
        buffer.freeRead();
        
        // Fourth message should be LOW (priority 3)
        auto len4 = buffer.requestLength(tmp_buffer.length);
        auto ptr4 = buffer.readPtr(len4);
        REQUIRE(std::string(ptr4) == "low_priority_msg");
        REQUIRE(buffer.getLastReadPriority() == 3);
        buffer.freeRead();
        
        REQUIRE(buffer.empty());
      }
    }
    
    WHEN("Multiple messages of same priority are added") {
      buffer.pushWithPriority("critical1", 0);
      buffer.pushWithPriority("critical2", 0);
      buffer.pushWithPriority("normal1", 2);
      buffer.pushWithPriority("critical3", 0);
      
      THEN("Same priority messages maintain FIFO order") {
        temp_buffer_t tmp_buffer;
        
        // All critical messages should come first
        auto ptr1 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr1) == "critical1");
        buffer.freeRead();
        
        auto ptr2 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr2) == "critical2");
        buffer.freeRead();
        
        auto ptr3 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr3) == "critical3");
        buffer.freeRead();
        
        auto ptr4 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr4) == "normal1");
        buffer.freeRead();
      }
    }
  }
  
  GIVEN("A SentBuffer using legacy bool priority API") {
    SentBuffer<std::string> buffer;
    
    WHEN("Messages are added with legacy bool priority") {
      buffer.push("normal1", false);  // NORMAL (priority 2)
      buffer.push("high1", true);     // HIGH (priority 1)
      buffer.push("normal2", false);  // NORMAL (priority 2)
      buffer.push("high2", true);     // HIGH (priority 1)
      
      THEN("High priority messages are sent before normal") {
        temp_buffer_t tmp_buffer;
        
        auto ptr1 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr1) == "high1");
        REQUIRE(buffer.getLastReadPriority() == 1);  // HIGH
        buffer.freeRead();
        
        auto ptr2 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr2) == "high2");
        REQUIRE(buffer.getLastReadPriority() == 1);  // HIGH
        buffer.freeRead();
        
        auto ptr3 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr3) == "normal1");
        REQUIRE(buffer.getLastReadPriority() == 2);  // NORMAL
        buffer.freeRead();
        
        auto ptr4 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr4) == "normal2");
        REQUIRE(buffer.getLastReadPriority() == 2);  // NORMAL
        buffer.freeRead();
      }
    }
  }
  
  GIVEN("A SentBuffer tracking statistics") {
    SentBuffer<std::string> buffer;
    
    WHEN("Various priority messages are queued and sent") {
      // Queue messages
      buffer.pushWithPriority("critical1", 0);
      buffer.pushWithPriority("critical2", 0);
      buffer.pushWithPriority("high1", 1);
      buffer.pushWithPriority("normal1", 2);
      buffer.pushWithPriority("normal2", 2);
      buffer.pushWithPriority("low1", 3);
      
      THEN("Statistics reflect queued messages") {
        auto stats = buffer.getStats();
        REQUIRE(stats.totalQueued == 6);
        REQUIRE(stats.criticalQueued == 2);
        REQUIRE(stats.highQueued == 1);
        REQUIRE(stats.normalQueued == 2);
        REQUIRE(stats.lowQueued == 1);
        REQUIRE(stats.criticalSent == 0);
      }
      
      WHEN("Messages are sent") {
        temp_buffer_t tmp_buffer;
        
        // Send first 3 messages
        buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        buffer.freeRead();
        buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        buffer.freeRead();
        buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        buffer.freeRead();
        
        THEN("Statistics track sent messages") {
          auto stats = buffer.getStats();
          REQUIRE(stats.criticalSent == 2);
          REQUIRE(stats.highSent == 1);
          REQUIRE(stats.normalSent == 0);
          REQUIRE(stats.lowSent == 0);
        }
      }
    }
  }
  
  GIVEN("A SentBuffer with boundary conditions") {
    SentBuffer<std::string> buffer;
    
    WHEN("Priority value is clamped") {
      buffer.pushWithPriority("msg1", 10);  // Should clamp to 3 (LOW)
      buffer.pushWithPriority("msg2", 255); // Should clamp to 3 (LOW)
      
      THEN("Priority is clamped to valid range") {
        temp_buffer_t tmp_buffer;
        buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(buffer.getLastReadPriority() == 3);  // Clamped to LOW
      }
    }
    
    WHEN("Buffer is cleared") {
      buffer.pushWithPriority("msg1", 0);
      buffer.pushWithPriority("msg2", 1);
      buffer.clear();
      
      THEN("Buffer is empty and statistics reset") {
        REQUIRE(buffer.empty());
        REQUIRE(buffer.size() == 0);
        auto stats = buffer.getStats();
        REQUIRE(stats.totalQueued == 0);
        REQUIRE(stats.criticalQueued == 0);
      }
    }
  }
}

SCENARIO("Priority levels match MessageQueue enum values") {
  GIVEN("MessagePriority enum values") {
    WHEN("Comparing with buffer priority levels") {
      THEN("Values are consistent") {
        REQUIRE(PRIORITY_CRITICAL == 0);
        REQUIRE(PRIORITY_HIGH == 1);
        REQUIRE(PRIORITY_NORMAL == 2);
        REQUIRE(PRIORITY_LOW == 3);
      }
    }
  }
}

SCENARIO("SentBuffer handles empty buffer gracefully") {
  GIVEN("An empty SentBuffer") {
    SentBuffer<std::string> buffer;
    
    WHEN("Attempting to read from empty buffer") {
      temp_buffer_t tmp_buffer;
      auto len = buffer.requestLength(tmp_buffer.length);
      
      THEN("Returns 0 length") {
        REQUIRE(len == 0);
        REQUIRE(buffer.empty());
      }
    }
    
    WHEN("Calling freeRead on empty buffer") {
      buffer.freeRead();
      
      THEN("No crash or error occurs") {
        REQUIRE(buffer.empty());
      }
    }
  }
}

SCENARIO("SentBuffer handles interleaved operations") {
  GIVEN("A SentBuffer with messages") {
    SentBuffer<std::string> buffer;
    
    WHEN("Messages are added, read, added again") {
      buffer.pushWithPriority("msg1", 0);  // CRITICAL
      buffer.pushWithPriority("msg2", 2);  // NORMAL
      
      temp_buffer_t tmp_buffer;
      
      // Read first message
      auto ptr1 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
      REQUIRE(std::string(ptr1) == "msg1");
      buffer.freeRead();
      
      // Add more messages
      buffer.pushWithPriority("msg3", 0);  // CRITICAL
      buffer.pushWithPriority("msg4", 1);  // HIGH
      
      THEN("Priority ordering is maintained") {
        // Should get msg3 (CRITICAL) next
        auto ptr2 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr2) == "msg3");
        REQUIRE(buffer.getLastReadPriority() == 0);
        buffer.freeRead();
        
        // Then msg4 (HIGH)
        auto ptr3 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr3) == "msg4");
        REQUIRE(buffer.getLastReadPriority() == 1);
        buffer.freeRead();
        
        // Finally msg2 (NORMAL)
        auto ptr4 = buffer.readPtr(buffer.requestLength(tmp_buffer.length));
        REQUIRE(std::string(ptr4) == "msg2");
        REQUIRE(buffer.getLastReadPriority() == 2);
        buffer.freeRead();
        
        REQUIRE(buffer.empty());
      }
    }
  }
}
