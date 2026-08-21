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

// Enable OTA support
#define PAINLESSMESH_ENABLE_OTA

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

// ---------------------------------------------------------------------------
// Gateway blocking-request budget
// ---------------------------------------------------------------------------
//
// The gateway's Internet handler runs inside the cooperative TaskScheduler, so
// every millisecond it spends inside a blocking HTTPClient call is a
// millisecond in which no mesh task runs -- including the node-sync replies
// that every peer's NODE_TIMEOUT watchdog is waiting for. If the gateway can
// block for longer than NODE_TIMEOUT, its peers reap a connection to a node
// that is perfectly healthy, and the mesh partitions around the gateway with
// the reported signature "Internet available via gateway: YES / Mesh
// connections active: NO" (issues #318, #332).
//
// The budgets below therefore have to add up to comfortably less than
// NODE_TIMEOUT. `painlessmesh::gateway::gatewayBlockingBudgetMs()` static
// asserts exactly that, so raising one of these past the watchdog is a compile
// error rather than a field partition. If you genuinely need a longer HTTP
// timeout, raise NODE_TIMEOUT with it.

/** Socket timeout, in milliseconds, for a gateway Internet request. */
#ifndef GATEWAY_HTTP_TIMEOUT_MS
#define GATEWAY_HTTP_TIMEOUT_MS 5000UL
#endif

/** Socket timeout, in milliseconds, for the captive-portal probe. */
#ifndef GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS
#define GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS 2000UL
#endif

/** How long, in milliseconds, a connectivity probe result stays cached.
 *
 * Both the DNS reachability check and the captive-portal probe are network
 * round trips. Running them per message would put an unbounded Internet round
 * trip in front of every single mesh->Internet send.
 */
#ifndef GATEWAY_CONNECTIVITY_CACHE_MS
#define GATEWAY_CONNECTIVITY_CACHE_MS 60000UL
#endif

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
