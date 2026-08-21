// Regression fixture: painlessMesh must build with OTA compiled out.
//
// See platformio.ini in this directory for why this exists as a separate
// project rather than as another desktop test.

#include "painlessMesh.h"

// The assertion. configuration.hpp guards its default definition behind
// PAINLESSMESH_DISABLE_OTA; if that guard is ever dropped, this fails here
// instead of silently shipping OTA to operators who followed SECURITY.md.
#ifdef PAINLESSMESH_ENABLE_OTA
#error \
    "PAINLESSMESH_DISABLE_OTA did not compile OTA out. The opt-out documented in SECURITY.md is broken -- see painlessmesh/configuration.hpp."
#endif

#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

Scheduler userScheduler;
painlessMesh mesh;

// Exercises the ordinary API surface, so this also catches a guard placed so
// tightly that a non-OTA build stops compiling -- offerOTA(), initOTASend()
// and initOTAReceive() live behind the same #ifdef in mesh.hpp.
void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}

void loop() { mesh.update(); }
