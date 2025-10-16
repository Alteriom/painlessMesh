# Debugging Guide

This guide provides tools and techniques for debugging painlessMesh applications.

## Debug Message Types

painlessMesh includes a built-in debugging system that lets you control which types of messages are logged.

### Setting Debug Message Types

```cpp
#include "painlessMesh.h"

painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  
  // Set which message types to display
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Initialize mesh
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}
```

### Available Debug Message Types

| Type | Description |
|------|-------------|
| `ERROR` | Critical errors and failures |
| `STARTUP` | Initialization and startup messages |
| `CONNECTION` | Connection and disconnection events |
| `SYNC` | Time synchronization messages |
| `COMMUNICATION` | Message sending/receiving |
| `GENERAL` | General informational messages |
| `MSG_TYPES` | Message type information |
| `REMOTE` | Remote node messages |
| `APPLICATION` | Application-level messages |
| `DEBUG` | Detailed debugging information |

### Common Debug Configurations

**Minimal (Production):**

```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP);
```

**Standard (Development):**

```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
```

**Verbose (Troubleshooting):**

```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | SYNC | COMMUNICATION);
```

**Full Debug (Deep Dive):**

```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | SYNC | 
                      COMMUNICATION | GENERAL | MSG_TYPES | 
                      REMOTE | APPLICATION | DEBUG);
```

## Common Debugging Scenarios

### 1. Nodes Not Connecting

**Symptoms:**

- Nodes don't appear in `mesh.getNodeList()`
- `onNewConnection` callback never fires
- Mesh appears to run but no communication

**Debugging Steps:**

```cpp
// Enable connection debugging
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | SYNC);

void setup() {
  Serial.begin(115200);
  
  // Add connection callbacks
  mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
    Serial.printf("Total nodes: %d\n", mesh.getNodeList().size());
  });
  
  mesh.onChangedConnections([]() {
    Serial.printf("Connections changed. Current nodes: %d\n", 
                  mesh.getNodeList().size());
    auto nodes = mesh.getNodeList();
    for (auto node : nodes) {
      Serial.printf("  - Node: %u\n", node);
    }
  });
  
  mesh.onDroppedConnection([](uint32_t nodeId) {
    Serial.printf("Lost connection: %u\n", nodeId);
  });
}
```

**Common Causes:**

- Mismatched `MESH_PREFIX` or `MESH_PASSWORD`
- Different `MESH_PORT` values
- Wi-Fi channel conflicts
- Power supply issues (voltage drops during connection)
- Too many nodes (exceeds platform limits)

### 2. Messages Not Received

**Symptoms:**

- `mesh.sendBroadcast()` or `mesh.sendSingle()` returns true but messages not received
- `onReceive` callback never fires

**Debugging Steps:**

```cpp
// Enable communication debugging
mesh.setDebugMsgTypes(ERROR | COMMUNICATION | MSG_TYPES);

void setup() {
  Serial.begin(115200);
  
  mesh.onReceive([](uint32_t from, String& msg) {
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
  });
  
  // Add send status checking
  if (mesh.sendBroadcast(msg)) {
    Serial.println("Broadcast sent successfully");
  } else {
    Serial.println("ERROR: Broadcast failed to send");
  }
}
```

**Common Causes:**

- Message buffer overflow (message too large)
- Network congestion (too many messages)
- JSON parsing errors (malformed messages)
- Node ID mismatch for `sendSingle()`
- Memory constraints on receiving node

### 3. Time Synchronization Issues

**Symptoms:**

- `mesh.getNodeTime()` returns unexpected values
- Time-dependent features not working
- `onNodeTimeAdjusted` fires frequently

**Debugging Steps:**

```cpp
// Enable time sync debugging
mesh.setDebugMsgTypes(ERROR | SYNC);

void setup() {
  Serial.begin(115200);
  
  mesh.onNodeTimeAdjusted([](int32_t offset) {
    Serial.printf("Time adjusted by: %d microseconds\n", offset);
    Serial.printf("Current mesh time: %u\n", mesh.getNodeTime());
  });
  
  // Periodically check time
  Task checkTime(TASK_SECOND * 10, TASK_FOREVER, []() {
    Serial.printf("Mesh time: %u, System: %u\n", 
                  mesh.getNodeTime(), micros());
  });
  userScheduler.addTask(checkTime);
  checkTime.enable();
}
```

### 4. Memory Issues

**Symptoms:**

- Random crashes or reboots
- Mesh stops responding
- `heap_caps_check_integrity` failures on ESP32

**Debugging Steps:**

```cpp
void setup() {
  Serial.begin(115200);
  
  // Monitor free memory
  Task memoryCheck(TASK_SECOND * 5, TASK_FOREVER, []() {
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    #ifdef ESP32
    Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
    Serial.printf("Largest free block: %u bytes\n", 
                  heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    #endif
  });
  userScheduler.addTask(memoryCheck);
  memoryCheck.enable();
}
```

**Memory Optimization Tips:**

- Use `String` sparingly, prefer `const char*`
- Limit broadcast message frequency
- Reduce node count for ESP8266
- Clear unused tasks from scheduler
- Use `F()` macro for string literals

### 5. OTA Update Failures

**Symptoms:**

- OTA updates don't start
- Updates fail partway through
- Nodes become unresponsive during update

**Debugging Steps:**

```cpp
// Enable OTA debugging
mesh.setDebugMsgTypes(ERROR | STARTUP | COMMUNICATION);

void setup() {
  Serial.begin(115200);
  
  // Add OTA callbacks if using broadcast OTA
  mesh.onOTAProgress([](size_t progress, size_t total) {
    Serial.printf("OTA Progress: %u/%u bytes (%.1f%%)\n", 
                  progress, total, (progress * 100.0) / total);
  });
  
  mesh.onOTAComplete([]() {
    Serial.println("OTA Update Complete - Rebooting...");
  });
  
  mesh.onOTAError([](int error) {
    Serial.printf("OTA Error: %d\n", error);
  });
}
```

**Common Causes:**

- Insufficient flash memory
- Power interruption during update
- Network instability
- Firmware size exceeds partition size
- Memory fragmentation

## Debugging Tools

### 1. Serial Monitor

**Arduino IDE:**

- Tools → Serial Monitor
- Set baud rate to 115200
- Enable newline and carriage return

**PlatformIO:**

```bash
pio device monitor --baud 115200
```

**Screen (Linux/Mac):**

```bash
screen /dev/ttyUSB0 115200
```

### 2. Network Analyzer

**Wireshark:**

- Capture Wi-Fi traffic to see mesh packets
- Filter by ESP32/ESP8266 MAC addresses
- Analyze packet timing and retransmissions

### 3. Mesh Topology Visualization

For MQTT-enabled setups:

- Use MQTT Explorer to view mesh topology
- Monitor `alteriom/mesh/topology` topic
- Visualize node connections and status

### 4. Remote Logging

**Log Server Example:**

```cpp
// On one node, set up as log server
mesh.onReceive([](uint32_t from, String& msg) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, msg);
  
  if (doc["type"] == "log") {
    Serial.printf("[%u] %s\n", from, doc["msg"].as<const char*>());
  }
});

// On other nodes, send logs
void logToServer(const char* message) {
  DynamicJsonDocument doc(256);
  doc["type"] = "log";
  doc["msg"] = message;
  
  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);
}
```

## Performance Profiling

### Measure Message Latency

```cpp
uint32_t sendTime = 0;

void sendMessage() {
  DynamicJsonDocument doc(256);
  doc["type"] = "ping";
  doc["timestamp"] = mesh.getNodeTime();
  
  String msg;
  serializeJson(doc, msg);
  
  sendTime = millis();
  mesh.sendBroadcast(msg);
}

void onMessageReceived(uint32_t from, String& msg) {
  DynamicJsonDocument doc(256);
  deserializeJson(doc, msg);
  
  if (doc["type"] == "ping") {
    uint32_t latency = millis() - sendTime;
    Serial.printf("Ping latency: %u ms\n", latency);
  }
}
```

### Monitor Task Execution

```cpp
void setup() {
  // Enable TaskScheduler debugging
  userScheduler.enableAll();
  
  Task debugTask(TASK_SECOND * 10, TASK_FOREVER, []() {
    Serial.printf("Active tasks: %d\n", userScheduler.size());
    // List all tasks
    for (auto task = userScheduler.getFirstTask(); 
         task != nullptr; 
         task = task->getNext()) {
      Serial.printf("  Task ID: %d, Enabled: %d\n", 
                    task->getId(), task->isEnabled());
    }
  });
  userScheduler.addTask(debugTask);
  debugTask.enable();
}
```

## Best Practices

1. **Start Minimal:** Enable only ERROR and STARTUP for initial testing
2. **Add Incrementally:** Add more debug types as needed
3. **Use Callbacks:** Implement all mesh callbacks to catch events
4. **Monitor Memory:** Regularly check free heap, especially on ESP8266
5. **Test Incrementally:** Test with 2 nodes before scaling to larger networks
6. **Power Management:** Ensure stable power supply during debugging
7. **Version Control:** Use consistent firmware versions across all nodes
8. **Document Issues:** Keep notes on error patterns and solutions

## Advanced Debugging

### Enable TaskScheduler Debug Mode

```cpp
// Add to top of sketch BEFORE including headers
#define _TASK_DEBUG

#include <TaskScheduler.h>
#include "painlessMesh.h"
```

### Enable ArduinoJson Debug

```cpp
#define ARDUINOJSON_DEBUG 1
#include <ArduinoJson.h>
```

### ESP32 Core Debug Level

In `platformio.ini`:

```ini
build_flags =
  -DCORE_DEBUG_LEVEL=5  ; 0=None, 5=Verbose
```

## Troubleshooting Checklist

- [ ] All nodes use same MESH_PREFIX, MESH_PASSWORD, MESH_PORT
- [ ] Serial baud rate set to 115200
- [ ] Debug messages enabled for relevant types
- [ ] Callbacks implemented (onReceive, onNewConnection, etc.)
- [ ] Free heap monitored (especially on ESP8266)
- [ ] Power supply stable (2A+ recommended)
- [ ] Firmware versions consistent across nodes
- [ ] JSON messages properly formatted
- [ ] Message sizes within limits (< 1KB recommended)
- [ ] Network not oversaturated (reasonable message frequency)

## Getting Help

If you're still experiencing issues:

1. **Search Existing Issues:** Check [GitHub Issues](https://github.com/Alteriom/painlessMesh/issues)
2. **Community Forum:** Post on [painlessMesh Discussions](https://github.com/Alteriom/painlessMesh/discussions)
3. **Include Details:**
   - Hardware (ESP32/ESP8266 model)
   - Firmware version
   - Number of nodes
   - Debug output
   - Minimal reproducible example
4. **Check Documentation:**
   - [Common Issues](common-issues.md)
   - [FAQ](faq.md)
   - [API Reference](../api/core-api.md)

## See Also

- [Common Issues](common-issues.md) - Known problems and solutions
- [FAQ](faq.md) - Frequently asked questions
- [Performance Optimization](../architecture/mesh-architecture.md) - Scaling best practices
- [MQTT Bridge Commands](../MQTT_BRIDGE_COMMANDS.md) - MQTT debugging
