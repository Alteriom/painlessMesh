#ifndef _PAINLESS_MESH_TCP_HPP_
#define _PAINLESS_MESH_TCP_HPP_

#include <list>

#include "Arduino.h"
#include "painlessmesh/configuration.hpp"

#include "painlessmesh/logger.hpp"

namespace painlessmesh {
namespace tcp {

// TCP connection retry configuration
// These can be tuned for different network conditions
// Increased values to better handle real-world mesh network conditions where:
// - TCP server may need more time to be ready after AP initialization
// - Network stack stabilization takes longer on some hardware
// - Multiple nodes connecting simultaneously can cause temporary overload
static const uint8_t TCP_CONNECT_MAX_RETRIES = 5;       // Max retry attempts before giving up
static const uint32_t TCP_CONNECT_RETRY_DELAY_MS = 1000; // Delay between retry attempts (1 second)
static const uint32_t TCP_CONNECT_STABILIZATION_DELAY_MS = 500; // Delay after IP acquisition (500ms)
// TCP_CLIENT_CLEANUP_DELAY_MS is defined in connection.hpp since it's used in the destructor
// Delay before WiFi reconnection after all TCP retries are exhausted
// This prevents rapid reconnection loops when TCP server is persistently unavailable
// Gives the TCP server more time to recover and reduces network congestion
static const uint32_t TCP_EXHAUSTION_RECONNECT_DELAY_MS = 10000; // 10 seconds before reconnection

// Duration to block a node after TCP connection retry exhaustion (60 seconds)
// This prevents repeatedly trying to connect to nodes with unresponsive TCP servers
static const uint32_t TCP_FAILURE_BLOCK_DURATION_MS = 60000;

// Safe operating bounds applied by clampTcpRetryConfig().
// Each retry allocates a new AsyncClient and schedules a task, so an unbounded
// maxRetries is a heap and recursion-depth hazard on ESP8266. A zero retry
// delay would schedule retry tasks with no spacing, i.e. a hot loop allocating
// an AsyncClient per scheduler tick.
static const uint8_t TCP_RETRY_MAX_RETRIES_LIMIT = 10;
static const uint32_t TCP_RETRY_MIN_DELAY_MS = 50;
static const uint32_t TCP_RETRY_MAX_DELAY_MS = 60000;

/**
 * User-tunable TCP connection retry parameters
 *
 * One instance is stored per mesh (painlessmesh::Mesh), configured through
 * Mesh::setTcpRetryConfig(). The member defaults are spelled as the legacy
 * constants above rather than as literals, so the "defaults match the previous
 * hardcoded behaviour exactly" guarantee is enforced by the compiler and cannot
 * silently drift.
 *
 * NOTE: this deliberately lives on the mesh instance rather than at namespace
 * scope. A mutable namespace-scope instance in this header would give every
 * translation unit its own private copy, so a sketch that configured the mesh
 * in one TU and connected from another would silently fall back to defaults
 * with no diagnostic.
 */
struct TcpRetryConfig {
  /// Max TCP connect retry attempts before falling back to a WiFi reconnect
  uint8_t maxRetries = TCP_CONNECT_MAX_RETRIES;
  /// Base delay between retry attempts (ms), scaled by exponential backoff
  uint32_t retryDelayMs = TCP_CONNECT_RETRY_DELAY_MS;
  /// Delay after IP acquisition before the TCP connect is attempted (ms)
  uint32_t stabilizationDelayMs = TCP_CONNECT_STABILIZATION_DELAY_MS;
  /// Delay before the WiFi reconnect that follows retry exhaustion (ms)
  uint32_t exhaustionReconnectDelayMs = TCP_EXHAUSTION_RECONNECT_DELAY_MS;
  /// Duration a node is blocklisted after retry exhaustion (ms, 0 = never)
  uint32_t failureBlockDurationMs = TCP_FAILURE_BLOCK_DURATION_MS;
};

/**
 * Delay before retry attempt number `retryCount`
 *
 * Exponential backoff with the multiplier capped at 8, giving the default
 * sequence 1s, 2s, 4s, 8s, 8s. Capping prevents excessive delays and keeps the
 * product clear of uint32_t overflow.
 *
 * @param cfg Active retry configuration
 * @param retryCount Zero-based retry attempt number
 * @return Delay in milliseconds
 */
inline uint32_t retryBackoffDelay(const TcpRetryConfig &cfg,
                                  uint8_t retryCount) {
  uint8_t backoffMultiplier = (retryCount < 3) ? (1U << retryCount) : 8;
  return cfg.retryDelayMs * backoffMultiplier;
}

/**
 * Coerce a retry configuration into safe operating bounds
 *
 * Only the two values that can render a node unusable are clamped:
 * maxRetries (heap/recursion pressure) and retryDelayMs (hot-loop floor and
 * overflow ceiling). maxRetries == 0 is explicitly allowed - it means "fall
 * back to a WiFi reconnect on the first TCP error", which is the whole point
 * of the low-latency profile. stabilizationDelayMs, exhaustionReconnectDelayMs
 * and failureBlockDurationMs are left alone because 0 is meaningful for each
 * (skip stabilization / reconnect immediately / never blocklist).
 *
 * @param cfg Configuration to clamp (taken by value, returned clamped)
 * @return The clamped configuration
 */
inline TcpRetryConfig clampTcpRetryConfig(TcpRetryConfig cfg) {
  if (cfg.maxRetries > TCP_RETRY_MAX_RETRIES_LIMIT)
    cfg.maxRetries = TCP_RETRY_MAX_RETRIES_LIMIT;
  if (cfg.retryDelayMs < TCP_RETRY_MIN_DELAY_MS)
    cfg.retryDelayMs = TCP_RETRY_MIN_DELAY_MS;
  if (cfg.retryDelayMs > TCP_RETRY_MAX_DELAY_MS)
    cfg.retryDelayMs = TCP_RETRY_MAX_DELAY_MS;
  return cfg;
}

inline uint32_t encodeNodeId(const uint8_t *hwaddr) {
  using namespace painlessmesh::logger;
  Log(GENERAL, "encodeNodeId():\n");
  uint32_t value = 0;

  // Extract last 4 bytes of MAC address (skip first 2 bytes)
  // Big endian (aka "network order") encoding
  value |= hwaddr[2] << 24;  // Byte 2 -> bits 31-24
  value |= hwaddr[3] << 16;  // Byte 3 -> bits 23-16
  value |= hwaddr[4] << 8;   // Byte 4 -> bits 15-8
  value |= hwaddr[5];        // Byte 5 -> bits 7-0
  return value;
}

// Decode nodeId from mesh IP address
// Mesh IPs follow format: 10.(nodeId >> 8).(nodeId & 0xFF).1
inline uint32_t decodeNodeIdFromIP(IPAddress ip) {
#if defined(PAINLESSMESH_BOOST)
  // In test environment, IPAddress doesn't support indexing
  // Return 0 to indicate invalid/unknown node ID
  (void)ip;  // Suppress unused parameter warning
  return 0;
#else
  // Validate mesh network IP format: 10.x.x.1
  // First octet must be 10, last octet must be 1
  const uint8_t MESH_IP_FIRST_OCTET = 10;
  const uint8_t MESH_IP_LAST_OCTET = 1;
  if (ip[0] != MESH_IP_FIRST_OCTET || ip[3] != MESH_IP_LAST_OCTET) {
    return 0;  // Invalid mesh IP
  }
  
  // Extract nodeId from second and third octets
  // NodeId = (octet2 << 8) | octet3
  const uint8_t BYTE_SHIFT = 8;  // Bits per byte
  uint32_t nodeId = ((uint32_t)ip[1] << BYTE_SHIFT) | (uint32_t)ip[2];
  return nodeId;
#endif
}

template <class T, class M>
void initServer(AsyncServer &server, M &mesh) {
  using namespace logger;
  server.setNoDelay(true);

  server.onClient(
      [&mesh](void *arg, AsyncClient *client) {
        if (mesh.semaphoreTake()) {
          Log(CONNECTION, "New AP connection incoming\n");
          auto conn = std::make_shared<T>(client, &mesh, false);
          conn->initTasks();
          mesh.subs.push_back(conn);
          mesh.semaphoreGive();
        }
      },
      NULL);
  server.begin();
}

/**
 * Establish TCP connection with retry mechanism and exponential backoff
 * 
 * This function attempts to connect to the mesh network via TCP.
 * If the connection fails (error -14 ERR_CONN or other errors), it will
 * retry up to the configured maxRetries times before triggering a full
 * WiFi reconnection cycle. See Mesh::setTcpRetryConfig() to tune the retry
 * envelope; the defaults reproduce the historic hardcoded behaviour.
 * 
 * The retry mechanism helps handle timing issues where:
 * - The TCP server may not be immediately ready after AP initialization
 * - Network stack may need time to stabilize after IP acquisition
 * - Transient network conditions may cause temporary connection failures
 * - Multiple nodes connecting simultaneously may cause temporary overload
 * 
 * Exponential backoff is used to increase delay between retries, which:
 * - Gives the TCP server more time to recover from overload
 * - Reduces network contention when multiple nodes are retrying
 * - Improves overall connection success rate in congested networks
 * 
 * @param client AsyncClient to use for connection
 * @param ip Target IP address
 * @param port Target port
 * @param mesh Reference to mesh instance for callbacks
 * @param retryCount Current retry attempt (default 0, used internally for recursion)
 */
template <class T, class M>
void connect(AsyncClient &client, IPAddress ip, uint16_t port, M &mesh, 
             uint8_t retryCount = 0) {
  using namespace logger;

  // Snapshot the retry configuration once, before any lambda is built. The
  // onError lambda outlives this stack frame, so it captures this snapshot by
  // value rather than reading mesh's member later: a user callback could
  // otherwise mutate the config between the connect attempt and the error,
  // tearing the backoff schedule mid-sequence. Each retry re-enters connect()
  // and re-snapshots, so a config change still takes effect on the next
  // attempt.
  const TcpRetryConfig cfg = mesh.getTcpRetryConfig();

  Log(CONNECTION, "tcp::connect(): Attempting connection to port %d (attempt %d/%d)\n",
      port, retryCount + 1, cfg.maxRetries + 1);

  // Guard shared between onError and onConnect: under normal conditions a
  // TCP connection attempt leads to only ONE of the two outcomes. But if
  // WiFi drops right as the TCP handshake completes, AsyncTCP can queue
  // BOTH events (connection succeeded at the TCP level + abort due to the
  // WiFi link being lost) for the same AsyncClient, and both callbacks end
  // up firing for the SAME object. Without this guard, the client would be
  // "handed over" twice: once to a BufferedConnection (via onConnect, which
  // becomes its owner and will delete it via its own destructor) and once
  // to the retry path (via onError, which schedules it again for deletion)
  // - two scheduleAsyncClientDeletion() calls on the same pointer, and
  // therefore a double deferred deletion on the same object once both
  // tasks fire.
  auto claimed = std::make_shared<bool>(false);
  
  // Store retry count and connection parameters for the error handler
  // We need to capture these by value since they're used in the lambda
  client.onError([&mesh, ip, port, retryCount, claimed, cfg](void *, AsyncClient *client, int8_t err) {
    if (*claimed) {
      // onConnect has already claimed this client (the connection actually
      // succeeded, and it's already been wrapped in a BufferedConnection
      // that owns it): don't touch the same object again.
      Log(CONNECTION,
          "tcp_err(): onError fired after onConnect had already claimed "
          "the client - ignored to avoid double handling\n");
      return;
    }
    *claimed = true;
    if (mesh.semaphoreTake()) {
      Log(CONNECTION, "tcp_err(): error trying to connect %d (attempt %d/%d)\n",
          err, retryCount + 1, cfg.maxRetries + 1);
      
      // Check if we have retries left - retry logic only works on real hardware
      // In test environment (PAINLESSMESH_BOOST), fall through to dropped connection
      // Note: ip and port are used in retry logic below, suppress unused warnings for test builds
      (void)ip;
      (void)port;
#if !defined(PAINLESSMESH_BOOST) && (defined(ESP32) || defined(ESP8266))
      if (retryCount < cfg.maxRetries) {
        // Delay grows with exponential backoff, multiplier capped at 8.
        // With the default 1000ms base that is: 1s, 2s, 4s, 8s, 8s.
        uint32_t retryDelay = retryBackoffDelay(cfg, retryCount);

        Log(CONNECTION, "tcp_err(): Scheduling retry in %u ms\n", retryDelay);
        
        // Schedule a retry after a delay using the mesh's task scheduler
        // Note: &mesh is captured by reference because:
        // 1. Mesh is a singleton that lives for the program's lifetime
        // 2. The task scheduler belongs to the mesh, so mesh is always valid when task runs
        // 3. Copying the mesh object is not possible/allowed
        // Recursion depth is strictly bounded by cfg.maxRetries (default 5),
        // which clampTcpRetryConfig() never lets exceed
        // TCP_RETRY_MAX_RETRIES_LIMIT (10).
        mesh.addTask([&mesh, ip, port, retryCount]() {
          Log(CONNECTION, "tcp_err(): Retrying TCP connection...\n");
          
          // Create a new AsyncClient for the retry
          // On success, the client is managed by the Connection object
          // On failure, the onError handler for the new client will handle cleanup
          AsyncClient *pRetryConn = new AsyncClient();
          connect<T, M>((*pRetryConn), ip, port, mesh, retryCount + 1);
        }, retryDelay);
        
        // Defer deletion of the failed AsyncClient to prevent heap corruption
        // Use the centralized deletion scheduler to ensure proper spacing between deletions
        // This prevents concurrent cleanup operations in the AsyncTCP library
        scheduleAsyncClientDeletion(mesh.mScheduler, client, "tcp_err(retry)");
        
        mesh.semaphoreGive();
        return;
      }
      
      // All retries exhausted - schedule delayed reconnection
      // Adding a significant delay before reconnection prevents rapid reconnection loops
      // when the TCP server is persistently unavailable or overloaded
      Log(CONNECTION, "tcp_err(): All %d retries exhausted for IP %s\n",
          cfg.maxRetries + 1, ip.toString().c_str());
      
      // Block this node temporarily to prevent immediate reconnection to the same unresponsive node
      // This helps when the bridge's TCP server is down but WiFi AP is still running
      // The blocklist is only used during AP filtering, so it won't affect existing connections
      #if !defined(PAINLESSMESH_BOOST)
      // Try to decode nodeId from IP and block it
      // Only works for mesh IPs (format: 10.x.x.1)
      // A failureBlockDurationMs of 0 is an explicit "never blocklist" opt-out.
      // Passing it through would set blockUntil = millis() + 0, which
      // isNodeBlocked() reads as already expired - so the entry would do
      // nothing except linger in the blocklist until cleanup sweeps it.
      uint32_t failedNodeId = decodeNodeIdFromIP(ip);
      if (failedNodeId != 0 && cfg.failureBlockDurationMs > 0) {
        // Note: This requires M to be wifi::Mesh which has blockNodeAfterTCPFailure
        mesh.blockNodeAfterTCPFailure(ip, cfg.failureBlockDurationMs);
      }
      #endif

      Log(CONNECTION, "tcp_err(): Scheduling WiFi reconnection in %u ms\n",
          cfg.exhaustionReconnectDelayMs);
      
      // Defer deletion of the failed AsyncClient to prevent heap corruption
      // Use the centralized deletion scheduler to ensure proper spacing between deletions
      // This prevents concurrent cleanup operations in the AsyncTCP library
      scheduleAsyncClientDeletion(mesh.mScheduler, client, "tcp_err(exhaustion)");
#endif
      // Defer callback execution to avoid crashes in error handler context
      // Execute callbacks after semaphore is released and error handler completes
      // The delay helps prevent endless rapid reconnection loops by giving the TCP server
      // more time to recover and reducing network congestion from multiple retrying nodes
      mesh.addTask([&mesh]() {
        Log(CONNECTION, "tcp_err(): Executing delayed WiFi reconnection after retry exhaustion\n");
        mesh.droppedConnectionCallbacks.execute(0, true);
      }, cfg.exhaustionReconnectDelayMs);
      mesh.semaphoreGive();
    }
  });

  client.onConnect(
      [&mesh, claimed](void *, AsyncClient *client) {
        if (*claimed) {
          // onError has already fired for this client (e.g. an abort
          // caused by losing WiFi right while the TCP handshake was
          // completing): the client is already scheduled for deletion by
          // the retry path. Wrapping it in a new BufferedConnection now
          // would give it two owners at once. This connection did however
          // genuinely succeed at the TCP level (that's why onConnect
          // fired): close it explicitly here, so that when the deferred
          // deletion already scheduled by onError fires, it finds a
          // properly closed client instead of one still "connected" -
          // otherwise we'd fall right back into the bug we're fixing.
          Log(CONNECTION,
              "tcp::connect(): onConnect fired after onError had already "
              "claimed the client - closing the connection and discarding "
              "it\n");
          client->close();
          return;
        }
        *claimed = true;
        if (mesh.semaphoreTake()) {
          Log(CONNECTION, "New STA connection incoming\n");
          auto conn = std::make_shared<T>(client, &mesh, true);
          conn->initTasks();
          mesh.subs.push_back(conn);
          mesh.semaphoreGive();
        }
      },
      NULL);

  client.connect(ip, port);
}
}  // namespace tcp
}  // namespace painlessmesh
#endif
