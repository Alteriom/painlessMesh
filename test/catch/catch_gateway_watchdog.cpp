/**
 * @file catch_gateway_watchdog.cpp
 * @brief Regression tests for the gateway scheduler stall (issues #318, #332)
 *
 * A gateway node relays mesh messages to the Internet from inside the
 * cooperative TaskScheduler, using blocking HTTPClient calls. Nothing else
 * runs during those calls, but wall-clock time keeps passing -- so every
 * peer's NODE_TIMEOUT watchdog can fall due mid-stall and fire the instant the
 * scheduler resumes, closing connections to nodes that never went missing.
 * Users reported the resulting partition as "Internet available via gateway:
 * YES / Mesh connections active: NO".
 *
 * The fix has two halves, and both are covered here:
 *
 *  1. gatewayBlockingBudgetMs() must stay under NODE_TIMEOUT, so a stall can
 *     never outlast what a *remote* peer is willing to wait for a node-sync
 *     reply. Nothing the gateway does locally can help that peer; only being
 *     quick enough can.
 *  2. refreshPeerWatchdogs() re-arms the gateway's own view of its peers after
 *     a stall, so overdue watchdogs get a fresh window instead of firing
 *     immediately.
 *
 * The pre-fix mitigation did neither: it disabled timeOutTask on the
 * requesting connection only, and it did so *before* the stall.
 */
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "Arduino.h"

#include "painlessmesh/gateway.hpp"

#include <list>
#include <memory>

painlessmesh::logger::LogClass Log;

namespace {

// Short enough to keep the tests fast; the helper is agnostic to the actual
// interval, which on a real Connection is NODE_TIMEOUT.
constexpr unsigned long kWatchdogMs = 50;

/** Minimal stand-in for painlessmesh::Connection: a watchdog that closes. */
struct FakeConnection {
  uint32_t nodeId = 0;
  bool closed = false;
  Task timeOutTask;

  void arm(Scheduler& scheduler) {
    timeOutTask.set(kWatchdogMs, TASK_ONCE, [this]() { this->closed = true; });
    scheduler.addTask(timeOutTask);
  }
};

/** Minimal stand-in for the mesh: layout::Layout exposes `subs` publicly. */
struct FakeMesh {
  std::list<std::shared_ptr<FakeConnection> > subs;
};

std::shared_ptr<FakeConnection> addPeer(FakeMesh& mesh, Scheduler& scheduler,
                                        uint32_t nodeId) {
  auto conn = std::make_shared<FakeConnection>();
  conn->nodeId = nodeId;
  conn->arm(scheduler);
  mesh.subs.push_back(conn);
  return conn;
}

/** Burn `ms` of wall-clock time *without* running the scheduler.
 *
 * This is what a blocking HTTPClient call does to a painlessMesh node.
 * In the test environment delay() maps to usleep(), so the argument is
 * microseconds.
 */
void stall(uint32_t ms) {
  auto start = millis();
  while (millis() - start < ms) delay(1000);
}

/** Run the scheduler for `ms`, the way loop() would. */
void pump(Scheduler& scheduler, uint32_t ms) {
  auto start = millis();
  do {
    scheduler.execute();
    delay(1000);
  } while (millis() - start < ms);
}

}  // namespace

SCENARIO("The gateway blocking budget fits inside the mesh watchdog") {
  GIVEN("the configured gateway timeouts") {
    THEN("a single request cannot outlast a peer's NODE_TIMEOUT") {
      // The compile-time twin of this check lives in gateway.hpp. Asserting it
      // at runtime too means the CI log names the numbers when it regresses.
      REQUIRE(painlessmesh::gateway::gatewayBlockingBudgetMs() *
                  TASK_MILLISECOND <
              static_cast<unsigned long>(NODE_TIMEOUT));
    }

    THEN("the budget is the sum of the two blocking socket timeouts") {
      REQUIRE(painlessmesh::gateway::gatewayBlockingBudgetMs() ==
              static_cast<unsigned long>(GATEWAY_HTTP_TIMEOUT_MS) +
                  static_cast<unsigned long>(GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS));
    }

    THEN("it leaves real headroom, not just a strict inequality") {
      // Fitting by 1ms would satisfy the static_assert and still partition the
      // mesh: the peer's deadline is already partly spent by the time its
      // request reaches a gateway that is mid-stall. Require a quarter of
      // NODE_TIMEOUT to be left over.
      auto budget =
          painlessmesh::gateway::gatewayBlockingBudgetMs() * TASK_MILLISECOND;
      auto watchdog = static_cast<unsigned long>(NODE_TIMEOUT);
      REQUIRE(watchdog - budget >= watchdog / 4);
    }
  }
}

SCENARIO("A stall reaps peers whose watchdog fell due while the CPU was held") {
  GIVEN("two peers with a running watchdog") {
    Scheduler scheduler;
    FakeMesh mesh;
    auto a = addPeer(mesh, scheduler, 1);
    auto b = addPeer(mesh, scheduler, 2);
    a->timeOutTask.restartDelayed();
    b->timeOutTask.restartDelayed();

    WHEN("the scheduler is held past the watchdog and then resumes") {
      stall(kWatchdogMs * 3);
      // A few passes rather than one: Scheduler::execute() is not guaranteed
      // to drain every due task in a single sweep, and the point here is that
      // the watchdogs fire immediately, not which pass they land on.
      scheduler.execute();
      scheduler.execute();
      scheduler.execute();

      THEN("both peers are closed as soon as the scheduler resumes") {
        REQUIRE(a->closed);
        REQUIRE(b->closed);
      }
    }

    WHEN("the watchdogs are compensated before the scheduler resumes") {
      stall(kWatchdogMs * 3);
      auto refreshed =
          painlessmesh::gateway::refreshPeerWatchdogs(mesh, kWatchdogMs * 3);
      scheduler.execute();

      THEN("both watchdogs were compensated") { REQUIRE(refreshed == static_cast<size_t>(2)); }

      THEN("neither peer is closed") {
        REQUIRE_FALSE(a->closed);
        REQUIRE_FALSE(b->closed);
      }

      THEN("they still fire if the peer really is gone") {
        pump(scheduler, kWatchdogMs * 3);
        REQUIRE(a->closed);
        REQUIRE(b->closed);
      }
    }
  }
}

SCENARIO("Refreshing never arms a watchdog that was not already running") {
  GIVEN("an idle peer whose watchdog is disabled") {
    // This is the normal steady state: nodeSyncTask arms timeOutTask when a
    // sync request goes out, and the NODE_SYNC_REPLY handler disables it
    // again. Arming it here would invent a deadline for a healthy link and
    // close it -- turning a reliability fix into a disconnect bug.
    Scheduler scheduler;
    FakeMesh mesh;
    auto idle = addPeer(mesh, scheduler, 1);
    REQUIRE_FALSE(idle->timeOutTask.isEnabled());

    WHEN("the watchdogs are refreshed and the scheduler runs well past NODE_TIMEOUT") {
      auto refreshed =
          painlessmesh::gateway::refreshPeerWatchdogs(mesh, kWatchdogMs);
      pump(scheduler, kWatchdogMs * 3);

      THEN("nothing was re-armed") { REQUIRE(refreshed == static_cast<size_t>(0)); }
      THEN("the watchdog is still disabled") {
        REQUIRE_FALSE(idle->timeOutTask.isEnabled());
      }
      THEN("the idle peer is not closed") { REQUIRE_FALSE(idle->closed); }
    }
  }

  GIVEN("a mix of armed and idle peers") {
    Scheduler scheduler;
    FakeMesh mesh;
    auto armed = addPeer(mesh, scheduler, 1);
    auto idle = addPeer(mesh, scheduler, 2);
    armed->timeOutTask.restartDelayed();

    WHEN("the watchdogs are refreshed") {
      auto refreshed =
          painlessmesh::gateway::refreshPeerWatchdogs(mesh, kWatchdogMs);

      THEN("only the armed peer is counted") { REQUIRE(refreshed == static_cast<size_t>(1)); }
      THEN("the idle peer stays disabled") {
        REQUIRE_FALSE(idle->timeOutTask.isEnabled());
      }
    }
  }
}

SCENARIO("Refreshing tolerates an empty or partly-empty peer list") {
  GIVEN("a mesh with no peers") {
    FakeMesh mesh;

    THEN("refreshing is a no-op") {
      REQUIRE(painlessmesh::gateway::refreshPeerWatchdogs(mesh, kWatchdogMs) ==
              static_cast<size_t>(0));
    }
  }

  GIVEN("a peer list containing a null entry") {
    // subs is a list of shared_ptr and is mutated from connection teardown, so
    // a null slot is reachable. Dereferencing it would crash the gateway on
    // the very path that is supposed to make it more reliable.
    Scheduler scheduler;
    FakeMesh mesh;
    mesh.subs.push_back(nullptr);
    auto armed = addPeer(mesh, scheduler, 1);
    armed->timeOutTask.restartDelayed();

    THEN("the null entry is skipped and the real peer still refreshed") {
      REQUIRE(painlessmesh::gateway::refreshPeerWatchdogs(mesh, kWatchdogMs) ==
              static_cast<size_t>(1));
    }
  }
}

SCENARIO("Compensation equals the measured stall, not a full reset (#417)") {
  GIVEN("a peer with a running watchdog") {
    Scheduler scheduler;
    FakeMesh mesh;
    auto peer = addPeer(mesh, scheduler, 1);
    peer->timeOutTask.restartDelayed();

    WHEN("a path that never blocked reports a zero stall") {
      auto refreshed = painlessmesh::gateway::refreshPeerWatchdogs(mesh, 0);

      THEN("nothing is compensated") {
        REQUIRE(refreshed == static_cast<size_t>(0));
      }

      THEN("the watchdog still fires on its original schedule") {
        pump(scheduler, kWatchdogMs * 2);
        REQUIRE(peer->closed);
      }
    }

    WHEN("a short stall is compensated while the peer stays silent") {
      // A watchdog granted only the measured stall keeps its original
      // baseline: deadline = armed-at + kWatchdogMs + stall. The peer must
      // still be reaped shortly after, not handed a full fresh window.
      stall(kWatchdogMs / 2);
      painlessmesh::gateway::refreshPeerWatchdogs(mesh, kWatchdogMs / 2);

      THEN("the peer is reaped once its observed time is spent") {
        pump(scheduler, kWatchdogMs + kWatchdogMs / 2);
        REQUIRE(peer->closed);
      }
    }
  }

  GIVEN("a dead peer on a gateway with continuous traffic from live peers") {
    // The regression #417 fixes: with restart-from-zero semantics, every
    // gateway request from a *live* peer granted the dead peer a fresh full
    // NODE_TIMEOUT, so as long as traffic kept arriving faster than the
    // watchdog interval the dead connection was never reaped.
    Scheduler scheduler;
    FakeMesh mesh;
    auto dead = addPeer(mesh, scheduler, 1);
    dead->timeOutTask.restartDelayed();

    WHEN("stall-compensated refreshes arrive faster than the watchdog window") {
      // Six cycles of a 10th-of-a-window stall + honest compensation, with
      // the scheduler observing half a window between them. Under the old
      // full-reset semantics the watchdog restarts every cycle and never
      // fires. Under measured-stall compensation the observed time
      // accumulates past kWatchdogMs and the dead peer is reaped mid-loop.
      for (int i = 0; i < 6 && !dead->closed; ++i) {
        stall(kWatchdogMs / 10);
        painlessmesh::gateway::refreshPeerWatchdogs(mesh, kWatchdogMs / 10);
        pump(scheduler, kWatchdogMs / 2);
      }

      THEN("the dead peer is still reaped") { REQUIRE(dead->closed); }
    }
  }
}
