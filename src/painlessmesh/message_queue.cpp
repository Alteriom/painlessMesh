#include "painlessmesh/message_queue.hpp"
#include "painlessmesh/logger.hpp"
#include <algorithm>  // for std::remove_if

// Only include filesystem headers in Arduino environment
#if defined(ARDUINO) && defined(PAINLESSMESH_ENABLE_ARDUINO_WIFI)
#include <FS.h>
#ifdef ESP32
#include <SPIFFS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS
#else  // ESP8266
#include <LittleFS.h>
#define FILESYSTEM LittleFS
#endif
#define FILESYSTEM_AVAILABLE
#endif

extern painlessmesh::logger::LogClass Log;

namespace painlessmesh {
namespace queue {

MessageQueue::MessageQueue() 
    : maxQueueSize(500), nextMessageId(1), maxRetryAttempts(3),
      persistentStorageEnabled(false), storagePath("/painlessmesh/queue.dat"),
      lastNotifiedState(QUEUE_EMPTY) {
  stats = QueueStats();
}

MessageQueue::~MessageQueue() {
  if (persistentStorageEnabled) {
    saveToStorage();
  }
}

void MessageQueue::init(uint32_t maxSize, bool enablePersistence, 
                       const TSTRING& path) {
  using namespace logger;
  
  maxQueueSize = maxSize;
  persistentStorageEnabled = enablePersistence;
  storagePath = path;
  
  Log(STARTUP, "MessageQueue::init(): maxSize=%u, persistence=%s, path=%s\n",
      maxSize, enablePersistence ? "enabled" : "disabled", path.c_str());
  
  if (persistentStorageEnabled) {
#ifdef FILESYSTEM_AVAILABLE
    // Initialize filesystem if not already mounted
    if (!FILESYSTEM.begin(true)) {
      Log(ERROR, "MessageQueue::init(): Failed to mount filesystem\n");
      persistentStorageEnabled = false;
    } else {
      Log(STARTUP, "MessageQueue::init(): Filesystem mounted successfully\n");
      // Try to load existing queue
      uint32_t loaded = loadFromStorage();
      if (loaded > 0) {
        Log(STARTUP, "MessageQueue::init(): Loaded %u messages from storage\n", loaded);
      }
    }
#else
    Log(ERROR, "MessageQueue::init(): Persistence not available (not Arduino platform)\n");
    persistentStorageEnabled = false;
#endif
  }
}

uint32_t MessageQueue::queueMessage(const TSTRING& payload, const TSTRING& destination,
                                   MessagePriority priority) {
  using namespace logger;
  
  // Check if we need to make space
  if (isFull()) {
    // Try to make space by removing low priority messages
    if (priority == PRIORITY_CRITICAL || priority == PRIORITY_HIGH) {
      if (!makeSpace()) {
        Log(ERROR, "MessageQueue::queueMessage(): Queue full, cannot make space\n");
        stats.totalDropped++;
        return 0;
      }
    } else {
      Log(ERROR, "MessageQueue::queueMessage(): Queue full, dropping message (priority %d)\n", 
          priority);
      stats.totalDropped++;
      return 0;
    }
  }
  
  QueuedMessage msg;
  msg.id = generateMessageId();
  msg.priority = priority;
  msg.timestamp = millis();
  msg.attempts = 0;
  msg.payload = payload;
  msg.destination = destination;
  
  queue.push_back(msg);
  stats.totalQueued++;
  stats.currentSize = queue.size();
  
  if (stats.currentSize > stats.peakSize) {
    stats.peakSize = stats.currentSize;
  }
  
  Log(COMMUNICATION, "MessageQueue::queueMessage(): Queued message #%u (priority %d, size %u/%u)\n",
      msg.id, priority, stats.currentSize, maxQueueSize);
  
  notifyStateChange();
  
  // Auto-save to storage if critical message
  if (persistentStorageEnabled && priority == PRIORITY_CRITICAL) {
    saveToStorage();
  }
  
  return msg.id;
}

uint32_t MessageQueue::flushQueue(sendCallback_t sendCallback) {
  using namespace logger;
  
  if (!sendCallback) {
    Log(ERROR, "MessageQueue::flushQueue(): No send callback provided\n");
    return 0;
  }
  
  uint32_t sentCount = 0;
  std::vector<uint32_t> toRemove;
  
  Log(COMMUNICATION, "MessageQueue::flushQueue(): Attempting to send %u messages\n", 
      queue.size());
  
  for (auto& msg : queue) {
    msg.attempts++;
    
    bool sent = sendCallback(msg.payload, msg.destination);
    
    if (sent) {
      Log(COMMUNICATION, "MessageQueue::flushQueue(): Message #%u sent successfully\n", 
          msg.id);
      toRemove.push_back(msg.id);
      stats.totalSent++;
      sentCount++;
    } else {
      Log(ERROR, "MessageQueue::flushQueue(): Message #%u failed (attempt %u/%u)\n",
          msg.id, msg.attempts, maxRetryAttempts);
      
      if (msg.attempts >= maxRetryAttempts) {
        Log(ERROR, "MessageQueue::flushQueue(): Message #%u exceeded retry limit, removing\n",
            msg.id);
        toRemove.push_back(msg.id);
        stats.totalFailed++;
      }
    }
  }
  
  // Remove successfully sent and failed messages
  for (uint32_t id : toRemove) {
    queue.erase(
      std::remove_if(queue.begin(), queue.end(),
                    [id](const QueuedMessage& m) { return m.id == id; }),
      queue.end()
    );
  }
  
  stats.currentSize = queue.size();
  
  Log(COMMUNICATION, "MessageQueue::flushQueue(): Sent %u messages, %u remaining\n",
      sentCount, stats.currentSize);
  
  notifyStateChange();
  
  // Save updated queue to storage
  if (persistentStorageEnabled && !queue.empty()) {
    saveToStorage();
  }
  
  return sentCount;
}

uint32_t MessageQueue::getQueuedMessageCount(MessagePriority priority) const {
  uint32_t count = 0;
  for (const auto& msg : queue) {
    if (msg.priority == priority) {
      count++;
    }
  }
  return count;
}

uint32_t MessageQueue::getQueuedMessageCount() const {
  return queue.size();
}

uint32_t MessageQueue::pruneQueue(uint32_t maxAgeHours) {
  using namespace logger;
  
  uint32_t maxAgeMs = maxAgeHours * 3600000UL;  // Convert to milliseconds
  uint32_t now = millis();
  uint32_t removed = 0;
  
  auto it = queue.begin();
  while (it != queue.end()) {
    uint32_t age = now - it->timestamp;
    if (age > maxAgeMs) {
      Log(GENERAL, "MessageQueue::pruneQueue(): Removing old message #%u (age %u hours)\n",
          it->id, age / 3600000UL);
      it = queue.erase(it);
      removed++;
    } else {
      ++it;
    }
  }
  
  if (removed > 0) {
    stats.currentSize = queue.size();
    Log(GENERAL, "MessageQueue::pruneQueue(): Removed %u old messages\n", removed);
    notifyStateChange();
    
    if (persistentStorageEnabled) {
      saveToStorage();
    }
  }
  
  return removed;
}

void MessageQueue::clear() {
  using namespace logger;
  
  uint32_t count = queue.size();
  queue.clear();
  stats.currentSize = 0;
  
  Log(GENERAL, "MessageQueue::clear(): Cleared %u messages\n", count);
  
  notifyStateChange();
  
  if (persistentStorageEnabled) {
    saveToStorage();
  }
}

bool MessageQueue::saveToStorage() {
#ifdef FILESYSTEM_AVAILABLE
  using namespace logger;
  
  if (!persistentStorageEnabled) {
    return false;
  }
  
  Log(GENERAL, "MessageQueue::saveToStorage(): Saving %u messages to %s\n",
      queue.size(), storagePath.c_str());
  
  // Create directory if it doesn't exist
  String dirPath = storagePath.substring(0, storagePath.lastIndexOf('/'));
  if (!FILESYSTEM.exists(dirPath.c_str())) {
    if (!FILESYSTEM.mkdir(dirPath.c_str())) {
      Log(ERROR, "MessageQueue::saveToStorage(): Failed to create directory %s\n",
          dirPath.c_str());
      return false;
    }
  }
  
  File file = FILESYSTEM.open(storagePath.c_str(), "w");
  if (!file) {
    Log(ERROR, "MessageQueue::saveToStorage(): Failed to open file for writing\n");
    return false;
  }
  
  // Write messages in JSON Lines format (one JSON object per line)
  for (const auto& msg : queue) {
    String line = "{\"id\":";
    line += msg.id;
    line += ",\"priority\":";
    line += (int)msg.priority;
    line += ",\"timestamp\":";
    line += msg.timestamp;
    line += ",\"attempts\":";
    line += msg.attempts;
    line += ",\"payload\":\"";
    // Escape quotes in payload
    String escapedPayload = msg.payload;
    escapedPayload.replace("\"", "\\\"");
    line += escapedPayload;
    line += "\",\"destination\":\"";
    line += msg.destination;
    line += "\"}\n";
    
    file.print(line);
  }
  
  file.close();
  
  Log(GENERAL, "MessageQueue::saveToStorage(): Successfully saved %u messages\n",
      queue.size());
  
  return true;
#else
  return false;
#endif
}

uint32_t MessageQueue::loadFromStorage() {
#ifdef FILESYSTEM_AVAILABLE
  using namespace logger;
  
  if (!persistentStorageEnabled) {
    return 0;
  }
  
  if (!FILESYSTEM.exists(storagePath.c_str())) {
    Log(GENERAL, "MessageQueue::loadFromStorage(): No existing queue file found\n");
    return 0;
  }
  
  File file = FILESYSTEM.open(storagePath.c_str(), "r");
  if (!file) {
    Log(ERROR, "MessageQueue::loadFromStorage(): Failed to open file for reading\n");
    return 0;
  }
  
  uint32_t loaded = 0;
  
  // Read and parse each line
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    
    if (line.length() == 0) continue;
    
    // Parse JSON manually (simple parsing for basic fields)
    // Format: {"id":123,"priority":0,"timestamp":456,"attempts":0,"payload":"...","destination":"..."}
    
    QueuedMessage msg;
    
    // Extract id
    int idStart = line.indexOf("\"id\":") + 5;
    int idEnd = line.indexOf(",", idStart);
    msg.id = line.substring(idStart, idEnd).toInt();
    
    // Extract priority
    int priStart = line.indexOf("\"priority\":") + 11;
    int priEnd = line.indexOf(",", priStart);
    msg.priority = (MessagePriority)line.substring(priStart, priEnd).toInt();
    
    // Extract timestamp
    int tsStart = line.indexOf("\"timestamp\":") + 12;
    int tsEnd = line.indexOf(",", tsStart);
    msg.timestamp = line.substring(tsStart, tsEnd).toInt();
    
    // Extract attempts
    int attStart = line.indexOf("\"attempts\":") + 11;
    int attEnd = line.indexOf(",", attStart);
    msg.attempts = line.substring(attStart, attEnd).toInt();
    
    // Extract payload
    int payStart = line.indexOf("\"payload\":\"") + 11;
    int payEnd = line.indexOf("\",\"destination\":", payStart);
    msg.payload = line.substring(payStart, payEnd);
    // Unescape quotes
    msg.payload.replace("\\\"", "\"");
    
    // Extract destination
    int destStart = line.indexOf("\"destination\":\"") + 15;
    int destEnd = line.indexOf("\"}", destStart);
    msg.destination = line.substring(destStart, destEnd);
    
    queue.push_back(msg);
    loaded++;
    
    // Update next message ID
    if (msg.id >= nextMessageId) {
      nextMessageId = msg.id + 1;
    }
  }
  
  file.close();
  
  stats.currentSize = queue.size();
  
  Log(GENERAL, "MessageQueue::loadFromStorage(): Loaded %u messages from storage\n",
      loaded);
  
  return loaded;
#else
  return 0;
#endif
}

uint32_t MessageQueue::generateMessageId() {
  return nextMessageId++;
}

bool MessageQueue::makeSpace() {
  using namespace logger;
  
  // First, try to remove LOW priority messages
  auto it = queue.begin();
  while (it != queue.end()) {
    if (it->priority == PRIORITY_LOW) {
      Log(GENERAL, "MessageQueue::makeSpace(): Removing LOW priority message #%u\n",
          it->id);
      it = queue.erase(it);
      stats.totalDropped++;
      stats.currentSize = queue.size();
      return true;
    } else {
      ++it;
    }
  }
  
  // If no LOW priority messages, try NORMAL priority
  it = queue.begin();
  while (it != queue.end()) {
    if (it->priority == PRIORITY_NORMAL) {
      // Only remove old NORMAL messages (older than 1 hour)
      uint32_t age = millis() - it->timestamp;
      if (age > 3600000UL) {  // 1 hour
        Log(GENERAL, "MessageQueue::makeSpace(): Removing old NORMAL priority message #%u\n",
            it->id);
        it = queue.erase(it);
        stats.totalDropped++;
        stats.currentSize = queue.size();
        return true;
      }
    }
    ++it;
  }
  
  Log(ERROR, "MessageQueue::makeSpace(): Could not free space (only CRITICAL/HIGH messages)\n");
  return false;
}

QueueState MessageQueue::calculateQueueState() const {
  if (queue.empty()) {
    return QUEUE_EMPTY;
  }
  
  float fillPercent = (float)queue.size() / (float)maxQueueSize;
  
  if (fillPercent >= 1.0f) {
    return QUEUE_FULL;
  } else if (fillPercent >= 0.75f) {
    return QUEUE_75_PERCENT;
  } else if (fillPercent >= 0.50f) {
    return QUEUE_50_PERCENT;
  } else if (fillPercent >= 0.25f) {
    return QUEUE_25_PERCENT;
  } else {
    return QUEUE_EMPTY;  // Treat < 25% as "empty" for notification purposes
  }
}

void MessageQueue::notifyStateChange() {
  QueueState currentState = calculateQueueState();
  
  if (currentState != lastNotifiedState) {
    lastNotifiedState = currentState;
    
    if (stateCallback) {
      stateCallback(currentState, queue.size());
    }
  }
}

}  // namespace queue
}  // namespace painlessmesh
