# Queued Alarms Example

## Overview

This example demonstrates **priority-based message queueing** for critical IoT systems that cannot afford to lose data during Internet outages. It's designed for the fish farm dissolved oxygen (O2) monitoring use case described in [Issue #66](https://github.com/Alteriom/painlessMesh/issues/66).

## Problem Statement

In production IoT systems like fish farm monitoring, **critical alarms must never be lost**. When the bridge node loses Internet connectivity:

- ❌ **Without queueing**: Critical O2 alarms are lost → fish die
- ✅ **With queueing**: Alarms are queued and delivered when connection restored → supervisor notified, fish saved

## Features

### ✅ Priority-Based Queueing

- **CRITICAL** (Priority 0): Life-safety alarms - never dropped
- **HIGH** (Priority 1): Important warnings - preserved up to 80% capacity
- **NORMAL** (Priority 2): Regular data - preserved up to 60% capacity  
- **LOW** (Priority 3): Telemetry - dropped first when queue full

### ✅ Automatic Queue Management

- Queues messages when Internet unavailable
- Flushes queue when Internet restored
- Prunes old messages (configurable age)
- Monitors queue health with callbacks

### ✅ Production Ready

- Survives Internet outages (queuing)
- Handles queue overflow intelligently
- Provides queue statistics
- Retry logic with attempt tracking

## Hardware Requirements

- **ESP32** or **ESP8266** 
- At least 2 nodes (1 bridge + 1 sensor node)
- Bridge node needs WiFi router access

## Configuration

### 1. Mesh Network Settings

```cpp
#define MESH_PREFIX     "FishFarmMesh"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555
```

### 2. Router Credentials (Bridge Node)

```cpp
#define ROUTER_SSID     "YourWiFiSSID"
#define ROUTER_PASSWORD "YourWiFiPassword"
```

Enable in setup for bridge node:
```cpp
mesh.stationManual(ROUTER_SSID, ROUTER_PASSWORD);
```

### 3. Sensor Thresholds

Adjust for your sensors (dissolved oxygen in mg/L):

```cpp
#define CRITICAL_O2_THRESHOLD  3.0  // Life-critical
#define WARNING_O2_THRESHOLD   5.0  // Warning level
```

### 4. Queue Configuration

```cpp
#define MAX_QUEUE_SIZE  500  // Max messages
#define QUEUE_PRUNE_AGE (24 * 60 * 60 * 1000)  // 24 hours
```

## How It Works

### Normal Operation (Internet Available)

```
Sensor → Mesh → Bridge → Internet → Cloud/MQTT
```

Messages sent immediately, no queueing.

### Offline Mode (No Internet)

```
Sensor → Mesh → Bridge → Queue (Priority-based)
                         ↓
                    [CRITICAL never dropped]
                    [LOW dropped first]
```

Messages queued with priority, delivered when Internet restored.

### Internet Restored

```
Queue → Flush → MQTT/HTTP → Cloud
  ↓
Remove on success
Retry on failure (max 3 attempts)
```

## Usage

### 1. Flash Bridge Node

1. Uncomment these lines in `setup()`:
   ```cpp
   mesh.stationManual(ROUTER_SSID, ROUTER_PASSWORD);
   mesh.setHostname("FishFarmBridge");
   ```
2. Upload to ESP32/ESP8266 with router access
3. Bridge connects to router and provides Internet to mesh

### 2. Flash Sensor Nodes

1. Leave router lines commented
2. Upload to sensor node ESP32/ESP8266
3. Node joins mesh and monitors sensors

### 3. Monitor Serial Output

**Normal operation:**
```
✅ ONLINE MODE - Internet restored
📊 Telemetry: 7.32 mg/L
📊 Telemetry: 6.85 mg/L
```

**Internet lost:**
```
⚠️  OFFLINE MODE ACTIVATED
  Queue size: 0 messages
📊 Telemetry: 6.42 mg/L - queued #1
  [Queue: 1 messages]
```

**Critical alarm (offline):**
```
🚨 CRITICAL O2 ALARM: 2.87 mg/L - QUEUED #5
  [Queue: 5 messages (1 CRITICAL)]
```

**Internet restored:**
```
✅ ONLINE MODE - Internet restored
  Flushing 5 queued messages...
  Sending queued message #1 (priority=3, attempts=0)
  Sending queued message #5 (priority=0, attempts=0)
  ✅ Queue flushed (5 messages sent)
```

## Queue States

The example monitors queue health:

- **EMPTY**: No messages queued
- **NORMAL**: Queue has space available
- **75% FULL**: Warning - queue reaching capacity
- **FULL**: Queue full - dropping LOW priority messages

Example output:
```
⚠️  Queue 75% full (375 messages)
🚨 Queue FULL (500 messages) - dropping LOW priority
```

## Testing Without Hardware

### Simulate Internet Loss

In real deployment, Internet loss is automatic. For testing, you can:

1. **Disconnect router**: Physically disconnect Ethernet/WAN
2. **Block MAC address**: Router settings → Block bridge MAC
3. **Power cycle router**: Turn off router
4. **Modify code**: Add test button to toggle `offlineMode`

### Verify Queue Behavior

1. Start with Internet connected
2. Cause Internet loss (any method above)
3. Wait for critical alarms to queue
4. Restore Internet
5. Verify messages are flushed

Expected sequence:
```
✅ Online → ⚠️ Offline (queueing) → ✅ Online (flush queue)
```

## Integration with Cloud Services

### MQTT Example

Replace simulated sending with MQTT:

```cpp
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// In setup()
mqttClient.setServer("mqtt.example.com", 1883);

// In bridgeStatusCallback()
for (auto& msg : messages) {
  bool sent = mqttClient.publish(
    msg.destination.c_str(),  // Topic from queueMessage()
    msg.payload.c_str()
  );
  
  if (sent) {
    mesh.removeQueuedMessage(msg.id);
  } else {
    mesh.incrementQueuedMessageAttempts(msg.id);
  }
}
```

### HTTP Example

Replace simulated sending with HTTP POST:

```cpp
#include <HTTPClient.h>

HTTPClient http;

for (auto& msg : messages) {
  http.begin(msg.destination);  // URL from queueMessage()
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.POST(msg.payload);
  bool sent = (httpCode == 200 || httpCode == 201);
  
  if (sent) {
    mesh.removeQueuedMessage(msg.id);
  } else {
    mesh.incrementQueuedMessageAttempts(msg.id);
  }
  
  http.end();
}
```

## Message Format

Example JSON message structure:

### Critical Alarm
```json
{
  "type": "CRITICAL_ALARM",
  "sensor": "O2",
  "value": 2.87,
  "threshold": 3.0,
  "tankId": "TANK_A",
  "nodeId": 123456789,
  "timestamp": 1234567890
}
```

### Warning
```json
{
  "type": "WARNING",
  "sensor": "O2",
  "value": 4.5,
  "threshold": 5.0,
  "nodeId": 123456789
}
```

### Telemetry
```json
{
  "sensor": "O2",
  "value": 7.32,
  "nodeId": 123456789
}
```

## API Reference

### Enable Queue

```cpp
mesh.enableMessageQueue(true, MAX_QUEUE_SIZE);
```

### Queue Message

```cpp
uint32_t msgId = mesh.queueMessage(
  payload,              // Message content
  destination,          // Cloud endpoint/topic
  PRIORITY_CRITICAL     // Priority level
);
```

### Flush Queue

```cpp
auto messages = mesh.flushMessageQueue();
for (auto& msg : messages) {
  if (sendToCloud(msg)) {
    mesh.removeQueuedMessage(msg.id);
  }
}
```

### Query Queue

```cpp
uint32_t total = mesh.getQueuedMessageCount();
uint32_t critical = mesh.getQueuedMessageCount(PRIORITY_CRITICAL);
```

### Callbacks

```cpp
mesh.onBridgeStatusChanged([](uint32_t bridgeId, bool hasInternet) {
  // Handle Internet connectivity change
});

mesh.onQueueStateChanged([](QueueState state, uint32_t count) {
  // Handle queue state change (EMPTY, NORMAL, 75%, FULL)
});
```

## Troubleshooting

### Queue Always Full

- Increase `MAX_QUEUE_SIZE`
- Decrease message frequency
- Lower message priorities
- Reduce `QUEUE_PRUNE_AGE`

### Messages Not Flushing

- Check `bridgeStatusCallback()` is called
- Verify Internet connectivity with `mesh.hasInternetConnection()`
- Check MQTT/HTTP sending code
- Monitor serial for errors

### High Memory Usage

- Reduce `MAX_QUEUE_SIZE`
- Enable aggressive pruning
- Use shorter message payloads
- Monitor with `ESP.getFreeHeap()`

## Performance

### Memory Usage

| Queue Size | RAM Usage (approx) |
|------------|-------------------|
| 100        | ~20 KB            |
| 500        | ~100 KB           |
| 1000       | ~200 KB           |

**ESP32**: Can handle 1000+ messages  
**ESP8266**: Recommend ≤500 messages

### Throughput

- **Queue**: ~1000 msg/sec
- **Flush**: Limited by MQTT/HTTP send rate (~10-50 msg/sec)

## Related Documentation

- [Issue #66: Message Queueing Feature](https://github.com/Alteriom/painlessMesh/issues/66)
- [Issue #63: Bridge Status Broadcast](https://github.com/Alteriom/painlessMesh/issues/63)
- [painlessMesh Documentation](https://gitlab.com/painlessMesh/painlessMesh)

## License

MIT License - See repository LICENSE file
