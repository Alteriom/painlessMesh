#ifndef _PAINLESS_MESH_VALIDATION_HPP_
#define _PAINLESS_MESH_VALIDATION_HPP_

/**
 * Input validation and security utilities for painlessMesh
 *
 * This module provides comprehensive validation utilities to improve
 * security and robustness of the mesh network.
 */

#include <list>
#include <map>
#ifndef ARDUINO
#include <chrono>
#endif
#include "ArduinoJson.h"
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
 * Rate limiting for preventing message spam
 */
class RateLimiter {
 public:
  RateLimiter(size_t max_messages_per_second = 10, size_t window_size_ms = 1000)
      : max_messages_(max_messages_per_second), window_size_(window_size_ms) {}

  bool allow_message(uint32_t node_id) {
    uint32_t current_time = get_current_time();
    auto& history = node_history_[node_id];

    // Remove old entries outside the window
    while (!history.empty() &&
           (current_time - history.front()) > window_size_) {
      history.pop_front();
    }

    // Check if limit is exceeded
    if (history.size() >= max_messages_) {
      return false;
    }

    // Record this message
    history.push_back(current_time);
    return true;
  }

  void clear_node_history(uint32_t node_id) { node_history_.erase(node_id); }

  void clear_all_history() { node_history_.clear(); }

 private:
  uint32_t get_current_time() const {
#ifdef ARDUINO
    return millis();
#else
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
        .count();
#endif
  }

  size_t max_messages_;
  size_t window_size_;
  std::map<uint32_t, std::list<uint32_t>> node_history_;
};

/**
 * JSON message validator
 */
class MessageValidator {
 public:
  explicit MessageValidator(const ValidationConfig& config = ValidationConfig{})
      : config_(config) {}

  /**
   * Validate a JSON message for basic structure and security
   */
  ValidationResult validate_message(const JsonObject& obj,
                                    size_t message_size = 0) const {
    // Check message size
    if (message_size > config_.max_message_size) {
      return ValidationResult::MESSAGE_TOO_LARGE;
    }

    // Check required fields based on message type
    if (!obj["type"].is<int>()) {
      return ValidationResult::MISSING_REQUIRED_FIELD;
    }

    int msg_type = obj["type"].as<int>();

    // Validate node IDs if present
    if (obj["from"].is<uint32_t>()) {
      uint32_t from_id = obj["from"].as<uint32_t>();
      if (!is_valid_node_id(from_id)) {
        return ValidationResult::INVALID_NODE_ID;
      }
    }

    if (obj["dest"].is<uint32_t>()) {
      uint32_t dest_id = obj["dest"].as<uint32_t>();
      if (dest_id != 0 && !is_valid_node_id(dest_id)) {  // 0 is broadcast
        return ValidationResult::INVALID_NODE_ID;
      }
    }

    // Validate string fields
    for (JsonPair pair : obj) {
      if (pair.value().is<const char*>()) {
        const char* str_value = pair.value().as<const char*>();
        if (strlen(str_value) > config_.max_string_length) {
          return ValidationResult::INVALID_FIELD_VALUE;
        }
      }
    }

    return ValidationResult::VALID;
  }

  /**
   * Validate node ID range
   */
  bool is_valid_node_id(uint32_t node_id) const {
    return node_id >= config_.min_node_id && node_id <= config_.max_node_id;
  }

  /**
   * Get validation error message
   */
  const char* get_error_message(ValidationResult result) const {
    switch (result) {
      case ValidationResult::VALID:
        return "Valid";
      case ValidationResult::INVALID_JSON:
        return "Invalid JSON format";
      case ValidationResult::MISSING_REQUIRED_FIELD:
        return "Missing required field";
      case ValidationResult::INVALID_FIELD_TYPE:
        return "Invalid field type";
      case ValidationResult::INVALID_FIELD_VALUE:
        return "Invalid field value";
      case ValidationResult::MESSAGE_TOO_LARGE:
        return "Message too large";
      case ValidationResult::INVALID_NODE_ID:
        return "Invalid node ID";
      case ValidationResult::RATE_LIMIT_EXCEEDED:
        return "Rate limit exceeded";
      default:
        return "Unknown error";
    }
  }

  const ValidationConfig& get_config() const { return config_; }
  void set_config(const ValidationConfig& config) { config_ = config; }

 private:
  ValidationConfig config_;
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