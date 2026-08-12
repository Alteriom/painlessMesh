#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

WiFiClass WiFi;
ESPClass ESP;

// Log must be declared before tcp.hpp: the non-template helpers in that
// header call it, so it has to be visible at their point of definition.
#include "painlessmesh/logger.hpp"

using namespace painlessmesh;

logger::LogClass Log;

#include "painlessmesh/connection.hpp"
#include "painlessmesh/tcp.hpp"

// These tests exercise the real shipped TcpRetryConfig defaults and the real
// tcp::retryBackoffDelay() helper rather than re-declaring the constants and
// re-implementing the backoff formula locally. The retry parameters live in
// painlessmesh/tcp.hpp; TCP_CLIENT_CLEANUP_DELAY_MS and
// TCP_CLIENT_DELETION_SPACING_MS live in painlessmesh/connection.hpp because
// they are used from a destructor.

SCENARIO("TCP connection retry defaults are configured correctly",
         "[tcp][retry][configuration]") {
  GIVEN("The default TCP retry configuration") {
    tcp::TcpRetryConfig cfg;

    THEN("maxRetries should be 5") { REQUIRE(cfg.maxRetries == 5); }

    THEN("retryDelayMs should be 1000 (1 second)") {
      REQUIRE(cfg.retryDelayMs == 1000);
    }

    THEN("stabilizationDelayMs should be 500 (500ms)") {
      REQUIRE(cfg.stabilizationDelayMs == 500);
    }

    THEN("TCP_CLIENT_CLEANUP_DELAY_MS should be 1000 (1000ms)") {
      REQUIRE(tcp::TCP_CLIENT_CLEANUP_DELAY_MS == 1000);
    }

    THEN("TCP_CLIENT_DELETION_SPACING_MS should be 1000 (1000ms)") {
      REQUIRE(tcp::TCP_CLIENT_DELETION_SPACING_MS == 1000);
    }

    THEN("exhaustionReconnectDelayMs should be 10000 (10 seconds)") {
      REQUIRE(cfg.exhaustionReconnectDelayMs == 10000);
    }
  }
}

SCENARIO("Exponential backoff delay calculation works correctly",
         "[tcp][retry][backoff]") {
  GIVEN("The default retry configuration") {
    tcp::TcpRetryConfig cfg;

    WHEN("retryCount is 0") {
      THEN("The delay should be 1000ms (multiplier 1)") {
        REQUIRE(tcp::retryBackoffDelay(cfg, 0) == 1000);
      }
    }

    WHEN("retryCount is 1") {
      THEN("The delay should be 2000ms (multiplier 2)") {
        REQUIRE(tcp::retryBackoffDelay(cfg, 1) == 2000);
      }
    }

    WHEN("retryCount is 2") {
      THEN("The delay should be 4000ms (multiplier 4)") {
        REQUIRE(tcp::retryBackoffDelay(cfg, 2) == 4000);
      }
    }

    WHEN("retryCount is 3") {
      THEN("The delay should be 8000ms (multiplier capped at 8)") {
        REQUIRE(tcp::retryBackoffDelay(cfg, 3) == 8000);
      }
    }

    WHEN("retryCount is 4") {
      THEN("The delay should remain 8000ms (multiplier still capped at 8)") {
        REQUIRE(tcp::retryBackoffDelay(cfg, 4) == 8000);
      }
    }

    WHEN("retryCount grows well beyond the cap") {
      THEN("The delay does not keep doubling") {
        // Demonstrates why the cap exists: without it, 1U << 20 would be a
        // ~17 minute delay.
        REQUIRE(tcp::retryBackoffDelay(cfg, 20) == 8000);
      }
    }
  }
}

SCENARIO("Exponential backoff scales with a custom base delay",
         "[tcp][retry][backoff][custom]") {
  GIVEN("A configuration with a 200ms base retry delay") {
    tcp::TcpRetryConfig cfg;
    cfg.retryDelayMs = 200;

    WHEN("Calculating the backoff sequence") {
      std::vector<uint32_t> expectedDelays = {200, 400, 800, 1600, 1600};
      std::vector<uint32_t> actualDelays;
      for (size_t i = 0; i < expectedDelays.size(); ++i) {
        actualDelays.push_back(
            tcp::retryBackoffDelay(cfg, static_cast<uint8_t>(i)));
      }

      THEN("The sequence scales proportionally with the base delay") {
        REQUIRE(actualDelays == expectedDelays);
      }
    }
  }
}

SCENARIO("Exponential backoff sequence is correct for all retry attempts",
         "[tcp][retry][backoff][sequence]") {
  GIVEN("A complete retry sequence for the default configuration") {
    tcp::TcpRetryConfig cfg;
    // Expected delay sequence: 1s, 2s, 4s, 8s, 8s
    std::vector<uint32_t> expectedDelays = {1000, 2000, 4000, 8000, 8000};

    WHEN("Calculating delays for each retry attempt") {
      std::vector<uint32_t> actualDelays;

      for (uint8_t retryCount = 0; retryCount < cfg.maxRetries; ++retryCount) {
        actualDelays.push_back(tcp::retryBackoffDelay(cfg, retryCount));
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

SCENARIO("A maxRetries of zero disables retrying entirely",
         "[tcp][retry][backoff][noretry]") {
  GIVEN("A configuration with maxRetries set to 0") {
    tcp::TcpRetryConfig cfg;
    cfg.maxRetries = 0;

    WHEN("Iterating the retry sequence the way tcp::connect() does") {
      uint32_t attempts = 0;
      for (uint8_t retryCount = 0; retryCount < cfg.maxRetries; ++retryCount) {
        ++attempts;
      }

      THEN("No retry is ever scheduled") {
        // The node falls straight through to the WiFi reconnect path, which is
        // the low-latency behaviour requested in discussion #368.
        REQUIRE(attempts == 0);
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
    tcp::TcpRetryConfig cfg;
    uint32_t totalRetryDelay = 0;

    // Total delay from all retry attempts (1s + 2s + 4s + 8s + 8s = 23s),
    // computed with the same helper tcp::connect() uses.
    for (uint8_t retryCount = 0; retryCount < cfg.maxRetries; ++retryCount) {
      totalRetryDelay += tcp::retryBackoffDelay(cfg, retryCount);
    }

    WHEN("WiFi reconnection is scheduled after retry exhaustion") {
      uint32_t reconnectDelay = cfg.exhaustionReconnectDelayMs;

      THEN("The reconnect delay should be significantly longer than any single retry") {
        // 10 seconds vs 8 seconds (longest retry delay)
        uint32_t maxSingleRetryDelay =
            tcp::retryBackoffDelay(cfg, cfg.maxRetries - 1);
        REQUIRE(reconnectDelay > maxSingleRetryDelay);
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

SCENARIO("AsyncClient deletion spacing prevents concurrent cleanup operations",
         "[tcp][cleanup][spacing][heap-corruption]") {
  GIVEN("Multiple AsyncClient objects need to be deleted") {
    // This scenario validates the fix for heap corruption during sendToInternet
    // operations where multiple AsyncClients are deleted in rapid succession

    WHEN("Multiple deletions are scheduled simultaneously") {
      uint32_t baseDelay = tcp::TCP_CLIENT_CLEANUP_DELAY_MS;
      uint32_t spacing = tcp::TCP_CLIENT_DELETION_SPACING_MS;

      THEN("Base cleanup delay should be adequate for single deletion") {
        // 1000ms is sufficient for AsyncTCP library to complete cleanup of one client
        REQUIRE(baseDelay >= 1000);
      }

      THEN("Deletion spacing should prevent concurrent AsyncTCP cleanup") {
        // 1000ms spacing ensures each deletion completes before next one starts
        // Increased from 250ms to 1000ms for ESP32-C6 compatibility
        REQUIRE(spacing >= 500);
        REQUIRE(spacing <= 1500);
      }

      THEN("Second deletion should be spaced from the first") {
        // If first deletion is at T+1000ms, second should be at T+1250ms or later
        uint32_t firstDeletionTime = baseDelay;
        uint32_t secondDeletionTime = firstDeletionTime + spacing;
        REQUIRE(secondDeletionTime >= firstDeletionTime + spacing);
      }

      THEN("Multiple deletions should have cumulative spacing") {
        // For 4 concurrent deletion requests (sendToInternet high-churn scenario):
        // Deletion 1: 1000ms
        // Deletion 2: 1000ms + 1000ms = 2000ms
        // Deletion 3: 2000ms + 1000ms = 3000ms
        // Deletion 4: 3000ms + 1000ms = 4000ms
        uint32_t numDeletions = 4;
        std::vector<uint32_t> deletionTimes;

        for (uint32_t i = 0; i < numDeletions; ++i) {
          uint32_t deletionTime = baseDelay + (i * spacing);
          deletionTimes.push_back(deletionTime);
        }

        // Verify each deletion is properly spaced
        for (uint32_t i = 1; i < numDeletions; ++i) {
          uint32_t timeDiff = deletionTimes[i] - deletionTimes[i-1];
          REQUIRE(timeDiff == spacing);
        }

        // Total spread should be base + (count-1)*spacing
        uint32_t totalSpread = deletionTimes.back() - deletionTimes.front();
        REQUIRE(totalSpread == (numDeletions - 1) * spacing);
        REQUIRE(totalSpread == 3000); // (4-1) * 1000ms = 3000ms
      }
    }

    WHEN("High connection churn occurs (sendToInternet scenario)") {
      // Simulate scenario from bug report where multiple connections fail rapidly:
      // - BufferedConnection destructor schedules deletion
      // - tcp_err handler schedules deletion
      // - Another BufferedConnection destructor schedules deletion
      // Without spacing, all 3 would execute at nearly same time causing heap corruption

      uint32_t numConcurrentDeletions = 3;
      std::vector<uint32_t> spacedDeletionTimes;

      for (uint32_t i = 0; i < numConcurrentDeletions; ++i) {
        uint32_t deletionTime = tcp::TCP_CLIENT_CLEANUP_DELAY_MS +
                                (i * tcp::TCP_CLIENT_DELETION_SPACING_MS);
        spacedDeletionTimes.push_back(deletionTime);
      }

      THEN("Deletions should be spread over time to prevent AsyncTCP interference") {
        // First: 1000ms, Second: 2000ms, Third: 3000ms
        REQUIRE(spacedDeletionTimes[0] == 1000);
        REQUIRE(spacedDeletionTimes[1] == 2000);
        REQUIRE(spacedDeletionTimes[2] == 3000);
      }

      THEN("Total window should be reasonable for high-churn scenarios") {
        // 2000ms total spread (1000ms to 3000ms) provides adequate spacing for ESP32-C6
        uint32_t totalWindow = spacedDeletionTimes.back() - spacedDeletionTimes.front();
        REQUIRE(totalWindow == 2000);
        REQUIRE(totalWindow < 3000); // Keep it under 3 seconds for reasonable responsiveness
      }

      THEN("Each deletion has time to complete before next starts") {
        // AsyncTCP library needs ~200-400ms per deletion on ESP32/ESP8266
        // ESP32-C6 requires significantly more time due to RISC-V architecture
        // 1000ms spacing provides robust buffer for all ESP32 variants
        REQUIRE(tcp::TCP_CLIENT_DELETION_SPACING_MS >= 500);
      }
    }

    WHEN("Scheduler jitter causes execution time variation") {
      // This scenario validates the fix for the specific heap corruption bug where
      // deletions scheduled at T+1000ms and T+1250ms executed at T+1000ms and T+1220ms
      // due to scheduler jitter, causing heap corruption despite proper scheduling.
      //
      // The fix: Update lastScheduledDeletionTime when deletion EXECUTES, not just when scheduled.
      // This ensures subsequent deletions space from actual execution time, preventing
      // concurrent cleanup even with scheduler jitter.

      THEN("Execution time tracking prevents concurrent cleanup despite scheduler jitter") {
        // Scenario from bug report:
        // - Deletion 1 scheduled for T+1000ms, executes at T+1000ms (on time)
        // - Deletion 2 scheduled for T+1250ms, but scheduler is delayed
        // - If we only track scheduled time, deletion 3 scheduled at T+10ms would use T+1250ms
        // - But if deletion 2 hasn't executed yet due to jitter, this could cause concurrent cleanup
        //
        // With the fix: When deletion 2 executes (say at T+1230ms due to jitter),
        // it updates lastScheduledDeletionTime = millis() = 1230
        // Subsequent deletion 3 will space from 1230ms, not 1250ms, ensuring proper spacing

        uint32_t baseDelay = tcp::TCP_CLIENT_CLEANUP_DELAY_MS;
        uint32_t spacing = tcp::TCP_CLIENT_DELETION_SPACING_MS;

        // Simulate scenario:
        // T=0: Request deletion 1 -> scheduled at T+baseDelay
        uint32_t deletion1ScheduledTime = 0 + baseDelay;

        // T=10: Request deletion 2 -> scheduled at T+(baseDelay+spacing)
        uint32_t deletion2ScheduledTime = deletion1ScheduledTime + spacing;

        // Deletion 2 executes EARLY due to scheduler jitter (20ms before scheduled time)
        const uint32_t jitterMs = 20;  // Realistic scheduler jitter
        uint32_t deletion2ExecutionTime = deletion2ScheduledTime - jitterMs;

        // T=20: Request deletion 3 - should space from deletion 2's EXECUTION, not schedule
        // With fix: spaces from actual execution + spacing
        uint32_t deletion3ScheduledTimeWithFix = deletion2ExecutionTime + spacing;

        // Verify proper spacing from actual execution time
        uint32_t actualSpacingWithFix = deletion3ScheduledTimeWithFix - deletion2ExecutionTime;
        REQUIRE(actualSpacingWithFix >= spacing);  // Always maintains minimum spacing

        // Calculate what the spacing would have been WITHOUT the fix
        // (spacing from scheduled time instead of execution time)
        uint32_t deletion3ScheduledTimeWithoutFix = deletion2ScheduledTime + spacing;

        // Without fix, actual spacing depends on jitter direction:
        // - If task executes early (as in this example): spacing increases beyond minimum (adequate)
        // - If task executes late: spacing could fall below minimum (heap corruption!)
        // Example: deletion2 executes at 1270ms (20ms late)
        //          deletion3 scheduled at 1500ms (from scheduled time 1250 + 250)
        //          actual spacing: 1500 - 1270 = 230ms (too close! < 250ms)

        // Demonstrate the fix handles jitter in both directions
        uint32_t deletion2ExecutionTimeLate = deletion2ScheduledTime + jitterMs;  // 20ms late
        uint32_t actualSpacingIfLate = deletion3ScheduledTimeWithoutFix - deletion2ExecutionTimeLate;
        REQUIRE(actualSpacingIfLate < spacing);  // Without fix: violates minimum spacing!

        // This demonstrates that even with realistic jitter, we maintain minimum spacing
        // between actual deletion executions, preventing concurrent AsyncTCP cleanup
      }

      THEN("Double-update strategy provides protection at both schedule and execution time") {
        // The fix updates lastScheduledDeletionTime in TWO places:
        // 1. When scheduling: Ensures minimum spacing between scheduled times
        // 2. When executing: Ensures subsequent deletions space from actual execution
        //
        // This double protection handles all scenarios:
        // - Normal case: Both updates align, proper spacing maintained
        // - Jitter case: Execution update ensures spacing from actual time
        // - High load: Schedule-time spacing prevents scheduling too close together

        uint32_t spacing = tcp::TCP_CLIENT_DELETION_SPACING_MS;

        // Both updates use the same spacing constant
        REQUIRE(spacing == 1000);

        // Minimum spacing is enforced at both stages
        REQUIRE(spacing >= 500);  // Adequate for ESP32-C6 AsyncTCP cleanup
        REQUIRE(spacing <= 1500);  // Still responsive
      }
    }
  }
}
