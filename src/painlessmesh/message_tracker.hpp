#ifndef _PAINLESS_MESH_MESSAGE_TRACKER_HPP_
#define _PAINLESS_MESH_MESSAGE_TRACKER_HPP_

#include <map>
#include <algorithm>
#include "painlessmesh/configuration.hpp"
#include "painlessmesh/logger.hpp"

// External logger instance
extern painlessmesh::logger::LogClass Log;

namespace painlessmesh {

/**
 * Key for tracking messages by their unique combination of ID and origin node
 */
struct MessageKey {
  uint32_t messageId;
  uint32_t originNode;
  
  bool operator<(const MessageKey& other) const {
    if (messageId != other.messageId) {
      return messageId < other.messageId;
    }
    return originNode < other.originNode;
  }
  
  bool operator==(const MessageKey& other) const {
    return messageId == other.messageId && originNode == other.originNode;
  }
};

/**
 * Tracked message entry with timestamp and acknowledgment status
 */
struct TrackedMessage {
  uint32_t timestamp;     // When the message was first processed (millis)
  bool acknowledged;      // Whether the message has been acknowledged
  
  TrackedMessage() : timestamp(0), acknowledged(false) {}
  TrackedMessage(uint32_t ts) : timestamp(ts), acknowledged(false) {}
};

/**
 * MessageTracker - Prevents duplicate message processing and tracks acknowledgments
 * 
 * This class provides efficient tracking of processed messages to prevent
 * duplicate processing in mesh networks. It supports:
 * - Configurable maximum tracked messages (memory-efficient for ESP8266)
 * - Automatic cleanup of expired entries
 * - Acknowledgment tracking for reliable delivery
 * - Thread-safe consideration for ESP32
 * 
 * Example usage:
 * \code
 * MessageTracker tracker(500, 60000);  // Max 500 messages, 60s timeout
 * 
 * uint32_t msgId = 12345;
 * uint32_t originNode = 67890;
 * 
 * // Check if message was already processed
 * if (!tracker.isProcessed(msgId, originNode)) {
 *     // Process the message
 *     processMessage(msg);
 *     
 *     // Mark as processed
 *     tracker.markProcessed(msgId, originNode);
 * }
 * 
 * // Later, mark as acknowledged
 * tracker.markAcknowledged(msgId, originNode);
 * 
 * // Periodically cleanup old entries
 * tracker.cleanup();
 * \endcode
 */
class MessageTracker {
public:
  /**
   * Constructor
   * @param maxMessages Maximum number of messages to track (default: 500)
   * @param timeoutMs Timeout in milliseconds for automatic cleanup (default: 60000)
   */
  MessageTracker(uint16_t maxMessages = 500, uint32_t timeoutMs = 60000)
    : maxTrackedMessages(maxMessages), messageTimeoutMs(timeoutMs) {}
  
  /**
   * Check if a message has already been processed
   * @param messageId The unique message identifier
   * @param originNode The node that originated the message
   * @return true if the message was already processed, false otherwise
   */
  bool isProcessed(uint32_t messageId, uint32_t originNode) {
    MessageKey key = {messageId, originNode};
    auto it = trackedMessages.find(key);
    return it != trackedMessages.end();
  }
  
  /**
   * Mark a message as processed
   * If the tracker is at capacity, oldest entries will be removed first.
   * @param messageId The unique message identifier
   * @param originNode The node that originated the message
   */
  void markProcessed(uint32_t messageId, uint32_t originNode) {
    MessageKey key = {messageId, originNode};
    uint32_t currentTime = millis();
    
    // Check if already exists
    auto it = trackedMessages.find(key);
    if (it != trackedMessages.end()) {
      // Update timestamp but preserve acknowledged status
      it->second.timestamp = currentTime;
      Log(logger::GENERAL, "MessageTracker: Updated message %u from node %u\n",
          messageId, originNode);
      return;
    }
    
    // Handle capacity 0 case - don't add new entries
    if (maxTrackedMessages == 0) {
      Log(logger::GENERAL, "MessageTracker: Cannot track message %u (capacity=0)\n",
          messageId);
      return;
    }
    
    // Enforce memory limits before adding new entry
    if (trackedMessages.size() >= maxTrackedMessages) {
      enforceMemoryLimit();
    }
    
    // Add new entry
    trackedMessages[key] = TrackedMessage(currentTime);
    Log(logger::GENERAL, "MessageTracker: Tracked message %u from node %u (size=%zu)\n",
        messageId, originNode, trackedMessages.size());
  }
  
  /**
   * Mark a message as acknowledged
   * @param messageId The unique message identifier
   * @param originNode The node that originated the message
   * @return true if the message was found and marked, false otherwise
   */
  bool markAcknowledged(uint32_t messageId, uint32_t originNode) {
    MessageKey key = {messageId, originNode};
    auto it = trackedMessages.find(key);
    
    if (it != trackedMessages.end()) {
      it->second.acknowledged = true;
      Log(logger::GENERAL, "MessageTracker: Acknowledged message %u from node %u\n",
          messageId, originNode);
      return true;
    }
    
    Log(logger::GENERAL, "MessageTracker: Message %u from node %u not found for acknowledgment\n",
        messageId, originNode);
    return false;
  }
  
  /**
   * Check if a message has been acknowledged
   * @param messageId The unique message identifier
   * @param originNode The node that originated the message
   * @return true if acknowledged, false if not found or not acknowledged
   */
  bool isAcknowledged(uint32_t messageId, uint32_t originNode) {
    MessageKey key = {messageId, originNode};
    auto it = trackedMessages.find(key);
    
    if (it != trackedMessages.end()) {
      return it->second.acknowledged;
    }
    
    return false;
  }
  
  /**
   * Cleanup expired entries based on the configured timeout
   * @return Number of entries removed
   */
  uint32_t cleanup() {
    uint32_t currentTime = millis();
    uint32_t removedCount = 0;
    
    auto it = trackedMessages.begin();
    while (it != trackedMessages.end()) {
      // Handle millis() overflow - if currentTime < timestamp, assume overflow
      uint32_t age;
      if (currentTime >= it->second.timestamp) {
        age = currentTime - it->second.timestamp;
      } else {
        // Overflow occurred
        age = (0xFFFFFFFF - it->second.timestamp) + currentTime + 1;
      }
      
      if (age > messageTimeoutMs) {
        it = trackedMessages.erase(it);
        removedCount++;
      } else {
        ++it;
      }
    }
    
    if (removedCount > 0) {
      Log(logger::GENERAL, "MessageTracker: Cleaned up %u expired entries (size=%zu)\n",
          removedCount, trackedMessages.size());
    }
    
    return removedCount;
  }
  
  /**
   * Get the current number of tracked messages
   * @return Number of entries in the tracker
   */
  size_t size() const {
    return trackedMessages.size();
  }
  
  /**
   * Check if the tracker is empty
   * @return true if no messages are being tracked
   */
  bool empty() const {
    return trackedMessages.empty();
  }
  
  /**
   * Clear all tracked messages
   */
  void clear() {
    trackedMessages.clear();
    Log(logger::GENERAL, "MessageTracker: Cleared all entries\n");
  }
  
  /**
   * Get the maximum number of tracked messages
   * @return Configured maximum
   */
  uint16_t getMaxMessages() const {
    return maxTrackedMessages;
  }
  
  /**
   * Get the timeout value in milliseconds
   * @return Configured timeout
   */
  uint32_t getTimeoutMs() const {
    return messageTimeoutMs;
  }
  
  /**
   * Set a new maximum for tracked messages
   * If current size exceeds new max, oldest entries will be removed
   * @param maxMessages New maximum
   */
  void setMaxMessages(uint16_t maxMessages) {
    maxTrackedMessages = maxMessages;
    
    // Enforce new limit if needed
    while (trackedMessages.size() > maxTrackedMessages) {
      enforceMemoryLimit();
    }
  }
  
  /**
   * Set a new timeout value
   * @param timeoutMs New timeout in milliseconds
   */
  void setTimeoutMs(uint32_t timeoutMs) {
    messageTimeoutMs = timeoutMs;
  }

private:
  std::map<MessageKey, TrackedMessage> trackedMessages;
  uint16_t maxTrackedMessages;
  uint32_t messageTimeoutMs;
  
  /**
   * Enforce memory limit by removing the oldest entry
   * @return true if an entry was removed, false if tracker was empty
   */
  bool enforceMemoryLimit() {
    if (trackedMessages.empty()) {
      return false;
    }
    
    // Find the oldest entry
    auto oldest = trackedMessages.begin();
    uint32_t oldestTimestamp = oldest->second.timestamp;
    
    for (auto it = trackedMessages.begin(); it != trackedMessages.end(); ++it) {
      if (it->second.timestamp < oldestTimestamp) {
        oldest = it;
        oldestTimestamp = it->second.timestamp;
      }
    }
    
    Log(logger::GENERAL, "MessageTracker: Evicting oldest entry (msg=%u, node=%u) to enforce limit\n",
        oldest->first.messageId, oldest->first.originNode);
    
    trackedMessages.erase(oldest);
    return true;
  }
};

}  // namespace painlessmesh

#endif  // _PAINLESS_MESH_MESSAGE_TRACKER_HPP_
