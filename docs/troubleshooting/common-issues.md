# Common Issues and Solutions

This guide covers the most frequently encountered problems when working with painlessMesh and their solutions.

## Connection Issues

### Nodes Not Connecting

**Symptoms:**
- Nodes don't appear in each other's node lists
- No "New Connection" messages in serial output
- Mesh remains disconnected

**Solutions:**

#### 1. Check Network Credentials
Ensure all nodes use identical network settings:

```cpp
// These MUST be identical on all nodes
#define MESH_PREFIX     "MyMeshNetwork"    // Exact match required
#define MESH_PASSWORD   "password123"      // Case sensitive
#define MESH_PORT       5555               // Must match
```

#### 2. Verify WiFi Range
- Nodes must be within WiFi range (typically 50-200m)
- Check for interference from other 2.4GHz devices
- Try moving nodes closer together for testing

#### 3. Check Serial Output
Enable connection debugging:

```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
```

Look for error messages like:
- "Failed to connect to mesh"
- "Connection timeout"
- "Authentication failed"

#### 4. Reset Network Settings
Clear stored WiFi credentials:

```cpp
void setup() {
    // Add this before mesh.init()
    WiFi.disconnect(true);  // Clear stored networks
    delay(1000);
    
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}
```

### Frequent Disconnections

**Symptoms:**
- Nodes connect but disconnect shortly after
- Repeated "Connection dropped" messages
- Unstable mesh topology

**Solutions:**

#### 1. Check Power Supply
- Ensure stable power supply (USB power can be insufficient)
- Use adequate power adapters (≥1A for ESP32, ≥500mA for ESP8266)
- Check for voltage drops during WiFi transmission

#### 2. Reduce Connection Load
Limit the number of simultaneous connections:

```cpp
// Reduce connection count on ESP8266
#define MAX_CONN 2  // Instead of default 4
```

#### 3. Increase Connection Timeout
```cpp
// Extend connection timeouts
#define CONNECTION_TIMEOUT 60000  // 60 seconds instead of 30
```

#### 4. Memory Issues
Monitor memory usage:

```cpp
void checkMemory() {
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    if (ESP.getFreeHeap() < 10000) {  // ESP32
        Serial.println("WARNING: Low memory!");
    }
}
```

## Message Delivery Issues

### Messages Not Being Received

**Symptoms:**
- `sendBroadcast()` returns `true` but messages don't arrive
- Callbacks not triggered
- Silent message failures

**Solutions:**

#### 1. Check Callback Registration
Ensure callbacks are set before `mesh.init()`:

```cpp
void setup() {
    Serial.begin(115200);
    
    // Set callbacks BEFORE init
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    
    // Then initialize
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}
```

#### 2. Verify Message Format
Check JSON formatting for custom messages:

```cpp
void sendSensorData() {
    // Proper JSON formatting
    String msg = "{";
    msg += "\"type\":\"sensor\",";
    msg += "\"value\":" + String(sensorValue) + ",";
    msg += "\"timestamp\":" + String(mesh.getNodeTime());
    msg += "}";  // Don't forget closing brace
    
    mesh.sendBroadcast(msg);
}
```

#### 3. Enable Communication Debugging
```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | COMMUNICATION);
```

#### 4. Check Network Congestion
Reduce message frequency if network is congested:

```cpp
// Instead of every second
Task taskSendMessage(10000, TASK_FOREVER, &sendMessage);  // Every 10 seconds
```

### Large Messages Being Dropped

**Symptoms:**
- Small messages work, large ones don't
- Memory errors in serial output
- Random message failures

**Solutions:**

#### 1. Reduce Message Size
```cpp
// Keep messages under 1KB for reliability
String createMessage() {
    String msg = "{\"data\":\"";
    msg += shortData; // Keep data concise
    msg += "\"}";
    
    if (msg.length() > 1000) {
        Serial.println("Warning: Message too large");
        return "{}"; // Send empty object
    }
    
    return msg;
}
```

#### 2. Split Large Data
```cpp
void sendLargeData(String largeData) {
    const size_t chunkSize = 500;
    
    for (size_t i = 0; i < largeData.length(); i += chunkSize) {
        String chunk = largeData.substring(i, i + chunkSize);
        String msg = "{\"chunk\":" + String(i/chunkSize) + ",\"data\":\"" + chunk + "\"}";
        mesh.sendBroadcast(msg);
        delay(100); // Small delay between chunks
    }
}
```

## Memory Issues

### ESP8266 Memory Limitations

**Symptoms:**
- Frequent crashes or resets
- "Out of memory" errors
- Unstable behavior under load

**Solutions:**

#### 1. Reduce Buffer Sizes
```cpp
// Use smaller JSON documents
DynamicJsonDocument doc(512);  // Instead of 1024 or larger

// Limit string sizes
TSTRING deviceName;
deviceName.reserve(32);  // Pre-allocate reasonable size
```

#### 2. Limit Concurrent Connections
```cpp
#define MAX_CONN 2  // ESP8266 works best with 2-3 connections
```

#### 3. Optimize Task Usage
```cpp
// Combine multiple tasks into one
Task taskMultiFunction(30000, TASK_FOREVER, [](){
    sendSensorData();
    checkStatus();
    cleanupMemory();
});
```

#### 4. Implement Memory Monitoring
```cpp
void monitorMemory() {
    uint32_t freeHeap = ESP.getFreeHeap();
    Serial.printf("Free heap: %u bytes\n", freeHeap);
    
    if (freeHeap < 5000) {  // Critical threshold for ESP8266
        Serial.println("CRITICAL: Very low memory!");
        // Take corrective action
        mesh.stop();
        delay(1000);
        ESP.restart();
    }
}
```

### ESP32 Memory Issues

**Symptoms:**
- Slower performance over time
- Memory leaks
- Task watchdog timeouts

**Solutions:**

#### 1. Monitor Heap Fragmentation
```cpp
void checkHeapHealth() {
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Largest block: %u bytes\n", ESP.getMaxAllocHeap());
    
    // If largest block is much smaller than free heap, 
    // heap is fragmented
    if (ESP.getMaxAllocHeap() < ESP.getFreeHeap() / 2) {
        Serial.println("Warning: Heap fragmentation detected");
    }
}
```

#### 2. Use Static Allocation When Possible
```cpp
// Instead of dynamic allocation
StaticJsonDocument<1024> doc;  // Pre-allocated

// Or use stack allocation for small objects
char buffer[256];
snprintf(buffer, sizeof(buffer), "{\"value\":%d}", value);
```

## Compilation Issues

### Library Not Found

**Symptoms:**
- "painlessMesh.h: No such file or directory"
- Library compilation errors

**Solutions:**

#### 1. Arduino IDE
- Go to **Sketch → Include Library → Manage Libraries**
- Search for "painlessMesh" and install latest version
- Ensure ArduinoJson and TaskScheduler are also installed

#### 2. PlatformIO
Add to `platformio.ini`:
```ini
lib_deps = 
    painlessMesh
    bblanchon/ArduinoJson@^6.21.3
    arkhipenko/TaskScheduler@^3.7.0
```

### Version Compatibility Issues

**Symptoms:**
- Compilation errors after library updates
- API function not found errors
- Deprecated warnings

**Solutions:**

#### 1. Check Version Compatibility
```cpp
// Check painlessMesh version
#include "painlessMesh.h"
Serial.printf("painlessMesh version: %s\n", PAINLESSMESH_VERSION);
```

#### 2. Lock Library Versions
In `platformio.ini`:
```ini
lib_deps = 
    painlessMesh@1.5.0        # Lock to specific version
    bblanchon/ArduinoJson@6.21.3
    arkhipenko/TaskScheduler@3.7.0
```

#### 3. Update Deprecated APIs
```cpp
// Old API (deprecated)
mesh.onReceive(&receivedCallback);

// New API (if changed in your version)
mesh.onReceive([](uint32_t from, String& msg) {
    receivedCallback(from, msg);
});
```

## Performance Issues

### Slow Message Delivery

**Symptoms:**
- Messages take several seconds to arrive
- High latency in mesh communication
- Sluggish response times

**Solutions:**

#### 1. Check Network Topology
```cpp
void printTopology() {
    String topology = mesh.subConnectionJson(true);
    Serial.println("Current topology:");
    Serial.println(topology);
    
    // Look for long chains or star topologies
    // Optimal: balanced tree with short paths
}
```

#### 2. Reduce Message Frequency
```cpp
// Instead of high frequency
Task taskFastSender(1000, TASK_FOREVER, &sendMessage);  // Every second

// Use lower frequency
Task taskSlowSender(10000, TASK_FOREVER, &sendMessage); // Every 10 seconds
```

#### 3. Optimize Message Size
```cpp
// Compact JSON formatting
String createOptimizedMessage() {
    // Use short field names
    String msg = "{\"t\":" + String(temp) + ",\"h\":" + String(hum) + "}";
    return msg;
}
```

### High CPU Usage

**Symptoms:**
- ESP becomes hot during operation
- Watchdog timer resets
- Sluggish response to other tasks

**Solutions:**

#### 1. Add Delays in Tight Loops
```cpp
void loop() {
    mesh.update();
    
    // Add small delay to prevent CPU overload
    delay(10);
    
    // Or yield to other tasks
    yield();
}
```

#### 2. Reduce Debug Output
```cpp
// Only use essential debug messages in production
mesh.setDebugMsgTypes(ERROR);  // Only errors
```

#### 3. Optimize Task Scheduling
```cpp
// Spread tasks over time
Task task1(30000, TASK_FOREVER, &function1);     // Every 30s
Task task2(35000, TASK_FOREVER, &function2);     // Every 35s (offset)
Task task3(40000, TASK_FOREVER, &function3);     // Every 40s
```

## Platform-Specific Issues

### ESP8266 Specific

**Reset Loops:**
```cpp
// Increase watchdog timeout
ESP.wdtDisable();
// Perform long operations
ESP.wdtEnable(5000);  // 5 second timeout
```

**Flash Memory Issues:**
```cpp
// Check flash size
Serial.printf("Flash size: %u bytes\n", ESP.getFlashChipSize());

// Ensure adequate space for SPIFFS/LittleFS
// Reserve at least 64KB for filesystem
```

### ESP32 Specific

**Core Affinity:**
```cpp
// Pin mesh tasks to specific core if needed
void setup() {
    // Use core 0 for mesh (core 1 for app)
    xTaskCreatePinnedToCore(meshTask, "MeshTask", 8192, NULL, 1, NULL, 0);
}
```

**Partition Scheme:**
Ensure adequate partition sizes in partition table:
```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x140000,
app1,     app,  ota_1,   0x150000,0x140000,
spiffs,   data, spiffs,  0x290000,0x160000,
```

## Debugging Techniques

### Serial Output Analysis

Enable comprehensive debugging:
```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | SYNC | 
                     COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);
```

Look for patterns in the output:
- Repeated connection attempts
- Memory allocation failures
- Message transmission errors
- Time synchronization issues

### Network Analysis

Use WiFi monitoring tools:
- WiFi Analyzer apps to check channel congestion
- Router logs to see connection patterns
- Packet capture tools for advanced debugging

### Code Instrumentation

Add timing measurements:
```cpp
void timedFunction() {
    unsigned long start = millis();
    
    // Your code here
    performOperation();
    
    unsigned long duration = millis() - start;
    if (duration > 1000) {  // Alert if > 1 second
        Serial.printf("Slow operation: %lu ms\n", duration);
    }
}
```

## Getting Help

If you're still experiencing issues:

1. **Check the [FAQ](faq.md)** for additional solutions
2. **Search existing issues** on [GitHub](https://github.com/Alteriom/painlessMesh/issues)
3. **Post in the community forum** with:
   - Complete serial output with debug enabled
   - Hardware details (ESP32/ESP8266 model)
   - Network topology (number of nodes, layout)
   - Code snippets showing the problem
4. **Create a minimal test case** that reproduces the issue
5. **Include library versions** and platform information

## Prevention Best Practices

- **Start simple** - Test with 2 nodes before scaling up
- **Monitor resources** - Check memory and CPU usage regularly
- **Use version control** - Track changes that might introduce issues
- **Test incremental changes** - Don't change everything at once
- **Document your setup** - Keep notes on working configurations
- **Regular testing** - Verify mesh operation after any changes

Remember: Most mesh networking issues are related to network configuration, power supply, or memory management. Check these fundamentals first before diving into complex debugging.