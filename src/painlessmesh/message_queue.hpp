#ifndef _PAINLESS_MESH_MESSAGE_QUEUE_HPP_
#define _PAINLESS_MESH_MESSAGE_QUEUE_HPP_

#include <vector>
#include <functional>
#include <cstdint>

// Include configuration to get TSTRING definition
// Note: In test environment, configuration will be overridden by test/catch/Arduino.h
// which defines the configuration guard and TSTRING as std::string
#include "painlessmesh/configuration.hpp"

namespace painlessmesh {
namespace queue {

/**
 * Message priority levels for queue management
 * Higher priority messages are preserved over lower priority ones
 */
enum MessagePriority {
  PRIORITY_CRITICAL = 0,  // Life/safety critical (e.g., O2 alarms) - never dropped
  PRIORITY_HIGH = 1,      // Important but not life-threatening
  PRIORITY_NORMAL = 2,    // Regular sensor data
  PRIORITY_LOW = 3        // Non-essential telemetry
};

/**
 * Queue state indicators for status callbacks
 */
enum QueueState {
  QUEUE_EMPTY,           // Queue is empty
  QUEUE_25_PERCENT,      // Queue is 25% full
  QUEUE_50_PERCENT,      // Queue is 50% full
  QUEUE_75_PERCENT,      // Queue is 75% full
  QUEUE_FULL             // Queue is full (at capacity)
};

/**
 * Statistics about queue operations
 */
struct QueueStats {
  uint32_t totalQueued = 0;      // Total messages queued
  uint32_t totalSent = 0;        // Total messages successfully sent
  uint32_t totalDropped = 0;     // Total messages dropped (queue full)
  uint32_t totalFailed = 0;      // Total messages failed after retries
  uint32_t currentSize = 0;      // Current number of messages in queue
  uint32_t peakSize = 0;         // Maximum size reached
};

/**
 * Individual queued message structure
 */
struct QueuedMessage {
  uint32_t id;                   // Unique message ID
  MessagePriority priority;      // Message priority level
  uint32_t timestamp;            // When queued (millis())
  uint32_t attempts;             // Number of send attempts
  TSTRING payload;               // Message content
  TSTRING destination;           // Cloud endpoint/topic/identifier
  
  QueuedMessage() : id(0), priority(PRIORITY_NORMAL), timestamp(0), 
                    attempts(0), payload(""), destination("") {}
};

/**
 * Message Queue Manager
 * 
 * Manages queuing of messages when Internet is unavailable
 * Supports priority-based queuing and optional persistent storage
 */
class MessageQueue {
public:
  typedef std::function<void(QueueState state, uint32_t messageCount)> queueStateCallback_t;
  typedef std::function<bool(const TSTRING& payload, const TSTRING& destination)> sendCallback_t;
  
  MessageQueue();
  ~MessageQueue();
  
  /**
   * Initialize the message queue
   * 
   * @param maxSize Maximum number of messages to queue
   * @param enablePersistence Enable SPIFFS/LittleFS persistence
   * @param storagePath Path to queue storage file
   */
  void init(uint32_t maxSize = 500, bool enablePersistence = false, 
            const TSTRING& storagePath = "/painlessmesh/queue.dat");
  
  /**
   * Queue a message for later delivery
   * 
   * @param payload Message content
   * @param destination Where to send the message
   * @param priority Message priority level
   * @return Message ID if queued successfully, 0 if failed
   */
  uint32_t queueMessage(const TSTRING& payload, const TSTRING& destination,
                        MessagePriority priority = PRIORITY_NORMAL);
  
  /**
   * Attempt to send all queued messages
   * 
   * @param sendCallback Function to actually send the message
   * @return Number of messages successfully sent
   */
  uint32_t flushQueue(sendCallback_t sendCallback);
  
  /**
   * Get number of queued messages
   * 
   * @param priority If specified, count only messages of this priority
   * @return Number of messages
   */
  uint32_t getQueuedMessageCount(MessagePriority priority) const;
  uint32_t getQueuedMessageCount() const;
  
  /**
   * Remove messages older than specified age
   * 
   * @param maxAgeHours Maximum age in hours
   * @return Number of messages removed
   */
  uint32_t pruneQueue(uint32_t maxAgeHours);
  
  /**
   * Clear all messages from queue
   */
  void clear();
  
  /**
   * Get queue statistics
   */
  QueueStats getStats() const { return stats; }
  
  /**
   * Set callback for queue state changes
   */
  void onQueueStateChanged(queueStateCallback_t callback) {
    stateCallback = callback;
  }
  
  /**
   * Set maximum retry attempts for messages
   */
  void setMaxRetryAttempts(uint32_t attempts) {
    maxRetryAttempts = attempts;
  }
  
  /**
   * Get maximum retry attempts
   */
  uint32_t getMaxRetryAttempts() const {
    return maxRetryAttempts;
  }
  
  /**
   * Save queue to persistent storage (if enabled)
   * 
   * @return true if successful
   */
  bool saveToStorage();
  
  /**
   * Load queue from persistent storage (if enabled)
   * 
   * @return Number of messages loaded
   */
  uint32_t loadFromStorage();
  
  /**
   * Check if queue is full
   */
  bool isFull() const {
    return queue.size() >= maxQueueSize;
  }
  
  /**
   * Check if queue is empty
   */
  bool isEmpty() const {
    return queue.empty();
  }

private:
  std::vector<QueuedMessage> queue;
  uint32_t maxQueueSize;
  uint32_t nextMessageId;
  uint32_t maxRetryAttempts;
  bool persistentStorageEnabled;
  TSTRING storagePath;
  QueueStats stats;
  queueStateCallback_t stateCallback;
  
  /**
   * Generate next unique message ID
   */
  uint32_t generateMessageId();
  
  /**
   * Remove low priority messages to make space
   * 
   * @return true if space was freed
   */
  bool makeSpace();
  
  /**
   * Calculate current queue state
   */
  QueueState calculateQueueState() const;
  
  /**
   * Notify state change callback if state changed
   */
  void notifyStateChange();
  
  /**
   * Last notified state (to avoid duplicate notifications)
   */
  QueueState lastNotifiedState;
};

}  // namespace queue
}  // namespace painlessmesh

#endif  // _PAINLESS_MESH_MESSAGE_QUEUE_HPP_
