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

inline uint32_t encodeNodeId(const uint8_t *hwaddr) {
  using namespace painlessmesh::logger;
  Log(GENERAL, "encodeNodeId():\n");
  uint32_t value = 0;

  value |= hwaddr[2] << 24;  // Big endian (aka "network order"):
  value |= hwaddr[3] << 16;
  value |= hwaddr[4] << 8;
  value |= hwaddr[5];
  return value;
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
        // Deleting from within the error callback can cause use-after-free issues
        // as the AsyncTCP library may still be referencing the object
        // Use TCP_CLIENT_CLEANUP_DELAY_MS to give AsyncTCP library time to complete
        // its internal cleanup before we delete the object
        // Note: client is captured by value (pointer copy) and we are the sole owner
        mesh.addTask([client]() {
          Log(CONNECTION, "tcp_err(): Cleaning up failed AsyncClient (retry path)\n");
          delete client;
        }, TCP_CLIENT_CLEANUP_DELAY_MS);
        
        mesh.semaphoreGive();
        return;
      }
      
      // All retries exhausted - schedule delayed reconnection
      // Adding a significant delay before reconnection prevents rapid reconnection loops
      // when the TCP server is persistently unavailable or overloaded
      Log(CONNECTION, "tcp_err(): All %d retries exhausted, scheduling WiFi reconnection in %u ms\n",
          TCP_CONNECT_MAX_RETRIES + 1, TCP_EXHAUSTION_RECONNECT_DELAY_MS);
      
      // Defer deletion of the failed AsyncClient to prevent heap corruption
      // Deleting from within the error callback can cause use-after-free issues
      // as the AsyncTCP library may still be referencing the object
      // Use TCP_CLIENT_CLEANUP_DELAY_MS to give AsyncTCP library time to complete
      // its internal cleanup before we delete the object
      // Note: client is captured by value (pointer copy) and we are the sole owner
      mesh.addTask([client]() {
        Log(CONNECTION, "tcp_err(): Cleaning up failed AsyncClient (exhaustion path)\n");
        delete client;
      }, TCP_CLIENT_CLEANUP_DELAY_MS);
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
