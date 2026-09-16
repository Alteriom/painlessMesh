// Pins the "raise both together" remedy documented in CHANGELOG.md and
// SECURITY.md: an endpoint that needs longer than the derived gateway budget
// is handled by raising NODE_TIMEOUT and GATEWAY_HTTP_TIMEOUT_MS together.
//
// That remedy was unfollowable. configuration.hpp defined NODE_TIMEOUT
// unconditionally, so a -DNODE_TIMEOUT on the build line was either silently
// overwritten by the header or a macro-redefinition error under -Werror -- the
// same defect as the `#undef PAINLESSMESH_ENABLE_OTA` advice this release also
// had to fix, in a macro that is genuinely meant to be tuned. Both no-op
// macros beside it (MIN_FREE_MEMORY, MAX_MESSAGE_QUEUE) were already guarded.
//
// NODE_TIMEOUT is supplied by target_compile_definitions() in CMakeLists.txt,
// not by a #define here: it has to be set before any painlessMesh header is
// parsed in every translation unit of the target, which is what the documented
// build-flag form does.
//
// LIMITATION: this file cannot pin the production guard. Arduino.h below is
// test/catch/Arduino.h, which defines _PAINLESS_MESH_CONFIGURATION_HPP_ and
// supplies its own guarded NODE_TIMEOUT, so the #ifndef in the real header is
// never parsed here. If configuration.hpp went unconditional again, this test
// would still pass on the shim's guard. test/ci/tuned-timeouts/ covers that: a
// shim-free PlatformIO build of the real header on both cores. What this file
// adds is cheap per-commit proof on three compilers that the override reaches
// the derived gateway budget and keeps it inside the watchdog.

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

// Declare logger for test environment
painlessmesh::logger::LogClass Log;

#ifndef PAINLESSMESH_TEST_NODE_TIMEOUT_S
#error \
    "PAINLESSMESH_TEST_NODE_TIMEOUT_S is missing -- it must come from target_compile_definitions() in CMakeLists.txt, or this test proves nothing."
#endif

SCENARIO("NODE_TIMEOUT is overridable from the build line") {
  GIVEN("a target built with -DNODE_TIMEOUT") {
    THEN("the override survives painlessMesh's headers") {
      // The whole point: an unguarded #define in configuration.hpp would have
      // replaced this with the 10s default (or failed the build outright).
      REQUIRE(NODE_TIMEOUT == PAINLESSMESH_TEST_NODE_TIMEOUT_S * TASK_SECOND);
    }

    THEN("the gateway budget tracks it instead of the stock default") {
      // GATEWAY_HTTP_TIMEOUT_MS and GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS are
      // derived from NODE_TIMEOUT, so raising one raises the budget with it --
      // which is what makes "raise both together" work rather than trip the
      // static_assert in gateway.hpp.
      REQUIRE(GATEWAY_HTTP_TIMEOUT_MS == PAINLESSMESH_TEST_NODE_TIMEOUT_S * 1000 / 5);
      REQUIRE(GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS ==
              PAINLESSMESH_TEST_NODE_TIMEOUT_S * 1000 / 10);
      REQUIRE(GATEWAY_DNS_TIMEOUT_MS ==
              PAINLESSMESH_TEST_NODE_TIMEOUT_S * 1000 / 10);
    }

    THEN("the blocking budget still fits inside the raised watchdog") {
      // gateway.hpp static_asserts this at compile time; restating it at
      // runtime documents the invariant the override has to preserve.
      REQUIRE(gateway::gatewayBlockingBudgetMs() * TASK_MILLISECOND <
              NODE_TIMEOUT);
    }
  }
}
