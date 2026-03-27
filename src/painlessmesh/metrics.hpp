#ifndef _PAINLESS_MESH_METRICS_HPP_
#define _PAINLESS_MESH_METRICS_HPP_

/**
 * Performance metrics and monitoring utilities for painlessMesh
 *
 * This module provides comprehensive performance monitoring capabilities
 * to help optimize mesh network performance and diagnose issues.
 */

#include <algorithm>
#include <string>
#ifndef ARDUINO
#include <chrono>
#endif
#include "painlessmesh/configuration.hpp"

namespace painlessmesh {
namespace metrics {

/**
 * High-resolution timer for performance measurements
 */
class Timer {
 public:
  Timer() : start_time_(get_time()) {}

  void reset() { start_time_ = get_time(); }

  uint32_t elapsed_ms() const {
    return static_cast<uint32_t>(get_time() - start_time_);
  }

  uint32_t elapsed_us() const {
#ifdef ESP32
    return static_cast<uint32_t>(esp_timer_get_time() - start_time_us_);
#else
    return elapsed_ms() * 1000;  // Fallback to millisecond precision
#endif
  }

 private:
  uint32_t get_time() const {
#ifdef ARDUINO
    return millis();
#else
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
        .count();
#endif
  }

  uint32_t start_time_;
#ifdef ESP32
  int64_t start_time_us_ = esp_timer_get_time();
#endif
};

}  // namespace metrics
}  // namespace painlessmesh

#endif  // _PAINLESS_MESH_METRICS_HPP_
