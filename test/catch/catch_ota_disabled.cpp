// Pins the OTA opt-out documented in SECURITY.md §OTA firmware updates.
//
// OTA is on by default and there is no signature check on incoming images, so
// compiling the plugin out is the only mitigation the library offers. That
// mitigation was previously unreachable: configuration.hpp defined
// PAINLESSMESH_ENABLE_OTA unconditionally, so the `#undef` the docs told
// operators to use was overwritten by the very header the docs pointed at, and
// nothing in CI compiled an OTA-free build to notice. This translation unit is
// that build.
//
// The first run of this test earned its keep by failing: the desktop harness
// shims (test/catch/Arduino.h, test/boost/Arduino.h) defined
// PAINLESSMESH_ENABLE_OTA a second time, so fixing configuration.hpp alone
// still produced an OTA build here. Both now mirror the same guard.
//
// PAINLESSMESH_DISABLE_OTA is deliberately NOT defined in this file. It comes
// from target_compile_definitions() in CMakeLists.txt, because that is the
// only correct way to set it: defining it in one source file would give that
// translation unit a painlessmesh::Mesh with different members than the rest
// of the link, which is an ODR violation with no required diagnostic. The
// test sets it the way SECURITY.md tells operators to.

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "painlessmesh/mesh.hpp"

using namespace painlessmesh;

// Declare logger for test environment
painlessmesh::logger::LogClass Log;

#ifndef PAINLESSMESH_DISABLE_OTA
#error \
    "PAINLESSMESH_DISABLE_OTA is missing -- it must come from target_compile_definitions() in CMakeLists.txt, or this test proves nothing."
#endif

#ifdef PAINLESSMESH_ENABLE_OTA
#error \
    "PAINLESSMESH_DISABLE_OTA did not suppress PAINLESSMESH_ENABLE_OTA -- the OTA opt-out documented in SECURITY.md is broken."
#endif

SCENARIO("PAINLESSMESH_DISABLE_OTA compiles the OTA plugin out") {
  GIVEN("A translation unit built with PAINLESSMESH_DISABLE_OTA") {
    THEN("PAINLESSMESH_ENABLE_OTA is not defined") {
      // The #error above is the real assertion -- this build would not exist
      // if the macro had survived. Restated at runtime so the guarantee shows
      // up in the test report rather than only in a compiler diagnostic.
#ifdef PAINLESSMESH_ENABLE_OTA
      REQUIRE(false);
#else
      REQUIRE(true);
#endif
    }

    THEN("the mesh still builds and initialises without the OTA plugin") {
      // offerOTA(), initOTASend(), initOTAReceive() and the ota.hpp include all
      // live behind PAINLESSMESH_ENABLE_OTA in mesh.hpp. If disabling OTA left
      // a dangling reference to painlessmesh::plugin::ota, or left one of those
      // members outside the guard, this file would not compile.
      Scheduler scheduler;
      Mesh<Connection> mesh;
      mesh.init(&scheduler, 12345);

      REQUIRE(mesh.getNodeId() == 12345);
      REQUIRE(mesh.getNodeList().size() == 0);

      mesh.stop();
    }
  }
}
