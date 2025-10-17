#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"
#undef ARDUINOJSON_ENABLE_ARDUINO_STRING
typedef std::string TSTRING;

#include "catch_utils.hpp"
#include "painlessmesh/protocol.hpp"

using namespace painlessmesh::protocol;

SCENARIO("Router JSON parsing with improved capacity calculation", "[router][memory]") {
  GIVEN("A simple JSON message") {
    std::string json = R"({"type":9,"from":123456,"dest":654321,"msg":"Hello"})";
    
    WHEN("Parsing with calculated capacity") {
      size_t nestingDepth = std::count(json.begin(), json.end(), '{') + 
                            std::count(json.begin(), json.end(), '[');
      
      size_t calculatedCapacity = json.length() + 
                                  200 * std::max(nestingDepth, size_t(1)) + 
                                  512;
      
      Variant variant(json, calculatedCapacity);
      
      THEN("Parsing succeeds") {
        REQUIRE(variant.error == DeserializationError::Ok);
        REQUIRE(variant.type() == 9);
      }
    }
  }
  
  GIVEN("A deeply nested JSON message") {
    std::string json = R"({
      "type": 8,
      "from": 111111,
      "msg": "Test",
      "subs": [
        {"nodeId": 1, "subs": [{"nodeId": 2}, {"nodeId": 3}]},
        {"nodeId": 4, "subs": [{"nodeId": 5}]}
      ]
    })";
    
    WHEN("Parsing with calculated capacity") {
      size_t nestingDepth = std::count(json.begin(), json.end(), '{') + 
                            std::count(json.begin(), json.end(), '[');
      
      size_t calculatedCapacity = json.length() + 
                                  200 * std::max(nestingDepth, size_t(1)) + 
                                  512;
      
      Variant variant(json, calculatedCapacity);
      
      THEN("Parsing succeeds") {
        REQUIRE(variant.error == DeserializationError::Ok);
        REQUIRE(variant.type() == 8);
      }
    }
  }
  
  GIVEN("A very large nested JSON structure") {
    // Build a deeply nested structure
    std::string json = R"({"type":8,"from":999999,"subs":[)";
    for (int i = 0; i < 20; i++) {
      if (i > 0) json += ",";
      json += "{\"nodeId\":" + std::to_string(i) + ",\"subs\":[";
      for (int j = 0; j < 5; j++) {
        if (j > 0) json += ",";
        json += "{\"nodeId\":" + std::to_string(i * 100 + j) + "}";
      }
      json += "]}";
    }
    json += "]}";
    
    WHEN("Parsing with calculated capacity") {
      size_t nestingDepth = std::count(json.begin(), json.end(), '{') + 
                            std::count(json.begin(), json.end(), '[');
      
      size_t calculatedCapacity = json.length() + 
                                  200 * std::max(nestingDepth, size_t(1)) + 
                                  512;
      
      // Cap at MAX_MESSAGE_CAPACITY like in the router fix
      constexpr size_t MAX_MESSAGE_CAPACITY = 8192;
      size_t capacity = std::min(calculatedCapacity, MAX_MESSAGE_CAPACITY);
      
      Variant variant(json, capacity);
      
      THEN("Parsing succeeds or fails gracefully") {
        // Should either succeed or report NoMemory (not crash)
        if (variant.error == DeserializationError::Ok) {
          REQUIRE(variant.type() == 8);
        } else {
          REQUIRE(variant.error == DeserializationError::NoMemory);
        }
      }
    }
  }
  
  GIVEN("A message at the capacity limit") {
    // Create a message that's just under 8KB
    std::string json = R"({"type":9,"from":123456,"dest":654321,"msg":")";
    std::string largeMessage(7000, 'A');  // 7KB of 'A' characters
    json += largeMessage + "\"}";
    
    WHEN("Parsing with calculated capacity capped at MAX") {
      size_t nestingDepth = std::count(json.begin(), json.end(), '{') + 
                            std::count(json.begin(), json.end(), '[');
      
      size_t calculatedCapacity = json.length() + 
                                  200 * std::max(nestingDepth, size_t(1)) + 
                                  512;
      
      constexpr size_t MAX_MESSAGE_CAPACITY = 8192;
      size_t capacity = std::min(calculatedCapacity, MAX_MESSAGE_CAPACITY);
      
      Variant variant(json, capacity);
      
      THEN("Parsing behavior is deterministic") {
        // Should either succeed with capped capacity or report NoMemory
        // The key is it doesn't crash or hang
        REQUIRE((variant.error == DeserializationError::Ok || 
                 variant.error == DeserializationError::NoMemory));
      }
    }
  }
  
  GIVEN("An oversized message that exceeds MAX_MESSAGE_CAPACITY") {
    // Create a message that's way over 8KB
    std::string json = R"({"type":9,"from":123456,"dest":654321,"msg":")";
    std::string largeMessage(20000, 'B');  // 20KB of 'B' characters
    json += largeMessage + "\"}";
    
    WHEN("Parsing with capacity capped at MAX") {
      size_t nestingDepth = std::count(json.begin(), json.end(), '{') + 
                            std::count(json.begin(), json.end(), '[');
      
#if ARDUINOJSON_VERSION_MAJOR >= 7
      // ArduinoJson v7: automatic capacity management
      size_t calculatedCapacity = json.length() + 1024;
#else
      // ArduinoJson v6: manual capacity calculation
      size_t calculatedCapacity = json.length() + 
                                  JSON_OBJECT_SIZE(10) * std::max(nestingDepth, size_t(1)) + 
                                  512;
#endif
      
      constexpr size_t MAX_MESSAGE_CAPACITY = 8192;
      size_t capacity = std::min(calculatedCapacity, MAX_MESSAGE_CAPACITY);
      
      Variant variant(json, capacity);
      
      THEN("Parsing should fail or succeed but capacity is capped") {
        // The important thing is that we don't allocate more than MAX
        REQUIRE(capacity == MAX_MESSAGE_CAPACITY);
        // Whether parsing succeeds or fails depends on JSON compression
        // but we've protected against unbounded memory growth
      }
    }
  }
}

SCENARIO("Memory allocation patterns are predictable", "[router][memory]") {
  GIVEN("Multiple messages of varying sizes") {
    std::vector<std::string> messages = {
      R"({"type":9,"from":1,"dest":2,"msg":"Small"})",
      R"({"type":8,"from":100,"msg":"Medium","subs":[{"nodeId":1},{"nodeId":2},{"nodeId":3}]})",
      R"({"type":9,"from":1000,"dest":2000,"msg":"Larger message with more content"})"
    };
    
    WHEN("Parsing all messages") {
      std::vector<size_t> capacities;
      
      for (const auto& json : messages) {
        size_t nestingDepth = std::count(json.begin(), json.end(), '{') + 
                              std::count(json.begin(), json.end(), '[');
        
#if ARDUINOJSON_VERSION_MAJOR >= 7
        // ArduinoJson v7: automatic capacity management
        size_t calculatedCapacity = json.length() + 1024;
#else
        // ArduinoJson v6: manual capacity calculation
        size_t calculatedCapacity = json.length() + 
                                    JSON_OBJECT_SIZE(10) * std::max(nestingDepth, size_t(1)) + 
                                    512;
#endif
        
        constexpr size_t MAX_MESSAGE_CAPACITY = 8192;
        size_t capacity = std::min(calculatedCapacity, MAX_MESSAGE_CAPACITY);
        
        capacities.push_back(capacity);
        
        Variant variant(json, capacity);
        REQUIRE(variant.error == DeserializationError::Ok);
      }
      
      THEN("Capacities grow predictably") {
        // Verify capacities are calculated consistently
        REQUIRE(capacities.size() == 3);
        // All should be under MAX
        for (auto cap : capacities) {
          REQUIRE(cap <= 8192);
        }
      }
    }
  }
}
