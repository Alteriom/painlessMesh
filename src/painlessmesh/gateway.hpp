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
#include "painlessmesh/plugin.hpp"
#include "painlessmesh/protocol.hpp"

#include <functional>

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

}  // namespace gateway
}  // namespace painlessmesh

#endif  // _PAINLESS_MESH_GATEWAY_HPP_
