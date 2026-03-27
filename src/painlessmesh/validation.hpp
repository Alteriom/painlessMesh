#ifndef _PAINLESS_MESH_VALIDATION_HPP_
#define _PAINLESS_MESH_VALIDATION_HPP_

/**
 * Input validation and security utilities for painlessMesh
 *
 * This module provides comprehensive validation utilities to improve
 * security and robustness of the mesh network.
 */

#ifndef ARDUINO
#include <chrono>
#endif
#include "painlessmesh/configuration.hpp"

namespace painlessmesh {
namespace validation {

/**
 * Message validation result
 */
enum class ValidationResult {
  VALID = 0,
  INVALID_JSON,
  MISSING_REQUIRED_FIELD,
  INVALID_FIELD_TYPE,
  INVALID_FIELD_VALUE,
  MESSAGE_TOO_LARGE,
  INVALID_NODE_ID,
  RATE_LIMIT_EXCEEDED
};

/**
 * Configuration for message validation
 */
struct ValidationConfig {
  size_t max_message_size = 8192;     // Maximum message size in bytes
  size_t max_string_length = 1024;    // Maximum string field length
  uint32_t min_node_id = 1;           // Minimum valid node ID
  uint32_t max_node_id = 0xFFFFFFFF;  // Maximum valid node ID
  size_t max_nesting_depth = 10;      // Maximum JSON nesting depth
  bool strict_type_checking = true;   // Enable strict type validation
};

/**
 * Secure random number generation for mesh operations
 */
class SecureRandom {
 public:
  /**
   * Generate a cryptographically secure random number
   * Falls back to pseudo-random if hardware RNG is not available
   */
  static uint32_t generate() {
#ifdef ESP32
    return esp_random();
#elif defined(ESP8266)
    return RANDOM_REG32;
#else
    // Fallback for other platforms - use system rand with better seeding
    static bool seeded = false;
    if (!seeded) {
#ifdef ARDUINO
      // Arduino-compatible seeding
      srand(millis() ^ analogRead(A0));
#else
      // Non-Arduino platforms with std::chrono
      auto now = std::chrono::steady_clock::now();
      auto duration = now.time_since_epoch();
      uint32_t seed =
          std::chrono::duration_cast<std::chrono::microseconds>(duration)
              .count();
      srand(seed);
#endif
      seeded = true;
    }
    return ((uint32_t)rand() << 16) | rand();
#endif
  }

  /**
   * Generate random bytes into buffer
   */
  static void generate_bytes(uint8_t* buffer, size_t length) {
    for (size_t i = 0; i < length; i += sizeof(uint32_t)) {
      uint32_t random_val = generate();
      size_t copy_len = std::min(sizeof(uint32_t), length - i);
      memcpy(buffer + i, &random_val, copy_len);
    }
  }
};

}  // namespace validation
}  // namespace painlessmesh

#endif  // _PAINLESS_MESH_VALIDATION_HPP_