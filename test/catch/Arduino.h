/**
 * Wrapper file, which is used to test on PC hardware
 */
#ifndef ARDUINO_WRAP_H
#define ARDUINO_WRAP_H

#include <sys/time.h>
#include <unistd.h>

#define ARDUINO_ARCH_ESP8266
#define PAINLESSMESH_BOOST

#ifndef NULL
#define NULL 0
#endif

inline unsigned long millis() {
  struct timeval te;
  gettimeofday(&te, NULL);  // get current time
  long long milliseconds =
      te.tv_sec * 1000LL + te.tv_usec / 1000;  // calculate milliseconds
  // printf("milliseconds: %lld\n", milliseconds);
  return milliseconds;
}

inline unsigned long micros() {
  struct timeval te;
  gettimeofday(&te, NULL);  // get current time
  long long milliseconds = te.tv_sec * 1000000LL + te.tv_usec;
  return milliseconds;
}

inline void delay(int i) { usleep(i); }

inline void yield() {}

struct IPAddress {
  IPAddress() {}
  IPAddress(int, int, int, int) {}
};

#ifndef __FlashStringHelper
#include <cstddef>  // for size_t
#include <cstring>  // for strcpy, strlen

class __FlashStringHelper {
 private:
  const char* str;

 public:
  // Constructor from a C string (for F("") macro simulation)
  __FlashStringHelper(const char* s) : str(s) {}

  // Allow implicit conversion to const char* for compatibility
  operator const char*() const { return str; }

  // Optional: Add methods to mimic Arduino's flash string behavior
  char charAt(size_t index) const { return str[index]; }
  size_t length() const { return strlen(str); }
};

// Define the F() macro to return a __FlashStringHelper
#define F(str) __FlashStringHelper(str)
#endif

/**
 * Override the configution file.
 **/

#ifndef _PAINLESS_MESH_CONFIGURATION_HPP_
#define _PAINLESS_MESH_CONFIGURATION_HPP_

#define _TASK_PRIORITY  // Support for layered scheduling priority
#define _TASK_STD_FUNCTION

#include <TaskSchedulerDeclarations.h>

#define ARDUINOJSON_USE_LONG_LONG 1

#include <ArduinoJson.h>
#undef ARDUINOJSON_ENABLE_ARDUINO_STRING

#define ICACHE_FLASH_ATTR

#define PAINLESSMESH_ENABLE_STD_STRING

// Enable OTA support
#define PAINLESSMESH_ENABLE_OTA

#define NODE_TIMEOUT 5 * TASK_SECOND

typedef std::string TSTRING;

#ifdef ESP32
#define MAX_CONN 10
#else
#define MAX_CONN 4
#endif  // DEBUG

#include "fake_asynctcp.hpp"
#include "fake_serial.hpp"

extern WiFiClass WiFi;
extern ESPClass ESP;

#endif
#endif
