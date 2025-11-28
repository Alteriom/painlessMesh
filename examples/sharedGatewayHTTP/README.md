# Shared Gateway HTTP Example

This example demonstrates how to use **Shared Gateway Mode** with painlessMesh to send HTTP/HTTPS requests to external cloud APIs. It showcases advanced features including automatic failover, retry logic, priority messaging, and comprehensive status monitoring.

## Overview

In Shared Gateway HTTP mode:
- **All nodes** connect to the same WiFi router
- **All nodes** maintain mesh connectivity
- **Any node** can send HTTP requests to external APIs
- Automatic **failover** when Internet connection is lost
- **Priority messaging** for critical alarms
- **Retry logic** for failed requests

## Architecture

```
     ┌─────────────────────────────────────────────────┐
     │                   Internet                      │
     │            (api.example.com)                    │
     └──────────────────────┬──────────────────────────┘
                            │
                      WiFi Router
          ┌─────────────────┼─────────────────┐
          │                 │                 │
      Node A            Node B            Node C
      (HTTP Client)     (HTTP Client)     (HTTP Client)
      [AP+STA]          [AP+STA]          [AP+STA]
          │                 │                 │
          └─────────────────┼─────────────────┘
                      Mesh Network
```

## Features Demonstrated

### 1. HTTP/HTTPS Requests
- POST requests to external APIs
- JSON payload serialization
- Response handling and logging
- Support for both HTTP and HTTPS

### 2. Gateway Packages
- **GatewayDataPackage** (Type 620): For routing Internet requests
- **GatewayAckPackage** (Type 621): For delivery confirmations
- **GatewayPriority**: CRITICAL(0), HIGH(1), NORMAL(2), LOW(3)

### 3. Failover Support
- Automatic Internet connectivity checking
- Request queuing when offline
- Automatic retry when connection is restored

### 4. Status Monitoring
- Periodic status reports
- HTTP request statistics
- Memory and connectivity monitoring

## Hardware Requirements

- **ESP32** or **ESP8266** microcontroller
- WiFi router within range of all nodes
- Internet connectivity for external API access
- USB cable for programming and serial monitoring

## Configuration

Edit the following defines in `sharedGatewayHTTP.ino`:

### Mesh Network Settings
```cpp
// Must match across all nodes
#define MESH_PREFIX     "SharedHTTPMesh"     // Mesh network name
#define MESH_PASSWORD   "meshPassword123"    // Mesh network password
#define MESH_PORT       5555                 // TCP port (default: 5555)
```

### Router Settings
```cpp
// Your WiFi router configuration
#define ROUTER_SSID     "YourRouterSSID"     // Router name
#define ROUTER_PASSWORD "YourRouterPassword" // Router password
```

### HTTP API Settings
```cpp
// Target API endpoint
#define API_ENDPOINT    "http://httpbin.org/post"  // Test endpoint
#define API_HOST        "httpbin.org"              // Host for HTTPS
#define USE_HTTPS       false                      // true for HTTPS
#define HTTP_TIMEOUT    10000                      // Timeout in ms
```

### Timing Settings
```cpp
#define SENSOR_INTERVAL         30000  // Sensor data interval (ms)
#define STATUS_INTERVAL         10000  // Status report interval (ms)
#define RETRY_DELAY             5000   // Retry delay (ms)
#define MAX_RETRIES             3      // Max retry attempts
#define INTERNET_CHECK_INTERVAL 30000  // Internet check interval (ms)
```

## Building and Uploading

### Using PlatformIO (Recommended)

```bash
cd examples/sharedGatewayHTTP

# For ESP32
pio run -e esp32 -t upload

# For ESP8266
pio run -e esp8266 -t upload
```

### Using Arduino IDE

1. Open `sharedGatewayHTTP.ino` in Arduino IDE
2. Install the required libraries:
   - ArduinoJson
   - TaskScheduler
   - painlessMesh
   - AsyncTCP (ESP32) or ESPAsyncTCP (ESP8266)
3. Select your board (ESP32 or ESP8266)
4. Click Upload

## Expected Serial Output

After uploading, open Serial Monitor at 115200 baud:

### Startup Output
```
================================================
   painlessMesh - Shared Gateway HTTP Example
================================================

Target API: http://httpbin.org/post
HTTPS: Disabled

Initializing Shared Gateway Mode...

  Mesh SSID: SharedHTTPMesh
  Mesh Port: 5555
  Router SSID: YourRouterSSID

✓ Shared Gateway Mode initialized successfully!

================================================
Node ID: 2748965421
Shared Gateway Mode: Active
Router IP: 192.168.1.105
Router Channel: 6
Router RSSI: -45 dBm
Internet: Available
================================================

Tasks Enabled:
  - Sensor data upload every 30 seconds
  - Status report every 10 seconds
  - Internet check every 30 seconds
  - Retry pending requests every 5 seconds

Ready for operation!
```

### Sensor Data Upload
```
======== SENSOR DATA UPLOAD ========
Payload: {"nodeId":2748965421,"timestamp":1234567890,"temperature":25.3,...}
  → Sending HTTP request to: http://httpbin.org/post
  ← HTTP Response: 200
  Response: {"args":{},"data":"{...}","files":{},...
  ✓ Sensor data uploaded successfully (HTTP 200)
====================================
```

### Status Report
```
========== STATUS REPORT ==========
Node ID: 2748965421
Uptime: 120 seconds
Free Heap: 45320 bytes

--- Connectivity ---
Router: Connected
  IP: 192.168.1.105
  RSSI: -45 dBm
  Channel: 6
Internet: Available
Shared Gateway Mode: Active

--- Mesh Network ---
Mesh Nodes: 2

--- HTTP Statistics ---
Total Requests: 4
Successful: 4
Failed: 0
Pending Retry: 0
Success Rate: 100.0%
===================================
```

### Failover Behavior
```
⚠️  WiFi disconnected - Internet unavailable

======== SENSOR DATA UPLOAD ========
Payload: {"nodeId":2748965421,...}
  ⏳ Request queued for retry (ID: 12345, Queue size: 1)
====================================

✓ Internet connected
  Retrying 1 pending requests...

🔄 Retrying request 12345 (attempt 1/3)
  → Sending HTTP request to: http://httpbin.org/post
  ← HTTP Response: 200
  ✓ Retry successful (HTTP 200)
```

## Testing Failover

To test the failover functionality:

### Method 1: Disconnect Router
1. Start the node with the sketch running
2. Verify "Internet: Available" in status
3. Disconnect the router from Internet (unplug WAN cable)
4. Observe "Internet disconnected - failover active" message
5. Sensor data will be queued
6. Reconnect router's WAN cable
7. Observe automatic retry of queued requests

### Method 2: Simulate Internet Loss
1. Change your router's DNS settings to invalid values
2. The node will detect Internet loss (TCP check fails)
3. Requests will be queued
4. Restore DNS settings
5. Observe automatic retry

### Method 3: Move Out of Range
1. Start node within router range
2. Move node out of WiFi range
3. Observe failover behavior
4. Move back into range
5. Connection and queue processing resumes

## Troubleshooting

### HTTP Request Failures

**Symptoms:** `✗ HTTP request failed: connection refused`

**Solutions:**
1. Verify API endpoint URL is correct
2. Check Internet connectivity
3. Ensure firewall allows outbound HTTP/HTTPS
4. Try a different test endpoint (httpbin.org, postman-echo.com)

### HTTPS Certificate Errors

**Symptoms:** `✗ HTTP request failed: SSL error`

**Solutions:**
1. For testing, certificate validation is disabled
2. For production, add proper CA certificates
3. Verify system time is correct (SSL requires valid time)
4. Check if the server certificate is valid

### Queue Overflow

**Symptoms:** `⚠️ Queue full - request dropped`

**Solutions:**
1. Increase queue size (default: 10)
2. Increase retry frequency
3. Reduce sensor data frequency
4. Monitor `pendingRequests.size()` in status

### Memory Issues

**Symptoms:** Free heap decreasing, crashes

**Solutions:**
1. Reduce JSON document sizes
2. Limit pending request queue
3. Use ESP32 for larger workloads
4. Monitor `ESP.getFreeHeap()` regularly

## API Reference

### Key Gateway Types

```cpp
// GatewayDataPackage (Type 620) - For Internet requests
GatewayDataPackage pkg;
pkg.messageId = generateMessageId();
pkg.originNode = mesh.getNodeId();
pkg.timestamp = mesh.getNodeTime();
pkg.priority = static_cast<uint8_t>(GatewayPriority::PRIORITY_NORMAL);
pkg.destination = "https://api.example.com/data";
pkg.payload = "{\"sensor\": 42}";
pkg.contentType = "application/json";
pkg.requiresAck = true;

// GatewayAckPackage (Type 621) - For confirmations
GatewayAckPackage ack;
ack.messageId = originalPackage.messageId;
ack.originNode = originalPackage.originNode;
ack.success = true;
ack.httpStatus = 200;
ack.timestamp = mesh.getNodeTime();

// Priority Levels
GatewayPriority::PRIORITY_CRITICAL = 0  // Immediate
GatewayPriority::PRIORITY_HIGH = 1      // Urgent
GatewayPriority::PRIORITY_NORMAL = 2    // Standard
GatewayPriority::PRIORITY_LOW = 3       // Background
```

### Key Functions

```cpp
// Send HTTP request
bool sendHttpRequest(const String& url, const String& payload, int& httpCode);

// Send critical alarm (highest priority)
void sendCriticalAlarm(const String& alarmType, const String& message);

// Check Internet connectivity
void checkInternetConnectivity();

// Retry pending requests
void retryPendingRequests();
```

### Mesh Methods

```cpp
// Initialize shared gateway mode
bool success = mesh.initAsSharedGateway(
    MESH_PREFIX, MESH_PASSWORD,
    ROUTER_SSID, ROUTER_PASSWORD,
    &userScheduler, MESH_PORT
);

// Check mode status
bool isGateway = mesh.isSharedGatewayMode();

// Send with priority
mesh.sendBroadcast(msg, priority);  // 0-3
```

## Use Cases

### IoT Sensor Data Upload
Upload temperature, humidity, and other sensor data to cloud services like AWS IoT, Azure IoT Hub, or custom APIs.

### Fish Farm Monitoring
Critical O2 level alarms reach the cloud through any available node, ensuring no alerts are missed even if some nodes lose connectivity.

### Industrial Monitoring
Send machine telemetry and alarms to SCADA systems or cloud platforms with guaranteed delivery through automatic retry.

### Smart Building Systems
HVAC data and security alerts maintain connectivity through redundant paths, with priority handling for emergencies.

## Related Documentation

- [Shared Gateway Basic Example](../sharedGateway/sharedGateway.ino)
- [Gateway Design Documentation](../../docs/design/SHARED_GATEWAY_DESIGN.md)
- [Priority Messaging](../priority/README.md)
- [Bridge Failover Example](../bridge_failover/README.md)
- [MQTT Bridge Example](../mqttBridge/mqttBridge.ino)

## License

This example is part of painlessMesh and is released under the same license.
