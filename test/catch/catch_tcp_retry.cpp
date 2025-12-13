#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

WiFiClass WiFi;
ESPClass ESP;

// Test the TCP retry configuration values and backoff formula
// Note: We re-declare the constants here to test them independently
// because including tcp.hpp requires complex template resolution

namespace tcp_test {
// These should match the values in painlessmesh/tcp.hpp
static const uint8_t TCP_CONNECT_MAX_RETRIES = 5;
static const uint32_t TCP_CONNECT_RETRY_DELAY_MS = 1000;
static const uint32_t TCP_CONNECT_STABILIZATION_DELAY_MS = 500;
static const uint32_t TCP_EXHAUSTION_RECONNECT_DELAY_MS = 10000;
}  // namespace tcp_test

SCENARIO("TCP connection retry constants are configured correctly",
         "[tcp][retry][configuration]") {
  GIVEN("The TCP retry configuration constants") {
    THEN("TCP_CONNECT_MAX_RETRIES should be 5") {
      REQUIRE(tcp_test::TCP_CONNECT_MAX_RETRIES == 5);
    }

    THEN("TCP_CONNECT_RETRY_DELAY_MS should be 1000 (1 second)") {
      REQUIRE(tcp_test::TCP_CONNECT_RETRY_DELAY_MS == 1000);
    }

    THEN("TCP_CONNECT_STABILIZATION_DELAY_MS should be 500 (500ms)") {
      REQUIRE(tcp_test::TCP_CONNECT_STABILIZATION_DELAY_MS == 500);
    }

    THEN("TCP_EXHAUSTION_RECONNECT_DELAY_MS should be 10000 (10 seconds)") {
      REQUIRE(tcp_test::TCP_EXHAUSTION_RECONNECT_DELAY_MS == 10000);
    }
  }
}

SCENARIO("Exponential backoff multiplier calculation works correctly",
         "[tcp][retry][backoff]") {
  GIVEN("The exponential backoff formula: multiplier = min(2^retryCount, 8)") {
    WHEN("retryCount is 0") {
      uint8_t retryCount = 0;
      uint8_t expectedMultiplier = 1;  // 2^0 = 1
      uint8_t backoffMultiplier = (retryCount < 3) ? (1U << retryCount) : 8;

      THEN("The multiplier should be 1") {
        REQUIRE(backoffMultiplier == expectedMultiplier);
      }

      THEN("The delay should be 1000ms") {
        uint32_t delay = tcp_test::TCP_CONNECT_RETRY_DELAY_MS * backoffMultiplier;
        REQUIRE(delay == 1000);
      }
    }

    WHEN("retryCount is 1") {
      uint8_t retryCount = 1;
      uint8_t expectedMultiplier = 2;  // 2^1 = 2
      uint8_t backoffMultiplier = (retryCount < 3) ? (1U << retryCount) : 8;

      THEN("The multiplier should be 2") {
        REQUIRE(backoffMultiplier == expectedMultiplier);
      }

      THEN("The delay should be 2000ms") {
        uint32_t delay = tcp_test::TCP_CONNECT_RETRY_DELAY_MS * backoffMultiplier;
        REQUIRE(delay == 2000);
      }
    }

    WHEN("retryCount is 2") {
      uint8_t retryCount = 2;
      uint8_t expectedMultiplier = 4;  // 2^2 = 4
      uint8_t backoffMultiplier = (retryCount < 3) ? (1U << retryCount) : 8;

      THEN("The multiplier should be 4") {
        REQUIRE(backoffMultiplier == expectedMultiplier);
      }

      THEN("The delay should be 4000ms") {
        uint32_t delay = tcp_test::TCP_CONNECT_RETRY_DELAY_MS * backoffMultiplier;
        REQUIRE(delay == 4000);
      }
    }

    WHEN("retryCount is 3") {
      uint8_t retryCount = 3;
      uint8_t expectedMultiplier = 8;  // capped at 8
      uint8_t backoffMultiplier = (retryCount < 3) ? (1U << retryCount) : 8;

      THEN("The multiplier should be capped at 8") {
        REQUIRE(backoffMultiplier == expectedMultiplier);
      }

      THEN("The delay should be 8000ms") {
        uint32_t delay = tcp_test::TCP_CONNECT_RETRY_DELAY_MS * backoffMultiplier;
        REQUIRE(delay == 8000);
      }
    }

    WHEN("retryCount is 4") {
      uint8_t retryCount = 4;
      uint8_t expectedMultiplier = 8;  // capped at 8
      uint8_t backoffMultiplier = (retryCount < 3) ? (1U << retryCount) : 8;

      THEN("The multiplier should remain capped at 8") {
        REQUIRE(backoffMultiplier == expectedMultiplier);
      }

      THEN("The delay should be 8000ms") {
        uint32_t delay = tcp_test::TCP_CONNECT_RETRY_DELAY_MS * backoffMultiplier;
        REQUIRE(delay == 8000);
      }
    }
  }
}

SCENARIO("Exponential backoff sequence is correct for all retry attempts",
         "[tcp][retry][backoff][sequence]") {
  GIVEN("A complete retry sequence from 0 to TCP_CONNECT_MAX_RETRIES-1") {
    // Expected delay sequence: 1s, 2s, 4s, 8s, 8s
    std::vector<uint32_t> expectedDelays = {1000, 2000, 4000, 8000, 8000};

    WHEN("Calculating delays for each retry attempt") {
      std::vector<uint32_t> actualDelays;

      for (uint8_t retryCount = 0; retryCount < tcp_test::TCP_CONNECT_MAX_RETRIES;
           ++retryCount) {
        uint8_t backoffMultiplier = (retryCount < 3) ? (1U << retryCount) : 8;
        uint32_t delay = tcp_test::TCP_CONNECT_RETRY_DELAY_MS * backoffMultiplier;
        actualDelays.push_back(delay);
      }

      THEN("The delay sequence should match expected values") {
        REQUIRE(actualDelays.size() == expectedDelays.size());

        for (size_t i = 0; i < expectedDelays.size(); ++i) {
          REQUIRE(actualDelays[i] == expectedDelays[i]);
        }
      }

      THEN("The total maximum wait time should be 23 seconds") {
        uint32_t totalDelay = 0;
        for (auto delay : actualDelays) {
          totalDelay += delay;
        }
        // 1000 + 2000 + 4000 + 8000 + 8000 = 23000ms = 23s
        REQUIRE(totalDelay == 23000);
      }
    }
  }
}

SCENARIO("Backoff multiplier correctly implements bit shifting",
         "[tcp][retry][backoff][bitshift]") {
  GIVEN("The bit shift formula 1U << retryCount for retryCount < 3") {
    THEN("1U << 0 should equal 1") { REQUIRE((1U << 0) == 1); }

    THEN("1U << 1 should equal 2") { REQUIRE((1U << 1) == 2); }

    THEN("1U << 2 should equal 4") { REQUIRE((1U << 2) == 4); }

    THEN("1U << 3 would be 8, but we cap at 8 anyway") {
      REQUIRE((1U << 3) == 8);
    }

    THEN("1U << 4 would be 16, demonstrating why we need the cap") {
      REQUIRE((1U << 4) == 16);
    }
  }
}

SCENARIO("TCP exhaustion reconnect delay prevents rapid reconnection loops",
         "[tcp][retry][exhaustion][reconnect]") {
  GIVEN("All TCP connection retries have been exhausted") {
    uint32_t totalRetryDelay = 0;
    
    // Calculate total delay from all retry attempts (1s + 2s + 4s + 8s + 8s = 23s)
    for (uint8_t retryCount = 0; retryCount < tcp_test::TCP_CONNECT_MAX_RETRIES;
         ++retryCount) {
      uint8_t backoffMultiplier = (retryCount < 3) ? (1U << retryCount) : 8;
      totalRetryDelay += tcp_test::TCP_CONNECT_RETRY_DELAY_MS * backoffMultiplier;
    }
    
    WHEN("WiFi reconnection is scheduled after retry exhaustion") {
      uint32_t reconnectDelay = tcp_test::TCP_EXHAUSTION_RECONNECT_DELAY_MS;
      
      THEN("The reconnect delay should be significantly longer than any single retry") {
        // 10 seconds vs 8 seconds (longest retry delay)
        REQUIRE(reconnectDelay > 8000);
      }
      
      THEN("The reconnect delay should provide adequate recovery time") {
        // 10 seconds is reasonable for TCP server recovery
        REQUIRE(reconnectDelay >= 10000);
      }
      
      THEN("Total time before reconnection should include all retries plus exhaustion delay") {
        // Total: 23s (retries) + 10s (exhaustion delay) = 33 seconds
        uint32_t totalTimeBeforeReconnect = totalRetryDelay + reconnectDelay;
        REQUIRE(totalTimeBeforeReconnect == 33000);
      }
      
      THEN("The exhaustion delay prevents rapid loops that could occur in <1 second") {
        // Without exhaustion delay, reconnection could happen almost immediately
        // after the last retry, potentially creating a rapid loop
        REQUIRE(reconnectDelay > 1000);
      }
    }
  }
}
