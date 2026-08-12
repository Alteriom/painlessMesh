/**
 * @file catch_tcp_retry_config.cpp
 * @brief Tests for user-configurable TCP retry parameters (issue #378)
 *
 * Covers:
 * - TcpRetryConfig defaults reproduce the legacy hardcoded constants
 * - setTcpRetryConfig() / getTcpRetryConfig() round-trip
 * - Clamping of the two values that can render a node unusable
 * - Per-instance storage (the config must NOT be a namespace-scope global)
 *
 * NOTE ON COVERAGE: the retry path inside tcp::connect() is compiled out on
 * desktop builds (it is guarded by
 * `#if !defined(PAINLESSMESH_BOOST) && (defined(ESP32) || defined(ESP8266))`),
 * so these tests verify the config plumbing and the backoff arithmetic, not
 * the runtime onError -> reschedule -> reconnect chain. Verifying that
 * requires hardware.
 */

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"

// Global logger for test environment. Declared before mesh.hpp (which pulls in
// tcp.hpp) because the non-template helpers there call Log, so it must be
// visible at their point of definition.
#include "painlessmesh/logger.hpp"

painlessmesh::logger::LogClass Log;

#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

SCENARIO("TcpRetryConfig defaults match the legacy hardcoded constants",
         "[tcp][retry][config][defaults]") {
  GIVEN("A default-constructed TcpRetryConfig") {
    tcp::TcpRetryConfig cfg;

    // This is the acceptance criterion "default values match current behavior
    // exactly". The struct spells its defaults as these constants, so this
    // guards against someone later replacing them with drifting literals.
    THEN("maxRetries matches TCP_CONNECT_MAX_RETRIES") {
      REQUIRE(cfg.maxRetries == tcp::TCP_CONNECT_MAX_RETRIES);
      REQUIRE(cfg.maxRetries == 5);
    }

    THEN("retryDelayMs matches TCP_CONNECT_RETRY_DELAY_MS") {
      REQUIRE(cfg.retryDelayMs == tcp::TCP_CONNECT_RETRY_DELAY_MS);
      REQUIRE(cfg.retryDelayMs == 1000);
    }

    THEN("stabilizationDelayMs matches TCP_CONNECT_STABILIZATION_DELAY_MS") {
      REQUIRE(cfg.stabilizationDelayMs ==
              tcp::TCP_CONNECT_STABILIZATION_DELAY_MS);
      REQUIRE(cfg.stabilizationDelayMs == 500);
    }

    THEN("exhaustionReconnectDelayMs matches TCP_EXHAUSTION_RECONNECT_DELAY_MS") {
      REQUIRE(cfg.exhaustionReconnectDelayMs ==
              tcp::TCP_EXHAUSTION_RECONNECT_DELAY_MS);
      REQUIRE(cfg.exhaustionReconnectDelayMs == 10000);
    }

    THEN("failureBlockDurationMs matches TCP_FAILURE_BLOCK_DURATION_MS") {
      REQUIRE(cfg.failureBlockDurationMs ==
              tcp::TCP_FAILURE_BLOCK_DURATION_MS);
      REQUIRE(cfg.failureBlockDurationMs == 60000);
    }
  }
}

SCENARIO("A fresh mesh reports default TCP retry configuration",
         "[tcp][retry][config][defaults]") {
  GIVEN("An initialised mesh that has never been configured") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    mesh.init(&scheduler, 1234567);

    THEN("getTcpRetryConfig returns the struct defaults") {
      auto cfg = mesh.getTcpRetryConfig();
      tcp::TcpRetryConfig defaults;

      REQUIRE(cfg.maxRetries == defaults.maxRetries);
      REQUIRE(cfg.retryDelayMs == defaults.retryDelayMs);
      REQUIRE(cfg.stabilizationDelayMs == defaults.stabilizationDelayMs);
      REQUIRE(cfg.exhaustionReconnectDelayMs ==
              defaults.exhaustionReconnectDelayMs);
      REQUIRE(cfg.failureBlockDurationMs == defaults.failureBlockDurationMs);
    }
  }
}

SCENARIO("setTcpRetryConfig round-trips an in-range configuration",
         "[tcp][retry][config][roundtrip]") {
  GIVEN("An initialised mesh") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    mesh.init(&scheduler, 1234567);

    WHEN("A custom in-range configuration is applied") {
      tcp::TcpRetryConfig custom;
      custom.maxRetries = 8;
      custom.retryDelayMs = 250;
      custom.stabilizationDelayMs = 100;
      custom.exhaustionReconnectDelayMs = 2000;
      custom.failureBlockDurationMs = 15000;
      mesh.setTcpRetryConfig(custom);

      THEN("getTcpRetryConfig returns it unchanged") {
        auto cfg = mesh.getTcpRetryConfig();
        REQUIRE(cfg.maxRetries == 8);
        REQUIRE(cfg.retryDelayMs == 250);
        REQUIRE(cfg.stabilizationDelayMs == 100);
        REQUIRE(cfg.exhaustionReconnectDelayMs == 2000);
        REQUIRE(cfg.failureBlockDurationMs == 15000);
      }
    }
  }
}

SCENARIO("TCP retry configuration is stored per mesh instance",
         "[tcp][retry][config][isolation]") {
  // Regression test for the design decision that the config lives on the Mesh
  // instance rather than at namespace scope in tcp.hpp. A namespace-scope
  // mutable instance in a header would also give each translation unit its own
  // silent copy. If someone later moves the storage, this fails loudly.
  GIVEN("Two independently initialised meshes") {
    Scheduler schedulerA;
    Scheduler schedulerB;
    Mesh<Connection> meshA;
    Mesh<Connection> meshB;
    meshA.init(&schedulerA, 1111111);
    meshB.init(&schedulerB, 2222222);

    WHEN("Only the first mesh is configured") {
      tcp::TcpRetryConfig custom;
      custom.maxRetries = 1;
      custom.retryDelayMs = 100;
      meshA.setTcpRetryConfig(custom);

      THEN("The first mesh reports the custom values") {
        REQUIRE(meshA.getTcpRetryConfig().maxRetries == 1);
        REQUIRE(meshA.getTcpRetryConfig().retryDelayMs == 100);
      }

      THEN("The second mesh still reports defaults") {
        tcp::TcpRetryConfig defaults;
        REQUIRE(meshB.getTcpRetryConfig().maxRetries == defaults.maxRetries);
        REQUIRE(meshB.getTcpRetryConfig().retryDelayMs ==
                defaults.retryDelayMs);
      }
    }
  }
}

SCENARIO("Out-of-range TCP retry values are clamped",
         "[tcp][retry][config][clamping]") {
  GIVEN("An initialised mesh") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    mesh.init(&scheduler, 1234567);

    WHEN("maxRetries exceeds the safe upper limit") {
      tcp::TcpRetryConfig custom;
      custom.maxRetries = 250;
      mesh.setTcpRetryConfig(custom);

      THEN("It is clamped to TCP_RETRY_MAX_RETRIES_LIMIT") {
        // Each retry allocates an AsyncClient and recurses, so this bound is
        // a heap and recursion-depth guard, not a style preference.
        REQUIRE(mesh.getTcpRetryConfig().maxRetries ==
                tcp::TCP_RETRY_MAX_RETRIES_LIMIT);
      }
    }

    WHEN("retryDelayMs is zero") {
      tcp::TcpRetryConfig custom;
      custom.retryDelayMs = 0;
      mesh.setTcpRetryConfig(custom);

      THEN("It is raised to TCP_RETRY_MIN_DELAY_MS") {
        // A zero delay would schedule retries with no spacing: a hot loop
        // allocating an AsyncClient per scheduler tick.
        REQUIRE(mesh.getTcpRetryConfig().retryDelayMs ==
                tcp::TCP_RETRY_MIN_DELAY_MS);
      }
    }

    WHEN("retryDelayMs is at the uint32_t maximum") {
      tcp::TcpRetryConfig custom;
      custom.retryDelayMs = 0xFFFFFFFF;
      mesh.setTcpRetryConfig(custom);
      auto cfg = mesh.getTcpRetryConfig();

      THEN("It is lowered to TCP_RETRY_MAX_DELAY_MS") {
        REQUIRE(cfg.retryDelayMs == tcp::TCP_RETRY_MAX_DELAY_MS);
      }

      THEN("The backoff multiplier cannot overflow the delay") {
        // retryBackoffDelay multiplies by up to 8; the ceiling keeps that
        // product two orders of magnitude clear of wrapping.
        uint32_t maxDelay = tcp::retryBackoffDelay(cfg, 4);
        REQUIRE(maxDelay > cfg.retryDelayMs);
        REQUIRE(maxDelay == cfg.retryDelayMs * 8);
      }
    }
  }
}

SCENARIO("Meaningful zero values survive the setter",
         "[tcp][retry][config][clamping][zero]") {
  GIVEN("An initialised mesh") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    mesh.init(&scheduler, 1234567);

    WHEN("maxRetries is set to zero") {
      tcp::TcpRetryConfig custom;
      custom.maxRetries = 0;
      mesh.setTcpRetryConfig(custom);

      THEN("It is preserved, not coerced to a minimum of one") {
        // "Fail over to WiFi reconnect on the first TCP error" is exactly the
        // low-latency profile requested in discussion #368.
        REQUIRE(mesh.getTcpRetryConfig().maxRetries == 0);
      }
    }

    WHEN("The unclamped delay fields are set to zero") {
      tcp::TcpRetryConfig custom;
      custom.stabilizationDelayMs = 0;
      custom.exhaustionReconnectDelayMs = 0;
      custom.failureBlockDurationMs = 0;
      mesh.setTcpRetryConfig(custom);
      auto cfg = mesh.getTcpRetryConfig();

      THEN("They pass through untouched") {
        // 0 is meaningful for each: skip stabilization, reconnect immediately,
        // never blocklist.
        REQUIRE(cfg.stabilizationDelayMs == 0);
        REQUIRE(cfg.exhaustionReconnectDelayMs == 0);
        REQUIRE(cfg.failureBlockDurationMs == 0);
      }
    }
  }
}

SCENARIO("The documented tuning profiles are inside the legal envelope",
         "[tcp][retry][config][profiles]") {
  // Catches a doc/code split: if clamping ever tightens, the profiles shipped
  // in examples/tcpRetryConfig and the docs would silently stop working as
  // documented. Each profile must round-trip unchanged.
  GIVEN("An initialised mesh") {
    Scheduler scheduler;
    Mesh<Connection> mesh;
    mesh.init(&scheduler, 1234567);

    WHEN("The real-time profile is applied") {
      tcp::TcpRetryConfig realtime;
      realtime.maxRetries = 1;
      realtime.retryDelayMs = 200;
      realtime.stabilizationDelayMs = 100;
      realtime.exhaustionReconnectDelayMs = 1000;
      realtime.failureBlockDurationMs = 5000;
      mesh.setTcpRetryConfig(realtime);

      THEN("It round-trips without clamping") {
        auto cfg = mesh.getTcpRetryConfig();
        REQUIRE(cfg.maxRetries == realtime.maxRetries);
        REQUIRE(cfg.retryDelayMs == realtime.retryDelayMs);
        REQUIRE(cfg.stabilizationDelayMs == realtime.stabilizationDelayMs);
        REQUIRE(cfg.exhaustionReconnectDelayMs ==
                realtime.exhaustionReconnectDelayMs);
        REQUIRE(cfg.failureBlockDurationMs == realtime.failureBlockDurationMs);
      }
    }

    WHEN("The high-reliability profile is applied") {
      tcp::TcpRetryConfig reliable;
      reliable.maxRetries = 10;
      reliable.retryDelayMs = 2000;
      reliable.stabilizationDelayMs = 1000;
      reliable.exhaustionReconnectDelayMs = 30000;
      reliable.failureBlockDurationMs = 180000;
      mesh.setTcpRetryConfig(reliable);

      THEN("It round-trips without clamping") {
        auto cfg = mesh.getTcpRetryConfig();
        REQUIRE(cfg.maxRetries == reliable.maxRetries);
        REQUIRE(cfg.retryDelayMs == reliable.retryDelayMs);
        REQUIRE(cfg.stabilizationDelayMs == reliable.stabilizationDelayMs);
        REQUIRE(cfg.exhaustionReconnectDelayMs ==
                reliable.exhaustionReconnectDelayMs);
        REQUIRE(cfg.failureBlockDurationMs == reliable.failureBlockDurationMs);
      }
    }

    WHEN("The battery-saver profile is applied") {
      tcp::TcpRetryConfig battery;
      battery.maxRetries = 2;
      battery.retryDelayMs = 3000;
      battery.stabilizationDelayMs = 500;
      battery.exhaustionReconnectDelayMs = 60000;
      battery.failureBlockDurationMs = 300000;
      mesh.setTcpRetryConfig(battery);

      THEN("It round-trips without clamping") {
        auto cfg = mesh.getTcpRetryConfig();
        REQUIRE(cfg.maxRetries == battery.maxRetries);
        REQUIRE(cfg.retryDelayMs == battery.retryDelayMs);
        REQUIRE(cfg.stabilizationDelayMs == battery.stabilizationDelayMs);
        REQUIRE(cfg.exhaustionReconnectDelayMs ==
                battery.exhaustionReconnectDelayMs);
        REQUIRE(cfg.failureBlockDurationMs == battery.failureBlockDurationMs);
      }
    }
  }
}
