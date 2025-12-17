#ifndef _PAINLESS_MESH_CONNECTION_HPP_
#define _PAINLESS_MESH_CONNECTION_HPP_

#include <memory>

#include "Arduino.h"
#include "painlessmesh/configuration.hpp"

#include "painlessmesh/buffer.hpp"
#include "painlessmesh/logger.hpp"

extern painlessmesh::logger::LogClass Log;

namespace painlessmesh {
namespace tcp {

// Delay before cleaning up failed AsyncClient after connection error or close
// This prevents crashes when AsyncTCP library is still accessing the client internally
// The AsyncTCP library may take several hundred milliseconds to complete its internal cleanup
// When multiple connections are failing simultaneously (e.g., during mesh connection issues),
// the library needs even more time to safely process multiple cleanup operations
// Increased from 500ms to 1000ms to handle high-churn scenarios more reliably
static const uint32_t TCP_CLIENT_CLEANUP_DELAY_MS = 1000; // 1000ms delay before deleting AsyncClient

// Minimum spacing between consecutive AsyncClient deletions to prevent concurrent cleanup
// When multiple AsyncClients are deleted in rapid succession, the AsyncTCP library's
// internal cleanup routines can interfere with each other, causing heap corruption
// This spacing ensures each deletion completes before the next one begins
static const uint32_t TCP_CLIENT_DELETION_SPACING_MS = 250; // 250ms spacing between deletions

// Global state to track AsyncClient deletion scheduling
// This ensures deletions are spaced out even when multiple deletion requests arrive simultaneously
static uint32_t lastScheduledDeletionTime = 0; // Timestamp when last deletion was scheduled (milliseconds)

// Shared buffer for reading/writing to the buffer
static painlessmesh::buffer::temp_buffer_t shared_buffer;

/**
 * Schedule deletion of an AsyncClient with proper spacing to prevent concurrent cleanups
 * 
 * This function ensures that AsyncClient deletions are spaced out in time to prevent
 * the AsyncTCP library's internal cleanup routines from interfering with each other.
 * 
 * When multiple AsyncClient objects need to be deleted (e.g., during high connection churn
 * or sendToInternet scenarios), scheduling them all with the same delay can cause them to
 * execute concurrently, leading to heap corruption.
 * 
 * This function maintains a global timestamp of when the last deletion was scheduled and
 * calculates an appropriate delay for the new deletion to ensure adequate spacing.
 * 
 * @param scheduler The task scheduler to use for scheduling the deletion
 * @param client The AsyncClient pointer to delete
 * @param logPrefix Prefix for the log message (e.g., "~BufferedConnection" or "tcp_err")
 */
inline void scheduleAsyncClientDeletion(Scheduler* scheduler, AsyncClient* client, const char* logPrefix) {
  using namespace logger;
  
  if (!scheduler) {
    // Fallback: If scheduler not available, delete immediately (risky)
    Log(CONNECTION, "%s: No scheduler available, deleting AsyncClient immediately (risky)\n", logPrefix);
    delete client;
    return;
  }
  
  // Get current time in milliseconds
  uint32_t currentTime = millis();
  
  // Calculate the earliest time this deletion should execute
  // Base delay: TCP_CLIENT_CLEANUP_DELAY_MS (1000ms)
  uint32_t baseDelay = TCP_CLIENT_CLEANUP_DELAY_MS;
  
  // Calculate when this deletion should execute relative to the last scheduled deletion
  // If the last deletion was scheduled recently, we need to add additional spacing
  uint32_t targetDeletionTime = currentTime + baseDelay;
  
  // If there's a recent deletion scheduled, ensure we space out from it
  if (lastScheduledDeletionTime > 0) {
    // Calculate when the next deletion slot is available
    uint32_t nextAvailableSlot = lastScheduledDeletionTime + TCP_CLIENT_DELETION_SPACING_MS;
    
    // If our target deletion time is before the next available slot, push it out
    // Handle millis() rollover by using signed arithmetic
    int32_t timeUntilSlot = (int32_t)(nextAvailableSlot - targetDeletionTime);
    if (timeUntilSlot > 0) {
      targetDeletionTime = nextAvailableSlot;
    }
  }
  
  // Calculate the actual delay from now
  uint32_t actualDelay = targetDeletionTime - currentTime;
  
  // Update the last scheduled deletion time
  lastScheduledDeletionTime = targetDeletionTime;
  
  Log(CONNECTION, "%s: Scheduling AsyncClient deletion in %u ms (spaced from previous deletions)\n", 
      logPrefix, actualDelay);
  
  // Schedule the deletion task
  // Note: Task object is intentionally leaked to keep implementation simple
  // This is acceptable because:
  // 1. Connections are long-lived, destructor calls are infrequent
  // 2. Task object is small (~32-64 bytes) vs preventing critical heap corruption
  // 3. In typical deployments, memory impact is negligible (few KB over months)
  // 4. Alternative cleanup patterns would add significant complexity
  Task* cleanupTask = new Task(actualDelay * TASK_MILLISECOND, TASK_ONCE, [client, logPrefix]() {
    using namespace logger;
    Log(CONNECTION, "%s: Deferred cleanup of AsyncClient executing now\n", logPrefix);
    delete client;
  });
  
  scheduler->addTask(*cleanupTask);
  cleanupTask->enableDelayed();
}

/**
 * Class that performs buffered read and write to the tcp connection
 * (asyncclient)
 *
 * Note that the class expects to be wrapped in a shared_ptr to ensure proper
 * lifetime management. Objects generally will only be destroyed after close is
 * called (and the reading and writing tasks are stopped).
 */
class BufferedConnection
    : public std::enable_shared_from_this<BufferedConnection> {
 public:
  /**
   * Create a buffered connection around the client
   *
   *
   * BufferedConnection takes ownership of the client  and will delete the
   * pointer at the end.
   *
   * One should always call initialize after construction
   */
  BufferedConnection(AsyncClient *client) : client(client) {}

  ~BufferedConnection() {
    using namespace logger;
    Log.remote("~BufferedConnection");
    this->close();
    if (!client->freeable()) {
      client->close(true);
    }
    client->abort();
    
    // Defer deletion of the AsyncClient to prevent heap corruption
    // Use the centralized deletion scheduler to ensure proper spacing between deletions
    // See ISSUE_254_HEAP_CORRUPTION_FIX.md and ASYNCCLIENT_CLEANUP_FIX.md
    scheduleAsyncClientDeletion(mScheduler, client, "~BufferedConnection");
  }

  void initialize(Scheduler *scheduler) {
    // Store scheduler reference for deferred cleanup in destructor
    mScheduler = scheduler;
    
    auto self = this->shared_from_this();
    sentBufferTask.set(TASK_SECOND, TASK_FOREVER, [self]() {
      if (!self->sentBuffer.empty() && self->client->canSend()) {
        auto ret = self->writeNext();
        if (ret)
          self->sentBufferTask.forceNextIteration();
        else
          self->sentBufferTask.delay(100 * TASK_MILLISECOND);
      }
    });
    scheduler->addTask(sentBufferTask);
    sentBufferTask.enableDelayed();

    readBufferTask.set(TASK_SECOND, TASK_FOREVER, [self]() {
      if (!self->receiveBuffer.empty()) {
        TSTRING frnt = self->receiveBuffer.front();
        self->receiveBuffer.pop_front();
        if (!self->receiveBuffer.empty())
          self->readBufferTask.forceNextIteration();
        if (self->receiveCallback) self->receiveCallback(frnt);
      }
    });
    scheduler->addTask(readBufferTask);
    readBufferTask.enableDelayed();

    client->onAck(
        [self](void *arg, AsyncClient *client, size_t len, uint32_t time) {
          self->sentBufferTask.forceNextIteration();
        },
        NULL);

    client->onData(
        [self](void *arg, AsyncClient *client, void *data, size_t len) {
          self->receiveBuffer.push(static_cast<const char *>(data), len,
                                   shared_buffer);
          // Signal that we are done
          self->client->ack(len);
          self->readBufferTask.forceNextIteration();
        },
        NULL);

    client->onDisconnect(
        [self](void *arg, AsyncClient *client) { self->close(); }, NULL);
  }

  void close() {
    if (!mConnected) return;

    // Disable tasks and callbacks
    this->sentBufferTask.setCallback(NULL);
    this->sentBufferTask.disable();
    this->readBufferTask.setCallback(NULL);
    this->readBufferTask.disable();

    this->client->onData(NULL, NULL);
    this->client->onAck(NULL, NULL);
    this->client->onDisconnect(NULL, NULL);
    this->client->onError(NULL, NULL);

    if (client->connected()) {
      client->close();
    }

    receiveBuffer.clear();
    sentBuffer.clear();

    if (disconnectCallback) disconnectCallback();

    receiveCallback = NULL;
    disconnectCallback = NULL;

    mConnected = false;
  }

  bool write(const TSTRING &data, bool priority = false) {
    sentBuffer.push(data, priority);
    sentBufferTask.forceNextIteration();
    return true;
  }
  
  /**
   * Write data with explicit priority level (0-3)
   * 
   * \param data The data to send
   * \param priorityLevel Priority level: 0=CRITICAL, 1=HIGH, 2=NORMAL, 3=LOW
   */
  bool writeWithPriority(const TSTRING &data, uint8_t priorityLevel) {
    sentBuffer.pushWithPriority(data, priorityLevel);
    sentBufferTask.forceNextIteration();
    return true;
  }

  void onDisconnect(std::function<void()> callback) {
    disconnectCallback = callback;
  }

  void onReceive(std::function<void(TSTRING)> callback) {
    receiveCallback = callback;
  }

  bool connected() { return mConnected; }

 protected:
  bool mConnected = true;

  AsyncClient *client;
  Scheduler *mScheduler = nullptr; // Scheduler for deferred AsyncClient cleanup

  std::function<void(TSTRING)> receiveCallback;
  std::function<void()> disconnectCallback;

  painlessmesh::buffer::ReceiveBuffer<TSTRING> receiveBuffer;
  painlessmesh::buffer::SentBuffer<TSTRING> sentBuffer;

  bool writeNext() {
    if (sentBuffer.empty()) {
      return false;
    }
    auto len = sentBuffer.requestLength(shared_buffer.length);
    auto snd_len = client->space();
    if (len > snd_len) len = snd_len;
    if (len > 0) {
      // sentBuffer.read(len, shared_buffer);
      // auto written = client->write(shared_buffer.buffer, len, 1);
      auto data_ptr = sentBuffer.readPtr(len);
      auto written = client->write(data_ptr, len, 1);
      if (written == len) {
        // Get priority before freeing the read buffer
        uint8_t msgPriority = sentBuffer.getLastReadPriority();
        
        // Call send() for high priority messages (CRITICAL=0, HIGH=1)
        // This ensures they are transmitted immediately
        if (msgPriority <= 1) {
          client->send();
        }
        
        sentBuffer.freeRead();
        sentBufferTask.forceNextIteration();
        return true;
      } else if (written == 0) {
        return false;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  Task sentBufferTask;
  Task readBufferTask;

  template <typename T>
  std::shared_ptr<T> shared_from(T *derived) {
    return std::static_pointer_cast<T>(shared_from_this());
  }
};  // namespace tcp
};  // namespace tcp
};  // namespace painlessmesh

#endif
