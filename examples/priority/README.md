# Priority-Based Message Sending

## Overview

painlessMesh now supports priority-based message sending with 4 distinct priority levels. This ensures that critical messages (alarms, commands) are delivered before less important messages (logs, routine sensor data), even under high network load.

## Priority Levels

| Priority | Value | Use Case | Examples |
|----------|-------|----------|----------|
| **CRITICAL** | 0 | Life/safety critical messages | Fire alarms, oxygen level warnings, emergency shutdowns |
| **HIGH** | 1 | Important commands and urgent status | Device commands, critical status requests, immediate alerts |
| **NORMAL** | 2 | Regular operational data (default) | Sensor readings, routine updates, telemetry |
| **LOW** | 3 | Non-essential information | Debug logs, verbose telemetry, statistics |

## API Reference

### Broadcast Messages with Priority

```cpp
// Send with default NORMAL priority (backward compatible)
mesh.sendBroadcast(message);

// Send with explicit priority level (0-3)
mesh.sendBroadcast(message, priorityLevel);

// Examples
mesh.sendBroadcast(alarmData, 0);    // CRITICAL priority
mesh.sendBroadcast(command, 1);      // HIGH priority
mesh.sendBroadcast(sensorData, 2);   // NORMAL priority
mesh.sendBroadcast(debugLog, 3);     // LOW priority
```

### Direct Messages with Priority

```cpp
// Send to specific node with default NORMAL priority
mesh.sendSingle(destinationNodeId, message);

// Send to specific node with explicit priority
mesh.sendSingle(destinationNodeId, message, priorityLevel);

// Examples
mesh.sendSingle(123456, alarmMsg, 0);   // CRITICAL
mesh.sendSingle(123456, cmdMsg, 1);     // HIGH
mesh.sendSingle(123456, dataMsg, 2);    // NORMAL
mesh.sendSingle(123456, logMsg, 3);     // LOW
```

## How It Works

### Priority Scheduling

1. **Queue Management**: Messages are queued by priority level
2. **Send Order**: Higher priority messages (lower numbers) are sent first
3. **TCP Push**: CRITICAL (0) and HIGH (1) messages trigger immediate TCP push for faster delivery
4. **Fairness**: Within the same priority level, messages maintain FIFO order

### Performance Characteristics

- **Memory Overhead**: Minimal - adds ~8 bytes per queued message
- **CPU Overhead**: O(n) scan to find highest priority (typically negligible with small queues)
- **Network Impact**: 
  - CRITICAL/HIGH messages call `client->send()` for immediate transmission
  - NORMAL/LOW messages use standard TCP buffering

## Best Practices

### 1. Choose Appropriate Priority Levels

```cpp
// ✅ GOOD: Reserve CRITICAL for actual emergencies
if (oxygenLevel < CRITICAL_THRESHOLD) {
    mesh.sendBroadcast(alarmData, 0);  // CRITICAL
}

// ❌ BAD: Don't overuse CRITICAL priority
mesh.sendBroadcast(routineSensorData, 0);  // Wrong!
```

### 2. Use Priority Consistently

```cpp
// Define constants for clarity
const uint8_t PRIORITY_ALARM = 0;
const uint8_t PRIORITY_COMMAND = 1;
const uint8_t PRIORITY_SENSOR = 2;
const uint8_t PRIORITY_DEBUG = 3;

// Use throughout your code
mesh.sendBroadcast(alarmMsg, PRIORITY_ALARM);
mesh.sendBroadcast(sensorMsg, PRIORITY_SENSOR);
```

### 3. Consider Message Frequency

```cpp
// High-frequency messages should use NORMAL or LOW priority
void loop() {
    if (millis() - lastSensorRead > 1000) {
        // Sensor data every second - use NORMAL
        mesh.sendBroadcast(sensorData, 2);
    }
}

// Low-frequency critical events use CRITICAL/HIGH
void onAlarmDetected() {
    // Rare but critical event
    mesh.sendBroadcast(alarmData, 0);
}
```

### 4. Test Under Load

Always test your priority assignments under realistic network load:

```cpp
// Simulate high load
for (int i = 0; i < 100; i++) {
    mesh.sendBroadcast(normalData, 2);  // Many NORMAL messages
}

// Verify CRITICAL messages still get through quickly
mesh.sendBroadcast(criticalAlarm, 0);
```

## Examples

### Example 1: Safety Monitoring System

```cpp
void monitorSafety() {
    // Read oxygen sensor
    float oxygenLevel = readOxygenSensor();
    
    if (oxygenLevel < CRITICAL_LEVEL) {
        // CRITICAL: Life-threatening situation
        String alarm = "{\"type\":\"alarm\",\"sensor\":\"oxygen\",\"level\":" + 
                       String(oxygenLevel) + ",\"severity\":\"critical\"}";
        mesh.sendBroadcast(alarm, 0);
        
    } else if (oxygenLevel < WARNING_LEVEL) {
        // HIGH: Concerning but not immediate danger
        String warning = "{\"type\":\"warning\",\"sensor\":\"oxygen\",\"level\":" + 
                        String(oxygenLevel) + "}";
        mesh.sendBroadcast(warning, 1);
        
    } else {
        // NORMAL: Regular monitoring data
        String data = "{\"type\":\"sensor\",\"oxygen\":" + String(oxygenLevel) + "}";
        mesh.sendBroadcast(data, 2);
    }
}
```

### Example 2: Industrial Control System

```cpp
void sendControlCommand(uint32_t targetNode, String command) {
    // Control commands are HIGH priority
    String cmdMsg = "{\"type\":\"control\",\"command\":\"" + command + "\"}";
    mesh.sendSingle(targetNode, cmdMsg, 1);
}

void sendSensorData() {
    // Regular sensor data is NORMAL priority
    String data = "{\"type\":\"telemetry\",\"temp\":" + String(getTemp()) + "}";
    mesh.sendBroadcast(data, 2);
}

void sendDebugInfo() {
    // Debug logs are LOW priority
    String debug = "{\"type\":\"debug\",\"heap\":" + String(ESP.getFreeHeap()) + "}";
    mesh.sendBroadcast(debug, 3);
}
```

### Example 3: Smart Building Automation

```cpp
void handleFireAlarm() {
    // CRITICAL: Fire detected
    String alarm = "{\"type\":\"fire_alarm\",\"location\":\"Building A\",\"floor\":3}";
    mesh.sendBroadcast(alarm, 0);
    
    // Also send HIGH priority evacuation command to all displays
    String evacCmd = "{\"type\":\"command\",\"action\":\"evacuate\"}";
    mesh.sendBroadcast(evacCmd, 1);
}

void updateLighting() {
    // NORMAL: Regular lighting adjustments
    String lightingData = "{\"type\":\"lighting\",\"brightness\":75}";
    mesh.sendBroadcast(lightingData, 2);
}

void logActivity() {
    // LOW: Activity logs
    String log = "{\"type\":\"log\",\"activity\":\"motion_detected\",\"room\":\"Conference A\"}";
    mesh.sendBroadcast(log, 3);
}
```

## Statistics and Monitoring

The priority system tracks statistics for monitoring:

```cpp
// Note: Statistics API is internal to SentBuffer
// Access through connection stats if needed for debugging
```

## Backward Compatibility

The priority system is **100% backward compatible**:

```cpp
// Old code continues to work (uses NORMAL priority)
mesh.sendBroadcast(message);
mesh.sendSingle(nodeId, message);

// Legacy bool priority flag still works (false=NORMAL, true=HIGH)
mesh.sendBroadcast(message, true);  // Maps to HIGH priority
```

## Performance Considerations

### Memory Usage

- **Per Message**: +8 bytes (priority field + iterator overhead)
- **Total Overhead**: Minimal, typically <1KB for 100 queued messages

### ESP8266 vs ESP32

Both platforms support all 4 priority levels. On ESP8266 with limited memory:
- Monitor queue sizes
- Use LOW priority for verbose/debug messages
- Consider message frequency when assigning priorities

### Network Load

Under high load:
- CRITICAL messages bypass most queuing
- HIGH messages get priority scheduling
- NORMAL messages may experience slight delays
- LOW messages may be delayed significantly

This is by design - it ensures critical messages get through when you need them most.

## Troubleshooting

### Issue: CRITICAL messages still delayed

**Cause**: Network congestion at lower layers (WiFi, TCP)

**Solution**: 
- Reduce overall message volume
- Increase message send intervals for LOW priority messages
- Consider mesh topology (reduce hops)

### Issue: LOW priority messages never sent

**Cause**: Continuous stream of HIGH priority messages

**Solution**:
- Review HIGH priority message frequency
- Ensure proper priority assignment
- Implement rate limiting on HIGH priority sources

## See Also

- [Message Queue](../queued_alarms/README.md) - For offline message queueing
- [Basic Example](../basic/basic.ino) - Getting started with painlessMesh
- [Bridge Examples](../bridge/) - Internet connectivity patterns
