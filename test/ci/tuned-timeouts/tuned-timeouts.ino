// Regression fixture: NODE_TIMEOUT must survive as a build flag.
//
// See platformio.ini in this directory for why this exists as a separate
// project rather than only as a desktop test.

#include "painlessMesh.h"

// The assertion. configuration.hpp guards its NODE_TIMEOUT definition with
// #ifndef; if that guard is ever dropped, the header silently replaces the
// value below with the 10s default and the documented "raise both together"
// remedy stops working. Fail here instead.
#if NODE_TIMEOUT != (20 * TASK_SECOND)
#error \
    "-DNODE_TIMEOUT did not survive painlessMesh's headers. The remedy documented in CHANGELOG.md and SECURITY.md is broken -- see painlessmesh/configuration.hpp."
#endif

// The derived gateway timeouts must track the override, which is what makes
// raising the watchdog raise the budget with it rather than trip the
// static_assert in gateway.hpp. Written against TASK_MILLISECOND so it holds
// under _TASK_MICRO_RES too.
static_assert(GATEWAY_HTTP_TIMEOUT_MS ==
                  (20 * TASK_SECOND) / TASK_MILLISECOND / 5,
              "GATEWAY_HTTP_TIMEOUT_MS stopped tracking NODE_TIMEOUT");
static_assert(GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS ==
                  (20 * TASK_SECOND) / TASK_MILLISECOND / 10,
              "GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS stopped tracking NODE_TIMEOUT");
static_assert(GATEWAY_DNS_TIMEOUT_MS ==
                  (20 * TASK_SECOND) / TASK_MILLISECOND / 10,
              "GATEWAY_DNS_TIMEOUT_MS stopped tracking NODE_TIMEOUT");

#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}

void loop() { mesh.update(); }
