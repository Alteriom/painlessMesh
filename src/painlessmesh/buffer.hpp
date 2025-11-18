#ifndef _PAINLESS_MESH_BUFFER_HPP_
#define _PAINLESS_MESH_BUFFER_HPP_

#include <list>
#include <map>
#include <queue>

#include "Arduino.h"
#include "painlessmesh/configuration.hpp"

#ifndef TCP_MSS
#define TCP_MSS 1024
#endif

namespace painlessmesh {
namespace buffer {

// Temporary buffer used by ReceiveBuffer and SentBuffer
struct temp_buffer_t {
  size_t length = TCP_MSS;
  char buffer[TCP_MSS];
};

/**
 * \brief ReceivedBuffer handles cstrings and stores them as strings
 */
template <class T>
class ReceiveBuffer {
 public:
  ReceiveBuffer() { buffer = T(); }

  /**
   * Push a message into the buffer
   */
  void push(const char *cstr, size_t length, temp_buffer_t &buf) {
    auto data_ptr = cstr;
    do {
      auto len = strnlen(data_ptr, length);
      do {
        auto read_len = (std::min)(len, buf.length);
        memcpy(buf.buffer, data_ptr, read_len);
        buf.buffer[read_len] = '\0';
        auto newBuffer = T(buf.buffer);
        stringAppend(buffer, newBuffer);
        len -= newBuffer.length();
        length -= newBuffer.length();
        data_ptr += newBuffer.length() * sizeof(char);
      } while (len > 0);
      if (length > 0) {
        // Skip/remove the '\0' between the messages
        length -= 1;
        data_ptr += 1 * sizeof(char);
        if (buffer.length() > 0) {  // skip empty buffers
          jsonStrings.push_back(buffer);
          buffer = T();
        }
      }
    } while (length > 0);
  }

  /**
   * Get the oldest message from the buffer
   */
  T front() {
    if (!empty()) return jsonStrings.front();
    return T();
  }

  /**
   * Remove the oldest message from the buffer
   */
  void pop_front() { jsonStrings.pop_front(); }

  /**
   * Is the buffer empty
   */
  bool empty() { return jsonStrings.empty(); }

  /**
   * Clear the buffer
   */
  void clear() {
    jsonStrings.clear();
    buffer = T();
  }

 private:
  T buffer;
  std::list<T> jsonStrings;

  /**
   * Helper function to deal with difference Arduino String
   * and std::string
   */
  inline void stringAppend(T &buffer, T &newBuffer) {
    buffer.concat(newBuffer);
  };
};

#ifdef PAINLESSMESH_ENABLE_STD_STRING
template <>
inline void ReceiveBuffer<std::string>::stringAppend(std::string &buffer,
                                                     std::string &newBuffer) {
  buffer.append(newBuffer);
}
#endif

/**
 * Structure to hold a message with its priority level
 */
template <class T>
struct PrioritizedMessage {
  T message;
  uint8_t priority;  // 0=CRITICAL, 1=HIGH, 2=NORMAL, 3=LOW
  
  PrioritizedMessage(const T& msg, uint8_t prio = 2) : message(msg), priority(prio) {}
};

/**
 * \brief SentBuffer stores messages (strings) and allows them to be read in any
 * length with priority-based scheduling
 */
template <class T>
class SentBuffer {
 public:
  SentBuffer() {
    current_read_iterator = prioritizedMessages.end();
  };

  /**
   * push a message into the buffer with multi-level priority support.
   *
   * \param message The message to queue
   * \param priority Whether this is a high priority message (legacy bool API)
   *
   * Legacy API: High priority messages (true) will be sent to the front of the buffer
   * This maintains backward compatibility with existing code.
   */
  void push(const T &message, bool priority = false) {
    // Legacy API: map bool to uint8_t priority (false=2 NORMAL, true=1 HIGH)
    uint8_t priorityLevel = priority ? 1 : 2;
    pushWithPriority(message, priorityLevel);
  }

  /**
   * push a message with explicit priority level (0-3)
   *
   * \param message The message to queue
   * \param priorityLevel Priority level: 0=CRITICAL, 1=HIGH, 2=NORMAL, 3=LOW
   *
   * Messages are scheduled in priority order. Within same priority, FIFO order is maintained.
   */
  void pushWithPriority(const T &message, uint8_t priorityLevel) {
    // Clamp priority to valid range
    if (priorityLevel > 3) priorityLevel = 3;
    
    prioritizedMessages.push_back(PrioritizedMessage<T>(message, priorityLevel));
    
    // Track statistics
    totalMessagesQueued++;
    switch(priorityLevel) {
      case 0: criticalQueued++; break;
      case 1: highQueued++; break;
      case 2: normalQueued++; break;
      case 3: lowQueued++; break;
    }
  }

  /**
   * Request whether the passed length is readable
   *
   * Returns the actual length available (<= the requested length
   */
  size_t requestLength(size_t buffer_length) {
    // Use highest priority message available
    auto* msg = getNextMessage();
    if (!msg)
      return 0;
    else
      // String.toCharArray automatically turns the last character into
      // a \0, we need the extra space to deal with that annoyance
      return (std::min)(buffer_length - 1, msg->length() + 1);
  }

  /**
   * Read the given length into the passed buffer (copy mode)
   *
   * Note the user should first make sure the requested length is available
   * using `SentBuffer.requestLength()`, otherwise this function might fail.
   * Note that if multiple messages are read then they are separated using '\0'.
   * 
   * Performance note: Consider using readPtr() for zero-copy access when possible.
   */
  void read(size_t length, temp_buffer_t &buf) {
    // Note that toCharrArray always null terminates
    // independent of whether the whole string was read so we use one extra
    // space
    auto* msg = getNextMessage();
    if (msg) {
      msg->toCharArray(buf.buffer, length + 1);
      last_read_size = length;
      last_read_priority = getCurrentMessagePriority();
    }
  }

  /**
   * Returns a pointer directly to the highest priority message
   *
   * Note the user should first make sure the requested length is available
   * using `SentBuffer.requestLength()`, otherwise this function might fail.
   * Note that if multiple messages are read then they are separated using '\0'.
   */
  const char *readPtr(size_t length) {
    auto* msg = getNextMessage();
    if (msg) {
      last_read_size = length;
      last_read_priority = getCurrentMessagePriority();
      return msg->c_str();
    }
    return nullptr;
  }

  /**
   * Clear the previously read messages from the buffer.
   *
   * Should be called after a call of read() to clear the buffer.
   */
  void freeRead() {
    if (prioritizedMessages.empty()) {
      last_read_size = 0;
      current_read_iterator = prioritizedMessages.end();
      return;
    }
    
    // Find and remove the message that was just read (highest priority)
    auto it = findHighestPriorityMessage();
    if (it != prioritizedMessages.end()) {
      auto& msg = it->message;
      
      if (last_read_size == msg.length() + 1) {
        // Whole message was read, remove it
        // Track statistics
        switch(it->priority) {
          case 0: criticalSent++; break;
          case 1: highSent++; break;
          case 2: normalSent++; break;
          case 3: lowSent++; break;
        }
        prioritizedMessages.erase(it);
        current_read_iterator = prioritizedMessages.end();  // Reset iterator
        clean = true;
      } else {
        // Partial message read, remove the read portion
        stringEraseFront(msg, last_read_size);
        // Keep current_read_iterator pointing to this message for next read
        clean = false;
      }
    }
    last_read_size = 0;
  }

  bool empty() { return prioritizedMessages.empty(); }

  void clear() { 
    prioritizedMessages.clear(); 
    current_read_iterator = prioritizedMessages.end();
    totalMessagesQueued = 0;
    criticalQueued = highQueued = normalQueued = lowQueued = 0;
    criticalSent = highSent = normalSent = lowSent = 0;
  }

  size_t size() { return prioritizedMessages.size(); }
  
  /**
   * Get priority of the last read message
   */
  uint8_t getLastReadPriority() const { return last_read_priority; }
  
  /**
   * Get statistics about queued and sent messages
   */
  struct SendStats {
    uint32_t totalQueued;
    uint32_t criticalQueued;
    uint32_t highQueued;
    uint32_t normalQueued;
    uint32_t lowQueued;
    uint32_t criticalSent;
    uint32_t highSent;
    uint32_t normalSent;
    uint32_t lowSent;
  };
  
  SendStats getStats() const {
    SendStats stats;
    stats.totalQueued = totalMessagesQueued;
    stats.criticalQueued = criticalQueued;
    stats.highQueued = highQueued;
    stats.normalQueued = normalQueued;
    stats.lowQueued = lowQueued;
    stats.criticalSent = criticalSent;
    stats.highSent = highSent;
    stats.normalSent = normalSent;
    stats.lowSent = lowSent;
    return stats;
  }

 private:
  size_t last_read_size = 0;
  uint8_t last_read_priority = 2;  // NORMAL default
  bool clean = true;
  std::list<PrioritizedMessage<T>> prioritizedMessages;
  typename std::list<PrioritizedMessage<T>>::iterator current_read_iterator;
  
  // Statistics tracking
  uint32_t totalMessagesQueued = 0;
  uint32_t criticalQueued = 0;
  uint32_t highQueued = 0;
  uint32_t normalQueued = 0;
  uint32_t lowQueued = 0;
  uint32_t criticalSent = 0;
  uint32_t highSent = 0;
  uint32_t normalSent = 0;
  uint32_t lowSent = 0;
  
  /**
   * Find the highest priority message in the buffer
   * Returns iterator to the highest priority message, or end() if empty
   * 
   * Special handling: If we're in the middle of reading a message (not clean),
   * we continue with that message even if higher priority messages arrive.
   * This maintains backward compatibility with partial read behavior.
   */
  typename std::list<PrioritizedMessage<T>>::iterator findHighestPriorityMessage() {
    if (prioritizedMessages.empty()) {
      return prioritizedMessages.end();
    }
    
    // If we're in the middle of a partial read (!clean), continue with current_read_iterator
    // This maintains backward compatibility with the original buffer behavior
    if (!clean && current_read_iterator != prioritizedMessages.end()) {
      return current_read_iterator;
    }
    
    // Find message with lowest priority value (0=CRITICAL is highest)
    auto highest = prioritizedMessages.begin();
    for (auto it = prioritizedMessages.begin(); it != prioritizedMessages.end(); ++it) {
      if (it->priority < highest->priority) {
        highest = it;
      }
    }
    
    // Cache this for partial reads
    current_read_iterator = highest;
    return highest;
  }
  
  /**
   * Get pointer to next message to send (highest priority)
   */
  T* getNextMessage() {
    auto it = findHighestPriorityMessage();
    if (it != prioritizedMessages.end()) {
      return &(it->message);
    }
    return nullptr;
  }
  
  /**
   * Get priority of the current highest priority message
   */
  uint8_t getCurrentMessagePriority() {
    auto it = findHighestPriorityMessage();
    if (it != prioritizedMessages.end()) {
      return it->priority;
    }
    return 2;  // NORMAL default
  }

  inline void stringEraseFront(T &string, size_t length) {
    string.remove(0, length);
  };
};

#ifdef PAINLESSMESH_ENABLE_STD_STRING
template <>
inline void SentBuffer<std::string>::read(size_t length, temp_buffer_t &buf) {
  auto* msg = getNextMessage();
  if (msg) {
    msg->copy(buf.buffer, length);
    // Mimic String.toCharArray behaviour, which will insert
    // null termination at the end of original string and the last
    // character
    if (length == msg->length() + 1) buf.buffer[length - 1] = '\0';
    buf.buffer[length] = '\0';
    last_read_size = length;
    last_read_priority = getCurrentMessagePriority();
  }
}

template <>
inline void SentBuffer<std::string>::stringEraseFront(std::string &string,
                                                      size_t length) {
  string.erase(0, length);
};
#endif

}  // namespace buffer
}  // namespace painlessmesh
#endif
