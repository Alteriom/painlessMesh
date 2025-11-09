# Queued Alarms Example - Fish Farm Oxygen Monitoring

This example demonstrates message queuing for critical IoT applications where message loss during Internet outages is unacceptable.

## Use Case: Fish Farm Oxygen Monitoring

In aquaculture (fish farming), dissolved oxygen (DO) levels are critical for fish survival. If oxygen levels drop below 3 mg/L, fish can die within minutes. This example shows how to:

- Monitor dissolved oxygen levels continuously
- Send critical alarms that **must never be lost**
- Queue messages during Internet outages
- Automatically deliver all queued messages when connectivity is restored
- Persist critical messages across device reboots

## Features Demonstrated

### 1. Priority-Based Queuing

Messages are queued with four priority levels:

- **CRITICAL** (Priority 0): Life-threatening conditions - never dropped
  - Example: O2 < 3.0 mg/L (fish will die)
  
- **HIGH** (Priority 1): Important warnings - queued up to 80% capacity
  - Example: O2 < 5.0 mg/L (approaching danger zone)
  
- **NORMAL** (Priority 2): Regular data - queued up to 60% capacity
  - Example: Regular sensor readings
  
- **LOW** (Priority 3): Non-essential data - first to be dropped when queue fills
  - Example: Periodic telemetry

### 2. Persistent Storage

Critical messages are saved to SPIFFS/LittleFS and survive:
- Device reboots
- Power failures
- Firmware updates

### 3. Automatic Queue Flushing

When Internet connectivity is restored:
- All queued messages are automatically sent
- Messages are retried up to 3 times if sending fails
- Failed messages are removed after max retries
- Queue state is saved to persistent storage

### 4. Queue State Monitoring

Callbacks notify you when:
- Queue reaches 25%, 50%, 75%, or 100% capacity
- You can take action (e.g., alert operators, reduce telemetry frequency)

## Hardware Requirements

- **ESP32** or **ESP8266** microcontroller
- **Dissolved Oxygen Sensor** (simulated in this example)
  - Real sensors: Atlas Scientific DO sensor, DFRobot Gravity Analog DO sensor
- **WiFi connection** to router with Internet access

## Software Dependencies

- **painlessMesh** library (this repository)
- **ArduinoJson** library
- **SPIFFS** or **LittleFS** filesystem support

## Configuration

### Mesh Settings

```cpp
#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_mesh_password"
#define MESH_PORT       5555
```

### Router Settings (for bridge node)

```cpp
#define ROUTER_SSID     "your_router_ssid"
#define ROUTER_PASSWORD "your_router_password"
```

### Oxygen Thresholds

```cpp
#define CRITICAL_O2_THRESHOLD  3.0   // mg/L - fish die below this
#define WARNING_O2_THRESHOLD   5.0   // mg/L - warning level
#define NORMAL_O2_MIN          6.0   // mg/L - healthy minimum
#define NORMAL_O2_MAX          8.0   // mg/L - healthy maximum
```

## Usage

### Bridge Node Setup

1. Upload this sketch to an ESP32/ESP8266 that has router access
2. Configure `ROUTER_SSID` and `ROUTER_PASSWORD`
3. The node will act as bridge to Internet
4. It broadcasts bridge status to other nodes

### Sensor Node Setup

1. Upload this sketch to ESP32/ESP8266 with sensor attached
2. Comment out the bridge-specific lines:
   ```cpp
   // mesh.setRouterCredentials(ROUTER_SSID, ROUTER_PASSWORD);
   // mesh.initBridgeStatusBroadcast();
   // mesh.enableBridgeFailover(true);
   ```
3. Connect actual dissolved oxygen sensor to appropriate pin
4. Update `readDissolvedOxygenSensor()` to read from real sensor

### Cloud Integration

Replace the `sendMessageToCloud()` function with your actual cloud sending code:

**MQTT Example:**
```cpp
bool sendMessageToCloud(const String& payload, const String& destination) {
  return mqttClient.publish(destination.c_str(), payload.c_str());
}
```

**HTTP Example:**
```cpp
bool sendMessageToCloud(const String& payload, const String& destination) {
  HTTPClient http;
  http.begin(destination);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(payload);
  http.end();
  return (httpCode == 200);
}
```

## Expected Behavior

### Normal Operation (Internet Available)

```
📊 Telemetry sent: O2=7.2 mg/L
📊 Telemetry sent: O2=7.5 mg/L
⚠️  WARNING O2 ALARM: 4.8 mg/L - SENT
🚨 CRITICAL O2 ALARM: 2.7 mg/L - SENT IMMEDIATELY
```

### Offline Mode (Internet Lost)

```
=== Bridge Status Update ===
Bridge Node: 123456789
Internet: Disconnected
⚠️  OFFLINE MODE ACTIVATED
   Queue size: 0 messages
   Critical messages: 0
===========================

🚨 CRITICAL O2 ALARM: 2.5 mg/L - QUEUED #1 (offline mode)
📊 Telemetry queued #2: O2=6.8 mg/L
⚠️  WARNING O2 ALARM: 4.5 mg/L - QUEUED #3
```

### Internet Restored

```
=== Bridge Status Update ===
Bridge Node: 123456789
Internet: Connected
✓ ONLINE MODE - Internet restored
   Flushing 3 queued messages...
☁️  Sending to cloud [mqtt://cloud.farm.com/alarms/critical]: {...}
☁️  Sending to cloud [mqtt://cloud.farm.com/telemetry]: {...}
☁️  Sending to cloud [mqtt://cloud.farm.com/alarms/warning]: {...}
✓ Sent 3 queued messages
===========================
```

## Queue Management

### View Queue Statistics

```cpp
painlessmesh::queue::QueueStats stats = mesh.getQueueStats();
Serial.printf("Total Queued: %u\n", stats.totalQueued);
Serial.printf("Total Sent: %u\n", stats.totalSent);
Serial.printf("Total Dropped: %u\n", stats.totalDropped);
Serial.printf("Current Size: %u\n", stats.currentSize);
```

### Manual Queue Flushing

```cpp
uint32_t sent = mesh.flushMessageQueue(sendMessageToCloud);
Serial.printf("Sent %u messages\n", sent);
```

### Clear Queue

```cpp
mesh.clearMessageQueue();
```

### Prune Old Messages

```cpp
// Remove messages older than 24 hours
uint32_t pruned = mesh.pruneMessageQueue(24);
```

## Troubleshooting

### Messages Not Being Queued

- Check that `enableMessageQueue()` was called in setup
- Verify queue size limit hasn't been reached
- Check Serial output for queue state messages

### Persistent Storage Not Working

- Ensure SPIFFS is initialized: `SPIFFS.begin(true)`
- Check available flash space: `SPIFFS.totalBytes()` and `SPIFFS.usedBytes()`
- Verify storage path is valid

### Messages Not Being Sent

- Check `sendMessageToCloud()` implementation returns correct status
- Verify Internet connectivity with `mesh.hasInternetConnection()`
- Check retry attempts: `mesh.setMaxQueueRetryAttempts(3)`

### Queue Filling Up

- Reduce telemetry frequency
- Increase max queue size: `mesh.enableMessageQueue(1000, true, path)`
- Prioritize more critical messages
- Prune old messages more frequently

## Real-World Deployment Tips

1. **Test Thoroughly**: Simulate Internet outages and power failures
2. **Monitor Queue Size**: Set up alerts when queue > 75% full
3. **Persistent Storage**: Always enable for production systems
4. **Battery Backup**: Use UPS for critical sensors
5. **Multiple Sensors**: Deploy redundant sensors per tank
6. **Cellular Backup**: Consider 4G/LTE fallback for critical locations
7. **Local Alarms**: Add buzzer/LED for immediate local notification

## API Reference

See message_queue.hpp for complete API documentation.

### Key Methods

```cpp
// Enable queue
mesh.enableMessageQueue(maxSize, enablePersistence, storagePath);

// Queue message with priority
uint32_t msgId = mesh.queueMessage(payload, destination, priority);

// Flush queue
uint32_t sent = mesh.flushMessageQueue(sendCallback);

// Get queue info
uint32_t count = mesh.getQueuedMessageCount();
uint32_t criticalCount = mesh.getQueuedMessageCount(PRIORITY_CRITICAL);

// Manage queue
mesh.pruneMessageQueue(maxAgeHours);
mesh.clearMessageQueue();
mesh.saveQueueToStorage();
mesh.loadQueueFromStorage();

// Callbacks
mesh.onQueueStateChanged(callback);
```

## License

This example is part of the AlteriomPainlessMesh library and follows the same license.

## Support

For issues or questions:
- GitHub Issues: https://github.com/Alteriom/painlessMesh/issues
- Documentation: https://alteriom.github.io/painlessMesh/

## Credits

Developed for the painlessMesh community by Alteriom.
Inspired by real-world aquaculture monitoring requirements.
