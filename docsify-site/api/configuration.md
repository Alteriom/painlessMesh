# API Configuration

painlessMesh provides extensive configuration options to customize mesh behavior for your specific use case. These settings control network parameters, debug output, timing, and advanced features.

## Basic Configuration

### mesh.init()

Initialize the mesh with basic parameters.

```cpp
void mesh.init(String ssid, String password, Scheduler* userScheduler, 
               uint16_t port = 5555, 
               WiFiEventCallback connectCallback = NULL,
               uint8_t channel = 1)
```

**Parameters:**

- `ssid` - Mesh network name (prefix for AP names)
- `password` - Network password for security
- `userScheduler` - Pointer to your task scheduler
- `port` - TCP port for mesh communication (default: 5555)
- `connectCallback` - Optional WiFi event callback
- `channel` - WiFi channel to use (1-13)

**Example:**

```cpp
#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "SecurePassword123"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}
```

### Advanced Initialization

```cpp
void setup() {
    // Custom channel and callback
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, 
              MESH_PORT, wifiEventCallback, 6);
              
    // Additional configuration
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.setContainsRoot(true);
    mesh.setAP("MyMesh", "password", 6, false, 4);
}

void wifiEventCallback(WiFiEvent_t event) {
    switch(event) {
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("Station connected to AP");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("Station disconnected from AP");
            break;
    }
}
```

## Debug Configuration

### setDebugMsgTypes()

Control which debug messages are printed to Serial.

```cpp
void mesh.setDebugMsgTypes(uint16_t types)
```

**Message Types:**

- `ERROR` - Error messages only
- `STARTUP` - Initialization messages
- `CONNECTION` - Connection/disconnection events
- `SYNC` - Time synchronization messages
- `COMMUNICATION` - Message sending/receiving
- `GENERAL` - General information
- `MSG_TYPES` - Message type information
- `REMOTE` - Remote debugging messages

**Examples:**

```cpp
// Only errors and startup messages
mesh.setDebugMsgTypes(ERROR | STARTUP);

// All messages (verbose debugging)
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | SYNC | 
                      COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);

// No debug output (production)
mesh.setDebugMsgTypes(0);

// Custom combination for development
mesh.setDebugMsgTypes(ERROR | CONNECTION | COMMUNICATION);
```

### logLevel()

Set the logging level for more granular control.

```cpp
void mesh.logLevel(painlessmesh::logger::LogLevel level)
```

**Log Levels:**

- `ERROR_LEVEL` - Errors only
- `INFO_LEVEL` - Informational messages
- `DEBUG_LEVEL` - Debug information
- `VERBOSE_LEVEL` - Verbose output

```cpp
// Set to info level
mesh.logLevel(painlessmesh::logger::INFO_LEVEL);

// Enable verbose logging for troubleshooting
mesh.logLevel(painlessmesh::logger::VERBOSE_LEVEL);
```

## Network Configuration

### setContainsRoot()

Configure whether this mesh contains a root node (bridge to external network).

```cpp
void mesh.setContainsRoot(bool containsRoot = false)
```

**Parameters:**

- `containsRoot` - true if mesh has a bridge/root node

```cpp
// Enable if you have a bridge node to WiFi/Internet
mesh.setContainsRoot(true);

// Disable for standalone mesh (default)
mesh.setContainsRoot(false);
```

### setRoot()

Set this node as the root node (bridge).

```cpp
void mesh.setRoot(bool shouldRoot = true)
```

**Example - Bridge Node:**

```cpp
void setup() {
    // Configure as bridge node
    mesh.setContainsRoot(true);
    mesh.setRoot(true);
    
    // Connect to external WiFi
    mesh.stationManual("HomeWiFi", "password");
    
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
}
```

### setAP()

Configure Access Point parameters.

```cpp
void mesh.setAP(String ssid, String password = "", 
                uint8_t channel = 1, bool hidden = false, 
                uint8_t maxConnection = 4)
```

**Parameters:**

- `ssid` - AP network name
- `password` - AP password (empty for open network)
- `channel` - WiFi channel (1-13)
- `hidden` - Hide SSID from broadcast
- `maxConnection` - Maximum concurrent connections (1-4 on ESP8266, 1-10 on ESP32)

**Examples:**

```cpp
// Open network on channel 6, max 8 connections
mesh.setAP("OpenMesh", "", 6, false, 8);

// Secure hidden network
mesh.setAP("SecretMesh", "secret123", 11, true, 2);

// High capacity mesh (ESP32 only)
mesh.setAP("LargeMesh", "password", 1, false, 10);
```

### stationManual()

Manually configure station mode connection (for bridge nodes).

```cpp
void mesh.stationManual(String ssid, String password, 
                        uint16_t port = 0, IPAddress remote_ip = IPAddress())
```

**Parameters:**

- `ssid` - External WiFi network name
- `password` - External WiFi password
- `port` - Port for external connection
- `remote_ip` - Specific IP to connect to

**Example:**

```cpp
// Connect to home WiFi as bridge
mesh.stationManual("HomeWiFi", "wifipassword");

// Connect to specific server
mesh.stationManual("CorpWiFi", "password", 8080, IPAddress(192,168,1,100));
```

## Timing Configuration

### setNodeSyncTime()

Configure time synchronization interval.

```cpp
void mesh.setNodeSyncTime(uint32_t time_us)
```

**Parameters:**

- `time_us` - Sync interval in microseconds

```cpp
// Sync every 5 seconds (default)
mesh.setNodeSyncTime(5 * 1000000);

// More frequent sync for time-critical applications
mesh.setNodeSyncTime(1 * 1000000);

// Less frequent sync to reduce network traffic
mesh.setNodeSyncTime(30 * 1000000);
```

### setNodeTimeout()

Set node timeout for connection management.

```cpp
void mesh.setNodeTimeout(uint32_t timeout_us)
```

**Parameters:**

- `timeout_us` - Timeout in microseconds

```cpp
// 10 second timeout (default)
mesh.setNodeTimeout(10 * 1000000);

// Shorter timeout for fast networks
mesh.setNodeTimeout(5 * 1000000);

// Longer timeout for unreliable connections
mesh.setNodeTimeout(30 * 1000000);
```

## Message Configuration

### Package Registration

Register custom package types for type-safe messaging.

```cpp
template<typename T>
void mesh.registerPackage()
```

**Example:**

```cpp
// Register custom package types
mesh.registerPackage<SensorPackage>();
mesh.registerPackage<CommandPackage>();
mesh.registerPackage<StatusPackage>();
```

### Message Limits

Configure message size and queue limits.

```cpp
// Set maximum message size (bytes)
mesh.setMaxMessageSize(1024);

// Set message queue size
mesh.setMessageQueueSize(50);

// Set retry attempts for failed messages
mesh.setRetryAttempts(3);
```

## Performance Configuration

### Connection Limits

Configure connection behavior for different scenarios.

```cpp
void setup() {
    // High-density network (more connections)
    mesh.setAP("DenseMesh", "password", 1, false, 10); // ESP32 only
    mesh.setNodeTimeout(5 * 1000000); // Faster timeout
    mesh.setNodeSyncTime(1 * 1000000); // Frequent sync
    
    // Low-power network (fewer connections, longer intervals)
    mesh.setAP("PowerMesh", "password", 1, false, 2);
    mesh.setNodeTimeout(60 * 1000000); // Longer timeout
    mesh.setNodeSyncTime(30 * 1000000); // Less frequent sync
}
```

### Memory Management

Configure memory usage for resource-constrained devices.

```cpp
void setup() {
    // Reduce memory usage on ESP8266
    mesh.setMaxMessageSize(512);        // Smaller messages
    mesh.setMessageQueueSize(20);       // Smaller queue
    mesh.setAP("LiteMesh", "pass", 1, false, 2); // Fewer connections
    
    // ESP32 can handle larger configurations
    mesh.setMaxMessageSize(2048);       // Larger messages
    mesh.setMessageQueueSize(100);      // Larger queue
    mesh.setAP("FullMesh", "pass", 1, false, 8); // More connections
}
```

## Security Configuration

### Encryption

Configure mesh security settings.

```cpp
void setup() {
    // Strong password for mesh security
    String strongPassword = "VerySecurePassword2023!";
    mesh.init(MESH_PREFIX, strongPassword, &userScheduler, MESH_PORT);
    
    // Hidden network for additional security
    mesh.setAP("SecureMesh", strongPassword, 6, true, 4);
}
```

### Access Control

Implement node authentication and authorization.

```cpp
bool authorizedNodes[] = {123456789, 987654321, 555444333};

mesh.onReceive([](uint32_t from, String& msg) {
    // Check if node is authorized
    bool authorized = false;
    for (uint32_t nodeId : authorizedNodes) {
        if (from == nodeId) {
            authorized = true;
            break;
        }
    }
    
    if (!authorized) {
        Serial.printf("Unauthorized message from node %u\n", from);
        return;
    }
    
    // Process authorized message
    processMessage(from, msg);
});
```

## Environment-Specific Configuration

### Indoor Environment

Optimized for indoor use with good WiFi coverage.

```cpp
void configureIndoor() {
    mesh.setAP("IndoorMesh", "password", 6, false, 6);
    mesh.setNodeTimeout(10 * 1000000);    // 10 second timeout
    mesh.setNodeSyncTime(5 * 1000000);     // 5 second sync
    mesh.setDebugMsgTypes(ERROR | CONNECTION);
}
```

### Outdoor Environment

Configured for outdoor use with potentially unreliable connections.

```cpp
void configureOutdoor() {
    mesh.setAP("OutdoorMesh", "password", 1, false, 4);
    mesh.setNodeTimeout(30 * 1000000);     // 30 second timeout
    mesh.setNodeSyncTime(15 * 1000000);    // 15 second sync
    mesh.setRetryAttempts(5);               // More retries
    mesh.setDebugMsgTypes(ERROR | CONNECTION | SYNC);
}
```

### Industrial Environment

Heavy-duty configuration for industrial applications.

```cpp
void configureIndustrial() {
    mesh.setAP("IndustrialMesh", "SecureIndustrial2023", 11, true, 8);
    mesh.setNodeTimeout(60 * 1000000);     // 60 second timeout
    mesh.setNodeSyncTime(10 * 1000000);    // 10 second sync
    mesh.setMaxMessageSize(2048);          // Larger messages
    mesh.setRetryAttempts(3);
    mesh.setDebugMsgTypes(ERROR | STARTUP);
}
```

### Low Power Configuration

Optimized for battery-powered devices.

```cpp
void configureLowPower() {
    mesh.setAP("PowerMesh", "password", 1, false, 2);
    mesh.setNodeTimeout(120 * 1000000);    // 2 minute timeout
    mesh.setNodeSyncTime(60 * 1000000);    // 1 minute sync
    mesh.setMaxMessageSize(256);           // Small messages
    mesh.setMessageQueueSize(10);          // Small queue
    mesh.setDebugMsgTypes(ERROR);          // Minimal logging
}
```

## Dynamic Configuration

### Runtime Configuration Changes

Modify configuration during runtime based on conditions.

```cpp
void adaptiveConfiguration() {
    int networkSize = mesh.getNodeList().size() + 1;
    int freeHeap = ESP.getFreeHeap();
    
    // Adjust based on network size
    if (networkSize > 10) {
        mesh.setNodeSyncTime(15 * 1000000);  // Reduce sync frequency
        mesh.setMaxMessageSize(512);         // Smaller messages
    } else {
        mesh.setNodeSyncTime(5 * 1000000);   // Normal sync
        mesh.setMaxMessageSize(1024);        // Normal messages
    }
    
    // Adjust based on available memory
    if (freeHeap < 50000) {
        mesh.setMessageQueueSize(10);        // Reduce queue
        mesh.setDebugMsgTypes(ERROR);        // Minimal debug
    } else {
        mesh.setMessageQueueSize(50);        // Normal queue
        mesh.setDebugMsgTypes(ERROR | CONNECTION);
    }
}

// Call periodically to adjust configuration
Task adaptiveTask(60000, TASK_FOREVER, adaptiveConfiguration);
```

### Configuration Profiles

Switch between predefined configuration profiles.

```cpp
enum ConfigProfile { DEVELOPMENT, PRODUCTION, LOW_POWER, HIGH_PERFORMANCE };

void applyConfigProfile(ConfigProfile profile) {
    switch(profile) {
        case DEVELOPMENT:
            mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | COMMUNICATION);
            mesh.setNodeTimeout(5 * 1000000);
            mesh.setNodeSyncTime(1 * 1000000);
            break;
            
        case PRODUCTION:
            mesh.setDebugMsgTypes(ERROR);
            mesh.setNodeTimeout(30 * 1000000);
            mesh.setNodeSyncTime(10 * 1000000);
            break;
            
        case LOW_POWER:
            configureLowPower();
            break;
            
        case HIGH_PERFORMANCE:
            mesh.setAP("FastMesh", "password", 1, false, 10);
            mesh.setNodeTimeout(5 * 1000000);
            mesh.setNodeSyncTime(1 * 1000000);
            mesh.setMaxMessageSize(2048);
            break;
    }
}
```

## Configuration Validation

### Verify Settings

Check configuration validity and system capabilities.

```cpp
bool validateConfiguration() {
    // Check memory availability
    if (ESP.getFreeHeap() < 30000) {
        Serial.println("WARNING: Low memory for current configuration");
        return false;
    }
    
    // Validate node timeout vs sync time
    uint32_t timeout = mesh.getNodeTimeout();
    uint32_t syncTime = mesh.getNodeSyncTime();
    
    if (timeout < syncTime * 2) {
        Serial.println("WARNING: Node timeout too short relative to sync time");
        return false;
    }
    
    // Check connection limits for platform
    #ifdef ESP8266
    if (mesh.getMaxConnections() > 4) {
        Serial.println("WARNING: ESP8266 supports max 4 connections");
        return false;
    }
    #endif
    
    return true;
}
```

See also:

- [Core API](core-api.md) - Complete method reference
- [Callbacks](callbacks.md) - Event handling
- [Performance Tuning](../advanced/performance.md) - Optimization guidelines