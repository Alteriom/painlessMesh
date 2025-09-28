# Mesh Architecture

This document explains how painlessMesh works internally, its design principles, and architectural decisions.

## Overview

painlessMesh creates a self-organizing, self-healing wireless mesh network using ESP8266 and ESP32 devices. The mesh automatically handles:

- **Node Discovery**: Devices find each other automatically
- **Routing**: Messages find optimal paths through the network
- **Topology Management**: Network adapts to nodes joining/leaving
- **Time Synchronization**: All nodes maintain synchronized clocks
- **Connection Management**: Automatic reconnection and load balancing

## Core Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Application Layer                     │
├─────────────────────────────────────────────────────────────┤
│                        Plugin System                        │
├─────────────────────────────────────────────────────────────┤
│                        Mesh Layer                           │
├─────────────────────────────────────────────────────────────┤
│                        Protocol Layer                       │
├─────────────────────────────────────────────────────────────┤
│                        Network Layer (TCP)                  │
├─────────────────────────────────────────────────────────────┤
│                        WiFi Layer (ESP32/ESP8266)           │
└─────────────────────────────────────────────────────────────┘
```

### Layer Responsibilities

**Application Layer**
- User code and application logic
- Custom message handling
- Business logic implementation

**Plugin System**  
- Type-safe message packaging
- Custom package types (sensor data, commands, etc.)
- Message serialization/deserialization

**Mesh Layer**
- Node discovery and connection management
- Mesh topology maintenance
- Time synchronization across nodes

**Protocol Layer**
- Message routing algorithms
- Package type identification
- Connection lifecycle management

**Network Layer**
- TCP connection handling
- Message queuing and transmission
- Connection multiplexing

**WiFi Layer**
- Low-level ESP32/ESP8266 WiFi management
- Access point and station mode handling
- Radio frequency management

## Network Topology

painlessMesh creates a **dynamic tree topology** that automatically reorganizes based on network conditions.

### Tree Structure

```
       Root Node
      /    |    \
   Node1  Node2  Node3
   /  \           |
Node4 Node5    Node6
```

### Key Properties

- **No Single Point of Failure**: If any node fails, the mesh reorganizes
- **Shortest Path Routing**: Messages take optimal routes to destinations
- **Load Distribution**: Connections balanced across available nodes
- **Scalable**: Can handle dozens of nodes (limited by ESP memory)

### Node Roles

**Root Node**
- Acts as the network coordinator
- Maintains network topology information
- Can be any node in the network
- Role automatically transfers if root fails

**Intermediate Nodes**
- Forward messages between other nodes
- Maintain connections to parent and children
- Participate in topology discovery

**Leaf Nodes**
- End devices with minimal connections
- Typically sensor or actuator nodes
- Minimal memory and processing overhead

## Message Routing

painlessMesh implements multiple routing strategies:

### 1. Broadcast Routing
Messages sent to all nodes in the network.

```cpp
// Sender
mesh.sendBroadcast("Hello everyone!");

// All nodes receive the message
void receivedCallback(uint32_t from, String &msg) {
    // Handle broadcast message
}
```

**Algorithm**:
1. Message originates at source node
2. Each node forwards to all connected neighbors (except sender)
3. Duplicate detection prevents loops
4. Message reaches all nodes in network

### 2. Single Node Routing
Messages sent to a specific node by ID.

```cpp
// Send to specific node
mesh.sendSingle(targetNodeId, "Hello specific node!");
```

**Algorithm**:
1. Source looks up target in routing table
2. Message forwarded to next hop toward target
3. Each intermediate node repeats until target reached
4. Automatic route discovery if target unknown

### 3. Neighbor Routing
Messages sent only to directly connected neighbors.

```cpp
// Plugin system example
NeighbourPackage pkg;
mesh.sendPackage(&pkg);
```

**Use Cases**:
- Topology discovery
- Local status updates
- Connection management

## Connection Management

### Connection Lifecycle

```
Disconnected → Discovering → Connecting → Connected → Disconnected
     ↑                                        ↓
     └────────── Connection Lost ←───────────┘
```

### Discovery Process

1. **Scan Phase**: Node scans for available mesh networks
2. **Evaluation Phase**: Evaluates signal strength and load
3. **Connection Phase**: Establishes TCP connection
4. **Handshake Phase**: Exchanges node information
5. **Integration Phase**: Updates routing tables

### Connection Parameters

```cpp
// Maximum connections per node
#define MAX_CONN 4

// Connection timeout
#define CONNECTION_TIMEOUT 30000

// Reconnection interval
#define RECONNECT_INTERVAL 10000
```

### Load Balancing

- Nodes prefer connections with fewer existing connections
- Signal strength influences connection preference
- Automatic connection redistribution when topology changes

## Time Synchronization

painlessMesh maintains synchronized time across all nodes using a distributed algorithm.

### Synchronization Process

1. **Root Election**: Node with most connections becomes time root
2. **Time Distribution**: Root broadcasts its time periodically
3. **Offset Calculation**: Each node calculates offset from root
4. **Propagation**: Time updates propagate through mesh hierarchy

### Time API

```cpp
// Get synchronized mesh time
uint32_t meshTime = mesh.getNodeTime();

// Convert to milliseconds since startup
uint32_t meshTimeMs = meshTime / 1000;

// Time adjustment callback
void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Time adjusted by %d microseconds\n", offset);
}
```

### Use Cases

- **Synchronized Actions**: All nodes can execute actions at same time
- **Data Timestamping**: Consistent timestamps across sensors
- **Event Correlation**: Match events across different nodes
- **Scheduling**: Coordinate scheduled tasks

## Memory Management

### Memory Layout (ESP32)

```
┌─────────────────────┐ 320KB Total RAM
│   Application       │
├─────────────────────┤
│   Mesh Buffers      │ ~50-100KB
├─────────────────────┤
│   TCP Buffers       │ ~20-40KB
├─────────────────────┤
│   WiFi Stack        │ ~30-50KB
├─────────────────────┤
│   System/FreeRTOS   │ ~50-100KB
└─────────────────────┘
```

### Memory Optimization

**Message Queuing**
- Outbound message queue per connection
- Configurable queue sizes
- Automatic message dropping under memory pressure

**Connection Limits**
- Maximum connections limit prevents memory exhaustion
- Dynamic connection management based on available memory

**Buffer Management**
- Shared buffer pools for message processing
- Automatic garbage collection of unused buffers

### ESP8266 Considerations

- **Limited RAM**: ~80KB total, requires careful memory management
- **Fewer Connections**: Typically 2-4 max connections
- **Smaller Buffers**: Reduced message queue sizes
- **Conservative Limits**: More aggressive connection limits

## Error Handling

### Connection Failures

```cpp
void droppedConnectionCallback(uint32_t nodeId) {
    Serial.printf("Lost connection to node %u\n", nodeId);
    // Mesh automatically attempts reconnection
}
```

### Message Delivery

- **Best Effort**: No guaranteed delivery by default
- **Automatic Retry**: Connection-level retries for TCP
- **Route Recovery**: Automatic routing table updates
- **Graceful Degradation**: Network continues with reduced connectivity

### Network Partitions

When the mesh splits into separate networks:

1. **Detection**: Nodes detect missing connections
2. **Recovery Attempts**: Try to reconnect to lost nodes
3. **Partition Handling**: Each partition operates independently
4. **Reunification**: Automatic merge when connectivity restored

## Performance Characteristics

### Throughput

- **Per Connection**: ~100-500 KB/s depending on ESP model
- **Network Wide**: Scales with number of parallel paths
- **Bottlenecks**: Root node can become bottleneck in star topology

### Latency

- **Direct Connection**: 10-50ms typical
- **Multi-hop**: +20-50ms per hop
- **Factors**: Network load, message size, ESP processing speed

### Scalability

- **Node Count**: 10-50 nodes typical (limited by memory and connections)
- **Network Diameter**: 5-7 hops maximum recommended
- **Geographic Range**: 50-200m typical WiFi range

## Design Decisions

### Why Tree Topology?

**Advantages**:
- Simple routing algorithms
- No loops (prevents broadcast storms)
- Efficient bandwidth utilization
- Predictable behavior

**Trade-offs**:
- Single points of failure (mitigated by automatic reorganization)
- Not optimal for all traffic patterns
- Root node may become bottleneck

### Why TCP?

**Advantages**:
- Reliable delivery within connections
- Flow control prevents buffer overflow
- Standard protocol with good tools

**Trade-offs**:
- Higher overhead than UDP
- Connection setup latency
- Memory overhead for connection state

### Why JSON Messages?

**Advantages**:
- Human-readable debugging
- Flexible schema evolution
- Good library support
- Cross-platform compatibility

**Trade-offs**:
- Higher bandwidth usage than binary
- Parsing overhead
- Larger memory footprint

## Security Considerations

### Current Security

- **Network Password**: Simple WPA2 network password
- **No Message Encryption**: Messages sent in plaintext over WiFi
- **No Authentication**: Nodes with correct password can join

### Security Limitations

- Vulnerable to network sniffing
- No node authentication beyond network password
- No message integrity verification
- Susceptible to man-in-the-middle attacks

### Future Enhancements

- Message-level encryption
- Node certificates and authentication
- Perfect forward secrecy
- Secure key distribution

## Next Steps

- Learn about the [Plugin System](plugin-system.md)
- Understand [Message Routing](routing.md) in detail
- Explore [Time Synchronization](time-sync.md) mechanisms
- See [Performance Optimization](../advanced/performance.md) techniques