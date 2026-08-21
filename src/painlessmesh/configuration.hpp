#ifndef _PAINLESS_MESH_CONFIGURATION_HPP_
#define _PAINLESS_MESH_CONFIGURATION_HPP_

// Include Arduino.h in test environment to ensure TSTRING is defined
#if defined(PAINLESSMESH_BOOST) || defined(ARDUINO_ARCH_ESP8266) || \
    defined(ARDUINO_ARCH_ESP32)
#ifndef ARDUINO
#include "Arduino.h"
#endif
#endif

#include <list>

#include "painlessTaskOptions.h"

#include <TaskSchedulerDeclarations.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#undef ARDUINOJSON_ENABLE_STD_STRING
#include <ArduinoJson.h>
#undef ARDUINOJSON_ENABLE_STD_STRING

// Enable (arduino) wifi support
#define PAINLESSMESH_ENABLE_ARDUINO_WIFI

// Enable OTA support. Define PAINLESSMESH_DISABLE_OTA to compile the OTA
// plugin out entirely (see SECURITY.md §OTA). It has to be a build flag --
// `-DPAINLESSMESH_DISABLE_OTA`, or platformio.ini `build_flags` -- because a
// `#undef PAINLESSMESH_ENABLE_OTA` in the sketch runs before painlessMesh.h
// includes this header, which then defines the macro straight back.
#ifndef PAINLESSMESH_DISABLE_OTA
#define PAINLESSMESH_ENABLE_OTA
#endif

// NOTE: there is deliberately no PAINLESSMESH_OTA_REQUIRE_SIGNATURE flag.
// OTA acceptance (painlessmesh/ota.hpp, OTA_OP_CODES::DATA) authenticates
// nothing -- it matches the sender-supplied md5/role/hardware tuple -- so a
// flag that did not gate that path would read as a security control while
// enforcing nothing. Defining it fails the build rather than passing
// silently, so no fleet can ship believing its OTA images are verified.
// Compiling OTA out is the only mitigation the library offers today.
#ifdef PAINLESSMESH_OTA_REQUIRE_SIGNATURE
#error \
    "PAINLESSMESH_OTA_REQUIRE_SIGNATURE is not implemented: painlessMesh does not verify OTA image signatures. Use -DPAINLESSMESH_DISABLE_OTA to compile OTA out, or drop this define."
#endif

// NOTE: there is likewise no PAINLESSMESH_DISABLE_ACK flag. MessageTracker
// (painlessmesh/message_tracker.hpp) is a header-only utility the sketch
// instantiates and sizes itself; the library holds no tracker instance, so
// there is no library-side allocation for such a flag to reclaim.

// NOTE: `MIN_FREE_MEMORY` and `MAX_MESSAGE_QUEUE` are kept as deprecated
// no-op compatibility macros. The library does not read either macro:
// the auto-flushing message queue they were meant to tune never landed
// (see #385, PR #383 review). `MessageQueue`
// (`painlessmesh/message_queue.hpp`) is a manual priority buffer with
// its own per-instance `maxSize` argument. Their historical default
// values are preserved so downstream code that referenced them keeps
// its prior behavior.
#ifndef MIN_FREE_MEMORY
#define MIN_FREE_MEMORY 4000
#endif

#ifndef MAX_MESSAGE_QUEUE
#define MAX_MESSAGE_QUEUE 50
#endif

#define NODE_TIMEOUT 10 * TASK_SECOND
#define SCAN_INTERVAL 30 * TASK_SECOND  // AP scan period in ms

// A gateway relays to the Internet with blocking HTTPClient calls, from inside
// the cooperative scheduler. Those calls must finish well inside NODE_TIMEOUT
// or the gateway's peers reap connections to a node that is perfectly healthy
// (issues #318, #332). The socket-timeout budget that enforces this is derived
// from NODE_TIMEOUT in painlessmesh/gateway.hpp -- see GATEWAY_HTTP_TIMEOUT_MS
// there. Raising NODE_TIMEOUT raises that budget with it.

#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#if ESP_ARDUINO_VERSION_MAJOR >= 3
#include "esp_mac.h"  // required for core 3.x - exposes esp_mac_type_t values
#endif
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif  // ESP32

// Define TSTRING - use std::string in test environment, Arduino String
// otherwise
#if defined(PAINLESSMESH_BOOST)
// Test environment - TSTRING already defined in test Arduino.h as std::string
#else
typedef String TSTRING;
#endif

// backward compatibility
template <typename T>
using SimpleList = std::list<T>;

namespace painlessmesh {
namespace wifi {
class Mesh;
};
};  // namespace painlessmesh

/** A convenience typedef to access the mesh class*/
#ifdef PAINLESSMESH_ENABLE_ARDUINO_WIFI
using painlessMesh = painlessmesh::wifi::Mesh;
#endif

#ifdef ESP32
#define MAX_CONN 10
#else
#define MAX_CONN 4
#endif  // DEBUG

#endif
