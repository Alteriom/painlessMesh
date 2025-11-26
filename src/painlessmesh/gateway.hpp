#ifndef _PAINLESS_MESH_GATEWAY_HPP_
#define _PAINLESS_MESH_GATEWAY_HPP_

/**
 * @file gateway.hpp
 * @brief Shared Gateway Mode configuration for painlessMesh
 *
 * This file defines the SharedGatewayConfig structure which provides
 * configuration for the Shared Gateway Mode functionality. This feature
 * allows any node in the mesh to become a gateway to the Internet when
 * configured with router credentials.
 *
 * MEMORY FOOTPRINT
 * ================
 * The SharedGatewayConfig structure has an estimated memory footprint of:
 * - Base fields (booleans, integers): ~44 bytes
 * - TSTRING fields (routerSSID, routerPassword, internetCheckHost):
 *   - ESP8266/ESP32 String: ~12 bytes overhead per String + content length
 *   - PC/Test std::string: ~32 bytes overhead per string + content length
 * - Total estimated minimum: ~80 bytes (ESP) to ~140 bytes (PC/Test)
 * - With typical content: ~150-250 bytes depending on SSID/password length
 *
 * For ESP8266 with ~80KB RAM, this represents <0.5% of available memory.
 * For ESP32 with ~320KB RAM, this represents <0.1% of available memory.
 *
 * CONFIGURATION DEFAULTS
 * ======================
 * All time-based fields are stored in milliseconds for consistency with
 * the Alteriom time field naming convention. Default values are chosen
 * for reliable operation in typical home/office mesh deployments.
 */

#include "Arduino.h"
#include "painlessmesh/configuration.hpp"
#include "painlessmesh/logger.hpp"

namespace painlessmesh {
namespace gateway {

/**
 * @brief Validation result structure for SharedGatewayConfig
 *
 * Provides detailed validation feedback with error messages.
 */
struct ValidationResult {
  bool valid = true;
  TSTRING errorMessage = "";

  ValidationResult() = default;
  ValidationResult(bool v, const TSTRING& msg) : valid(v), errorMessage(msg) {}

  explicit operator bool() const { return valid; }
};

/**
 * @brief Configuration structure for Shared Gateway Mode
 *
 * This structure holds all configuration parameters needed for a node
 * to operate as a shared gateway, providing Internet connectivity to
 * the mesh network.
 *
 * When enabled and configured with valid router credentials, a node can:
 * - Connect to an external WiFi router for Internet access
 * - Relay messages from the mesh to Internet services (e.g., MQTT)
 * - Participate in gateway election when the current gateway fails
 * - Broadcast its status to other mesh nodes
 *
 * Example usage:
 * @code
 * SharedGatewayConfig config;
 * config.enabled = true;
 * config.routerSSID = "MyHomeWiFi";
 * config.routerPassword = "secretpassword";
 *
 * auto result = config.validate();
 * if (!result.valid) {
 *     Serial.println(result.errorMessage.c_str());
 * }
 * @endcode
 */
struct SharedGatewayConfig {
  // ============================================
  // Core Configuration
  // ============================================

  /**
   * @brief Enable or disable shared gateway functionality
   *
   * When false, this node will not attempt to act as a gateway.
   */
  bool enabled = false;

  /**
   * @brief Router SSID to connect for Internet access
   *
   * The SSID of the external WiFi router that provides Internet connectivity.
   * Required when enabled is true.
   */
  TSTRING routerSSID = "";

  /**
   * @brief Router password for authentication
   *
   * The password for the external WiFi router.
   * May be empty for open networks (not recommended).
   */
  TSTRING routerPassword = "";

  // ============================================
  // Internet Connectivity Checking
  // ============================================

  /**
   * @brief Interval between Internet connectivity checks in milliseconds
   *
   * How often the gateway should verify it can reach the Internet.
   * Lower values provide faster failure detection but increase network usage.
   * Default: 30000ms (30 seconds)
   */
  uint32_t internetCheckInterval = 30000;

  /**
   * @brief Host to ping for Internet connectivity verification
   *
   * A reliable external host used to verify Internet connectivity.
   * Default: "8.8.8.8" (Google Public DNS)
   */
  TSTRING internetCheckHost = "8.8.8.8";

  /**
   * @brief Port for Internet connectivity check
   *
   * The port to use when checking connectivity to internetCheckHost.
   * Default: 53 (DNS port)
   */
  uint16_t internetCheckPort = 53;

  /**
   * @brief Timeout for Internet connectivity check in milliseconds
   *
   * Maximum time to wait for a response from the Internet check host.
   * Default: 5000ms (5 seconds)
   */
  uint32_t internetCheckTimeout = 5000;

  // ============================================
  // Message Handling
  // ============================================

  /**
   * @brief Number of retry attempts for message delivery
   *
   * How many times to retry sending a message before considering it failed.
   * Default: 3
   */
  uint8_t messageRetryCount = 3;

  /**
   * @brief Interval between retry attempts in milliseconds
   *
   * Base delay between message retry attempts.
   * Actual delay may use exponential backoff.
   * Default: 1000ms (1 second)
   */
  uint32_t retryInterval = 1000;

  /**
   * @brief Timeout for tracking duplicate messages in milliseconds
   *
   * How long to remember message IDs to prevent duplicate processing.
   * Default: 60000ms (60 seconds)
   */
  uint32_t duplicateTrackingTimeout = 60000;

  /**
   * @brief Maximum number of messages to track for deduplication
   *
   * The maximum number of message IDs to store for duplicate detection.
   * Older entries are removed when this limit is reached.
   * Default: 500
   */
  uint16_t maxTrackedMessages = 500;

  // ============================================
  // Gateway Coordination
  // ============================================

  /**
   * @brief Interval for broadcasting gateway heartbeat in milliseconds
   *
   * How often the gateway broadcasts its status to the mesh.
   * Other nodes use this to detect gateway health.
   * Default: 15000ms (15 seconds)
   */
  uint32_t gatewayHeartbeatInterval = 15000;

  /**
   * @brief Timeout for detecting gateway failure in milliseconds
   *
   * If no heartbeat is received within this period, the gateway is
   * considered failed and election may begin.
   * Should be at least 2x gatewayHeartbeatInterval.
   * Default: 45000ms (45 seconds)
   */
  uint32_t gatewayFailureTimeout = 45000;

  /**
   * @brief Whether this node should participate in gateway elections
   *
   * When true, this node may become a gateway if the current gateway fails.
   * Requires routerSSID and routerPassword to be set.
   * Default: true
   */
  bool participateInElection = true;

  // ============================================
  // Advanced Configuration
  // ============================================

  /**
   * @brief Priority for relayed messages (0 = highest/CRITICAL)
   *
   * The priority level assigned to messages being relayed through the gateway.
   * Lower values = higher priority.
   * Default: 0 (CRITICAL priority)
   */
  uint8_t relayedMessagePriority = 0;

  /**
   * @brief Whether to maintain a permanent connection to the router
   *
   * When true, the gateway maintains a continuous connection to the router.
   * When false, the gateway may disconnect when idle to save power.
   * Default: true
   */
  bool maintainPermanentConnection = true;

  // ============================================
  // Validation Methods
  // ============================================

  /**
   * @brief Validate the configuration
   *
   * Performs comprehensive validation of all configuration fields.
   * Returns a ValidationResult with detailed error information.
   *
   * @return ValidationResult indicating validity and any error messages
   */
  ValidationResult validate() const {
    // If not enabled, configuration is valid (nothing to validate)
    if (!enabled) {
      return ValidationResult(true, "");
    }

    // Router SSID is required when enabled
    if (routerSSID.length() == 0) {
      return ValidationResult(false, "routerSSID is required when enabled");
    }

    // SSID length validation (max 32 characters per WiFi spec)
    if (routerSSID.length() > 32) {
      return ValidationResult(false,
                              "routerSSID exceeds maximum length of 32 characters");
    }

    // Password length validation (WPA2 max is 63 characters)
    if (routerPassword.length() > 63) {
      return ValidationResult(
          false, "routerPassword exceeds maximum length of 63 characters");
    }

    // Internet check host validation
    if (internetCheckHost.length() == 0) {
      return ValidationResult(false, "internetCheckHost cannot be empty");
    }

    // Interval validations (minimum sensible values)
    if (internetCheckInterval < 1000) {
      return ValidationResult(
          false, "internetCheckInterval must be at least 1000ms");
    }

    if (internetCheckTimeout < 100) {
      return ValidationResult(false,
                              "internetCheckTimeout must be at least 100ms");
    }

    if (internetCheckTimeout >= internetCheckInterval) {
      return ValidationResult(
          false,
          "internetCheckTimeout must be less than internetCheckInterval");
    }

    if (gatewayHeartbeatInterval < 1000) {
      return ValidationResult(
          false, "gatewayHeartbeatInterval must be at least 1000ms");
    }

    if (gatewayFailureTimeout < gatewayHeartbeatInterval * 2) {
      return ValidationResult(
          false,
          "gatewayFailureTimeout should be at least 2x gatewayHeartbeatInterval");
    }

    if (duplicateTrackingTimeout < 1000) {
      return ValidationResult(
          false, "duplicateTrackingTimeout must be at least 1000ms");
    }

    if (maxTrackedMessages < 10) {
      return ValidationResult(false, "maxTrackedMessages must be at least 10");
    }

    if (retryInterval < 100) {
      return ValidationResult(false, "retryInterval must be at least 100ms");
    }

    return ValidationResult(true, "");
  }

  /**
   * @brief Check if this node can participate in gateway elections
   *
   * A node can participate if:
   * - participateInElection is true
   * - routerSSID is configured
   *
   * @return true if eligible to become a gateway
   */
  bool canParticipateInElection() const {
    return participateInElection && routerSSID.length() > 0;
  }

  /**
   * @brief Check if the configuration has valid router credentials
   *
   * @return true if router SSID is set
   */
  bool hasRouterCredentials() const { return routerSSID.length() > 0; }

  /**
   * @brief Get the estimated memory footprint in bytes
   *
   * Returns an estimate of the memory used by this configuration instance.
   * Useful for monitoring memory usage on constrained devices.
   *
   * @return Estimated memory usage in bytes
   */
  size_t estimatedMemoryFootprint() const {
    size_t baseSize = sizeof(SharedGatewayConfig);
    // Add dynamic string content (not included in sizeof)
    baseSize += routerSSID.length();
    baseSize += routerPassword.length();
    baseSize += internetCheckHost.length();
    return baseSize;
  }
};

}  // namespace gateway
}  // namespace painlessmesh

#endif  // _PAINLESS_MESH_GATEWAY_HPP_
