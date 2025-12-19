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
 * retry up to TCP_CONNECT_MAX_RETRIES times before triggering a full
 * WiFi reconnection cycle.
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
  
  Log(CONNECTION, "tcp::connect(): Attempting connection to port %d (attempt %d/%d)\n",
      port, retryCount + 1, TCP_CONNECT_MAX_RETRIES + 1);
  
  // Store retry count and connection parameters for the error handler
  // We need to capture these by value since they're used in the lambda
  client.onError([&mesh, ip, port, retryCount](void *, AsyncClient *client, int8_t err) {
    if (mesh.semaphoreTake()) {
      Log(CONNECTION, "tcp_err(): error trying to connect %d (attempt %d/%d)\n", 
          err, retryCount + 1, TCP_CONNECT_MAX_RETRIES + 1);
      
      // Check if we have retries left - retry logic only works on real hardware
      // In test environment (PAINLESSMESH_BOOST), fall through to dropped connection
      // Note: ip and port are used in retry logic below, suppress unused warnings for test builds
      (void)ip;
      (void)port;
#if !defined(PAINLESSMESH_BOOST) && (defined(ESP32) || defined(ESP8266))
      if (retryCount < TCP_CONNECT_MAX_RETRIES) {
        // Calculate delay with exponential backoff: base_delay * 2^retryCount
        // This gives increasing time between retries as failures accumulate:
        // - retryCount=0: 1000ms * 1 = 1s
        // - retryCount=1: 1000ms * 2 = 2s
        // - retryCount=2: 1000ms * 4 = 4s
        // - retryCount=3: 1000ms * 8 = 8s (capped at 8)
        // - retryCount=4: 1000ms * 8 = 8s (capped at 8)
        // Cap multiplier at 8 to prevent excessive delays
        uint8_t backoffMultiplier = (retryCount < 3) ? (1U << retryCount) : 8;
        uint32_t retryDelay = TCP_CONNECT_RETRY_DELAY_MS * backoffMultiplier;
        
        Log(CONNECTION, "tcp_err(): Scheduling retry in %u ms (backoff x%d)\n", 
            retryDelay, backoffMultiplier);
        
        // Schedule a retry after a delay using the mesh's task scheduler
        // Note: &mesh is captured by reference because:
        // 1. Mesh is a singleton that lives for the program's lifetime
        // 2. The task scheduler belongs to the mesh, so mesh is always valid when task runs
        // 3. Copying the mesh object is not possible/allowed
        // Recursion depth is strictly bounded by TCP_CONNECT_MAX_RETRIES (default: 5)
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
          TCP_CONNECT_MAX_RETRIES + 1, ip.toString().c_str());
      
      // Block this node temporarily to prevent immediate reconnection to the same unresponsive node
      // This helps when the bridge's TCP server is down but WiFi AP is still running
      // The blocklist is only used during AP filtering, so it won't affect existing connections
      #if !defined(PAINLESSMESH_BOOST)
      // Try to decode nodeId from IP and block it
      // Only works for mesh IPs (format: 10.x.x.1)
      uint32_t failedNodeId = decodeNodeIdFromIP(ip);
      if (failedNodeId != 0) {
        // Note: This requires M to be wifi::Mesh which has blockNodeAfterTCPFailure
        mesh.blockNodeAfterTCPFailure(ip, TCP_FAILURE_BLOCK_DURATION_MS);
      }
      #endif
      
      Log(CONNECTION, "tcp_err(): Scheduling WiFi reconnection in %u ms\n",
          TCP_EXHAUSTION_RECONNECT_DELAY_MS);
      
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
      }, TCP_EXHAUSTION_RECONNECT_DELAY_MS);
      mesh.semaphoreGive();
    }
  });

  client.onConnect(
      [&mesh](void *, AsyncClient *client) {
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
