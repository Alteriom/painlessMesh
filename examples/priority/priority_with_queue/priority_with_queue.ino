/**
 * Advanced Priority Example with Message Queue Integration
 * 
 * Demonstrates how to use priority messaging with the message queue
 * for offline/Internet-unavailable scenarios.
 * 
 * This example shows:
 * - Priority-based message sending
 * - Message queueing when Internet is unavailable
 * - Automatic queue flushing when Internet becomes available
 * - Priority ordering during queue flush
 * 
 * Hardware: ESP32 or ESP8266
 */

#include "painlessMesh.h"

#define MESH_PREFIX     "PriorityQueueMesh"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

bool internetAvailable = false;
uint32_t bridgeNodeId = 0;

// Task to check bridge status
Task taskCheckBridge(5000, TASK_FOREVER, []() {
    bool hasInternet = mesh.hasInternetConnection();
    
    if (hasInternet != internetAvailable) {
        internetAvailable = hasInternet;
        Serial.printf("Internet status changed: %s\n", hasInternet ? "ONLINE" : "OFFLINE");
        
        if (hasInternet) {
            // Internet came back online - flush queued messages by priority
            flushQueuedMessages();
        }
    }
});

// Task to generate sensor data
Task taskSensorData(10000, TASK_FOREVER, []() {
    String sensorData = "{\"type\":\"sensor\",\"temp\":" + String(random(15, 30)) + 
                       ",\"humidity\":" + String(random(30, 70)) + "}";
    
    sendMessage(sensorData, 2);  // NORMAL priority
});

// Task to generate status updates
Task taskStatusUpdate(30000, TASK_FOREVER, []() {
    String status = "{\"type\":\"status\",\"uptime\":" + String(millis() / 1000) + 
                   ",\"heap\":" + String(ESP.getFreeHeap()) + "}";
    
    sendMessage(status, 1);  // HIGH priority
});

void setup() {
    Serial.begin(115200);
    
    // Enable message queue with 500 message capacity
    mesh.enableMessageQueue(true, 500);
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onBridgeStatusChanged(&bridgeStatusChanged);
    mesh.onQueueStateChanged(&queueStateChanged);
    
    // Add tasks
    userScheduler.addTask(taskCheckBridge);
    userScheduler.addTask(taskSensorData);
    userScheduler.addTask(taskStatusUpdate);
    
    taskCheckBridge.enable();
    taskSensorData.enable();
    taskStatusUpdate.enable();
    
    Serial.println("Priority + Queue demo started");
    Serial.println("Messages will be queued when Internet is unavailable");
    Serial.println("Queue will be flushed by priority when Internet returns");
}

void loop() {
    mesh.update();
}

/**
 * Send a message with priority
 * - If Internet available: send immediately via mesh with priority
 * - If Internet unavailable: queue for later with priority
 */
void sendMessage(String message, uint8_t priority) {
    if (internetAvailable && bridgeNodeId != 0) {
        // Internet available - send with priority
        mesh.sendSingle(bridgeNodeId, message, priority);
        Serial.printf("Sent message with priority %u\n", priority);
        
    } else {
        // Internet unavailable - queue with priority
        // Map our priority levels to MessageQueue priorities
        MessagePriority queuePriority;
        switch(priority) {
            case 0: queuePriority = PRIORITY_CRITICAL; break;
            case 1: queuePriority = PRIORITY_HIGH; break;
            case 2: queuePriority = PRIORITY_NORMAL; break;
            case 3: queuePriority = PRIORITY_LOW; break;
            default: queuePriority = PRIORITY_NORMAL; break;
        }
        
        uint32_t msgId = mesh.queueMessage(message, "mqtt://cloud", queuePriority);
        if (msgId > 0) {
            Serial.printf("Queued message #%u with priority %u (offline mode)\n", msgId, priority);
        } else {
            Serial.println("Failed to queue message - queue full!");
        }
    }
}

/**
 * Send a critical alarm - always gets through
 */
void sendCriticalAlarm(String alarmType, String alarmMessage) {
    String criticalMsg = "{\"type\":\"alarm\",\"alarm\":\"" + alarmType + 
                        "\",\"msg\":\"" + alarmMessage + "\",\"time\":" + 
                        String(millis()) + "}";
    
    // CRITICAL priority - send immediately even if queued
    sendMessage(criticalMsg, 0);
    
    Serial.printf("CRITICAL ALARM: %s - %s\n", alarmType.c_str(), alarmMessage.c_str());
}

/**
 * Flush queued messages by priority when Internet returns
 */
void flushQueuedMessages() {
    Serial.println("Flushing queued messages by priority...");
    
    // Get all queued messages (already sorted by priority in MessageQueue)
    auto messages = mesh.flushMessageQueue();
    
    Serial.printf("Found %d queued messages to send\n", messages.size());
    
    // Send each message with its original priority
    for (auto& msg : messages) {
        // Convert MessagePriority back to send priority (0-3)
        uint8_t sendPriority;
        switch(msg.priority) {
            case PRIORITY_CRITICAL: sendPriority = 0; break;
            case PRIORITY_HIGH: sendPriority = 1; break;
            case PRIORITY_NORMAL: sendPriority = 2; break;
            case PRIORITY_LOW: sendPriority = 3; break;
            default: sendPriority = 2; break;
        }
        
        // Send with original priority
        if (bridgeNodeId != 0) {
            bool sent = mesh.sendSingle(bridgeNodeId, msg.payload, sendPriority);
            
            if (sent) {
                // Remove from queue on success
                mesh.removeQueuedMessage(msg.id);
                Serial.printf("Sent queued message #%u (priority %u)\n", msg.id, sendPriority);
            } else {
                // Failed to send - increment attempts
                uint32_t attempts = mesh.incrementQueuedMessageAttempts(msg.id);
                Serial.printf("Failed to send queued message #%u (attempt %u)\n", msg.id, attempts);
                
                // If too many attempts, remove it
                if (attempts > 5) {
                    mesh.removeQueuedMessage(msg.id);
                    Serial.printf("Dropped message #%u after 5 attempts\n", msg.id);
                }
            }
        }
        
        // Small delay between sends to avoid overwhelming the network
        delay(10);
    }
    
    Serial.printf("Queue flush complete. Remaining: %u messages\n", 
                 mesh.getQueuedMessageCount());
}

void receivedCallback(uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
}

void bridgeStatusChanged(uint32_t nodeId, bool hasInternet) {
    Serial.printf("Bridge %u status: Internet %s\n", 
                 nodeId, hasInternet ? "ONLINE" : "OFFLINE");
    
    if (hasInternet) {
        bridgeNodeId = nodeId;
        internetAvailable = true;
        
        // Flush queued messages with priority
        flushQueuedMessages();
    } else {
        internetAvailable = false;
    }
}

void queueStateChanged(QueueState state, uint32_t count) {
    switch(state) {
        case QUEUE_EMPTY:
            Serial.println("Queue is empty");
            break;
        case QUEUE_NORMAL:
            Serial.printf("Queue normal: %u messages\n", count);
            break;
        case QUEUE_75_PERCENT:
            Serial.printf("WARNING: Queue 75%% full (%u messages)\n", count);
            break;
        case QUEUE_FULL:
            Serial.printf("ALERT: Queue is FULL (%u messages)\n", count);
            break;
    }
}

// Example: Simulate a critical event
void simulateCriticalEvent() {
    sendCriticalAlarm("FIRE", "Fire detected in Building A, Floor 3");
}

// Example: Monitor queue statistics
void printQueueStats() {
    auto stats = mesh.getQueueStats();
    
    Serial.println("=== Queue Statistics ===");
    Serial.printf("Total queued: %u\n", stats.totalQueued);
    Serial.printf("Total sent: %u\n", stats.totalSent);
    Serial.printf("Total dropped: %u\n", stats.totalDropped);
    Serial.printf("Current size: %u / %u\n", stats.currentSize, stats.maxSize);
    Serial.println();
    Serial.printf("CRITICAL queued: %u\n", stats.criticalQueued);
    Serial.printf("HIGH queued: %u\n", stats.highQueued);
    Serial.printf("NORMAL queued: %u\n", stats.normalQueued);
    Serial.printf("LOW queued: %u\n", stats.lowQueued);
    Serial.println("=======================");
}
