#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"
#include "catch_utils.hpp"
#include "painlessmesh/metrics.hpp"

using namespace painlessmesh;
using namespace painlessmesh::metrics;

SCENARIO("Timer functionality works correctly") {
    GIVEN("A timer instance") {
        Timer timer;

        WHEN("Measuring elapsed time") {
            uint32_t start_time = timer.elapsed_ms();

            // Simulate some work (not real delay to avoid slowing tests)
            for (volatile int i = 0; i < 1000; i++) {
                // Busy wait
            }

            uint32_t end_time = timer.elapsed_ms();

            THEN("Elapsed time should be non-negative") {
                REQUIRE(end_time >= start_time);
            }
        }

        WHEN("Resetting the timer") {
            timer.reset();
            uint32_t elapsed = timer.elapsed_ms();

            THEN("Elapsed time should be close to zero") {
                REQUIRE(elapsed < 10); // Allow some margin for execution time
            }
        }
    }
}
