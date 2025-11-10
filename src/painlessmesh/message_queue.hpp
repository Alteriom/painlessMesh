#ifndef _PAINLESS_MESH_MESSAGE_QUEUE_HPP_
#define _PAINLESS_MESH_MESSAGE_QUEUE_HPP_

#include <vector>
#include <algorithm>
#include "painlessmesh/configuration.hpp"
#include "painlessmesh/logger.hpp"

namespace painlessmesh {

// External logger instance
extern logger::LogClass Log;

/**
 * Message priority levels for queue management
 * Higher priority messages are preserved when queue is full
 */
enum MessagePriority {
  PRIORITY_CRITICAL = 0,  // Life/safety critical (O2 alarms, fire alarms)
  PRIORITY_HIGH     = 1,  // Important but not life-threatening
  PRIORITY_NORMAL   = 2,  // Regular sensor data
  PRIORITY_LOW      = 3   // Non-essential telemetry
};

/**
 * Queue state for monitoring and callbacks
 */
enum QueueState {
  QUEUE_EMPTY,       // Queue is empty
  QUEUE_NORMAL,      // Queue has space available
  QUEUE_75_PERCENT,  // Queue is 75% full - warning threshold
  QUEUE_FULL         // Queue is full - dropping low priority messages
};

/**
 * Queued message structure
 * Contains all metadata needed for reliable message delivery
 */
struct QueuedMessage {
  uint32_t id;              // Unique message ID
  MessagePriority priority; // Message priority level
  uint32_t timestamp;       // When message was queued (millis)
  uint32_t attempts;        // Number of send attempts
  TSTRING payload;          // Message content
  TSTRING destination;      // Cloud endpoint/topic (optional metadata)
  
  QueuedMessage() : id(0), priority(PRIORITY_NORMAL), timestamp(0), attempts(0) {}
  
  QueuedMessage(uint32_t msgId, MessagePriority prio, uint32_t ts, 
                const TSTRING& data, const TSTRING& dest = "")
    : id(msgId), priority(prio), timestamp(ts), attempts(0), 
      payload(data), destination(dest) {}
};

/**
 * Message queue statistics
 */
struct QueueStats {
  uint32_t totalQueued = 0;   // Total messages ever queued
  uint32_t totalSent = 0;     // Total messages successfully sent
  uint32_t totalDropped = 0;  // Total messages dropped (queue full)
  uint32_t currentSize = 0;   // Current queue size
  uint32_t maxSize = 0;       // Configured max size
  
  // Priority-specific counts
  uint32_t criticalQueued = 0;
  uint32_t highQueued = 0;
  uint32_t normalQueued = 0;
  uint32_t lowQueued = 0;
};

// Queue state change callback type
typedef std::function<void(QueueState state, uint32_t messageCount)> queueStateChangedCallback_t;

/**
 * Message Queue for offline/Internet-unavailable mode
 * 
 * Provides priority-based message queueing with support for:
 * - Priority levels (CRITICAL messages never dropped)
 * - Queue size limits with intelligent eviction
 * - Statistics tracking
 * - State change callbacks
 * 
 * Example usage:
 * \code
 * MessageQueue queue(1000);  // Max 1000 messages
 * 
 * // Queue a critical message
 * uint32_t msgId = queue.enqueue(PRIORITY_CRITICAL, "alarm_data", "mqtt://...");
 * 
 * // Get queue size
 * uint32_t size = queue.size();
 * 
 * // Get all messages (for sending)
 * std::vector<QueuedMessage> messages = queue.getMessages();
 * 
 * // Remove sent message
 * queue.remove(msgId);
 * \endcode
 */
class MessageQueue {
public:
  /**
   * Constructor
   * @param maxSize Maximum number of messages in queue
   */
  explicit MessageQueue(uint32_t maxSize = 1000) 
    : maxQueueSize(maxSize), nextMessageId(1) {
    messages.reserve(maxSize);
  }
  
  /**
   * Enqueue a message with priority
   * 
   * @param priority Message priority level
   * @param payload Message content
   * @param destination Optional destination metadata
   * @return Message ID if queued, 0 if dropped
   */
  uint32_t enqueue(MessagePriority priority, const TSTRING& payload, 
                   const TSTRING& destination = "") {
    // Check if queue is full
    if (messages.size() >= maxQueueSize) {
      // Try to make space by removing low priority messages
      if (!makeSpace(priority)) {
        stats.totalDropped++;
        Log(logger::ERROR, "MessageQueue: Failed to queue message (queue full, priority too low)\n");
        return 0;  // Could not queue
      }
    }
    
    uint32_t msgId = nextMessageId++;
    uint32_t timestamp = millis();
    
    QueuedMessage msg(msgId, priority, timestamp, payload, destination);
    messages.push_back(msg);
    
    // Update statistics
    stats.totalQueued++;
    stats.currentSize = messages.size();
    updatePriorityStats();
    
    // Check for state change
    checkStateChange();
    
    Log(logger::GENERAL, "MessageQueue: Enqueued message #%u (priority=%d, size=%u/%u)\n",
        msgId, priority, messages.size(), maxQueueSize);
    
    return msgId;
  }
  
  /**
   * Remove a message from the queue
   * @param messageId ID of message to remove
   * @return true if message was found and removed
   */
  bool remove(uint32_t messageId) {
    auto it = std::find_if(messages.begin(), messages.end(),
                          [messageId](const QueuedMessage& msg) {
                            return msg.id == messageId;
                          });
    
    if (it != messages.end()) {
      messages.erase(it);
      stats.totalSent++;
      stats.currentSize = messages.size();
      updatePriorityStats();
      checkStateChange();
      
      Log(logger::GENERAL, "MessageQueue: Removed message #%u (size=%u)\n",
          messageId, messages.size());
      return true;
    }
    
    return false;
  }
  
  /**
   * Get all queued messages
   * @return Vector of queued messages (ordered by timestamp)
   */
  std::vector<QueuedMessage> getMessages() const {
    return messages;
  }
  
  /**
   * Get current queue size
   * @return Number of messages in queue
   */
  uint32_t size() const {
    return messages.size();
  }
  
  /**
   * Get count of messages with specific priority
   * @param priority Priority level to count
   * @return Number of messages with that priority
   */
  uint32_t size(MessagePriority priority) const {
    return std::count_if(messages.begin(), messages.end(),
                        [priority](const QueuedMessage& msg) {
                          return msg.priority == priority;
                        });
  }
  
  /**
   * Check if queue is empty
   * @return true if no messages queued
   */
  bool empty() const {
    return messages.empty();
  }
  
  /**
   * Clear all messages from queue
   */
  void clear() {
    messages.clear();
    stats.currentSize = 0;
    updatePriorityStats();
    checkStateChange();
    Log(logger::GENERAL, "MessageQueue: Cleared all messages\n");
  }
  
  /**
   * Get queue statistics
   * @return QueueStats structure
   */
  QueueStats getStats() const {
    return stats;
  }
  
  /**
   * Set queue state change callback
   * @param callback Function to call when queue state changes
   */
  void onStateChanged(queueStateChangedCallback_t callback) {
    stateChangedCallback = callback;
  }
  
  /**
   * Increment send attempt counter for a message
   * @param messageId ID of message
   * @return New attempt count, or 0 if message not found
   */
  uint32_t incrementAttempts(uint32_t messageId) {
    auto it = std::find_if(messages.begin(), messages.end(),
                          [messageId](const QueuedMessage& msg) {
                            return msg.id == messageId;
                          });
    
    if (it != messages.end()) {
      it->attempts++;
      return it->attempts;
    }
    
    return 0;
  }
  
  /**
   * Remove old messages (for queue pruning)
   * @param maxAgeMs Maximum age in milliseconds
   * @return Number of messages removed
   */
  uint32_t pruneOldMessages(uint32_t maxAgeMs) {
    uint32_t currentTime = millis();
    uint32_t removedCount = 0;
    
    auto it = messages.begin();
    while (it != messages.end()) {
      if (currentTime - it->timestamp > maxAgeMs) {
        it = messages.erase(it);
        removedCount++;
      } else {
        ++it;
      }
    }
    
    if (removedCount > 0) {
      stats.currentSize = messages.size();
      updatePriorityStats();
      checkStateChange();
      Log(logger::GENERAL, "MessageQueue: Pruned %u old messages\n", removedCount);
    }
    
    return removedCount;
  }

private:
  std::vector<QueuedMessage> messages;
  uint32_t maxQueueSize;
  uint32_t nextMessageId;
  QueueStats stats;
  QueueState currentState = QUEUE_EMPTY;
  queueStateChangedCallback_t stateChangedCallback;
  
  /**
   * Try to make space in queue by removing low priority messages
   * @param newPriority Priority of message trying to be added
   * @return true if space was made
   */
  bool makeSpace(MessagePriority newPriority) {
    // CRITICAL messages can always evict lower priority
    // HIGH messages can evict NORMAL and LOW
    // NORMAL messages can only evict LOW
    // LOW messages cannot evict anything
    
    if (newPriority == PRIORITY_LOW) {
      return false;  // LOW priority can't evict anything
    }
    
    // Try to remove lowest priority messages first
    for (int targetPriority = PRIORITY_LOW; targetPriority > newPriority; targetPriority--) {
      auto it = std::find_if(messages.begin(), messages.end(),
                            [targetPriority](const QueuedMessage& msg) {
                              return msg.priority == static_cast<MessagePriority>(targetPriority);
                            });
      
      if (it != messages.end()) {
        Log(logger::GENERAL, "MessageQueue: Evicting message #%u (priority=%d) to make space\n",
            it->id, it->priority);
        messages.erase(it);
        stats.totalDropped++;
        return true;
      }
    }
    
    return false;
  }
  
  /**
   * Update priority-specific statistics
   */
  void updatePriorityStats() {
    stats.criticalQueued = size(PRIORITY_CRITICAL);
    stats.highQueued = size(PRIORITY_HIGH);
    stats.normalQueued = size(PRIORITY_NORMAL);
    stats.lowQueued = size(PRIORITY_LOW);
  }
  
  /**
   * Check if queue state has changed and fire callback
   */
  void checkStateChange() {
    QueueState newState;
    
    if (messages.empty()) {
      newState = QUEUE_EMPTY;
    } else if (messages.size() >= maxQueueSize) {
      newState = QUEUE_FULL;
    } else if (messages.size() >= maxQueueSize * 3 / 4) {
      newState = QUEUE_75_PERCENT;
    } else {
      newState = QUEUE_NORMAL;
    }
    
    if (newState != currentState) {
      currentState = newState;
      if (stateChangedCallback) {
        stateChangedCallback(currentState, messages.size());
      }
    }
  }
};

}  // namespace painlessmesh

#endif  // _PAINLESS_MESH_MESSAGE_QUEUE_HPP_
