# Alteriom painlessMesh Extensions

This directory contains Alteriom-specific extensions and examples for the painlessMesh library.

## Package Types

### SensorPackage (Type 200)
Broadcast package for sharing environmental sensor data across the mesh.

**Fields:**
- `temperature` - Temperature in Celsius
- `humidity` - Relative humidity percentage  
- `pressure` - Atmospheric pressure in hPa
- `sensorId` - Unique sensor identifier
- `timestamp` - Unix timestamp of measurement
- `batteryLevel` - Battery level percentage (0-100)

### CommandPackage (Type 201)
Single-destination package for sending commands to specific nodes.

**Fields:**
- `command` - Command type identifier
- `targetDevice` - Target device ID
- `parameters` - Command parameters as JSON string
- `commandId` - Unique command identifier for tracking

### StatusPackage (Type 202)
Broadcast package for sharing device health and status information.

**Fields:**
- `deviceStatus` - Device status flags
- `uptime` - Device uptime in seconds
- `freeMemory` - Free memory in KB
- `wifiStrength` - WiFi signal strength (0-100)
- `firmwareVersion` - Current firmware version string

### EnhancedStatusPackage (Type 203) - Phase 1
Extended status package with comprehensive health metrics (18 fields).

**Additional Fields:**
- `firmwareMD5` - Firmware hash for OTA verification
- `nodeCount`, `connectionCount` - Mesh statistics
- `messagesReceived`, `messagesSent`, `messagesDropped` - Message counters
- `avgLatency`, `packetLossRate`, `throughput` - Performance metrics
- `alertFlags`, `lastError` - Alert system

### MetricsPackage (Type 204) - NEW in v1.7.7
Comprehensive performance metrics for detailed monitoring and dashboards.

**Key Fields:**
- `cpuUsage`, `loopIterations`, `taskQueueSize` - CPU and processing metrics
- `freeHeap`, `minFreeHeap`, `heapFragmentation`, `maxAllocHeap` - Memory metrics
- `bytesReceived`, `bytesSent`, `packetsReceived`, `packetsSent` - Network performance
- `avgResponseTime`, `maxResponseTime`, `avgMeshLatency` - Timing metrics
- `connectionQuality`, `wifiRSSI` - Connection quality indicators
- `collectionTimestamp`, `collectionInterval` - Collection metadata

**Use Cases:**
- Real-time performance dashboards
- Capacity planning and optimization
- Network throughput analysis
- Latency monitoring

### HealthCheckPackage (Type 605) - NEW in v1.7.7
Proactive health monitoring with problem detection and recommendations.

**Key Fields:**
- `healthStatus` - Overall health: 0=critical, 1=warning, 2=healthy
- `problemFlags` - Bit flags for specific problems (16 types)
- `memoryHealth`, `networkHealth`, `performanceHealth` - Component health scores (0-100)
- `memoryTrend` - Memory leak detection (bytes/hour)
- `packetLossPercent`, `reconnectionCount` - Network stability
- `missedDeadlines`, `maxLoopTime` - Performance indicators
- `temperature`, `temperatureHealth` - Environmental monitoring
- `estimatedTimeToFailure` - Predictive maintenance indicator
- `recommendations` - Actionable guidance for operators

**Problem Flags:**
- 0x0001 - Low memory warning
- 0x0002 - High CPU usage
- 0x0004 - Connection instability
- 0x0008 - High packet loss
- 0x0010 - Network congestion
- 0x0020 - Low battery
- 0x0040 - Thermal warning
- 0x0080 - Mesh partition detected
- 0x0100 - OTA in progress
- 0x0200 - Configuration error

**Use Cases:**
- Proactive problem detection
- Predictive maintenance
- Automated alerting
- Memory leak detection

## Examples

### `alteriom_sensor_node.ino`
Complete Arduino sketch demonstrating:
- Periodic sensor data broadcasting
- Command handling
- Status reporting
- Message type discrimination
- Integration with painlessMesh

### `phase1_features.ino` (NEW)
Phase 1 OTA enhancement example demonstrating:
- Compressed OTA transfer infrastructure
- Enhanced status reporting with comprehensive metrics
- Alert system implementation
- Usage patterns for Phase 1 features

### `metrics_health_node.ino` (NEW in v1.7.7)
Comprehensive monitoring node example demonstrating:
- MetricsPackage (Type 204) collection and broadcasting
- HealthCheckPackage (Type 605) proactive monitoring (MESH_METRICS)
- CPU usage calculation and tracking
- Memory leak detection with trend analysis
- Network quality assessment
- Problem flag detection and alerting
- Predictive maintenance indicators
- Configurable collection intervals

## Usage

```cpp
#include "alteriom_sensor_package.hpp"
using namespace alteriom;

// Create and send sensor data
SensorPackage sensor;
sensor.temperature = 25.0;
sensor.humidity = 60.0;
// ... set other fields
mesh.sendBroadcast(sensor.toJsonString());

// Handle incoming commands
void handleMessage(String& msg) {
    auto doc = parseJson(msg);
    if (doc["type"] == 201) {
        CommandPackage cmd(doc.as<JsonObject>());
        processCommand(cmd);
    }
}
```

## Building

### Arduino IDE
1. Open any `.ino` file in Arduino IDE
2. Select your board (ESP32 or ESP8266)
3. Install required libraries via Library Manager:
   - AlteriomPainlessMesh
   - ArduinoJson
   - TaskScheduler
   - AsyncTCP (ESP32) or ESPAsyncTCP (ESP8266)
4. Compile and upload

### PlatformIO
Build all examples with PlatformIO:

```bash
# Build for ESP32
cd examples/alteriom
pio run -e esp32

# Build for ESP8266
pio run -e esp8266

# Build for both platforms
pio run
```

The `platformio.ini` file in this directory configures both ESP32 and ESP8266 platforms with all required dependencies.

### Docker (Alteriom Docker Images)
For automated builds using Alteriom's Docker images:

```bash
# Using alteriom-docker-images for PlatformIO builds
docker run --rm -v $(pwd):/project alteriom/platformio:latest \
  pio run -d /project/examples/alteriom -e esp32

# Or use the CI test script
bash ../../test/ci/test_platformio.sh --example alteriom
```

## Testing

Run the Alteriom package tests:
```bash
./bin/catch_alteriom_packages
```

This validates:
- JSON serialization/deserialization
- Package type consistency
- Field preservation
- Edge case handling
- Integration with painlessMesh plugin system