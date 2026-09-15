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

#if defined(ESP32) || defined(ESP8266)
#include <WiFiClient.h>
#endif
#include "painlessmesh/plugin.hpp"
#include "painlessmesh/protocol.hpp"
#include "painlessmesh/validation.hpp"

#include <functional>
#include <map>
#include <vector>

namespace painlessmesh {
namespace gateway {

/** Return whether a channel can be announced for a 2.4 GHz mesh takeover. */
constexpr bool isValidMeshChannel(uint8_t channel) {
  return channel >= 1 && channel <= 13;
}

/** Decide whether a peer must follow an elected bridge to another channel. */
constexpr bool shouldFollowBridgeChannel(uint32_t localNodeId,
                                         uint32_t electedBridgeId,
                                         uint8_t currentChannel,
                                         uint8_t announcedChannel) {
  return electedBridgeId != localNodeId &&
         isValidMeshChannel(announcedChannel) &&
         announcedChannel != currentChannel;
}

/**
 * @brief One occurrence of the mesh SSID seen by an all-channel scan.
 */
struct MeshChannelCandidate {
  uint8_t channel;
  int32_t rssi;
};

/**
 * @brief Where a node that has lost the mesh should go.
 *
 * Channel re-detection runs only after a node's own partition has produced
 * nothing new for a while, so the mesh it can still see on its *current*
 * channel is the partition it is stranded in. When the SSID is also visible
 * on another channel, that is the rest of the network — a bridge that moved
 * to its router's channel, most often — and the node should go there.
 *
 * Returning the first match, as this used to, made the outcome depend on
 * scan order: a stranded node that happened to see its own partition first
 * concluded nothing had changed and stayed stranded.
 *
 * A node that knows the router has a better signal than strength: a bridge's
 * AP is on its router's channel, so a mesh with a bridge lives there. A
 * failover backup that booted while the mesh was split across two channels
 * saw one AP on each, two dB apart, and joined the one without the bridge;
 * it had a partition of two to itself for the hundred seconds it took the
 * re-detection rules to move it, and the election ran out of time.
 *
 * @param candidates  Every channel the mesh SSID was seen on, with RSSI.
 * @param avoidChannel The node's current mesh channel; 0 = no preference.
 * @param routerChannel The channel the router was seen on; 0 = unknown.
 * @return The candidate on routerChannel if there is one; else the strongest
 *         on a channel other than avoidChannel; failing that the strongest
 *         on avoidChannel; 0 if there are none.
 */
inline uint8_t pickMeshChannel(const std::vector<MeshChannelCandidate>& candidates,
                               uint8_t avoidChannel, uint8_t routerChannel = 0) {
  const MeshChannelCandidate* elsewhere = nullptr;
  const MeshChannelCandidate* here = nullptr;
  for (const auto& c : candidates) {
    if (!isValidMeshChannel(c.channel)) continue;
    if (c.channel == routerChannel) return c.channel;
    const MeshChannelCandidate*& slot = (c.channel == avoidChannel) ? here : elsewhere;
    if (slot == nullptr || c.rssi > slot->rssi) slot = &c;
  }
  if (elsewhere != nullptr) return elsewhere->channel;
  if (here != nullptr) return here->channel;
  return 0;
}

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
    // A full probe answers the question the on-demand budget exists for.
    onDemandSpent_ = false;
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
   * @brief Probe once outside the schedule, for a caller that needs the
   *        answer now (issue #450)
   *
   * A bridge's first periodic probe runs while its station is still
   * associating and fails, and the next is a full interval away. A send in
   * that window may spend one extra probe to learn that the uplink has come
   * up since. One: the probe is a blocking connect with a timeout, so the
   * budget is a single on-demand probe per periodic one. A node whose uplink
   * really is down pays it once per interval, not on every call.
   *
   * @return true if Internet is reachable now
   */
  bool checkOnDemand() {
    if (status_.available) return true;
    if (onDemandSpent_) return false;
    const bool connected = checkNow();
    onDemandSpent_ = true;
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
#elif defined(ESP32) || defined(ESP8266)
    // No station, no uplink: answer at once instead of spending the connect
    // timeout on a link that does not exist. This is what keeps the
    // on-demand probe cheap on a bridge whose router is down.
    if (WiFi.status() != WL_CONNECTED) {
      status_.lastError = "Station not connected";
      return false;
    }
    WiFiClient client;
    uint32_t started = millis();
#ifdef ESP32
    bool connected = client.connect(checkHost_.c_str(), checkPort_, checkTimeout_);
#else
    // ESP8266's WiFiClient lacks ESP32's per-connect timeout overload, so the
    // timeout is set on the client instead. It is milliseconds, not seconds:
    // WiFiClient inherits Stream::setTimeout ("maximum milliseconds to wait")
    // and connect() hands _timeout straight to WiFi.hostByName(), whose
    // parameter is named timeout_ms. Dividing by 1000 here gave DNS five
    // milliseconds to resolve, so the check failed every time and an ESP8266
    // shared gateway never reported local Internet.
    client.setTimeout(checkTimeout_);
    bool connected = client.connect(checkHost_.c_str(), checkPort_);
#endif
    status_.lastLatencyMs = millis() - started;
    if (!connected) {
      status_.lastError = "TCP connectivity check failed";
      return false;
    }
    client.stop();
    return true;
#else
    status_.lastError = "Internet health checks are unsupported on this platform";
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
  bool onDemandSpent_ = false;

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
   * @brief Random value drawn once per sendToInternet() call
   *
   * The same on every attempt at that call, and part of its X-Request-Id and
   * Idempotency-Key (requestIdFor()). messageId alone repeats: its counter is
   * 16 bits, so a node that sends more than 65,535 requests in one boot
   * reissues old ids, and a service that remembers keys would drop the new
   * request as a repeat. 0 from a node that predates the field. JSON key
   * "nonce", omitted when 0.
   */
  uint32_t requestNonce = 0;

  /**
   * @brief Number of additional JSON fields in this package
   *
   * Used for jsonObjectSize() calculation in ArduinoJson v6.
   * Count: msgId, origin, ts, prio, dest_url, payload, content, retry, ack,
   * nonce = 10 fields
   */
  static constexpr int numPackageFields = 10;

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
    requestNonce = jsonObj["nonce"] | 0UL;

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
    if (requestNonce != 0) jsonObj["nonce"] = requestNonce;
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
    // The counter starts at a random point each boot. Starting at zero, the
    // first request after every reboot carried the same id as the first
    // request of the boot before -- and so the same X-Request-Id and
    // Idempotency-Key, which a service that remembers keys drops as a repeat.
    static uint16_t counter =
        static_cast<uint16_t>(validation::SecureRandom::generate());
    ++counter;
    if (counter == 0) ++counter;
    // Combine node ID (upper 16 bits) with counter (lower 16 bits)
    return ((nodeId & 0xFFFF) << 16) | counter;
  }

};

/**
 * @brief HTTPClient transport errors, by what they say about the request
 *
 * HTTPClient::GET()/POST() return a negative code when the request failed
 * below HTTP. The ESP32 and ESP8266 cores number them the same way. What
 * matters to a retry is whether the request can have reached the server:
 * resending one that did delivers it twice, which for a request with an
 * effect -- a message, a payment, a counter -- is a second effect (the rig
 * showed a timed-out send issued four times).
 *
 * Where each is raised, in both cores' HTTPClient::sendRequest() for the
 * buffer GET()/POST() the gateway uses: -1, -2 and -3 before the request is
 * complete; everything else from handleHeaderResponse(), which runs after the
 * whole request was written -- so -4 is a connection that closed before the
 * reply began and -7 a reply that was not HTTP, both with the request already
 * at the server. -6 and -8 come from stream sends and from reading a body.
 */
enum HttpTransportError : int {
  HTTP_TRANSPORT_CONNECTION_REFUSED = -1,  ///< no connection was made
  HTTP_TRANSPORT_SEND_HEADER_FAILED = -2,  ///< the request line never completed
  HTTP_TRANSPORT_SEND_PAYLOAD_FAILED = -3, ///< the body never completed
  HTTP_TRANSPORT_NOT_CONNECTED = -4,       ///< closed before the reply began; sent
  HTTP_TRANSPORT_CONNECTION_LOST = -5,     ///< dropped while reading the reply; sent
  HTTP_TRANSPORT_NO_STREAM = -6,           ///< no stream to send or read with
  HTTP_TRANSPORT_NO_HTTP_SERVER = -7,      ///< the reply was not HTTP; sent
  HTTP_TRANSPORT_TOO_LESS_RAM = -8,        ///< out of memory sending or reading
  HTTP_TRANSPORT_ENCODING = -9,            ///< the reply arrived malformed; sent
  HTTP_TRANSPORT_STREAM_WRITE = -10,       ///< the reply could not be stored; sent
  HTTP_TRANSPORT_READ_TIMEOUT = -11,       ///< sent, and no reply in time
};

/**
 * @brief Can a request that failed with this transport error have reached
 *        the server?
 *
 * True -- the safe answer -- for every code that is not known to fail before
 * the request was complete, including codes this library does not know.
 */
inline bool transportErrorMayHaveReachedServer(int rawCode) {
  switch (rawCode) {
    case HTTP_TRANSPORT_CONNECTION_REFUSED:
    case HTTP_TRANSPORT_SEND_HEADER_FAILED:
    case HTTP_TRANSPORT_SEND_PAYLOAD_FAILED:
      return false;
    default:
      return true;
  }
}

/**
 * @brief The outcome of a gateway HTTP request, classified for the ack
 *
 * HTTPClient::GET()/POST() return an int with two distinct meanings: a
 * positive value is an HTTP status code from the destination, while a
 * negative value is one of the client's own transport errors (see
 * HttpTransportError). Zero is not produced by either.
 *
 * GatewayAckPackage::httpStatus is a uint16_t, so a negative code cannot be
 * forwarded as-is. Transport errors are reported as httpStatus 0 with the
 * cause in GatewayAckPackage::error, and whether the origin node may resend
 * the request travels in GatewayAckPackage::retryable.
 *
 * The library applies HTTP's meaning of a status and nothing else. Whether a
 * particular service's reply means what the application wanted -- a message
 * really queued, a record really written -- is the application's decision,
 * made from the status and the response the result carries.
 */
struct HttpRequestOutcome {
  /** True only for 200, 201, 202 and 204. */
  bool success = false;

  /** Value to place in GatewayAckPackage::httpStatus (0 for transport errors). */
  uint16_t ackStatus = 0;

  /** True when the client failed before any HTTP status was received. */
  bool transportError = false;

  /**
   * True for a 2xx outside 200, 201, 202 and 204. 203 says a proxy
   * transformed the reply; 205, 206 and 208 are not what a request to an API
   * endpoint expects. The server answered, so it is never retried; it is
   * reported as a failure the application can inspect.
   */
  bool unverifiedStatus = false;

  /**
   * Whether resending the identical request is safe and useful: the request
   * cannot have reached the server, or the server said it did not take it
   * and to come back (429 Too Many Requests, 503 Service Unavailable). A
   * reply that may mean the request was processed -- any 2xx, a 500, a
   * gateway timeout, a read timeout -- is not retried, because a retry would
   * be a second copy of it.
   */
  bool retryable = false;

  /**
   * One-line summary of the response body, for the error an application
   * reads; empty on success and when no body was read.
   */
  TSTRING reason;
};

/**
 * Longest response-body summary carried in an acknowledgment. Long enough
 * for a service that repeats the request back before its verdict -- the
 * reply in issue #463 spent 97 of 120 characters on the echo and was cut
 * before the part that said why.
 */
static const size_t GATEWAY_RESPONSE_REASON_MAX = 240;

/**
 * How much of a response body the gateway keeps for the summary: its first
 * GATEWAY_RESPONSE_HEAD_BYTES and its last GATEWAY_RESPONSE_TAIL_BYTES.
 * Enough for a service's verdict at either end, small enough that an HTML
 * error page cannot eat an ESP8266's heap.
 */
static const size_t GATEWAY_RESPONSE_HEAD_BYTES = 512;
static const size_t GATEWAY_RESPONSE_TAIL_BYTES = 256;

/**
 * How far into a body the gateway reads looking for its end. Past this the
 * tail kept is the last of what was read, not the body's end; reading on
 * would hold the uplink for a reply nobody summarises.
 */
static const size_t GATEWAY_RESPONSE_SCAN_BYTES = 8192;

/** How long the gateway reads the body for, after the status. */
static const uint32_t GATEWAY_RESPONSE_HEAD_TIMEOUT_MS = 250;

/**
 * Longest wait a server's Retry-After may impose before a retry. A longer one
 * is not honoured with a retry at all: the request fails and says when the
 * server asked to be retried, which is the application's call to make.
 */
static const uint32_t GATEWAY_RETRY_AFTER_MAX_MS = 60000;

inline bool responseContains(const TSTRING& haystack, const char* needle) {
#if defined(PAINLESSMESH_BOOST)
  return haystack.find(needle) != std::string::npos;
#else
  return haystack.indexOf(needle) >= 0;
#endif
}

/**
 * @brief Reduce a response body to one line fit for a log or an error string
 *
 * Tags are dropped and whitespace collapsed, so an HTML page reads as text in
 * a serial log. A body longer than maxLen keeps its beginning and its end,
 * joined by " ... ": services often lead with an echo of the request and
 * finish with the verdict, and a summary that keeps only the beginning keeps
 * the echo and loses the verdict (issue #463).
 */
inline TSTRING summarizeResponseBody(
    const TSTRING& body, size_t maxLen = GATEWAY_RESPONSE_REASON_MAX) {
  TSTRING text;
  bool inTag = false;
  bool pendingSpace = false;
  for (size_t i = 0; i < body.length(); ++i) {
    const char c = body[i];
    if (c == '<') {
      inTag = true;
      continue;
    }
    if (c == '>') {
      inTag = false;
      pendingSpace = true;
      continue;
    }
    if (inTag) continue;
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      pendingSpace = true;
      continue;
    }
    if (pendingSpace && text.length() > 0) text += ' ';
    pendingSpace = false;
    text += c;
  }
  if (text.length() <= maxLen) return text;

  static const char JOIN[] = " ... ";
  const size_t joinLength = sizeof(JOIN) - 1;
  const size_t room = maxLen > joinLength ? maxLen - joinLength : 0;
  const size_t head = room / 2;
  const size_t tail = room - head;
  TSTRING out;
  for (size_t i = 0; i < head; ++i) out += text[i];
  out += JOIN;
  for (size_t i = text.length() - tail; i < text.length(); ++i) out += text[i];
  return out;
}

/**
 * @brief The start and the end of a response body, read a byte at a time
 *
 * The gateway cannot keep a whole body, and a service may put its verdict at
 * either end: CallMeBot echoes the request first and says why last (#463), so
 * a reply of a kilobyte kept as its first 512 bytes had lost the part that
 * mattered before any summary ran. This keeps the first
 * GATEWAY_RESPONSE_HEAD_BYTES and a rolling last GATEWAY_RESPONSE_TAIL_BYTES,
 * and text() joins them with " ... " when bytes were dropped between.
 */
class ResponseExcerpt {
 public:
  /** Keep one more byte; false once GATEWAY_RESPONSE_SCAN_BYTES were seen. */
  bool add(char c) {
    ++seen_;
    if (head_.length() < GATEWAY_RESPONSE_HEAD_BYTES) {
      head_ += c;
    } else {
      tail_[(tailStart_ + tailLength_) % GATEWAY_RESPONSE_TAIL_BYTES] = c;
      if (tailLength_ < GATEWAY_RESPONSE_TAIL_BYTES) {
        ++tailLength_;
      } else {
        tailStart_ = (tailStart_ + 1) % GATEWAY_RESPONSE_TAIL_BYTES;
      }
    }
    return seen_ < GATEWAY_RESPONSE_SCAN_BYTES;
  }

  void add(const TSTRING& text) {
    for (size_t i = 0; i < text.length(); ++i) {
      if (!add(text[i])) return;
    }
  }

  TSTRING text() const {
    TSTRING out = head_;
    if (seen_ > head_.length() + tailLength_) out += " ... ";
    for (size_t i = 0; i < tailLength_; ++i) {
      out += tail_[(tailStart_ + i) % GATEWAY_RESPONSE_TAIL_BYTES];
    }
    return out;
  }

 private:
  TSTRING head_;
  char tail_[GATEWAY_RESPONSE_TAIL_BYTES] = {};
  size_t tailStart_ = 0;
  size_t tailLength_ = 0;
  size_t seen_ = 0;
};

/**
 * @brief Removes HTTP/1.1 chunked transfer framing from a body read a byte at
 *        a time, and passes the content on to a ResponseExcerpt
 *
 * HTTPClient's getString() decodes a chunked body but keeps all of it; the
 * gateway reads the raw stream to keep only a bounded excerpt, and the raw
 * stream is the framing: CallMeBot's real reply reached the application as
 * "a6 Message to: ... Message queued. ... 0" (hardware rig, 2026-09-15) --
 * the chunk size in front, the terminating zero-length chunk behind. This
 * reads the framing (hex size, optional ;extensions, CRLF, data, CRLF, ...,
 * 0, trailers) and forwards only the data. A malformed size line stops
 * decoding rather than guessing: what was decoded so far is kept.
 */
class ChunkedBodyDecoder {
 public:
  explicit ChunkedBodyDecoder(ResponseExcerpt& excerpt) : excerpt_(excerpt) {}

  /** One byte of the raw body; false once the body ended or the excerpt is
   *  full, when the caller can stop reading. */
  bool add(char c) {
    switch (state_) {
      case State::Size:
        if (c == '\r') {
          state_ = State::SizeLf;
        } else if (c == ';') {
          state_ = State::Extension;
        } else if (c == ' ' || c == '\t') {
          // Tolerated around the size, as many servers emit it.
        } else {
          const int digit = hexValue(c);
          if (digit < 0 || sizeDigits_ >= 8) return stop();
          remaining_ = (remaining_ << 4) | static_cast<uint32_t>(digit);
          ++sizeDigits_;
        }
        return true;
      case State::Extension:
        if (c == '\r') state_ = State::SizeLf;
        return true;
      case State::SizeLf:
        if (c != '\n' || sizeDigits_ == 0) return stop();
        sizeDigits_ = 0;
        if (remaining_ == 0) {
          state_ = State::Done;
          return false;
        }
        state_ = State::Data;
        return true;
      case State::Data:
        --remaining_;
        if (remaining_ == 0) state_ = State::DataCr;
        if (!excerpt_.add(c)) {
          state_ = State::Done;
          return false;
        }
        return true;
      case State::DataCr:
        if (c != '\r') return stop();
        state_ = State::DataLf;
        return true;
      case State::DataLf:
        if (c != '\n') return stop();
        state_ = State::Size;
        return true;
      case State::Done:
        return false;
    }
    return false;
  }

  void add(const TSTRING& raw) {
    for (size_t i = 0; i < raw.length(); ++i) {
      if (!add(raw[i])) return;
    }
  }

  /** True when the terminating zero-length chunk was read. */
  bool complete() const { return state_ == State::Done && !malformed_; }

 private:
  enum class State { Size, Extension, SizeLf, Data, DataCr, DataLf, Done };

  static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  }

  bool stop() {
    malformed_ = true;
    state_ = State::Done;
    return false;
  }

  ResponseExcerpt& excerpt_;
  State state_ = State::Size;
  uint32_t remaining_ = 0;
  uint8_t sizeDigits_ = 0;
  bool malformed_ = false;
};

/** Whether a Transfer-Encoding header value names chunked encoding. */
inline bool transferEncodingIsChunked(const TSTRING& value) {
  TSTRING lower;
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    lower += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
  }
  return responseContains(lower, "chunked");
}

/**
 * @brief The delay a Retry-After header asks for, in milliseconds
 *
 * Only the delay-seconds form is read; an HTTP-date, or anything else, is
 * 0 -- no instruction -- rather than a guess. Values past
 * GATEWAY_RETRY_AFTER_MAX_MS are returned as they are, so the caller can see
 * the server asked for longer than it will wait.
 */
inline uint32_t parseRetryAfterMs(const TSTRING& value) {
  uint64_t seconds = 0;
  size_t digits = 0;
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == ' ' || c == '\t') {
      if (digits == 0) continue;
      break;
    }
    if (c < '0' || c > '9') return 0;
    seconds = seconds * 10 + static_cast<uint64_t>(c - '0');
    if (seconds > 0xFFFFFFFFULL / 1000ULL) return 0xFFFFFFFFUL;
    ++digits;
  }
  return digits == 0 ? 0 : static_cast<uint32_t>(seconds * 1000ULL);
}

/**
 * @brief The identifier the gateway sends with a request, as both
 *        X-Request-Id and Idempotency-Key
 *
 * The same for every attempt at one sendToInternet() call, so a service that
 * honours Idempotency-Key treats a retry as the request it already has, and
 * anything recording requests can count the copies one call produced.
 *
 * The request's nonce keeps it unique beyond its messageId, whose counter
 * wraps after 65,535 requests in a boot. A package from a node that predates
 * the nonce (0) keeps the two-part form it always had.
 */
inline TSTRING requestIdFor(uint32_t originNode, uint32_t messageId,
                            uint32_t requestNonce = 0) {
  char buffer[40];
  if (requestNonce == 0) {
    snprintf(buffer, sizeof(buffer), "pm-%08x-%08x",
             static_cast<unsigned>(originNode), static_cast<unsigned>(messageId));
  } else {
    snprintf(buffer, sizeof(buffer), "pm-%08x-%08x-%08x",
             static_cast<unsigned>(originNode), static_cast<unsigned>(messageId),
             static_cast<unsigned>(requestNonce));
  }
  return TSTRING(buffer);
}

/** A request nonce: random, and never the 0 that means "none". */
inline uint32_t newRequestNonce() {
  uint32_t nonce = validation::SecureRandom::generate();
  return nonce != 0 ? nonce : 1;
}

/**
 * @brief Classify an HTTPClient result for the gateway acknowledgment
 *
 * HTTP semantics only. 200, 201, 202 and 204 are a success. Any other 2xx is
 * an unverified failure the server answered; 1xx, 3xx, 4xx and 5xx are
 * failures. A retry is allowed only when it cannot produce a second copy of
 * the request: a transport error that failed before the request was complete,
 * or 429/503, where the server says it did not take the request. The reason
 * is a summary of whatever body was read, so the application learns why.
 *
 * @param rawCode The int returned by HTTPClient::GET() or ::POST()
 * @param body The start of the response body, empty if none was read
 * @return Classified outcome, safe to place in a GatewayAckPackage
 */
inline HttpRequestOutcome classifyHttpResult(int rawCode,
                                             const TSTRING& body = TSTRING()) {
  HttpRequestOutcome outcome;
  if (rawCode <= 0) {
    outcome.transportError = true;
    outcome.retryable = !transportErrorMayHaveReachedServer(rawCode);
    return outcome;
  }
  outcome.ackStatus =
      static_cast<uint16_t>(rawCode > 0xFFFF ? 0xFFFF : rawCode);
  outcome.success =
      rawCode == 200 || rawCode == 201 || rawCode == 202 || rawCode == 204;
  outcome.unverifiedStatus = !outcome.success && rawCode >= 200 && rawCode < 300;
  outcome.retryable = rawCode == 429 || rawCode == 503;
  if (!outcome.success) outcome.reason = summarizeResponseBody(body);
  return outcome;
}

/**
 * The phrase a node puts in a failed GATEWAY_ACK when it received a gateway
 * request but serves none: a bridge that rebooted, crashed or was reflashed
 * as a regular node announces nothing, and its peers keep routing Internet
 * requests to it until its last bridge status ages out. The origin node
 * recognises the phrase, forgets that node as a gateway, and retries through
 * the next one. Changing it breaks that recognition across a mixed fleet.
 */
static const char GATEWAY_NOT_A_GATEWAY_PHRASE[] = "is not an Internet gateway";

/**
 * How long a destination whose name failed to resolve is refused without
 * another lookup (issue #453). On ESP32 a DNS lookup has no timeout this
 * library can set, so a dead name stalls the cooperative scheduler for the
 * resolver's own patience; once a minute is survivable, once per attempt --
 * with the origin node retrying and the bridge's own sends adding theirs --
 * was not. On ESP8266 the core bounds HTTPClient's own lookup by the HTTP
 * timeout, so the gateway performs no separate lookup there and this cache
 * is never fed.
 */
static const uint32_t GATEWAY_DNS_NEGATIVE_TTL_MS = 60000;

/**
 * @brief The host of an http(s) URL, without scheme, port, path or query
 *
 * "https://api.example.com:8443/x?y" -> "api.example.com". Empty when the
 * URL has no host.
 */
inline TSTRING hostFromUrl(const TSTRING& url) {
  const char* s = url.c_str();
  const size_t n = url.length();
  size_t i = 0;
  for (size_t k = 0; k + 2 < n; ++k) {
    if (s[k] == ':' && s[k + 1] == '/' && s[k + 2] == '/') {
      i = k + 3;
      break;
    }
  }
  size_t j = i;
  while (j < n && s[j] != '/' && s[j] != '?' && s[j] != '#' && s[j] != ':') ++j;
  TSTRING host;
  for (size_t k = i; k < j; ++k) host += s[k];
  return host;
}

/** True for a dotted-decimal IPv4 literal, which needs no DNS. */
inline bool looksLikeIpLiteral(const TSTRING& host) {
  if (host.length() == 0) return false;
  for (size_t i = 0; i < host.length(); ++i) {
    const char c = host[i];
    if (!(c >= '0' && c <= '9') && c != '.') return false;
  }
  return true;
}

/**
 * @brief A few hosts that recently failed to resolve, and when
 *
 * Small and fixed: a gateway talks to a handful of destinations. A host is
 * remembered with a TTL; while it is within it, isFailing() says so and the
 * caller answers the request without a lookup.
 */
class NegativeDnsCache {
 public:
  // An enum, not a static const member: the test suite passes it to Catch2 by
  // reference, which needs a definition C++14 cannot give an in-class
  // constant without an out-of-line one.
  enum : size_t { SLOTS = 4 };

  void remember(const TSTRING& host, uint32_t nowMs,
                uint32_t ttlMs = GATEWAY_DNS_NEGATIVE_TTL_MS) {
    Entry* slot = find(host);
    if (slot == nullptr) {
      slot = &entries_[0];
      for (size_t i = 0; i < SLOTS; ++i) {
        if (!entries_[i].used) {
          slot = &entries_[i];
          break;
        }
        if (static_cast<int32_t>(entries_[i].at - slot->at) < 0) slot = &entries_[i];
      }
    }
    slot->used = true;
    slot->host = host;
    slot->at = nowMs;
    slot->ttl = ttlMs;
  }

  /** Is `host` inside its negative TTL? `ageMs` receives how long ago it failed. */
  bool isFailing(const TSTRING& host, uint32_t nowMs, uint32_t* ageMs = nullptr) {
    Entry* slot = find(host);
    if (slot == nullptr) return false;
    const uint32_t age = nowMs - slot->at;
    if (age >= slot->ttl) {
      slot->used = false;
      return false;
    }
    if (ageMs != nullptr) *ageMs = age;
    return true;
  }

  void forget(const TSTRING& host) {
    Entry* slot = find(host);
    if (slot != nullptr) slot->used = false;
  }

  size_t size() const {
    size_t n = 0;
    for (size_t i = 0; i < SLOTS; ++i) n += entries_[i].used ? 1 : 0;
    return n;
  }

 private:
  struct Entry {
    TSTRING host;
    uint32_t at = 0;
    uint32_t ttl = 0;
    bool used = false;
  };

  Entry* find(const TSTRING& host) {
    for (size_t i = 0; i < SLOTS; ++i) {
      if (entries_[i].used && entries_[i].host == host) return &entries_[i];
    }
    return nullptr;
  }

  Entry entries_[SLOTS];
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
   * @brief The start of the response body, summarized to one line
   *
   * Carried on success as well as failure, because whether a reply means what
   * the application wanted is the application's decision, not the library's.
   * Empty when no body was read. JSON key "resp", omitted when empty.
   */
  TSTRING response = "";

  /**
   * @brief Whether the origin node may resend the identical request
   *
   * 1 when the request cannot have reached the server or the server asked to
   * be retried, 0 when a resend could deliver it twice. -1 when the gateway
   * did not say -- one that predates the field -- and the origin node falls
   * back to its own reading of the status. JSON key "retry", omitted at -1.
   */
  int8_t retryable = -1;

  /**
   * @brief How long the server asked to wait before a retry, in milliseconds
   *
   * From a Retry-After header on a 429 or 503. JSON key "retryAfter",
   * omitted when 0.
   */
  uint32_t retryAfterMs = 0;

  /**
   * @brief Number of additional JSON fields in this package
   *
   * Used for jsonObjectSize() calculation in ArduinoJson v6.
   * Count: msgId, origin, success, http, err, ts, resp, retry, retryAfter = 9
   */
  static constexpr int numPackageFields = 9;

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
    if (jsonObj.containsKey("resp"))
      response = jsonObj["resp"].as<TSTRING>();
    if (jsonObj.containsKey("retry"))
      retryable = jsonObj["retry"].as<bool>() ? 1 : 0;
#else
    if (jsonObj["err"].is<TSTRING>())
      error = jsonObj["err"].as<TSTRING>();
    if (jsonObj["resp"].is<TSTRING>())
      response = jsonObj["resp"].as<TSTRING>();
    if (jsonObj["retry"].is<bool>())
      retryable = jsonObj["retry"].as<bool>() ? 1 : 0;
#endif
    retryAfterMs = jsonObj["retryAfter"] | 0UL;
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
    // Added in 2.1.0 and omitted when they say nothing, so an ack to a node
    // that predates them is the ack it always was.
    if (response.length() > 0) jsonObj["resp"] = response;
    if (retryable >= 0) jsonObj["retry"] = retryable == 1;
    if (retryAfterMs > 0) jsonObj["retryAfter"] = retryAfterMs;
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
    return JSON_OBJECT_SIZE(noJsonFields + numPackageFields) + error.length() +
           response.length();
  }
#endif

};

// ===========================================================================
// Scheduler-stall protection for blocking gateway requests
// ===========================================================================

// The socket timeouts below are derived from NODE_TIMEOUT rather than written
// as absolute numbers, so a gateway request cannot outlast the mesh watchdog in
// *any* build. That is not hypothetical tidiness: the host test environment
// overrides NODE_TIMEOUT to 5s (test/catch/Arduino.h shadows configuration.hpp
// wholesale via its include guard), so hardcoded defaults sized for the 10s
// production watchdog would blow the assertion below there.
//
// They live in this header, not in configuration.hpp, for the same reason --
// gateway.hpp is reached through both configurations, configuration.hpp is not.
//
// At the production NODE_TIMEOUT of 10s this gives 5000ms for the request and
// 2000ms for the captive-portal probe. Dividing by TASK_MILLISECOND first
// normalises out the scheduler resolution, so the result is milliseconds under
// _TASK_MICRO_RES too.

/** Socket timeout, in milliseconds, for a gateway Internet request.
 *
 * An HTTPClient call chain can wait on the socket twice — once sending the
 * request/reading the headers and once reading the body — so the blocking
 * budget below counts this value twice (issue #416). That is why the default
 * is NODE_TIMEOUT/5 rather than the pre-2.0 NODE_TIMEOUT/2: the *wall-clock*
 * worst case of the request, not one socket wait, has to fit inside the mesh
 * watchdog. Endpoints that genuinely need longer must raise NODE_TIMEOUT
 * along with this (the static_assert below enforces that). */
#ifndef GATEWAY_HTTP_TIMEOUT_MS
#define GATEWAY_HTTP_TIMEOUT_MS ((NODE_TIMEOUT) / TASK_MILLISECOND / 5)
#endif

/** Socket timeout, in milliseconds, for the captive-portal probe. Counted
 * twice in the blocking budget, same as GATEWAY_HTTP_TIMEOUT_MS. */
#ifndef GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS
#define GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS ((NODE_TIMEOUT) / TASK_MILLISECOND / 10)
#endif

/** Timeout, in milliseconds, for the DNS reachability probe (issue #416).
 *
 * Only the ESP8266 core exposes a hostByName() overload with a timeout
 * parameter; on ESP32 the probe is skipped entirely (the captive-portal
 * probe, which is an HTTP round trip bounded by
 * GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS, establishes reachability instead).
 * The budget counts this term unconditionally, which is conservative on
 * ESP32. */
#ifndef GATEWAY_DNS_TIMEOUT_MS
#define GATEWAY_DNS_TIMEOUT_MS ((NODE_TIMEOUT) / TASK_MILLISECOND / 10)
#endif

/** How long, in milliseconds, a connectivity probe result stays cached.
 *
 * Both the DNS reachability check and the captive-portal probe are network
 * round trips. Running them per message would put an Internet round trip in
 * front of every single mesh->Internet send.
 */
#ifndef GATEWAY_CONNECTIVITY_CACHE_MS
#define GATEWAY_CONNECTIVITY_CACHE_MS 60000UL
#endif

/**
 * @brief Wall-clock ceiling of the gateway's blocking calls on the
 *        GATEWAY_DATA path, in milliseconds (issue #416).
 *
 * Each HTTPClient call chain is counted at *two* socket waits — GET()/POST()
 * (send + header read) and getString() (body read) — because
 * HTTPClient::setTimeout() bounds an individual socket wait, not the whole
 * call. The captive-portal probe and the destination request both have that
 * shape and run back to back, and the DNS reachability probe (bounded by
 * GATEWAY_DNS_TIMEOUT_MS on ESP8266, skipped on ESP32) runs before them
 * whenever the GATEWAY_CONNECTIVITY_CACHE_MS window has expired.
 *
 * @warning One residual is not in this budget: hostname resolution on ESP32
 *          is not separately boundable in the cores this library targets.
 *          Since issue #453 the handler performs that lookup itself on
 *          ESP32 and remembers a failure for GATEWAY_DNS_NEGATIVE_TTL_MS, so
 *          on a network with blackholed DNS the request path can still
 *          exceed this ceiling on ESP32, but at most once per TTL per
 *          destination host rather than once per attempt. On ESP8266 the
 *          core bounds HTTPClient's own lookup by the HTTP timeout. See
 *          SECURITY.md "Gateway blocking: the mesh partition risk".
 */
constexpr unsigned long gatewayBlockingBudgetMs() {
  return 2UL * static_cast<unsigned long>(GATEWAY_HTTP_TIMEOUT_MS) +
         2UL * static_cast<unsigned long>(GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS) +
         static_cast<unsigned long>(GATEWAY_DNS_TIMEOUT_MS);
}

// A gateway that can block longer than the mesh watchdog partitions the mesh
// around itself (issues #318, #332). Catch what is expressible at compile time
// -- both socket waits of each HTTP call plus the DNS probe; see the ESP32
// resolver caveat above for the one wait this cannot cover. TASK_ constants
// are scaled by the scheduler's resolution, so the comparison is written in
// scheduler units to stay correct under _TASK_MICRO_RES too.
static_assert(gatewayBlockingBudgetMs() * TASK_MILLISECOND < NODE_TIMEOUT,
              "Gateway blocking budget (2x GATEWAY_HTTP_TIMEOUT_MS + 2x "
              "GATEWAY_CAPTIVE_PORTAL_TIMEOUT_MS + GATEWAY_DNS_TIMEOUT_MS) "
              "must stay below NODE_TIMEOUT, or a gateway request stalls the "
              "scheduler for longer than its peers are willing to wait and "
              "the mesh partitions around the gateway. Raise NODE_TIMEOUT if "
              "you need a longer HTTP timeout.");

/**
 * Reserve the blocking HTTP budget on the requester's route watchdog.
 *
 * A gateway compensates its own peer watchdogs after a blocking request, but
 * it cannot modify the requester's timer. That timer may already be partly
 * spent, so extend its existing deadline by the bounded gateway budget before
 * sending. Disabled watchdogs remain disabled.
 */
template <typename T>
bool reserveGatewayBlockingBudget(T& connection) {
  if (!connection.timeOutTask.isEnabled()) return false;
  connection.timeOutTask.adjust(static_cast<long>(
      gatewayBlockingBudgetMs() * TASK_MILLISECOND));
  return true;
}

/**
 * @brief Give every peer's running watchdog back the time a blocking call hid.
 *
 * Call this immediately after returning from a blocking Internet call, before
 * the scheduler next runs, passing the measured wall-clock duration of the
 * stall.
 *
 * Nothing executes while the gateway is inside `HTTPClient::GET()`/`POST()`,
 * but wall-clock time keeps passing. So a `timeOutTask` whose NODE_TIMEOUT
 * deadline fell during the stall is already overdue when the scheduler
 * resumes, and fires on the very next `execute()` -- closing a peer that never
 * actually went missing (issues #318, #332).
 *
 * The compensation equals the measured stall, no more (issue #417): each
 * enabled watchdog's existing deadline is postponed by `stalledMs` via
 * `Task::adjust()`, which extends `iDelay` without touching the task's
 * baseline. A peer's watchdog therefore measures only time the mesh was
 * actually able to observe the peer. The earlier implementation restarted
 * every watchdog from zero (`restartDelayed()`) on every exit path -- which
 * meant any live peer's gateway traffic granted a genuinely dead peer a
 * fresh full NODE_TIMEOUT, so the dead peer was never reaped.
 * `Task::delay()` would be wrong in the other direction: it resets the
 * baseline to now, shortening the deadline of a peer that still had more
 * than `stalledMs` remaining and disconnecting healthy peers early.
 *
 * Paths that did not block must pass 0 and compensate nothing.
 *
 * Only *enabled* watchdogs are touched, and this matters: `timeOutTask` is
 * armed by `nodeSyncTask` when a sync request goes out and disabled again when
 * the reply lands, so a disabled watchdog means the peer has nothing
 * outstanding. Enabling it here would invent a NODE_TIMEOUT deadline for an
 * idle-but-healthy link whose callback is `Connection::close()` -- turning a
 * reliability fix into a disconnect bug.
 *
 * This protects the gateway's own view of its peers. The peers' view of the
 * gateway depends on keeping the stall shorter than their watchdog, which is
 * what gatewayBlockingBudgetMs() is meant to bound -- though see the @warning
 * there for why that bound is not airtight (#416). Request-side reservation
 * plus gateway-side compensation are both needed.
 *
 * @tparam T   Mesh type exposing `subs` (see painlessmesh::layout::Layout).
 * @param mesh The mesh whose direct peer connections should be refreshed.
 * @param stalledMs Measured wall-clock duration of the blocking section, in
 *        milliseconds. 0 means nothing blocked and nothing is compensated.
 * @return Number of peers whose watchdog deadline was postponed.
 */
template <typename T>
size_t refreshPeerWatchdogs(T& mesh, unsigned long stalledMs) {
  if (stalledMs == 0) return 0;  // nothing blocked, nothing to give back
  size_t refreshed = 0;
  for (auto&& connection : mesh.subs) {
    if (!connection) continue;
    if (!connection->timeOutTask.isEnabled()) continue;
    connection->timeOutTask.adjust(
        static_cast<long>(stalledMs * TASK_MILLISECOND));
    ++refreshed;
  }
  return refreshed;
}

}  // namespace gateway
}  // namespace painlessmesh

#endif  // _PAINLESS_MESH_GATEWAY_HPP_
