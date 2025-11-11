# Multi-Hop Routing Demo

This example demonstrates the new multi-hop routing capabilities implemented in painlessMesh.

## Features Demonstrated

### 1. Hop Count Calculation (`getHopCount()`)
- Calculates the actual number of hops to reach any node in the mesh
- Returns 0 for self, 1 for direct connections, actual count for multi-hop paths
- Returns -1 if the node is unreachable

### 2. Routing Table (`getRoutingTable()`)
- Returns a complete routing table mapping each destination to its next hop
- For direct connections, destination equals next hop
- For multi-hop paths, shows which neighbor to route through

### 3. Path Discovery (`getPathToNode()`)
- Finds the complete path from this node to any target node
- Returns a vector of node IDs representing the shortest path
- Useful for visualizing mesh topology and debugging connectivity

## How It Works

The implementation uses **Breadth-First Search (BFS)** to:
- Find shortest paths in the mesh topology
- Build accurate routing tables
- Calculate hop counts efficiently

### Algorithm Complexity
- **Time**: O(V + E) where V = nodes, E = edges
- **Space**: O(V) for visited set and queue
- **Suitable for**: ESP8266 (80KB RAM) and ESP32 (320KB RAM)

## Running the Example

### Hardware Requirements
- 2 or more ESP32 or ESP8266 boards
- No additional hardware needed

### Software Requirements
- painlessMesh library (with multi-hop routing support)
- TaskScheduler library
- ArduinoJson library

### Setup Instructions

1. Upload this sketch to multiple ESP devices
2. Each device will automatically join the mesh
3. Open Serial Monitor (115200 baud) on any device
4. Observe routing information every 10 seconds

### Expected Output

```
Routing Demo Started
Node ID: 123456789

=== Routing Information ===
Mesh contains 3 nodes (plus this node)

Hop Counts:
  Node 234567890: 1 hop
  Node 345678901: 2 hops
  Node 456789012: 3 hops

Routing Table (Destination -> Next Hop):
  To 234567890 -> via 234567890 (direct connection)
  To 345678901 -> via 234567890
  To 456789012 -> via 234567890

Path to node 456789012:
  123456789 -> 234567890 -> 345678901 -> 456789012
  (Total: 3 hops)
========================
```

## Use Cases

### Network Diagnostics
- Monitor mesh topology changes in real-time
- Identify connectivity issues
- Measure network depth

### Load Balancing
- Choose optimal paths for data transmission
- Distribute traffic across multiple routes
- Avoid overloading single-hop nodes

### Network Visualization
- Build visual representations of mesh topology
- Display on OLED/LCD screens
- Create web-based network maps

## Performance Considerations

### Memory Usage
- Routing table size: ~8 bytes per node
- BFS temporary storage: ~16 bytes per node
- Example mesh of 50 nodes: ~1.2KB total

### CPU Usage
- Routing calculation: ~1-5ms for typical meshes
- Triggered only on topology changes
- No continuous overhead

## Advanced Usage

### Custom Routing Logic

```cpp
// Find nodes within N hops
std::vector<uint32_t> findNodesWithinRange(int maxHops) {
  std::vector<uint32_t> nearby;
  auto nodeList = mesh.getNodeList(false);
  
  for (auto nodeId : nodeList) {
    int hops = mesh.getHopCount(nodeId);
    if (hops > 0 && hops <= maxHops) {
      nearby.push_back(nodeId);
    }
  }
  
  return nearby;
}
```

### Route Optimization

```cpp
// Select best route based on hop count
uint32_t selectBestRoute(std::vector<uint32_t> candidates) {
  uint32_t best = 0;
  int minHops = 999;
  
  for (auto candidate : candidates) {
    int hops = mesh.getHopCount(candidate);
    if (hops > 0 && hops < minHops) {
      minHops = hops;
      best = candidate;
    }
  }
  
  return best;
}
```

## Troubleshooting

### "All nodes show 2 hops"
- Old firmware without multi-hop support
- Update to latest painlessMesh version

### "Routing table is empty"
- No other nodes in the mesh
- Check mesh credentials (SSID/password)
- Verify mesh is initialized

### "Path discovery returns empty vector"
- Target node is unreachable or left the mesh
- Check connectivity with `getHopCount()` first

## Related Examples

- `namedMesh.ino` - Basic mesh setup
- `diagnosticsExample.ino` - Advanced diagnostics
- `bridge_failover.ino` - Multi-bridge routing

## References

- [painlessMesh Documentation](https://alteriom.github.io/painlessMesh/)
- [Multi-Hop Routing Issue #XXX](https://github.com/Alteriom/painlessMesh/issues/XXX)
- BFS Algorithm: [Wikipedia](https://en.wikipedia.org/wiki/Breadth-first_search)
