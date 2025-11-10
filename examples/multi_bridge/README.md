# Multi-Bridge Example

This example demonstrates advanced multi-bridge coordination for high-availability mesh networks with multiple simultaneous Internet gateways.

## Overview

In a multi-bridge deployment, multiple nodes act as bridges to the Internet simultaneously. This provides:

- **High Availability**: Zero downtime during failover
- **Load Balancing**: Distribute traffic across multiple uplinks
- **Geographic Distribution**: Bridges in different locations
- **Redundancy**: Multiple paths to Internet

## Architecture

```
Internet Connection A → Primary Bridge (Priority 10)
                              ↓
                        Mesh Network
                              ↓
Internet Connection B → Secondary Bridge (Priority 5)
                              ↓
                        Regular Nodes
```

## Files

- **primary_bridge.ino** - Primary bridge with highest priority (10)
- **secondary_bridge.ino** - Secondary bridge for redundancy (priority 5)
- **regular_node.ino** - Regular mesh node that uses bridges

## How It Works

### Bridge Priority

Bridges are assigned priorities (1-10):
- **10 = Primary**: Handles all traffic when available
- **5 = Secondary**: Hot standby, takes over if primary fails
- **1 = Standby**: Only used if all higher priority bridges fail

### Bridge Selection Strategies

1. **PRIORITY_BASED** (default): Always use highest priority available bridge
2. **ROUND_ROBIN**: Distribute load evenly across all bridges
3. **BEST_SIGNAL**: Use bridge with best WiFi signal strength

### Coordination Protocol

Bridges exchange coordination messages (Type 613) containing:
- Priority level
- Current role (primary/secondary/standby)
- Load percentage
- List of known peer bridges

Regular nodes track all bridges and automatically select the best one based on the configured strategy.

## Setup Instructions

### Hardware Required

- 3+ ESP32 or ESP8266 devices
- 1-2 WiFi routers with Internet connection
- USB cables for programming

### Configuration

#### Primary Bridge
1. Flash `primary_bridge.ino` to first ESP32
2. Configure router credentials:
   ```cpp
   #define ROUTER_SSID     "YourPrimaryRouter"
   #define ROUTER_PASSWORD "routerpass"
   ```
3. Priority is set to 10 (highest)

#### Secondary Bridge
1. Flash `secondary_bridge.ino` to second ESP32
2. Configure router credentials (can be same or different router):
   ```cpp
   #define ROUTER_SSID     "YourSecondaryRouter"
   #define ROUTER_PASSWORD "routerpass2"
   ```
3. Priority is set to 5 (secondary)

#### Regular Nodes
1. Flash `regular_node.ino` to remaining ESP32 devices
2. No router credentials needed
3. They automatically discover and use bridges

### Testing

#### Normal Operation
1. Power on primary bridge - should connect to router
2. Power on secondary bridge - should connect to router
3. Power on regular nodes - should join mesh
4. Regular nodes should prefer primary bridge (priority 10)

#### Failover Test
1. Disconnect primary bridge from power
2. Regular nodes should detect loss within 60 seconds
3. Regular nodes should automatically switch to secondary bridge
4. No data loss or interruption

#### Load Balancing Test (Round-Robin)
1. On regular nodes, change strategy:
   ```cpp
   mesh.setBridgeSelectionStrategy(mesh.ROUND_ROBIN);
   ```
2. Messages should alternate between bridges

## Bridge Selection Strategies

### Priority-Based (Default)

Best for most deployments. Uses highest priority bridge available.

```cpp
mesh.setBridgeSelectionStrategy(mesh.PRIORITY_BASED);
```

**When to use:**
- Production systems needing predictable routing
- When you have primary and backup Internet connections
- Clear preference for one connection over another

### Round-Robin Load Balancing

Distributes traffic evenly across all bridges.

```cpp
mesh.setBridgeSelectionStrategy(mesh.ROUND_ROBIN);
```

**When to use:**
- High traffic scenarios
- Multiple equal-quality Internet connections
- Load distribution is important

### Best Signal

Always uses bridge with strongest WiFi signal.

```cpp
mesh.setBridgeSelectionStrategy(mesh.BEST_SIGNAL);
```

**When to use:**
- Large mesh networks
- Nodes move around
- Signal strength affects performance significantly

## API Reference

### Multi-Bridge Configuration

```cpp
// Enable multi-bridge mode
mesh.enableMultiBridge(true);

// Set selection strategy
mesh.setBridgeSelectionStrategy(mesh.PRIORITY_BASED);
mesh.setBridgeSelectionStrategy(mesh.ROUND_ROBIN);
mesh.setBridgeSelectionStrategy(mesh.BEST_SIGNAL);

// Set max concurrent bridges (default: 2, max: 5)
mesh.setMaxBridges(3);
```

### Bridge Initialization

```cpp
// Initialize as bridge with priority
mesh.initAsBridge(meshSSID, meshPassword,
                  routerSSID, routerPassword,
                  &scheduler, port, priority);
```

### Bridge Discovery

```cpp
// Get list of active bridge node IDs
std::vector<uint32_t> bridges = mesh.getActiveBridges();

// Get recommended bridge for next message
uint32_t bridgeId = mesh.getRecommendedBridge();

// Manually select bridge for next transmission
mesh.selectBridge(bridgeId);

// Check if multi-bridge is enabled
bool enabled = mesh.isMultiBridgeEnabled();
```

## Expected Output

### Primary Bridge
```
=== Multi-Bridge: PRIMARY BRIDGE ===

=== Bridge Mode Initialization (Priority: 10, Role: primary) ===
✓ Router connected on channel 6
✓ Router IP: 192.168.1.100
Bridge coordination enabled (priority: 10, role: primary)

=== Primary Bridge Status ===
Node ID: 123456
Connected Nodes: 3
Active Bridges: 2
  - Bridge: 123456 (ME)
  - Bridge: 789012
Recommended Bridge: 123456
```

### Secondary Bridge
```
=== Multi-Bridge: SECONDARY BRIDGE ===

=== Bridge Mode Initialization (Priority: 5, Role: secondary) ===
✓ Router connected on channel 6
✓ Router IP: 192.168.1.101
Bridge coordination enabled (priority: 5, role: secondary)

=== Secondary Bridge Status ===
Node ID: 789012
Connected Nodes: 3
Active Bridges: 2
  - Bridge: 123456
  - Bridge: 789012 (ME)
Recommended Bridge: 123456
✓ Standby mode - primary bridge is active
```

### Regular Node
```
=== Multi-Bridge: REGULAR NODE ===

=== Network Status ===
Node ID: 456789
Connected Nodes: 4
Active Bridges: 2
  - Bridge: 123456
  - Bridge: 789012
Internet Available: YES

--- Sending Sensor Data ---
Recommended Bridge: 123456
Message: Temperature: 22.3°C, Humidity: 45.2%
✓ Sent to bridge
```

## Troubleshooting

### No Bridges Found

**Symptoms**: `Active Bridges: 0`

**Solutions**:
- Verify bridge nodes are powered on
- Check router credentials are correct
- Ensure bridges successfully connected to router
- Check all devices use same MESH_PREFIX and MESH_PASSWORD

### Bridges Not Coordinating

**Symptoms**: Bridges don't see each other

**Solutions**:
- Verify `enableMultiBridge(true)` is called on all bridges
- Check mesh connectivity between bridges
- Look for "Bridge coordination" messages in logs
- Ensure bridges are on same mesh network

### Failover Not Working

**Symptoms**: No automatic switch to secondary

**Solutions**:
- Check `mesh.onBridgeStatusChanged()` callback is registered
- Verify secondary bridge has Internet connection
- Ensure bridge status broadcasts are enabled
- Check 60-second timeout hasn't been exceeded

### Wrong Bridge Selected

**Symptoms**: Secondary used instead of primary

**Solutions**:
- Verify priorities are correct (primary=10, secondary=5)
- Check strategy is PRIORITY_BASED
- Ensure primary bridge has Internet connection
- Look for priority values in coordination messages

## Advanced Configuration

### Three or More Bridges

```cpp
// Bridge 1: Primary (priority 10)
mesh.initAsBridge(ssid, pass, router1, pass1, &sched, port, 10);

// Bridge 2: Secondary (priority 7)
mesh.initAsBridge(ssid, pass, router2, pass2, &sched, port, 7);

// Bridge 3: Tertiary (priority 3)
mesh.initAsBridge(ssid, pass, router3, pass3, &sched, port, 3);
```

### Geographic Distribution

Use when mesh spans multiple buildings:

```
Building A: Bridge1 (priority 10) → Internet A
Building B: Bridge2 (priority 10) → Internet B
```

Both bridges have same priority, nodes use closest one (BEST_SIGNAL strategy).

### Traffic Shaping (Future)

Route different message types to different bridges:

```cpp
// Coming in future release
mesh.setBridgeSelectionStrategy(mesh.TRAFFIC_TYPE);
mesh.routeTrafficType(ALARM_MESSAGE, bridge1);
mesh.routeTrafficType(SENSOR_DATA, bridge2);
```

## Dependencies

- Issue #63: Bridge Status Broadcast ✅ (IMPLEMENTED)
- Issue #64: Bridge Failover ✅ (IMPLEMENTED)
- Issue #65: Multi-Bridge Coordination ✅ (THIS FEATURE)

## Related Examples

- `examples/bridge/` - Basic single bridge setup
- `examples/bridge_failover/` - Single bridge with automatic failover
- `examples/bridgeAwareSensorNode/` - Node that checks bridge status

## Further Reading

- [Bridge Architecture Documentation](../../BRIDGE_TO_INTERNET.md)
- [Bridge Health Monitoring](../../BRIDGE_HEALTH_MONITORING_IMPLEMENTATION.md)
- [painlessMesh Wiki](https://gitlab.com/painlessMesh/painlessMesh/-/wikis/home)
