/**
 * @file catch_connection_cleanup.cpp
 * @brief Regression tests for issue #373 — use-after-free in Task::disable()
 *
 * scheduleAsyncClientDeletion() used to delete its own cleanup Task from
 * inside that task's onDisable callback. TaskScheduler's Task::disable()
 * writes to the task object (iScheduler->iCurrent) *after* onDisable
 * returns, so the old code was a guaranteed use-after-free — crashing ESP32
 * nodes on every connection teardown (issue #373, fixed via PR #376).
 *
 * The fix marks the cleanup task with setSelfDestruct(true)
 * (_TASK_SELF_DESTRUCT, enabled in painlessTaskOptions.h): the Scheduler
 * deletes the task from within execute(), after disable() has returned.
 *
 * These tests prove their worth under AddressSanitizer (see the asan job in
 * ci.yml): against the pre-fix code they report heap-use-after-free inside
 * Task::disable(); with the fix they run clean. Without ASan they still
 * exercise the full schedule → fire → self-destruct lifecycle and guard
 * against outright crashes and double-frees.
 */
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "Arduino.h"
#include "painlessmesh/connection.hpp"

painlessmesh::logger::LogClass Log;

using painlessmesh::tcp::BufferedConnection;
using painlessmesh::tcp::scheduleAsyncClientDeletion;
using painlessmesh::tcp::TCP_CLIENT_CLEANUP_DELAY_MS;
using painlessmesh::tcp::TCP_CLIENT_DELETION_SPACING_MS;

namespace {

// Pump the scheduler for (at least) `ms` milliseconds of wall-clock time.
// The test-environment delay() maps to usleep(), so delay(1000) ≈ 1 ms.
void pump(Scheduler& scheduler, uint32_t ms) {
  auto start = millis();
  while (millis() - start < ms) {
    scheduler.execute();
    delay(1000);
  }
}

}  // namespace

SCENARIO(
    "scheduleAsyncClientDeletion tears down its own cleanup task safely "
    "(issue #373)") {
  GIVEN("A scheduler with several AsyncClient deletions scheduled") {
    Scheduler scheduler;
    const int numClients = 3;
    for (int i = 0; i < numClients; ++i) {
      scheduleAsyncClientDeletion(&scheduler, new AsyncClient(),
                                  "catch_connection_cleanup");
    }

    WHEN("the scheduler runs past the cleanup delay and spacing window") {
      // Deletions are spaced TCP_CLIENT_DELETION_SPACING_MS apart after a
      // TCP_CLIENT_CLEANUP_DELAY_MS base delay; add margin for jitter.
      pump(scheduler, TCP_CLIENT_CLEANUP_DELAY_MS +
                          numClients * TCP_CLIENT_DELETION_SPACING_MS + 500);

      THEN("every cleanup task has fired and self-destructed cleanly") {
        // The load-bearing assertion is AddressSanitizer staying silent:
        // the pre-fix code deleted the Task inside its own onDisable, and
        // Task::disable() then wrote to the freed object. Reaching this
        // point without a crash is the non-ASan part of the regression.
        SUCCEED("cleanup tasks completed without use-after-free");
      }
    }
  }
}

SCENARIO(
    "BufferedConnection destruction defers AsyncClient deletion safely "
    "under churn") {
  GIVEN("A scheduler and repeated connection create/destroy cycles") {
    Scheduler scheduler;
    const int churnCycles = 3;

    WHEN("connections are created, initialized, and destroyed in sequence") {
      for (int i = 0; i < churnCycles; ++i) {
        auto conn = std::make_shared<BufferedConnection>(new AsyncClient());
        conn->initialize(&scheduler);
        pump(scheduler, 10);
        // ~BufferedConnection calls scheduleAsyncClientDeletion(), the
        // path that crashed in the field (issue #373 backtraces show
        // Task::disable() ← Scheduler::execute() ← Mesh::update()).
        conn.reset();
      }

      THEN("pumping past the cleanup window completes without crashes") {
        pump(scheduler,
             TCP_CLIENT_CLEANUP_DELAY_MS +
                 (churnCycles + 1) * TCP_CLIENT_DELETION_SPACING_MS + 500);
        SUCCEED("deferred deletions completed without use-after-free");
      }
    }
  }
}
