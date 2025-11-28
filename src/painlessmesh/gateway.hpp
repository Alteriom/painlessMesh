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
#include "painlessmesh/message_tracker.hpp"
#include "painlessmesh/plugin.hpp"
#include "painlessmesh/protocol.hpp"

#include <functional>
#include <map>

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

/**
 * @brief Structure to hold Internet connectivity check results
 *
 * Contains detailed information about the last Internet connectivity check,
 * including whether it succeeded, timing information, and error details.
 */
struct InternetStatus {
  bool available = false;          ///< Whether Internet is currently available
  uint32_t lastCheckTime = 0;      ///< Timestamp of last check (millis)
  uint32_t lastSuccessTime = 0;    ///< Timestamp of last successful check (millis)
  uint32_t checkCount = 0;         ///< Total number of checks performed
  uint32_t successCount = 0;       ///< Number of successful checks
  uint32_t failureCount = 0;       ///< Number of failed checks
  uint32_t lastLatencyMs = 0;      ///< Latency of last successful check in ms
  TSTRING lastError = "";          ///< Error message from last failed check
  TSTRING checkHost = "";          ///< Host used for connectivity check
  uint16_t checkPort = 0;          ///< Port used for connectivity check

  /**
   * @brief Get the uptime percentage of Internet connectivity
   * @return Percentage of successful checks (0-100), or 0 if no checks performed
   */
  uint8_t getUptimePercent() const {
    if (checkCount == 0) return 0;
    return static_cast<uint8_t>((successCount * 100) / checkCount);
  }

  /**
   * @brief Get time since last successful Internet check
   * @return Milliseconds since last success, or UINT32_MAX if never succeeded
   */
  uint32_t getTimeSinceLastSuccess() const {
    if (lastSuccessTime == 0) return UINT32_MAX;
    return millis() - lastSuccessTime;
  }

  /**
   * @brief Check if Internet status is stale (no recent check)
   * @param maxAgeMs Maximum age in milliseconds (default: 60000)
   * @return true if last check was too long ago
   */
  bool isStale(uint32_t maxAgeMs = 60000) const {
    if (lastCheckTime == 0) return true;
    return (millis() - lastCheckTime) > maxAgeMs;
  }
};

/**
 * @brief Callback type for Internet connectivity change events
 */
typedef std::function<void(bool available)> InternetChangedCallback_t;

/**
 * @brief Internet Health Checker class for periodic connectivity monitoring
 *
 * This class performs periodic TCP connection tests to verify Internet
 * connectivity. It is designed to be non-blocking and work with the
 * TaskScheduler for asynchronous operation.
 *
 * PLATFORM SUPPORT:
 * - ESP32/ESP8266: Uses WiFiClient for actual TCP connections
 * - PC/Test: Mocked connectivity (always fails in test environment)
 *
 * Example usage:
 * @code
 * InternetHealthChecker checker;
 * checker.setConfig(gatewayConfig);
 * checker.onConnectivityChanged([](bool available) {
 *   Serial.printf("Internet %s\n", available ? "connected" : "disconnected");
 * });
 * checker.start(scheduler);
 * @endcode
 */
class InternetHealthChecker {
 public:
  InternetHealthChecker() = default;

  /**
   * @brief Configure the health checker with gateway settings
   * @param config SharedGatewayConfig with check parameters
   */
  void setConfig(const SharedGatewayConfig& config) {
    checkHost_ = config.internetCheckHost;
    checkPort_ = config.internetCheckPort;
    checkInterval_ = config.internetCheckInterval;
    checkTimeout_ = config.internetCheckTimeout;
    status_.checkHost = checkHost_;
    status_.checkPort = checkPort_;
  }

  /**
   * @brief Set custom check host and port
   * @param host Host to check (IP address or hostname)
   * @param port Port to connect to (default: 53 for DNS)
   */
  void setCheckTarget(const TSTRING& host, uint16_t port = 53) {
    checkHost_ = host;
    checkPort_ = port;
    status_.checkHost = checkHost_;
    status_.checkPort = checkPort_;
  }

  /**
   * @brief Set check interval
   * @param intervalMs Interval between checks in milliseconds
   */
  void setCheckInterval(uint32_t intervalMs) {
    checkInterval_ = intervalMs;
  }

  /**
   * @brief Set check timeout
   * @param timeoutMs Timeout for each check in milliseconds
   */
  void setCheckTimeout(uint32_t timeoutMs) {
    checkTimeout_ = timeoutMs;
  }

  /**
   * @brief Register callback for connectivity changes
   * @param callback Function to call when connectivity status changes
   */
  void onConnectivityChanged(InternetChangedCallback_t callback) {
    connectivityChangedCallback_ = callback;
  }

  /**
   * @brief Check if local Internet is currently available
   * @return true if last check succeeded
   */
  bool hasLocalInternet() const {
    return status_.available;
  }

  /**
   * @brief Get detailed Internet status
   * @return InternetStatus structure with full details
   */
  InternetStatus getStatus() const {
    return status_;
  }

  /**
   * @brief Perform an immediate Internet connectivity check
   *
   * This method performs a synchronous TCP connection test.
   * On ESP32/ESP8266, it uses WiFiClient.
   * In test environment, connectivity is mocked.
   *
   * @return true if connection succeeded
   */
  bool checkNow() {
    status_.checkCount++;
    status_.lastCheckTime = millis();

    bool connected = performTcpCheck();

    if (connected) {
      status_.successCount++;
      status_.lastSuccessTime = millis();
      status_.lastError = "";
    } else {
      status_.failureCount++;
    }

    // Detect status change and fire callback
    if (connected != status_.available) {
      status_.available = connected;
      if (connectivityChangedCallback_) {
        connectivityChangedCallback_(connected);
      }
    }

    return connected;
  }

  /**
   * @brief Get check interval
   * @return Check interval in milliseconds
   */
  uint32_t getCheckInterval() const {
    return checkInterval_;
  }

  /**
   * @brief Get check timeout
   * @return Check timeout in milliseconds
   */
  uint32_t getCheckTimeout() const {
    return checkTimeout_;
  }

  /**
   * @brief Get check host
   * @return Host being checked
   */
  TSTRING getCheckHost() const {
    return checkHost_;
  }

  /**
   * @brief Get check port
   * @return Port being checked
   */
  uint16_t getCheckPort() const {
    return checkPort_;
  }

  /**
   * @brief Reset all statistics
   */
  void resetStats() {
    status_.checkCount = 0;
    status_.successCount = 0;
    status_.failureCount = 0;
    status_.lastCheckTime = 0;
    status_.lastSuccessTime = 0;
    status_.lastLatencyMs = 0;
    status_.lastError = "";
  }

#ifdef PAINLESSMESH_BOOST
  /**
   * @brief Set mock connectivity result (test environment only)
   * @param connected Whether to simulate connected state
   */
  void setMockConnected(bool connected) {
    mockConnected_ = connected;
  }
#endif

 private:
  /**
   * @brief Perform the actual TCP connection check
   *
   * Platform-specific implementation:
   * - ESP32/ESP8266: Uses WiFiClient to connect
   * - Test/PC: Returns mock value
   *
   * @return true if connection succeeded
   */
  bool performTcpCheck() {
#ifdef PAINLESSMESH_BOOST
    // Test environment - use mock value
    if (mockConnected_) {
      status_.lastLatencyMs = 10;  // Simulated latency
      return true;
    }
    status_.lastError = "Mock: No Internet in test environment";
    return false;
#else
    // Arduino/ESP environment - actual TCP check
    // Note: WiFiClient usage is handled in the arduino-specific code
    // This base implementation returns false; override in wifi.hpp
    status_.lastError = "Not implemented in base class";
    return false;
#endif
  }

  // Configuration
  TSTRING checkHost_ = "8.8.8.8";
  uint16_t checkPort_ = 53;
  uint32_t checkInterval_ = 30000;
  uint32_t checkTimeout_ = 5000;

  // State
  InternetStatus status_;
  InternetChangedCallback_t connectivityChangedCallback_;

#ifdef PAINLESSMESH_BOOST
  bool mockConnected_ = false;
#endif
};

/**
 * @brief Priority levels for GatewayDataPackage messages
 *
 * Defines the priority levels for message routing through the gateway.
 * Lower values indicate higher priority.
 *
 * @note Uses PRIORITY_ prefix to avoid conflicts with Arduino macros
 *       (HIGH and LOW are defined in esp32-hal-gpio.h)
 */
enum class GatewayPriority : uint8_t {
  PRIORITY_CRITICAL = 0,  ///< Critical messages - immediate processing
  PRIORITY_HIGH = 1,      ///< High priority - processed before normal
  PRIORITY_NORMAL = 2,    ///< Normal priority - standard processing
  PRIORITY_LOW = 3        ///< Low priority - processed when idle
};

/**
 * @brief Gateway Data Package for routing Internet requests through mesh
 *
 * This package enables mesh nodes to send data through a gateway node to
 * the Internet. It provides a standardized format for:
 * - HTTP requests to external APIs
 * - MQTT message publishing
 * - WebSocket communications
 * - Any other Internet-bound data
 *
 * MEMORY FOOTPRINT
 * ================
 * The GatewayDataPackage structure has an estimated memory footprint of:
 * - Base fields (from SinglePackage): ~20 bytes
 * - Fixed fields (messageId, originNode, timestamp, priority, retryCount,
 *   requiresAck): ~18 bytes
 * - TSTRING fields (destination, payload, contentType):
 *   - ESP8266/ESP32 String: ~12 bytes overhead per String + content length
 *   - PC/Test std::string: ~32 bytes overhead per string + content length
 * - Total estimated minimum: ~74 bytes (ESP) to ~134 bytes (PC/Test)
 * - With typical content: ~200-500 bytes depending on payload size
 *
 * For ESP8266 with ~80KB RAM, keep payload under 1KB for safety.
 * For ESP32 with ~320KB RAM, larger payloads are acceptable.
 *
 * MESSAGE ID GENERATION
 * =====================
 * Use generateMessageId(nodeId) to create unique message IDs.
 * The ID combines a per-node counter with the node ID to ensure
 * uniqueness across the mesh network.
 *
 * Example usage:
 * @code
 * GatewayDataPackage pkg;
 * pkg.messageId = GatewayDataPackage::generateMessageId(mesh.getNodeId());
 * pkg.originNode = mesh.getNodeId();
 * pkg.timestamp = mesh.getNodeTime();
 * pkg.priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_NORMAL);
 * pkg.destination = "https://api.example.com/data";
 * pkg.payload = "{\"sensor\": 42}";
 * pkg.contentType = "application/json";
 * pkg.requiresAck = true;
 *
 * mesh.sendPackage(&pkg);
 * @endcode
 *
 * Type ID: 620 (GATEWAY_DATA)
 * Base class: SinglePackage (routed to specific gateway node)
 */
class GatewayDataPackage : public plugin::SinglePackage {
 public:
  /**
   * @brief Unique message identifier
   *
   * Generated using generateMessageId() to ensure uniqueness across the mesh.
   * Used for tracking, acknowledgment, and deduplication.
   */
  uint32_t messageId = 0;

  /**
   * @brief Node ID that originated this message
   *
   * The node that created the message, which may differ from the
   * 'from' field during relay operations.
   */
  uint32_t originNode = 0;

  /**
   * @brief Creation timestamp
   *
   * Mesh time when the message was created.
   * Used for TTL calculations and ordering.
   */
  uint32_t timestamp = 0;

  /**
   * @brief Message priority (0=CRITICAL, 1=HIGH, 2=NORMAL, 3=LOW)
   *
   * Determines processing order at the gateway.
   * Use GatewayPriority enum for type-safe values.
   */
  uint8_t priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_NORMAL);

  /**
   * @brief Destination URL or endpoint
   *
   * The Internet destination for this data. Examples:
   * - "https://api.example.com/sensor"
   * - "mqtt://broker.example.com/topic"
   * - "wss://ws.example.com/stream"
   */
  TSTRING destination = "";

  /**
   * @brief Application payload data
   *
   * The actual data to send to the destination.
   * Format depends on contentType (JSON, binary, etc.).
   */
  TSTRING payload = "";

  /**
   * @brief MIME content type
   *
   * Describes the format of the payload. Common values:
   * - "application/json"
   * - "text/plain"
   * - "application/octet-stream"
   */
  TSTRING contentType = "application/json";

  /**
   * @brief Number of relay attempts
   *
   * Incremented each time the message is relayed.
   * Can be used for hop counting and loop detection.
   */
  uint8_t retryCount = 0;

  /**
   * @brief Whether acknowledgment is required
   *
   * When true, the gateway should send a response back
   * confirming successful delivery to the Internet destination.
   */
  bool requiresAck = false;

  /**
   * @brief Number of additional JSON fields in this package
   *
   * Used for jsonObjectSize() calculation in ArduinoJson v6.
   * Count: msgId, origin, ts, prio, dest_url, payload, content, retry, ack = 9 fields
   */
  static constexpr int numPackageFields = 9;

  /**
   * @brief Default constructor
   *
   * Creates a GatewayDataPackage with type ID 620 (GATEWAY_DATA).
   */
  GatewayDataPackage() : SinglePackage(protocol::GATEWAY_DATA) {}

  /**
   * @brief Construct from JSON object
   *
   * Deserializes a GatewayDataPackage from a JSON object.
   * Compatible with ArduinoJson v6 and v7.
   *
   * @param jsonObj JSON object containing package data
   */
  GatewayDataPackage(JsonObject jsonObj) : SinglePackage(jsonObj) {
    messageId = jsonObj["msgId"];
    originNode = jsonObj["origin"];
    timestamp = jsonObj["ts"];
    priority = jsonObj["prio"];
    retryCount = jsonObj["retry"];
    requiresAck = jsonObj["ack"] | false;

#if ARDUINOJSON_VERSION_MAJOR < 7
    if (jsonObj.containsKey("dest_url"))
      destination = jsonObj["dest_url"].as<TSTRING>();
    if (jsonObj.containsKey("payload"))
      payload = jsonObj["payload"].as<TSTRING>();
    if (jsonObj.containsKey("content"))
      contentType = jsonObj["content"].as<TSTRING>();
#else
    if (jsonObj["dest_url"].is<TSTRING>())
      destination = jsonObj["dest_url"].as<TSTRING>();
    if (jsonObj["payload"].is<TSTRING>())
      payload = jsonObj["payload"].as<TSTRING>();
    if (jsonObj["content"].is<TSTRING>())
      contentType = jsonObj["content"].as<TSTRING>();
#endif
  }

  /**
   * @brief Serialize to JSON object
   *
   * Adds all package fields to the provided JSON object.
   *
   * @param jsonObj JSON object to add fields to
   * @return The modified JSON object
   */
  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = SinglePackage::addTo(std::move(jsonObj));
    jsonObj["msgId"] = messageId;
    jsonObj["origin"] = originNode;
    jsonObj["ts"] = timestamp;
    jsonObj["prio"] = priority;
    jsonObj["dest_url"] = destination;
    jsonObj["payload"] = payload;
    jsonObj["content"] = contentType;
    jsonObj["retry"] = retryCount;
    jsonObj["ack"] = requiresAck;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  /**
   * @brief Calculate JSON object size for ArduinoJson v6
   *
   * Used for buffer allocation when serializing.
   *
   * @return Estimated size in bytes
   */
  size_t jsonObjectSize() const {
    // noJsonFields (from base class) + numPackageFields (our fields)
    return JSON_OBJECT_SIZE(noJsonFields + numPackageFields) + destination.length() +
           payload.length() + contentType.length();
  }
#endif

  /**
   * @brief Generate a unique message ID
   *
   * Creates a unique message ID by combining a per-node counter
   * with the node ID. This ensures uniqueness across the mesh
   * even if multiple nodes generate IDs simultaneously.
   *
   * The ID format is:
   * - Upper 16 bits: Lower 16 bits of node ID
   * - Lower 16 bits: Incrementing counter (wraps at 65535)
   *
   * @note This function is not thread-safe. On ESP8266/ESP32, this is
   * acceptable as the main loop is single-threaded. For multi-threaded
   * environments, consider using atomic operations.
   *
   * @param nodeId The ID of the node generating the message
   * @return A unique message ID
   */
  static uint32_t generateMessageId(uint32_t nodeId) {
    static uint16_t counter = 0;
    ++counter;
    // Combine node ID (upper 16 bits) with counter (lower 16 bits)
    return ((nodeId & 0xFFFF) << 16) | counter;
  }

  /**
   * @brief Get the estimated memory footprint
   *
   * Returns an estimate of the memory used by this package instance.
   *
   * @return Estimated memory usage in bytes
   */
  size_t estimatedMemoryFootprint() const {
    size_t baseSize = sizeof(GatewayDataPackage);
    // Add dynamic string content (not included in sizeof)
    baseSize += destination.length();
    baseSize += payload.length();
    baseSize += contentType.length();
    return baseSize;
  }
};

/**
 * @brief Gateway Acknowledgment Package for delivery confirmations
 *
 * This package is sent from the gateway back to the origin node to confirm
 * delivery status of a GatewayDataPackage. It provides feedback on whether
 * the data was successfully delivered to the Internet destination.
 *
 * MEMORY FOOTPRINT
 * ================
 * The GatewayAckPackage structure has an estimated memory footprint of:
 * - Base fields (from SinglePackage): ~20 bytes
 * - Fixed fields (messageId, originNode, success, httpStatus, timestamp): ~14 bytes
 * - TSTRING field (error):
 *   - ESP8266/ESP32 String: ~12 bytes overhead + content length
 *   - PC/Test std::string: ~32 bytes overhead + content length
 * - Total estimated minimum: ~46 bytes (ESP) to ~66 bytes (PC/Test)
 * - With typical error message: ~100-200 bytes
 *
 * For ESP8266 with ~80KB RAM, this represents <0.3% of available memory.
 * For ESP32 with ~320KB RAM, this represents <0.1% of available memory.
 *
 * Example usage:
 * @code
 * // Gateway responding to a successful delivery
 * GatewayAckPackage ack;
 * ack.messageId = originalPackage.messageId;
 * ack.originNode = originalPackage.originNode;
 * ack.dest = originalPackage.originNode;  // Route back to origin
 * ack.from = mesh.getNodeId();
 * ack.success = true;
 * ack.httpStatus = 200;
 * ack.timestamp = mesh.getNodeTime();
 *
 * mesh.sendPackage(&ack);
 *
 * // Gateway responding to a failed delivery
 * GatewayAckPackage ack;
 * ack.messageId = originalPackage.messageId;
 * ack.originNode = originalPackage.originNode;
 * ack.dest = originalPackage.originNode;
 * ack.from = mesh.getNodeId();
 * ack.success = false;
 * ack.httpStatus = 503;
 * ack.error = "Service unavailable";
 * ack.timestamp = mesh.getNodeTime();
 *
 * mesh.sendPackage(&ack);
 * @endcode
 *
 * Type ID: 621 (GATEWAY_ACK)
 * Base class: SinglePackage (routed back to origin node)
 */
class GatewayAckPackage : public plugin::SinglePackage {
 public:
  /**
   * @brief Original message ID being acknowledged
   *
   * The messageId from the GatewayDataPackage that this acknowledgment
   * corresponds to. Used for correlation at the origin node.
   */
  uint32_t messageId = 0;

  /**
   * @brief Original sender node ID
   *
   * The node ID that originally sent the GatewayDataPackage.
   * Used for routing and correlation.
   */
  uint32_t originNode = 0;

  /**
   * @brief Delivery success status
   *
   * True if the message was successfully delivered to the Internet
   * destination, false otherwise.
   */
  bool success = false;

  /**
   * @brief HTTP response code (if applicable)
   *
   * The HTTP status code received from the Internet destination.
   * Examples: 200 (OK), 404 (Not Found), 500 (Server Error).
   * Set to 0 if not applicable (e.g., connection failure).
   */
  uint16_t httpStatus = 0;

  /**
   * @brief Error message (if failed)
   *
   * A human-readable error message describing why delivery failed.
   * Empty string if success is true.
   */
  TSTRING error = "";

  /**
   * @brief Acknowledgment timestamp
   *
   * Mesh time when the acknowledgment was created.
   * Can be used to calculate round-trip time.
   */
  uint32_t timestamp = 0;

  /**
   * @brief Number of additional JSON fields in this package
   *
   * Used for jsonObjectSize() calculation in ArduinoJson v6.
   * Count: msgId, origin, success, http, err, ts = 6 fields
   */
  static constexpr int numPackageFields = 6;

  /**
   * @brief Default constructor
   *
   * Creates a GatewayAckPackage with type ID 621 (GATEWAY_ACK).
   */
  GatewayAckPackage() : SinglePackage(protocol::GATEWAY_ACK) {}

  /**
   * @brief Construct from JSON object
   *
   * Deserializes a GatewayAckPackage from a JSON object.
   * Compatible with ArduinoJson v6 and v7.
   *
   * @param jsonObj JSON object containing package data
   */
  GatewayAckPackage(JsonObject jsonObj) : SinglePackage(jsonObj) {
    messageId = jsonObj["msgId"];
    originNode = jsonObj["origin"];
    success = jsonObj["success"] | false;
    httpStatus = jsonObj["http"];
    timestamp = jsonObj["ts"];

#if ARDUINOJSON_VERSION_MAJOR < 7
    if (jsonObj.containsKey("err"))
      error = jsonObj["err"].as<TSTRING>();
#else
    if (jsonObj["err"].is<TSTRING>())
      error = jsonObj["err"].as<TSTRING>();
#endif
  }

  /**
   * @brief Serialize to JSON object
   *
   * Adds all package fields to the provided JSON object.
   *
   * @param jsonObj JSON object to add fields to
   * @return The modified JSON object
   */
  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = SinglePackage::addTo(std::move(jsonObj));
    jsonObj["msgId"] = messageId;
    jsonObj["origin"] = originNode;
    jsonObj["success"] = success;
    jsonObj["http"] = httpStatus;
    jsonObj["err"] = error;
    jsonObj["ts"] = timestamp;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  /**
   * @brief Calculate JSON object size for ArduinoJson v6
   *
   * Used for buffer allocation when serializing.
   *
   * @return Estimated size in bytes
   */
  size_t jsonObjectSize() const {
    // noJsonFields (from base class) + numPackageFields (our fields)
    return JSON_OBJECT_SIZE(noJsonFields + numPackageFields) + error.length();
  }
#endif

  /**
   * @brief Get the estimated memory footprint
   *
   * Returns an estimate of the memory used by this package instance.
   *
   * @return Estimated memory usage in bytes
   */
  size_t estimatedMemoryFootprint() const {
    size_t baseSize = sizeof(GatewayAckPackage);
    // Add dynamic string content (not included in sizeof)
    baseSize += error.length();
    return baseSize;
  }
};

/**
 * @brief Gateway Heartbeat Package for primary gateway health monitoring
 *
 * This package is broadcast periodically by the primary gateway to inform all
 * nodes in the mesh about the gateway's health status. It provides essential
 * information for:
 * - Gateway availability monitoring
 * - Internet connectivity status
 * - Gateway election decision-making
 * - Failover detection
 *
 * BROADCAST INTERVAL
 * ==================
 * The primary gateway should broadcast heartbeats at the interval configured
 * in SharedGatewayConfig::gatewayHeartbeatInterval (default: 15 seconds).
 *
 * TIMEOUT DETECTION
 * =================
 * If no heartbeat is received within SharedGatewayConfig::gatewayFailureTimeout
 * (default: 45 seconds), the gateway should be considered failed and election
 * may begin.
 *
 * MEMORY FOOTPRINT
 * ================
 * The GatewayHeartbeatPackage structure has an estimated memory footprint of:
 * - Base fields (from BroadcastPackage): ~16 bytes
 * - Fixed fields (isPrimary, hasInternet, routerRSSI, uptime, timestamp): ~11 bytes
 * - Total estimated: ~27 bytes
 *
 * For ESP8266 with ~80KB RAM, this represents <0.1% of available memory.
 * For ESP32 with ~320KB RAM, this represents <0.01% of available memory.
 *
 * Example usage:
 * @code
 * // Primary gateway broadcasting heartbeat
 * GatewayHeartbeatPackage heartbeat;
 * heartbeat.from = mesh.getNodeId();
 * heartbeat.isPrimary = true;
 * heartbeat.hasInternet = internetChecker.hasLocalInternet();
 * heartbeat.routerRSSI = WiFi.RSSI();
 * heartbeat.uptime = millis() / 1000;  // Convert to seconds
 * heartbeat.timestamp = mesh.getNodeTime();
 *
 * mesh.sendPackage(&heartbeat);
 *
 * // Node receiving heartbeat
 * void onHeartbeat(GatewayHeartbeatPackage& heartbeat) {
 *     lastGatewayHeartbeat = millis();
 *     primaryGatewayId = heartbeat.from;
 *     hasGatewayInternet = heartbeat.hasInternet;
 * }
 * @endcode
 *
 * Type ID: 622 (GATEWAY_HEARTBEAT)
 * Base class: BroadcastPackage (sent to all nodes in mesh)
 */
class GatewayHeartbeatPackage : public plugin::BroadcastPackage {
 public:
  /**
   * @brief Indicates if this is the primary gateway
   *
   * True if the node sending this heartbeat is the currently elected
   * primary gateway. Secondary/standby gateways may also send heartbeats
   * with isPrimary = false for coordination purposes.
   */
  bool isPrimary = false;

  /**
   * @brief Indicates if this gateway has Internet connectivity
   *
   * True if the gateway can currently reach the Internet.
   * Used by other nodes to determine if data can be routed
   * to Internet services through this gateway.
   */
  bool hasInternet = false;

  /**
   * @brief Signal strength to the router in dBm
   *
   * The RSSI value indicating signal quality to the external WiFi router.
   * Range: typically -90 (weak) to -30 (strong) dBm.
   * Used in gateway election to prefer gateways with better connections.
   * Set to 0 if not connected to a router.
   */
  int8_t routerRSSI = 0;

  /**
   * @brief Gateway uptime in seconds
   *
   * How long this gateway has been running, in seconds.
   * Used in election tiebreakers - longer uptime indicates stability.
   */
  uint32_t uptime = 0;

  /**
   * @brief Heartbeat timestamp
   *
   * Mesh time when this heartbeat was generated.
   * Used for calculating latency and detecting stale heartbeats.
   */
  uint32_t timestamp = 0;

  /**
   * @brief Number of additional JSON fields in this package
   *
   * Used for jsonObjectSize() calculation in ArduinoJson v6.
   * Count: primary, internet, rssi, uptime, ts = 5 fields
   */
  static constexpr int numPackageFields = 5;

  /**
   * @brief Default constructor
   *
   * Creates a GatewayHeartbeatPackage with type ID 622 (GATEWAY_HEARTBEAT).
   */
  GatewayHeartbeatPackage() : BroadcastPackage(protocol::GATEWAY_HEARTBEAT) {}

  /**
   * @brief Construct from JSON object
   *
   * Deserializes a GatewayHeartbeatPackage from a JSON object.
   * Compatible with ArduinoJson v6 and v7.
   *
   * @param jsonObj JSON object containing package data
   */
  GatewayHeartbeatPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    isPrimary = jsonObj["primary"] | false;
    hasInternet = jsonObj["internet"] | false;
    routerRSSI = jsonObj["rssi"] | 0;
    uptime = jsonObj["uptime"] | 0;
    timestamp = jsonObj["ts"] | 0;
  }

  /**
   * @brief Serialize to JSON object
   *
   * Adds all package fields to the provided JSON object.
   *
   * @param jsonObj JSON object to add fields to
   * @return The modified JSON object
   */
  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["primary"] = isPrimary;
    jsonObj["internet"] = hasInternet;
    jsonObj["rssi"] = routerRSSI;
    jsonObj["uptime"] = uptime;
    jsonObj["ts"] = timestamp;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  /**
   * @brief Calculate JSON object size for ArduinoJson v6
   *
   * Used for buffer allocation when serializing.
   *
   * @return Estimated size in bytes
   */
  size_t jsonObjectSize() const {
    // noJsonFields (from base class) + numPackageFields (our fields)
    return JSON_OBJECT_SIZE(noJsonFields + numPackageFields);
  }
#endif

  /**
   * @brief Get the estimated memory footprint
   *
   * Returns an estimate of the memory used by this package instance.
   *
   * @return Estimated memory usage in bytes
   */
  size_t estimatedMemoryFootprint() const {
    // No dynamic strings, so sizeof gives accurate footprint
    return sizeof(GatewayHeartbeatPackage);
  }

  /**
   * @brief Check if this heartbeat indicates a healthy gateway
   *
   * A gateway is considered healthy if it is primary and has Internet access.
   *
   * @return true if the gateway is healthy
   */
  bool isHealthy() const {
    return isPrimary && hasInternet;
  }

  /**
   * @brief Check if the router signal strength is acceptable
   *
   * Signal strength is considered acceptable if RSSI is better than -70 dBm.
   * This is a typical threshold for reliable WiFi connectivity.
   *
   * @return true if signal strength is acceptable
   */
  bool hasAcceptableSignal() const {
    // RSSI of 0 typically means not connected
    // RSSI better than -70 dBm is considered acceptable
    return routerRSSI != 0 && routerRSSI > -70;
  }
};

/**
 * @brief Gateway Election Manager for coordinating primary gateway selection
 *
 * This class implements a deterministic election protocol for selecting a
 * primary gateway in the mesh network. It provides:
 * - Primary gateway failure detection via heartbeat monitoring
 * - Deterministic winner selection (highest RSSI, then highest node ID)
 * - Split-brain prevention during elections
 * - Cooldown period to prevent election thrashing
 *
 * ELECTION STATE MACHINE
 * ======================
 * The manager operates in three states:
 * 1. IDLE: Monitoring heartbeats, no election in progress
 * 2. ELECTION_RUNNING: Election triggered, collecting candidates
 * 3. COOLDOWN: Post-election cooldown to prevent rapid re-elections
 *
 * ELECTION ALGORITHM
 * ==================
 * Only nodes with Internet connectivity (hasInternet == true) can be candidates.
 * Winner is selected by:
 * 1. Highest RSSI wins
 * 2. If RSSI tie, highest node ID wins
 *
 * This ensures deterministic, consistent winner selection across all nodes.
 *
 * MEMORY FOOTPRINT
 * ================
 * The GatewayElectionManager has an estimated memory footprint of:
 * - Fixed fields: ~60 bytes
 * - Candidate map: ~40 bytes per candidate + map overhead
 * - Total estimated: ~200-500 bytes depending on candidate count
 *
 * Example usage:
 * @code
 * GatewayElectionManager election;
 * election.configure(gatewayConfig);
 * election.setNodeId(mesh.getNodeId());
 * election.setLocalCandidate(internetChecker.hasLocalInternet(), WiFi.RSSI());
 *
 * // In heartbeat callback
 * void onHeartbeat(GatewayHeartbeatPackage& heartbeat) {
 *     election.processHeartbeat(heartbeat);
 * }
 *
 * // In update loop (call periodically)
 * if (election.update(millis())) {
 *     // This node won the election, broadcast as primary
 *     broadcastPrimaryHeartbeat();
 * }
 *
 * // Register callback for election results
 * election.onElectionResult([](uint32_t winnerId, bool isLocal) {
 *     Serial.printf("Election winner: %u (local: %s)\n",
 *                   winnerId, isLocal ? "yes" : "no");
 * });
 * @endcode
 */
class GatewayElectionManager {
 public:
  /**
   * @brief Election state machine states
   */
  enum class ElectionState {
    IDLE,             ///< Monitoring heartbeats, no election in progress
    ELECTION_RUNNING, ///< Election triggered, collecting candidates
    COOLDOWN          ///< Post-election cooldown period
  };

  /**
   * @brief Callback type for election result notifications
   * @param winnerId The node ID of the election winner
   * @param isLocalNode True if this node is the winner
   */
  typedef std::function<void(uint32_t winnerId, bool isLocalNode)>
      ElectionResultCallback_t;

  /**
   * @brief Default constructor
   *
   * Creates a GatewayElectionManager with default configuration:
   * - gatewayFailureTimeout: 45000ms
   * - electionDuration: 5000ms
   * - electionCooldownPeriod: 60000ms
   */
  GatewayElectionManager()
      : state_(ElectionState::IDLE),
        nodeId_(0),
        localHasInternet_(false),
        localRssi_(0),
        primaryGatewayId_(0),
        isElectedPrimary_(false),
        lastPrimaryHeartbeatTime_(0),
        electionStartTime_(0),
        cooldownStartTime_(0),
        gatewayFailureTimeout_(45000),
        electionDuration_(5000),
        electionCooldownPeriod_(60000) {}

  /**
   * @brief Configure the election manager using SharedGatewayConfig
   *
   * Updates the failure timeout from the provided configuration.
   *
   * @param config SharedGatewayConfig with timeout parameters
   */
  void configure(const SharedGatewayConfig& config) {
    gatewayFailureTimeout_ = config.gatewayFailureTimeout;
    Log(logger::GENERAL,
        "GatewayElectionManager: Configured with failureTimeout=%ums\n",
        gatewayFailureTimeout_);
  }

  /**
   * @brief Set the local node ID
   *
   * Must be called before the manager can participate in elections.
   *
   * @param nodeId The local node's unique identifier
   */
  void setNodeId(uint32_t nodeId) {
    nodeId_ = nodeId;
    Log(logger::GENERAL, "GatewayElectionManager: Node ID set to %u\n", nodeId_);
  }

  /**
   * @brief Set the local node's election candidacy parameters
   *
   * Updates whether this node can participate in elections based on
   * Internet connectivity and signal strength.
   *
   * @param hasInternet True if this node has Internet connectivity
   * @param rssi Signal strength to the router in dBm
   */
  void setLocalCandidate(bool hasInternet, int8_t rssi) {
    localHasInternet_ = hasInternet;
    localRssi_ = rssi;
  }

  /**
   * @brief Process an incoming gateway heartbeat
   *
   * Updates internal state based on the received heartbeat:
   * - If from primary, updates last heartbeat time
   * - If during election, may defer to higher-priority primary
   *
   * @param heartbeat The received GatewayHeartbeatPackage
   * @param currentTime Current time in milliseconds (e.g., millis())
   */
  void processHeartbeat(const GatewayHeartbeatPackage& heartbeat,
                        uint32_t currentTime) {
    // Track this node as a potential candidate if it has Internet
    if (heartbeat.hasInternet) {
      Candidate candidate;
      candidate.nodeId = heartbeat.from;
      candidate.rssi = heartbeat.routerRSSI;
      candidate.hasInternet = heartbeat.hasInternet;
      candidate.lastSeen = currentTime;
      candidates_[heartbeat.from] = candidate;
    }

    // Handle heartbeat from a node claiming to be primary
    if (heartbeat.isPrimary) {
      // During election, check for split-brain prevention
      if (state_ == ElectionState::ELECTION_RUNNING) {
        // If sender has higher priority (higher RSSI, or same RSSI and higher
        // nodeId), defer to them
        if (shouldDeferTo(heartbeat.routerRSSI, heartbeat.from)) {
          Log(logger::GENERAL,
              "GatewayElectionManager: Deferring to primary node %u "
              "(RSSI=%d)\n",
              heartbeat.from, heartbeat.routerRSSI);
          primaryGatewayId_ = heartbeat.from;
          isElectedPrimary_ = false;
          lastPrimaryHeartbeatTime_ = currentTime;
          transitionToCooldown(currentTime);
          return;
        }
      } else {
        // Not in election, accept this node as primary
        primaryGatewayId_ = heartbeat.from;
        lastPrimaryHeartbeatTime_ = currentTime;

        // If we were primary but another node claims primary with higher
        // priority, step down
        if (isElectedPrimary_ && heartbeat.from != nodeId_) {
          if (shouldDeferTo(heartbeat.routerRSSI, heartbeat.from)) {
            Log(logger::GENERAL,
                "GatewayElectionManager: Stepping down, deferring to node %u\n",
                heartbeat.from);
            isElectedPrimary_ = false;
          }
        }
      }
    }
  }

  /**
   * @brief Process an incoming gateway heartbeat (convenience overload)
   *
   * Calls processHeartbeat(heartbeat, millis()) for backward compatibility.
   * For testing, prefer processHeartbeat(heartbeat, currentTime) for
   * deterministic behavior.
   *
   * @param heartbeat The received GatewayHeartbeatPackage
   */
  void processHeartbeat(const GatewayHeartbeatPackage& heartbeat) {
    processHeartbeat(heartbeat, millis());
  }

  /**
   * @brief Update the election state machine
   *
   * Should be called periodically (e.g., every second) to:
   * - Check for primary gateway timeout
   * - Progress election state machine
   * - Determine election winners
   *
   * @param currentTime Current time in milliseconds (e.g., millis())
   * @return true if this node should broadcast as primary (won election)
   */
  bool update(uint32_t currentTime) {
    bool shouldBroadcastAsPrimary = false;

    switch (state_) {
      case ElectionState::IDLE:
        // Check if primary gateway has timed out
        if (primaryGatewayId_ != 0 && lastPrimaryHeartbeatTime_ != 0) {
          uint32_t timeSinceLastHeartbeat =
              currentTime - lastPrimaryHeartbeatTime_;
          if (timeSinceLastHeartbeat > gatewayFailureTimeout_) {
            Log(logger::GENERAL,
                "GatewayElectionManager: Primary gateway %u timed out "
                "(no heartbeat for %ums)\n",
                primaryGatewayId_, timeSinceLastHeartbeat);
            // Primary failed, start election if we can participate
            if (canParticipateInElection()) {
              startElection(currentTime);
            } else {
              // Can't participate, just clear the primary
              primaryGatewayId_ = 0;
            }
          }
        } else if (primaryGatewayId_ == 0 && canParticipateInElection()) {
          // No known primary and we can participate, start election
          startElection(currentTime);
        }
        break;

      case ElectionState::ELECTION_RUNNING:
        // Check if election duration has elapsed
        if (currentTime - electionStartTime_ >= electionDuration_) {
          // Election complete, select winner
          uint32_t winnerId = selectWinner();
          if (winnerId != 0) {
            primaryGatewayId_ = winnerId;
            isElectedPrimary_ = (winnerId == nodeId_);

            Log(logger::GENERAL,
                "GatewayElectionManager: Election complete, winner=%u "
                "(local=%s)\n",
                winnerId, isElectedPrimary_ ? "yes" : "no");

            // Notify via callback
            if (electionResultCallback_) {
              electionResultCallback_(winnerId, isElectedPrimary_);
            }

            if (isElectedPrimary_) {
              shouldBroadcastAsPrimary = true;
              lastPrimaryHeartbeatTime_ = currentTime;
            }
          } else {
            Log(logger::GENERAL,
                "GatewayElectionManager: Election complete, no valid candidates\n");
            primaryGatewayId_ = 0;
            isElectedPrimary_ = false;
          }

          transitionToCooldown(currentTime);
        }
        break;

      case ElectionState::COOLDOWN:
        // Check if cooldown period has elapsed
        if (currentTime - cooldownStartTime_ >= electionCooldownPeriod_) {
          Log(logger::GENERAL,
              "GatewayElectionManager: Cooldown complete, returning to IDLE\n");
          state_ = ElectionState::IDLE;
        }
        break;
    }

    return shouldBroadcastAsPrimary;
  }

  /**
   * @brief Check if this node is the elected primary gateway
   * @return true if this node won the last election
   */
  bool isElectedPrimary() const { return isElectedPrimary_; }

  /**
   * @brief Get the current election state
   * @return Current ElectionState
   */
  ElectionState getState() const { return state_; }

  /**
   * @brief Get the current primary gateway node ID
   * @return Primary gateway node ID, or 0 if none
   */
  uint32_t getPrimaryGatewayId() const { return primaryGatewayId_; }

  /**
   * @brief Register a callback for election results
   *
   * The callback is invoked when an election completes with the winner's
   * node ID and whether this node is the winner.
   *
   * @param callback Function to call on election completion
   */
  void onElectionResult(ElectionResultCallback_t callback) {
    electionResultCallback_ = callback;
  }

  /**
   * @brief Force start an election
   *
   * Manually triggers an election, useful for testing or when a node
   * needs to force re-election. Only starts if not already in election
   * or cooldown.
   *
   * @param currentTime Current time in milliseconds (e.g., millis())
   */
  void startElection(uint32_t currentTime) {
    if (state_ != ElectionState::IDLE) {
      Log(logger::GENERAL,
          "GatewayElectionManager: Cannot start election, state=%d\n",
          static_cast<int>(state_));
      return;
    }

    Log(logger::GENERAL,
        "GatewayElectionManager: Starting election (nodeId=%u, "
        "hasInternet=%s, rssi=%d)\n",
        nodeId_, localHasInternet_ ? "yes" : "no", localRssi_);

    state_ = ElectionState::ELECTION_RUNNING;
    electionStartTime_ = currentTime;
    isElectedPrimary_ = false;

    // Clear old candidates and add self if eligible
    candidates_.clear();
    if (localHasInternet_ && nodeId_ != 0) {
      Candidate localCandidate;
      localCandidate.nodeId = nodeId_;
      localCandidate.rssi = localRssi_;
      localCandidate.hasInternet = localHasInternet_;
      localCandidate.lastSeen = electionStartTime_;
      candidates_[nodeId_] = localCandidate;
    }
  }

  /**
   * @brief Force start an election (convenience overload)
   *
   * Calls startElection(millis()) for backward compatibility.
   * For testing, prefer startElection(currentTime) for deterministic behavior.
   */
  void startElection() { startElection(millis()); }

  /**
   * @brief Reset all election state
   *
   * Clears all state including primary gateway, candidates, and returns
   * to IDLE state. Does not clear configuration or node ID.
   */
  void reset() {
    state_ = ElectionState::IDLE;
    primaryGatewayId_ = 0;
    isElectedPrimary_ = false;
    lastPrimaryHeartbeatTime_ = 0;
    electionStartTime_ = 0;
    cooldownStartTime_ = 0;
    candidates_.clear();
    Log(logger::GENERAL, "GatewayElectionManager: State reset\n");
  }

  /**
   * @brief Set the election duration
   * @param durationMs Duration in milliseconds (default: 5000)
   */
  void setElectionDuration(uint32_t durationMs) {
    electionDuration_ = durationMs;
  }

  /**
   * @brief Set the cooldown period
   * @param periodMs Cooldown period in milliseconds (default: 60000)
   */
  void setCooldownPeriod(uint32_t periodMs) {
    electionCooldownPeriod_ = periodMs;
  }

  /**
   * @brief Get the number of known candidates
   * @return Number of candidates in the current election
   */
  size_t getCandidateCount() const { return candidates_.size(); }

 private:
  /**
   * @brief Internal structure for tracking election candidates
   */
  struct Candidate {
    uint32_t nodeId = 0;
    int8_t rssi = 0;
    bool hasInternet = false;
    uint32_t lastSeen = 0;
  };

  /**
   * @brief Check if this node can participate in elections
   * @return true if node has Internet and valid node ID
   */
  bool canParticipateInElection() const {
    return localHasInternet_ && nodeId_ != 0;
  }

  /**
   * @brief Check if we should defer to another node
   *
   * Used for split-brain prevention. Defers to nodes with:
   * 1. Higher RSSI, or
   * 2. Same RSSI and higher node ID
   *
   * @param otherRssi The other node's RSSI
   * @param otherNodeId The other node's ID
   * @return true if we should defer to the other node
   */
  bool shouldDeferTo(int8_t otherRssi, uint32_t otherNodeId) const {
    // Higher RSSI wins
    if (otherRssi > localRssi_) return true;
    if (otherRssi < localRssi_) return false;
    // Same RSSI, higher node ID wins
    return otherNodeId > nodeId_;
  }

  /**
   * @brief Select the winner from current candidates
   *
   * Implements deterministic winner selection:
   * 1. Highest RSSI wins
   * 2. If RSSI tie, highest node ID wins
   *
   * @return Winner's node ID, or 0 if no valid candidates
   */
  uint32_t selectWinner() const {
    uint32_t winnerId = 0;
    int8_t winnerRssi = -128;  // Minimum possible RSSI

    for (const auto& pair : candidates_) {
      const Candidate& candidate = pair.second;

      // Only consider candidates with Internet
      if (!candidate.hasInternet) continue;

      // Check if this candidate beats the current winner
      bool isBetter = false;
      if (candidate.rssi > winnerRssi) {
        isBetter = true;
      } else if (candidate.rssi == winnerRssi && candidate.nodeId > winnerId) {
        isBetter = true;
      }

      if (isBetter) {
        winnerId = candidate.nodeId;
        winnerRssi = candidate.rssi;
      }
    }

    return winnerId;
  }

  /**
   * @brief Transition to cooldown state
   * @param currentTime Current time in milliseconds
   */
  void transitionToCooldown(uint32_t currentTime) {
    state_ = ElectionState::COOLDOWN;
    cooldownStartTime_ = currentTime;
    candidates_.clear();
  }

  // State
  ElectionState state_;
  uint32_t nodeId_;
  bool localHasInternet_;
  int8_t localRssi_;
  uint32_t primaryGatewayId_;
  bool isElectedPrimary_;
  uint32_t lastPrimaryHeartbeatTime_;
  uint32_t electionStartTime_;
  uint32_t cooldownStartTime_;

  // Configuration
  uint32_t gatewayFailureTimeout_;
  uint32_t electionDuration_;
  uint32_t electionCooldownPeriod_;

  // Candidates map: nodeId -> Candidate
  std::map<uint32_t, Candidate> candidates_;

  // Callback
  ElectionResultCallback_t electionResultCallback_;
};

/**
 * @brief Metrics for monitoring GatewayMessageHandler duplicate detection
 *
 * This structure tracks statistics about message processing and duplicate
 * detection in the gateway message handler. Useful for monitoring system
 * health and debugging duplicate-related issues.
 *
 * Example usage:
 * @code
 * GatewayMessageHandler handler;
 * // ... process messages ...
 * GatewayMetrics metrics = handler.getMetrics();
 * Serial.printf("Duplicates: %u, Processed: %u\n",
 *               metrics.duplicatesDetected, metrics.messagesProcessed);
 * @endcode
 */
struct GatewayMetrics {
  /**
   * @brief Count of duplicate messages detected and dropped
   *
   * Incremented each time an incoming message is identified as a duplicate
   * and skipped from processing.
   */
  uint32_t duplicatesDetected = 0;

  /**
   * @brief Total count of messages successfully processed
   *
   * Incremented for each unique message that passes duplicate checking
   * and is processed.
   */
  uint32_t messagesProcessed = 0;

  /**
   * @brief Count of acknowledgments sent
   *
   * Incremented each time an acknowledgment is sent for a message.
   */
  uint32_t acknowledgmentsSent = 0;

  /**
   * @brief Count of duplicate acknowledgments skipped
   *
   * Incremented when an acknowledgment would have been sent, but was
   * skipped because one was already sent for that message.
   */
  uint32_t duplicateAcksSkipped = 0;

  /**
   * @brief Reset all metrics to zero
   */
  void reset() {
    duplicatesDetected = 0;
    messagesProcessed = 0;
    acknowledgmentsSent = 0;
    duplicateAcksSkipped = 0;
  }

  /**
   * @brief Get the duplicate detection rate as a percentage
   * @return Percentage of messages that were duplicates (0-100), or 0 if no messages
   */
  uint8_t getDuplicateRate() const {
    uint64_t total = static_cast<uint64_t>(messagesProcessed) +
                     static_cast<uint64_t>(duplicatesDetected);
    if (total == 0) return 0;
    // Use uint64_t arithmetic to prevent overflow
    return static_cast<uint8_t>((static_cast<uint64_t>(duplicatesDetected) * 100) / total);
  }

  /**
   * @brief Get the duplicate ack rate as a percentage
   * @return Percentage of acks that were duplicates (0-100), or 0 if no acks
   */
  uint8_t getDuplicateAckRate() const {
    uint64_t total = static_cast<uint64_t>(acknowledgmentsSent) +
                     static_cast<uint64_t>(duplicateAcksSkipped);
    if (total == 0) return 0;
    // Use uint64_t arithmetic to prevent overflow
    return static_cast<uint8_t>((static_cast<uint64_t>(duplicateAcksSkipped) * 100) / total);
  }
};

}  // namespace gateway

/**
 * @brief Gateway Message Handler with duplicate prevention
 *
 * This class integrates MessageTracker to prevent duplicate message processing
 * in gateway operations. It provides:
 * - Duplicate message detection and dropping
 * - Single acknowledgment per message enforcement
 * - Metrics for monitoring duplicate detection
 * - Configurable tracker limits via SharedGatewayConfig
 * - Logging for duplicate detection events
 *
 * MEMORY FOOTPRINT
 * ================
 * The GatewayMessageHandler uses a MessageTracker internally, which stores
 * message entries in a std::map. Memory usage depends on configuration:
 * - Default (500 messages): ~20KB estimated
 * - ESP8266 recommended (100 messages): ~4KB estimated
 * - Each tracked message: ~40 bytes (MessageKey + TrackedMessage + map overhead)
 *
 * Example usage:
 * @code
 * GatewayMessageHandler handler;
 *
 * // Configure from shared gateway config
 * SharedGatewayConfig config;
 * config.maxTrackedMessages = 500;
 * config.duplicateTrackingTimeout = 60000;
 * handler.configure(config);
 *
 * // Handle incoming message
 * GatewayDataPackage pkg;
 * // ... populate pkg ...
 *
 * if (handler.handleIncomingMessage(pkg)) {
 *     // Process the message - it's not a duplicate
 *     processGatewayRequest(pkg);
 *
 *     // Check if we should send an ack
 *     if (pkg.requiresAck && handler.shouldSendAcknowledgment(pkg.messageId, pkg.originNode)) {
 *         sendAck(pkg);
 *         handler.markAcknowledgmentSent(pkg.messageId, pkg.originNode);
 *     }
 * }
 *
 * // Periodically cleanup old entries
 * handler.cleanup();
 *
 * // Monitor metrics
 * auto metrics = handler.getMetrics();
 * @endcode
 */
class GatewayMessageHandler {
 public:
  /**
   * @brief Default constructor
   *
   * Creates a GatewayMessageHandler with default configuration:
   * - maxTrackedMessages: 500
   * - duplicateTrackingTimeout: 60000ms (60 seconds)
   */
  GatewayMessageHandler() : tracker_(500, 60000) {}

  /**
   * @brief Configure the handler using SharedGatewayConfig parameters
   *
   * Updates the internal MessageTracker with configuration values from
   * the provided SharedGatewayConfig. This should be called before
   * processing messages to ensure proper configuration.
   *
   * @param config SharedGatewayConfig with tracker parameters
   */
  void configure(const gateway::SharedGatewayConfig& config) {
    tracker_.setMaxMessages(config.maxTrackedMessages);
    tracker_.setTimeoutMs(config.duplicateTrackingTimeout);
    Log(logger::GENERAL,
        "GatewayMessageHandler: Configured with maxMessages=%u, timeout=%ums\n",
        config.maxTrackedMessages, config.duplicateTrackingTimeout);
  }

  /**
   * @brief Handle an incoming GatewayDataPackage and check for duplicates
   *
   * Checks if the message has already been processed using the MessageTracker.
   * If the message is a duplicate, it is dropped silently and metrics are updated.
   * If the message is new, it is marked as processed and should be handled.
   *
   * @param pkg The incoming GatewayDataPackage to check
   * @return true if the message should be processed (not a duplicate)
   * @return false if the message is a duplicate and should be skipped
   */
  bool handleIncomingMessage(const gateway::GatewayDataPackage& pkg) {
    // Check if this message was already processed
    if (tracker_.isProcessed(pkg.messageId, pkg.originNode)) {
      metrics_.duplicatesDetected++;
      Log(logger::GENERAL,
          "GatewayMessageHandler: Duplicate message detected (msgId=%u, origin=%u)\n",
          pkg.messageId, pkg.originNode);
      return false;
    }

    // Mark as processed and allow handling
    tracker_.markProcessed(pkg.messageId, pkg.originNode);
    metrics_.messagesProcessed++;
    Log(logger::GENERAL,
        "GatewayMessageHandler: Processing new message (msgId=%u, origin=%u)\n",
        pkg.messageId, pkg.originNode);
    return true;
  }

  /**
   * @brief Check if an acknowledgment should be sent for a message
   *
   * Checks if an acknowledgment has already been sent for the specified
   * message. This prevents sending duplicate acknowledgments during
   * network partitions or message retries.
   *
   * @param messageId The unique message identifier
   * @param originNode The node that originated the message
   * @return true if acknowledgment should be sent (not already acked)
   * @return false if acknowledgment was already sent (skip sending)
   */
  bool shouldSendAcknowledgment(uint32_t messageId, uint32_t originNode) {
    // Check if we already sent an ack for this message
    if (tracker_.isAcknowledged(messageId, originNode)) {
      metrics_.duplicateAcksSkipped++;
      Log(logger::GENERAL,
          "GatewayMessageHandler: Duplicate ack skipped (msgId=%u, origin=%u)\n",
          messageId, originNode);
      return false;
    }
    return true;
  }

  /**
   * @brief Mark that an acknowledgment has been sent for a message
   *
   * Records that an acknowledgment was sent for the specified message.
   * Subsequent calls to shouldSendAcknowledgment() for this message
   * will return false.
   *
   * @param messageId The unique message identifier
   * @param originNode The node that originated the message
   */
  void markAcknowledgmentSent(uint32_t messageId, uint32_t originNode) {
    // First ensure the message is tracked (in case ack is sent before processing)
    if (!tracker_.isProcessed(messageId, originNode)) {
      tracker_.markProcessed(messageId, originNode);
    }

    tracker_.markAcknowledged(messageId, originNode);
    metrics_.acknowledgmentsSent++;
    Log(logger::GENERAL,
        "GatewayMessageHandler: Acknowledgment sent (msgId=%u, origin=%u)\n",
        messageId, originNode);
  }

  /**
   * @brief Cleanup old entries from the tracker
   *
   * Removes expired entries from the internal MessageTracker based on
   * the configured timeout. Should be called periodically to free memory.
   *
   * @return Number of entries removed
   */
  uint32_t cleanup() {
    return tracker_.cleanup();
  }

  /**
   * @brief Get current metrics for monitoring
   *
   * Returns a copy of the current metrics structure containing
   * statistics about duplicate detection and message processing.
   *
   * @return GatewayMetrics structure with current statistics
   */
  gateway::GatewayMetrics getMetrics() const {
    return metrics_;
  }

  /**
   * @brief Reset all metrics to zero
   *
   * Clears all metrics counters. Does not affect tracked messages.
   */
  void resetMetrics() {
    metrics_.reset();
    Log(logger::GENERAL, "GatewayMessageHandler: Metrics reset\n");
  }

  /**
   * @brief Get the number of currently tracked messages
   * @return Number of entries in the tracker
   */
  size_t getTrackedMessageCount() const {
    return tracker_.size();
  }

  /**
   * @brief Clear all tracked messages
   *
   * Removes all entries from the tracker. Does not affect metrics.
   */
  void clearTrackedMessages() {
    tracker_.clear();
  }

 private:
  MessageTracker tracker_;
  gateway::GatewayMetrics metrics_;
};

}  // namespace painlessmesh

#endif  // _PAINLESS_MESH_GATEWAY_HPP_
