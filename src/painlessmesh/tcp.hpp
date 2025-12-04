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
static const uint8_t TCP_CONNECT_MAX_RETRIES = 3;      // Max retry attempts before giving up
static const uint32_t TCP_CONNECT_RETRY_DELAY_MS = 500; // Delay between retry attempts
static const uint32_t TCP_CONNECT_STABILIZATION_DELAY_MS = 100; // Delay after IP acquisition

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
 * Establish TCP connection with retry mechanism
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
#if !defined(PAINLESSMESH_BOOST) && (defined(ESP32) || defined(ESP8266))
      if (retryCount < TCP_CONNECT_MAX_RETRIES) {
        Log(CONNECTION, "tcp_err(): Scheduling retry in %u ms\n", TCP_CONNECT_RETRY_DELAY_MS);
        
        // Schedule a retry after a delay
        // Use addTask to schedule retry without blocking
        mesh.addTask(TCP_CONNECT_RETRY_DELAY_MS, TASK_ONCE, [&mesh, ip, port, retryCount]() {
          Log(CONNECTION, "tcp_err(): Retrying TCP connection...\n");
          
          // Create a new AsyncClient for the retry
          AsyncClient *pRetryConn = new AsyncClient();
          connect<T, M>((*pRetryConn), ip, port, mesh, retryCount + 1);
        });
        
        mesh.semaphoreGive();
        return;
      }
      
      // All retries exhausted, trigger full reconnection
      Log(CONNECTION, "tcp_err(): All %d retries exhausted, triggering WiFi reconnection\n",
          TCP_CONNECT_MAX_RETRIES + 1);
#endif
      mesh.droppedConnectionCallbacks.execute(0, true);
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
