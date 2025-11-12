# Station Credentials Design Rationale

## Question

Why does `mesh.init()` require a separate `mesh.stationManual()` call to connect to a router, instead of accepting station credentials directly?

## Answer: Multiple Valid Approaches

The library now supports **three approaches** for connecting a bridge node to a router, each with different use cases:

### 1. Separate stationManual() Call (Original Design)

```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6);
mesh.stationManual(STATION_SSID, STATION_PASSWORD);
mesh.setRoot(true);
mesh.setContainsRoot(true);
```

**When to use:**
- Maximum flexibility - can change router connection without reinitializing mesh
- Dynamic router selection at runtime
- Need to call `setHostname()` or other WiFi configuration between init and connection
- Following existing examples or legacy code

**Advantages:**
- Separation of concerns: mesh setup vs router connection
- Can reconnect to different routers without mesh reinitialization
- More control over connection timing and error handling

### 2. Optional Parameters in init() (New Convenience Feature)

```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, 
          WIFI_AP_STA, 6, 0, MAX_CONN,
          STATION_SSID, STATION_PASSWORD);  // Optional parameters
mesh.setRoot(true);
mesh.setContainsRoot(true);
```

**When to use:**
- Simple bridge setup with known credentials
- Static configuration (credentials won't change)
- Want slightly more concise code
- Don't need hostname or other WiFi customization

**Advantages:**
- One line instead of two for basic bridge setup
- All connection parameters in one place
- Still maintains full flexibility of other options

### 3. initAsBridge() Method (Recommended for New Projects)

```cpp
mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                  STATION_SSID, STATION_PASSWORD,
                  &userScheduler, MESH_PORT);
```

**When to use:**
- New bridge implementations (recommended)
- Want automatic channel detection
- Need simplest possible setup
- Following modern best practices

**Advantages:**
- **Automatic channel detection** - no manual channel configuration needed
- Automatically sets node as root
- Maintains router connection through channel switches
- Broadcasts bridge status (Type 610) automatically
- Comprehensive initialization in one call

## Design Rationale for Original Separation

The original design separated `init()` and `stationManual()` for good architectural reasons:

### 1. Separation of Concerns

**Mesh Setup (`init()`):**
- Creates mesh network (AP mode)
- Sets up mesh routing and protocol
- Configures mesh-specific parameters
- Lifetime: typically never changes

**Router Connection (`stationManual()`):**
- Connects to external WiFi (STA mode)
- Different lifecycle - may connect/disconnect/change
- Network-specific credentials and settings
- Can be reconfigured at runtime

This separation allows clean code organization and different lifecycles for each concern.

### 2. Not All Nodes Need Router Connection

In a typical mesh network:
- **1 bridge node**: Needs router connection (AP+STA mode)
- **N regular nodes**: Mesh only (AP mode, or AP+STA for mesh connections)

Regular nodes use:
```cpp
mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA);
// No stationManual() call - not a bridge
```

If `init()` always required station credentials, it would be confusing for regular nodes.

### 3. Dynamic Router Switching

Some advanced use cases require changing router connections at runtime:

```cpp
// Initial setup
mesh.init(...);
mesh.stationManual("Router1", "pass1");

// Later, switch to different router
mesh.stationManual("Router2", "pass2");

// Or respond to failover
void onRouterDisconnect() {
    mesh.stationManual(backupSSID, backupPassword);
}
```

With station credentials baked into `init()`, this flexibility would be lost.

### 4. Additional WiFi Configuration

Many users need to configure WiFi settings between initialization and connection:

```cpp
mesh.init(...);
mesh.setHostname("MESH_BRIDGE");  // Must be before stationManual()
mesh.stationManual(...);
```

The separation provides a natural place for these configurations.

### 5. Error Handling and Retry Logic

Separating the calls allows better error handling:

```cpp
mesh.init(...);  // This typically doesn't fail

// Retry router connection with backoff
for (int retry = 0; retry < 3; retry++) {
    if (tryStationConnect()) break;
    delay(1000 * (retry + 1));
}
```

## Comparison Table

| Approach | Setup Complexity | Flexibility | Channel Detection | Best For |
|----------|-----------------|-------------|-------------------|----------|
| **stationManual()** | Medium | Highest | Manual | Dynamic configs, legacy code |
| **init() params** | Low-Medium | High | Manual | Simple static bridges |
| **initAsBridge()** | Lowest | Medium | Automatic | New projects, recommended |

## Recommendation

**For new projects:** Use `initAsBridge()` - it's the modern, recommended approach with automatic channel detection.

**For existing projects:** The original `init()` + `stationManual()` pattern remains fully supported and appropriate.

**For simple bridges:** The new optional parameters in `init()` provide a middle ground with good flexibility.

All three approaches are valid and will continue to be supported. Choose based on your specific needs.

## Implementation Note

When station credentials are passed to `init()`, the implementation internally calls `stationManual()` after mesh initialization. This maintains consistency and code reuse while providing convenience.

```cpp
// Inside init() implementation
if (!stationSSID.empty() && (connectMode & WIFI_STA)) {
    this->stationManual(stationSSID, stationPassword);
}
```

This design ensures all three approaches use the same underlying connection logic.
