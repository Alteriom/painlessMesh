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

// NOTE: `MIN_FREE_MEMORY` and `MAX_MESSAGE_QUEUE` used to live here as
// safety knobs for an auto-flushing message queue that never landed
// (see #385, PR #383 review). They were read by nothing in the library
// and were removed to avoid implying behavior that does not exist. The
// `MessageQueue` class (`painlessmesh/message_queue.hpp`) is a manual
// priority buffer with its own per-instance `maxSize` argument.

#define NODE_TIMEOUT 10 * TASK_SECOND
#define SCAN_INTERVAL 30 * TASK_SECOND  // AP scan period in ms

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
